/* $Id: demuxmpeg.c,v 1.4 2004/10/19 23:11:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

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
        /* extract the system clock reference (scr) */
        int64_t scr = ((uint64_t)(d[pos] & 0x38) << 27) |
                      ((uint64_t)(d[pos] & 0x03) << 28) |
                      ((uint64_t)(d[pos+1]) << 20) |
                      ((uint64_t)(d[pos+2] >> 3) << 15) |
                      ((uint64_t)(d[pos+2] & 3) << 13) |
                      ((uint64_t)(d[pos+3]) << 5) |
                      (d[pos+4] >> 3);
        int64_t scr_delta = scr - state->last_scr;
        if ( scr_delta > (90*700) || scr_delta < 0 )
        {
            ++state->scr_changes;
            state->scr_offset += scr_delta - 1;
        }
        state->last_scr = scr;
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

// "null" demuxer (makes a copy of input buf & returns it in list)
// used when the reader for some format includes its own demuxer.
// for example, ffmpeg.
int hb_demux_null( hb_buffer_t * buf_ps, hb_list_t * list_es, hb_psdemux_t* state )
{
    hb_buffer_t *buf = hb_buffer_init( buf_ps->size );

    // copy everything from the old to the new except the data ptr & alloc
    uint8_t *data = buf->data;
    int alloc = buf->alloc;
    *buf = *buf_ps;
    buf->data = data;
    buf->alloc = alloc;

    // now copy the data
    memcpy( buf->data, buf_ps->data, buf_ps->size );
    hb_list_add( list_es, buf );
    return 1;
}
