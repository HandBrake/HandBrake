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

/**
 * @file
 * @ingroup lavc_codec_hwaccel_qsv
 * Common header for QSV/MediaSDK acceleration
 */

/**
 * @defgroup lavc_codec_hwaccel_qsv QSV/MediaSDK based Decode/Encode and VPP
 * @ingroup lavc_codec_hwaccel
 *
 *  As Intel Quick Sync Video (QSV) can decode/preprocess/encode with HW
 *  acceleration.
 *
 *  Supported features:
 *    - access:
 *      - format AV_PIX_FMT_QSV_H264, AVCodec decoder based implementation
 *      - name "h264_qsv", avcodec_find_decoder_by_name( "h264_qsv")
 *    - IO Pattern:
 *      - Opaque memory: MFX_IOPATTERN_OUT_OPAQUE_MEMORY // Video memory is
 *                       MFX_IMPL_HARDWARE or MFX_IMPL_AUTO and runtime support,
 *                       otherwise: System Memory
 *      - System memory: MFX_IOPATTERN_OUT_SYSTEM_MEMORY
 *    - Allocators:
 *      - default allocator for System memory: MFX_MEMTYPE_SYSTEM_MEMORY
 *    - details:
 *      implementation as "per frame"
 *
 *  TODO list:
 *    - access:
 *      - format AV_PIX_FMT_QSV_MPEG2
 *      - format AV_PIX_FMT_QSV_VC1
 *      - format AV_PIX_FMT_QSV, see "details" below
 *    - IO Pattern:
 *      - VIDEO_MEMORY  // MFX_IOPATTERN_OUT_VIDEO_MEMORY
 *    - Allocators:
 *      - Video memory: MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET /
 *                      MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET
 *    - details:
 *      "per slice" support: AV_PIX_FMT_QSV with AVHWAccel based implementation
 *
 *  Note hb_qsv_config struct required to fill in via
 *  AVCodecContext.hwaccel_context
 *
 *  As per frame, note AVFrame.data[2] (qsv_atom) used for frame atom id,
 *  data/linesize should be used together with SYSTEM_MEMORY and tested
 *
 *  Note: Compilation would require:
 *   - Intel MediaSDK headers, Full SDK is available from the original web site:
 *                     http://software.intel.com/en-us/vcsource/tools/media-SDK
 *     Will be referenced as mfx*.h (mfxdefs.h, mfxstructures.h, ... )
 *  and
 *  - Final application has to link against Intel MediaSDK dispatcher, available
 *     at MediaSDK as well
 *
 *  Target OS: as per available dispatcher and driver support
 *
 *  Implementation details:
 *   Provided struct hb_qsv_context contain several struct hb_qsv_space(s) for decode,
 *   VPP and encode.
 *   hb_qsv_space just contain needed environment for the appropriate action.
 *   Based on this - pipeline (see pipes) will be build to pass details such as
 *   mfxFrameSurface1* and mfxSyncPoint* from one action to the next.
 *
 *  Resources re-usage (hb_qsv_flush_stages):
 *     hb_qsv_context *qsv = (hb_qsv_context *)video_codec_ctx->priv_data;
 *     hb_qsv_list *pipe = (hb_qsv_list *)video_frame->data[2];
 *     hb_qsv_flush_stages( qsv->pipes, &pipe );
 *
 *  DTS re-usage:
 *     hb_qsv_dts_pop(qsv);
 *
 *   for video,DX9/11 memory it has to be Unlock'ed as well
 *
 *  Implementation is thread aware and uses synchronization point(s) from MediaSDK
 *  as per configuration.
 *
 *  For the details of MediaSDK usage and options available - please refer to the
 *  available documentation at MediaSDK.
 *
 *  Feature set used from MSDK is defined by HB_QSV_MSDK_VERSION_MAJOR and
 *  HB_QSV_MSDK_VERSION_MINOR
 *
 * @{
 */

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


// sleep is defined in milliseconds
#define hb_qsv_sleep(x) av_usleep((x)*1000)

