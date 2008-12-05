/* $Id: demuxmpeg.c,v 1.4 2004/10/19 23:11:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

static inline void check_mpeg_scr( hb_psdemux_t *state, int64_t scr, int tol )
{
    /*
     * This section of code implements the timing model of
     * the "Standard Target Decoder" (STD) of the MPEG2 standard
     * (specified in ISO 13818-1 sections 2.4.2, 2.5.2 & Annex D).
     * The STD removes and corrects for clock discontinuities so
     * that the time stamps on the video, audio & other media
     * streams can be used for cross-media synchronization. To do
     * this the STD has its own timestamp value, the System Clock
     * Reference or SCR, in the PACK header. Clock discontinuities
     * are detected using the SCR & and the adjustment needed
     * to correct post-discontinuity timestamps to be contiguous
     * with pre-discontinuity timestamps is computed from pre- and
     * post-discontinuity values of the SCR. Then this adjustment
     * is applied to every media timestamp (PTS).
     *
     * ISO 13818-1 says there must be an SCR at least every 700ms
     * (100ms for Transport Streams) so if the difference between
     * this SCR & the previous is >700ms it's a discontinuity.
     * If the difference is negative it's non-physical (time doesn't
     * go backward) and must also be a discontinuity. When we find a
     * discontinuity we adjust the scr_offset so that the SCR of the
     * new packet lines up with that of the previous packet.
     */

    // we declare a discontinuity if there's a gap of more than
    // 'tol'ms between the last scr & this or if this scr goes back
    // by more than half a frame time.
    int64_t scr_delta = scr - state->last_scr;
    if ( scr_delta > 90*tol || scr_delta < -90*10 )
    {
        ++state->scr_changes;
        state->last_pts = -1;
    }
    state->last_scr = scr;
}

/* Basic MPEG demuxer */

int hb_demux_ps( hb_buffer_t * buf_ps, hb_list_t * list_es, hb_psdemux_t* state )
{
    hb_buffer_t * buf_es;
    int           pos = 0;

#define d (buf_ps->data)

    /* pack_header */
    if( d[pos] != 0 || d[pos+1] != 0 ||
        d[pos+2] != 0x1 || d[pos+3] != 0xBA )
    {
        hb_log( "hb_demux_ps: not a PS packet (%02x%02x%02x%02x)",
                d[pos], d[pos+1], d[pos+2], d[pos+3] );
        return 0;
    }
    pos += 4;                    /* pack_start_code */

    if ( state )
    {
        /* extract the system clock reference (scr) */
        int64_t scr = ((uint64_t)(d[pos] & 0x38) << 27) |
                      ((uint64_t)(d[pos] & 0x03) << 28) |
                      ((uint64_t)(d[pos+1]) << 20) |
                      ((uint64_t)(d[pos+2] >> 3) << 15) |
                      ((uint64_t)(d[pos+2] & 3) << 13) |
                      ((uint64_t)(d[pos+3]) << 5) |
                      (d[pos+4] >> 3);
        check_mpeg_scr( state, scr, 100 );
    }

    pos += 9;                    /* pack_header */
    pos += 1 + ( d[pos] & 0x7 ); /* stuffing bytes */

    /* system_header */
    if( d[pos] == 0 && d[pos+1] == 0 &&
        d[pos+2] == 0x1 && d[pos+3] == 0xBB )
    {
        int header_length;

        pos           += 4; /* system_header_start_code */
        header_length  = ( d[pos] << 8 ) + d[pos+1];
        pos           += 2 + header_length;
    }

    /* pes */
    while( pos + 6 < buf_ps->size &&
           d[pos] == 0 && d[pos+1] == 0 && d[pos+2] == 0x1 )
    {
        int      id;
        int      pes_packet_length;
        int      pes_packet_end;
        int      pes_header_d_length;
        int      pes_header_end;
        int      has_pts;
        int64_t  pts = -1, dts = -1;

        pos               += 3;               /* packet_start_code_prefix */
        id           = d[pos];
        pos               += 1;

        pes_packet_length  = ( d[pos] << 8 ) + d[pos+1];
        pos               += 2;               /* pes_packet_length */
        pes_packet_end     = pos + pes_packet_length;

        if( id != 0xE0 && id != 0xBD &&
            ( id & 0xC0 ) != 0xC0  )
        {
            /* Not interesting */
            pos = pes_packet_end;
            continue;
        }

        has_pts            = d[pos+1] >> 6;
        pos               += 2;               /* Required headers */

        pes_header_d_length  = d[pos];
        pos                    += 1;
        pes_header_end          = pos + pes_header_d_length;

        if( has_pts )
        {
            pts = ( (uint64_t)(d[pos] & 0xe ) << 29 ) +
                  ( d[pos+1] << 22 ) +
                  ( ( d[pos+2] >> 1 ) << 15 ) +
                  ( d[pos+3] << 7 ) +
                  ( d[pos+4] >> 1 );
            if ( has_pts & 1 )
            {
                dts = ( (uint64_t)(d[pos+5] & 0xe ) << 29 ) +
                      ( d[pos+6] << 22 ) +
                      ( ( d[pos+7] >> 1 ) << 15 ) +
                      ( d[pos+8] << 7 ) +
                      ( d[pos+9] >> 1 );
            }
            else
            {
                dts = pts;
            }
        }

        pos = pes_header_end;

        if( id == 0xBD )
        {
            id |= ( d[pos] << 8 );
            if( ( id & 0xF0FF ) == 0x80BD ) /* A52 */
            {
                pos += 4;
            }
            else if( ( id & 0xE0FF ) == 0x20BD || /* SPU */
                     ( id & 0xF0FF ) == 0xA0BD )  /* LPCM */
            {
                pos += 1;
            }
        }

        /* Sanity check */
        if( pos >= pes_packet_end )
        {
            pos = pes_packet_end;
            continue;
        }

        /* Here we hit we ES payload */
        buf_es = hb_buffer_init( pes_packet_end - pos );

        buf_es->id       = id;
        buf_es->start    = pts;
        buf_es->renderOffset = dts;
        buf_es->stop     = -1;
        if (id == 0xE0) {
            // Consume a chapter break, and apply it to the ES.
            buf_es->new_chap = buf_ps->new_chap;
            buf_ps->new_chap = 0;
        }
        memcpy( buf_es->data, d + pos, pes_packet_end - pos );

        hb_list_add( list_es, buf_es );

        pos = pes_packet_end;
    }

#undef d

    return 1;
}

