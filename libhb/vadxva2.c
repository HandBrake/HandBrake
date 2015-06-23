/* vadxva2.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */

#ifdef USE_HWD

#include "vadxva2.h"
#include "extras/cl.h"
#include "oclnv12toyuv.h"

static int   hb_va_setup( hb_va_dxva2_t *dxva2, void **hw, int width, int height );
static int   hb_va_get( hb_va_dxva2_t *dxva2, AVFrame *frame );
static int   hb_d3d_create_device( hb_va_dxva2_t *dxva2 );
static void  hb_d3d_destroy_device( hb_va_dxva2_t *dxvva2 );
static int   hb_d3d_create_device_manager( hb_va_dxva2_t *dxva2 );
static void  hb_d3d_destroy_device_manager( hb_va_dxva2_t *dxva2 );
static int   hb_dx_create_video_service( hb_va_dxva2_t *dxva2 );
static void  hb_dx_destroy_video_service( hb_va_dxva2_t *dxva2 );
static int   hb_dx_find_video_service_conversion( hb_va_dxva2_t *dxva2, GUID *input, D3DFORMAT *output );
static int   hb_dx_create_video_decoder( hb_va_dxva2_t *dxva2, int codec_id, const  hb_title_t* fmt );
static void  hb_dx_create_video_conversion( hb_va_dxva2_t *dxva2 );
static const hb_d3d_format_t *hb_d3d_find_format( D3DFORMAT format );
static const hb_dx_mode_t *hb_dx_find_mode( const GUID *guid );
static void hb_dx_destroy_video_decoder( hb_va_dxva2_t *dxva2 );
/**
 * It destroys a Direct3D device manager
 */
static void hb_d3d_destroy_device_manager( hb_va_dxva2_t *dxva2 )
{
    if( dxva2->devmng )
        IDirect3DDeviceManager9_Release( dxva2->devmng );
}
/**
 * It releases a Direct3D device and its resources.
 */
static void hb_d3d_destroy_device( hb_va_dxva2_t *dxva2 )
{
    if( dxva2->d3ddev )
        IDirect3DDevice9_Release( dxva2->d3ddev );
    if( dxva2->d3dobj )
        IDirect3D9_Release( dxva2->d3dobj );
}
/**
 * It destroys a DirectX video service
 */
static void hb_dx_destroy_video_service( hb_va_dxva2_t *dxva2 )
{
    if( dxva2->device )
        IDirect3DDeviceManager9_CloseDeviceHandle( dxva2->devmng, dxva2->device );

    if( dxva2->vs )
        IDirectXVideoDecoderService_Release( dxva2->vs );
}

static const hb_d3d_format_t *hb_d3d_find_format( D3DFORMAT format )
{
    unsigned i;
    for( i = 0; d3d_formats[i].name; i++ )
    {
        if( d3d_formats[i].format == format )
            return &d3d_formats[i];
    }
    return NULL;
}

static void hb_dx_create_video_conversion( hb_va_dxva2_t *dxva2 )
{
    switch( dxva2->render )
    {
        case MAKEFOURCC( 'N', 'V', '1', '2' ):
            dxva2->output = MAKEFOURCC( 'Y', 'V', '1', '2' );
            break;
        default:
            dxva2->output = dxva2->render;
            break;
    }
}

void hb_va_release( hb_va_dxva2_t *dxva2, AVFrame *frame )
{
    LPDIRECT3DSURFACE9 d3d = (LPDIRECT3DSURFACE9)(uintptr_t)frame->data[3];
    unsigned i;
    for( i = 0; i < dxva2->surface_count; i++ )
    {
        hb_va_surface_t *surface = &dxva2->surface[i];
        if( surface->d3d == d3d )
            surface->refcount--;
    }
}


void hb_va_close( hb_va_dxva2_t *dxva2 )
{
    hb_dx_destroy_video_decoder( dxva2 );
    hb_dx_destroy_video_service( dxva2 );
    hb_d3d_destroy_device_manager( dxva2 );
    hb_d3d_destroy_device( dxva2 );

    if( dxva2->hdxva2_dll )
        FreeLibrary( dxva2->hdxva2_dll );
    if( dxva2->hd3d9_dll )
        FreeLibrary( dxva2->hd3d9_dll );

    if ( dxva2->nv12toyuv_tmp_in )
        free( dxva2->nv12toyuv_tmp_in );
    if ( dxva2->nv12toyuv_tmp_out )
        free( dxva2->nv12toyuv_tmp_out );

    dxva2->description = NULL;
    free( dxva2 );
}

