/* nvenc_common.c
 *
 * Copyright (c) 2003-2022 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/nvenc_common.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_NVENC
#include <ffnvcodec/nvEncodeAPI.h>
#include <ffnvcodec/dynlink_loader.h>
#endif

static int is_nvenc_available = -1;
static int is_nvenc_av1_available = -1;
static int is_nvenc_hevc_available = -1;

static double cuda_version = -1;

int hb_nvenc_get_cuda_version()
{
   if (cuda_version != -1)
   {
       return cuda_version;
   }

    #if HB_PROJECT_FEATURE_NVENC
        CUresult apiErr = 0;
        CUcontext cuda_ctx;
        CudaFunctions *cu = NULL;
        CUdevice dev;
        int major, minor, devices;
          
        apiErr = cuda_load_functions(&cu, NULL);
        if (apiErr == CUDA_SUCCESS) 
        {
            apiErr = cu->cuInit(0);
            if (apiErr == CUDA_SUCCESS) 
            {
                cu->cuDeviceGetCount(&devices);
                if (!devices)
                {
                    cuda_version = 0;
                    free(cu);
                    return cuda_version;
                }

                // For now, lets just work off the primary device we find.
                for (int i = 0; i < devices; ++i) 
                {
                    int result = cu->cuDeviceGet(&dev, i);
                    if (result == CUDA_SUCCESS)
                    {
                        cu->cuCtxCreate(&cuda_ctx, 0, dev);
                        break;
                    }
                }

                apiErr = cu->cuDeviceComputeCapability(&major, &minor, dev);

                if (apiErr == CUDA_SUCCESS) 
                {
                    cuda_version = major * 1000 + minor;
                    hb_log("CUDA Version: %i.%i", major, minor);

                    free(cu);
                    return cuda_version;
                }
            }
        }

    #else
        cuda_version = 0;
    #endif

    return cuda_version;
}

int hb_check_nvenc_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    if (is_nvenc_available != -1)
    {
        return is_nvenc_available;
    }

    #if HB_PROJECT_FEATURE_NVENC
        uint32_t nvenc_ver;
        void *context = NULL;
        NvencFunctions *nvenc_dl = NULL;

        int loadErr = nvenc_load_functions(&nvenc_dl, context);
        if (loadErr < 0) 
        {
            is_nvenc_available = 0;
            return 0;
        }

        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS) 
        {
            is_nvenc_available = 0;
            hb_log("nvenc: not available");
            return 0;
        }
        else 
        {
            hb_log("nvenc: version %d.%d is available", nvenc_ver >> 4, nvenc_ver & 0xf);
            is_nvenc_available = 1;
            
            if (hb_check_nvdec_available())
            {
                hb_log("nvdec: is available");
            } 
            else 
            {
                hb_log("nvdec: is not compiled into this build");
            }
            return 1;
        }

        return 1;
    #else
        is_nvenc_available = 0;
        hb_log("nvenc: not available");
        return 0;
    #endif
}

int hb_nvenc_h264_available()
{
    return hb_check_nvenc_available();
}

int hb_nvenc_h265_available()
{
    if (is_nvenc_hevc_available != -1)
    {
        return is_nvenc_hevc_available;
    }
    
    if (!hb_check_nvenc_available())
    {
        is_nvenc_hevc_available = 0;
        return is_nvenc_hevc_available;
    }
    
    if (hb_nvenc_get_cuda_version() >= 5002) 
    {
        is_nvenc_hevc_available = 1;
    } 
    else 
    {
        is_nvenc_hevc_available = 0;
    }
        
    return is_nvenc_hevc_available;
}

int hb_nvenc_av1_available()
{
    if (is_nvenc_av1_available != -1)
    {
        return is_nvenc_av1_available;
    }
    
    if (!hb_check_nvenc_available()){
        is_nvenc_av1_available = 0;
        return is_nvenc_av1_available;
    }
    
    if (hb_nvenc_get_cuda_version() >= 8009) 
    {
        is_nvenc_av1_available = 1;
    } 
    else 
    {
        is_nvenc_av1_available = 0;
    }
        
    return is_nvenc_av1_available;
}

int hb_check_nvdec_available()
{
    #if HB_PROJECT_FEATURE_NVDEC
        return 1;
    #else
        return 0;
    #endif
}

const char * hb_map_nvenc_preset_name (const char * preset)
{
    if (preset == NULL)
    {
        return "p4";
    }

    if (strcmp(preset, "fastest") == 0) {
      return "p1";
    }  else if (strcmp(preset, "faster") == 0) {
      return "p2";
    } else if (strcmp(preset, "fast") == 0) {
       return "p3";
    } else if (strcmp(preset, "medium") == 0) {
      return "p4";
    } else if (strcmp(preset, "slow") == 0) {
      return "p5";
    } else if (strcmp(preset, "slower") == 0) {
       return "p6";
    } else if (strcmp(preset, "slowest") == 0) {
      return "p7";
    }

    return "p4"; // Default to Medium
}
