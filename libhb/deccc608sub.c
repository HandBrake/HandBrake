/* deccc608sub.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * From ccextractor, leave this file as intact and close to the original as possible so that
 * it is easy to patch in fixes - even though this file contains code that we don't need.
 *
 * Note that the SRT sub generation from CC could be useful for mkv subs.
 */
#include "hb.h"
#include "deccc608sub.h"

#define SSA_PREAMBLE_LEN 24
/*
 * ccextractor static configuration variables.
 */
static int debug_608 = 0;
static int cc_channel = 1;
static int subs_delay = 0;

/*
 * Get the time of the last buffer that we have received.
 */
static int64_t get_last_pts(struct s_write *wb)
{
    return wb->last_pts;
}

#define fatal(N, ...) // N

int rowdata[] = {11,-1,1,2,3,4,12,13,14,15,5,6,7,8,9,10};
// Relationship between the first PAC byte and the row number

// The following enc_buffer is not used at the moment, if it does get used
// we need to bring it into the swrite struct. Same for "str".
#define INITIAL_ENC_BUFFER_CAPACITY     2048

static const unsigned char pac2_attribs[][3]= // Color, font, ident
{
    {COL_WHITE,     FONT_REGULAR,               0},  // 0x40 || 0x60
    {COL_WHITE,     FONT_UNDERLINED,            0},  // 0x41 || 0x61
    {COL_GREEN,     FONT_REGULAR,               0},  // 0x42 || 0x62
    {COL_GREEN,     FONT_UNDERLINED,            0},  // 0x43 || 0x63
    {COL_BLUE,      FONT_REGULAR,               0},  // 0x44 || 0x64
    {COL_BLUE,      FONT_UNDERLINED,            0},  // 0x45 || 0x65
    {COL_CYAN,      FONT_REGULAR,               0},  // 0x46 || 0x66
    {COL_CYAN,      FONT_UNDERLINED,            0},  // 0x47 || 0x67
    {COL_RED,       FONT_REGULAR,               0},  // 0x48 || 0x68
    {COL_RED,       FONT_UNDERLINED,            0},  // 0x49 || 0x69
    {COL_YELLOW,    FONT_REGULAR,               0},  // 0x4a || 0x6a
    {COL_YELLOW,    FONT_UNDERLINED,            0},  // 0x4b || 0x6b
    {COL_MAGENTA,   FONT_REGULAR,               0},  // 0x4c || 0x6c
    {COL_MAGENTA,   FONT_UNDERLINED,            0},  // 0x4d || 0x6d
    {COL_WHITE,     FONT_ITALICS,               0},  // 0x4e || 0x6e
    {COL_WHITE,     FONT_UNDERLINED_ITALICS,    0},  // 0x4f || 0x6f
    {COL_WHITE,     FONT_REGULAR,               0},  // 0x50 || 0x70
    {COL_WHITE,     FONT_UNDERLINED,            0},  // 0x51 || 0x71
    {COL_WHITE,     FONT_REGULAR,               4},  // 0x52 || 0x72
    {COL_WHITE,     FONT_UNDERLINED,            4},  // 0x53 || 0x73
    {COL_WHITE,     FONT_REGULAR,               8},  // 0x54 || 0x74
    {COL_WHITE,     FONT_UNDERLINED,            8},  // 0x55 || 0x75
    {COL_WHITE,     FONT_REGULAR,               12}, // 0x56 || 0x76
    {COL_WHITE,     FONT_UNDERLINED,            12}, // 0x57 || 0x77
    {COL_WHITE,     FONT_REGULAR,               16}, // 0x58 || 0x78
    {COL_WHITE,     FONT_UNDERLINED,            16}, // 0x59 || 0x79
    {COL_WHITE,     FONT_REGULAR,               20}, // 0x5a || 0x7a
    {COL_WHITE,     FONT_UNDERLINED,            20}, // 0x5b || 0x7b
    {COL_WHITE,     FONT_REGULAR,               24}, // 0x5c || 0x7c
    {COL_WHITE,     FONT_UNDERLINED,            24}, // 0x5d || 0x7d
    {COL_WHITE,     FONT_REGULAR,               28}, // 0x5e || 0x7e
    {COL_WHITE,     FONT_UNDERLINED,            28}  // 0x5f || 0x7f
};

// Default color
static enum color_code default_color=COL_WHITE;

static const char *command_type[] =
{
    "Unknown",
    "EDM - EraseDisplayedMemory",
    "RCL - ResumeCaptionLoading",
    "EOC - End Of Caption",
    "TO1 - Tab Offset, 1 column",
    "TO2 - Tab Offset, 2 column",
    "TO3 - Tab Offset, 3 column",
    "RU2 - Roll up 2 rows",
    "RU3 - Roll up 3 rows",
    "RU4 - Roll up 4 rows",
    "CR  - Carriage Return",
    "ENM - Erase non-displayed memory",
    "BS  - Backspace",
    "RTD - Resume Text Display"
};

static const char *font_text[]=
{
    "regular",
    "italics",
    "underlined",
    "underlined italics"
};

static const char *color_text[][2]=
{
    {"white",       "&HFFFFFF&"},
    {"green",       "&H00FF00&"},
    {"blue",        "&HFF0000&"},
    {"cyan",        "&HFFFF00&"},
    {"red",         "&H0000FF&"},
    {"yellow",      "&H00FFFF&"},
    {"magenta",     "&HFF00FF&"},
    {"userdefined", "&HFFFFFF&"}
};

static int general_608_init (struct s_write *wb)
{
    if( !wb->enc_buffer )
    {
        wb->enc_buffer=(unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY);
        if (wb->enc_buffer==NULL)
            return -1;
        wb->enc_buffer_capacity=INITIAL_ENC_BUFFER_CAPACITY;
    }

    if( !wb->subline) {
        wb->subline = malloc(2048);

        if (!wb->subline)
        {
            return -1;
        }
    }

    wb->new_sentence = 1;
    wb->new_channel = 1;
    wb->in_xds_mode = 0;

    hb_buffer_list_clear(&wb->list);
    wb->last_pts = 0;
    return 0;
}

/*
 * Free up CC memory - don't call this from HB just yet since it will cause
 * parallel encodes to fail - to be honest they will be stuffed anyway since
 * the CC's may be overwriting the buffers.
 */
static void general_608_close (struct s_write *wb)
{
    if( wb->enc_buffer ) {
        free(wb->enc_buffer);
        wb->enc_buffer_capacity = 0;
        wb->enc_buffer_used = 0;
    }
    if( wb->subline ) {
        free(wb->subline);
    }
    hb_buffer_list_close(&wb->list);
}


#include <ctype.h>

