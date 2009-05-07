/*
 * From ccextractor, leave this file as intact and close to the original as possible so that 
 * it is easy to patch in fixes - even though this file contains code that we don't need.
 *
 * Note that the SRT sub generation from CC could be useful for mkv subs.
 */
#ifndef __deccc608sub_H__
#define __deccc608sub_H__

#include "common.h"

struct s_write;

void handle_end_of_data (struct s_write *wb);
void process608 (const unsigned char *data, int length, struct s_write *wb);
void get_char_in_latin_1 (unsigned char *buffer, unsigned char c);
void get_char_in_unicode (unsigned char *buffer, unsigned char c);
int get_char_in_utf_8 (unsigned char *buffer, unsigned char c);
unsigned char cctolower (unsigned char c);
unsigned char cctoupper (unsigned char c);
int general_608_init (struct s_write *wb);
void general_608_close (struct s_write *wb);

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


struct eia608_screen // A CC buffer
{
    unsigned char characters[15][33]; 
    unsigned char colors[15][33];
    unsigned char fonts[15][33]; // Extra char at the end for a 0
    int row_used[15]; // Any data in row?
    int empty; // Buffer completely empty?    	
};

#define LLONG long long

struct eia608
{
    struct eia608_screen buffer1;
    struct eia608_screen buffer2;  
    int cursor_row, cursor_column;
    int visible_buffer;
    int srt_counter; // Number of subs currently written
    int screenfuls_counter; // Number of meaningful screenfuls written
    LLONG current_visible_start_ms; // At what time did the current visible buffer became so?
    // unsigned current_visible_start_cc; // At what time did the current visible buffer became so?
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
    hb_buffer_t *hb_buffer;
    hb_buffer_t *hb_last_buffer;
    uint64_t last_pts;
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
    OF_RAW	= 0,
    OF_SRT	= 1,
    OF_SAMI = 2,
    OF_TRANSCRIPT = 3,
    OF_RCWT = 4
};

#endif
