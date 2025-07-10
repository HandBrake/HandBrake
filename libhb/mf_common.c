/* mf_common.c
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/mf_common.h"
#include "handbrake/handbrake.h"

static int is_mf_available = -1;
static int is_mf_av1_available = -1;
static int is_mf_h264_available = -1;
static int is_mf_h265_available = -1;

#if HB_PROJECT_FEATURE_MF
#if !HAVE_UWP
#define LOAD_MF_FUNCTION(library, func_name) \
    MFFunctions functions; \
    functions.func_name = (void *)hb_dlsym(library, #func_name); \
    if (!functions.func_name) { \
        hb_log("DLL mfplat.dll failed to find function "\
           #func_name "\n"); \
        return -1; \
    }
#else
// In UWP (which lacks LoadLibrary), just link directly against
// the functions - this requires building with new/complete enough
// import libraries.
#define LOAD_MF_FUNCTION(library, func_name) \
    MFFunctions functions; \
    functions.func_name = func_name; \
    if (!functions.func_name) { \
        hb_log("DLL mfplat.dll failed to find function "\
           #func_name "\n"); \
        return -1; \
    }
#endif

static int mf_load_library()
{

    HMODULE library = NULL;
#if !HAVE_UWP
    library = hb_dlopen("mfplat.dll");

    if (!library) {
        hb_log("DLL mfplat.dll failed to open");
        is_mf_available = 0;
        return -1;
    }
#endif

    // MFTEnumEx is missing in Windows Vista's mfplat.dll.
    LOAD_MF_FUNCTION(library, MFTEnumEx);

    return 0;
}

int hb_is_mf_encoder_available(const GUID* pSubType)
{
    IMFActivate** ppActivate = NULL;
    MFT_REGISTER_TYPE_INFO outputType = { MFMediaType_Video, *pSubType };
    UINT32 numTransforms = 0;
    int is_available = -1;

    HRESULT hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_ENCODER,
        MFT_ENUM_FLAG_HARDWARE,  // Only check for hardware MFTs
        NULL,
        &outputType,
        &ppActivate,
        &numTransforms
    );

    if (SUCCEEDED(hr) && numTransforms > 0)
    {
        is_available = 1;
    }
    else
    {
        is_available = 0;
    }

    if (ppActivate)
    {
        for (UINT32 i = 0; i < numTransforms; i++)
        {
            ppActivate[i]->lpVtbl->Release(ppActivate[i]);
        }
        CoTaskMemFree(ppActivate);
    }

    return is_available;
}
#endif

int hb_check_mf_available()
{
    if (hb_is_hardware_disabled())
    {
        return 0;
    }

    if (is_mf_available != -1)
    {
        return is_mf_available;
    }

#if HB_PROJECT_FEATURE_MF
        int loadErr = mf_load_library();
        if (loadErr < 0) 
        {
            is_mf_available = 0;
            return 0;
        }

        return 1;
#else
        is_mf_available = 0;
        hb_log("mf: not available");
        return 0;
#endif
}

int hb_mf_h264_available()
{
    if (is_mf_h264_available != -1)
    {
        return is_mf_h264_available;
    }

    if (!hb_check_mf_available())
    {
        is_mf_h264_available = 0;
        return is_mf_h264_available;
    }

#if HB_PROJECT_FEATURE_MF
    if (hb_is_mf_encoder_available(&MFVideoFormat_H264))
    {
        is_mf_h264_available = 1;
    } 
    else 
    {
        is_mf_h264_available = 0;
    }
#endif

    return is_mf_h264_available;
}

int hb_mf_h265_available()
{
    if (is_mf_h265_available != -1)
    {
        return is_mf_h265_available;
    }

    if (!hb_check_mf_available()){
        is_mf_h265_available = 0;
        return is_mf_h265_available;
    }

#if HB_PROJECT_FEATURE_MF
    if (hb_is_mf_encoder_available(&MFVideoFormat_HEVC))
    {
        is_mf_h265_available = 1;
    }
    else
    {
        is_mf_h265_available = 0;
    }
#endif

    return is_mf_h265_available;
}

int hb_mf_av1_available()
{
    if (is_mf_av1_available != -1)
    {
        return is_mf_av1_available;
    }
    
    if (!hb_check_mf_available()){
        is_mf_av1_available = 0;
        return is_mf_av1_available;
    }

#if HB_PROJECT_FEATURE_MF
    if (hb_is_mf_encoder_available(&MFVideoFormat_AV1)) 
    {
        is_mf_av1_available = 1;
    } 
    else 
    {
        is_mf_av1_available = 0;
    }
#endif

    return is_mf_av1_available;
}

#if HB_PROJECT_FEATURE_MF
int hb_directx_available()
{
    if (hb_is_hardware_disabled())
    {
        return 0;
    }
    enum AVHWDeviceType hw_type = av_hwdevice_find_type_by_name("d3d11va");
    if (hw_type == AV_HWDEVICE_TYPE_NONE)
    {
        hb_log("directx: not available on this system");
        return 0;
    }

    hb_log("directx: is available");
    return 1;
}

static const int mf_encoders[] =
{
    HB_VCODEC_INVALID
};

hb_hwaccel_t hb_hwaccel_mf =
{
    .id         = HB_DECODE_MF,
    .name       = "mf hwaccel",
    .encoders   = mf_encoders,
    .type       = AV_HWDEVICE_TYPE_D3D11VA,
    .hw_pix_fmt = AV_PIX_FMT_D3D11,
    .caps       = HB_HWACCEL_CAP_SCAN
};

#else // HB_PROJECT_FEATURE_MF

int hb_directx_available()
{
    return -1;
}

#endif // HB_PROJECT_FEATURE_MF