// mpeg transport stream demuxer. the elementary stream headers have been
// stripped off and buf_ps has all the info gleaned from them: id is set,
// start contains the pts (if any), renderOffset contains the dts (if any)
// and stop contains the pcr (if it changed).
int hb_demux_ts( hb_buffer_t *buf_ps, hb_list_t *list_es, hb_psdemux_t *state )
{
    if ( state )
    {
        // we're keeping track of timing (i.e., not in scan)
        // check if there's a new pcr in this packet
        if ( buf_ps->stop >= 0 )
        {
            // we have a new pcr
            check_mpeg_scr( state, buf_ps->stop, 300 );
            buf_ps->stop = -1;
        }
        if ( buf_ps->start >= 0 )
        {
            // Program streams have an SCR in every PACK header so they
            // can't lose their clock reference. But the PCR in Transport
            // streams is typically on <.1% of the packets. If a PCR
            // packet gets lost and it marks a clock discontinuity then
            // the data following it will be referenced to the wrong
            // clock & introduce huge gaps or throw our A/V sync off.
            // We try to protect against that here by sanity checking
            // timestamps against the current reference clock and discarding
            // packets where the DTS is "too far" from its clock.
            int64_t fdelta = buf_ps->start - state->last_scr;
            if ( fdelta < -300 * 90000LL || fdelta > 300 * 90000LL )
            {
                // packet too far behind or ahead of its clock reference
                ++state->dts_drops;
                return 1;
            }
            if ( state->last_pts >= 0 )
            {
                fdelta = buf_ps->start - state->last_pts;
                if ( fdelta < -5 * 90000LL || fdelta > 5 * 90000LL )
                {
                    // Packet too far from last. This may be a NZ TV broadcast
                    // as they like to change the PCR without sending a PCR
                    // update. Since it may be a while until they actually tell
                    // us the new PCR use the PTS as the PCR.
                    ++state->scr_changes;
                    state->last_scr = buf_ps->start;
                }
            }
            state->last_pts = buf_ps->start;
        }
    }

    hb_buffer_t *buf = hb_buffer_init( buf_ps->alloc );
    hb_buffer_swap_copy( buf_ps, buf );
    hb_list_add( list_es, buf );

    return 1;
}

// "null" demuxer (makes a copy of input buf & returns it in list)
// used when the reader for some format includes its own demuxer.
// for example, ffmpeg.
int hb_demux_null( hb_buffer_t * buf_ps, hb_list_t * list_es, hb_psdemux_t* state )
{
    // if we don't have a time offset yet, use this timestamp as the offset.
    if ( state && state->scr_changes == 0 &&
         ( buf_ps->start >= 0 || buf_ps->renderOffset >= 0 ) )
    {
        ++state->scr_changes;
        state->last_scr = buf_ps->start >= 0 ? buf_ps->start : buf_ps->renderOffset;
    }

    hb_buffer_t *buf = hb_buffer_init( buf_ps->alloc );
    hb_buffer_swap_copy( buf_ps, buf );
    hb_list_add( list_es, buf );

    return 1;
}

const hb_muxer_t hb_demux[] = { hb_demux_ps, hb_demux_ts, hb_demux_null };
