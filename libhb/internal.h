/* internal.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hbffmpeg.h"

/***********************************************************************
 * common.c
 **********************************************************************/
void hb_log( char * log, ... ) HB_WPRINTF(1,2);
extern int global_verbosity_level; // Global variable for hb_deep_log
typedef enum hb_debug_level_s
{
    HB_SUPPORT_LOG      = 1, // helpful in tech support
    HB_HOUSEKEEPING_LOG = 2, // stuff we hate scrolling through  
    HB_GRANULAR_LOG     = 3  // sample-by-sample
} hb_debug_level_t;
void hb_valog( hb_debug_level_t level, const char * prefix, const char * log, va_list args) HB_WPRINTF(3,0);
void hb_deep_log( hb_debug_level_t level, char * log, ... ) HB_WPRINTF(2,3);
void hb_error( char * fmt, ...) HB_WPRINTF(1,2);
void hb_hexdump( hb_debug_level_t level, const char * label, const uint8_t * data, int len );

int  hb_list_bytes( hb_list_t * );
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size );
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, uint64_t * pos );
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

/*
 * Holds a packet of data that is moving through the transcoding process.
 * 
 * May have metadata associated with it via extra fields
 * that are conditionally used depending on the type of packet.
 */
struct hb_buffer_s
{
    int           size;     // size of this packet
    int           alloc;    // used internally by the packet allocator (hb_buffer_init)
    uint8_t *     data;     // packet data
    int           offset;   // used internally by packet lists (hb_list_t)

    /*
     * Corresponds to the order that this packet was read from the demuxer.
     * 
     * It is important that video decoder work-objects pass this value through
     * from their input packets to the output packets they generate. Otherwise
     * RENDERSUB subtitles (especially VOB subtitles) will break.
     * 
     * Subtitle decoder work-objects that output a renderable subtitle
     * format (ex: PICTURESUB) must also be careful to pass the sequence number
     * through for the same reason.
     */
    int64_t       sequence;

    struct settings
    {
        enum { AUDIO_BUF, VIDEO_BUF, SUBTITLE_BUF, FRAME_BUF, OTHER_BUF } type;

        int           id;           // ID of the track that the packet comes from
        int64_t       start;        // start time of frame
        double        duration;     // Actual duration, may be fractional ticks
        int64_t       stop;         // stop time of frame
        int64_t       renderOffset; // DTS used by b-frame offsets in muxmp4
        int64_t       pcr;
        uint8_t       discontinuity;
        int           new_chap;     // Video packets: if non-zero, is the index of the chapter whose boundary was crossed

    #define HB_FRAME_IDR    0x01
    #define HB_FRAME_I      0x02
    #define HB_FRAME_AUDIO  0x04
    #define HB_FRAME_P      0x10
    #define HB_FRAME_B      0x20
    #define HB_FRAME_BREF   0x40
    #define HB_FRAME_KEY    0x0F
    #define HB_FRAME_REF    0xF0
        uint8_t       frametype;
        uint16_t      flags;
    } s;

    struct format
    {
        int           x;
        int           y;
        int           width;
        int           height;
        int           fmt;
    } f;

    struct plane
    {
        uint8_t     * data;
        int           stride;
        int           width;
        int           height;
        int           height_stride;
        int           size;
    } plane[4]; // 3 Color components + alpha

    // PICTURESUB subtitle packets:

    // Video packets (after processing by the hb_sync_video work-object):
    //   A (copy of a) PICTURESUB subtitle packet that needs to be burned into 
    //   this video packet by the vobsub renderer filter
    //
    //   Subtitles that are simply passed thru are NOT attached to the 
    //   associated video packets.
    hb_buffer_t * sub;

    // Packets in a list:
    //   the next packet in the list
    hb_buffer_t * next;
};

void hb_buffer_pool_init( void );
void hb_buffer_pool_free( void );

hb_buffer_t * hb_buffer_init( int size );
hb_buffer_t * hb_frame_buffer_init( int pix_fmt, int w, int h);
void          hb_buffer_init_planes( hb_buffer_t * b );
void          hb_buffer_realloc( hb_buffer_t *, int size );
void          hb_video_buffer_realloc( hb_buffer_t * b, int w, int h );
void          hb_buffer_reduce( hb_buffer_t * b, int size );
void          hb_buffer_close( hb_buffer_t ** );
hb_buffer_t * hb_buffer_dup( const hb_buffer_t * src );
int           hb_buffer_copy( hb_buffer_t * dst, const hb_buffer_t * src );
void          hb_buffer_swap_copy( hb_buffer_t *src, hb_buffer_t *dst );
void          hb_buffer_move_subs( hb_buffer_t * dst, hb_buffer_t * src );

