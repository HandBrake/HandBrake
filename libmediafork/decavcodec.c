/* $Id: decavcodec.c,v 1.6 2005/03/06 04:08:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "mediafork.h"

#include "ffmpeg/avcodec.h"

int  decavcodecInit( hb_work_object_t *, hb_job_t * );
int  decavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void decavcodecClose( hb_work_object_t * );

hb_work_object_t hb_decavcodec =
{
    WORK_DECAVCODEC,
    "MPGA decoder (libavcodec)",
    decavcodecInit,
    decavcodecWork,
    decavcodecClose
};

struct hb_work_private_s
{
    hb_job_t       * job;

    AVCodecContext * context;
    int64_t          pts_last;
};


/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
int decavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job   = job;

    codec = avcodec_find_decoder( CODEC_ID_MP2 );
    pv->context = avcodec_alloc_context();
    avcodec_open( pv->context, codec );
    pv->pts_last = -1;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void decavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    avcodec_close( pv->context );
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int decavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in, * buf, * last = NULL;
    int   pos, len, out_size, i;
    short buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    uint64_t cur;

    *buf_out = NULL;

    if( in->start < 0 ||
        ( pv->pts_last > 0 &&
          in->start > pv->pts_last &&
          in->start - pv->pts_last < 5000 ) ) /* Hacky */
    {
        cur = pv->pts_last;
    }
    else
    {
        cur = in->start;
    }

    pos = 0;
    while( pos < in->size )
    {
        len = avcodec_decode_audio( pv->context, buffer, &out_size,
                                    in->data + pos, in->size - pos );
        if( out_size )
        {
            short * s16;
            float * fl32;

            buf = hb_buffer_init( 2 * out_size );

            buf->start = cur;
            buf->stop  = cur + 90000 * ( out_size / 4 ) /
                         pv->context->sample_rate;
            cur = buf->stop;

            s16  = buffer;
            fl32 = (float *) buf->data;
            for( i = 0; i < out_size / 2; i++ )
            {
                fl32[i] = s16[i];
            }

            if( last )
            {
                last = last->next = buf;
            }
            else
            {
                *buf_out = last = buf;
            }
        }

        pos += len;
    }

    pv->pts_last = cur;

    return HB_WORK_OK;
}

