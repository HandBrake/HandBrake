/* $Id: Scale.c,v 1.10 2004/03/08 11:32:48 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#define USE_FFMPEG

#ifdef USE_FFMPEG
#include "ffmpeg/avcodec.h"
#endif

typedef struct HBScale
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;

    HBBuffer           * deintBuffer;
    HBList             * scaledBufferList;
#ifdef USE_FFMPEG
    ImgReSampleContext * context;
    AVPicture            rawPicture;
    AVPicture            deintPicture;
    AVPicture            scaledPicture;
#endif
} HBScale;

/* Local prototypes */
static int ScaleWork( HBWork * );
#ifndef USE_FFMPEG
static void Deinterlace( uint8_t * in, uint8_t * out, int w, int h,
                         int tcrop, int bcrop, int lcrop, int rcrop );
static void Resample( uint8_t * in, uint8_t * out, int oldw, int oldh,
                      int neww, int newh, int tcrop, int bcrop,
                      int lcrop, int rcrop );
#endif

HBWork * HBScaleInit( HBHandle * handle, HBTitle * title )
{
    HBScale * s;
    if( !( s = malloc( sizeof( HBScale ) ) ) )
    {
        HBLog( "HBScaleInit: malloc() failed, gonna crash" );
        return NULL;
    }

    s->name = strdup( "Scale" );
    s->work = ScaleWork;

    s->handle = handle;
    s->title  = title;

    /* Allocate a constant buffer used for deinterlacing */
    s->deintBuffer = HBBufferInit( 3 * title->inWidth *
                                    title->inHeight / 2 );

#ifdef USE_FFMPEG
    avpicture_fill( &s->deintPicture, s->deintBuffer->data,
                    PIX_FMT_YUV420P, title->inWidth, title->inHeight );

    /* Init libavcodec */
    s->context =
        img_resample_full_init( title->outWidth, title->outHeight,
                                title->inWidth,  title->inHeight,
                                title->topCrop,  title->bottomCrop,
                                title->leftCrop, title->rightCrop );
#endif

    s->scaledBufferList = HBListInit();

    return (HBWork*) s;
}

void HBScaleClose( HBWork ** _s )
{
    HBScale * s = (HBScale*) *_s;

#ifdef USE_FFMPEG
    img_resample_close( s->context );
#endif
    HBListClose( &s->scaledBufferList );
    HBBufferClose( &s->deintBuffer );
    free( s->name );
    free( s );

    *_s = NULL;
}

static int ScaleWork( HBWork * w )
{
    HBScale  * s     = (HBScale*) w;
    HBTitle  * title = s->title;
    HBBuffer * rawBuffer;
    HBBuffer * scaledBuffer;
    HBBuffer * tmpBuffer;
#ifndef USE_FFMPEG
    uint8_t  * in, * out;
    int        plane, shift;
#endif

    int didSomething = 0;

    /* Push scaled buffer(s) */
    while( ( scaledBuffer = (HBBuffer*)
                HBListItemAt( s->scaledBufferList, 0 ) ) )
    {
        tmpBuffer = scaledBuffer;
        if( HBFifoPush( title->scaledFifo, &scaledBuffer ) )
        {
            didSomething = 1;
            HBListRemove( s->scaledBufferList, tmpBuffer );
        }
        else
        {
            return didSomething;
        }
    }

    /* Get a new raw picture */
    if( ( rawBuffer = HBFifoPop( title->rawFifo ) ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    /* Allocate new buffer for the scaled picture */
    scaledBuffer = HBBufferInit( 3 * title->outWidth *
                                 title->outHeight / 2 );
    scaledBuffer->position = rawBuffer->position;
    scaledBuffer->pass     = rawBuffer->pass;

#ifdef USE_FFMPEG
    /* libavcodec stuff */
    avpicture_fill( &s->rawPicture, rawBuffer->data, PIX_FMT_YUV420P,
                    title->inWidth, title->inHeight );
    avpicture_fill( &s->scaledPicture, scaledBuffer->data,
                    PIX_FMT_YUV420P, title->outWidth,
                    title->outHeight );

    /* Do the job */
    if( title->deinterlace )
    {
        avpicture_deinterlace( &s->deintPicture, &s->rawPicture,
                               PIX_FMT_YUV420P, title->inWidth,
                               title->inHeight );
        img_resample( s->context, &s->scaledPicture,
                      &s->deintPicture );
    }
    else
    {
        img_resample( s->context, &s->scaledPicture, &s->rawPicture );
    }
#else
    if( title->deinterlace )
    {
        in  = rawBuffer->data;
        out = s->deintBuffer->data;
        for( plane = 0; plane < 3; plane++ )
        {
            shift = plane ? 1 : 0;
            Deinterlace( in, out, title->inWidth >> shift,
                         title->inHeight >> shift,
                         title->topCrop >> shift,
                         title->bottomCrop >> shift,
                         title->leftCrop >> shift,
                         title->rightCrop >> shift  );
            in  += title->inWidth * title->inHeight >> ( 2 * shift );
            out += title->inWidth * title->inHeight >> ( 2 * shift );
        }
    }

    in  = title->deinterlace ? s->deintBuffer->data : rawBuffer->data;
    out = scaledBuffer->data;
    for( plane = 0; plane < 3; plane++ )
    {
        shift = plane ? 1 : 0;
        Resample( in, out, title->inWidth >> shift,
                  title->inHeight >> shift, title->outWidth >> shift,
                  title->outHeight >> shift, title->topCrop >> shift,
                  title->bottomCrop >> shift, title->leftCrop >> shift,
                  title->rightCrop >> shift );
        in  += title->inWidth * title->inHeight >> ( 2 * shift );
        out += title->outWidth * title->outHeight >> ( 2 * shift );;
    }
#endif

    HBListAdd( s->scaledBufferList, scaledBuffer );

    if( rawBuffer->repeat )
    {
        tmpBuffer = HBBufferInit( scaledBuffer->size );
        tmpBuffer->position = scaledBuffer->position;
        tmpBuffer->pass     = scaledBuffer->pass;
        memcpy( tmpBuffer->data, scaledBuffer->data,
                scaledBuffer->size );

        HBListAdd( s->scaledBufferList, tmpBuffer );
    }

    /* Free memory */
    HBBufferClose( &rawBuffer );

    return didSomething;
}

#ifndef USE_FFMPEG
static void Deinterlace( uint8_t * in, uint8_t * out, int w, int h,
                         int tcrop, int bcrop, int lcrop, int rcrop )
{
    int i, j;
    
    /* First line */
    if( !tcrop )
    {
        memcpy( out, in + lcrop, w - lcrop - rcrop );
    }

    /* Merge lines */
    for( i = MAX( 1, tcrop ); i < h - bcrop; i++ )
    {
        for( j = lcrop; j < w - rcrop; j++ )
        {
            out[i*w+j] = ( in[(i-1)*w+j] + in[i*w+j] ) / 2;
        }
    }
}

static void Resample( uint8_t * in, uint8_t * out, int oldw, int oldh,
                      int neww, int newh, int tcrop, int bcrop,
                      int lcrop, int rcrop )
{
    int i, j;
    int cropw = oldw - lcrop - rcrop;
    int croph = oldh - tcrop - bcrop;
    for( i = 0; i < newh; i++ )
    {
        for( j = 0; j < neww; j++ )
        {
            out[i*neww+j] = in[(tcrop+i*croph/newh)*oldw +
                               lcrop+j*cropw/neww];
        }
    }
}
#endif

