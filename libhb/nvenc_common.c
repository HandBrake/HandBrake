/* nvenc_common.c
 *
 * Copyright (c) 2003-2019 HandBrake Team
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

int hb_check_nvenc_available();

int hb_nvenc_h264_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return 0;
    #endif
}

int hb_nvenc_h265_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return 0;
    #endif
}

static int isAvailable = -1;
int hb_check_nvenc_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    if (isAvailable != -1){
        return isAvailable;
    }

    #if HB_PROJECT_FEATURE_NVENC
        uint32_t nvenc_ver;
        void *context = NULL;
        NvencFunctions *nvenc_dl = NULL;

        int loadErr = nvenc_load_functions(&nvenc_dl, context);
        if (loadErr < 0) {
            isAvailable = 0;
            return 0;
        }

        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS)
        {
            isAvailable = 0;
            return 0;
        }

        hb_log("Nvenc version %d.%d\n", nvenc_ver >> 4, nvenc_ver & 0xf);
        if ((NVENCAPI_MAJOR_VERSION << 4 | NVENCAPI_MINOR_VERSION) > nvenc_ver) {
            hb_log("NVENC version not supported. Disabling feature.");
            isAvailable = 0;
            return 0;
        }

        isAvailable = 1;
        return 1;
    #else
        return 0;
    #endif
}