/**
 * It creates a DXVA2 decoder using the given video format
 */
static int hb_dx_create_video_decoder( hb_va_dxva2_t *dxva2, int codec_id, const  hb_title_t* fmt )
{
    int surface_alignment;
    dxva2->width = fmt->geometry.width;
    dxva2->height = fmt->geometry.height;

    switch( codec_id )
    {
        case AV_CODEC_ID_H264:
            dxva2->surface_count = 16 + 1;
            surface_alignment = 16;
            break;
        case AV_CODEC_ID_HEVC:
            dxva2->surface_count = 16 + 1;
            surface_alignment = 128;
            break;
        case AV_CODEC_ID_MPEG2VIDEO:
            dxva2->surface_count = 2 + 1;
            surface_alignment = 32;
            break;
        default:
            dxva2->surface_count = 2 + 1;
            surface_alignment = 16;
            break;
    }
    
    dxva2->surface_width  = HB_ALIGN(fmt->geometry.width,  surface_alignment);
    dxva2->surface_height = HB_ALIGN(fmt->geometry.height, surface_alignment);
    
    LPDIRECT3DSURFACE9 surface_list[VA_DXVA2_MAX_SURFACE_COUNT];
    if( FAILED( IDirectXVideoDecoderService_CreateSurface( dxva2->vs,
                                                           dxva2->surface_width,
                                                           dxva2->surface_height,
                                                           dxva2->surface_count - 1,
                                                           dxva2->render,
                                                           D3DPOOL_DEFAULT,
                                                           0,
                                                           DXVA2_VideoDecoderRenderTarget,
                                                           surface_list, NULL )))
    {
        hb_log( "dxva2:IDirectXVideoAccelerationService_CreateSurface failed" );
        dxva2->surface_count = 0;
        return HB_WORK_ERROR;
    }

    unsigned i;
    for( i = 0; i<dxva2->surface_count; i++ )
    {
        hb_va_surface_t *surface = &dxva2->surface[i];
        surface->d3d = surface_list[i];
        surface->refcount = 0;
        surface->order = 0;
    }
    hb_log( "dxva2:CreateSurface succeed with %d, fmt (%dx%d) surfaces (%dx%d)", dxva2->surface_count,
            fmt->geometry.width, fmt->geometry.height, dxva2->surface_width, dxva2->surface_height );
    DXVA2_VideoDesc dsc;
    memset( &dsc, 0, sizeof(dsc));
    dsc.SampleWidth = fmt->geometry.width;
    dsc.SampleHeight = fmt->geometry.height;
    dsc.Format = dxva2->render;

    if( fmt->vrate.num > 0 && fmt->vrate.den > 0 )
    {
        dsc.InputSampleFreq.Numerator = fmt->vrate.num;
        dsc.InputSampleFreq.Denominator = fmt->vrate.den;
    }
    else
    {
        dsc.InputSampleFreq.Numerator = 0;
        dsc.InputSampleFreq.Denominator = 0;
    }

    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;

    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0; //DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0; //DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0; //DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0; //DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0; //DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0; //DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0; //DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    if( FAILED( IDirectXVideoDecoderService_GetDecoderConfigurations( dxva2->vs, &dxva2->input, &dsc, NULL, &cfg_count, &cfg_list )))
    {
        hb_log( "dxva2:IDirectXVideoDecoderService_GetDecoderConfigurations failed" );
        return HB_WORK_ERROR;
    }
    hb_log( "dxva2:we got %d decoder configurations", cfg_count );

    /* Select the best decoder configuration */
    int cfg_score = 0;
    for( i = 0; i < cfg_count; i++ )
    {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];
        hb_log( "dxva2:configuration[%d] ConfigBitstreamRaw %d", i, cfg->ConfigBitstreamRaw );
        int score;
        if( cfg->ConfigBitstreamRaw == 1 )
            score = 1;
        else if( codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2 )
            score = 2;
        else
            continue;
        if( IsEqualGUID( &cfg->guidConfigBitstreamEncryption, &DXVA_NoEncrypt ))
            score += 16;
        if( cfg_score < score )
        {
            dxva2->cfg = *cfg;
            cfg_score = score;
        }
    }
    //my_release(cfg_list);
    if( cfg_score <= 0 )
    {
        hb_log( "dxva2:Failed to find a supported decoder configuration" );
        return HB_WORK_ERROR;
    }

    /* Create the decoder */
    IDirectXVideoDecoder *decoder;
    if( FAILED( IDirectXVideoDecoderService_CreateVideoDecoder( dxva2->vs, &dxva2->input, &dsc, &dxva2->cfg, surface_list, dxva2->surface_count, &decoder )))
    {
        hb_log( "dxva2:IDirectXVideoDecoderService_CreateVideoDecoder failed" );
        return HB_WORK_ERROR;
    }
    dxva2->decoder = decoder;
    hb_log( "dxva2:IDirectXVideoDecoderService_CreateVideoDecoder succeed" );
    return HB_WORK_OK;
}

