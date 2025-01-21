/* parsecsv.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
    A very simple CSV file parser.
 */

typedef struct hb_csv_file_s hb_csv_file_t;
typedef struct hb_csv_cell_s hb_csv_cell_t;

struct hb_csv_file_s
{
    FILE  * fileref;
    int     eof;
    int     parse_state;
    int     curr_row;
    int     curr_col;
};

struct hb_csv_cell_s
{
    char    cell_text[1024];
    int     cell_row;
    int     cell_col;
};

/* Open a CSV File */
hb_csv_file_t *hb_open_csv_file( const char *filepath );
void hb_close_csv_file( hb_csv_file_t *file );

/* Parse CSV Cells */
hb_csv_cell_t *hb_read_next_cell( hb_csv_file_t *file );
void hb_dispose_cell( hb_csv_cell_t *cell );
