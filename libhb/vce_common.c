/* vce_common.c
 *
 * Copyright (c) 2003-2018 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifdef USE_VCE

int hb_vce_h264_available()
{
    return 1;
}

int hb_vce_h265_available()
{
    return 1;
}

#else

int hb_vce_h264_available()
{
    return 0;
}

int hb_vce_h265_available()
{
    return 0;
}

#endif // USE_QSV
