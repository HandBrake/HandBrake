/* vt_common.c

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "vt_common.h"

#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>

//#define VT_STATS

#ifdef VT_STATS
static void toggle_vt_gva_stats(bool state)
{
    CFPropertyListRef cf_state = state ? kCFBooleanTrue : kCFBooleanFalse;
    CFPreferencesSetValue(CFSTR("gvaEncoderPerf"), cf_state, CFSTR("com.apple.GVAEncoder"), kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    CFPreferencesSetValue(CFSTR("gvaEncoderPSNR"), cf_state, CFSTR("com.apple.GVAEncoder"), kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    CFPreferencesSetValue(CFSTR("gvaEncoderSSIM"), cf_state, CFSTR("com.apple.GVAEncoder"), kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);

    //CFPreferencesSetValue(CFSTR("gvaEncoderStats"), cf_state, CFSTR("com.apple.GVAEncoder"), kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    //CFPreferencesSetValue(CFSTR("gvaDebug"), cf_state, CFSTR("com.apple.AppleGVA"), kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
}
#endif

static const CFStringRef encoder_id_h264 = CFSTR("com.apple.videotoolbox.videoencoder.h264.gva");
static const CFStringRef encoder_id_h265 = CFSTR("com.apple.videotoolbox.videoencoder.hevc.gva");

int encvt_available(CFStringRef encoder)
{
    CFArrayRef encoder_list;
    VTCopyVideoEncoderList(NULL, &encoder_list);
    CFIndex size = CFArrayGetCount(encoder_list);

    for (CFIndex i = 0; i < size; i++ )
    {
        CFDictionaryRef encoder_dict = CFArrayGetValueAtIndex(encoder_list, i);
        CFStringRef encoder_id = CFDictionaryGetValue(encoder_dict, kVTVideoEncoderSpecification_EncoderID);
        if (CFEqual(encoder_id, encoder))
        {
            CFRelease(encoder_list);
            return 1;
        }
    }
    CFRelease(encoder_list);
    return 0;
}

int hb_vt_h264_is_available()
{
    return encvt_available(encoder_id_h264);
}

int hb_vt_h265_is_available()
{
    return encvt_available(encoder_id_h265);
}
