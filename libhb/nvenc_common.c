/* nvenc_common.c
 *
 * Copyright (c) 2003-2018 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hbffmpeg.h"

#ifdef USE_NVENC
#include <ffnvcodec/nvEncodeAPI.h>
#include <ffnvcodec/dynlink_loader.h>
#endif 

int hb_nvenc_h264_available()
{
    #ifdef USE_NVENC
        return hb_check_nvenc_available();
    #else
        return 0;
    #endif 
}

int hb_nvenc_h265_available()
{
    #ifdef USE_NVENC
        return hb_check_nvenc_available();
    #else
        return 0;
    #endif 
}

int hb_check_nvenc_available() 
{
    #ifdef USE_NVENC
        uint32_t nvenc_ver;
        void *context;
        NvencFunctions *nvenc_dl = NULL;

        int loadErr = nvenc_load_functions(&nvenc_dl, context);
        if (loadErr < 0) {
            return 0;
        }
  
        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS)
        {
            return 0;
        }

        return 1;
    #else
        return 0;
    #endif 
}