#define HB_QSV_ZERO_MEMORY(VAR)                    {memset(&VAR, 0, sizeof(VAR));}
#define HB_QSV_ALIGN32(X)                      (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define HB_QSV_ALIGN16(value)                  (((value + 15) >> 4) << 4)
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

#define HB_QSV_ID_BUFFER MFX_MAKEFOURCC('B','U','F','F')
#define HB_QSV_ID_FRAME  MFX_MAKEFOURCC('F','R','M','E')

#define HB_QSV_SURFACE_NUM              80
#define HB_QSV_SYNC_NUM                 HB_QSV_SURFACE_NUM*3/4
#define HB_QSV_JOB_SIZE_DEFAULT         10
#define HB_QSV_SYNC_TIME_DEFAULT        10000
// see hb_qsv_get_free_sync, hb_qsv_get_free_surface , 100 if usleep(10*1000)(10ms) == 1 sec
#define HB_QSV_REPEAT_NUM_DEFAULT      100
#define HB_QSV_ASYNC_DEPTH_DEFAULT     4

#define HB_QSV_AVC_DECODER_WIDTH_MAX   4096
#define HB_QSV_AVC_DECODER_HEIGHT_MAX  4096

// version of MSDK/QSV API currently used
#define HB_QSV_MSDK_VERSION_MAJOR  1
#define HB_QSV_MSDK_VERSION_MINOR  3

typedef enum HB_QSV_STAGE_TYPE {

#define HB_QSV_DECODE_MASK   0x001
    HB_QSV_DECODE   = 0x001,

#define HB_QSV_VPP_MASK      0x0F0
    // "Mandatory VPP filter" , might be with "Hint-based VPP filters"
    HB_QSV_VPP_DEFAULT = 0x010,
    // "User Modules" etc
    HB_QSV_VPP_USER = 0x020,

#define av_QSV_ENCODE_MASK   0x100
    HB_QSV_ENCODE   = 0x100
#define HB_QSV_ANY_MASK      0xFFF
} HB_QSV_STAGE_TYPE;

typedef struct QSVMid {
    AVBufferRef *hw_frames_ref;
    mfxHDLPair *handle_pair;

    AVFrame *locked_frame;
    AVFrame *hw_frame;
    mfxFrameSurface1 surf;
} QSVMid;

typedef struct QSVFrame {
    AVFrame *frame;
    mfxFrameSurface1 surface;
    mfxEncodeCtrl enc_ctrl;
    mfxExtDecodedFrameInfo dec_info;
    mfxExtBuffer *ext_param;

    int queued;
    int used;

    struct QSVFrame *next;
} QSVFrame;

#define HB_QSV_FFMPEG_INITIAL_POOL_SIZE (0)
#define HB_QSV_FFMPEG_EXTRA_HW_FRAMES (60)

typedef struct HBQSVFramesContext {
    AVBufferRef *hw_frames_ctx;
} HBQSVFramesContext;

typedef struct hb_qsv_list {
    // practically pthread_mutex_t
    void *mutex;
    pthread_mutexattr_t   mta;

    void **items;
    int items_alloc;

    int items_count;
} hb_qsv_list;

typedef struct hb_qsv_sync {
    mfxSyncPoint*   p_sync;
    int             in_use;
} hb_qsv_sync;

typedef struct hb_qsv_stage {
    HB_QSV_STAGE_TYPE type;
    struct {
        mfxBitstream *p_bs;
        mfxFrameSurface1 *p_surface;
        HBQSVFramesContext *p_frames_ctx;
    } in;
    struct {
        mfxBitstream *p_bs;
        mfxFrameSurface1 *p_surface;
        hb_qsv_sync *sync;
    } out;
    hb_qsv_list *pending;
} hb_qsv_stage;

typedef struct hb_qsv_task {
    mfxBitstream *bs;
    hb_qsv_stage *stage;
} hb_qsv_task;