// Returns number of bytes used
static int get_char_in_utf8(unsigned char *buffer, unsigned char c)
{
    if (c == 0x00)
        return 0;

    // Regular line-21 character set, mostly ASCII except these exceptions
    if (c < 0x80)
    {
        switch (c)
        {
        case 0x2a: // lowercase a, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0xa1;
            return 2;
        case 0x5c: // lowercase e, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0xa9;
            return 2;
        case 0x5e: // lowercase i, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0xad;
            return 2;
        case 0x5f: // lowercase o, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0xb3;
            return 2;
        case 0x60: // lowercase u, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0xba;
            return 2;
        case 0x7b: // lowercase c with cedilla
            *buffer = 0xc3;
            *(buffer+1) = 0xa7;
            return 2;
        case 0x7c: // division symbol
            *buffer = 0xc3;
            *(buffer+1) = 0xb7;
            return 2;
        case 0x7d: // uppercase N tilde
            *buffer = 0xc3;
            *(buffer+1) = 0x91;
            return 2;
        case 0x7e: // lowercase n tilde
            *buffer = 0xc3;
            *(buffer+1) = 0xb1;
            return 2;
        default:
            *buffer = c;
            return 1;
        }
    }
    switch (c)
    {
        // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE = 0x11 AND LOW BETWEEN 0x30 AND 0x3F
        case 0x80: // Registered symbol (R)
            *buffer = 0xc2;
            *(buffer+1) = 0xae;
            return 2;
        case 0x81: // degree sign
            *buffer = 0xc2;
            *(buffer+1) = 0xb0;
            return 2;
        case 0x82: // 1/2 symbol
            *buffer = 0xc2;
            *(buffer+1) = 0xbd;
            return 2;
        case 0x83: // Inverted (open) question mark
            *buffer = 0xc2;
            *(buffer+1) = 0xbf;
            return 2;
        case 0x84: // Trademark symbol (TM)
            *buffer = 0xe2;
            *(buffer+1) = 0x84;
            *(buffer+2) = 0xa2;
            return 3;
        case 0x85: // Cents symbol
            *buffer = 0xc2;
            *(buffer+1) = 0xa2;
            return 2;
        case 0x86: // Pounds sterling
            *buffer = 0xc2;
            *(buffer+1) = 0xa3;
            return 2;
        case 0x87: // Music note
            *buffer = 0xe2;
            *(buffer+1) = 0x99;
            *(buffer+2) = 0xaa;
            return 3;
        case 0x88: // lowercase a, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0xa0;
            return 2;
        case 0x89: // transparent space, we make it regular
            *buffer = 0x20;
            return 1;
        case 0x8a: // lowercase e, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0xa8;
            return 2;
        case 0x8b: // lowercase a, circumflex accent
            *buffer = 0xc3;
            *(buffer+1) = 0xa2;
            return 2;
        case 0x8c: // lowercase e, circumflex accent
            *buffer = 0xc3;
            *(buffer+1) = 0xaa;
            return 2;
        case 0x8d: // lowercase i, circumflex accent
            *buffer = 0xc3;
            *(buffer+1) = 0xae;
            return 2;
        case 0x8e: // lowercase o, circumflex accent
            *buffer = 0xc3;
            *(buffer+1) = 0xb4;
            return 2;
        case 0x8f: // lowercase u, circumflex accent
            *buffer = 0xc3;
            *(buffer+1) = 0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE = 0x12 AND LOW BETWEEN 0x20 AND 0x3F
        case 0x90: // capital letter A with acute
            *buffer = 0xc3;
            *(buffer+1) = 0x81;
            return 2;
        case 0x91: // capital letter E with acute
            *buffer = 0xc3;
            *(buffer+1) = 0x89;
            return 2;
        case 0x92: // capital letter O with acute
            *buffer = 0xc3;
            *(buffer+1) = 0x93;
            return 2;
        case 0x93: // capital letter U with acute
            *buffer = 0xc3;
            *(buffer+1) = 0x9a;
            return 2;
        case 0x94: // capital letter U with diaresis
            *buffer = 0xc3;
            *(buffer+1) = 0x9c;
            return 2;
        case 0x95: // lowercase letter U with diaeresis
            *buffer = 0xc3;
            *(buffer+1) = 0xbc;
            return 2;
        case 0x96: // apostrophe
            *buffer = 0x27;
            return 1;
        case 0x97: // inverted exclamation mark
            *buffer = 0xc2;
            *(buffer+1) = 0xa1;
            return 2;
        case 0x98: // asterisk
            *buffer = 0x2a;
            return 1;
        case 0x99: // apostrophe (yes, duped). See CCADI source code.
            *buffer = 0x27;
            return 1;
        case 0x9a: // hyphen-minus
            *buffer = 0x2d;
            return 1;
        case 0x9b: // copyright sign
            *buffer = 0xc2;
            *(buffer+1) = 0xa9;
            return 2;
        case 0x9c: // Service mark
            *buffer = 0xe2;
            *(buffer+1) = 0x84;
            *(buffer+2) = 0xa0;
            return 3;
        case 0x9d: // Full stop (.)
            *buffer = 0x2e;
            return 1;
        case 0x9e: // Quoatation mark
            *buffer = 0x22;
            return 1;
        case 0x9f: // Quoatation mark
            *buffer = 0x22;
            return 1;
        case 0xa0: // uppercase A, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0x80;
            return 2;
        case 0xa1: // uppercase A, circumflex
            *buffer = 0xc3;
            *(buffer+1) = 0x82;
            return 2;
        case 0xa2: // uppercase C with cedilla
            *buffer = 0xc3;
            *(buffer+1) = 0x87;
            return 2;
        case 0xa3: // uppercase E, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0x88;
            return 2;
        case 0xa4: // uppercase E, circumflex
            *buffer = 0xc3;
            *(buffer+1) = 0x8a;
            return 2;
        case 0xa5: // capital letter E with diaresis
            *buffer = 0xc3;
            *(buffer+1) = 0x8b;
            return 2;
        case 0xa6: // lowercase letter e with diaresis
            *buffer = 0xc3;
            *(buffer+1) = 0xab;
            return 2;
        case 0xa7: // uppercase I, circumflex
            *buffer = 0xc3;
            *(buffer+1) = 0x8e;
            return 2;
        case 0xa8: // uppercase I, with diaresis
            *buffer = 0xc3;
            *(buffer+1) = 0x8f;
            return 2;
        case 0xa9: // lowercase i, with diaresis
            *buffer = 0xc3;
            *(buffer+1) = 0xaf;
            return 2;
        case 0xaa: // uppercase O, circumflex
            *buffer = 0xc3;
            *(buffer+1) = 0x94;
            return 2;
        case 0xab: // uppercase U, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0x99;
            return 2;
        case 0xac: // lowercase u, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0xb9;
            return 2;
        case 0xad: // uppercase U, circumflex
            *buffer = 0xc3;
            *(buffer+1) = 0x9b;
            return 2;
        case 0xae: // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer = 0xc2;
            *(buffer+1) = 0xab;
            return 2;
        case 0xaf: // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer = 0xc2;
            *(buffer+1) = 0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE = 0x13 AND LOW BETWEEN 0x20 AND 0x3F
        case 0xb0: // Uppercase A, tilde
            *buffer = 0xc3;
            *(buffer+1) = 0x83;
            return 2;
        case 0xb1: // Lowercase a, tilde
            *buffer = 0xc3;
            *(buffer+1) = 0xa3;
            return 2;
        case 0xb2: // Uppercase I, acute accent
            *buffer = 0xc3;
            *(buffer+1) = 0x8d;
            return 2;
        case 0xb3: // Uppercase I, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0x8c;
            return 2;
        case 0xb4: // Lowercase i, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0xac;
            return 2;
        case 0xb5: // Uppercase O, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0x92;
            return 2;
        case 0xb6: // Lowercase o, grave accent
            *buffer = 0xc3;
            *(buffer+1) = 0xb2;
            return 2;
        case 0xb7: // Uppercase O, tilde
            *buffer = 0xc3;
            *(buffer+1) = 0x95;
            return 2;
        case 0xb8: // Lowercase o, tilde
            *buffer = 0xc3;
            *(buffer+1) = 0xb5;
            return 2;
        case 0xb9: // Open curly brace
            *buffer = 0x7b;
            return 1;
        case 0xba: // Closing curly brace
            *buffer = 0x7d;
            return 1;
        case 0xbb: // Backslash
            *buffer = 0x5c;
            return 1;
        case 0xbc: // Caret
            *buffer = 0x5e;
            return 1;
        case 0xbd: // Underscore
            *buffer = 0x5f;
            return 1;
        case 0xbe: // Pipe (broken bar)
            *buffer = 0xc2;
            *(buffer+1) = 0xa6;
            return 1;
        case 0xbf: // Tilde
            *buffer = 0x7e; // Not sure
            return 1;
        case 0xc0: // Uppercase A, umlaut
            *buffer = 0xc3;
            *(buffer+1) = 0x84;
            return 2;
        case 0xc1: // Lowercase A, umlaut
            *buffer = 0xc3;
            *(buffer+1) = 0xa4;
            return 2;
        case 0xc2: // Uppercase O, umlaut
            *buffer = 0xc3;
            *(buffer+1) = 0x96;
            return 2;
        case 0xc3: // Lowercase o, umlaut
            *buffer = 0xc3;
            *(buffer+1) = 0xb6;
            return 2;
        case 0xc4: // Esszett (sharp S)
            *buffer = 0xc3;
            *(buffer+1) = 0x9f;
            return 2;
        case 0xc5: // Yen symbol
            *buffer = 0xc2;
            *(buffer+1) = 0xa5;
            return 2;
        case 0xc6: // Currency symbol
            *buffer = 0xc2;
            *(buffer+1) = 0xa4;
            return 2;
        case 0xc7: // Vertical bar
            *buffer = 0x7c;
            return 1;
        case 0xc8: // Uppercase A, ring
            *buffer = 0xc3;
            *(buffer+1) = 0x85;
            return 2;
        case 0xc9: // Lowercase A, ring
            *buffer = 0xc3;
            *(buffer+1) = 0xa5;
            return 2;
        case 0xca: // Uppercase O, slash
            *buffer = 0xc3;
            *(buffer+1) = 0x98;
            return 2;
        case 0xcb: // Lowercase o, slash
            *buffer = 0xc3;
            *(buffer+1) = 0xb8;
            return 2;
        case 0xcc: // Upper left corner
            *buffer = 0xe2;
            *(buffer+1) = 0x8c;
            *(buffer+2) = 0x9c;
            return 3;
        case 0xcd: // Upper right corner
            *buffer = 0xe2;
            *(buffer+1) = 0x8c;
            *(buffer+2) = 0x9d;
            return 3;
        case 0xce: // Lower left corner
            *buffer = 0xe2;
            *(buffer+1) = 0x8c;
            *(buffer+2) = 0x9e;
            return 3;
        case 0xcf: // Lower right corner
            *buffer = 0xe2;
            *(buffer+1) = 0x8c;
            *(buffer+2) = 0x9f;
            return 3;
        default: //
            *buffer = '?'; // I'll do it eventually, I promise
            return 1; // This are weird chars anyway
    }
}