hb_fifo_t   * hb_fifo_init( int capacity, int thresh );
int           hb_fifo_size( hb_fifo_t * );
int           hb_fifo_size_bytes( hb_fifo_t * );
int           hb_fifo_is_full( hb_fifo_t * );
float         hb_fifo_percent_full( hb_fifo_t * f );
hb_buffer_t * hb_fifo_get( hb_fifo_t * );
hb_buffer_t * hb_fifo_get_wait( hb_fifo_t * );
hb_buffer_t * hb_fifo_see( hb_fifo_t * );
hb_buffer_t * hb_fifo_see_wait( hb_fifo_t * );
hb_buffer_t * hb_fifo_see2( hb_fifo_t * );
void          hb_fifo_push( hb_fifo_t *, hb_buffer_t * );
void          hb_fifo_push_wait( hb_fifo_t *, hb_buffer_t * );
int           hb_fifo_full_wait( hb_fifo_t * f );
void          hb_fifo_push_head( hb_fifo_t *, hb_buffer_t * );
void          hb_fifo_push_list_element( hb_fifo_t *fifo, hb_buffer_t *buffer_list );
hb_buffer_t * hb_fifo_get_list_element( hb_fifo_t *fifo );
void          hb_fifo_close( hb_fifo_t ** );
void          hb_fifo_flush( hb_fifo_t * f );

static inline int hb_image_stride( int pix_fmt, int width, int plane )
{
    int linesize = av_image_get_linesize( pix_fmt, width, plane );

    // Make buffer SIMD friendly.
    // Decomb requires stride aligned to 32 bytes
    // TODO: eliminate extra buffer copies in decomb
    linesize = MULTIPLE_MOD_UP( linesize, 32 );
    return linesize;
}

static inline int hb_image_width(int pix_fmt, int width, int plane)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);

    if (desc != NULL && (plane == 1 || plane == 2))
    {
        // The wacky arithmatic assures rounding up.
        width = -((-width) >> desc->log2_chroma_w);
    }

    return width;
}

static inline int hb_image_height_stride(int pix_fmt, int height, int plane)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);

    // Decomb requires 6 extra lines and stride aligned to 32 bytes
    height = MULTIPLE_MOD_UP(height + 6, 32);
    if (desc != NULL && (plane == 1 || plane == 2))
    {
        height = height >> desc->log2_chroma_h;
    }

    return height;
}

static inline int hb_image_height(int pix_fmt, int height, int plane)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);

    if (desc != NULL && (plane == 1 || plane == 2))
    {
        // The wacky arithmatic assures rounding up.
        height = -((-height) >> desc->log2_chroma_h);
    }

    return height;
}

// this routine gets a buffer for an uncompressed YUV420 video frame
// with dimensions width x height.
static inline hb_buffer_t * hb_video_buffer_init( int width, int height )
{
    return hb_frame_buffer_init( AV_PIX_FMT_YUV420P, width, height );
}

/***********************************************************************
 * Threads: update.c, scan.c, work.c, reader.c, muxcommon.c
 **********************************************************************/
hb_thread_t * hb_update_init( int * build, char * version );
hb_thread_t * hb_scan_init( hb_handle_t *, volatile int * die, 
                            const char * path, int title_index, 
                            hb_title_set_t * title_set, int preview_count, 
                            int store_previews, uint64_t min_duration );
hb_thread_t * hb_work_init( hb_list_t * jobs,
                            volatile int * die, int * error, hb_job_t ** job );
void ReadLoop( void * _w );
hb_work_object_t * hb_muxer_init( hb_job_t * );
hb_work_object_t * hb_get_work( int );
hb_work_object_t * hb_codec_decoder( int );
hb_work_object_t * hb_codec_encoder( int );

/***********************************************************************
 * sync.c
 **********************************************************************/
hb_work_object_t * hb_sync_init( hb_job_t * job );

/***********************************************************************
 * mpegdemux.c
 **********************************************************************/
typedef struct {
    int64_t last_scr;       /* unadjusted SCR from most recent pack */
    int64_t scr_delta;
    int64_t last_pts;       /* last pts we saw */
    int     scr_changes;    /* number of SCR discontinuities */
    int     dts_drops;      /* number of drops because DTS too far from SCR */
    int     new_chap;
} hb_psdemux_t;

typedef void (*hb_muxer_t)(hb_buffer_t *, hb_list_t *, hb_psdemux_t*);

void hb_demux_ps( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );
void hb_demux_ts( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );
void hb_demux_null( hb_buffer_t * ps_buf, hb_list_t * es_list, hb_psdemux_t * );

extern const hb_muxer_t hb_demux[];

/***********************************************************************
 * decmetadata.c
 **********************************************************************/
extern int decmetadata( hb_title_t *title );

/***********************************************************************
 * batch.c
 **********************************************************************/
typedef struct hb_batch_s hb_batch_t;

hb_batch_t  * hb_batch_init( char * path );
void          hb_batch_close( hb_batch_t ** _d );
int           hb_batch_title_count( hb_batch_t * d );
hb_title_t  * hb_batch_title_scan( hb_batch_t * d, int t );

/***********************************************************************
 * dvd.c
 **********************************************************************/
typedef struct hb_bd_s hb_bd_t;
typedef union  hb_dvd_s hb_dvd_t;
typedef struct hb_stream_s hb_stream_t;

