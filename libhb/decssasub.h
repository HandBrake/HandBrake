/* decssasub.h
 *
 * Copyright (c) 2003-2015 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef __DECSSASUB_H__
#define __DECSSASUB_H__

typedef struct
{
    uint32_t    flags;

    uint32_t    fg_rgb;     // forground color
    uint32_t    alt_rgb;    // secondary color
    uint32_t    ol_rgb;     // outline color
    uint32_t    bg_rgb;     // background color

    uint32_t    fg_alpha;     // forground alpha
    uint32_t    alt_alpha;    // secondary alpha
    uint32_t    ol_alpha;     // outline alpha
    uint32_t    bg_alpha;     // background alpha
} hb_subtitle_style_t;

#define HB_STYLE_FLAG_ITALIC    0x0001
#define HB_STYLE_FLAG_BOLD      0x0002
#define HB_STYLE_FLAG_UNDERLINE 0x0004

char * hb_ssa_to_text(char *in, int *consumed, hb_subtitle_style_t *style);
void hb_ssa_style_init(hb_subtitle_style_t *style);

#endif // __DECSSASUB_H__
