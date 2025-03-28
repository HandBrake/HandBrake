/* worknull.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* Does no work ;) but also serves as a example template */

#include "handbrake/handbrake.h"

struct hb_work_private_s
{
};

/***********************************************************************
 * Init
 ***********************************************************************
 * Initialize hb_work_private_t data
 **********************************************************************/
static int Init( hb_work_object_t * w, hb_job_t * job )
{
    w->private_data = NULL;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free any allocation in hb_work_private_t
 **********************************************************************/
static void Close( hb_work_object_t * w )
{
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * Take an input buffer, send an output buffer
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;

    // Pass input to output
    *buf_out = *buf_in;
    // Mark input buffer NULL so work loop doesn't delete it
    *buf_in = NULL;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - say that we're done */
        return HB_WORK_DONE;
    }
    return HB_WORK_OK;
}

/***********************************************************************
 * Info
 ***********************************************************************
 * Retrieve current info about context initialized during Init
 **********************************************************************/
static int Info( hb_work_object_t *w, hb_work_info_t *info )
{
    memset(info, 0, sizeof(*info));

    // Indicate no info is returned
    return 0;
}

/***********************************************************************
 * BSInfo
 ***********************************************************************
 * Retrieve info, does not require Init(), but uses current context
 * if Init has already been called.
 * buf contains stream data to extract info from.
 **********************************************************************/
static int BSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                   hb_work_info_t *info )
{
    memset( info, 0, sizeof(*info) );

    // Indicate no info is returned
    return 0;
}

/***********************************************************************
 * Flush
 ***********************************************************************
 * Reset context without closing, kind of poorly named :(
 **********************************************************************/
static void Flush( hb_work_object_t *w )
{
}

hb_work_object_t hb_workpass =
{
    .id     = WORK_PASS,
    .name   = "Passthrough",
    .init   = Init,
    .work   = Work,
    .close  = Close,
    .info   = Info,
    .bsinfo = BSInfo,
    .flush  = Flush,
};
