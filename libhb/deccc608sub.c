/*
 * From ccextractor, leave this file as intact and close to the original as possible so that 
 * it is easy to patch in fixes - even though this file contains code that we don't need.
 *
 * Note that the SRT sub generation from CC could be useful for mkv subs.
 */
#include "hb.h"
#include "deccc608sub.h"

/*
 * ccextractor static configuration variables.
 */
static int debug_608 = 0;
static int trim_subs = 0;
static int nofontcolor = 0;
static enum encoding_type encoding = ENC_UTF_8;
static int cc_channel = 1;
static enum output_format write_format = OF_TRANSCRIPT;
static int sentence_cap = 1;
static int subs_delay = 0;
static LLONG screens_to_process = -1;
static int processed_enough = 0;
static int gui_mode_reports = 0;
static int norollup = 1;
static int direct_rollup = 0;

static LLONG get_fts(void)
{
    return 0;
}

#define fatal(N, ...) // N
#define XMLRPC_APPEND(N, ...) // N

int     rowdata[] = {11,-1,1,2,3,4,12,13,14,15,5,6,7,8,9,10};
// Relationship between the first PAC byte and the row number

// The following enc_buffer is not used at the moment, if it does get used
// we need to bring it into the swrite struct. Same for "str".
#define INITIAL_ENC_BUFFER_CAPACITY		2048

unsigned char *enc_buffer=NULL; // Generic general purpose buffer
unsigned char str[2048]; // Another generic general purpose buffer
unsigned enc_buffer_used;
unsigned enc_buffer_capacity;

