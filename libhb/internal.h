/* $Id: internal.h,v 1.41 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

/***********************************************************************
 * common.c
 **********************************************************************/
void hb_log( char * log, ... );

int  hb_list_bytes( hb_list_t * );
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size );
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, int * pos );
void hb_list_empty( hb_list_t ** );

hb_title_t * hb_title_init( char * dvd, int index );
void         hb_title_close( hb_title_t ** );

/***********************************************************************
 * hb.c
 **********************************************************************/
int  hb_get_pid( hb_handle_t * );
void hb_set_state( hb_handle_t *, hb_state_t * );

/***********************************************************************
 * fifo.c
 **********************************************************************/
typedef struct hb_buffer_s hb_buffer_t;
struct hb_buffer_s
{
    int           size;
    int           alloc;
    uint8_t *     data;
    int           cur;

    int           id;
    int64_t       start;
    int64_t       stop;
    int           key;

    int           x;
    int           y;
    int           width;
    int           height;

    hb_buffer_t * sub;

    hb_buffer_t * next;
};

hb_buffer_t * hb_buffer_init( int size );
void          hb_buffer_realloc( hb_buffer_t *, int size );
void          hb_buffer_close( hb_buffer_t ** );

typedef struct hb_fifo_s hb_fifo_t;

hb_fifo_t   * hb_fifo_init();
int           hb_fifo_size( hb_fifo_t * );
int           hb_fifo_is_full( hb_fifo_t * );
hb_buffer_t * hb_fifo_get( hb_fifo_t * );
hb_buffer_t * hb_fifo_see( hb_fifo_t * );
void          hb_fifo_push( hb_fifo_t *, hb_buffer_t * );
void          hb_fifo_close( hb_fifo_t ** );

/***********************************************************************
 * Threads: update.c, scan.c, work.c, reader.c, muxcommon.c
 **********************************************************************/
hb_thread_t * hb_update_init( int * build, char * version );
hb_thread_t * hb_scan_init( hb_handle_t *, const char * path,
                            int title_index, hb_list_t * list_title );
hb_thread_t * hb_work_init( hb_list_t * jobs, int cpu_count,
                            volatile int * die, int * error );
hb_thread_t  * hb_reader_init( hb_job_t * );
hb_thread_t  * hb_muxer_init( hb_job_t * );

/***********************************************************************
 * libmpeg2 wrapper
 ***********************************************************************
 * It is exported here because it is used at several places
 **********************************************************************/
typedef struct   hb_libmpeg2_s hb_libmpeg2_t;

hb_libmpeg2_t  * hb_libmpeg2_init();
int              hb_libmpeg2_decode( hb_libmpeg2_t *,
                                      hb_buffer_t * es_buf,
                                      hb_list_t * raw_list );
void             hb_libmpeg2_info( hb_libmpeg2_t * m, int * width,
                                    int * height, int * rate );
void             hb_libmpeg2_close( hb_libmpeg2_t ** );

/***********************************************************************
 * mpegdemux.c
 **********************************************************************/
int hb_demux_ps( hb_buffer_t * ps_buf, hb_list_t * es_list );

/***********************************************************************
 * dvd.c
 **********************************************************************/
typedef struct hb_dvd_s hb_dvd_t;

hb_dvd_t *   hb_dvd_init( char * path );
int          hb_dvd_title_count( hb_dvd_t * );
hb_title_t * hb_dvd_title_scan( hb_dvd_t *, int title );
int          hb_dvd_start( hb_dvd_t *, int title, int chapter );
void         hb_dvd_stop( hb_dvd_t * );
int          hb_dvd_seek( hb_dvd_t *, float );
int          hb_dvd_read( hb_dvd_t *, hb_buffer_t * );
int          hb_dvd_chapter( hb_dvd_t * );
void         hb_dvd_close( hb_dvd_t ** );

/***********************************************************************
 * Work objects
 **********************************************************************/
typedef struct hb_work_object_s hb_work_object_t;

#define HB_WORK_COMMON \
    hb_lock_t  * lock; \
    int          used; \
    uint64_t     time; \
    char       * name; \
    hb_fifo_t  * fifo_in; \
    hb_fifo_t  * fifo_out; \
    int       (* work)  ( hb_work_object_t *, hb_buffer_t **, \
                          hb_buffer_t ** ); \
    void      (* close) ( hb_work_object_t ** )

#define HB_WORK_IDLE     0
#define HB_WORK_OK       1
#define HB_WORK_ERROR    2
#define HB_WORK_DONE     3


#define DECLARE_WORK_NORMAL( a ) \
    hb_work_object_t * hb_work_##a##_init( hb_job_t * );

#define DECLARE_WORK_AUDIO( a ) \
    hb_work_object_t * hb_work_##a##_init( hb_job_t *, hb_audio_t * );

DECLARE_WORK_NORMAL( sync );
DECLARE_WORK_NORMAL( decmpeg2 );
DECLARE_WORK_NORMAL( decsub );
DECLARE_WORK_NORMAL( render );
DECLARE_WORK_NORMAL( encavcodec );
DECLARE_WORK_NORMAL( encxvid );
DECLARE_WORK_NORMAL( encx264 );
DECLARE_WORK_AUDIO( deca52 );
DECLARE_WORK_AUDIO( decavcodec );
DECLARE_WORK_AUDIO( declpcm );
DECLARE_WORK_AUDIO( encfaac );
DECLARE_WORK_AUDIO( enclame );
DECLARE_WORK_AUDIO( encvorbis );

/***********************************************************************
 * Muxers
 **********************************************************************/
typedef struct hb_mux_object_s hb_mux_object_t;
typedef struct hb_mux_data_s   hb_mux_data_t;

#define HB_MUX_COMMON \
    int (*init)      ( hb_mux_object_t * ); \
    int (*mux)       ( hb_mux_object_t *, hb_mux_data_t *, \
                       hb_buffer_t * ); \
    int (*end)       ( hb_mux_object_t * );

#define DECLARE_MUX( a ) \
    hb_mux_object_t  * hb_mux_##a##_init( hb_job_t * );

DECLARE_MUX( mp4 );
DECLARE_MUX( avi );
DECLARE_MUX( ogm );

