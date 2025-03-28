/* ssautil.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#include <stdio.h>
#include <ctype.h>
#include "handbrake/handbrake.h"
#include "handbrake/ssautil.h"

struct hb_subtitle_style_s
{
    char      * name;
    char      * font_name;
    int         font_size;

    uint32_t    flags;

    uint32_t    fg_rgb;     // foreground color
    uint32_t    alt_rgb;    // secondary color
    uint32_t    ol_rgb;     // outline color
    uint32_t    bg_rgb;     // background color

    uint32_t    fg_alpha;     // foreground alpha
    uint32_t    alt_alpha;    // secondary alpha
    uint32_t    ol_alpha;     // outline alpha
    uint32_t    bg_alpha;     // background alpha
};

struct hb_subtitle_style_context_s
{
    hb_subtitle_style_t   current;
    int                   event_style_default;
    hb_subtitle_style_t * styles;
    int                   styles_count;
    int                   styles_size;
    int                   style_default;
};

struct hb_tx3g_output_buf_s
{
    int       alloc;
    int       size;
    uint8_t * buf;
};

struct hb_tx3g_style_context_s
{
    hb_tx3g_output_buf_t          style_atoms;
    int                           style_atom_count;
    int                           style_start;
    int                           height;
    hb_subtitle_style_context_t * in_style;
    hb_subtitle_style_t           out_style;
    uint8_t                       flush;
};

static void ssa_style_reset(hb_subtitle_style_context_t * ctx)
{
    if (ctx->styles != NULL && ctx->style_default < ctx->styles_count)
    {
        ctx->current = ctx->styles[ctx->event_style_default];
    }
    else
    {
        ctx->current.font_name = HB_FONT_SANS;
        ctx->current.font_size = 72;
        ctx->current.flags     = 0;
        ctx->current.fg_rgb    = 0x00FFFFFF;
        ctx->current.alt_rgb   = 0x00FFFFFF;
        ctx->current.ol_rgb    = 0x000F0F0F;
        ctx->current.bg_rgb    = 0x000F0F0F;

        ctx->current.fg_alpha  = 0xFF;
        ctx->current.alt_alpha = 0xFF;
        ctx->current.ol_alpha  = 0xFF;
        ctx->current.bg_alpha  = 0xFF;
    }
}

static int ssa_style_set(hb_subtitle_style_context_t * ctx, const char * style)
{
    int ii;

    if (ctx->styles != NULL && style != NULL && style[0] != 0)
    {
        for (ii = 0; ii < ctx->styles_count; ii++)
        {
            if (!strcasecmp(ctx->styles[ii].name, style))
            {
                ctx->current = ctx->styles[ii];
                return ii;
            }
        }
    }
    ssa_style_reset(ctx);
    return ctx->style_default;
}

static int ssa_update_style(const char *ssa, hb_subtitle_style_context_t *ctx)
{
    int pos, end, index;
    hb_subtitle_style_t * style = &ctx->current;

    if (ssa[0] != '{')
        return 0;

    pos = 1;
    while (ssa[pos] != '}' && ssa[pos] != '\0')
    {
        index = -1;

        // Skip any malformed markup junk
        while (strchr("\\}", ssa[pos]) == NULL) pos++;
        pos++;
        // Early exit if there is no tag
        if (ssa[pos] == '\0')
        {
            break;
        }
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
        if (ssa[pos] == 'r')
        {
            // Style reset
            char * style = hb_strndup(ssa + pos + 1, end - (pos + 1));
            ssa_style_set(ctx, style);
            free(style);
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

static char * ssa_to_text(const char *in, int *consumed,
                          hb_subtitle_style_context_t *ctx)
{
    int markup_len = 0;
    int in_pos = 0;
    int out_pos = 0;
    char *out = malloc(strlen(in) + 1); // out will never be longer than in

    for (in_pos = 0; in[in_pos] != '\0'; in_pos++)
    {
        if ((markup_len = ssa_update_style(in + in_pos, ctx)))
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

static char * get_field(char ** pos)
{
    char * result = NULL;
    if (pos == NULL || *pos == NULL || **pos == 0)
    {
        return NULL;
    }
    char * start = *pos;
    while (isspace(*start)) start++;
    char * end = strchr(start, ',');
    if (end != NULL)
    {
        result = hb_strndup(start, end - start);
        *pos = end + 1;
    }
    else
    {
        result = strdup(start);
        *pos = NULL;
    }
    return result;
}

static char * sgetline(char * str)
{
    char * eol;

    if (str == NULL)
    {
        return NULL;
    }

    // find end of line
    eol = strchr(str, '\n');
    if (eol != NULL)
    {
        if (eol > str && *(eol - 1) == '\r')
        {
            eol--;
        }
    }
    if (eol != NULL)
    {
        return hb_strndup(str, eol - str);
    }
    else
    {
        return strdup(str);
    }
}

static char ** get_fields(char * line, int last)
{
    int    count = 0, ii;
    char * pos;

    if (line == NULL || *line == 0)
    {
        return NULL;
    }

    // count number of fields
    count = 1;
    pos = line;
    while ((pos = strchr(pos, ',')) != NULL)
    {
        count++;
        pos++;
    }
    if (last > 0 && count > last)
    {
        count = last;
    }
    char ** result = calloc(count + 1, sizeof(char*));
    pos = line;
    for (ii = 0; ii < count - 1; ii++)
    {
        result[ii] = get_field(&pos);
    }
    result[ii] = strdup(pos);

    return result;
}

static int field_index(char ** fields, char * name)
{
    int ii;

    if (fields == NULL || name == NULL)
    {
        return -1;
    }
    for (ii = 0; fields[ii] != NULL; ii++)
    {
        if (!strcasecmp(name, fields[ii]))
        {
            return ii;
        }
    }
    return -1;
}

static const char * field_value(char ** style, int index)
{
    if (index >= 0 && index < hb_str_vlen(style))
    {
        return style[index];
    }
    return NULL;
}

typedef struct ssa_style_indices_s
{
    int style_name_index;
    int font_name_index;
    int font_size_index;
    int fg_color_index;
    int alt_color_index;
    int ol_color_index;
    int bg_color_index;
    int bold_index;
    int italic_index;
    int underline_index;
} ssa_style_indices_t;

static void fill_field_indices(char **fields, ssa_style_indices_t * indices)
{
    indices->style_name_index = field_index(fields, "Name");
    indices->font_name_index  = field_index(fields, "Fontname");
    indices->font_size_index  = field_index(fields, "Fontsize");
    indices->fg_color_index   = field_index(fields, "PrimaryColour");
    indices->alt_color_index  = field_index(fields, "SecondaryColour");
    indices->ol_color_index   = field_index(fields, "OutlineColour");
    indices->bg_color_index   = field_index(fields, "BackColour");
    indices->bold_index       = field_index(fields, "Bold");
    indices->italic_index     = field_index(fields, "Italic");
    indices->underline_index  = field_index(fields, "Underline");
}

static int add_style(hb_subtitle_style_context_t *ctx,
                      char ** style, ssa_style_indices_t *field_indices)
{
    const char * name;
    const char * value;
    int          size;
    uint32_t     rgb;
    uint32_t     alpha;
    uint32_t     flag;
    int          style_index;

    if (style == NULL)
    {
        return 0;
    }
    if (ctx->styles_count + 1 > ctx->styles_size)
    {
        void * tmp;
        ctx->styles_size = (ctx->styles_count + 1) * 2;

        tmp = realloc(ctx->styles, ctx->styles_size *
                      sizeof(hb_subtitle_style_t));
        if (tmp == NULL)
        {
            return 1;
        }
        ctx->styles = tmp;
    }
    style_index = ctx->styles_count;

    name = field_value(style, field_indices->style_name_index);
    if (name == NULL)
    {
        name = "Default";
    }
    if (!strcasecmp(name, "Default"))
    {
        ctx->style_default = style_index;
        ctx->event_style_default = ctx->style_default;
    }
    ctx->styles[style_index].name = strdup(name);

    value = field_value(style, field_indices->font_name_index);
    if (value == NULL)
    {
        value = HB_FONT_SANS;
    }
    ctx->styles[style_index].font_name = strdup(value);

    value = field_value(style, field_indices->font_size_index);
    if (value == NULL)
    {
        size = 72;
    }
    else
    {
        size = strtol(value, NULL, 0);
    }
    ctx->styles[style_index].font_size = size;

    value = field_value(style, field_indices->fg_color_index);
    if (value == NULL || strlen(value) < 3)
    {
        rgb = 0x00ffffff;
        alpha = 0xff;
    }
    else
    {
        int abgr = strtol(value+2, NULL, 16);
        rgb = HB_BGR_TO_RGB(abgr);
        alpha = abgr >> 24;
    }
    ctx->styles[style_index].fg_rgb = rgb;
    // SSA alpha is inverted 0 is opaque
    ctx->styles[style_index].fg_alpha = 255 - alpha;

    value = field_value(style, field_indices->alt_color_index);
    if (value == NULL || strlen(value) < 3)
    {
        rgb = 0x00ffffff;
        alpha = 0xff;
    }
    else
    {
        int abgr = strtol(value+2, NULL, 16);
        rgb = HB_BGR_TO_RGB(abgr);
        alpha = abgr >> 24;
    }
    ctx->styles[style_index].alt_rgb = rgb;
    ctx->styles[style_index].alt_alpha = alpha;

    value = field_value(style, field_indices->ol_color_index);
    if (value == NULL || strlen(value) < 3)
    {
        rgb = 0x000f0f0f;
        alpha = 0xff;
    }
    else
    {
        int abgr = strtol(value+2, NULL, 16);
        rgb = HB_BGR_TO_RGB(abgr);
        alpha = abgr >> 24;
    }
    ctx->styles[style_index].ol_rgb = rgb;
    ctx->styles[style_index].ol_alpha = alpha;

    value = field_value(style, field_indices->bg_color_index);
    if (value == NULL || strlen(value) < 3)
    {
        rgb = 0x000f0f0f;
        alpha = 0xff;
    }
    else
    {
        int abgr = strtol(value+2, NULL, 16);
        rgb = HB_BGR_TO_RGB(abgr);
        alpha = abgr >> 24;
    }
    ctx->styles[style_index].bg_rgb = rgb;
    ctx->styles[style_index].bg_alpha = alpha;

    ctx->styles[style_index].flags = 0;

    value = field_value(style, field_indices->bold_index);
    if (value == NULL)
    {
        flag = 0;
    }
    else
    {
        flag = strtol(value, NULL, 0) ? HB_STYLE_FLAG_BOLD : 0;
    }
    ctx->styles[style_index].flags |= flag;

    value = field_value(style, field_indices->italic_index);
    if (value == NULL)
    {
        flag = 0;
    }
    else
    {
        flag = strtol(value, NULL, 0) ? HB_STYLE_FLAG_ITALIC : 0;
    }
    ctx->styles[style_index].flags |= flag;

    value = field_value(style, field_indices->underline_index);
    if (value == NULL)
    {
        flag = 0;
    }
    else
    {
        flag = strtol(value, NULL, 0) ? HB_STYLE_FLAG_UNDERLINE : 0;
    }
    ctx->styles[style_index].flags |= flag;

    ctx->styles_count = style_index + 1;

    return 0;
}

hb_subtitle_style_context_t * hb_subtitle_style_init(const uint8_t * ssa_buf, int size)
{
    hb_subtitle_style_context_t * ctx;
    char * ssa_header = malloc(size + 1);

    if (ssa_header == NULL)
    {
        return NULL;
    }
    memcpy(ssa_header, ssa_buf, size);
    ssa_header[size] = 0;

    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
    {
        return NULL;
    }
    if (ssa_header != NULL)
    {
        // Find beginning of styles
        char *  pos = strstr(ssa_header, "[V4");

        if (pos != NULL)
        {
            pos = strstr(pos, "\nFormat:");
            if (pos != NULL)
            {
                char ** fields;
                char  * line = sgetline(pos + 8);

                fields = get_fields(line, 0);
                free(line);

                if (fields != NULL)
                {
                    ssa_style_indices_t field_indices;

                    fill_field_indices(fields, &field_indices);

                    pos    = strstr(pos, "\nStyle:");
                    while (pos != NULL)
                    {
                        char ** style;

                        line = sgetline(pos + 7);
                        style = get_fields(line, 0);
                        free(line);

                        if (add_style(ctx, style, &field_indices))
                        {
                            hb_str_vfree(style);
                            break;
                        }
                        hb_str_vfree(style);
                        pos = strstr(pos + 7, "\nStyle:");
                    }

                    hb_str_vfree(fields);
                }
            }
        }
    }
    ssa_style_reset(ctx);
    free(ssa_header);
    return ctx;
}

void hb_subtitle_style_close(hb_subtitle_style_context_t ** pctx)
{
    if (pctx == NULL || *pctx == NULL)
    {
        return;
    }
    hb_subtitle_style_context_t * ctx = *pctx;

    if (ctx->styles != NULL)
    {
        int ii;

        for (ii = 0; ii < ctx->styles_count; ii++)
        {
            free(ctx->styles[ii].name);
            free(ctx->styles[ii].font_name);
        }
    }
    free(ctx->styles);
    free(ctx);
    *pctx = NULL;
}

#define TX3G_STYLES (HB_STYLE_FLAG_BOLD   |   \
                     HB_STYLE_FLAG_ITALIC |   \
                     HB_STYLE_FLAG_UNDERLINE)

static int check_realloc_output(hb_tx3g_output_buf_t * output, int size)
{
    if (output->alloc < size)
    {
        uint8_t * tmp;

        output->alloc = size + 1024;
        output->size  = size;
        tmp = realloc(output->buf, output->alloc);
        if (tmp == NULL)
        {
            hb_error("realloc failed!");
            free(output->buf);
            output->size = 0;
            output->alloc = 0;
            output->buf   = NULL;
            return 0;
        }
        output->buf = tmp;
    }
    return 1;
}

static int tx3g_update_style_atoms(hb_tx3g_style_context_t *ctx, int stop)
{
    uint8_t * style_entry;
    uint8_t   face = 0;
    int       font_size;
    int       pos  = 10 + (12 * ctx->style_atom_count);
    int       size = 10 + (12 * (ctx->style_atom_count + 1));

    if (!check_realloc_output(&ctx->style_atoms, size))
    {
        return 0;
    }
    style_entry = ctx->style_atoms.buf + pos;

    if (ctx->out_style.flags & HB_STYLE_FLAG_BOLD)
    {
        face |= 1;
    }
    if (ctx->out_style.flags & HB_STYLE_FLAG_ITALIC)
    {
        face |= 2;
    }
    if (ctx->out_style.flags & HB_STYLE_FLAG_UNDERLINE)
    {
        face |= 4;
    }

    style_entry[0]  = (ctx->style_start >> 8) & 0xff;   // startChar
    style_entry[1]  = ctx->style_start & 0xff;
    style_entry[2]  = (stop >> 8) & 0xff;               // endChar
    style_entry[3]  = stop & 0xff;
    style_entry[4]  = 0;            // font-ID msb
    style_entry[5]  = 1;            // font-ID lsb
    style_entry[6]  = face;         // face-style-flags
    font_size       = 0.05 * ctx->height;
    if (font_size < 12)
    {
        font_size = 12;
    }
    else if (font_size > 255)
    {
        font_size = 255;
    }
    style_entry[7]  = font_size;    // font-size
    style_entry[8]  = (ctx->out_style.fg_rgb >> 16) & 0xff; // r
    style_entry[9]  = (ctx->out_style.fg_rgb >> 8)  & 0xff; // g
    style_entry[10] = (ctx->out_style.fg_rgb)       & 0xff; // b
    style_entry[11] = ctx->out_style.fg_alpha;              // a

    ctx->style_atom_count++;

    return 1;
}

static int tx3g_update_style(hb_tx3g_style_context_t *ctx, int utf8_end_pos)
{
    hb_subtitle_style_t * style = &ctx->in_style->current;

    // do we need to add a style atom?
    if (((ctx->out_style.flags  ^ style->flags) & TX3G_STYLES) ||
          ctx->out_style.fg_rgb   != style->fg_rgb             ||
          ctx->out_style.fg_alpha != style->fg_alpha           ||
          ctx->flush)
    {
        if (ctx->style_start < utf8_end_pos)
        {
            if (!tx3g_update_style_atoms(ctx, utf8_end_pos))
            {
                return 0;
            }
            ctx->style_start = utf8_end_pos;
        }
        ctx->out_style = *style;
        ctx->flush = 0;
    }

    return 1;
}

hb_tx3g_style_context_t *
hb_tx3g_style_init(int height, const uint8_t * ssa_buf, int size)
{
    hb_tx3g_style_context_t * ctx;

    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
    {
        return NULL;
    }
    ctx->in_style = hb_subtitle_style_init(ssa_buf, size);
    ctx->height            = height;
    ctx->style_atoms.buf   = NULL;
    ctx->style_atoms.size  = 0;
    ctx->style_atoms.alloc = 0;
    ctx->style_atom_count  = 0;
    ctx->style_start       = 0;
    ctx->out_style         = ctx->in_style->current;
    ctx->flush             = 1;

    return ctx;
}

void hb_tx3g_style_reset(hb_tx3g_style_context_t * ctx)
{
    ctx->style_atoms.buf   = NULL;
    ctx->style_atoms.size  = 0;
    ctx->style_atoms.alloc = 0;
    ctx->style_atom_count  = 0;
    ctx->style_start       = 0;
    ctx->out_style         = ctx->in_style->current;
    ctx->flush             = 1;
}

void hb_tx3g_style_close(hb_tx3g_style_context_t ** pctx)
{
    if (pctx == NULL || *pctx == NULL)
    {
        return;
    }
    hb_tx3g_style_context_t * ctx = *pctx;

    hb_subtitle_style_close(&ctx->in_style);
    free(ctx);
    *pctx = NULL;
}

/*
 * Copy the input to output removing markup and adding markup to the style
 * atom where appropriate.
 */