// Encodes a generic string. Note that since we use the encoders for closed
// caption data, text would have to be encoded as CCs... so using special
// characters here it's a bad idea.
static unsigned encode_line(unsigned char *buffer, unsigned char *text)
{
    unsigned bytes = 0;
    while (*text)
    {
        *buffer++ = *text++;
        bytes++;
    }
    return bytes;
}

static unsigned stuff_space(unsigned char *buffer, int space)
{
    int ii;
    for (ii = 0; ii < space; ii++)
    {
        *buffer++ = '\\';
        *buffer++ = 'h';
    }
    return space * 2;
}

static void find_limit_characters(unsigned char *line, int *first_non_blank,
                                  int *last_non_blank)
{
    int i;

    *last_non_blank = -1;
    *first_non_blank = -1;
    for (i = 0; i < CC608_SCREEN_WIDTH; i++)
    {
        unsigned char c = line[i];
        if (c != ' ' && c != 0x89)
        {
            if (*first_non_blank == -1)
                *first_non_blank = i;
            *last_non_blank = i;
        }
    }
}

static unsigned get_decoder_line_encoded(struct s_write *wb,
                                         unsigned char *buffer, int line_num,
                                         struct eia608_screen *data)
{
    uint8_t font_style;
    uint8_t font_color;
    int i;

    unsigned char *line = data->characters[line_num];
    unsigned char *orig = buffer; // Keep for debugging
    int first = 0, last = 31;

    find_limit_characters(line, &first, &last);
    for (i = first; i <= last; i++)
    {
        // Handle color
        font_color = data->colors[line_num][i];
        font_style = data->fonts[line_num][i];

        // Handle reset to defaults
        if ((font_style & FONT_STYLE_MASK) == 0 && font_color == COL_WHITE)
        {
            if (((font_style ^ wb->prev_font_style) & FONT_STYLE_MASK) ||
                (font_color != wb->prev_font_color))
            {
                buffer += encode_line(buffer, (uint8_t*)"{\\r}");
            }
        }
        else
        {
            // Open markup
            if (((font_style ^ wb->prev_font_style) & FONT_STYLE_MASK) ||
                (font_color != wb->prev_font_color))
            {
                // style changed
                buffer += encode_line(buffer, (uint8_t*)"{");
            }

            // Handle underlined
            if ((font_style ^ wb->prev_font_style) & FONT_UNDERLINED)
            {
                int enable = !!(font_style & FONT_UNDERLINED);
                buffer += encode_line(buffer, (uint8_t*)"\\u");
                *buffer++ = enable + 0x30;
            }

            // Handle italics
            if ((font_style ^ wb->prev_font_style) & FONT_ITALICS)
            {
                int enable = !!(font_style & FONT_ITALICS);
                buffer += encode_line(buffer, (uint8_t*)"\\i");
                *buffer++ = enable + 0x30;
            }

            // Handle color
            if (font_color != wb->prev_font_color)
            {
                buffer += encode_line(buffer, (uint8_t*)"\\1c");
                buffer += encode_line(buffer,
                                      (uint8_t*)color_text[font_color][1]);
            }

            // Close markup
            if (((font_style ^ wb->prev_font_style) & FONT_STYLE_MASK) ||
                (font_color != wb->prev_font_color))
            {
                // style changed
                buffer += encode_line(buffer, (uint8_t*)"}");
            }
        }
        wb->prev_font_style = font_style;
        wb->prev_font_color = font_color;

        int bytes = 0;
        bytes = get_char_in_utf8(buffer, line[i]);
        buffer += bytes;
    }
    *buffer = 0;
    return (unsigned) (buffer - orig); // Return length
}


static void clear_eia608_cc_buffer (struct eia608_screen *data)
{
    int i;

    for (i=0;i<15;i++)
    {
        memset(data->characters[i],' ',CC608_SCREEN_WIDTH);
        data->characters[i][CC608_SCREEN_WIDTH]=0;
        memset (data->colors[i],default_color,CC608_SCREEN_WIDTH+1);
        memset (data->fonts[i],FONT_REGULAR,CC608_SCREEN_WIDTH+1);
        data->row_used[i]=0;
    }
    data->empty=1;
}

