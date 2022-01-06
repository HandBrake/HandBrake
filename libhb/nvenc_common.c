/* nvenc_common.c
 *
 * Copyright (c) 2003-2022 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_NVENC
#include <ffnvcodec/nvEncodeAPI.h>
#include <ffnvcodec/dynlink_loader.h>
#endif

static int is_nvenc_available = -1;

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
        if (loadErr < 0) {
            return 0;
        }

        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS) {
            is_nvenc_available = 0;
            return 0;
        } else {
            hb_deep_log(1, "NVENC version %d.%d\n", nvenc_ver >> 4, nvenc_ver & 0xf);
            is_nvenc_available = 1;
            return 1;
        }

        return 1;
    #else
        return 0;
    #endif
}

int hb_nvenc_h264_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return is_nvenc_available;
    #endif
}

int hb_nvenc_h265_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return is_nvenc_available;
    #endif
}

char * hb_map_nvenc_preset_name (const char * preset){

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
