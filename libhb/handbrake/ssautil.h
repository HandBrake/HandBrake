/* ssautil.h
 *
 * Copyright (c) 2003-2023 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_SSAUTIL_H
#define HANDBRAKE_SSAUTIL_H

typedef struct hb_subtitle_style_s hb_subtitle_style_t;
typedef struct hb_subtitle_style_context_s hb_subtitle_style_context_t;

typedef struct hb_tx3g_output_buf_s hb_tx3g_output_buf_t;
typedef struct hb_tx3g_style_context_s hb_tx3g_style_context_t;

#define HB_STYLE_FLAG_ITALIC    0x0001
#define HB_STYLE_FLAG_BOLD      0x0002
#define HB_STYLE_FLAG_UNDERLINE 0x0004

hb_subtitle_style_context_t * hb_subtitle_style_init(const char * ssa_header);
hb_tx3g_style_context_t     * hb_tx3g_style_init(
                                        int height, const char * ssa_header);
void hb_subtitle_style_close(hb_subtitle_style_context_t ** ctx);
void hb_tx3g_style_close(hb_tx3g_style_context_t ** ctx);

void hb_muxmp4_process_subtitle_style(
        hb_tx3g_style_context_t * ctx,
        uint8_t  * input, uint8_t  ** output,
        uint8_t ** style, uint16_t  * stylesize);

#endif // HANDBRAKE_SSAUTIL_H