typedef HWND (WINAPI *PROCGETSHELLWND)();
/**
 * It creates a DirectX video service
 */
static int hb_d3d_create_device( hb_va_dxva2_t *dxva2 )
{
    LPDIRECT3D9 (WINAPI *Create9)( UINT SDKVersion );
    Create9 = (void*)GetProcAddress( dxva2->hd3d9_dll, TEXT( "Direct3DCreate9" ));
    if( !Create9 )
    {
        hb_log( "dxva2:Cannot locate reference to Direct3DCreate9 ABI in DLL" );
        return HB_WORK_ERROR;
    }
    LPDIRECT3D9 d3dobj;
    d3dobj = Create9( D3D_SDK_VERSION );
    if( !d3dobj )
    {
        hb_log( "dxva2:Direct3DCreate9 failed" );
        return HB_WORK_ERROR;
    }
    dxva2->d3dobj = d3dobj;
    D3DADAPTER_IDENTIFIER9 *d3dai = &dxva2->d3dai;
    if( FAILED( IDirect3D9_GetAdapterIdentifier( dxva2->d3dobj, D3DADAPTER_DEFAULT, 0, d3dai )))
    {
        hb_log( "dxva2:IDirect3D9_GetAdapterIdentifier failed" );
        memset( d3dai, 0, sizeof(*d3dai));
    }

    PROCGETSHELLWND GetShellWindow;
    HMODULE hUser32 = GetModuleHandle( "user32" );
    GetShellWindow = (PROCGETSHELLWND)
                     GetProcAddress( hUser32, "GetShellWindow" );

    D3DPRESENT_PARAMETERS *d3dpp = &dxva2->d3dpp;
    memset( d3dpp, 0, sizeof(*d3dpp));
    d3dpp->Flags                  = D3DPRESENTFLAG_VIDEO;
    d3dpp->Windowed               = TRUE;
    d3dpp->hDeviceWindow          = NULL;
    d3dpp->SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp->MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3dpp->PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp->BackBufferCount        = 0;                  /* FIXME what to put here */
    d3dpp->BackBufferFormat       = D3DFMT_X8R8G8B8;    /* FIXME what to put here */
    d3dpp->BackBufferWidth        = 0;
    d3dpp->BackBufferHeight       = 0;
    d3dpp->EnableAutoDepthStencil = FALSE;

    LPDIRECT3DDEVICE9 d3ddev;
    //if (FAILED(IDirect3D9_CreateDevice(d3dobj, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetShellWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, d3dpp, &d3ddev)))
    if( FAILED( IDirect3D9_CreateDevice( d3dobj,
                                         D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL,
                                         GetShellWindow(),
                                         D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED,
                                         d3dpp,
                                         &d3ddev )))
    {
        hb_log( "dxva2:IDirect3D9_CreateDevice failed" );
        return HB_WORK_ERROR;
    }
    dxva2->d3ddev = d3ddev;

    return HB_WORK_OK;
}
/**
 * It creates a Direct3D device manager
 */