typedef struct hb_qsv_space {

    uint8_t is_init_done;

    HB_QSV_STAGE_TYPE type;

    mfxVideoParam m_mfxVideoParam;

    mfxFrameAllocResponse response;
    mfxFrameAllocRequest request[2];    // [0] - in, [1] - out, if needed

    mfxExtBuffer **p_ext_params;
    uint16_t p_ext_param_num;

    uint16_t surface_num_max_used;
    uint16_t surface_num;
    mfxFrameSurface1 *p_surfaces[HB_QSV_SURFACE_NUM];

    uint16_t sync_num_max_used;
    uint16_t sync_num;
    hb_qsv_sync *p_syncp[HB_QSV_SYNC_NUM];

    mfxBitstream bs;
    uint8_t *p_buf;
    size_t p_buf_max_size;

    // only for encode and tasks
    hb_qsv_list *tasks;

    hb_qsv_list *pending;

    // storage for allocations/mfxMemId*
    mfxMemId *mids;
} hb_qsv_space;

typedef struct hb_qsv_context {
    volatile int is_context_active;

    mfxIMPL impl;
    mfxSession mfx_session;
    mfxVersion ver;

    // decode
    hb_qsv_space *dec_space;
    // encode
    hb_qsv_space *enc_space;
    // vpp
    hb_qsv_list *vpp_space;

    hb_qsv_list *pipes;

    // MediaSDK starting from API version 1.6 includes DecodeTimeStamp
    // in addition to TimeStamp
    // see also HB_QSV_MSDK_VERSION_MINOR , HB_QSV_MSDK_VERSION_MAJOR
    hb_qsv_list *dts_seq;

    // practically pthread_mutex_t
    void *qts_seq_mutex;

    int is_anex;

    void *qsv_config;

    int num_sw_filters;
    int num_hw_filters;
    int la_is_enabled;
    int qsv_hw_filters_via_video_memory_are_enabled;
    int qsv_hw_filters_via_system_memory_are_enabled;
    int memory_type;
    int out_range;
    int full_path_is_enabled;
    const char *vpp_scale_mode;
    const char *vpp_interpolation_method;
    int dx_index;
    AVBufferRef *hb_hw_device_ctx;
    AVBufferRef *hb_ffmpeg_qsv_hw_frames_ctx;
    HBQSVFramesContext *hb_dec_qsv_frames_ctx;
    HBQSVFramesContext *hb_vpp_qsv_frames_ctx;

    mfxHDL device_manager_handle;
    mfxHandleType device_manager_handle_type;
    void *device_context;
    hb_display_t *display;
} hb_qsv_context;

typedef enum {
    QSV_PART_ANY = 0,
    QSV_PART_LOWER,
    QSV_PART_UPPER
} hb_qsv_split;

typedef struct {
    int64_t dts;
} hb_qsv_dts;

typedef struct hb_qsv_alloc_frame {
    mfxU32 id;
    mfxFrameInfo info;
} hb_qsv_alloc_frame;

typedef struct hb_qsv_alloc_buffer {
    mfxU32 id;
    mfxU32 nbytes;
    mfxU16 type;
} hb_qsv_alloc_buffer;

