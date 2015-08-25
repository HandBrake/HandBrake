/* deccc608sub.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * From ccextractor...
 */
#ifndef __DECCC608SUB_H__
#define __DECCC608SUB_H__

#include "common.h"

struct s_write;

#define CC608_SCREEN_WIDTH  32

enum cc_modes
{
    MODE_POPUP = 0,
    MODE_ROLLUP_2 = 1,
    MODE_ROLLUP_3 = 2,
    MODE_ROLLUP_4 = 3,
    MODE_TEXT = 4
};

enum color_code
{
    COL_WHITE = 0,
    COL_GREEN = 1,
    COL_BLUE = 2,
    COL_CYAN = 3,
    COL_RED = 4,
    COL_YELLOW = 5,
    COL_MAGENTA = 6,
    COL_USERDEFINED = 7
};


enum font_bits
{
    FONT_REGULAR = 0,
    FONT_ITALICS = 1,
    FONT_UNDERLINED = 2,
    FONT_UNDERLINED_ITALICS = 3
};

#define FONT_STYLE_MASK FONT_UNDERLINED_ITALICS

struct eia608_screen // A CC buffer
{
    unsigned char characters[15][33];
    unsigned char colors[15][33];
    unsigned char fonts[15][33]; // Extra char at the end for a 0
    int row_used[15]; // Any data in row?
    int empty; // Buffer completely empty?
    int dirty; // Flag indicates buffer has changed since written
};

struct eia608
{
    struct eia608_screen buffer1;
    struct eia608_screen buffer2;
    int cursor_row, cursor_column;
    int visible_buffer;
    int ssa_counter; // Number of subs currently written
    int screenfuls_counter; // Number of meaningful screenfuls written
    int64_t current_visible_start_ms; // At what time did the current visible buffer became so?
    enum cc_modes mode;
    unsigned char last_c1, last_c2;
    int channel; // Currently selected channel
    unsigned char color; // Color we are currently using to write
    unsigned char font; // Font we are currently using to write
    int rollup_base_row;
};

struct s_write {
    struct eia608 *data608;
    FILE *fh;
    unsigned char *subline;
    int new_sentence;
    int new_channel;
    int in_xds_mode;
    hb_buffer_list_t list;
    hb_buffer_t *hb_buffer;
    hb_buffer_t *hb_last_buffer;
    uint64_t last_pts;
    unsigned char *enc_buffer; // Generic general purpose buffer
    unsigned enc_buffer_used;
    unsigned enc_buffer_capacity;

    int clear_sub_needed;   // Indicates that we need to send a null
                            // subtitle to clear the current subtitle

    int rollup_cr;  // Flag indicates if CR command performed by rollup
    int direct_rollup;
    int line;   // SSA line number
    int width;
    int height;
    int crop[4];
    hb_rational_t par;
    uint8_t prev_font_style;
    uint8_t prev_font_color;
};

enum command_code
{
    COM_UNKNOWN = 0,
    COM_ERASEDISPLAYEDMEMORY = 1,
    COM_RESUMECAPTIONLOADING = 2,
    COM_ENDOFCAPTION = 3,
    COM_TABOFFSET1 = 4,
    COM_TABOFFSET2 = 5,
    COM_TABOFFSET3 = 6,
    COM_ROLLUP2 = 7,
    COM_ROLLUP3 = 8,
    COM_ROLLUP4 = 9,
    COM_CARRIAGERETURN = 10,
    COM_ERASENONDISPLAYEDMEMORY = 11,
    COM_BACKSPACE = 12,
    COM_RESUMETEXTDISPLAY = 13
};

enum encoding_type
{
    ENC_UNICODE = 0,
    ENC_LATIN_1 = 1,
    ENC_UTF_8 = 2
};

enum output_format
{
    OF_RAW  = 0,
    OF_SRT  = 1,
    OF_SAMI = 2,
    OF_TRANSCRIPT = 3,
    OF_RCWT = 4
};

#endif // __DECCC608SUB_H__