hb_dvd_t *   hb_dvd_init( char * path );
int          hb_dvd_title_count( hb_dvd_t * );
hb_title_t * hb_dvd_title_scan( hb_dvd_t *, int title, uint64_t min_duration );
int          hb_dvd_start( hb_dvd_t *, hb_title_t *title, int chapter );
void         hb_dvd_stop( hb_dvd_t * );
int          hb_dvd_seek( hb_dvd_t *, float );
hb_buffer_t * hb_dvd_read( hb_dvd_t * );
int          hb_dvd_chapter( hb_dvd_t * );
int          hb_dvd_is_break( hb_dvd_t * d );
void         hb_dvd_close( hb_dvd_t ** );
int          hb_dvd_angle_count( hb_dvd_t * d );
void         hb_dvd_set_angle( hb_dvd_t * d, int angle );
int          hb_dvd_main_feature( hb_dvd_t * d, hb_list_t * list_title );

hb_bd_t     * hb_bd_init( char * path );
int           hb_bd_title_count( hb_bd_t * d );
hb_title_t  * hb_bd_title_scan( hb_bd_t * d, int t, uint64_t min_duration );
int           hb_bd_start( hb_bd_t * d, hb_title_t *title );
void          hb_bd_stop( hb_bd_t * d );
int           hb_bd_seek( hb_bd_t * d, float f );
int           hb_bd_seek_pts( hb_bd_t * d, uint64_t pts );
int           hb_bd_seek_chapter( hb_bd_t * d, int chapter );
hb_buffer_t * hb_bd_read( hb_bd_t * d );
int           hb_bd_chapter( hb_bd_t * d );
void          hb_bd_close( hb_bd_t ** _d );
void          hb_bd_set_angle( hb_bd_t * d, int angle );
int           hb_bd_main_feature( hb_bd_t * d, hb_list_t * list_title );

hb_stream_t * hb_bd_stream_open( hb_title_t *title );
void hb_ts_stream_reset(hb_stream_t *stream);
hb_stream_t * hb_stream_open( char * path, hb_title_t *title, int scan );
void		 hb_stream_close( hb_stream_t ** );
hb_title_t * hb_stream_title_scan( hb_stream_t *, hb_title_t *);
hb_buffer_t * hb_stream_read( hb_stream_t * );
int          hb_stream_seek( hb_stream_t *, float );
int          hb_stream_seek_ts( hb_stream_t * stream, int64_t ts );
int          hb_stream_seek_chapter( hb_stream_t *, int );
int          hb_stream_chapter( hb_stream_t * );

hb_buffer_t * hb_ts_decode_pkt( hb_stream_t *stream, const uint8_t * pkt );


#define STR4_TO_UINT32(p) \
    ((((const uint8_t*)(p))[0] << 24) | \
     (((const uint8_t*)(p))[1] << 16) | \
     (((const uint8_t*)(p))[2] <<  8) | \
      ((const uint8_t*)(p))[3])

/***********************************************************************
 * Work objects
 **********************************************************************/
#define HB_CONFIG_MAX_SIZE (2*8192)
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
    } extradata;

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
    WORK_SYNC_VIDEO = 1,
    WORK_SYNC_AUDIO,
    WORK_DECMPEG2,
    WORK_DECCC608,
    WORK_DECVOBSUB,
    WORK_DECSRTSUB,
    WORK_DECUTF8SUB,
    WORK_DECTX3GSUB,
    WORK_DECSSASUB,
    WORK_ENCVOBSUB,
    WORK_RENDER,
    WORK_ENCAVCODEC,
    WORK_ENCX264,
    WORK_ENCTHEORA,
    WORK_DECA52,
    WORK_DECAVCODEC,
    WORK_DECAVCODECV,
    WORK_DECLPCM,
    WORK_ENCFAAC,
    WORK_ENCLAME,
    WORK_ENCVORBIS,
    WORK_ENC_CA_AAC,
    WORK_ENC_CA_HAAC,
    WORK_ENCAVCODEC_AUDIO,
    WORK_MUX,
    WORK_READER,
    WORK_DECPGSSUB,
    WORK_DECAVCODECVACCL
};

extern hb_filter_object_t hb_filter_detelecine;
extern hb_filter_object_t hb_filter_deinterlace;
extern hb_filter_object_t hb_filter_deblock;
extern hb_filter_object_t hb_filter_denoise;
extern hb_filter_object_t hb_filter_decomb;
extern hb_filter_object_t hb_filter_rotate;
extern hb_filter_object_t hb_filter_crop_scale;
extern hb_filter_object_t hb_filter_render_sub;
extern hb_filter_object_t hb_filter_vfr;

// Picture flags used by filters
#ifndef PIC_FLAG_REPEAT_FIRST_FIELD
#define PIC_FLAG_REPEAT_FIRST_FIELD 256
#endif
#ifndef PIC_FLAG_TOP_FIELD_FIRST
#define PIC_FLAG_TOP_FIELD_FIRST 8
#endif
#ifndef PIC_FLAG_PROGRESSIVE_FRAME
#define PIC_FLAG_PROGRESSIVE_FRAME 16
#endif

#define PIC_FLAG_REPEAT_FRAME 512

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