typedef struct hb_qsv_allocators_space {
    hb_qsv_space *space;
    mfxFrameAllocator frame_alloc;
} hb_qsv_allocators_space;

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
     * Range of numbers that indicate trade-offs between quality and speed.
     * Format: from 1/MFX_TARGETUSAGE_BEST_QUALITY to 7/MFX_TARGETUSAGE_BEST_SPEED inclusive
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int target_usage;

    /**
     * Number of reference frames; if NumRefFrame = 0, this parameter is not specified.
     * Format: 0 and more
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
    int num_ref_frame;

    /**
     * Distance between I- or P- key frames; if it is zero, the GOP structure is unspecified.
     * Note: If GopRefDist = 1, there are no B-frames used.
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
     int gop_ref_dist;

    /**
     * Number of pictures within the current GOP (Group of Pictures); if GopPicSize=0,
     * then the GOP size is unspecified. If GopPicSize=1, only I-frames are used.
     *
     * - encoding: Set by user.
     * - decoding: unused
     */
     int gop_pic_size;

    /**
     * Set type of surfaces used with QSV
     * Format: "IOPattern enum" of Media SDK
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int io_pattern;

    /**
     * Set amount of additional surfaces might be needed
     * Format: amount of additional buffers(surfaces+syncs)
     * to allocate in advance
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int additional_buffers;

    /**
     * If pipeline should be sync.
     * Format: wait time in milliseconds,
     *         HB_QSV_SYNC_TIME_DEFAULT/10000 might be a good value
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int sync_need;

    /**
     * Type of implementation needed
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int impl_requested;

    /**
     * if QSV usage is multithreaded.
     * Format: Yes/No, 1/0
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    int usage_threaded;

    /**
     * if QSV use an external allocation (valid per session/mfxSession)
     * Format: pointer to allocators, if default: 0
     *
     * note that:
     * System Memory:   can be used without provided and external allocator,
     *  meaning MediaSDK will use an internal one
     * Video Memory:    in this case - we must provide an external allocator
     * Also, Media SDK session doesn't require external allocator if the application
     *  uses opaque memory
     *
     * Calls SetFrameAllocator/SetBufferAllocator
     * (MFXVideoCORE_SetFrameAllocator/MFXVideoCORE_SetBufferAllocator)
     * are to pass allocators to Media SDK
     *
     * - encoding: Set by user.
     * - decoding: Set by user.
     */
    hb_qsv_allocators_space *allocators;

} hb_qsv_config;

#define ANEX_UNKNOWN    0
#define ANEX_PREFIX     1
#define ANEX_NO_PREFIX  2

static const uint8_t ff_prefix_code[] = { 0x00, 0x00, 0x00, 0x01 };

int hb_qsv_get_free_sync(hb_qsv_space *, hb_qsv_context *);
int hb_qsv_get_free_surface(hb_qsv_space *, hb_qsv_context *, mfxFrameInfo *,
                     hb_qsv_split);
int hb_qsv_get_free_encode_task(hb_qsv_list *);

int hb_qsv_wait_on_sync(hb_qsv_context *, hb_qsv_stage *);

void hb_qsv_add_context_usage(hb_qsv_context *, int);

void hb_qsv_pipe_list_create(hb_qsv_list **, int);
void hb_qsv_pipe_list_clean(hb_qsv_list **);

void hb_qsv_add_stagee(hb_qsv_list **, hb_qsv_stage *, int);
hb_qsv_stage *hb_qsv_get_last_stage(hb_qsv_list *);
hb_qsv_list *hb_qsv_pipe_by_stage(hb_qsv_list *, hb_qsv_stage *);
void hb_qsv_flush_stages(hb_qsv_list *, hb_qsv_list **, int);

void hb_qsv_dts_ordered_insert(hb_qsv_context *, int, int, int64_t, int);
void hb_qsv_dts_pop(hb_qsv_context *);

hb_qsv_stage *hb_qsv_stage_init(void);
void hb_qsv_stage_clean(hb_qsv_stage **, int);
int hb_qsv_context_clean(hb_qsv_context *, int);

int ff_qsv_is_sync_in_pipe(mfxSyncPoint *, hb_qsv_context *);
int ff_qsv_is_surface_in_pipe(mfxFrameSurface1 *, hb_qsv_context *);

hb_qsv_list *hb_qsv_list_init(int);
int hb_qsv_list_lock(hb_qsv_list *);
int hb_qsv_list_unlock(hb_qsv_list *);
int hb_qsv_list_add(hb_qsv_list *, void *);
void hb_qsv_list_rem(hb_qsv_list *, void *);
void hb_qsv_list_insert(hb_qsv_list *, int, void *);
void hb_qsv_list_close(hb_qsv_list **);

int hb_qsv_list_count(hb_qsv_list *);
void *hb_qsv_list_item(hb_qsv_list *, int);

/* @} */

#endif // HANDBRAKE_QSV_LIBAV_H