static void init_eia608 (struct eia608 *data)
{
    data->cursor_column = 0;
    data->cursor_row = 0;
    clear_eia608_cc_buffer (&data->buffer1);
    clear_eia608_cc_buffer (&data->buffer2);
    data->visible_buffer = 1;
    data->last_c1 = 0;
    data->last_c2 = 0;
    data->mode = MODE_POPUP;
    data->current_visible_start_ms = 0;
    data->ssa_counter = 0;
    data->screenfuls_counter = 0;
    data->channel = 1;
    data->color = default_color;
    data->font = FONT_REGULAR;
    data->rollup_base_row = 14;
}

static struct eia608_screen *get_current_hidden_buffer(struct s_write *wb)
{
    struct eia608_screen *data;
    if (wb->data608->visible_buffer == 1)
        data = &wb->data608->buffer2;
    else
        data = &wb->data608->buffer1;
    return data;
}

static struct eia608_screen *get_current_visible_buffer(struct s_write *wb)
{
    struct eia608_screen *data;
    if (wb->data608->visible_buffer == 1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;
    return data;
}

static void swap_visible_buffer(struct s_write *wb)
{
    wb->data608->visible_buffer = (wb->data608->visible_buffer == 1) ? 2 : 1;
}

static struct eia608_screen *get_writing_buffer(struct s_write *wb)
{
    struct eia608_screen *use_buffer=NULL;
    switch (wb->data608->mode)
    {
        case MODE_POPUP: // Write on the non-visible buffer
            use_buffer = get_current_hidden_buffer(wb);
            break;
        case MODE_ROLLUP_2: // Write directly to screen
        case MODE_ROLLUP_3:
        case MODE_ROLLUP_4:
            use_buffer = get_current_visible_buffer(wb);
            break;
        default:
            fatal (EXIT_BUG_BUG, "Caption mode has an illegal value at get_writing_buffer(), this is a bug.\n");
    }
    return use_buffer;
}

static void write_char(const unsigned char c, struct s_write *wb)
{
    if (wb->data608->mode != MODE_TEXT)
    {
        struct eia608_screen * use_buffer = get_writing_buffer(wb);
        /* hb_log ("\rWriting char [%c] at %s:%d:%d\n",c,
        use_buffer == &wb->data608->buffer1?"B1":"B2",
        wb->data608->cursor_row,wb->data608->cursor_column); */
        use_buffer->characters[wb->data608->cursor_row][wb->data608->cursor_column] = c;
        use_buffer->colors[wb->data608->cursor_row][wb->data608->cursor_column] = wb->data608->color;
        use_buffer->fonts[wb->data608->cursor_row][wb->data608->cursor_column] = wb->data608->font;
        use_buffer->row_used[wb->data608->cursor_row] = 1;
        use_buffer->empty = 0;
        if (wb->data608->cursor_column < 31)
            wb->data608->cursor_column++;
        use_buffer->dirty = 1;
    }

}

/* Handle MID-ROW CODES. */
static void handle_text_attr(const unsigned char c1, const unsigned char c2,
                             struct s_write *wb)
{
    // Handle channel change
    wb->data608->channel=wb->new_channel;
    if (wb->data608->channel!=cc_channel)
        return;
    if (debug_608)
        hb_log ("\r608: text_attr: %02X %02X",c1,c2);
    if ( ((c1!=0x11 && c1!=0x19) ||
        (c2<0x20 || c2>0x2f)) && debug_608)
    {
        hb_log ("\rThis is not a text attribute!\n");
    }
    else
    {
        int i = c2-0x20;
        wb->data608->color=pac2_attribs[i][0];
        wb->data608->font=pac2_attribs[i][1];
        if (debug_608)
            hb_log("  --  Color: %s,  font: %s\n",
                color_text[wb->data608->color][0],
                font_text[wb->data608->font]);
        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
    }
}

static int write_cc_buffer_as_ssa(struct eia608_screen *data,
                                  struct s_write *wb)
{
    int wrote_something = 0;
    int i;
    int64_t ms_start = wb->data608->current_visible_start_ms;
    //int64_t ms_end = get_last_pts(wb) + subs_delay;

    ms_start += subs_delay;
    if (ms_start<0) // Drop screens that because of subs_delay start too early
        return 0;

    if (debug_608)
    {
        char timeline[128];
        wb->data608->ssa_counter++;
        sprintf (timeline,"%u\r\n",wb->data608->ssa_counter);

        hb_log ("\n- - - SSA caption - - -\n");
        hb_log ("%s", timeline);
    }

    /*
     * Write all the lines into enc_buffer, and then write that out at the end
     * ensure that we only have two lines, insert a newline after the first one,
     * and have a big bottom line (strip spaces from any joined lines).
     */
    int rows = 0, columns = 0;
    int min_row = 15, max_row = 0;
    int min_col = 41, max_col = 0;
    for (i = 0; i < 15; i++)
    {
        if (data->row_used[i])
        {
            int first, last;

            rows++;
            find_limit_characters(data->characters[i], &first, &last);
            if (last - first + 1 > columns)
                columns = last - first + 1;
            if (min_col > first)
                min_col = first;
            if (min_row > i)
                min_row = i;
            if (max_col < last)
                max_col = last;
            if (max_row < i)
                max_row = i;
        }
    }

    wb->prev_font_style = FONT_REGULAR;
    wb->prev_font_color = COL_WHITE;
    wb->enc_buffer_used = 0;

    int cropped_width, cropped_height, font_size;
    int cell_width, cell_height;
    int safe_x, safe_y;
    int min_safe_x, min_safe_y;
    double aspect;

    cropped_height = wb->height - wb->crop[0] - wb->crop[1];
    cropped_width = wb->width - wb->crop[2] - wb->crop[3];
    aspect = (double)wb->width * wb->par.num /
                    (wb->height * wb->par.den);

    // CC grid is 16 rows by 32 colums (for 4:3 video)
    // Our SSA resolution is the title resolution
    // Tranlate CC grid to SSA coordinates
    // The numbers are tweaked to keep things off the very
    // edges of the screen and in the "safe" zone
    int screen_columns = 32;
    if (aspect >= 1.6)
    {
        // If the display aspect is close to or greater than 16:9
        // then width of screen is 42 columns (see CEA-708)
        screen_columns = 42;
    }
    font_size = wb->height * .8 * .08;

    safe_x = 0.1 * wb->width;
    safe_y = 0.1 * wb->height;
    min_safe_x = 0.025 * cropped_width;
    min_safe_y = 0.025 * cropped_height;
    cell_height = (wb->height - 2 * safe_y) / 16; 
    cell_width  = (wb->width  - 2 * safe_x) / screen_columns; 

    char *pos;
    int y, x, top;
    int col = min_col;
    if (aspect >= 1.6)
    {
        // If the display aspect is close to or greater than 16:9
        // center the CC in about a 4:3 region
        col += 5;
    }
    y = cell_height * (min_row + 1 + rows) + safe_y - wb->crop[0];
    x = cell_width * col + safe_x - wb->crop[2];
    top = y - rows * font_size;

