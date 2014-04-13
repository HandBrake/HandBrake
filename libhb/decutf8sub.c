/* decutf8sub.c

   Copyright (c) 2003-2014 HandBrake Team
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
#include "decsrtsub.h"

struct hb_work_private_s
{
    int line;   // SSA line number
};

static int decutf8Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t * pv;
    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
        return 1;
    w->private_data = pv;

    // Generate generic SSA Script Info.
    int height = job->title->height - job->crop[0] - job->crop[1];
    int width = job->title->width - job->crop[2] - job->crop[3];
    hb_subtitle_add_ssa_header(w->subtitle, width, height);

    return 0;
}

static int decutf8Work(hb_work_object_t * w,
                       hb_buffer_t **buf_in, hb_buffer_t **buf_out)
{
    hb_work_private_t * pv = w->private_data;
    // Pass the packets through without modification
    hb_buffer_t *out = *buf_in;

    out->s.frametype = HB_FRAME_SUBTITLE;

    // Warn if the subtitle's duration has not been passed through by the
    // demuxer, which will prevent the subtitle from displaying at all
    if (out->s.stop == 0)
    {
        hb_log("decutf8sub: subtitle packet lacks duration");
    }

    hb_srt_to_ssa(out, ++pv->line);

    *buf_in  = NULL;
    *buf_out = out;

    if (out->size == 0)
        return HB_WORK_DONE;
    return HB_WORK_OK;
}

static void decutf8Close(hb_work_object_t *w)
{
    free(w->private_data);
}

hb_work_object_t hb_decutf8sub =
{
    WORK_DECUTF8SUB,
    "UTF-8 Subtitle Decoder",
    decutf8Init,
    decutf8Work,
    decutf8Close
};
