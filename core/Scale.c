/* $Id: Scale.c,v 1.14 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "ffmpeg/avcodec.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;

    HBBuffer           * deintBuffer;
    ImgReSampleContext * context;
    AVPicture            rawPicture;
    AVPicture            deintPicture;
    AVPicture            scaledPicture;
};

/* Local prototypes */
static int ScaleWork( HBWork * );

HBWork * HBScaleInit( HBHandle * handle, HBTitle * title )
{
    HBWork * w;
    if( !( w = malloc( sizeof( HBWork ) ) ) )
    {
        HBLog( "HBScaleInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "Scale" );
    w->work = ScaleWork;

    w->handle = handle;
    w->title  = title;

    /* Allocate a constant buffer used for deinterlacing */
    w->deintBuffer = HBBufferInit( 3 * title->inWidth *
                                   title->inHeight / 2 );

    avpicture_fill( &w->deintPicture, w->deintBuffer->data,
                    PIX_FMT_YUV420P, title->inWidth, title->inHeight );

    /* Init libavcodec */
    w->context =
        img_resample_full_init( title->outWidth, title->outHeight,
                                title->inWidth,  title->inHeight,
                                title->topCrop,  title->bottomCrop,
                                title->leftCrop, title->rightCrop );

    return w;
}

void HBScaleClose( HBWork ** _w )
{
    HBWork * w = *_w;

    img_resample_close( w->context );
    HBBufferClose( &w->deintBuffer );
    free( w->name );
    free( w );
    *_w = NULL;
}

static int ScaleWork( HBWork * w )
{
    HBTitle  * title = w->title;
    HBBuffer * rawBuffer;
    HBBuffer * scaledBuffer;
    HBBuffer * tmpBuffer;

    if( HBFifoIsHalfFull( title->scaledFifo ) )
    {
        return 0;
    }

    /* Get a new raw picture */
    if( !( rawBuffer = HBFifoPop( title->rawFifo ) ) )
    {
        return 0;
    }

    /* Allocate new buffer for the scaled picture */
    scaledBuffer = HBBufferInit( 3 * title->outWidth *
                                 title->outHeight / 2 );
    scaledBuffer->position = rawBuffer->position;
    scaledBuffer->pass     = rawBuffer->pass;

    /* libavcodec stuff */
    avpicture_fill( &w->rawPicture, rawBuffer->data, PIX_FMT_YUV420P,
                    title->inWidth, title->inHeight );
    avpicture_fill( &w->scaledPicture, scaledBuffer->data,
                    PIX_FMT_YUV420P, title->outWidth,
                    title->outHeight );

    /* Do the job */
    if( title->deinterlace )
    {
        avpicture_deinterlace( &w->deintPicture, &w->rawPicture,
                               PIX_FMT_YUV420P, title->inWidth,
                               title->inHeight );
        img_resample( w->context, &w->scaledPicture,
                      &w->deintPicture );
    }
    else
    {
        img_resample( w->context, &w->scaledPicture, &w->rawPicture );
    }

    if( rawBuffer->repeat )
    {
        tmpBuffer = HBBufferInit( scaledBuffer->size );
        tmpBuffer->position = scaledBuffer->position;
        tmpBuffer->pass     = scaledBuffer->pass;
        memcpy( tmpBuffer->data, scaledBuffer->data,
                scaledBuffer->size );

        if( !HBFifoPush( title->scaledFifo, &tmpBuffer ) )
        {
            HBLog( "HBScale: HBFifoPush failed" );
        }
    }

    if( !HBFifoPush( title->scaledFifo, &scaledBuffer ) )
    {
        HBLog( "HBScale: HBFifoPush failed" );
    }

    /* Free memory */
    HBBufferClose( &rawBuffer );

    return 1;
}

