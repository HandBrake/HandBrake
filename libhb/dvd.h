/* $Id: dvd.h,v 1.1 2004/08/02 07:19:05 stebbins Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_DVD_H
#define HB_DVD_H

#include "dvdnav/dvdnav.h"
#include "dvdread/ifo_read.h"
#include "dvdread/nav_read.h"

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
};

typedef struct hb_dvdnav_s hb_dvdnav_t;
typedef struct hb_dvdread_s hb_dvdread_t;

union hb_dvd_s
{
	hb_dvdread_t dvdread;
	hb_dvdnav_t  dvdnav;
};


struct hb_dvd_func_s
{
	hb_dvd_t *    (* init)        ( char * );
	void          (* close)       ( hb_dvd_t ** );
	char        * (* name)        ( char * );
	int           (* title_count) ( hb_dvd_t * );
	hb_title_t  * (* title_scan)  ( hb_dvd_t *, int );
	int           (* start)       ( hb_dvd_t *, int, int );
	void          (* stop)        ( hb_dvd_t * );
	int           (* seek)        ( hb_dvd_t *, float );
	int           (* read)        ( hb_dvd_t *, hb_buffer_t * );
	int           (* chapter)     ( hb_dvd_t * );
	int           (* angle_count) ( hb_dvd_t * );
	void          (* set_angle)   ( hb_dvd_t *, int );
};
typedef struct hb_dvd_func_s hb_dvd_func_t;

hb_dvd_func_t * hb_dvdnav_methods( void );
hb_dvd_func_t * hb_dvdread_methods( void );

#endif // HB_DVD_H


