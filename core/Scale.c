/* $Id: Scale.c,v 1.9 2004/01/16 19:39:23 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "ffmpeg/avcodec.h"

typedef struct HBScale
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;

    ImgReSampleContext * context;
    AVPicture            rawPicture;
    HBBuffer           * deintBuffer;
    AVPicture            deintPicture;
    HBList             * scaledBufferList;
    AVPicture            scaledPicture;
} HBScale;

/* Local prototypes */
static int ScaleWork( HBWork * );

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

    /* Init libavcodec */
    s->context =
        img_resample_full_init( title->outWidth, title->outHeight,
                                title->inWidth,  title->inHeight,
                                title->topCrop,  title->bottomCrop,
                                title->leftCrop, title->rightCrop );

    /* Allocate a constant buffer used for deinterlacing */
    s->deintBuffer = HBBufferInit( 3 * title->inWidth *
                                    title->inHeight / 2 );
    avpicture_fill( &s->deintPicture, s->deintBuffer->data,
                    PIX_FMT_YUV420P, title->inWidth, title->inHeight );

    s->scaledBufferList = HBListInit();

    return (HBWork*) s;
}

void HBScaleClose( HBWork ** _s )
{
    HBScale * s = (HBScale*) *_s;

    img_resample_close( s->context );
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