#define GUARANTEE(length) if (length>enc_buffer_capacity) \
{enc_buffer_capacity*=2; enc_buffer=(unsigned char*) realloc (enc_buffer, enc_buffer_capacity); \
    if (enc_buffer==NULL) { fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory, bailing out\n"); } \
}

const unsigned char pac2_attribs[][3]= // Color, font, ident
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

// Preencoded strings
unsigned char encoded_crlf[16]; 
unsigned int encoded_crlf_length;
unsigned char encoded_br[16];
unsigned int encoded_br_length;

// Default color
unsigned char usercolor_rgb[8]="";
enum color_code default_color=COL_WHITE;

const char *sami_header= // TODO: Revise the <!-- comments
"<SAMI>\n\
<HEAD>\n\
<STYLE TYPE=\"text/css\">\n\
<!--\n\
P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n\
text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n\
.UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n\
-->\n\
</STYLE>\n\
</HEAD>\n\n\
<BODY>\n";

const char *command_type[] =
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

const char *font_text[]=
{
    "regular",
    "italics",
    "underlined",
    "underlined italics"
};

const char *cc_modes_text[]=
{
    "Pop-Up captions"
};

const char *color_text[][2]=
{
    {"white",""},
    {"green","<font color=\"#00ff00\">"},
    {"blue","<font color=\"#0000ff\">"},
    {"cyan","<font color=\"#00ffff\">"},
    {"red","<font color=\"#ff0000\">"},
    {"yellow","<font color=\"#ffff00\">"},
    {"magenta","<font color=\"#ff00ff\">"},
    {"userdefined","<font color=\""}
};

int general_608_init (struct s_write *wb)
{
    /*
     * Not currently used.
     *
    if( !enc_buffer )
    {
        enc_buffer=(unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY); 
        if (enc_buffer==NULL)
            return -1;
        enc_buffer_capacity=INITIAL_ENC_BUFFER_CAPACITY;
    }
    */

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
    return 0;
}

/*
 * Free up CC memory - don't call this from HB just yet since it will cause
 * parallel encodes to fail - to be honest they will be stuffed anyway since
 * the CC's may be overwriting the buffers.
 */
void general_608_close (struct s_write *wb)
{
    if( enc_buffer ) {
        free(enc_buffer);
        enc_buffer_capacity = 0;
        enc_buffer_used = 0;
    }
    if( wb->subline ) {
        free(wb->subline);
    }
}


#include <ctype.h>

void get_char_in_latin_1 (unsigned char *buffer, unsigned char c)
{
    unsigned char c1='?';
    if (c<0x80) 
    {	
        // Regular line-21 character set, mostly ASCII except these exceptions
        switch (c)
        {
            case 0x2a: // lowercase a, acute accent
                c1=0xe1;
                break;
            case 0x5c: // lowercase e, acute accent
                c1=0xe9;
                break;
            case 0x5e: // lowercase i, acute accent
                c1=0xed;
                break;			
            case 0x5f: // lowercase o, acute accent
                c1=0xf3;
                break;
            case 0x60: // lowercase u, acute accent
                c1=0xfa;
                break;
            case 0x7b: // lowercase c with cedilla
                c1=0xe7;
                break;
            case 0x7c: // division symbol
                c1=0xf7;
                break;
            case 0x7d: // uppercase N tilde
                c1=0xd1;
                break;
            case 0x7e: // lowercase n tilde
                c1=0xf1;
                break;
            default:
                c1=c;
                break;
        }
        *buffer=c1;
        return;
    }
    switch (c)
    {
        // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x11 AND LOW BETWEEN 0x30 AND 0x3F		
        case 0x80: // Registered symbol (R)
            c1=0xae;
            break;			
        case 0x81: // degree sign
            c1=0xb0;
            break;
        case 0x82: // 1/2 symbol			
            c1=0xbd;
            break;
        case 0x83: // Inverted (open) question mark			
            c1=0xbf;
            break;
        case 0x84: // Trademark symbol (TM) - Does not exist in Latin 1
            break;			
        case 0x85: // Cents symbol			
            c1=0xa2;
            break;
        case 0x86: // Pounds sterling			
            c1=0xa3;
            break;
        case 0x87: // Music note - Not in latin 1, so we use 'pilcrow'
            c1=0xb6;
            break;
        case 0x88: // lowercase a, grave accent
            c1=0xe0;
            break;
        case 0x89: // transparent space, we make it regular
            c1=0x20;			
            break;
        case 0x8a: // lowercase e, grave accent
            c1=0xe8;
            break;
        case 0x8b: // lowercase a, circumflex accent
            c1=0xe2;
            break;
        case 0x8c: // lowercase e, circumflex accent
            c1=0xea;			
            break;
        case 0x8d: // lowercase i, circumflex accent
            c1=0xee;
            break;
        case 0x8e: // lowercase o, circumflex accent
            c1=0xf4;
            break;
        case 0x8f: // lowercase u, circumflex accent
            c1=0xfb;
            break;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x12 AND LOW BETWEEN 0x20 AND 0x3F
        case 0x90: // capital letter A with acute
            c1=0xc1;
            break;
        case 0x91: // capital letter E with acute
            c1=0xc9;
            break;
        case 0x92: // capital letter O with acute
            c1=0xd3;
            break;
        case 0x93: // capital letter U with acute
            c1=0xda;
            break;
        case 0x94: // capital letter U with diaresis
            c1=0xdc;
            break;
        case 0x95: // lowercase letter U with diaeresis
            c1=0xfc;
            break;
        case 0x96: // apostrophe
            c1=0x27;			
            break;
        case 0x97: // inverted exclamation mark			
            c1=0xa1;
            break;
        case 0x98: // asterisk
            c1=0x2a;			
            break;
        case 0x99: // apostrophe (yes, duped). See CCADI source code.
            c1=0x27;			
            break;
        case 0x9a: // hyphen-minus
            c1=0x2d;			
            break;
        case 0x9b: // copyright sign
            c1=0xa9;
            break;
        case 0x9c: // Service Mark - not available in latin 1
            break;
        case 0x9d: // Full stop (.)
            c1=0x2e;
            break;
        case 0x9e: // Quoatation mark
            c1=0x22;			
            break;
        case 0x9f: // Quoatation mark
            c1=0x22;			
            break;
        case 0xa0: // uppercase A, grave accent
            c1=0xc0;
            break;
        case 0xa1: // uppercase A, circumflex
            c1=0xc2;
            break;			
        case 0xa2: // uppercase C with cedilla
            c1=0xc7;
            break;
        case 0xa3: // uppercase E, grave accent
            c1=0xc8;
            break;
        case 0xa4: // uppercase E, circumflex
            c1=0xca;
            break;
        case 0xa5: // capital letter E with diaresis
            c1=0xcb;
            break;
        case 0xa6: // lowercase letter e with diaresis
            c1=0xeb;
            break;
        case 0xa7: // uppercase I, circumflex
            c1=0xce;
            break;
        case 0xa8: // uppercase I, with diaresis
            c1=0xcf;
            break;
        case 0xa9: // lowercase i, with diaresis
            c1=0xef;
            break;
        case 0xaa: // uppercase O, circumflex
            c1=0xd4;
            break;
        case 0xab: // uppercase U, grave accent
            c1=0xd9;
            break;
        case 0xac: // lowercase u, grave accent
            c1=0xf9;
            break;
        case 0xad: // uppercase U, circumflex
            c1=0xdb;
            break;
        case 0xae: // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            c1=0xab;
            break;
        case 0xaf: // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            c1=0xbb;
            break;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x13 AND LOW BETWEEN 0x20 AND 0x3F
        case 0xb0: // Uppercase A, tilde
            c1=0xc3;
            break;
        case 0xb1: // Lowercase a, tilde
            c1=0xe3;
            break;
        case 0xb2: // Uppercase I, acute accent
            c1=0xcd;
            break;
        case 0xb3: // Uppercase I, grave accent
            c1=0xcc;
            break;
        case 0xb4: // Lowercase i, grave accent
            c1=0xec;
            break;
        case 0xb5: // Uppercase O, grave accent
            c1=0xd2;
            break;
        case 0xb6: // Lowercase o, grave accent
            c1=0xf2;
            break;
        case 0xb7: // Uppercase O, tilde
            c1=0xd5;
            break;
        case 0xb8: // Lowercase o, tilde
            c1=0xf5;
            break;
        case 0xb9: // Open curly brace
            c1=0x7b;
            break;
        case 0xba: // Closing curly brace
            c1=0x7d;            
            break;
        case 0xbb: // Backslash
            c1=0x5c;
            break;
        case 0xbc: // Caret
            c1=0x5e;
            break;
        case 0xbd: // Underscore
            c1=0x5f;
            break;
        case 0xbe: // Pipe (broken bar)            
            c1=0xa6;
            break;
        case 0xbf: // Tilde
            c1=0x7e; 
            break;
        case 0xc0: // Uppercase A, umlaut            
            c1=0xc4;
            break;
        case 0xc1: // Lowercase A, umlaut
            c1=0xe3; 
            break;
        case 0xc2: // Uppercase O, umlaut
            c1=0xd6;
            break;
        case 0xc3: // Lowercase o, umlaut
            c1=0xf6;
            break;
        case 0xc4: // Esszett (sharp S)
            c1=0xdf;
            break;
        case 0xc5: // Yen symbol
            c1=0xa5;
            break;
        case 0xc6: // Currency symbol
            c1=0xa4;
            break;            
        case 0xc7: // Vertical bar
            c1=0x7c;
            break;            
        case 0xc8: // Uppercase A, ring
            c1=0xc5;
            break;
        case 0xc9: // Lowercase A, ring
            c1=0xe5;
            break;
        case 0xca: // Uppercase O, slash
            c1=0xd8;
            break;
        case 0xcb: // Lowercase o, slash
            c1=0xf8;
            break;
        case 0xcc: // Upper left corner
        case 0xcd: // Upper right corner
        case 0xce: // Lower left corner
        case 0xcf: // Lower right corner
        default: // For those that don't have representation
            *buffer='?'; // I'll do it eventually, I promise
            break; // This are weird chars anyway
    }
    *buffer=c1;	
}

void get_char_in_unicode (unsigned char *buffer, unsigned char c)
{
    unsigned char c1,c2;
    switch (c)
    {
        case 0x84: // Trademark symbol (TM) 
            c2=0x21;
            c1=0x22;
            break;
        case 0x87: // Music note
            c2=0x26;
            c1=0x6a;
            break;
        case 0x9c: // Service Mark
            c2=0x21;
            c1=0x20;
            break;
        case 0xcc: // Upper left corner
            c2=0x23;
            c1=0x1c;
            break;			
        case 0xcd: // Upper right corner
            c2=0x23;
            c1=0x1d;
            break;
        case 0xce: // Lower left corner
            c2=0x23;
            c1=0x1e;
            break;
        case 0xcf: // Lower right corner
            c2=0x23;
            c1=0x1f;
            break;
        default: // Everything else, same as latin-1 followed by 00			
            get_char_in_latin_1 (&c1,c);
            c2=0;
            break;
    }
    *buffer=c1;
    *(buffer+1)=c2;
}

int get_char_in_utf_8 (unsigned char *buffer, unsigned char c) // Returns number of bytes used
{
    if (c<0x80) // Regular line-21 character set, mostly ASCII except these exceptions
    {
        switch (c)
        {
        case 0x2a: // lowercase a, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xa1;
            return 2;
        case 0x5c: // lowercase e, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xa9;
            return 2;
        case 0x5e: // lowercase i, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xad;
            return 2;
        case 0x5f: // lowercase o, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xb3;
            return 2;
        case 0x60: // lowercase u, acute accent
            *buffer=0xc3;
            *(buffer+1)=0xba;
            return 2;
        case 0x7b: // lowercase c with cedilla
            *buffer=0xc3;
            *(buffer+1)=0xa7;
            return 2;
        case 0x7c: // division symbol
            *buffer=0xc3;
            *(buffer+1)=0xb7;
            return 2;
        case 0x7d: // uppercase N tilde
            *buffer=0xc3;
            *(buffer+1)=0x91;
            return 2;
        case 0x7e: // lowercase n tilde
            *buffer=0xc3;
            *(buffer+1)=0xb1;
            return 2;
        default:
            *buffer=c;
            return 1;
        }
    }
    switch (c)
    {
        // THIS BLOCK INCLUDES THE 16 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x11 AND LOW BETWEEN 0x30 AND 0x3F		
        case 0x80: // Registered symbol (R)
            *buffer=0xc2;
            *(buffer+1)=0xae;			
            return 2;
        case 0x81: // degree sign
            *buffer=0xc2;
            *(buffer+1)=0xb0;
            return 2;
        case 0x82: // 1/2 symbol
            *buffer=0xc2;
            *(buffer+1)=0xbd;
            return 2;
        case 0x83: // Inverted (open) question mark
            *buffer=0xc2;
            *(buffer+1)=0xbf;
            return 2;
        case 0x84: // Trademark symbol (TM)
            *buffer=0xe2;
            *(buffer+1)=0x84;
            *(buffer+2)=0xa2;
            return 3;
        case 0x85: // Cents symbol
            *buffer=0xc2;
            *(buffer+1)=0xa2;
            return 2;
        case 0x86: // Pounds sterling
            *buffer=0xc2;
            *(buffer+1)=0xa3;
            return 2;
        case 0x87: // Music note			
            *buffer=0xe2;
            *(buffer+1)=0x99;
            *(buffer+2)=0xaa;
            return 3;
        case 0x88: // lowercase a, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xa0;
            return 2;
        case 0x89: // transparent space, we make it regular
            *buffer=0x20;			
            return 1;
        case 0x8a: // lowercase e, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xa8;
            return 2;
        case 0x8b: // lowercase a, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xa2;
            return 2;
        case 0x8c: // lowercase e, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xaa;
            return 2;
        case 0x8d: // lowercase i, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xae;
            return 2;
        case 0x8e: // lowercase o, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xb4;
            return 2;
        case 0x8f: // lowercase u, circumflex accent
            *buffer=0xc3;
            *(buffer+1)=0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x12 AND LOW BETWEEN 0x20 AND 0x3F
        case 0x90: // capital letter A with acute
            *buffer=0xc3;
            *(buffer+1)=0x81;
            return 2;
        case 0x91: // capital letter E with acute
            *buffer=0xc3;
            *(buffer+1)=0x89;
            return 2;
        case 0x92: // capital letter O with acute
            *buffer=0xc3;
            *(buffer+1)=0x93;
            return 2;
        case 0x93: // capital letter U with acute
            *buffer=0xc3;
            *(buffer+1)=0x9a;
            return 2;
        case 0x94: // capital letter U with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x9c;
            return 2;
        case 0x95: // lowercase letter U with diaeresis
            *buffer=0xc3;
            *(buffer+1)=0xbc;
            return 2;
        case 0x96: // apostrophe
            *buffer=0x27;			
            return 1;
        case 0x97: // inverted exclamation mark
            *buffer=0xc1;
            *(buffer+1)=0xa1;
            return 2;
        case 0x98: // asterisk
            *buffer=0x2a;			
            return 1;
        case 0x99: // apostrophe (yes, duped). See CCADI source code.
            *buffer=0x27;			
            return 1;
        case 0x9a: // hyphen-minus
            *buffer=0x2d;			
            return 1;
        case 0x9b: // copyright sign
            *buffer=0xc2;
            *(buffer+1)=0xa9;
            return 2;
        case 0x9c: // Service mark 
            *buffer=0xe2;			
            *(buffer+1)=0x84;
            *(buffer+2)=0xa0;
            return 3;
        case 0x9d: // Full stop (.)
            *buffer=0x2e;			
            return 1;
        case 0x9e: // Quoatation mark
            *buffer=0x22;			
            return 1;
        case 0x9f: // Quoatation mark
            *buffer=0x22;			
            return 1;
        case 0xa0: // uppercase A, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x80;
            return 2;
        case 0xa1: // uppercase A, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x82;
            return 2;
        case 0xa2: // uppercase C with cedilla
            *buffer=0xc3;
            *(buffer+1)=0x87;
            return 2;
        case 0xa3: // uppercase E, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x88;
            return 2;
        case 0xa4: // uppercase E, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x8a;
            return 2;
        case 0xa5: // capital letter E with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x8b;
            return 2;
        case 0xa6: // lowercase letter e with diaresis
            *buffer=0xc3;
            *(buffer+1)=0xab;
            return 2;
        case 0xa7: // uppercase I, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x8e;
            return 2;
        case 0xa8: // uppercase I, with diaresis
            *buffer=0xc3;
            *(buffer+1)=0x8f;
            return 2;
        case 0xa9: // lowercase i, with diaresis
            *buffer=0xc3;
            *(buffer+1)=0xaf;
            return 2;
        case 0xaa: // uppercase O, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x94;
            return 2;
        case 0xab: // uppercase U, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x99;
            return 2;
        case 0xac: // lowercase u, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xb9;
            return 2;
        case 0xad: // uppercase U, circumflex
            *buffer=0xc3;
            *(buffer+1)=0x9b;
            return 2;
        case 0xae: // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer=0xc2;
            *(buffer+1)=0xab;
            return 2;
        case 0xaf: // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
            *buffer=0xc2;
            *(buffer+1)=0xbb;
            return 2;
        // THIS BLOCK INCLUDES THE 32 EXTENDED (TWO-BYTE) LINE 21 CHARACTERS
        // THAT COME FROM HI BYTE=0x13 AND LOW BETWEEN 0x20 AND 0x3F
        case 0xb0: // Uppercase A, tilde
            *buffer=0xc3;
            *(buffer+1)=0x83;
            return 2;
        case 0xb1: // Lowercase a, tilde
            *buffer=0xc3;
            *(buffer+1)=0xa3;
            return 2;
        case 0xb2: // Uppercase I, acute accent
            *buffer=0xc3;
            *(buffer+1)=0x8d;
            return 2;
        case 0xb3: // Uppercase I, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x8c;
            return 2;
        case 0xb4: // Lowercase i, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xac;
            return 2;
        case 0xb5: // Uppercase O, grave accent
            *buffer=0xc3;
            *(buffer+1)=0x92;
            return 2;
        case 0xb6: // Lowercase o, grave accent
            *buffer=0xc3;
            *(buffer+1)=0xb2;
            return 2;
        case 0xb7: // Uppercase O, tilde
            *buffer=0xc3;
            *(buffer+1)=0x95;
            return 2;
        case 0xb8: // Lowercase o, tilde
            *buffer=0xc3;
            *(buffer+1)=0xb5;
            return 2;
        case 0xb9: // Open curly brace
            *buffer=0x7b;
            return 1;
        case 0xba: // Closing curly brace
            *buffer=0x7d;            
            return 1;
        case 0xbb: // Backslash
            *buffer=0x5c;
            return 1;
        case 0xbc: // Caret
            *buffer=0x5e;
            return 1;
        case 0xbd: // Underscore
            *buffer=0x5f;
            return 1;
        case 0xbe: // Pipe (broken bar)
            *buffer=0xc2; 
            *(buffer+1)=0xa6;
            return 1;
        case 0xbf: // Tilde
            *buffer=0x7e; // Not sure
            return 1;
        case 0xc0: // Uppercase A, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0x84;
            return 2;
        case 0xc1: // Lowercase A, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0xa4;
            return 2;
        case 0xc2: // Uppercase O, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0x96;
            return 2;
        case 0xc3: // Lowercase o, umlaut
            *buffer=0xc3; 
            *(buffer+1)=0xb6;
            return 2;
        case 0xc4: // Esszett (sharp S)
            *buffer=0xc3;
            *(buffer+1)=0x9f;
            return 2;
        case 0xc5: // Yen symbol
            *buffer=0xc2;
            *(buffer+1)=0xa5;
            return 2;
        case 0xc6: // Currency symbol
            *buffer=0xc2;
            *(buffer+1)=0xa4;
            return 2;
        case 0xc7: // Vertical bar
            *buffer=0x7c; 
            return 1;
        case 0xc8: // Uppercase A, ring
            *buffer=0xc3;
            *(buffer+1)=0x85;
            return 2;
        case 0xc9: // Lowercase A, ring
            *buffer=0xc3;
            *(buffer+1)=0xa5;
            return 2;
        case 0xca: // Uppercase O, slash
            *buffer=0xc3;
            *(buffer+1)=0x98;
            return 2;
        case 0xcb: // Lowercase o, slash
            *buffer=0xc3;
            *(buffer+1)=0xb8;
            return 2;
        case 0xcc: // Upper left corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9c;
            return 3;
        case 0xcd: // Upper right corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9d;
            return 3;
        case 0xce: // Lower left corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9e;
            return 3;
        case 0xcf: // Lower right corner
            *buffer=0xe2;
            *(buffer+1)=0x8c;
            *(buffer+2)=0x9f;
            return 3;
        default: // 
            *buffer='?'; // I'll do it eventually, I promise
            return 1; // This are weird chars anyway
    }
}

unsigned char cctolower (unsigned char c)
{
    if (c>='A' && c<='Z')
        return tolower(c);
    switch (c)
    {
        case 0x7d: // uppercase N tilde
            return 0x7e;
        case 0x90: // capital letter A with acute
            return 0x2a;
        case 0x91: // capital letter E with acute
            return 0x5c; 
        case 0x92: // capital letter O with acute
            return 0x5f; 
        case 0x93: // capital letter U with acute
            return 0x60; 
        case 0xa2: // uppercase C with cedilla
            return 0x7b; 
        case 0xa0: // uppercase A, grave accent
            return 0x88; 
        case 0xa3: // uppercase E, grave accent
            return 0x8a; 
        case 0xa1: // uppercase A, circumflex
            return 0x8b; 
        case 0xa4: // uppercase E, circumflex
            return 0x8c; 
        case 0xa7: // uppercase I, circumflex
            return 0x8d; 
        case 0xaa: // uppercase O, circumflex
            return 0x8e; 
        case 0xad: // uppercase U, circumflex
            return 0x8f; 
        case 0x94: // capital letter U with diaresis
            return 0x95; 
        case 0xa5: // capital letter E with diaresis
            return 0xa6; 
        case 0xa8: // uppercase I, with diaresis
            return 0xa9; 
        case 0xab: // uppercase U, grave accent
            return 0xac; 
        case 0xb0: // Uppercase A, tilde
            return 0xb1;
        case 0xb2: // Uppercase I, acute accent
            return 0x5e;
        case 0xb3: // Uppercase I, grave accent
            return 0xb4;
        case 0xb5: // Uppercase O, grave accent
            return 0xb6;
        case 0xb7: // Uppercase O, tilde
            return 0xb8;
        case 0xc0: // Uppercase A, umlaut
            return 0xc1;
        case 0xc2: // Uppercase O, umlaut
            return 0xc3;
        case 0xc8: // Uppercase A, ring
            return 0xc9;
        case 0xca: // Uppercase O, slash
            return 0xcb;
    }
    return c;
}

unsigned char cctoupper (unsigned char c)
{
    if (c>='a' && c<='z')
        return toupper(c);
    switch (c)
    {
        case 0x7e: // lowercase n tilde
            return 0x7d;
        case 0x2a: // lowercase a, acute accent
            return 0x90;
        case 0x5c: // lowercase e, acute accent
            return 0x91;
        case 0x5e: // lowercase i, acute accent
            return 0xb2;
        case 0x5f: // lowercase o, acute accent
            return 0x92;
        case 0x60: // lowercase u, acute accent
            return 0x93;
        case 0x7b: // lowercase c with cedilla
            return 0xa2;
        case 0x88: // lowercase a, grave accent
            return 0xa0;
        case 0x8a: // lowercase e, grave accent
            return 0xa3;
        case 0x8b: // lowercase a, circumflex accent
            return 0xa1;
        case 0x8c: // lowercase e, circumflex accent
            return 0xa4;
        case 0x8d: // lowercase i, circumflex accent
            return 0xa7;
        case 0x8e: // lowercase o, circumflex accent
            return 0xaa;
        case 0x8f: // lowercase u, circumflex accent
            return 0xad;
        case 0x95: // lowercase letter U with diaeresis
            return 0x94;
        case 0xa6: // lowercase letter e with diaresis
            return 0xa5;
        case 0xa9: // lowercase i, with diaresis
            return 0xa8;
        case 0xac: // lowercase u, grave accent
            return 0xab;
        case 0xb1: // Lowercase a, tilde
            return 0xb0; 
        case 0xb4: // Lowercase i, grave accent
            return 0xb3;
        case 0xb6: // Lowercase o, grave accent
            return 0xb5; 
        case 0xb8: // Lowercase o, tilde	
            return 0xb7;
        case 0xc1: // Lowercase A, umlaut	
            return 0xc0; 
        case 0xc3: // Lowercase o, umlaut
            return 0xc2;
        case 0xc9: // Lowercase A, ring
            return 0xc8; 
        case 0xcb: // Lowercase o, slash
            return 0xca; 
    }
    return c;
}


// Encodes a generic string. Note that since we use the encoders for closed caption
// data, text would have to be encoded as CCs... so using special characters here
// it's a bad idea. 
unsigned encode_line (unsigned char *buffer, unsigned char *text)
{ 
    unsigned bytes=0;
    while (*text)
    {		
        switch (encoding)
        {
            case ENC_UTF_8:
            case ENC_LATIN_1:
                *buffer=*text;
                bytes++;
                buffer++;
                break;
        case ENC_UNICODE:				
            *buffer=*text;				
            *(buffer+1)=0;
            bytes+=2;				
            buffer+=2;
            break;
        }		
        text++;
    }
    return bytes;
}

#define ISSEPARATOR(c) (c==' ' || c==0x89 || ispunct(c) \
    || c==0x99) // This is the apostrofe. We get it here in CC encoding, not ASCII


void correct_case (int line_num, struct eia608_screen *data)
{
/*     int i=0; */
/*     while (i<spell_words) */
/*     { */
/*         char *c=(char *) data->characters[line_num]; */
/*         size_t len=strlen (spell_correct[i]); */
/*         while ((c=strstr (c,spell_lower[i]))!=NULL) */
/*         { */
/*             // Make sure it's a whole word (start of line or */
/*             // preceded by space, and end of line or followed by */
/*             // space) */
/*             unsigned char prev; */
/*             if (c==(char *) data->characters[line_num]) // Beginning of line... */
/*                 prev=' '; // ...Pretend we had a blank before */
/*             else */
/*                 prev=*(c-1);              */
/*             unsigned char next; */
/*             if (c-(char *) data->characters[line_num]+len==CC608_SCREEN_WIDTH) // End of line... */
/*                 next=' '; // ... pretend we have a blank later */
/*             else */
/*                 next=*(c+len);			 */
/*             if ( ISSEPARATOR(prev) && ISSEPARATOR(next)) */
/*             { */
/*                 memcpy (c,spell_correct[i],len); */
/*             } */
/*             c++; */
/*         } */
/*         i++; */
/*     } */
}

void capitalize (int line_num, struct eia608_screen *data, int *new_sentence)
{
    int i;

    for (i=0;i<CC608_SCREEN_WIDTH;i++)
    {
        switch (data->characters[line_num][i])
        {
            case ' ': 
            case 0x89: // This is a transparent space
            case '-':
                break; 
            case '.': // Fallthrough
            case '?': // Fallthrough
            case '!':
            case ':':
                *new_sentence=1;
                break;
            default:
                if (*new_sentence)			
                    data->characters[line_num][i]=cctoupper (data->characters[line_num][i]);
                else
                    data->characters[line_num][i]=cctolower (data->characters[line_num][i]);
                *new_sentence=0;
                break;
        }
    }
}

void find_limit_characters (unsigned char *line, int *first_non_blank, int *last_non_blank)
{
    int i;

    *last_non_blank=-1;
    *first_non_blank=-1;
    for (i=0;i<CC608_SCREEN_WIDTH;i++)
    {
        unsigned char c=line[i];
        if (c!=' ' && c!=0x89)
        {
            if (*first_non_blank==-1)
                *first_non_blank=i;
            *last_non_blank=i;
        }
    }
}

unsigned get_decoder_line_basic (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    unsigned char *line = data->characters[line_num];
    int last_non_blank=-1;
    int first_non_blank=-1;
    unsigned char *orig=buffer; // Keep for debugging
    int i;
    find_limit_characters (line, &first_non_blank, &last_non_blank);

    if (first_non_blank==-1)
    {
        *buffer=0;
        return 0;
    }

    int bytes=0;
    for (i=first_non_blank;i<=last_non_blank;i++)
    {
        char c=line[i];
        switch (encoding)
        {
            case ENC_UTF_8:
                bytes=get_char_in_utf_8 (buffer,c);
                break;
            case ENC_LATIN_1:
                get_char_in_latin_1 (buffer,c);
                bytes=1;
                break;
            case ENC_UNICODE:
                get_char_in_unicode (buffer,c);
                bytes=2;				
                break;
        }
        buffer+=bytes;
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length
}

unsigned get_decoder_line_encoded_for_gui (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    unsigned char *line = data->characters[line_num];	
    unsigned char *orig=buffer; // Keep for debugging
    int first=0, last=31;
    int i;

    find_limit_characters(line,&first,&last);
    for (i=first;i<=last;i++)
    {	
        get_char_in_latin_1 (buffer,line[i]);
        buffer++;
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length

}

unsigned get_decoder_line_encoded (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    int col = COL_WHITE;
    int underlined = 0;
    int italics = 0;	
    int i;

    unsigned char *line = data->characters[line_num];	
    unsigned char *orig=buffer; // Keep for debugging
    int first=0, last=31;
    if (trim_subs)
        find_limit_characters(line,&first,&last);
    for (i=first;i<=last;i++)
    {	
        // Handle color
        int its_col = data->colors[line_num][i];
        if (its_col != col  && !nofontcolor)
        {
            if (col!=COL_WHITE) // We need to close the previous font tag
            {
                buffer+= encode_line (buffer,(unsigned char *) "</font>");
            }
            // Add new font tag
            buffer+=encode_line (buffer, (unsigned char*) color_text[its_col][1]);
            if (its_col==COL_USERDEFINED)
            {
                // The previous sentence doesn't copy the whole 
                // <font> tag, just up to the quote before the color
                buffer+=encode_line (buffer, (unsigned char*) usercolor_rgb);
                buffer+=encode_line (buffer, (unsigned char*) "\">");
            }			

            col = its_col;
        }
        // Handle underlined
        int is_underlined = data->fonts[line_num][i] & FONT_UNDERLINED;
        if (is_underlined && underlined==0) // Open underline
        {
            buffer+=encode_line (buffer, (unsigned char *) "<u>");
        }
        if (is_underlined==0 && underlined) // Close underline
        {
            buffer+=encode_line (buffer, (unsigned char *) "</u>");
        } 
        underlined=is_underlined;
        // Handle italics
        int has_ita = data->fonts[line_num][i] & FONT_ITALICS;		
        if (has_ita && italics==0) // Open italics
        {
            buffer+=encode_line (buffer, (unsigned char *) "<i>");
        }
        if (has_ita==0 && italics) // Close italics
        {
            buffer+=encode_line (buffer, (unsigned char *) "</i>");
        } 
        italics=has_ita;
        int bytes=0;
        switch (encoding)
        {
            case ENC_UTF_8:
                bytes=get_char_in_utf_8 (buffer,line[i]);
                break;
            case ENC_LATIN_1:
                get_char_in_latin_1 (buffer,line[i]);
                bytes=1;
                break;
            case ENC_UNICODE:
                get_char_in_unicode (buffer,line[i]);
                bytes=2;				
                break;
        }
        buffer+=bytes;        
    }
    if (italics)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</i>");
    }
    if (underlined)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</u>");
    }
    if (col != COL_WHITE && !nofontcolor)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</font>");
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length
}


void delete_all_lines_but_current (struct eia608_screen *data, int row)
{
    int i;
    for (i=0;i<15;i++)
    {
        if (i!=row)
        {
            memset(data->characters[i],' ',CC608_SCREEN_WIDTH);
            data->characters[i][CC608_SCREEN_WIDTH]=0;		
            memset (data->colors[i],default_color,CC608_SCREEN_WIDTH+1); 
            memset (data->fonts[i],FONT_REGULAR,CC608_SCREEN_WIDTH+1); 
            data->row_used[i]=0;        
        }
    }
}

void clear_eia608_cc_buffer (struct eia608_screen *data)
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

void init_eia608 (struct eia608 *data)
{
    data->cursor_column=0;
    data->cursor_row=0;
    clear_eia608_cc_buffer (&data->buffer1);
    clear_eia608_cc_buffer (&data->buffer2);
    data->visible_buffer=1;
    data->last_c1=0;
    data->last_c2=0;
    data->mode=MODE_POPUP;
    // data->current_visible_start_cc=0;
    data->current_visible_start_ms=0;
    data->srt_counter=0;
    data->screenfuls_counter=0;
    data->channel=1;	
    data->color=default_color;
    data->font=FONT_REGULAR;
    data->rollup_base_row=14;
}

struct eia608_screen *get_writing_buffer (struct s_write *wb)
{
    struct eia608_screen *use_buffer=NULL;
    switch (wb->data608->mode)
    {
        case MODE_POPUP: // Write on the non-visible buffer
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer2;
            else
                use_buffer = &wb->data608->buffer1;
            break;
        case MODE_ROLLUP_2: // Write directly to screen
        case MODE_ROLLUP_3:
        case MODE_ROLLUP_4:
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer1;
            else
                use_buffer = &wb->data608->buffer2;
            break;
        default:
            fatal (EXIT_BUG_BUG, "Caption mode has an illegal value at get_writing_buffer(), this is a bug.\n");            
    }
    return use_buffer;
}

void write_char (const unsigned char c, struct s_write *wb)
{
    if (wb->data608->mode!=MODE_TEXT)
    {
        struct eia608_screen * use_buffer=get_writing_buffer(wb);
        /* hb_log ("\rWriting char [%c] at %s:%d:%d\n",c,
        use_buffer == &wb->data608->buffer1?"B1":"B2",
        wb->data608->cursor_row,wb->data608->cursor_column); */
        use_buffer->characters[wb->data608->cursor_row][wb->data608->cursor_column]=c;
        use_buffer->colors[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->color;
        use_buffer->fonts[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->font;	
        use_buffer->row_used[wb->data608->cursor_row]=1;
        use_buffer->empty=0;
        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
    }

}

/* Handle MID-ROW CODES. */
void handle_text_attr (const unsigned char c1, const unsigned char c2, struct s_write *wb)
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
            hb_log ("  --  Color: %s,  font: %s\n",
            color_text[wb->data608->color][0],
            font_text[wb->data608->font]);
        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
    }
}

void mstotime (LLONG milli, unsigned *hours, unsigned *minutes,
               unsigned *seconds, unsigned *ms)
{
    // LLONG milli = (LLONG) ((ccblock*1000)/29.97);
    *ms=(unsigned) (milli%1000); // milliseconds
    milli=(milli-*ms)/1000;  // Remainder, in seconds
    *seconds = (int) (milli%60);
    milli=(milli-*seconds)/60; // Remainder, in minutes
    *minutes = (int) (milli%60);
    milli=(milli-*minutes)/60; // Remainder, in hours
    *hours=(int) milli;
}

void write_subtitle_file_footer (struct s_write *wb)
{
    switch (write_format)
    {
        case OF_SAMI:
            sprintf ((char *) str,"</BODY></SAMI>\n");
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                hb_log ("\r%s\n", str);
            }
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
            //fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
            XMLRPC_APPEND(enc_buffer,enc_buffer_used);
            break;
        default: // Nothing to do. Only SAMI has a footer
            break;
    }
}

