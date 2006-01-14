/* $Id: X264Enc.c,v 1.8 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* x264 */
#include "x264.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    *handle;
    HBTitle     *title;

    x264_t         *h;
    x264_picture_t picture;
};

/* Local prototypes */
static int HBX264EncWork( HBWork * );

HBWork * HBX264EncInit( HBHandle * handle, HBTitle * title )
{
    HBWork * w = malloc( sizeof( HBWork ) );

    if( w )
    {
        x264_param_t param;

        w->name     = strdup( "X264Enc" );
        w->work     = HBX264EncWork;

        w->handle   = handle;
        w->title    = title;

        x264_param_default( &param );

        param.i_width = title->outWidth;
        param.i_height= title->outHeight;
        param.i_iframe = 20 * title->rate / title->rateBase;
        param.i_idrframe = 1;
        param.b_cabac = 0;
        param.analyse.inter =
            X264_ANALYSE_I16x16|X264_ANALYSE_I4x4|X264_ANALYSE_P16x16|
            X264_ANALYSE_P16x8|X264_ANALYSE_P8x16|X264_ANALYSE_P8x8|
            X264_ANALYSE_SMART_PSUB;

        if( ( w->h = x264_encoder_open( &param ) ) == NULL )
        {
            HBLog( "x264: x264_encoder_new failed" );
            return NULL;
        }
        memset( &w->picture, 0, sizeof( x264_picture_t ) );
        w->picture.i_width  = param.i_width;
        w->picture.i_height = param.i_height;
        w->picture.i_plane  = 3;

        HBLog( "x264: opening with %dx%d iframes=%d", param.i_width, param.i_height, param.i_iframe );
    }

    return w;
}

void HBX264EncClose( HBWork ** _w )
{
    HBWork * w = *_w;
    x264_encoder_close( w->h );
    free( w->name );
    free( w );
    *_w = NULL;
}

/* TODO trash buffer->pass == 1
 * return 0 ->rien fait
 *        1 -> sinon
 */
static int HBX264EncWork( HBWork * w )
{
    HBTitle    * title = w->title;

    HBBuffer * frame, * buffer;

    x264_nal_t *nal;
    int        i_nal;
    int        i;

    if( HBFifoIsHalfFull( title->outFifo ) )
    {
        return 0;
    }

    if( !( frame = HBFifoPop( title->scaledFifo ) ) )
    {
        return 0;
    }

    if( frame->pass == 1 )
    {
        /* Trash all first pass buffer */
        HBBufferClose( &frame );
        return 1;
    }

    w->picture.i_stride[0] = title->outWidth;
    w->picture.i_stride[1] = title->outWidth/2;
    w->picture.i_stride[2] = title->outWidth/2;

    w->picture.plane[0] = frame->data;
    w->picture.plane[1] = &w->picture.plane[0][title->outWidth*title->outHeight];
    w->picture.plane[2] = &w->picture.plane[1][title->outWidth*title->outHeight/4];

    x264_encoder_encode( w->h, &nal, &i_nal, &w->picture );

    buffer = HBBufferInit( 3 * title->outWidth * title->outHeight / 2 ); /* FIXME */
    buffer->keyFrame = 0;
    buffer->position = frame->position;
    buffer->size = 0;

    for( i = 0; i < i_nal; i++ )
    {
        int i_data = buffer->alloc - buffer->size;
        int i_size;

        i_size = x264_nal_encode( &buffer->data[buffer->size],
                                  &i_data, 1, &nal[i] );
        if( i_size <= 0 )
        {
            fprintf( stderr, "#################### error" );
        }

        buffer->size += i_size;

        if( nal[i].i_ref_idc == NAL_PRIORITY_HIGH ||
            nal[i].i_ref_idc == NAL_PRIORITY_HIGHEST )
        {
            buffer->keyFrame = 1;
        }
    }

    /* Inform the GUI about the current position */
    HBPosition( w->handle, frame->position );

    HBBufferClose( &frame );

    if( !HBFifoPush( title->outFifo, &buffer ) )
    {
        HBLog( "HBX264Enc: HBFifoPush failed" );
    }

    return 1;
}
