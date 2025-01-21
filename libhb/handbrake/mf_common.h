/* mf_common.h
 *
 * Copyright (c) Dash Santosh <dash.sathyanarayanan@multicorewareinc.com>
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_MF_COMMON_H
#define HANDBRAKE_MF_COMMON_H

#include "handbrake/hbffmpeg.h"

#if HB_PROJECT_FEATURE_MF
#include <windows.h>
#include <mfapi.h>
#include <mfobjects.h>

typedef struct MFFunctions {
    // MFTEnumEx is missing in Windows Vista's mfplat.dll.
    HRESULT (WINAPI *MFTEnumEx)(GUID guidCategory, UINT32 Flags,
                                const MFT_REGISTER_TYPE_INFO *pInputType,
                                const MFT_REGISTER_TYPE_INFO *pOutputType,
                                IMFActivate ***pppMFTActivate,
                                UINT32 *pnumMFTActivate);
} MFFunctions;
#endif // HB_PROJECT_FEATURE_MF

int            hb_mf_h264_available();
int            hb_mf_h265_available();
int            hb_mf_av1_available();
int            hb_check_mf_available();

#endif // HANDBRAKE_MF_COMMON_H