void fhb_log_encoded (FILE *fh, const char *string)
{
    GUARANTEE(strlen (string)*3);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) string);
    fwrite (enc_buffer,enc_buffer_used,1,fh);
}

void write_subtitle_file_header (struct s_write *wb)
{
    switch (write_format)
    {
        case OF_SRT: // Subrip subtitles have no header
            break; 
        case OF_SAMI: // This header brought to you by McPoodle's CCASDI  
            //fhb_log_encoded (wb->fh, sami_header);
            GUARANTEE(strlen (sami_header)*3);
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) sami_header);
            //fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
            XMLRPC_APPEND(enc_buffer,enc_buffer_used);
            break;
        case OF_RCWT: // Write header
            //fwrite (rcwt_header, sizeof(rcwt_header),1,wb->fh);
            break;
        case OF_TRANSCRIPT: // No header. Fall thru
        default:
            break;
    }
}

void write_cc_line_as_transcript (struct eia608_screen *data, struct s_write *wb, int line_number)
{
    hb_buffer_t *buffer;

    if (sentence_cap)
    {
        capitalize(line_number,data, &wb->new_sentence);
        correct_case(line_number,data);
    }
    int length = get_decoder_line_basic (wb->subline, line_number, data);
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        hb_log ("\r");
        hb_log ("%s\n",wb->subline);
    }
    if (length>0)
    {
        //fwrite (wb->subline, 1, length, wb->fh);
        /*
         * Put this subtitle in a hb_buffer_t and shove it into the subtitle fifo
         */
        buffer = hb_buffer_init( strlen( wb->subline ) + 1 );
        buffer->start = wb->last_pts;
        buffer->stop = wb->last_pts+1;
        strcpy( buffer->data, wb->subline );
        //hb_log("CC %lld: %s", buffer->stop, wb->subline);

        hb_fifo_push( wb->subtitle->fifo_raw, buffer );

        XMLRPC_APPEND(wb->subline,length);
        //fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
        XMLRPC_APPEND(encoded_crlf,encoded_crlf_length);    
    }
    // fhb_log (wb->fh,encoded_crlf);
}

