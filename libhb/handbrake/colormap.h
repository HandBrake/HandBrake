/* colormap.h
 *
 * Copyright (c) 2003-2023 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_COLORMAP_H
#define HANDBRAKE_COLORMAP_H

#define HB_RGB_TO_BGR(c)    (((c & 0xff0000) >> 16) | \
                             ((c & 0x00ff00)      ) | \
                             ((c & 0x0000ff) << 16))
#define HB_BGR_TO_RGB(c)    HB_RGB_TO_BGR(c)

uint32_t hb_rgb_lookup_by_name(const char *color);

#endif // HANDBRAKE_COLORMAP_H
