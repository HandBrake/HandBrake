/* $Id: demuxmpeg.c,v 1.4 2004/10/19 23:11:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

/* Basic MPEG demuxer, only works with DVDs (2048 bytes packets) */

int hb_demux_ps( hb_buffer_t * buf_ps, hb_list_t * list_es )
{
    hb_buffer_t * buf_es;
    int           pos;

    pos = 0;

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
        int64_t  pts = -1;

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

        has_pts             = ( ( d[pos+1] >> 6 ) & 0x2 ) ? 1 : 0;
        pos               += 2;               /* Required headers */

        pes_header_d_length  = d[pos];
        pos                    += 1;
        pes_header_end          = pos + pes_header_d_length;

        if( has_pts )
        {
            pts = ( ( ( (uint64_t) d[pos] >> 1 ) & 0x7 ) << 30 ) +
                  ( d[pos+1] << 22 ) +
                  ( ( d[pos+2] >> 1 ) << 15 ) +
                  ( d[pos+3] << 7 ) +
                  ( d[pos+4] >> 1 );
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