    if (top < min_safe_y)
        y = (rows * font_size) + min_safe_y;
    if (y > cropped_height - min_safe_y)
        y = cropped_height - min_safe_y;
    if (x + columns * cell_width > cropped_width - min_safe_x)
        x = cropped_width - columns * cell_width - min_safe_x;
    if (x < min_safe_x)
        x = min_safe_x;
    pos = hb_strdup_printf("{\\an1\\pos(%d,%d)}", x, y);

    int line = 1;
    for (i = 0; i < 15; i++)
    {
        if (data->row_used[i])
        {
            int first, last;
            // Get position for this CC
            find_limit_characters(data->characters[i], &first, &last);

            /*
             * The intention was to use a newline but QT doesn't like it,
             * old code still here just in case..
             */
            int space = first - min_col;
            if (line == 1) {
                wb->enc_buffer_used += encode_line(
                        wb->enc_buffer + wb->enc_buffer_used, (uint8_t*)pos);
                wb->enc_buffer_used += stuff_space(
                        wb->enc_buffer + wb->enc_buffer_used, space);
                wb->enc_buffer_used += get_decoder_line_encoded(wb,
                        wb->enc_buffer + wb->enc_buffer_used, i, data);
                line = 2;
            } else {
                wb->enc_buffer_used += encode_line(
                        wb->enc_buffer + wb->enc_buffer_used, (uint8_t*)"\\N");
                wb->enc_buffer_used += stuff_space(
                        wb->enc_buffer + wb->enc_buffer_used, space);
                wb->enc_buffer_used += get_decoder_line_encoded(wb,
                        wb->enc_buffer + wb->enc_buffer_used, i, data);
            }
        }
    }
    free(pos);
    if (wb->enc_buffer_used && wb->enc_buffer[0] != 0 && data->dirty)
    {
        hb_buffer_t *buffer;
        int len;

        // bump past null terminator
        wb->enc_buffer_used++;
        buffer = hb_buffer_init(wb->enc_buffer_used + SSA_PREAMBLE_LEN);
        buffer->s.frametype = HB_FRAME_SUBTITLE;
        buffer->s.start = ms_start;
        buffer->s.stop = AV_NOPTS_VALUE;
        sprintf((char*)buffer->data, "%d,,Default,,0,0,0,,", ++wb->line);
        len = strlen((char*)buffer->data);
        memcpy(buffer->data + len, wb->enc_buffer, wb->enc_buffer_used);
        hb_buffer_list_append(&wb->list, buffer);
        wrote_something=1;
        wb->clear_sub_needed = 1;
    }
    else if (wb->clear_sub_needed)
    {
        hb_buffer_t *buffer = hb_buffer_init(1);
        buffer->s.frametype = HB_FRAME_SUBTITLE;
        buffer->s.start = ms_start;
        buffer->s.stop = ms_start;
        buffer->data[0] = 0;
        hb_buffer_list_append(&wb->list, buffer);
        wb->clear_sub_needed = 0;
    }
    if (debug_608)
    {
        hb_log ("- - - - - - - - - - - -\r\n");
    }
    return wrote_something;
}

static int write_cc_buffer(struct s_write *wb)
{
    struct eia608_screen *data;
    int wrote_something=0;

    data = get_current_visible_buffer(wb);
    if (!data->dirty)
        return 0;
    wb->new_sentence=1;
    wrote_something = write_cc_buffer_as_ssa(data, wb);
    data->dirty = 0;
    return wrote_something;
}

static void move_roll_up(struct s_write *wb, int row)
{
    struct eia608_screen *use_buffer;
    int ii, src, dst, keep_lines;

    switch (wb->data608->mode)
    {
        case MODE_ROLLUP_2:
            keep_lines = 2;
            break;
        case MODE_ROLLUP_3:
            keep_lines = 3;
            break;
        case MODE_ROLLUP_4:
            keep_lines = 4;
            break;
        default:
            // Not rollup mode, nothing to do
            return;
    }

    if (row == wb->data608->rollup_base_row)
    {
        // base row didn't change, nothing to do
        return;
    }

    use_buffer = get_current_visible_buffer(wb);
    if (row < wb->data608->rollup_base_row)
    {
        src = wb->data608->rollup_base_row - keep_lines + 1;
        dst = row - keep_lines + 1;
        for (ii = 0; ii < keep_lines; ii++)
        {
            memcpy(use_buffer->characters[dst], use_buffer->characters[src], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->colors[dst], use_buffer->colors[src], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->fonts[dst], use_buffer->fonts[src], CC608_SCREEN_WIDTH+1);
            use_buffer->row_used[dst] = use_buffer->row_used[src];

            memset(use_buffer->characters[src], ' ', CC608_SCREEN_WIDTH);
            memset(use_buffer->colors[src], COL_WHITE, CC608_SCREEN_WIDTH);
            memset(use_buffer->fonts[src], FONT_REGULAR, CC608_SCREEN_WIDTH);
            use_buffer->characters[src][CC608_SCREEN_WIDTH] = 0;
            use_buffer->row_used[src] = 0;

            src++;
            dst++;
        }
    }
    else
    {
        src = wb->data608->rollup_base_row;
        dst = row;
        for (ii = 0; ii < keep_lines; ii++)
        {
            memcpy(use_buffer->characters[dst], use_buffer->characters[src], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->colors[dst], use_buffer->colors[src], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->fonts[dst], use_buffer->fonts[src], CC608_SCREEN_WIDTH+1);
            use_buffer->row_used[dst] = use_buffer->row_used[src];

            memset(use_buffer->characters[src], ' ', CC608_SCREEN_WIDTH);
            memset(use_buffer->colors[src], COL_WHITE, CC608_SCREEN_WIDTH);
            memset(use_buffer->fonts[src], FONT_REGULAR, CC608_SCREEN_WIDTH);
            use_buffer->characters[src][CC608_SCREEN_WIDTH] = 0;
            use_buffer->row_used[src] = 0;

            src--;
            dst--;
        }
    }
    use_buffer->dirty = 1;
}

static void roll_up(struct s_write *wb)
{
    struct eia608_screen *use_buffer;
    int i, j;

    use_buffer = get_current_visible_buffer(wb);
    int keep_lines;
    switch (wb->data608->mode)
    {
        case MODE_ROLLUP_2:
            keep_lines = 2;
            break;
        case MODE_ROLLUP_3:
            keep_lines = 3;
            break;
        case MODE_ROLLUP_4:
            keep_lines = 4;
            break;
        default: // Shouldn't happen
            keep_lines = 0;
            break;
    }
    int firstrow = -1, lastrow = -1;
    // Look for the last line used
    int rows_now = 0; // Number of rows in use right now
    for (i = 0; i < 15; i++)
    {
        if (use_buffer->row_used[i])
        {
            rows_now++;
            if (firstrow == -1)
                firstrow = i;
            lastrow = i;
        }
    }

    if (debug_608)
        hb_log ("\rIn roll-up: %d lines used, first: %d, last: %d\n", rows_now, firstrow, lastrow);

    if (lastrow==-1) // Empty screen, nothing to rollup
        return;

    for (j = lastrow - keep_lines + 1; j < lastrow; j++)
    {
        if (j >= 0)
        {
            memcpy(use_buffer->characters[j], use_buffer->characters[j+1], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->colors[j], use_buffer->colors[j+1], CC608_SCREEN_WIDTH+1);
            memcpy(use_buffer->fonts[j], use_buffer->fonts[j+1], CC608_SCREEN_WIDTH+1);
            use_buffer->row_used[j] = use_buffer->row_used[j+1];
        }
    }
    for (j = 0; j < (1 + wb->data608->cursor_row - keep_lines); j++)
    {
        memset(use_buffer->characters[j], ' ', CC608_SCREEN_WIDTH);
        memset(use_buffer->colors[j], COL_WHITE, CC608_SCREEN_WIDTH);
        memset(use_buffer->fonts[j], FONT_REGULAR, CC608_SCREEN_WIDTH);
        use_buffer->characters[j][CC608_SCREEN_WIDTH] = 0;
        use_buffer->row_used[j] = 0;
    }
    memset(use_buffer->characters[lastrow], ' ', CC608_SCREEN_WIDTH);
    memset(use_buffer->colors[lastrow], COL_WHITE, CC608_SCREEN_WIDTH);
    memset(use_buffer->fonts[lastrow], FONT_REGULAR, CC608_SCREEN_WIDTH);

    use_buffer->characters[lastrow][CC608_SCREEN_WIDTH] = 0;
    use_buffer->row_used[lastrow] = 0;

    // Sanity check
    rows_now = 0;
    for (i = 0; i < 15; i++)
        if (use_buffer->row_used[i])
            rows_now++;
    if (rows_now > keep_lines)
        hb_log ("Bug in roll_up, should have %d lines but I have %d.\n",
            keep_lines, rows_now);
    use_buffer->dirty = 1;
}

void erase_memory (struct s_write *wb, int displayed)
{
    struct eia608_screen *buf;
    if (displayed)
    {
        buf = get_current_visible_buffer(wb);
    }
    else
    {
        buf = get_current_hidden_buffer(wb);
    }
    clear_eia608_cc_buffer (buf);
}

static int is_current_row_empty (struct s_write *wb)
{
    struct eia608_screen *use_buffer;
    int i;

    use_buffer = get_current_visible_buffer(wb);
    for (i=0;i<CC608_SCREEN_WIDTH;i++)
    {
        if (use_buffer->characters[wb->data608->rollup_base_row][i]!=' ')
            return 0;
    }
    return 1;
}

/* Process GLOBAL CODES */
static void handle_command(unsigned char c1, const unsigned char c2,
                           struct s_write *wb)
{
    // Handle channel change
    wb->data608->channel=wb->new_channel;
    if (wb->data608->channel!=cc_channel)
        return;

