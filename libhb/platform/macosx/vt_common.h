/* vt_common.h

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

int  hb_vt_h264_is_available();
int  hb_vt_h265_is_available();

static const char * const hb_vt_h265_level_names[] =
{
    "auto",  NULL,
};