static int hb_d3d_create_device_manager( hb_va_dxva2_t *dxva2 )
{
    HRESULT(WINAPI *CreateDeviceManager9)( UINT *pResetToken, IDirect3DDeviceManager9 ** );
    CreateDeviceManager9 = (void*)GetProcAddress( dxva2->hdxva2_dll, TEXT( "DXVA2CreateDirect3DDeviceManager9" ));

    if( !CreateDeviceManager9 )
    {
        hb_log( "dxva2:cannot load function" );
        return HB_WORK_ERROR;
    }

    UINT token;
    IDirect3DDeviceManager9 *devmng;
    if( FAILED( CreateDeviceManager9( &token, &devmng )))
    {
        hb_log( "dxva2:OurDirect3DCreateDeviceManager9 failed" );
        return HB_WORK_ERROR;
    }
    dxva2->token = token;
    dxva2->devmng = devmng;

    long hr = IDirect3DDeviceManager9_ResetDevice( devmng, dxva2->d3ddev, token );
    if( FAILED( hr ))
    {
        hb_log( "dxva2:IDirect3DDeviceManager9_ResetDevice failed: %08x", (unsigned)hr );
        return HB_WORK_ERROR;
    }
    return HB_WORK_OK;
}
/**
 * It creates a DirectX video service
 */
static int hb_dx_create_video_service( hb_va_dxva2_t *dxva2 )
{
    HRESULT (WINAPI *CreateVideoService)( IDirect3DDevice9 *, REFIID riid, void **ppService );
    CreateVideoService = (void*)GetProcAddress( dxva2->hdxva2_dll, TEXT( "DXVA2CreateVideoService" ));

    if( !CreateVideoService )
    {
        hb_log( "dxva2:cannot load function" );
        return HB_WORK_ERROR;
    }

    HRESULT hr;

    HANDLE device;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle( dxva2->devmng, &device );
    if( FAILED( hr ))
    {
        hb_log( "dxva2:OpenDeviceHandle failed" );
        return HB_WORK_ERROR;
    }
    dxva2->device = device;

    IDirectXVideoDecoderService *vs;
    hr = IDirect3DDeviceManager9_GetVideoService( dxva2->devmng, device, &IID_IDirectXVideoDecoderService, (void*)&vs );
    if( FAILED( hr ))
    {
        hb_log( "dxva2:GetVideoService failed" );
        return HB_WORK_ERROR;
    }
    dxva2->vs = vs;

    return HB_WORK_OK;
}
/**
 * Find the best suited decoder mode GUID and render format.
 */
static int hb_dx_find_video_service_conversion( hb_va_dxva2_t *dxva2, GUID *input, D3DFORMAT *output )
{
    unsigned int input_count = 0;
    GUID *input_list = NULL;
    if( FAILED( IDirectXVideoDecoderService_GetDecoderDeviceGuids( dxva2->vs, &input_count, &input_list )))
    {
        hb_log( "dxva2:IDirectXVideoDecoderService_GetDecoderDeviceGuids failed" );
        return HB_WORK_ERROR;
    }
    unsigned i, j;
    for( i = 0; i < input_count; i++ )
    {
        const GUID *g = &input_list[i];
        const hb_dx_mode_t *mode = hb_dx_find_mode( g );
     }

    for( i = 0; dxva2_modes[i].name; i++ )
    {
        const hb_dx_mode_t *mode = &dxva2_modes[i];
        if( !mode->codec || mode->codec != dxva2->codec_id )
            continue;

        int is_suported = 0;
        const GUID *g;
        for( g = &input_list[0]; !is_suported && g < &input_list[input_count]; g++ )
        {
            is_suported = IsEqualGUID( mode->guid, g );
        }
        if( !is_suported )
            continue;

        unsigned int output_count = 0;
        D3DFORMAT *output_list = NULL;
        if( FAILED( IDirectXVideoDecoderService_GetDecoderRenderTargets( dxva2->vs, mode->guid, &output_count, &output_list )))
        {
            hb_log( "dxva2:IDirectXVideoDecoderService_GetDecoderRenderTargets failed" );
            continue;
        }
        for( j = 0; j < output_count; j++ )
        {
            const D3DFORMAT f = output_list[j];
            const hb_d3d_format_t *format = hb_d3d_find_format( f );
            if( format )
            {
                //hb_log( "dxva2:%s is supported for output", format->name );
            }
            else
            {
                hb_log( "dxvar2:%d is supported for output (%4.4s)", f, (const char*)&f );
            }
        }

        for( j = 0; d3d_formats[j].name; j++ )
        {
            const hb_d3d_format_t *format = &d3d_formats[j];
            int is_suported = 0;
            unsigned k;
            for( k = 0; !is_suported && k < output_count; k++ )
            {
                is_suported = format->format == output_list[k];
            }
            if( !is_suported )
                continue;
            *input  = *mode->guid;
            *output = format->format;
            return HB_WORK_OK;
        }
    }
    return HB_WORK_ERROR;
}
static const hb_dx_mode_t *hb_dx_find_mode( const GUID *guid )
{
    unsigned i;
    for( i = 0; dxva2_modes[i].name; i++ )
    {
        if( IsEqualGUID( dxva2_modes[i].guid, guid ))
            return &dxva2_modes[i];
    }
    return NULL;
}