    enum command_code command = COM_UNKNOWN;
    if (c1==0x15)
        c1=0x14;
    if ((c1==0x14 || c1==0x1C) && c2==0x2C)
        command = COM_ERASEDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x20)
        command = COM_RESUMECAPTIONLOADING;
    if ((c1==0x14 || c1==0x1C) && c2==0x2F)
        command = COM_ENDOFCAPTION;
    if ((c1==0x17 || c1==0x1F) && c2==0x21)
        command = COM_TABOFFSET1;
    if ((c1==0x17 || c1==0x1F) && c2==0x22)
        command = COM_TABOFFSET2;
    if ((c1==0x17 || c1==0x1F) && c2==0x23)
        command = COM_TABOFFSET3;
    if ((c1==0x14 || c1==0x1C) && c2==0x25)
        command = COM_ROLLUP2;
    if ((c1==0x14 || c1==0x1C) && c2==0x26)
        command = COM_ROLLUP3;
    if ((c1==0x14 || c1==0x1C) && c2==0x27)
        command = COM_ROLLUP4;
    if ((c1==0x14 || c1==0x1C) && c2==0x2D)
        command = COM_CARRIAGERETURN;
    if ((c1==0x14 || c1==0x1C) && c2==0x2E)
        command = COM_ERASENONDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x21)
        command = COM_BACKSPACE;
    if ((c1==0x14 || c1==0x1C) && c2==0x2b)
        command = COM_RESUMETEXTDISPLAY;
    if (debug_608)
    {
        hb_log ("\rCommand: %02X %02X (%s)\n",c1,c2,command_type[command]);
    }
    switch (command)
    {
        case COM_BACKSPACE:
            if (wb->data608->cursor_column>0)
            {
                struct eia608_screen *data;
                data = get_writing_buffer(wb);
                wb->data608->cursor_column--;
                data->characters[wb->data608->cursor_row][wb->data608->cursor_column] = ' ';
                data->dirty = 1;
            }
            break;
        case COM_TABOFFSET1:
            if (wb->data608->cursor_column<31)
                wb->data608->cursor_column++;
            break;
        case COM_TABOFFSET2:
            wb->data608->cursor_column+=2;
            if (wb->data608->cursor_column>31)
                wb->data608->cursor_column=31;
            break;
        case COM_TABOFFSET3:
            wb->data608->cursor_column+=3;
            if (wb->data608->cursor_column>31)
                wb->data608->cursor_column=31;
            break;
        case COM_RESUMECAPTIONLOADING:
            wb->data608->mode=MODE_POPUP;
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            break;
        case COM_RESUMETEXTDISPLAY:
            wb->data608->mode=MODE_TEXT;
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            break;
        case COM_ROLLUP2:
            if (wb->data608->rollup_base_row + 1 < 2)
            {
                move_roll_up(wb, 1);
                wb->data608->rollup_base_row = 1;
            }
            if (wb->data608->mode==MODE_POPUP)
            {
                swap_visible_buffer(wb);
                if (write_cc_buffer(wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);
            }
            wb->data608->color=default_color;
            wb->data608->font=FONT_REGULAR;
            if (wb->data608->mode==MODE_ROLLUP_2 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU2, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
                wb->rollup_cr = 1;
            }
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            wb->data608->mode=MODE_ROLLUP_2;
            erase_memory (wb, 0);
            wb->data608->cursor_column = 0;
            wb->data608->cursor_row = wb->data608->rollup_base_row;
            break;
        case COM_ROLLUP3:
            if (wb->data608->rollup_base_row + 1 < 3)
            {
                move_roll_up(wb, 2);
                wb->data608->rollup_base_row = 2;
            }
            if (wb->data608->mode==MODE_POPUP)
            {
                if (write_cc_buffer(wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);
            }
            wb->data608->color=default_color;
            wb->data608->font=FONT_REGULAR;
            if (wb->data608->mode==MODE_ROLLUP_3 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU3, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
                wb->rollup_cr = 1;
            }
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            wb->data608->mode=MODE_ROLLUP_3;
            erase_memory (wb, 0);
            wb->data608->cursor_column = 0;
            wb->data608->cursor_row = wb->data608->rollup_base_row;
            break;
        case COM_ROLLUP4:
            if (wb->data608->rollup_base_row + 1 < 4)
            {
                move_roll_up(wb, 3);
                wb->data608->rollup_base_row = 3;
            }
            if (wb->data608->mode==MODE_POPUP)
            {
                if (write_cc_buffer(wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);
            }
            wb->data608->color=default_color;
            wb->data608->font=FONT_REGULAR;
            if (wb->data608->mode==MODE_ROLLUP_4 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU4, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
                wb->rollup_cr = 1;
            }
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            wb->data608->mode = MODE_ROLLUP_4;
            wb->data608->cursor_column = 0;
            wb->data608->cursor_row = wb->data608->rollup_base_row;
            erase_memory (wb, 0);
            break;
        case COM_CARRIAGERETURN:
            // In transcript mode, CR doesn't write the whole screen, to avoid
            // repeated lines.

            // Skip initial CR if rollup has already done it
            if (wb->rollup_cr && is_current_row_empty(wb))
            {
                wb->rollup_cr = 0;
                wb->data608->current_visible_start_ms = get_last_pts(wb);
                break;
            }
            if (write_cc_buffer(wb))
                wb->data608->screenfuls_counter++;
            roll_up(wb);
            wb->data608->cursor_column = 0;
            wb->data608->current_visible_start_ms = get_last_pts(wb);
            break;
        case COM_ERASENONDISPLAYEDMEMORY:
            erase_memory (wb,0);
            break;
        case COM_ERASEDISPLAYEDMEMORY:
            // There may be "displayed" rollup data that has not been
            // written to a buffer yet.
            if (wb->data608->mode == MODE_ROLLUP_2 ||
                wb->data608->mode == MODE_ROLLUP_3 ||
                wb->data608->mode == MODE_ROLLUP_4)
            {
                write_cc_buffer(wb);
            }
            erase_memory (wb,1);

            // the last pts is the time to remove the previously 
            // displayed CC from the display
            wb->data608->current_visible_start_ms = get_last_pts(wb);

            // Write "clear" subtitle if necessary
            struct eia608_screen *data;
            data = get_current_visible_buffer(wb);
            data->dirty = 1;
            write_cc_buffer(wb);
            break;
        case COM_ENDOFCAPTION: // Switch buffers
            // The currently *visible* buffer is leaving, so now we know it's ending
            // time. Time to actually write it to file.
            if (wb->data608->mode == MODE_POPUP)
            {
                swap_visible_buffer(wb);
                wb->data608->current_visible_start_ms = get_last_pts(wb);
            }
            if (write_cc_buffer(wb))
                wb->data608->screenfuls_counter++;

            if (wb->data608->mode != MODE_POPUP)
                swap_visible_buffer(wb);
            wb->data608->cursor_column = 0;
            wb->data608->cursor_row = 0;
            wb->data608->color=default_color;
            wb->data608->font=FONT_REGULAR;
            break;
        default:
            if (debug_608)
            {
                hb_log ("\rNot yet implemented.\n");
            }
            break;
    }
}

static void handle_end_of_data(struct s_write *wb)
{
    // We issue a EraseDisplayedMemory here so if there's any captions pending
    // they get written to file.
    handle_command (0x14, 0x2c, wb); // EDM
}

static void handle_double(const unsigned char c1, const unsigned char c2,
                          struct s_write *wb)
{
    unsigned char c;
    if (wb->data608->channel!=cc_channel)
        return;
    if (c2>=0x30 && c2<=0x3f)
    {
        c=c2 + 0x50; // So if c>=0x80 && c<=0x8f, it comes from here
        if (debug_608)
            hb_log ("\rDouble: %02X %02X  -->  %c\n",c1,c2,c);
        write_char(c,wb);
    }
}

/* Process EXTENDED CHARACTERS */
static unsigned char handle_extended(unsigned char hi, unsigned char lo,
                                     struct s_write *wb)
{
    // Handle channel change
    if (wb->new_channel > 2)
    {
        wb->new_channel -= 2;
        if (debug_608)
            hb_log ("\nChannel correction, now %d\n", wb->new_channel);
    }
    wb->data608->channel=wb->new_channel;
    if (wb->data608->channel!=cc_channel)
        return 0;

    // For lo values between 0x20-0x3f
    unsigned char c=0;

    if (debug_608)
        hb_log ("\rExtended: %02X %02X\n",hi,lo);
    if (lo>=0x20 && lo<=0x3f && (hi==0x12 || hi==0x13))
    {
        switch (hi)
        {
            case 0x12:
                c=lo+0x70; // So if c>=0x90 && c<=0xaf it comes from here
                break;
            case 0x13:
                c=lo+0x90; // So if c>=0xb0 && c<=0xcf it comes from here
                break;
        }
        // This column change is because extended characters replace
        // the previous character (which is sent for basic decoders
        // to show something similar to the real char)
        if (wb->data608->cursor_column>0)
            wb->data608->cursor_column--;

        write_char (c,wb);
    }
    return 1;
}

/* Process PREAMBLE ACCESS CODES (PAC) */
static void handle_pac(unsigned char c1, unsigned char c2, struct s_write *wb)
{
    // Handle channel change
    if (wb->new_channel > 2)
    {
        wb->new_channel -= 2;
        if (debug_608)
            hb_log ("\nChannel correction, now %d\n", wb->new_channel);
    }
    wb->data608->channel=wb->new_channel;
    if (wb->data608->channel!=cc_channel)
        return;

    int row=rowdata[((c1<<1)&14)|((c2>>5)&1)];

    if (debug_608)
        hb_log ("\rPAC: %02X %02X",c1,c2);

    if (c2>=0x40 && c2<=0x5f)
    {
        c2=c2-0x40;
    }
    else
    {
        if (c2>=0x60 && c2<=0x7f)
        {
            c2=c2-0x60;
        }
        else
        {
            if (debug_608)
                hb_log ("\rThis is not a PAC!!!!!\n");
            return;
        }
    }
    wb->data608->color=pac2_attribs[c2][0];
    wb->data608->font=pac2_attribs[c2][1];
    int indent=pac2_attribs[c2][2];
    if (debug_608)
        hb_log ("  --  Position: %d:%d, color: %s,  font: %s\n", row, indent,
                color_text[wb->data608->color][0],
                font_text[wb->data608->font]);

    // CC spec says to the preferred method to handle a roll-up base row
    // that causes the display to scroll off the top of the screen is to 
    // adjust the base row down.
    int keep_lines;
    switch (wb->data608->mode)
    {
        case MODE_ROLLUP_2:
            keep_lines = 2;
            break;
        case MODE_ROLLUP_3:
            keep_lines = 3;
            break;
        case MODE_ROLLUP_4:
            keep_lines = 4;
            break;
        default:
            // Not rollup mode, all rows ok
            keep_lines = 0;
            break;
    }
    if (row < keep_lines)
    {
        row = keep_lines;
    }
    if (wb->data608->mode != MODE_TEXT)
    {
        // According to Robson, row info is discarded in text mode
        // but column is accepted
        //
        // CC-608 spec says current rollup display must move to the
        // new position when the cursor row changes
        move_roll_up(wb, row - 1);
        wb->data608->cursor_row = row - 1 ; // Since the array is 0 based
    }
    wb->data608->rollup_base_row = row - 1;
    wb->data608->cursor_column = indent;
}


static void handle_single(const unsigned char c1, struct s_write *wb)
{
    if (c1<0x20 || wb->data608->channel!=cc_channel)
        return; // We don't allow special stuff here
    if (debug_608)
        hb_log ("%c",c1);

    /*if (debug_608)
    hb_log ("Character: %02X (%c) -> %02X (%c)\n",c1,c1,c,c); */
    write_char (c1,wb);
}

static int check_channel(unsigned char c1, struct s_write *wb)
{
    if (c1==0x14)
    {
        if (debug_608 && wb->data608->channel!=1)
            hb_log ("\nChannel change, now 1\n");
        return 1;
    }
    if (c1==0x1c)
    {
        if (debug_608 && wb->data608->channel!=2)
            hb_log ("\nChannel change, now 2\n");
        return 2;
    }
    if (c1==0x15)
    {
        if (debug_608 && wb->data608->channel!=3)
            hb_log ("\nChannel change, now 3\n");
        return 3;
    }
    if (c1==0x1d)
    {
        if (debug_608 && wb->data608->channel!=4)
            hb_log ("\nChannel change, now 4\n");
        return 4;
    }

    // Otherwise keep the current channel
    return wb->data608->channel;
}

/* Handle Command, special char or attribute and also check for
* channel changes.
* Returns 1 if something was written to screen, 0 otherwise */
static int disCommand(unsigned char hi, unsigned char lo, struct s_write *wb)
{
    int wrote_to_screen=0;

    /* Full channel changes are only allowed for "GLOBAL CODES",
    * "OTHER POSITIONING CODES", "BACKGROUND COLOR CODES",
    * "MID-ROW CODES".
    * "PREAMBLE ACCESS CODES", "BACKGROUND COLOR CODES" and
    * SPECIAL/SPECIAL CHARACTERS allow only switching
    * between 1&3 or 2&4. */
    wb->new_channel = check_channel (hi,wb);
    //if (wb->data608->channel!=cc_channel)
    //    continue;

    if (hi>=0x18 && hi<=0x1f)
        hi=hi-8;

    switch (hi)
    {
        case 0x10:
            if (lo>=0x40 && lo<=0x5f)
                handle_pac (hi,lo,wb);
            break;
        case 0x11:
            if (lo>=0x20 && lo<=0x2f)
                handle_text_attr (hi,lo,wb);
            if (lo>=0x30 && lo<=0x3f)
            {
                wrote_to_screen=1;
                handle_double (hi,lo,wb);
            }
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x12:
        case 0x13:
            if (lo>=0x20 && lo<=0x3f)
            {
                wrote_to_screen=handle_extended (hi,lo,wb);
            }
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x14:
        case 0x15:
            if (lo>=0x20 && lo<=0x2f)
                handle_command (hi,lo,wb);
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x16:
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x17:
            if (lo>=0x21 && lo<=0x23)
                handle_command (hi,lo,wb);
            if (lo>=0x2e && lo<=0x2f)
                handle_text_attr (hi,lo,wb);
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
    }
    return wrote_to_screen;
}

static void process608(const unsigned char *data, int length,
                       struct s_write *wb)
{
    static int textprinted = 0;
    int i;

    if (data!=NULL)
    {
        for (i=0;i<length;i=i+2)
        {
            unsigned char hi, lo;
            hi = data[i] & 0x7F; // Get rid of parity bit
            lo = data[i+1] & 0x7F; // Get rid of parity bit

            if (hi==0 && lo==0) // Just padding
                continue;
            if (hi>=0x01 && hi<=0x0E)
            {
                // XDS crap - mode. Would be nice to support it eventually
                // wb->data608->last_c1=0;
                // wb->data608->last_c2=0;
                wb->data608->channel=3; // Always channel 3
                wb->in_xds_mode=1;
            }
            if (hi==0x0F) // End of XDS block
            {
                wb->in_xds_mode=0;
                continue;
            }
            if (hi>=0x10 && hi<0x1F) // Non-character code or special/extended char
                // http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/CC_CODES.HTML
                // http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/CC_CHARS.HTML
            {
                // We were writing characters before, start a new line for
                // diagnostic output from disCommand()
                if (debug_608 && textprinted == 1 )
                {
                    hb_log("\n");
                    textprinted = 0;
                }

                wb->in_xds_mode=0; // Back to normal
                if (wb->data608->last_c1==hi && wb->data608->last_c2==lo)
                {
                    // Duplicate dual code, discard
                    continue;
                }
                wb->data608->last_c1=hi;
                wb->data608->last_c2=lo;
                disCommand (hi,lo,wb);
            }
            if (hi>=0x20) // Standard characters (always in pairs)
            {
                // Only print if the channel is active
                if (wb->data608->channel!=cc_channel)
                    continue;

                if (debug_608)
                {
                    if( textprinted == 0 )
                    {
                        hb_log("\n");
                        textprinted = 1;
                    }
                }

                handle_single(hi,wb);
                handle_single(lo,wb);
                wb->data608->last_c1=0;
                wb->data608->last_c2=0;
            }

            if ( debug_608 && !textprinted && wb->data608->channel==cc_channel )
            {   // Current FTS information after the characters are shown
                //hb_log("Current FTS: %s\n", print_mstime(get_last_pts()));
            }

            if ((wb->data608->mode == MODE_ROLLUP_2 ||
                 wb->data608->mode == MODE_ROLLUP_3 ||
                 wb->data608->mode == MODE_ROLLUP_4) &&
                wb->direct_rollup)
            {
                // If we are showing rollup on the fly (direct_rollup)
                // write a buffer now
                write_cc_buffer(wb);
                wb->data608->current_visible_start_ms = get_last_pts(wb);
            }
        }
    }
}

struct hb_work_private_s
{
    hb_job_t           * job;
    struct s_write     * cc608;
};

static int decccInit( hb_work_object_t * w, hb_job_t * job )
{
    int retval = 1;
    hb_work_private_t * pv;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if( pv )
    {
        w->private_data = pv;

        pv->job = job;

        pv->cc608 = calloc(1, sizeof(struct s_write));

        if( pv->cc608 )
        {
            pv->cc608->width = job->title->geometry.width;
            pv->cc608->height = job->title->geometry.height;
            memcpy(pv->cc608->crop, job->crop, sizeof(int[4]));
            pv->cc608->par = job->title->geometry.par;
            retval = general_608_init(pv->cc608);
            if( !retval )
            {
                pv->cc608->data608 = calloc(1, sizeof(struct eia608));
                if( !pv->cc608->data608 )
                {
                    retval = 1;
                }
                init_eia608(pv->cc608->data608);
            }
        }
    }
    if (!retval)
    {
        // Generate generic SSA Script Info.
        int height = job->title->geometry.height - job->crop[0] - job->crop[1];
        int width = job->title->geometry.width - job->crop[2] - job->crop[3];
        int safe_height = 0.8 * job->title->geometry.height;
        hb_subtitle_add_ssa_header(w->subtitle, "Courier New",
                                   .08 * safe_height, width, height);
    }
    // When rendering subs, we need to push rollup subtitles out
    // asap (instead of waiting for a completed line) so that we
    // do not miss the frame that they should be rendered over.
    pv->cc608->direct_rollup = w->subtitle->config.dest == RENDERSUB;
    return retval;
}

static int decccWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
               hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        handle_end_of_data(pv->cc608);
        /*
         * Grab any pending buffer and output them with the EOF on the end
         */
        *buf_in = NULL;
        hb_buffer_list_append(&pv->cc608->list, in);
        *buf_out = hb_buffer_list_clear(&pv->cc608->list);
        return HB_WORK_DONE;
    }

    pv->cc608->last_pts = in->s.start;
    process608(in->data, in->size, pv->cc608);

    /*
     * If there is one waiting then pass it on
     */
    *buf_out = hb_buffer_list_clear(&pv->cc608->list);
    return HB_WORK_OK;
}

static void decccClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    general_608_close( pv->cc608 );
    free( pv->cc608->data608 );
    free( pv->cc608 );
    free( w->private_data );
}

hb_work_object_t hb_deccc608 =
{
    WORK_DECCC608,
    "Closed Caption (608) decoder",
    decccInit,
    decccWork,
    decccClose
};
