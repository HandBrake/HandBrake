/* vadxva2.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */

#ifdef USE_HWD

#ifndef HB_VA_DXVA2_H
#define HB_VA_DXVA2_H

#include "hbffmpeg.h"
#include "d3d9.h"
#include "libavcodec/dxva2.h"
#include "dxva2api.h"
#include "common.h"
#include "opencl.h"
#include "openclwrapper.h"

#define HB_FOURCC( a, b, c, d ) ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#define MAKEFOURCC( a, b, c, d ) ((DWORD)(BYTE)(a) | ((DWORD)(BYTE)(b) << 8) | ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24 ))
#define HB_CODEC_YV12 HB_FOURCC( 'Y', 'V', '1', '2' )
#define HB_CODEC_NV12 HB_FOURCC( 'N', 'V', '1', '2' )
#define DXVA2_E_NOT_INITIALIZED     MAKE_HRESULT( 1, 4, 4096 )
#define DXVA2_E_NEW_VIDEO_DEVICE    MAKE_HRESULT( 1, 4, 4097 )
#define DXVA2_E_VIDEO_DEVICE_LOCKED MAKE_HRESULT( 1, 4, 4098 )
#define DXVA2_E_NOT_AVAILABLE       MAKE_HRESULT( 1, 4, 4099 )
#define VA_DXVA2_MAX_SURFACE_COUNT (64)