static void hb_dx_destroy_video_decoder( hb_va_dxva2_t *dxva2 )
{
    if( dxva2->decoder )
        IDirectXVideoDecoder_Release( dxva2->decoder );
    dxva2->decoder = NULL;

    unsigned i;
    for( i = 0; i<dxva2->surface_count; i++ )
        IDirect3DSurface9_Release( dxva2->surface[i].d3d );
    dxva2->surface_count = 0;
}
/**
 * setup dxva2
*/
static int hb_va_setup( hb_va_dxva2_t *dxva2, void **hw, int width, int height )
{
    if( dxva2->width == width && dxva2->height == height && dxva2->decoder )
        goto ok;

    hb_dx_destroy_video_decoder( dxva2 );
    *hw = NULL;
    dxva2->i_chroma = 0;

    if( width <= 0 || height <= 0 ) 
        return HB_WORK_ERROR;

    hb_title_t fmt;
    memset( &fmt, 0, sizeof(fmt));
    fmt.geometry.width = width;
    fmt.geometry.height = height;

    if( hb_dx_create_video_decoder( dxva2, dxva2->codec_id, &fmt ) == HB_WORK_ERROR )
        return HB_WORK_ERROR;
    dxva2->hw.decoder = dxva2->decoder;
    dxva2->hw.cfg = &dxva2->cfg;
    dxva2->hw.surface_count = dxva2->surface_count;
    dxva2->hw.surface = dxva2->hw_surface;

    unsigned i;
    for( i = 0; i < dxva2->surface_count; i++ )
        dxva2->hw.surface[i] = dxva2->surface[i].d3d;

    hb_dx_create_video_conversion( dxva2 );

ok:
    *hw = &dxva2->hw;
    const hb_d3d_format_t *output = hb_d3d_find_format( dxva2->output );
    dxva2->i_chroma = output->codec;
    return HB_WORK_OK;

}

static int hb_va_get( hb_va_dxva2_t *dxva2, AVFrame *frame )
{
    unsigned i, old;
    for( i = 0, old = 0; i < dxva2->surface_count; i++ )
    {
        hb_va_surface_t *surface = &dxva2->surface[i];
        if( !surface->refcount )
            break;
        if( surface->order < dxva2->surface[old].order )
            old = i;
    }
    if( i >= dxva2->surface_count )
        i = old;

    hb_va_surface_t *surface = &dxva2->surface[i];

    surface->refcount = 1;
    surface->order = dxva2->surface_order++;

    for( i = 0; i < 4; i++ )
    {
        frame->data[i] = NULL;
        frame->linesize[i] = 0;
        if( i == 0 || i == 3 )
            frame->data[i] = (void*)surface->d3d;
    }
    return HB_WORK_OK;
}
/**
 * nv12 to yuv of c reference
 */
static void hb_copy_from_nv12( uint8_t *dst, uint8_t *src[2], size_t src_pitch[2], unsigned width, unsigned height )
{
    unsigned int i, j;
    uint8_t *dstU, *dstV;
    dstU = dst + width*height;
    dstV = dstU + width*height/4;
    unsigned int heithtUV, widthUV;
    heithtUV = height/2;
    widthUV = width/2;

    for( i = 0; i < height; i++ ) //Y
    {
        memcpy( dst + i * width, src[0] + i * src_pitch[0], width );
    }
    for( i = 0; i < heithtUV; i++ )
    {
        for( j = 0; j < widthUV; j++ )
        {
            dstU[i * widthUV + j] = *(src[1] + i * src_pitch[1] + 2 * j);
            dstV[i  *widthUV + j] = *(src[1] + i * src_pitch[1] + 2 * j + 1);
        }
    }
}

/**
 *  lock frame data form surface.
 *  nv12 to yuv with opencl and with C reference
 *  scale with opencl
 */
