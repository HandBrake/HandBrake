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

#ifndef QSV_MEMORY_H
#define QSV_MEMORY_H

#include "libavcodec/qsv.h"
#include "msdk/mfxplugin.h"

typedef struct{

    struct{
        // for "planes" , Y and VU
        uint8_t     *data[2];
        int         strides[2];
    }               qsv_data;

    struct{
        // for each plane, Y U V
        uint8_t     *data[3];
        int         strides[3];
    }               data;
    int             width;
    int             height;
} qsv_memory_copy_t;

int qsv_nv12_to_yuv420(struct SwsContext* sws_context,hb_buffer_t* dst, mfxFrameSurface1* src,mfxCoreInterface *core);
int qsv_yuv420_to_nv12(struct SwsContext* sws_context,mfxFrameSurface1* dst, hb_buffer_t* src);

#endif // QSV_MEMORY_H
