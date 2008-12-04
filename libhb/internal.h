/* $Id: internal.h,v 1.41 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/***********************************************************************
 * common.c
 **********************************************************************/
void hb_log( char * log, ... );
extern int global_verbosity_level; // Global variable for hb_deep_log
typedef enum hb_debug_level_s
{
    HB_SUPPORT_LOG      = 1, // helpful in tech support
    HB_HOUSEKEEPING_LOG = 2, // stuff we hate scrolling through  
    HB_GRANULAR_LOG     = 3  // sample-by-sample
} hb_debug_level_t;
void hb_deep_log( hb_debug_level_t level, char * log, ... );
void hb_error( char * fmt, ...);

int  hb_list_bytes( hb_list_t * );
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size );
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, uint64_t * pos );
void hb_list_empty( hb_list_t ** );

hb_title_t * hb_title_init( char * dvd, int index );
void         hb_title_close( hb_title_t ** );

void         hb_filter_close( hb_filter_object_t ** );

/***********************************************************************
 * hb.c
 **********************************************************************/
int  hb_get_pid( hb_handle_t * );
void hb_set_state( hb_handle_t *, hb_state_t * );

/***********************************************************************
 * fifo.c
 **********************************************************************/
struct hb_buffer_s
{
    int           size;
    int           alloc;
    uint8_t *     data;
    int           cur;

    int64_t       sequence;

    int           id;
    int64_t       start;
    int64_t       stop;
    int           new_chap;

#define HB_FRAME_IDR    0x01
#define HB_FRAME_I      0x02
#define HB_FRAME_AUDIO  0x04
#define HB_FRAME_P      0x10
#define HB_FRAME_B      0x20
#define HB_FRAME_BREF   0x40
#define HB_FRAME_KEY    0x0F
#define HB_FRAME_REF    0xF0
    uint8_t       frametype;
    uint16_t       flags;

    /* Holds the output PTS from x264, for use by b-frame offsets in muxmp4.c */
    int64_t     renderOffset;

    int           x;
    int           y;
    int           width;
    int           height;

    hb_buffer_t * sub;

    hb_buffer_t * next;
};

void hb_buffer_pool_init( void );
void hb_buffer_pool_free( void );

hb_buffer_t * hb_buffer_init( int size );
void          hb_buffer_realloc( hb_buffer_t *, int size );
void          hb_buffer_close( hb_buffer_t ** );
void          hb_buffer_copy_settings( hb_buffer_t * dst,
                                       const hb_buffer_t * src );

hb_fifo_t   * hb_fifo_init();
int           hb_fifo_size( hb_fifo_t * );
int           hb_fifo_is_full( hb_fifo_t * );
float         hb_fifo_percent_full( hb_fifo_t * f );
hb_buffer_t * hb_fifo_get( hb_fifo_t * );
hb_buffer_t * hb_fifo_see( hb_fifo_t * );
hb_buffer_t * hb_fifo_see2( hb_fifo_t * );
void          hb_fifo_push( hb_fifo_t *, hb_buffer_t * );
void          hb_fifo_push_head( hb_fifo_t *, hb_buffer_t * );
void          hb_fifo_close( hb_fifo_t ** );

// this routine gets a buffer for an uncompressed YUV420 video frame
// with dimensions width x height.
static inline hb_buffer_t * hb_video_buffer_init( int width, int height )
{
    // Y requires w x h bytes. U & V each require (w+1)/2 x
    // (h+1)/2 bytes (the "+1" is to round up). We shift rather
    // than divide by 2 since the compiler can't know these ints
    // are positive so it generates very expensive integer divides
    // if we do "/2". The code here matches the calculation for
    // PIX_FMT_YUV420P in ffmpeg's avpicture_fill() which is required
    // for most of HB's filters to work right.
    return hb_buffer_init( width * height + ( ( width+1 ) >> 1 ) *
                           ( ( height+1 ) >> 1 ) * 2 );
}

// this routine 'moves' data from src to dst by interchanging 'data',
// 'size' & 'alloc' between them and copying the rest of the fields
// from src to dst.
static inline void hb_buffer_swap_copy( hb_buffer_t *src, hb_buffer_t *dst )
{
    uint8_t *data  = dst->data;
    int      size  = dst->size;
    int      alloc = dst->alloc;

    *dst = *src;

    src->data  = data;
    src->size  = size;
    src->alloc = alloc;
}

/***********************************************************************
 * Threads: update.c, scan.c, work.c, reader.c, muxcommon.c
 **********************************************************************/
hb_thread_t * hb_update_init( int * build, char * version );
hb_thread_t * hb_scan_init( hb_handle_t *, const char * path,
                            int title_index, hb_list_t * list_title,
                            int preview_count, int store_previews );
hb_thread_t * hb_work_init( hb_list_t * jobs, int cpu_count,
                            volatile int * die, int * error, hb_job_t ** job );
