/* decssasub.c

   Copyright (c) 2003-2018 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Converts SSA subtitles to either:
 * (1) TEXTSUB format: UTF-8 subtitles with limited HTML-style markup (<b>, <i>, <u>), or
 * (2) PICTURESUB format, using libass.
 *
 * SSA format references:
 *   http://www.matroska.org/technical/specs/subtitles/ssa.html
 *   http://moodub.free.fr/video/ass-specs.doc
 *   vlc-1.0.4/modules/codec/subtitles/subsass.c:ParseSSAString
 *
 * libass references:
 *   libass-0.9.9/ass.h
 *   vlc-1.0.4/modules/codec/libass.c
 *
 * @author David Foster (davidfstr)
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "hb.h"

#include <ass/ass.h>
#include "decssasub.h"
#include "colormap.h"

struct hb_work_private_s
{
    // If decoding to PICTURESUB format:
    int readOrder;

    hb_job_t *job;
};

#define SSA_VERBOSE_PACKETS 0

static int ssa_update_style(char *ssa, hb_subtitle_style_t *style)
{
    int pos, end, index;

    if (ssa[0] != '{')
        return 0;

    pos = 1;
    while (ssa[pos] != '}' && ssa[pos] != '\0')
    {
        index = -1;

        // Skip any malformed markup junk
        while (strchr("\\}", ssa[pos]) == NULL) pos++;
        pos++;
        // Check for an index that is in some markup (e.g. font color)
        if (isdigit(ssa[pos]))
        {
            index = ssa[pos++] - 0x30;
        }
        // Find the end of this markup clause
        end = pos;
        while (strchr("\\}", ssa[end]) == NULL) end++;
        // Handle simple integer valued attributes
        if (strchr("ibu", ssa[pos]) != NULL && isdigit(ssa[pos+1]))
        {
            int val = strtol(ssa + pos + 1, NULL, 0);
            switch (ssa[pos])
            {
                case 'i':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_ITALIC) |
                                   !!val * HB_STYLE_FLAG_ITALIC;
                    break;
                case 'b':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_BOLD) |
                                   !!val * HB_STYLE_FLAG_BOLD;
                    break;
                case 'u':
                    style->flags = (style->flags & ~HB_STYLE_FLAG_UNDERLINE) |
                                   !!val * HB_STYLE_FLAG_UNDERLINE;
                    break;
            }
        }
        if (ssa[pos] == 'c' && ssa[pos+1] == '&' && ssa[pos+2] == 'H')
        {
            // Font color markup
            char *endptr;
            uint32_t bgr;

            bgr = strtol(ssa + pos + 3, &endptr, 16);
            if (*endptr == '&')
            {
                switch (index)
                {
                    case -1:
                    case 1:
                        style->fg_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 2:
                        style->alt_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 3:
                        style->ol_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    case 4:
                        style->bg_rgb = HB_BGR_TO_RGB(bgr);
                        break;
                    default:
                        // Unknown color index, ignore
                        break;
                }
            }
        }
        if ((ssa[pos] == 'a' && ssa[pos+1] == '&' && ssa[pos+2] == 'H') ||
            (!strcmp(ssa+pos, "alpha") && ssa[pos+5] == '&' && ssa[pos+6] == 'H'))
        {
            // Font alpha markup
            char *endptr;
            uint8_t alpha;
            int alpha_pos = 3;

            if (ssa[1] == 'l')
                alpha_pos = 7;

            alpha = strtol(ssa + pos + alpha_pos, &endptr, 16);
            if (*endptr == '&')
            {
                // SSA alpha is inverted 0 is opaque
                alpha = 255 - alpha;
                switch (index)
                {
                    case -1:
                    case 1:
                        style->fg_alpha = alpha;
                        break;
                    case 2:
                        style->alt_alpha = alpha;
                        break;
                    case 3:
                        style->ol_alpha = alpha;
                        break;
                    case 4:
                        style->bg_alpha = alpha;
                        break;
                    default:
                        // Unknown alpha index, ignore
                        break;
                }
            }
        }
        pos = end;
    }
    if (ssa[pos] == '}')
        pos++;
    return pos;
}

char * hb_ssa_to_text(char *in, int *consumed, hb_subtitle_style_t *style)
{
    int markup_len = 0;
    int in_pos = 0;
    int out_pos = 0;
    char *out = malloc(strlen(in) + 1); // out will never be longer than in

    for (in_pos = 0; in[in_pos] != '\0'; in_pos++)
    {
        if ((markup_len = ssa_update_style(in + in_pos, style)))
        {
            *consumed = in_pos + markup_len;
            out[out_pos++] = '\0';
            return out;
        }
        // Check escape codes
        if (in[in_pos] == '\\')
        {
            in_pos++;
            switch (in[in_pos])
            {
                case '\0':
                    in_pos--;
                    break;
                case 'N':
                case 'n':
                    out[out_pos++] = '\n';
                    break;
                case 'h':
                    out[out_pos++] = ' ';
                    break;
                default:
                    out[out_pos++] = in[in_pos];
                    break;
            }
        }
        else
        {
            out[out_pos++] = in[in_pos];
        }
    }
    *consumed = in_pos;
    out[out_pos++] = '\0';
    return out;
}

void hb_ssa_style_init(hb_subtitle_style_t *style)
{
    style->flags = 0;

    style->fg_rgb    = 0x00FFFFFF;
    style->alt_rgb   = 0x00FFFFFF;
    style->ol_rgb    = 0x000F0F0F;
    style->bg_rgb    = 0x000F0F0F;

    style->fg_alpha  = 0xFF;
    style->alt_alpha = 0xFF;
    style->ol_alpha  = 0xFF;
    style->bg_alpha  = 0xFF;
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job = job;

    return 0;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;

#if SSA_VERBOSE_PACKETS
    printf("\nPACKET(%"PRId64",%"PRId64"): %.*s\n", in->s.start/90, in->s.stop/90, in->size, in->data);
#endif

    *buf_in = NULL;
    *buf_out = in;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        return HB_WORK_DONE;
    }

    // Not much to do here.  ffmpeg already supplies SSA subtitles in the
    // requried matroska packet format.
    //
    // We require string termination of the buffer
    hb_buffer_realloc(in, ++in->size);
    in->data[in->size - 1] = '\0';

    return HB_WORK_OK;
}

static void decssaClose( hb_work_object_t * w )
{
    free( w->private_data );
}

hb_work_object_t hb_decssasub =
{
    WORK_DECSSASUB,
    "SSA Subtitle Decoder",
    decssaInit,
    decssaWork,
    decssaClose
};