int write_cc_buffer_as_transcript (struct eia608_screen *data, struct s_write *wb)
{
    int i;

    int wrote_something = 0;
    if (debug_608)
    {
        hb_log ("\n- - - TRANSCRIPT caption - - -\n");        
    }
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {		
            write_cc_line_as_transcript (data,wb, i);
        }
        wrote_something=1;
    }
    if (debug_608)
    {
        hb_log ("- - - - - - - - - - - -\r\n");
    }
    return wrote_something;
}

void write_cc_buffer_to_gui (struct eia608_screen *data, struct s_write *wb)
{
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;    
    int i;

    LLONG ms_start= wb->data608->current_visible_start_ms;

    ms_start+=subs_delay;
    if (ms_start<0) // Drop screens that because of subs_delay start too early
        return;
    int time_reported=0;    
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {
            hb_log ("###SUBTITLE#");            
            if (!time_reported)
            {
                LLONG ms_end = get_fts()+subs_delay;		
                mstotime (ms_start,&h1,&m1,&s1,&ms1);
                mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
                // Note, only MM:SS here as we need to save space in the preview window
                hb_log ("%02u:%02u#%02u:%02u#",
                        h1*60+m1,s1, h2*60+m2,s2);
                time_reported=1;
            }
            else
                hb_log ("##");
            
            // We don't capitalize here because whatever function that was used
            // before to write to file already took care of it.
            int length = get_decoder_line_encoded_for_gui (wb->subline, i, data);
            fwrite (wb->subline, 1, length, stderr);
            fwrite ("\n",1,1,stderr);
        }
    }
    fflush (stderr);
}

