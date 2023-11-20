/* dvd.h

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_DVD_H
#define HANDBRAKE_DVD_H

#include "dvdnav/dvdnav.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"

#define HB_VOBSUB_STYLE_4_3        0
#define HB_VOBSUB_STYLE_WIDE       1
#define HB_VOBSUB_STYLE_LETTERBOX  2
#define HB_VOBSUB_STYLE_PANSCAN    3

struct hb_dvd_chapter_s
{
    int     index;
    int64_t duration;
    int     pgn;
    int     pgcn;
    int     block_start;
    int     block_end;
};

struct hb_dvdread_s
{
    char         * path;

    dvd_reader_t * reader;
    ifo_handle_t * vmg;

    int            vts;
    int            ttn;
    ifo_handle_t * ifo;
    dvd_file_t   * file;

    pgc_t        * pgc;
    int            cell_start;
    int            cell_end;
    int            title_start;
    int            title_end;
    int            title_block_count;
    int            cell_cur;
    int            cell_next;
    int            cell_overlap;
    int            block;
    int            pack_len;
    int            next_vobu;
    int            in_cell;
    int            in_sync;
    uint16_t       cur_vob_id;
    uint8_t        cur_cell_id;
    hb_handle_t  * h;
    int            chapter;
};

struct hb_dvdnav_s
{
    char         * path;

    dvdnav_t     * dvdnav;
    dvd_reader_t * reader;
    ifo_handle_t * vmg;
    int            title;
    int            title_block_count;
    int            chapter;
    int            cell;
    int            stopped;
    hb_handle_t  * h;

    ifo_handle_t * ifo;
    int            vts;
    int            pgcn;
    int            pgn;
    int64_t        duration;
    hb_list_t    * list_dvd_chapter;
};

typedef struct hb_dvd_chapter_s hb_dvd_chapter_t;
typedef struct hb_dvdnav_s hb_dvdnav_t;
typedef struct hb_dvdread_s hb_dvdread_t;

union hb_dvd_s
{
    hb_dvdread_t dvdread;
    hb_dvdnav_t  dvdnav;
};


struct hb_dvd_func_s
{
    hb_dvd_t *    (* init)        ( hb_handle_t *, const char * );
    void          (* close)       ( hb_dvd_t ** );
    char        * (* name)        ( char * );
    int           (* title_count) ( hb_dvd_t * );
    hb_title_t  * (* title_scan)  ( hb_dvd_t *, int, uint64_t );
    int           (* start)       ( hb_dvd_t *, hb_title_t *, int );
    void          (* stop)        ( hb_dvd_t * );
    int           (* seek)        ( hb_dvd_t *, float );
    hb_buffer_t * (* read)        ( hb_dvd_t * );
    int           (* chapter)     ( hb_dvd_t * );
    int           (* angle_count) ( hb_dvd_t * );
    void          (* set_angle)   ( hb_dvd_t *, int );
    int           (* main_feature)( hb_dvd_t *, hb_list_t * );
};
typedef struct hb_dvd_func_s hb_dvd_func_t;

hb_dvd_func_t * hb_dvdnav_methods( void );
hb_dvd_func_t * hb_dvdread_methods( void );

#endif // HANDBRAKE_DVD_H
