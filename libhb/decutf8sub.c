/* decutf8sub.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Decoder for UTF-8 subtitles obtained from file input-sources.
 * 
 * Input and output packet format is UTF-8 encoded text,
 * with limited HTML-style markup (only <b>, <i>, and <u>).
 * 
 * @author David Foster (davidfstr)
 */

#include <stdlib.h>
#include <stdio.h>
#include "hb.h"

static int decutf8Init( hb_work_object_t * w, hb_job_t * job )
{
    return 0;
}

static int decutf8Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = NULL;

    // Pass the packets through without modification
    out = in;

    // Warn if the subtitle's duration has not been passed through by the demuxer,
    // which will prevent the subtitle from displaying at all
    if ( out->s.stop == 0 ) {
        hb_log( "decutf8sub: subtitle packet lacks duration" );
    }
    
    // We shouldn't be storing the extra NULL character,
    // but the MP4 muxer expects this, unfortunately.
    if ( out->size > 0 && out->data[out->size - 1] != '\0' ) {
        // NOTE: out->size remains unchanged
        hb_buffer_realloc( out, out->size + 1 );
        out->data[out->size] = '\0';
    }
    
    *buf_in = NULL;
    *buf_out = out;
    return HB_WORK_OK;
}

static void decutf8Close( hb_work_object_t * w )
{
    // nothing
}

hb_work_object_t hb_decutf8sub =
{
    WORK_DECUTF8SUB,
    "UTF-8 Subtitle Decoder",
    decutf8Init,
    decutf8Work,
    decutf8Close
};
