/* $Id: Scale.c,v 1.4 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#include "Scale.h"
#include "Work.h"

#include <ffmpeg/avcodec.h>

/* Local prototypes */
static int ScaleWork( HBWork * );

struct HBScale
{
    HB_WORK_COMMON_MEMBERS

    HBHandle           * handle;
    HBTitle            * title;

    ImgReSampleContext * context;
    AVPicture            rawPicture;
    HBBuffer           * deintBuffer;
    AVPicture            deintPicture;
    HBBuffer           * scaledBuffer;
    AVPicture            scaledPicture;
};

HBScale * HBScaleInit( HBHandle * handle, HBTitle * title )
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

    s->scaledBuffer = NULL;

    return s;
}

void HBScaleClose( HBScale ** _s )
{
    HBScale * s = *_s;
    
    img_resample_close( s->context );
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

    int didSomething = 0;

    /* Push scaled buffer */
    if( s->scaledBuffer )
    {
        if( HBFifoPush( title->scaledFifo, &s->scaledBuffer ) )
        {
            didSomething = 1;
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
    s->scaledBuffer = HBBufferInit( 3 * title->outWidth *
                                    title->outHeight / 2 );
    s->scaledBuffer->position = rawBuffer->position;
    s->scaledBuffer->pass     = rawBuffer->pass;

    /* libavcodec stuff */
    avpicture_fill( &s->rawPicture, rawBuffer->data, PIX_FMT_YUV420P,
                    title->inWidth, title->inHeight );
    avpicture_fill( &s->scaledPicture, s->scaledBuffer->data,
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

    /* Free memory */
    HBBufferClose( &rawBuffer );
    
    return didSomething;
}