int write_cc_buffer_as_srt (struct eia608_screen *data, struct s_write *wb)
{
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;
    int wrote_something = 0;
    LLONG ms_start= wb->data608->current_visible_start_ms;
    int i;

    ms_start+=subs_delay;
    if (ms_start<0) // Drop screens that because of subs_delay start too early
        return 0;

    LLONG ms_end = get_fts()+subs_delay;		
    mstotime (ms_start,&h1,&m1,&s1,&ms1);
    mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
    char timeline[128];   
    wb->data608->srt_counter++;
    sprintf (timeline,"%u\r\n",wb->data608->srt_counter);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
    XMLRPC_APPEND(enc_buffer,enc_buffer_used);
    sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
        h1,m1,s1,ms1, h2,m2,s2,ms2);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
    if (debug_608)
    {
        hb_log ("\n- - - SRT caption - - -\n");
        hb_log (timeline);
    }
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);		
    XMLRPC_APPEND(enc_buffer,enc_buffer_used);
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {		
            if (sentence_cap)
            {
                capitalize(i,data, &wb->new_sentence);
                correct_case(i,data);
            }
            int length = get_decoder_line_encoded (wb->subline, i, data);
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                hb_log ("\r");
                hb_log ("%s\n",wb->subline);
            }
            fwrite (wb->subline, 1, length, wb->fh);
            XMLRPC_APPEND(wb->subline,length);
            fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
            XMLRPC_APPEND(encoded_crlf,encoded_crlf_length);
            wrote_something=1;
            // fhb_log (wb->fh,encoded_crlf);
        }
    }
    if (debug_608)
    {
        hb_log ("- - - - - - - - - - - -\r\n");
    }
    // fhb_log (wb->fh, encoded_crlf);
    fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
    XMLRPC_APPEND(encoded_crlf,encoded_crlf_length);
    return wrote_something;
}

