#ifdef SYS_DARWIN
#  include <Accelerate/Accelerate.h>
#else
#  include "ffmpeg/avcodec.h"
#endif

#include "scale.h"

struct hb_scale_s
{
#ifdef SYS_DARWIN
    int           inOffsets[3];;
    int           outOffsets[3];;
    vImage_Buffer inY;
    vImage_Buffer inUV;
    vImage_Buffer outY;
    vImage_Buffer outUV;
    uint8_t       * tempBuffer;
#else
#endif
};

hb_scale_t * hb_scale_init( int inWidth, int inHeight, int outWidth,
                            int outHeight, int * crop )
{
    hb_scale_t * s = calloc( 1, sizeof( hb_scale_t ) );

#ifdef SYS_DARWIN
    s->inY.width      = inWidth - crop[2] - crop[3];
    s->inY.height     = inHeight - crop[0] - crop[1];
    s->inY.rowBytes   = inWidth;
    s->inUV           = s->inY;
    s->inUV.width    /= 2;
    s->inUV.height   /= 2;
    s->inUV.rowBytes /= 2;

    s->inOffsets[0] = crop[0] * inWidth + crop[2];
    s->inOffsets[1] = inWidth * inHeight +
                      crop[0] * inWidth / 4 + crop[2] / 2;
    s->inOffsets[2] = s->inOffsets[1] + inWidth * inHeight / 4;

    s->outY.width      = outWidth;
    s->outY.height     = outHeight;
    s->outY.rowBytes   = outWidth;
    s->outUV           = s->outY;
    s->outUV.width    /= 2;
    s->outUV.height   /= 2;
    s->outUV.rowBytes /= 2;

    s->outOffsets[0] = 0;
    s->outOffsets[1] = outWidth * outHeight;
    s->outOffsets[2] = outWidth * outHeight * 5 / 4;

    int tempSize;
    tempSize      = vImageScale_Planar8( &s->inY, &s->outY, NULL,
                                         kvImageGetTempBufferSize );
    s->tempBuffer = malloc( tempSize );
#else
#endif

    return s;
}

void hb_scale_process( hb_scale_t * s, uint8_t * in, uint8_t * out )
{
#ifdef SYS_DARWIN
    vImage_Buffer imgIn, imgOut;

    imgIn  = s->inY;
    imgOut = s->outY;
    imgIn.data  = &in[s->inOffsets[0]];
    imgOut.data = &out[s->outOffsets[0]];
    vImageScale_Planar8( &imgIn, &imgOut, s->tempBuffer, kvImageEdgeExtend );

    imgIn  = s->inUV;
    imgOut = s->outUV;
    imgIn.data  = &in[s->inOffsets[1]];
    imgOut.data = &out[s->outOffsets[1]];
    vImageScale_Planar8( &imgIn, &imgOut, s->tempBuffer, kvImageEdgeExtend );

    imgIn  = s->inUV;
    imgOut = s->outUV;
    imgIn.data  = &in[s->inOffsets[2]];
    imgOut.data = &out[s->outOffsets[2]];
    vImageScale_Planar8( &imgIn, &imgOut, s->tempBuffer, kvImageEdgeExtend );
#else
#endif
}

void hb_scale_close( hb_scale_t ** _s )
{
    hb_scale_t * s = *_s;

#ifdef SYS_DARWIN
    free( s->tempBuffer );
#else
#endif

    free( s );
    *_s = NULL;
}