int hb_va_extract( hb_va_dxva2_t *dxva2, uint8_t *dst, AVFrame *frame, int job_w, int job_h, int *crop, hb_oclscale_t *os, int use_opencl, int use_decomb, int use_detelecine )

{
    LPDIRECT3DSURFACE9 d3d = (LPDIRECT3DSURFACE9)(uintptr_t)frame->data[3];
    D3DLOCKED_RECT lock;
    if( FAILED( IDirect3DSurface9_LockRect( d3d, &lock, NULL, D3DLOCK_READONLY )))
    {
        hb_log( "dxva2:Failed to lock surface" );
        return HB_WORK_ERROR;
    }

    if( dxva2->render == MAKEFOURCC( 'N', 'V', '1', '2' ))
    {
        uint8_t *plane[2] =
        {
            lock.pBits,
            (uint8_t*)lock.pBits + lock.Pitch * dxva2->surface_height
        };
        size_t pitch[2] =
        {
            lock.Pitch,
            lock.Pitch,
        };

        hb_copy_from_nv12( dst, plane, pitch, dxva2->width, dxva2->height );
    }
    IDirect3DSurface9_UnlockRect( d3d );
    return HB_WORK_OK;
}

/**
 *  create dxva2 service
 *  load library D3D9.dll
 */
hb_va_dxva2_t * hb_va_create_dxva2( hb_va_dxva2_t *dxva2, int codec_id )
{
    if( dxva2 )
    {
        hb_va_close( dxva2 );
        dxva2 = NULL;
    }

    hb_va_dxva2_t *dxva = calloc( 1, sizeof(*dxva) );
    if( !dxva ) return NULL;
    dxva->codec_id = codec_id;

    dxva->hd3d9_dll = LoadLibrary( TEXT( "D3D9.DLL" ) );
    if( !dxva->hd3d9_dll )
    {
        hb_deep_log(2, "dxva2:cannot load d3d9.dll");
        goto error;
    }
    dxva->hdxva2_dll = LoadLibrary( TEXT( "DXVA2.DLL" ) );
    if( !dxva->hdxva2_dll )
    {
        hb_deep_log(2, "dxva2:cannot load DXVA2.dll");
        goto error;
    }

    if( hb_d3d_create_device( dxva ) == HB_WORK_ERROR )
    {
        hb_deep_log(2, "dxva2:Failed to create Direct3D device");
        goto error;
    }

    if( hb_d3d_create_device_manager( dxva ) == HB_WORK_ERROR )
    {
        hb_deep_log(2, "dxva2:D3dCreateDeviceManager failed");
        goto error;
    }


    if( hb_dx_create_video_service( dxva ) == HB_WORK_ERROR )
    {
        hb_deep_log(2, "dxva2:DxCreateVideoService failed");
        goto error;
    }

    if( hb_dx_find_video_service_conversion( dxva, &dxva->input, &dxva->render ) == HB_WORK_ERROR )
    {
        hb_deep_log(2, "dxva2:DxFindVideoServiceConversion failed");
        goto error;
    }

    /*
     * We may get a valid DXVA2 decoder later on, but we won't be able to
     * use it if libavcodec is built without support for the appropriate
     * AVHWaccel, so we need to check for it before declaring victory.
     */
    AVHWAccel *hwaccel = NULL;
    for (hwaccel = av_hwaccel_next(NULL);
         hwaccel != NULL;
         hwaccel = av_hwaccel_next(hwaccel))
    {
        if (hwaccel->id == codec_id && hwaccel->pix_fmt == AV_PIX_FMT_DXVA2_VLD)
        {
            dxva->do_job = HB_WORK_OK;
            dxva->description = "DXVA2";
            return dxva;
        }
    }
    hb_deep_log(2, "dxva2:compatible AVHWAccel not found");

error:
    hb_va_close( dxva );
    return NULL;
}

void hb_va_new_dxva2( hb_va_dxva2_t *dxva2, AVCodecContext *p_context )
{
    if( p_context->width > 0 && p_context->height > 0 )
    {
        if( hb_va_setup( dxva2, &p_context->hwaccel_context, p_context->width, p_context->height ) == HB_WORK_ERROR )
        {
            hb_log( "dxva2:hb_va_Setup failed" );
            hb_va_close( dxva2 );
            dxva2 = NULL;
        }
    }
    if( dxva2 )
    {
        dxva2->input_pts[0] = 0;
        dxva2->input_pts[1] = 0;
        if( dxva2->description )
            hb_log(  "dxva2:Using %s for hardware decoding", dxva2->description );
        p_context->draw_horiz_band = NULL;
    }

}

