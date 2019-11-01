/* encvobsub.c

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

struct hb_work_private_s
{
    hb_job_t * job;
};

int encsubInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    return 0;
}

int encsubWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;

    if (w->subtitle->source != VOBSUB)
    {
        // Invalid source, send EOF, this shouldn't ever happen
        hb_log("encvobsub: invalid subtitle source");
        hb_buffer_close( buf_in );
        *buf_out = hb_buffer_eof_init();
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    /*
     * Not much to do, just pass the buffer on.
     * Some day, we may re-encode bd subtitles here ;)
     */
    if (buf_out)
    {
        *buf_out = in;
        *buf_in = NULL;
    }

    return HB_WORK_OK;
}

void encsubClose( hb_work_object_t * w )
{
    free( w->private_data );
}

hb_work_object_t hb_encvobsub =
{
    WORK_ENCVOBSUB,
    "VOBSUB encoder",
    encsubInit,
    encsubWork,
    encsubClose
};
