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

#ifndef HANDBRAKE_QSV_LIBAV_H
#define HANDBRAKE_QSV_LIBAV_H

#include <stdint.h>
#include <string.h>
#include "vpl/mfxvideo.h"
#include "vpl/mfxdispatcher.h"
#include "libavutil/mem.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"

#if defined (__GNUC__)
#include <pthread.h>
#define ff_qsv_atomic_inc(ptr) __sync_add_and_fetch(ptr,1)
#define ff_qsv_atomic_dec(ptr) __sync_sub_and_fetch (ptr,1)
#elif HAVE_WINDOWS_H            // MSVC case
#include <windows.h>
#if HAVE_PTHREADS
#include <pthread.h>
#elif HAVE_W32THREADS
#include "w32pthreads.h"
#endif
#define ff_qsv_atomic_inc(ptr) InterlockedIncrement(ptr)
#define ff_qsv_atomic_dec(ptr) InterlockedDecrement (ptr)
#endif

#ifndef HB_QSV_PRINT_RET_MSG
#define HB_QSV_PRINT_RET_MSG(ERR)              { fprintf(stderr, "Error code %d,\t%s\t%d\n", ERR, __FUNCTION__, __LINE__); }
#endif

#ifndef HB_QSV_DEBUG_ASSERT
#define HB_QSV_DEBUG_ASSERT(x,y)               { if ((x)) { fprintf(stderr, "\nASSERT: %s\n", y); } }
#endif

#define HB_QSV_CHECK_RET(P, X, ERR)                {if ((X) > (P)) {HB_QSV_PRINT_RET_MSG(ERR); return;}}
#define HB_QSV_CHECK_RESULT(P, X, ERR)             {if ((X) > (P)) {HB_QSV_PRINT_RET_MSG(ERR); return ERR;}}
#define HB_QSV_CHECK_POINTER(P, ERR)               {if (!(P)) {HB_QSV_PRINT_RET_MSG(ERR); return ERR;}}
#define HB_QSV_IGNORE_MFX_STS(P, X)                {if ((X) == (P)) {P = MFX_ERR_NONE;}}

#define HB_QSV_ASYNC_DEPTH_DEFAULT     4

#define HB_QSV_AVC_DECODER_WIDTH_MAX   4096
#define HB_QSV_AVC_DECODER_HEIGHT_MAX  4096

// version of MSDK/QSV API currently used
#define HB_QSV_MSDK_VERSION_MAJOR  1
#define HB_QSV_MSDK_VERSION_MINOR  3

#define HB_QSV_FFMPEG_INITIAL_POOL_SIZE (0)
#define HB_QSV_FFMPEG_EXTRA_HW_FRAMES (60)

typedef struct hb_qsv_context {
    volatile int is_context_active;

    mfxIMPL impl;
    mfxSession mfx_session;
    mfxVersion ver;

    int la_is_enabled;
    int memory_type;
    int out_range;
    int dx_index;
    int full_path_is_enabled;
    const char *vpp_scale_mode;
    const char *vpp_interpolation_method;
    AVBufferRef *hb_ffmpeg_qsv_hw_frames_ctx;

    hb_display_t *display;
} hb_qsv_context;

typedef struct hb_qsv_config {
    /**
     * Set asynch depth of processing with QSV
     * Format: 0 and more
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int async_depth;

    /**
     * Set type of surfaces used with QSV
     * Format: "IOPattern enum" of Media SDK
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int io_pattern;
} hb_qsv_config;

void hb_qsv_add_context_usage(hb_qsv_context *, int);
int hb_qsv_context_clean(hb_qsv_context *, int);

/* @} */

#endif // HANDBRAKE_QSV_LIBAV_H
