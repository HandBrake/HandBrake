/* demuxmpeg.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "libavutil/avutil.h"
#include "handbrake/handbrake.h"

static inline int check_mpeg_scr( hb_psdemux_t *state, int64_t scr, int tol )
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
    int discontinuity = 0;
    int64_t scr_delta = scr - state->last_scr;
    if (state->last_scr == AV_NOPTS_VALUE ||
        scr_delta > 90*tol || scr_delta < -90*10)
    {
        ++state->scr_changes;
        state->last_pts = AV_NOPTS_VALUE;
        discontinuity = 1;
    }
    state->last_scr = scr;
    return discontinuity;
}

static inline void save_chap( hb_psdemux_t *state, hb_buffer_t *buf )
{
    if ( state && buf->s.new_chap )
    {
        state->new_chap = buf->s.new_chap;
        buf->s.new_chap = 0;
    }
}

static inline void restore_chap( hb_psdemux_t *state, hb_buffer_t *buf )
{
    if ( state )
    {
        buf->s.new_chap = state->new_chap;
        state->new_chap = 0;
    }
}

/* Basic MPEG demuxer */

static void demux_dvd_ps( hb_buffer_t * buf, hb_buffer_list_t * list_es,
                          hb_psdemux_t* state )
{
    hb_buffer_t * buf_es;
    int           pos = 0;

    while ( buf )
    {
        save_chap( state, buf );

#define d (buf->data)
        /* pack_header */
        if( d[pos] != 0 || d[pos+1] != 0 ||
            d[pos+2] != 0x1 || d[pos+3] != 0xBA )
        {
            hb_log( "demux_dvd_ps: not a PS packet (%02x%02x%02x%02x)",
                    d[pos], d[pos+1], d[pos+2], d[pos+3] );
            hb_buffer_t *tmp = buf->next;
            buf->next = NULL;
            hb_buffer_close( &buf );
            buf = tmp;
            continue;
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
            check_mpeg_scr( state, scr, 700 );
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
        while( pos + 6 < buf->size &&
               d[pos] == 0 && d[pos+1] == 0 && d[pos+2] == 0x1 )
        {
            int      id;
            int      pes_packet_length;
            int      pes_packet_end;
            int      pes_header_d_length;
            int      pes_header_end;
            int      has_pts;
            int64_t  pts = AV_NOPTS_VALUE, dts = AV_NOPTS_VALUE;

            pos               += 3;               /* packet_start_code_prefix */
            id           = d[pos];
            pos               += 1;

            /* pack_header */
            if( id == 0xBA)
            {
                pos += 10 + (d[pos+9] & 7);
                continue;
            }

            /* system_header */
            if( id == 0xBB )
            {
                int header_length;

                header_length  = ( d[pos] << 8 ) + d[pos+1];
                pos           += 2 + header_length;
                continue;
            }

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

            buf_es->s.id           = id;
            buf_es->s.start        = pts;
            buf_es->s.renderOffset = dts;
            buf_es->s.duration     = (int64_t)AV_NOPTS_VALUE;
            buf_es->s.stop         = AV_NOPTS_VALUE;
            if ( state && id == 0xE0)
            {
                // Consume a chapter break, and apply it to the ES.
                restore_chap( state, buf_es );
            }
            memcpy( buf_es->data, d + pos, pes_packet_end - pos );

            hb_buffer_list_append(list_es, buf_es);

            pos = pes_packet_end;
        }
        hb_buffer_t *tmp = buf->next;
        buf->next = NULL;
        hb_buffer_close( &buf );
        buf = tmp;
    }
#undef d
}

// mpeg transport stream demuxer. the elementary stream headers have been
// stripped off and buf has all the info gleaned from them: id is set,
// start contains the pts (if any), renderOffset contains the dts (if any)
// and stop contains the pcr (if it changed).
static void demux_mpeg( hb_buffer_t *buf, hb_buffer_list_t *list_es,
                        hb_psdemux_t *state, int tolerance )
{
    while ( buf )
    {
        save_chap( state, buf );
        if ( state )
        {
            int discontinuity = 0;
            // we're keeping track of timing (i.e., not in scan)
            // check if there's a new pcr in this packet
            if ( buf->s.pcr >= 0 )
            {
                // we have a new pcr
                discontinuity = check_mpeg_scr( state, buf->s.pcr, tolerance );
                buf->s.pcr = AV_NOPTS_VALUE;
                // Some streams have consistently bad PCRs or SCRs
                // So filter out the offset
                if ( buf->s.start >= 0 )
                    state->scr_delta = buf->s.start - state->last_scr;
                else
                    state->scr_delta = 0;
            }
            if ( !discontinuity && buf->s.discontinuity )
            {
                // Buffer has been flagged as a discontinuity.  This happens
                // when a blueray changes clips.
                ++state->scr_changes;
                state->last_scr = buf->s.start;
                state->scr_delta = 0;
            }

            if ( buf->s.start >= 0 )
            {
                int64_t fdelta;
                if (buf->s.type == AUDIO_BUF || buf->s.type == VIDEO_BUF)
                {
                    if ( state->last_pts >= 0 )
                    {
                        fdelta = buf->s.start - state->last_pts;
                        if ( fdelta < -5 * 90000LL || fdelta > 5 * 90000LL )
                        {
                            // Packet too far from last. This may be a NZ TV
                            // broadcast as they like to change the PCR without
                            // sending a PCR update. Since it may be a while
                            // until they actually tell us the new PCR use the
                            // PTS as the PCR.
                            ++state->scr_changes;
                            state->last_scr = buf->s.start;
                            state->scr_delta = 0;
                        }
                    }
                    state->last_pts = buf->s.start;
                }
                if (state->last_scr != AV_NOPTS_VALUE)
                {
                    // Program streams have an SCR in every PACK header so they
                    // can't lose their clock reference. But the PCR in
                    // Transport streams is typically on <.1% of the packets.
                    // If a PCR packet gets lost and it marks a clock
                    // discontinuity then the data following it will be
                    // referenced to the wrong clock & introduce huge gaps or
                    // throw our A/V sync off. We try to protect against that
                    // here by sanity checking timestamps against the current
                    // reference clock and discarding packets where the DTS
                    // is "too far" from its clock.
                    fdelta = buf->s.start - state->last_scr - state->scr_delta;
                    if ( fdelta < -300 * 90000LL || fdelta > 300 * 90000LL )
                    {
                        // packet too far behind or ahead of its clock reference
                        buf->s.renderOffset = AV_NOPTS_VALUE;
                        buf->s.start = AV_NOPTS_VALUE;
                        buf->s.stop = AV_NOPTS_VALUE;
                    }
                    else
                    {
                        // Some streams have no PCRs.  In these cases, we
                        // will only get an "PCR" update if a large change
                        // in DTS or PTS is detected.  So we need to update
                        // our scr_delta with each valid timestamp so that
                        // fdelta does not continually grow.
                        state->scr_delta = buf->s.start - state->last_scr;
                    }
                }
            }

            if ( buf->s.type == VIDEO_BUF )
            {
                restore_chap( state, buf );
            }
        }

        hb_buffer_t *tmp = buf->next;
        buf->next = NULL;
        hb_buffer_list_append(list_es, buf);
        buf = tmp;
    }
}

static void demux_ts( hb_buffer_t *buf, hb_buffer_list_t *list_es,
                      hb_psdemux_t *state )
{
    // Distance between PCRs in TS is up to 100ms, but we have seen
    // streams that exceed this, so allow up to 300ms.
    demux_mpeg(buf, list_es, state, 300);
}

static void demux_ps( hb_buffer_t *buf, hb_buffer_list_t *list_es,
                      hb_psdemux_t *state )
{
    // Distance between SCRs in PS is up to 700ms
    demux_mpeg(buf, list_es, state, 700);
}

// "null" demuxer (makes a copy of input buf & returns it in list)
// used when the reader for some format includes its own demuxer.
// for example, ffmpeg.
static void demux_null( hb_buffer_t * buf, hb_buffer_list_t * list_es,
                        hb_psdemux_t* state )
{
    while ( buf )
    {
        save_chap( state, buf );
        if ( state )
        {
            // if we don't have a time offset yet,
            // use this timestamp as the offset.
            if (state->scr_changes == 0 &&
                (buf->s.start != AV_NOPTS_VALUE ||
                 buf->s.renderOffset != AV_NOPTS_VALUE))
            {
                ++state->scr_changes;
                state->last_scr = buf->s.start >= 0 ? buf->s.start : buf->s.renderOffset;
            }

            if ( buf->s.type == VIDEO_BUF )
            {
                restore_chap( state, buf );
            }
        }

        hb_buffer_t *tmp = buf->next;
        buf->next = NULL;
        hb_buffer_list_append(list_es, buf);
        buf = tmp;
    }
}

const hb_muxer_t hb_demux[] = { demux_dvd_ps, demux_ts, demux_ps, demux_null };