hb_thread_t  * hb_reader_init( hb_job_t * );
hb_thread_t  * hb_muxer_init( hb_job_t * );
hb_work_object_t * hb_get_work( int );
hb_work_object_t * hb_codec_decoder( int );
hb_work_object_t * hb_codec_encoder( int );

/***********************************************************************
 * mpegdemux.c
 **********************************************************************/
typedef struct {
    int64_t last_scr;       /* unadjusted SCR from most recent pack */
    int     scr_changes;    /* number of SCR discontinuities */
    int     flaky_clock;    /* try to compensate for PCR drops */
    int     dts_drops;      /* number of drops because DTS too far from SCR */
} hb_psdemux_t;

typedef int (*hb_muxer_t)(hb_buffer_t *, hb_list_t *, hb_psdemux_t*);

int hb_demux_ps( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );
int hb_demux_ss( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );
int hb_demux_null( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );

extern const hb_muxer_t hb_demux[];

/***********************************************************************
 * decmetadata.c
 **********************************************************************/
extern void decmetadata( hb_title_t *title );

/***********************************************************************
 * dvd.c
 **********************************************************************/
typedef struct hb_dvd_s hb_dvd_t;
typedef struct hb_stream_s hb_stream_t;

hb_dvd_t *   hb_dvd_init( char * path );
int          hb_dvd_title_count( hb_dvd_t * );
hb_title_t * hb_dvd_title_scan( hb_dvd_t *, int title );
int          hb_dvd_start( hb_dvd_t *, int title, int chapter );
void         hb_dvd_stop( hb_dvd_t * );
int          hb_dvd_seek( hb_dvd_t *, float );
int          hb_dvd_read( hb_dvd_t *, hb_buffer_t * );
int          hb_dvd_chapter( hb_dvd_t * );
int          hb_dvd_is_break( hb_dvd_t * d );
void         hb_dvd_close( hb_dvd_t ** );

hb_stream_t * hb_stream_open( char * path, hb_title_t *title );
void		 hb_stream_close( hb_stream_t ** );
hb_title_t * hb_stream_title_scan( hb_stream_t *);
int          hb_stream_read( hb_stream_t *, hb_buffer_t *);
int          hb_stream_seek( hb_stream_t *, float );

void       * hb_ffmpeg_context( int codec_param );
void       * hb_ffmpeg_avstream( int codec_param );

/***********************************************************************
 * Work objects
 **********************************************************************/
#define HB_CONFIG_MAX_SIZE 8192
union hb_esconfig_u
{

    struct
    {
        uint8_t bytes[HB_CONFIG_MAX_SIZE];
        int     length;
    } mpeg4;

	struct
	{
	    uint8_t  sps[HB_CONFIG_MAX_SIZE];
	    int       sps_length;
	    uint8_t  pps[HB_CONFIG_MAX_SIZE];
	    int       pps_length;
        uint32_t init_delay;
	} h264;

    struct
    {
        uint8_t headers[3][HB_CONFIG_MAX_SIZE];
    } theora;

    struct
    {
        uint8_t bytes[HB_CONFIG_MAX_SIZE];
        int     length;
    } aac;

    struct
    {
        uint8_t headers[3][HB_CONFIG_MAX_SIZE];
        char *language;
    } vorbis;

    struct
    {
    	/* ac3flags stores the flags from the AC3 source, as found in scan.c */
    	int     ac3flags;
        // next two items are used by the bsinfo routine to accumulate small
        // frames until we have enough to validate the crc.
        int     len;        // space currently used in 'buf'
        uint8_t buf[HB_CONFIG_MAX_SIZE-sizeof(int)];
    } a52;

    struct
    {
    	/* dcaflags stores the flags from the DCA source, as found in scan.c */
    	int  dcaflags;
    } dca;

};

enum
{
    WORK_SYNC = 1,
    WORK_DECMPEG2,
    WORK_DECSUB,
    WORK_RENDER,
    WORK_ENCAVCODEC,
    WORK_ENCXVID,
    WORK_ENCX264,
    WORK_ENCTHEORA,
    WORK_DECA52,
    WORK_DECDCA,
    WORK_DECAVCODEC,
    WORK_DECAVCODECV,
    WORK_DECAVCODECVI,
    WORK_DECAVCODECAI,
    WORK_DECLPCM,
    WORK_ENCFAAC,
    WORK_ENCLAME,
    WORK_ENCVORBIS
};

enum
{
    FILTER_DEINTERLACE = 1,
    FILTER_DEBLOCK,
    FILTER_DENOISE,
    FILTER_DETELECINE,
    FILTER_DECOMB
};

extern hb_work_object_t * hb_objects;

#define HB_WORK_IDLE     0
#define HB_WORK_OK       1
#define HB_WORK_ERROR    2
#define HB_WORK_DONE     3

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
DECLARE_MUX( mkv );