int write_cc_buffer_as_sami (struct eia608_screen *data, struct s_write *wb)
{
    int wrote_something=0;
    LLONG startms = wb->data608->current_visible_start_ms;
    int i;

    startms+=subs_delay;
    if (startms<0) // Drop screens that because of subs_delay start too early
        return 0; 

    LLONG endms   = get_fts()+subs_delay;
    endms--; // To prevent overlapping with next line.
    sprintf ((char *) str,"<SYNC start=\"%llu\"><P class=\"UNKNOWNCC\">\r\n",startms);
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        hb_log ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);		
    XMLRPC_APPEND(enc_buffer,enc_buffer_used);
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {				
            int length = get_decoder_line_encoded (wb->subline, i, data);
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                hb_log ("\r");
                hb_log ("%s\n",wb->subline);
            }
            fwrite (wb->subline, 1, length, wb->fh);
            XMLRPC_APPEND(wb->subline,length);
            wrote_something=1;
            if (i!=14)
            {
                fwrite (encoded_br, 1, encoded_br_length,wb->fh);			
                XMLRPC_APPEND(encoded_br, encoded_br_length);
            }
            fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
            XMLRPC_APPEND(encoded_crlf, encoded_crlf_length);
        }
    }
    sprintf ((char *) str,"</P></SYNC>\r\n");
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        hb_log ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
    XMLRPC_APPEND(enc_buffer,enc_buffer_used);
    sprintf ((char *) str,"<SYNC start=\"%llu\"><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n",endms);
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        hb_log ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
    XMLRPC_APPEND(enc_buffer,enc_buffer_used);
    return wrote_something;
}