static const GUID DXVA_NoEncrypt = { 0x1b81bed0, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID IID_IDirectXVideoDecoderService = {0xfc51a551, 0xd5e7, 0x11d9, {0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02}};
static const GUID DXVA2_ModeMPEG2_MoComp = { 0xe6a9f44b, 0x61b0, 0x4563, {0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66} };
static const GUID DXVA2_ModeMPEG2_IDCT = { 0xbf22ad00, 0x03ea, 0x4690, {0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e} };
static const GUID DXVA2_ModeMPEG2_VLD = { 0xee27417f, 0x5e28, 0x4e65, {0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9} };
static const GUID DXVA2_ModeH264_A = { 0x1b81be64, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeH264_B = { 0x1b81be65, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeH264_C = { 0x1b81be66, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeH264_D = { 0x1b81be67, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeH264_E = { 0x1b81be68, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeH264_F = { 0x1b81be69, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVADDI_Intel_ModeH264_A = { 0x604F8E64, 0x4951, 0x4c54, {0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6} };
static const GUID DXVADDI_Intel_ModeH264_C = { 0x604F8E66, 0x4951, 0x4c54, {0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6} };
static const GUID DXVADDI_Intel_ModeH264_E = { 0x604F8E68, 0x4951, 0x4c54, {0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6} };
static const GUID DXVA2_ModeWMV8_A = { 0x1b81be80, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeWMV8_B = { 0x1b81be81, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeWMV9_A = { 0x1b81be90, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeWMV9_B = { 0x1b81be91, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeWMV9_C = { 0x1b81be94, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeVC1_A = { 0x1b81beA0, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeVC1_B = { 0x1b81beA1, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeVC1_C = { 0x1b81beA2, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeVC1_D = { 0x1b81beA3, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID DXVA2_ModeHEVC_VLD_Main = { 0x5b11d51b, 0x2f4c,0x4452, {0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0} };

typedef struct
{
    LPDIRECT3DSURFACE9 d3d;
    int                refcount;
    unsigned int       order;

} hb_va_surface_t;

typedef struct
{
    uint8_t *base;
    uint8_t *buffer;
    size_t  size;

} hb_copy_cache_t;

typedef struct
{
    const char *name;
    D3DFORMAT  format;
    uint32_t   codec;

} hb_d3d_format_t;

typedef struct
{
    const char *name;
    const GUID *guid;
    int        codec;
} hb_dx_mode_t;

typedef struct
{
    char                            *description;
    int                             codec_id;
    uint32_t                        i_chroma;
    int                             width;
    int                             height;
    HINSTANCE                       hd3d9_dll;
    HINSTANCE                       hdxva2_dll;
    D3DPRESENT_PARAMETERS           d3dpp;
    LPDIRECT3D9                     d3dobj;
    D3DADAPTER_IDENTIFIER9          d3dai;
    LPDIRECT3DDEVICE9               d3ddev;
    UINT                            token;
    IDirect3DDeviceManager9         *devmng;
    HANDLE                          device;
    IDirectXVideoDecoderService     *vs;
    GUID                            input;
    D3DFORMAT                       render;
    DXVA2_ConfigPictureDecode       cfg;
    IDirectXVideoDecoder            *decoder;
    D3DFORMAT                       output;
    struct dxva_context             hw;
    unsigned                        surface_count;
    unsigned                        surface_order;
    int                             surface_width;
    int                             surface_height;
    uint32_t                        surface_chroma;
    hb_va_surface_t                 surface[VA_DXVA2_MAX_SURFACE_COUNT];
    LPDIRECT3DSURFACE9              hw_surface[VA_DXVA2_MAX_SURFACE_COUNT];
    IDirectXVideoProcessorService   *ps;
    IDirectXVideoProcessor          *vp;
    int64_t input_pts[2];
    int64_t input_dts;
    int     do_job;

    // running nv12toyuv kernel.
    cl_kernel nv12toyuv;
    cl_mem cl_mem_nv12;
    cl_mem cl_mem_yuv;
    uint8_t * nv12toyuv_tmp_in;
    uint8_t * nv12toyuv_tmp_out;
} hb_va_dxva2_t;

typedef struct FilterLink_T
{
    cl_mem cl_inbuf;
    cl_mem cl_outbuf;

    uint8_t *mem_inbuf;
    uint8_t *mem_outbuf;
    int width;
    int height;
    int linesizeY;
    int linesizeUV;
    int inmemdataflag;
    int outmemdataflag;
    int incldataflag;
    int outcldataflag;
    int framenum;
    int outputSize; 
} T_FilterLink;

static const hb_d3d_format_t d3d_formats[] =
{
    { "YV12",   MAKEFOURCC( 'Y', 'V', '1', '2' ),    HB_CODEC_YV12 },
    { "NV12",   MAKEFOURCC( 'N', 'V', '1', '2' ),    HB_CODEC_NV12 },
    { NULL, 0, 0 }
};

static const hb_dx_mode_t dxva2_modes[] =
{
    { "DXVA2_ModeMPEG2_VLD",    &DXVA2_ModeMPEG2_VLD,     AV_CODEC_ID_MPEG2VIDEO },
    { "DXVA2_ModeMPEG2_MoComp", &DXVA2_ModeMPEG2_MoComp,  0 },
    { "DXVA2_ModeMPEG2_IDCT",   &DXVA2_ModeMPEG2_IDCT,    0 },

    { "H.264 variable-length decoder (VLD), FGT",               &DXVA2_ModeH264_F,         AV_CODEC_ID_H264 },
    { "H.264 VLD, no FGT",                                      &DXVA2_ModeH264_E,         AV_CODEC_ID_H264 },
    { "H.264 VLD, no FGT (Intel)",                              &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },
    { "H.264 IDCT, FGT",                                        &DXVA2_ModeH264_D,         0             },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT", &DXVA2_ModeH264_C,         0             },
    { "H.264 inverse discrete cosine transform (IDCT), no FGT (Intel)", &DXVADDI_Intel_ModeH264_C, 0     },
    { "H.264 MoComp, FGT",                                      &DXVA2_ModeH264_B,         0             },
    { "H.264 motion compensation (MoComp), no FGT",             &DXVA2_ModeH264_A,         0             },
    { "H.264 motion compensation (MoComp), no FGT (Intel)",     &DXVADDI_Intel_ModeH264_A, 0             },

    { "Windows Media Video 8 MoComp",           &DXVA2_ModeWMV8_B, 0 },
    { "Windows Media Video 8 post processing",  &DXVA2_ModeWMV8_A, 0 },

    {  "Windows Media Video 9 IDCT",            &DXVA2_ModeWMV9_C, 0 },
    {  "Windows Media Video 9 MoComp",          &DXVA2_ModeWMV9_B, 0 },
    {  "Windows Media Video 9 post processing", &DXVA2_ModeWMV9_A, 0 },

    { "VC-1 VLD",             &DXVA2_ModeVC1_D, AV_CODEC_ID_VC1 },
    { "VC-1 VLD",             &DXVA2_ModeVC1_D, AV_CODEC_ID_WMV3 },
    { "VC-1 IDCT",            &DXVA2_ModeVC1_C, 0 },
    { "VC-1 MoComp",          &DXVA2_ModeVC1_B, 0 },
    { "VC-1 post processing", &DXVA2_ModeVC1_A, 0 },
    
    { "DXVA2_ModeHEVC_VLD_Main", &DXVA2_ModeHEVC_VLD_Main,  AV_CODEC_ID_HEVC },

    { NULL, NULL, 0 }
};

int hb_va_get_frame_buf( hb_va_dxva2_t *dxva2, AVCodecContext *p_context, AVFrame *frame );
int hb_va_extract( hb_va_dxva2_t *dxva2, uint8_t *dst, AVFrame *frame, int job_w, int job_h, int *crop, hb_oclscale_t *os, int use_opencl, int use_decomb, int use_detelecine );
enum PixelFormat hb_ffmpeg_get_format( AVCodecContext *, const enum PixelFormat * );
hb_va_dxva2_t *hb_va_create_dxva2( hb_va_dxva2_t *dxva2, int codec_id );
void hb_va_new_dxva2( hb_va_dxva2_t *dxva2, AVCodecContext *p_context );
void hb_va_release( hb_va_dxva2_t *dxva2, AVFrame *frame );
void  hb_va_close( hb_va_dxva2_t *dxva2 );
int hb_check_hwd_fmt( int fmt );

#endif // HB_VA_DXVA2_H

#endif // USE_HWD
