/* h265_common.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_H265_COMMON_H
#define HANDBRAKE_H265_COMMON_H

#include "handbrake/project.h"
#include <stdint.h>

// inspired by libavcodec/hevc.h
// in HEVC, all "random access point" NAL units are keyframes
#define HB_HEVC_NALU_KEYFRAME(nal_unit_type) (((nal_unit_type) >= 16) && ((nal_unit_type) <= 23))

static const char * const hb_x265_tune_names[]              = {
    "none", "psnr", "ssim", "grain", "zerolatency", "fastdecode", "animation", NULL };
static const char * const hb_h265_tier_names[]              = {
    "auto", "main", "high", NULL, };
static const char * const hb_h265_profile_names_8bit[]      = {
    "auto", "main", "mainstillpicture", NULL, };
static const char * const hb_x265_profile_names_8bit[]      = {
    "auto", "main", "mainstillpicture", "main444-8", "main444-intra", NULL, };
static const char * const hb_h265_profile_names_10bit[]     = {
    "auto", "main10", "main10-intra", NULL, };
static const char * const hb_x265_profile_names_10bit[]     = {
    "auto", "main10", "main10-intra", "main422-10", "main422-10-intra", "main444-10", "main444-10-intra", NULL, };
static const char * const hb_h265_profile_names_12bit[]     = {
    "auto", "main12", "main12-intra", NULL, };
static const char * const hb_x265_profile_names_12bit[]     = {
    "auto", "main12", "main12-intra", "main422-12", "main422-12-intra", "main444-12", "main444-12-intra", NULL, };
static const char * const hb_h265_profile_names_16bit[]     = {
    "auto", "main16", "main16-intra", NULL, };
static const char * const hb_h265_level_names[]             = {
    "auto", "1.0", "2.0", "2.1", "3.0", "3.1", "4.0", "4.1",
    "5.0", "5.1", "5.2", "6.0", "6.1", "6.2",  NULL, };
static const char * const hb_h265_level_names2[]            = {
    "auto", "10", "20", "21", "30", "31", "40", "41",
    "50", "51", "52", "60", "61", "62",  NULL, };
static const int          hb_h265_level_values[]            = {
    -1,  30,  60,  63,  90,  93, 120, 123,
    150, 153, 156, 180, 183, 186,   0, };

// stolen from libx265's x265.h
static const char * const hb_h265_fullrange_names[] = {
    "limited", "full", NULL, };
static const char * const hb_h265_vidformat_names[] = {
    "component", "pal", "ntsc", "secam", "mac", "undef", NULL, };
static const char * const hb_h265_colorprim_names[] = {
    "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m",
    "smpte240m", "film", "bt2020", NULL, };
static const char * const  hb_h265_transfer_names[] = {
    "", "bt709", "undef", "", "bt470m", "bt470bg", "smpte170m",
    "smpte240m", "linear", "log100", "log316", "iec61966-2-4",
    "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12", NULL, };
static const char * const hb_h265_colmatrix_names[] = {
    "GBR", "bt709", "undef", "", "fcc", "bt470bg", "smpte170m",
    "smpte240m", "YCgCo", "bt2020nc", "bt2020c", NULL, };

#endif // HANDBRAKE_H265_COMMON_H