struct eia608_screen *get_current_visible_buffer (struct s_write *wb)
{
    struct eia608_screen *data;
    if (wb->data608->visible_buffer==1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;
    return data;
}


int write_cc_buffer (struct s_write *wb)
{
    struct eia608_screen *data;
    int wrote_something=0;
    if (screens_to_process!=-1 && wb->data608->screenfuls_counter>=screens_to_process)
    {
        // We are done. 
        processed_enough=1;
        return 0;
    }
    if (wb->data608->visible_buffer==1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;

    if (!data->empty)
    {
        wb->new_sentence=1;
        switch (write_format)
        {
            case OF_SRT:
                wrote_something = write_cc_buffer_as_srt (data, wb);
                break;
            case OF_SAMI:
                wrote_something = write_cc_buffer_as_sami (data,wb);
                break;
            case OF_TRANSCRIPT:
                wrote_something = write_cc_buffer_as_transcript (data,wb);
                break;
        default: 
                break;
        }
        if (wrote_something && gui_mode_reports)
            write_cc_buffer_to_gui (data,wb);
    }
    return wrote_something;
}

void roll_up(struct s_write *wb)
{
    struct eia608_screen *use_buffer;
    int i, j;

    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;
    int keep_lines;
    switch (wb->data608->mode)
    {
        case MODE_ROLLUP_2:
            keep_lines=2;
            break;
        case MODE_ROLLUP_3:
            keep_lines=3;
            break;
        case MODE_ROLLUP_4:
            keep_lines=4;
            break;
        default: // Shouldn't happen
            keep_lines=0;
            break;
    }
    int firstrow=-1, lastrow=-1;
    // Look for the last line used
    int rows_now=0; // Number of rows in use right now
    for (i=0;i<15;i++)
    {
        if (use_buffer->row_used[i])
        {
            rows_now++;
            if (firstrow==-1)
                firstrow=i;
            lastrow=i;
        }
    }
    
    if (debug_608)
        hb_log ("\rIn roll-up: %d lines used, first: %d, last: %d\n", rows_now, firstrow, lastrow);

    if (lastrow==-1) // Empty screen, nothing to rollup
        return;

    for (j=lastrow-keep_lines+1;j<lastrow; j++)
    {
        if (j>=0)
        {
            memcpy (use_buffer->characters[j],use_buffer->characters[j+1],CC608_SCREEN_WIDTH+1);
            memcpy (use_buffer->colors[j],use_buffer->colors[j+1],CC608_SCREEN_WIDTH+1);
            memcpy (use_buffer->fonts[j],use_buffer->fonts[j+1],CC608_SCREEN_WIDTH+1);
            use_buffer->row_used[j]=use_buffer->row_used[j+1];
        }
    }
    for (j=0;j<(1+wb->data608->cursor_row-keep_lines);j++)
    {
        memset(use_buffer->characters[j],' ',CC608_SCREEN_WIDTH);			
        memset(use_buffer->colors[j],COL_WHITE,CC608_SCREEN_WIDTH);
        memset(use_buffer->fonts[j],FONT_REGULAR,CC608_SCREEN_WIDTH);
        use_buffer->characters[j][CC608_SCREEN_WIDTH]=0;
        use_buffer->row_used[j]=0;
    }
    memset(use_buffer->characters[lastrow],' ',CC608_SCREEN_WIDTH);
    memset(use_buffer->colors[lastrow],COL_WHITE,CC608_SCREEN_WIDTH);
    memset(use_buffer->fonts[lastrow],FONT_REGULAR,CC608_SCREEN_WIDTH);

    use_buffer->characters[lastrow][CC608_SCREEN_WIDTH]=0;
    use_buffer->row_used[lastrow]=0;
    
    // Sanity check
    rows_now=0;
    for (i=0;i<15;i++)
        if (use_buffer->row_used[i])
            rows_now++;
    if (rows_now>keep_lines)
        hb_log ("Bug in roll_up, should have %d lines but I have %d.\n",
            keep_lines, rows_now);
}

void erase_memory (struct s_write *wb, int displayed)
{
    struct eia608_screen *buf;
    if (displayed)
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer1;
        else
            buf=&wb->data608->buffer2;
    }
    else
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer2;
        else
            buf=&wb->data608->buffer1;
    }
    clear_eia608_cc_buffer (buf);
}

int is_current_row_empty (struct s_write *wb)
{
    struct eia608_screen *use_buffer;
    int i;

    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;
    for (i=0;i<CC608_SCREEN_WIDTH;i++)
    {
        if (use_buffer->characters[wb->data608->rollup_base_row][i]!=' ')
            return 0;
    }
    return 1;
}

