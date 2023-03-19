/* vt_common.h

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdio.h>

int hb_vt_is_encoder_available(int encoder);
int hb_vt_is_constant_quality_available(int encoder);
int hb_vt_is_multipass_available(int encoder);

const int* hb_vt_get_pix_fmts(int encoder);

const char* const* hb_vt_preset_get_names(int encoder);
const char* const* hb_vt_profile_get_names(int encoder);
const char* const* hb_vt_level_get_names(int encoder);
