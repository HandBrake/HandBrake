/* ********************************************************************* *\

Copyright (C) 2013 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

\* ********************************************************************* */

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_QSV

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/qsv_memory.h"

#if !HB_QSV_ONEVPL
int qsv_nv12_to_yuv420(struct SwsContext* sws_context,hb_buffer_t* dst, mfxFrameSurface1* src, mfxCoreInterface *core){
    int ret = 0;

    int in_pitch        = src->Data.Pitch;
    int w               = HB_QSV_ALIGN16(src->Info.Width);
    int h               = (MFX_PICSTRUCT_PROGRESSIVE == src->Info.PicStruct) ? HB_QSV_ALIGN16(src->Info.Height) : HB_QSV_ALIGN32(src->Info.Height);
    uint8_t *in_luma    = 0;
    uint8_t *in_chroma  = 0;
    static int copyframe_in_use = 1;


    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 accel_dst;

    if (copyframe_in_use)
    {
        accel_dst.Info.FourCC   = src->Info.FourCC;
        accel_dst.Info.CropH    = src->Info.CropH;
        accel_dst.Info.CropW    = src->Info.CropW;
        accel_dst.Info.CropY    = src->Info.CropY;
        accel_dst.Info.CropX    = src->Info.CropX;
        accel_dst.Info.Width    = w;
        accel_dst.Info.Height   = h;
        accel_dst.Data.Pitch    = src->Data.Pitch;
        accel_dst.Data.Y        = calloc( 1, in_pitch*h );
        accel_dst.Data.VU       = calloc( 1, in_pitch*h/2 );

        sts = core->CopyFrame(core->pthis, &accel_dst, src);

        if (sts < MFX_ERR_NONE)
        {
            free(accel_dst.Data.Y);
            free(accel_dst.Data.VU);
            copyframe_in_use = 0;
        }
        else
        {
            in_luma   = accel_dst.Data.Y  + accel_dst.Info.CropY * in_pitch     + accel_dst.Info.CropX;
            in_chroma = accel_dst.Data.VU + accel_dst.Info.CropY / 2 * in_pitch + accel_dst.Info.CropX;
        }
    }

    if (!copyframe_in_use)
    {
        in_luma   = src->Data.Y  + src->Info.CropY * in_pitch     + src->Info.CropX;
        in_chroma = src->Data.VU + src->Info.CropY / 2 * in_pitch + src->Info.CropX;
    }

    hb_video_buffer_realloc( dst, w, h );

    uint8_t *srcs[]   = { in_luma, in_chroma };
    int srcs_stride[] = { in_pitch, in_pitch };

    uint8_t *dsts[]   = { dst->plane[0].data, dst->plane[1].data, dst->plane[2].data };
    int dsts_stride[] = { dst->plane[0].stride, dst->plane[1].stride, dst->plane[2].stride };

    ret = sws_scale(sws_context, (const uint8_t* const *)srcs, srcs_stride,
                    0, h, dsts, dsts_stride );

    if (copyframe_in_use)
    {
        free(accel_dst.Data.Y);
        free(accel_dst.Data.VU);
    }

    return ret;
}
#endif

int qsv_convert_yuv_to_nv12(struct SwsContext *sws_context, mfxFrameSurface1 *dst, hb_buffer_t *src)
{
    int ret = 0;

    int h = src->plane[0].height;

    int out_pitch       = dst->Data.Pitch;
    uint8_t *out_luma   = dst->Data.Y;
    uint8_t *out_chroma = dst->Data.VU;

    uint8_t *srcs[]     = { src->plane[0].data, src->plane[1].data, src->plane[2].data };
    int srcs_stride[]   = { src->plane[0].stride, src->plane[1].stride, src->plane[2].stride };

    uint8_t *dsts[]     = { out_luma, out_chroma };
    int dsts_stride[]   = { out_pitch, out_pitch };

    ret = sws_scale(sws_context, (const uint8_t* const *)srcs, srcs_stride,
                    0, h, dsts, dsts_stride );

    return ret;
}

int qsv_copy_buffer_to_surface(mfxFrameSurface1* dst, hb_buffer_t* src)
{
    uint8_t* out_luma = dst->Data.Y;
    uint8_t* out_chroma = dst->Data.VU;
    int      out_pitch = dst->Data.Pitch;
    int      out_height = dst->Info.Height;

    // input buffer alignment from FFmpeg for p010 and nv12 could be different from surface alignment
    if (out_pitch == src->plane[0].stride)
    {
        int height = FFMIN(out_height, src->plane[0].height);
        memcpy(out_luma, src->plane[0].data, height * src->plane[0].stride);
        height = FFMIN(out_height, src->plane[1].height);
        memcpy(out_chroma, src->plane[1].data, height * src->plane[1].stride);
    }
    else
    {
        int pitch = FFMIN(out_pitch, src->plane[0].stride);
        int height = FFMIN(out_height, src->plane[0].height);
        for (int i = 0; i < height; i++)
        {
            uint8_t *out_row = out_luma + (i * out_pitch);
            uint8_t *in_row = src->plane[0].data + i * src->plane[0].stride;
            memcpy(out_row, in_row, pitch);
        }
        pitch = FFMIN(out_pitch, src->plane[1].stride);
        height = FFMIN(out_height / 2, src->plane[1].height);
        for (int i = 0; i < height; i++)
        {
            uint8_t *out_row = out_chroma + (i * out_pitch);
            uint8_t *in_row = src->plane[1].data + i * src->plane[1].stride;
            memcpy(out_row, in_row, pitch);
        }
    }

    return 0;
}
#endif // HB_PROJECT_FEATURE_QSV