char* hb_get_pix_fmt_name( int pix_fmt )
{
    static const char *ppsz_name[AV_PIX_FMT_NB] =
    {
        [AV_PIX_FMT_VDPAU_H264] = "AV_PIX_FMT_VDPAU_H264",
        [AV_PIX_FMT_VAAPI_IDCT] = "AV_PIX_FMT_VAAPI_IDCT",
        [AV_PIX_FMT_VAAPI_VLD] = "AV_PIX_FMT_VAAPI_VLD",
        [AV_PIX_FMT_VAAPI_MOCO] = "AV_PIX_FMT_VAAPI_MOCO",
        [AV_PIX_FMT_DXVA2_VLD] = "AV_PIX_FMT_DXVA2_VLD",
        [AV_PIX_FMT_YUYV422] = "AV_PIX_FMT_YUYV422",
        [AV_PIX_FMT_YUV420P] = "AV_PIX_FMT_YUV420P",
    };

    return ppsz_name[pix_fmt];
}

enum PixelFormat hb_ffmpeg_get_format( AVCodecContext *p_context, const enum PixelFormat *pi_fmt )
{
    int i;
    for( i = 0; pi_fmt[i] != AV_PIX_FMT_NONE; i++ )
    {
        hb_log( "dxva2:Available decoder output format %d (%s)", pi_fmt[i], hb_get_pix_fmt_name(pi_fmt[i]) ? : "Unknown" );
        if( pi_fmt[i] == AV_PIX_FMT_DXVA2_VLD )
        {
            return pi_fmt[i];
        }
    }
    return avcodec_default_get_format( p_context, pi_fmt );
}

int hb_va_get_frame_buf( hb_va_dxva2_t *dxva2, AVCodecContext *p_context, AVFrame *frame )
{
    frame->type = FF_BUFFER_TYPE_USER;
    if( hb_va_get( dxva2, frame ) == HB_WORK_ERROR )
    {
        hb_log( "VaGrabSurface failed" );
        return HB_WORK_ERROR;
    }
    return HB_WORK_OK;

}

int hb_check_hwd_fmt( int fmt )
{
    int result = 1;
    switch ( fmt )
    {
        case AV_PIX_FMT_YUV420P16LE:
        case AV_PIX_FMT_YUV420P16BE:
        case AV_PIX_FMT_YUV422P16LE:
        case AV_PIX_FMT_YUV422P16BE:
        case AV_PIX_FMT_YUV444P16LE:
        case AV_PIX_FMT_YUV444P16BE:
        case AV_PIX_FMT_YUV420P9BE:
        case AV_PIX_FMT_YUV420P9LE:
        case AV_PIX_FMT_YUV420P10BE:
        case AV_PIX_FMT_YUV420P10LE:
        case AV_PIX_FMT_YUV422P10BE:
        case AV_PIX_FMT_YUV422P10LE:
        case AV_PIX_FMT_YUV444P9BE:
        case AV_PIX_FMT_YUV444P9LE:
        case AV_PIX_FMT_YUV444P10BE:
        case AV_PIX_FMT_YUV444P10LE:
        case AV_PIX_FMT_YUV422P9BE:
        case AV_PIX_FMT_YUV422P9LE:
        case AV_PIX_FMT_GBRP9BE:
        case AV_PIX_FMT_GBRP9LE:
        case AV_PIX_FMT_GBRP10BE:
        case AV_PIX_FMT_GBRP10LE:
        case AV_PIX_FMT_GBRP16BE:
        case AV_PIX_FMT_GBRP16LE:
        case AV_PIX_FMT_YUVA420P9BE:
        case AV_PIX_FMT_YUVA420P9LE:
        case AV_PIX_FMT_YUVA422P9BE:
        case AV_PIX_FMT_YUVA422P9LE:
        case AV_PIX_FMT_YUVA444P9BE:
        case AV_PIX_FMT_YUVA444P9LE:
        case AV_PIX_FMT_YUVA420P10BE:
        case AV_PIX_FMT_YUVA420P10LE:
             result = 0; 
    }
    return result;
}

#endif // USE_HWD
