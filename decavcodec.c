/* $Id: decavcodec.c,v 1.6 2005/03/06 04:08:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t       * job;
    hb_audio_t     * audio;

    AVCodecContext * context;
    int64_t          pts_last;
};


/***********************************************************************
 * Local prototypes
 **********************************************************************/
static int  Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out );
static void Close( hb_work_object_t ** _w );

/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_decavcodec_init( hb_job_t * job,
                                            hb_audio_t * audio )
{
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    AVCodec * codec;
    w->name  = strdup( "MPGA decoder (libavcodec)" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->audio = audio;

    codec = avcodec_find_decoder( CODEC_ID_MP2 );
    w->context = avcodec_alloc_context();
    avcodec_open( w->context, codec );
    w->pts_last = -1;

    return w;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;
    avcodec_close( w->context );
    free( w->name );
    free( w );
    *_w = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in, * buf, * last = NULL;
    int   pos, len, out_size, i;
    short buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    uint64_t cur;

    *buf_out = NULL;

    if( in->start < 0 ||
        ( w->pts_last > 0 &&
          in->start > w->pts_last &&
          in->start - w->pts_last < 5000 ) ) /* Hacky */
    {
        cur = w->pts_last;
    }
    else
    {
        cur = in->start;
    }

    pos = 0;
    while( pos < in->size )
    {
        len = avcodec_decode_audio( w->context, buffer, &out_size,
                                    in->data + pos, in->size - pos );
        if( out_size )
        {
            short * s16;
            float * fl32;

            buf = hb_buffer_init( 2 * out_size );

            buf->start = cur;
            buf->stop  = cur + 90000 * ( out_size / 4 ) /
                         w->context->sample_rate;
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

    w->pts_last = cur;

    return HB_WORK_OK;
}

