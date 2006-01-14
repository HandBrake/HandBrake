/* $Id: X264Enc.c,v 1.3 2003/12/26 20:03:27 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* x264 */
#include "x264.h"

typedef struct HBX264Enc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    *handle;
    HBTitle     *title;

    HBBuffer    *buffer;

    x264_t         *h;
    x264_picture_t picture;
} HBX264Enc;

/* Local prototypes */
static int HBX264EncWork( HBWork * );

HBWork *HBX264EncInit( HBHandle * handle, HBTitle * title )
{
    HBX264Enc *f = malloc( sizeof( HBX264Enc ) );

    if( f )
    {
        x264_param_t param;

        f->name     = strdup( "X264Enc" );
        f->work     = HBX264EncWork;

        f->handle   = handle;
        f->title    = title;

        f->buffer = NULL;

        x264_param_default( &param );

        param.i_width = title->outWidth;
        param.i_height= title->outHeight;
        param.i_iframe = 5 * title->rate / title->rateBase;
        /* FIXME */
        param.b_deblocking_filter = 0;
        param.i_me = 1;

        if( ( f->h = x264_encoder_open( &param ) ) == NULL )
        {
            HBLog( "x264: x264_encoder_new failed" );
            return NULL;
        }
        memset( &f->picture, 0, sizeof( x264_picture_t ) );
        f->picture.i_width  = param.i_width;
        f->picture.i_height = param.i_height;
        f->picture.i_plane  = 3;

        HBLog( "x264: opening with %dx%d iframes=%d", param.i_width, param.i_height, param.i_iframe );
    }

    return (HBWork*) f;
}

void HBX264EncClose( HBWork **_f )
{
    HBX264Enc *f = (HBX264Enc*) *_f;

    x264_encoder_close( f->h );

    if( f->buffer )
    {
        HBBufferClose( &f->buffer );
    }

    free( f->name );

    free( f );

    *_f = NULL;
}

/* TODO trash buffer->pass == 1
 * return 0 ->rien fait
 *        1 -> sinon
 */
static int HBX264EncWork( HBWork * w )
{
    HBX264Enc  * f     = (HBX264Enc*) w;
    HBTitle    * title = f->title;

    HBBuffer * frame;
    int didSomething = 0;

    x264_nal_t *nal;
    int        i_nal;
    int        i;

    if( f->buffer )
    {
        if( !HBFifoPush( title->outFifo, &f->buffer ) )
        {
            /* nothing done */
            return 0;
        }
        didSomething = 1;
    }

    if( ( frame = HBFifoPop( title->scaledFifo ) ) == NULL )
    {
        return didSomething;
    }

    if( frame->pass == 1 )
    {
        /* Trash all first pass buffer */
        HBBufferClose( &frame );
        return 1;
    }

    f->picture.i_stride[0] = title->outWidth;
    f->picture.i_stride[1] = title->outWidth/2;
    f->picture.i_stride[2] = title->outWidth/2;

    f->picture.plane[0] = frame->data;
    f->picture.plane[1] = &f->picture.plane[0][title->outWidth*title->outHeight];
    f->picture.plane[2] = &f->picture.plane[1][title->outWidth*title->outHeight/4];

    x264_encoder_encode( f->h, &nal, &i_nal, &f->picture );

    f->buffer = HBBufferInit( 3 * title->outWidth * title->outHeight / 2 ); /* FIXME */
    f->buffer->keyFrame = 0;
    f->buffer->position = frame->position;
    f->buffer->size = 0;

    for( i = 0; i < i_nal; i++ )
    {
        int i_data = f->buffer->alloc - f->buffer->size;
        int i_size;

        i_size = x264_nal_encode( &f->buffer->data[f->buffer->size],
                                  &i_data, 1, &nal[i] );
        if( i_size <= 0 )
        {
            fprintf( stderr, "#################### error" );
        }

        f->buffer->size += i_size;

        if( nal[i].i_ref_idc == NAL_PRIORITY_HIGH ||
            nal[i].i_ref_idc == NAL_PRIORITY_HIGHEST )
        {
            f->buffer->keyFrame = 1;
        }
    }

    /* Inform the GUI about the current position */
    HBPosition( f->handle, frame->position );

    HBBufferClose( &frame );

    return 1;
}