/* Process GLOBAL CODES */
void handle_command (/*const */ unsigned char c1, const unsigned char c2, struct s_write *wb)
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
                wb->data608->cursor_column--;
                get_writing_buffer(wb)->characters[wb->data608->cursor_row][wb->data608->cursor_column]=' ';
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
            break;
        case COM_RESUMETEXTDISPLAY:
            wb->data608->mode=MODE_TEXT;
            break;
        case COM_ROLLUP2:            
            if (wb->data608->mode==MODE_POPUP)
            {
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);			
            }
            if (wb->data608->mode==MODE_ROLLUP_2 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU2, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
            }   
            wb->data608->mode=MODE_ROLLUP_2;
            erase_memory (wb, 0);
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=wb->data608->rollup_base_row;
            break;
        case COM_ROLLUP3:
            if (wb->data608->mode==MODE_POPUP)
            {
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);
            }
            if (wb->data608->mode==MODE_ROLLUP_3 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU3, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
            }
            wb->data608->mode=MODE_ROLLUP_3;
            erase_memory (wb, 0);
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=wb->data608->rollup_base_row;
            break;
        case COM_ROLLUP4:
            if (wb->data608->mode==MODE_POPUP)
            {
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, 1);			
            }
            if (wb->data608->mode==MODE_ROLLUP_4 && !is_current_row_empty(wb))
            {
                if (debug_608)
                    hb_log ("Two RU4, current line not empty. Simulating a CR\n");
                handle_command(0x14, 0x2D, wb);
            }
            
            wb->data608->mode=MODE_ROLLUP_4;
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=wb->data608->rollup_base_row;
            erase_memory (wb, 0);
            break;
        case COM_CARRIAGERETURN:
            // In transcript mode, CR doesn't write the whole screen, to avoid
            // repeated lines.
            if (write_format==OF_TRANSCRIPT)
            {
                write_cc_line_as_transcript(get_current_visible_buffer (wb), wb, wb->data608->cursor_row);
            }
            else
            {
                if (norollup)
                    delete_all_lines_but_current (get_current_visible_buffer (wb), wb->data608->cursor_row);
                if (write_cc_buffer(wb))
                    wb->data608->screenfuls_counter++;
            }
            roll_up(wb);		
            wb->data608->current_visible_start_ms=get_fts();
            wb->data608->cursor_column=0;
            break;
        case COM_ERASENONDISPLAYEDMEMORY:
            erase_memory (wb,0);
            break;
        case COM_ERASEDISPLAYEDMEMORY:
            // Write it to disk before doing this, and make a note of the new
            // time it became clear.
            if (write_format==OF_TRANSCRIPT && 
                (wb->data608->mode==MODE_ROLLUP_2 || wb->data608->mode==MODE_ROLLUP_3 ||
                wb->data608->mode==MODE_ROLLUP_4))
            {
                // In transcript mode we just write the cursor line. The previous lines
                // should have been written already, so writing everything produces
                // duplicate lines.                
                write_cc_line_as_transcript(get_current_visible_buffer (wb), wb, wb->data608->cursor_row);
            }
            else
            {
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
            }
            erase_memory (wb,1);
            wb->data608->current_visible_start_ms=get_fts();
            break;
        case COM_ENDOFCAPTION: // Switch buffers
            // The currently *visible* buffer is leaving, so now we know it's ending
            // time. Time to actually write it to file.
            if (write_cc_buffer (wb))
                wb->data608->screenfuls_counter++;
            wb->data608->visible_buffer = (wb->data608->visible_buffer==1) ? 2 : 1;
            wb->data608->current_visible_start_ms=get_fts();
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=0;
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

void handle_end_of_data (struct s_write *wb)
{
    // We issue a EraseDisplayedMemory here so if there's any captions pending
    // they get written to file. 
    handle_command (0x14, 0x2c, wb); // EDM
}

void handle_double (const unsigned char c1, const unsigned char c2, struct s_write *wb)
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
unsigned char handle_extended (unsigned char hi, unsigned char lo, struct s_write *wb)
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
void handle_pac (unsigned char c1, unsigned char c2, struct s_write *wb)
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
    int color=pac2_attribs[c2][0];
    int font=pac2_attribs[c2][1];
    int indent=pac2_attribs[c2][2];
    if (debug_608)
        hb_log ("  --  Position: %d:%d, color: %s,  font: %s\n",row,
        indent,color_text[color][0],font_text[font]);
    if (wb->data608->mode!=MODE_TEXT)
    {
        // According to Robson, row info is discarded in text mode
        // but column is accepted
        wb->data608->cursor_row=row-1 ; // Since the array is 0 based
    }
    wb->data608->rollup_base_row=row-1;
    wb->data608->cursor_column=indent;	
}


void handle_single (const unsigned char c1, struct s_write *wb)
{	
    if (c1<0x20 || wb->data608->channel!=cc_channel)
        return; // We don't allow special stuff here
    if (debug_608)
        hb_log ("%c",c1);

    /*if (debug_608)
    hb_log ("Character: %02X (%c) -> %02X (%c)\n",c1,c1,c,c); */
    write_char (c1,wb);
}

int check_channel (unsigned char c1, struct s_write *wb)
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
int disCommand (unsigned char hi, unsigned char lo, struct s_write *wb)
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
            if (lo>=0x21 && lo<=0x22)
                handle_command (hi,lo,wb);
            if (lo>=0x2e && lo<=0x2f)
                handle_text_attr (hi,lo,wb);
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
    }
    return wrote_to_screen;
}

void process608 (const unsigned char *data, int length, struct s_write *wb)
{
    static int textprinted = 0;
    int i;

    if (data!=NULL)
    {
        for (i=0;i<length;i=i+2)
        {
            unsigned char hi, lo;
            int wrote_to_screen=0; 
            hi = data[i] & 0x7F; // Get rid of parity bit
            lo = data[i+1] & 0x7F; // Get rid of parity bit

            if (hi==0 && lo==0) // Just padding
                continue;
            // hb_log ("\r[%02X:%02X]\n",hi,lo);

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
                wrote_to_screen=disCommand (hi,lo,wb);
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
                wrote_to_screen=1;
                wb->data608->last_c1=0;
                wb->data608->last_c2=0;
            }

            if ( debug_608 && !textprinted && wb->data608->channel==cc_channel )
            {   // Current FTS information after the characters are shown
                //hb_log("Current FTS: %s\n", print_mstime(get_fts()));
            }

            if (wrote_to_screen && direct_rollup && // If direct_rollup is enabled and
                (wb->data608->mode==MODE_ROLLUP_2 || // we are in rollup mode, write now.
                wb->data608->mode==MODE_ROLLUP_3 ||
                wb->data608->mode==MODE_ROLLUP_4))
            {
                // We don't increase screenfuls_counter here.
                write_cc_buffer (wb);
                wb->data608->current_visible_start_ms=get_fts();
            }
        }
    }
}


/* Return a pointer to a string that holds the printable characters
 * of the caption data block. FOR DEBUG PURPOSES ONLY! */
unsigned char *debug_608toASC (unsigned char *cc_data, int channel)
{
    static unsigned char output[3];

    unsigned char cc_valid = (cc_data[0] & 4) >>2;
    unsigned char cc_type = cc_data[0] & 3;
    unsigned char hi, lo;

    output[0]=' ';
    output[1]=' ';
    output[2]='\x00';

    if (cc_valid && cc_type==channel)
    {
        hi = cc_data[1] & 0x7F; // Get rid of parity bit
        lo = cc_data[2] & 0x7F; // Get rid of parity bit
        if (hi>=0x20)
        {
            output[0]=hi;
            output[1]=(lo>=20 ? lo : '.');
            output[2]='\x00';
        }
        else
        {
            output[0]='<';
            output[1]='>';
            output[2]='\x00';
        }
    }
    return output;
}