void hb_muxmp4_process_subtitle_style(
    hb_tx3g_style_context_t      * ctx,
    uint8_t                      * input,
    uint8_t                     ** out_buf,
    uint8_t                     ** out_style_atoms,
    uint16_t                     * stylesize)
{
    uint16_t                       utf8_pos = 0;
    int                            consumed, in_pos = 0, out_pos = 0, len;
    hb_tx3g_output_buf_t           output;
    char                         * text;
    const char                   * ssa_text, * style;

    output.buf       = NULL;
    output.alloc     = 0;
    output.size      = 0;
    *out_buf         = NULL;
    *out_style_atoms = NULL;
    *stylesize       = 0;

    ssa_style_reset(ctx->in_style);

    // Skip past the SSA preamble
    char ** event = get_fields((char*)input, 9);
    if (hb_str_vlen(event) < 9)
    {
        // Not enough fields
        goto fail;
    }

    style = event[2];
    ssa_text = event[8];
    ctx->in_style->event_style_default = ssa_style_set(ctx->in_style, style);
    hb_tx3g_style_reset(ctx);

    in_pos = 0;
    // Always allocate enough for empty string
    if (!check_realloc_output(&output, 1))
    {
        goto fail;
    }
    while (ssa_text[in_pos] != '\0')
    {
        text = ssa_to_text(ssa_text + in_pos, &consumed, ctx->in_style);
        if (text == NULL)
            break;

        // count UTF8 characters, and get length of text
        len = 0;
        int  ii, n;
        for (ii = 0; text[ii] != '\0'; ii += n)
        {
            int  jj;
            char c = text[ii];

            utf8_pos++;
            if      ((c & 0x80) == 0x00) n = 1;
            else if ((c & 0xE0) == 0xC0) n = 2;
            else if ((c & 0xF0) == 0xE0) n = 3;
            else if ((c & 0xF8) == 0xF0) n = 4;
            else                         n = 1; // invalid, but must handle

            // Prevent skipping null terminator
            for (jj = 1; jj < n && text[ii + jj] != '\0'; jj++);
            n = jj;
            len += n;
        }
        if (!check_realloc_output(&output, out_pos + len + 1))
        {
            goto fail;
        }
        strcpy((char*)output.buf + out_pos, text);
        free(text);
        out_pos += len;
        in_pos += consumed;
        if (!tx3g_update_style(ctx, utf8_pos))
        {
            goto fail;
        }
    }
    // Return to default style at end of line, flushes any pending
    // style changes
    ctx->flush = 1;
    if (!tx3g_update_style(ctx, utf8_pos))
    {
        goto fail;
    }

    // null terminate output string
    output.buf[out_pos] = 0;

    if (ctx->style_atom_count > 0)
    {
        *stylesize = 10 + (ctx->style_atom_count * 12);

        memcpy(ctx->style_atoms.buf + 4, "styl", 4);

        ctx->style_atoms.buf[0] = 0;
        ctx->style_atoms.buf[1] = 0;
        ctx->style_atoms.buf[2] = (*stylesize >> 8) & 0xff;
        ctx->style_atoms.buf[3] = *stylesize & 0xff;
        ctx->style_atoms.buf[8] = (ctx->style_atom_count >> 8) & 0xff;
        ctx->style_atoms.buf[9] = ctx->style_atom_count & 0xff;
        *out_style_atoms = ctx->style_atoms.buf;
    }
    *out_buf = output.buf;
    hb_str_vfree(event);

    return;

fail:
    hb_str_vfree(event);
    free(output.buf);
    free(ctx->style_atoms.buf);
}
