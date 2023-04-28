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
