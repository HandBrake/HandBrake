/* parsecsv.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <fcntl.h>
#include "handbrake/handbrake.h"
#include "parsecsv.h"

/* Internal declarations */
#define is_newline(_x)      ( (_x) == 13 || \
                              (_x) == 11 || \
                              (_x) == 10 )

#define is_white(_x)        ( (_x) == '\t' || \
                              (_x) == ' '  || \
                              is_newline(_x) )

#define is_sep(_x)          ( (_x) == ',' )

#define is_esc(_x)          ( (_x) == '\\' )

#define CSV_CHAR_ERROR      0x8000
#define CSV_CHAR_EOF        0x4000
#define CSV_CHAR_ROWSEP     0x2000
#define CSV_CHAR_COLSEP     0x1000

#define CSV_PARSE_NORMAL    0x0000
#define CSV_PARSE_SEEK      0x0001
#define CSV_PARSE_ESC       0x0002

static uint16_t hb_parse_character( hb_csv_file_t * file );
static void hb_trim_end( char *text );

/* Open a CSV File */
hb_csv_file_t *hb_open_csv_file( const char *filepath )
{
    hb_csv_file_t *file = NULL;
    FILE * fileref;

    if( filepath == NULL )
    {
        return file;
    }

    fileref = hb_fopen(filepath, "r");
    if( fileref == NULL )
    {
        return file;
    }

    file = malloc( sizeof( hb_csv_file_t ) );
    if( file == NULL )
    {
        return file;
    }

    file->fileref       = fileref;
    file->eof           = 0;
    file->parse_state   = CSV_PARSE_SEEK;
    file->curr_col      = 0;
    file->curr_row      = 0;
    return file;
}

void hb_close_csv_file( hb_csv_file_t *file )
{
    if( file == NULL )
    {
        return;
    }

    fclose( file->fileref );
    free( file );
}

/* Parse CSV Cells */
hb_csv_cell_t *hb_read_next_cell( hb_csv_file_t *file )
{
    hb_csv_cell_t *cell = NULL;
    uint16_t c;
    int index;

    if( file == NULL  )
    {
        return cell;
    }

    if( file->eof )
    {
        return cell;
    }

    cell = malloc( sizeof( hb_csv_cell_t ) );
    if( cell == NULL )
    {
        return cell;
    }

    cell->cell_row = file->curr_row;
    cell->cell_col = file->curr_col;
    index = 0;
    while( CSV_CHAR_EOF != (c = hb_parse_character( file ) ) )
    {
        if( c == CSV_CHAR_ROWSEP )
        {
            file->curr_row++;
            file->curr_col = 0;
            break;
        }
        else if( c == CSV_CHAR_COLSEP )
        {
            file->curr_col++;
            break;
        }
        else
        {
            if( index < 1023 )
            {
                cell->cell_text[index] = (char)c;
                index++;
            }
        }
    }

    if( c == CSV_CHAR_EOF )
    {
        file->eof = 1;
    }

    /* Terminate the cell text */
    cell->cell_text[index] = '\0';
    hb_trim_end( cell->cell_text );
    return cell;
}

void hb_dispose_cell( hb_csv_cell_t *cell )
{
    if( cell == NULL )
    {
        return;
    }

    free( cell );
}

/* Raw parsing */
static uint16_t hb_parse_character( hb_csv_file_t * file )
{
    int byte;
    uint16_t c = 0;
    int need_char = 1;

    if( file == NULL )
    {
        return CSV_CHAR_ERROR;
    }

    while( need_char )
    {
        byte = fgetc( file->fileref );
        if( feof( file->fileref ) )
        {
            return CSV_CHAR_EOF;
        }
        if( ferror( file->fileref ) )
        {
            return CSV_CHAR_ERROR;
        }

        if( file->parse_state == CSV_PARSE_SEEK && is_white(byte) )
        {
            continue;
        }
        else if( file->parse_state != CSV_PARSE_ESC && is_esc(byte) )
        {
            file->parse_state = CSV_PARSE_ESC;
            continue;
        }
        else if( file->parse_state != CSV_PARSE_ESC && is_sep(byte) )
        {
            file->parse_state = CSV_PARSE_SEEK;
            need_char = 0;
            c = CSV_CHAR_COLSEP;
        }
        else if( file->parse_state == CSV_PARSE_ESC )
        {
            file->parse_state = CSV_PARSE_NORMAL;
            need_char = 0;
            c = (uint16_t)byte;
        }
        else if( is_newline(byte) )
        {
            file->parse_state = CSV_PARSE_SEEK;
            need_char = 0;
            c = CSV_CHAR_ROWSEP;
        }
        else
        {
            file->parse_state = CSV_PARSE_NORMAL;
            need_char = 0;
            c = (uint16_t)byte;
        }
    }

    return c;
}

static void hb_trim_end( char *text )
{
    if( text == NULL )
    {
        return;
    }

    int i;

    for( i = strlen(text) - 1; i >= 0 && is_white(text[i]) ; i-- )
    {
        text[i] = '\0';
    }
}
