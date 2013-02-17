/* common.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
#ifndef HB_COMMON_H
#define HB_COMMON_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef USE_OPENCL
#include <CL/cl.h>
#endif
/*
 * It seems WinXP doesn't align the stack of new threads to 16 bytes.
 * To prevent crashes in SSE functions, we need to force stack alignement
 * of new threads.
 */
#if defined( __GNUC__ ) && (defined( _WIN32 ) || defined( __MINGW32__ ))
#    define attribute_align_thread __attribute__((force_align_arg_pointer))
#else
#    define attribute_align_thread
#endif

#if defined( __GNUC__ ) && !(defined( _WIN32 ) || defined( __MINGW32__ ))
#   define HB_WPRINTF(s,v) __attribute__((format(printf,s,v)))
#else
#   define HB_WPRINTF(s,v)
#endif

#if defined( SYS_MINGW )
#   define fseek fseeko64
#   define ftell ftello64
#   undef  fseeko
#   define fseeko fseeko64
#   undef  ftello
#   define ftello ftello64
#   define flockfile(...)
#   define funlockfile(...)
#   define getc_unlocked getc
#   undef  off_t
#   define off_t off64_t
#endif

#ifndef MIN
#define MIN( a, b ) ( (a) > (b) ? (b) : (a) )
#endif
#ifndef MAX
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )
#endif

#define EVEN( a )        ( (a) + ( (a) & 1 ) )
#define MULTIPLE_16( a ) ( 16 * ( ( (a) + 8 ) / 16 ) )
#define MULTIPLE_MOD( a, b ) ((b==1)?a:( b * ( ( (a) + (b / 2) - 1) / b ) ))
#define MULTIPLE_MOD_UP( a, b ) ((b==1)?a:( b * ( ( (a) + (b) - 1) / b ) ))
#define MULTIPLE_MOD_DOWN( a, b ) ((b==1)?a:( b * ( (a) / b ) ))

#define HB_DVD_READ_BUFFER_SIZE 2048

typedef struct hb_handle_s hb_handle_t;
typedef struct hb_list_s hb_list_t;
typedef struct hb_rate_s hb_rate_t;
typedef struct hb_mixdown_s hb_mixdown_t;
typedef struct hb_encoder_s hb_encoder_t;
typedef struct hb_job_s  hb_job_t;
typedef struct hb_title_set_s hb_title_set_t;
typedef struct hb_title_s hb_title_t;
typedef struct hb_chapter_s hb_chapter_t;
typedef struct hb_audio_s hb_audio_t;
typedef struct hb_audio_config_s hb_audio_config_t;
typedef struct hb_subtitle_s hb_subtitle_t;
typedef struct hb_subtitle_config_s hb_subtitle_config_t;
typedef struct hb_attachment_s hb_attachment_t;
typedef struct hb_metadata_s hb_metadata_t;
typedef struct hb_coverart_s hb_coverart_t;
typedef struct hb_state_s hb_state_t;
typedef union  hb_esconfig_u     hb_esconfig_t;
typedef struct hb_work_private_s hb_work_private_t;
typedef struct hb_work_object_s  hb_work_object_t;
typedef struct hb_filter_private_s hb_filter_private_t;
typedef struct hb_filter_object_s  hb_filter_object_t;
typedef struct hb_buffer_s hb_buffer_t;
typedef struct hb_fifo_s hb_fifo_t;
typedef struct hb_lock_s hb_lock_t;

#include "ports.h"
#ifdef __LIBHB__
#include "internal.h"
#define PRIVATE
#else
#define PRIVATE const
#endif
#include "audio_remap.h"
#include "libavutil/channel_layout.h"

hb_list_t * hb_list_init();
int         hb_list_count( const hb_list_t * );
void        hb_list_add( hb_list_t *, void * );
void        hb_list_insert( hb_list_t * l, int pos, void * p );
void        hb_list_rem( hb_list_t *, void * );
void      * hb_list_item( const hb_list_t *, int );
void        hb_list_close( hb_list_t ** );

void hb_reduce( int *x, int *y, int num, int den );
void hb_reduce64( int64_t *x, int64_t *y, int64_t num, int64_t den );
void hb_limit_rational64( int64_t *x, int64_t *y, int64_t num, int64_t den, int64_t limit );

#define HB_KEEP_WIDTH  0
#define HB_KEEP_HEIGHT 1
void hb_fix_aspect( hb_job_t * job, int keep );

void hb_job_set_advanced_opts( hb_job_t *job, const char *advanced_opts );
void hb_job_set_x264_preset( hb_job_t *job, const char *preset );
void hb_job_set_x264_tune( hb_job_t *job, const char *tune );
void hb_job_set_h264_profile( hb_job_t *job, const char *profile );
void hb_job_set_h264_level( hb_job_t *job, const char *level );
void hb_job_set_file( hb_job_t *job, const char *file );

hb_audio_t *hb_audio_copy(const hb_audio_t *src);
hb_list_t *hb_audio_list_copy(const hb_list_t *src);
void hb_audio_close(hb_audio_t **audio);
void hb_audio_config_init(hb_audio_config_t * audiocfg);
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg);
hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i);

hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src);
hb_list_t *hb_subtitle_list_copy(const hb_list_t *src);
void hb_subtitle_close( hb_subtitle_t **sub );
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track);
int hb_srt_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, 
               const char *lang);
int hb_subtitle_can_force( int source );
int hb_subtitle_can_burn( int source );
int hb_subtitle_can_pass( int source, int mux );

hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src);
int hb_gui_use_hwd_flag;

hb_list_t *hb_attachment_list_copy(const hb_list_t *src);
void hb_attachment_close(hb_attachment_t **attachment);

hb_metadata_t * hb_metadata_init();
hb_metadata_t * hb_metadata_copy(const hb_metadata_t *src);
void hb_metadata_close(hb_metadata_t **metadata);
void hb_metadata_set_name( hb_metadata_t *metadata, const char *name );
void hb_metadata_set_artist( hb_metadata_t *metadata, const char *artist );
void hb_metadata_set_composer( hb_metadata_t *metadata, const char *composer );
void hb_metadata_set_release_date( hb_metadata_t *metadata, const char *release_date );
void hb_metadata_set_comment( hb_metadata_t *metadata, const char *comment );
void hb_metadata_set_genre( hb_metadata_t *metadata, const char *genre );
void hb_metadata_set_album( hb_metadata_t *metadata, const char *album );
void hb_metadata_set_album_artist( hb_metadata_t *metadata, const char *album_artist );
void hb_metadata_set_description( hb_metadata_t *metadata, const char *description );
void hb_metadata_set_long_description( hb_metadata_t *metadata, const char *long_description );
void hb_metadata_add_coverart( hb_metadata_t *metadata, const uint8_t *data, int size, int type );
void hb_metadata_rem_coverart( hb_metadata_t *metadata, int ii );

hb_chapter_t *hb_chapter_copy(const hb_chapter_t *src);
hb_list_t *hb_chapter_list_copy(const hb_list_t *src);
void hb_chapter_close(hb_chapter_t **chapter);
void hb_chapter_set_title(hb_chapter_t *chapter, const char *title);

struct hb_rate_s
{
    const char *string;
    int         rate;
};

struct hb_mixdown_s
{
    const char *human_readable_name;
    const char *internal_name;
    const char *short_name;
    int         amixdown;
};

struct hb_encoder_s
{
    const char *human_readable_name; // note: used in presets
    const char *short_name;          // note: used in CLI
    int         encoder;             // HB_*CODEC_* define
    int         muxers;              // supported muxers
};

struct hb_subtitle_config_s
{
    enum subdest { RENDERSUB, PASSTHRUSUB } dest;
    int  force;
    int  default_track; 
    
    /* SRT subtitle tracks only */
    char src_filename[256];
    char src_codeset[40];
    int64_t offset;
};

#define HB_VIDEO_RATE_BASE   27000000

extern hb_rate_t    hb_video_rates[];
extern int          hb_video_rates_count;
extern hb_rate_t    hb_audio_rates[];
extern int          hb_audio_rates_count;
extern int          hb_audio_rates_default;
extern hb_rate_t    hb_audio_bitrates[];
extern int          hb_audio_bitrates_count;
extern hb_mixdown_t hb_audio_mixdowns[];
extern int          hb_audio_mixdowns_count;
extern hb_encoder_t hb_video_encoders[];
extern int          hb_video_encoders_count;
extern hb_encoder_t hb_audio_encoders[];
extern int          hb_audio_encoders_count;

/* Expose values for PInvoke */
hb_rate_t*    hb_get_video_rates();
int           hb_get_video_rates_count();
hb_rate_t*    hb_get_audio_rates();
int           hb_get_audio_rates_count();
int           hb_get_audio_rates_default();
hb_rate_t*    hb_get_audio_bitrates();
int           hb_get_audio_bitrates_count();
hb_mixdown_t* hb_get_audio_mixdowns();
int           hb_get_audio_mixdowns_count();
hb_encoder_t* hb_get_video_encoders();
int           hb_get_video_encoders_count();
hb_encoder_t* hb_get_audio_encoders();
int           hb_get_audio_encoders_count();

int         hb_mixdown_is_supported(int mixdown, uint32_t codec, uint64_t layout);
int         hb_mixdown_has_codec_support(int mixdown, uint32_t codec);
int         hb_mixdown_has_remix_support(int mixdown, uint64_t layout);
int         hb_mixdown_get_discrete_channel_count(int amixdown);
int         hb_mixdown_get_low_freq_channel_count(int amixdown);
int         hb_mixdown_get_mixdown_from_short_name(const char *short_name);
const char* hb_mixdown_get_short_name_from_mixdown(int amixdown);

void hb_autopassthru_apply_settings(hb_job_t *job);
void hb_autopassthru_print_settings(hb_job_t *job);
int  hb_autopassthru_get_encoder(int in_codec, int copy_mask, int fallback, int muxer);

int hb_get_default_audio_encoder(int muxer);

int hb_get_best_mixdown(uint32_t codec, uint64_t layout, int mixdown);
int hb_get_default_mixdown(uint32_t codec, uint64_t layout);

int hb_get_best_samplerate(uint32_t codec, int samplerate, int *sr_shift);

int  hb_find_closest_audio_bitrate(int bitrate);
void hb_get_audio_bitrate_limits(uint32_t codec, int samplerate, int mixdown, int *low, int *high);
int  hb_get_best_audio_bitrate(uint32_t codec, int bitrate, int samplerate, int mixdown);
int  hb_get_default_audio_bitrate(uint32_t codec, int samplerate, int mixdown);

void  hb_get_audio_quality_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);
float hb_get_best_audio_quality(uint32_t codec, float quality);
float hb_get_default_audio_quality(uint32_t codec);

void  hb_get_audio_compression_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);
float hb_get_best_audio_compression(uint32_t codec, float compression);
float hb_get_default_audio_compression(uint32_t codec);

struct hb_title_set_s
{
    hb_list_t   * list_title;
    int           feature;    // Detected DVD feature title
};

/******************************************************************************
 * hb_job_t: settings to be filled by the UI
 *****************************************************************************/
struct hb_job_s
{
    /* ID assigned by UI so it can groups job passes together */
    int             sequence_id;

    /* Pointer to the title to be ripped */
    hb_title_t    * title;
    int             feature; // Detected DVD feature title

    /* Chapter selection */
    int             chapter_start;
    int             chapter_end;

	/* Include chapter marker track in mp4? */
    int             chapter_markers;

    /* Picture settings:
         crop:                must be multiples of 2 (top/bottom/left/right)
         deinterlace:         0 or 1
         width:               must be a multiple of 2
         height:              must be a multiple of 2
         keep_ratio:          used by UIs
         grayscale:           black and white encoding
         pixel_ratio:         store pixel aspect ratio in the video
         pixel_aspect_width:  numerator for pixel aspect ratio
         pixel_aspect_height: denominator for pixel aspect ratio
         modulus:             set a number for dimensions to be multiples of
         maxWidth:            keep width below this
         maxHeight:           keep height below this */
    int             crop[4];
    int             deinterlace;
    hb_list_t     * list_filter;
    int             width;
    int             height;
    int             keep_ratio;
    int             grayscale;

    struct
    {
        int             mode;
        int             itu_par;
        int             par_width;
        int             par_height;
        int             dar_width;  // 0 if normal
        int             dar_height; // 0 if normal
        int             keep_display_aspect;
    } anamorphic;

    int             modulus;
    int             maxWidth;
    int             maxHeight;

    /* Video settings:
         vcodec:            output codec
         vquality:          output quality (0.0..1.0)
                            if < 0.0 or > 1.0, bitrate is used instead,
                            except with x264, to use its real QP/RF scale
         vbitrate:          output bitrate (kbps)
         vrate, vrate_base: output framerate is vrate / vrate_base
         cfr:               0 (vfr), 1 (cfr), 2 (pfr) [see render.c]
         pass:              0, 1 or 2 (or -1 for scan)
         advanced_opts:     string of extra advanced encoder options
         areBframes:        boolean to note if b-frames are included in advanced_opts */
#define HB_VCODEC_MASK   0x0000FF
#define HB_VCODEC_X264   0x000001
#define HB_VCODEC_THEORA 0x000002
#define HB_VCODEC_FFMPEG_MPEG4 0x000010
#define HB_VCODEC_FFMPEG       HB_VCODEC_FFMPEG_MPEG4
#define HB_VCODEC_FFMPEG_MPEG2 0x000020
#define HB_VCODEC_FFMPEG_MASK  0x0000F0

    int             vcodec;
    float           vquality;
    int             vbitrate;
    int             pfr_vrate;
    int             pfr_vrate_base;
    int             vrate;
    int             vrate_base;
    int             cfr;
    int             pass;
    int             fastfirstpass;
    char            *x264_preset;
    char            *x264_tune;
    char            *advanced_opts;
    char            *h264_profile;
    char            *h264_level;
    int             areBframes;

    int             color_matrix_code;
    int             color_prim;
    int             color_transfer;
    int             color_matrix;
// see https://developer.apple.com/quicktime/icefloe/dispatch019.html#colr
#define HB_COLR_PRI_BT709     1
#define HB_COLR_PRI_UNDEF     2
#define HB_COLR_PRI_EBUTECH   5 // use for bt470bg
#define HB_COLR_PRI_SMPTEC    6 // smpte170m; also use for bt470m and smpte240m
// 0, 3-4, 7-65535: reserved
#define HB_COLR_TRA_BT709     1 // also use for bt470m, bt470bg and smpte170m
#define HB_COLR_TRA_UNDEF     2
#define HB_COLR_TRA_SMPTE240M 7
// 0, 3-6, 8-65535: reserved
#define HB_COLR_MAT_BT709     1
#define HB_COLR_MAT_UNDEF     2
#define HB_COLR_MAT_SMPTE170M 6 // also use for fcc and bt470bg
#define HB_COLR_MAT_SMPTE240M 7
// 0, 3-5, 8-65535: reserved

    hb_list_t     * list_chapter;

    /* List of audio settings. */
    hb_list_t     * list_audio;
    int             acodec_copy_mask; // Auto Passthru allowed codecs
    int             acodec_fallback;  // Auto Passthru fallback encoder

    /* Subtitles */
    hb_list_t     * list_subtitle;

    hb_list_t     * list_attachment;

    hb_metadata_t * metadata;

    /* Muxer settings
         mux:  output file format
         file: file path */
#define HB_MUX_MASK 0xFF0000
#define HB_MUX_MP4  0x010000
#define HB_MUX_MKV  0x200000

    int             mux;
    char          * file;

    /* Allow MP4 files > 4 gigs */
    int             largeFileSize;
    int             mp4_optimize;
    int             ipod_atom;

    int                     indepth_scan;
    hb_subtitle_config_t    select_subtitle_config;

    int             angle;              // dvd angle to encode
    int             frame_to_start;     // declare eof when we hit this frame
    int64_t         pts_to_start;       // drop frames until  we pass this pts 
                                        //  in the time-linearized input stream
    int             frame_to_stop;      // declare eof when we hit this frame
    int64_t         pts_to_stop;        // declare eof when we pass this pts in
                                        //  the time-linearized input stream
    int             start_at_preview;   // if non-zero, encoding will start
                                        //  at the position of preview n
    int             seek_points;        //  out of N previews
    uint32_t        frames_to_skip;     // decode but discard this many frames
                                        //  initially (for frame accurate positioning
                                        //  to non-I frames).
    int use_opencl;/* 0 is disable use of opencl. 1 is enable use of opencl */
    int use_hwd;

#ifdef __LIBHB__
    /* Internal data */
    hb_handle_t   * h;
    hb_lock_t     * pause;
    volatile int  * die;
    volatile int    done;

    uint64_t        st_pause_date;
    uint64_t        st_paused;

    hb_fifo_t     * fifo_mpeg2;   /* MPEG-2 video ES */
    hb_fifo_t     * fifo_raw;     /* Raw pictures */
    hb_fifo_t     * fifo_sync;    /* Raw pictures, framerate corrected */
    hb_fifo_t     * fifo_render;  /* Raw pictures, scaled */
    hb_fifo_t     * fifo_mpeg4;   /* MPEG-4 video ES */

    hb_list_t     * list_work;

    hb_esconfig_t config;

    hb_mux_data_t * mux_data;
#endif
};

/* Audio starts here */
/* Audio Codecs */
#define HB_ACODEC_MASK      0x003FFF00
#define HB_ACODEC_FAAC      0x00000100
#define HB_ACODEC_LAME      0x00000200
#define HB_ACODEC_VORBIS    0x00000400
#define HB_ACODEC_AC3       0x00000800
#define HB_ACODEC_LPCM      0x00001000
#define HB_ACODEC_DCA       0x00002000
#define HB_ACODEC_CA_AAC    0x00004000
#define HB_ACODEC_CA_HAAC   0x00008000
#define HB_ACODEC_FFAAC     0x00010000
#define HB_ACODEC_FFMPEG    0x00020000
#define HB_ACODEC_DCA_HD    0x00040000
#define HB_ACODEC_MP3       0x00080000
#define HB_ACODEC_FFFLAC    0x00100000
#define HB_ACODEC_FFFLAC24  0x00200000
#define HB_ACODEC_FF_MASK   0x003F2000
#define HB_ACODEC_PASS_FLAG 0x40000000
#define HB_ACODEC_PASS_MASK (HB_ACODEC_MP3 | HB_ACODEC_FFAAC | HB_ACODEC_DCA_HD | HB_ACODEC_AC3 | HB_ACODEC_DCA)
#define HB_ACODEC_AUTO_PASS (HB_ACODEC_PASS_MASK | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_MP3_PASS  (HB_ACODEC_MP3 | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_AAC_PASS  (HB_ACODEC_FFAAC | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_AC3_PASS  (HB_ACODEC_AC3 | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_DCA_PASS  (HB_ACODEC_DCA | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_DCA_HD_PASS  (HB_ACODEC_DCA_HD | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_ANY       (HB_ACODEC_MASK | HB_ACODEC_PASS_FLAG)

#define HB_SUBSTREAM_BD_TRUEHD  0x72
#define HB_SUBSTREAM_BD_AC3     0x76
#define HB_SUBSTREAM_BD_DTSHD   0x72
#define HB_SUBSTREAM_BD_DTS     0x71

/* define an invalid VBR quality compatible with all VBR-capable codecs */
#define HB_INVALID_AUDIO_QUALITY (-3.)

struct hb_audio_config_s
{
    /* Output */
    struct
    {
        enum
        {
            // make sure audio->config.out.mixdown isn't treated as unsigned
            HB_INVALID_AMIXDOWN = -1,
            HB_AMIXDOWN_NONE = 0,
            HB_AMIXDOWN_MONO,
            HB_AMIXDOWN_LEFT,
            HB_AMIXDOWN_RIGHT,
            HB_AMIXDOWN_STEREO,
            HB_AMIXDOWN_DOLBY,
            HB_AMIXDOWN_DOLBYPLII,
            HB_AMIXDOWN_5POINT1,
            HB_AMIXDOWN_6POINT1,
            HB_AMIXDOWN_7POINT1,
            HB_AMIXDOWN_5_2_LFE,
        } mixdown; /* Audio mixdown */
        int      track; /* Output track number */
        uint32_t codec; /* Output audio codec */
        int      samplerate; /* Output sample rate (Hz) */
        int      samples_per_frame; /* Number of samples per frame */
        int      bitrate; /* Output bitrate (Kbps) */
        float    quality; /* Output quality (encoder-specific) */
        float    compression_level;  /* Output compression level (encoder-specific) */
        double   dynamic_range_compression; /* Amount of DRC applied to this track */
        double   gain; /* Gain (in dB), negative is quieter */
        int      normalize_mix_level; /* mix level normalization (boolean) */
        char *   name; /* Output track name */
    } out;

    /* Input */
    struct
    {
        int track; /* Input track number */
        PRIVATE uint32_t codec; /* Input audio codec */
        PRIVATE uint32_t codec_param; /* Per-codec config info */
        PRIVATE uint32_t reg_desc; /* Registration descriptor of source */
        PRIVATE uint32_t stream_type; /* Stream type from source stream */
        PRIVATE uint32_t substream_type; /* Substream type for multiplexed streams */
        PRIVATE uint32_t version; /* Bitsream version */
        PRIVATE uint32_t flags; /* Bitstream flags, codec-specific */
        PRIVATE uint32_t mode; /* Bitstream mode, codec-specific */
        PRIVATE int samplerate; /* Input sample rate (Hz) */
        PRIVATE int samples_per_frame; /* Number of samples per frame */
        PRIVATE int bitrate; /* Input bitrate (bps) */
        PRIVATE uint64_t channel_layout; /* Source channel layout, set by the audio decoder */
        PRIVATE hb_chan_map_t * channel_map; /* Source channel map, set by the audio decoder */
    } in;

    struct
    {
        PRIVATE char description[1024];
        PRIVATE char simple[1024];
        PRIVATE char iso639_2[4];
        PRIVATE uint8_t type; /* normal, visually impaired, director's commentary */
    } lang;
};

#ifdef __LIBHB__
struct hb_audio_s
{
    int id;

    hb_audio_config_t config;

    struct {
        hb_fifo_t * fifo_in;   /* AC3/MPEG/LPCM ES */
        hb_fifo_t * fifo_raw;  /* Raw audio */
        hb_fifo_t * fifo_sync; /* Resampled, synced raw audio */
        hb_fifo_t * fifo_out;  /* MP3/AAC/Vorbis ES */

        hb_esconfig_t config;
        hb_mux_data_t * mux_data;
        hb_fifo_t     * scan_cache;
    } priv;
};
#endif

struct hb_chapter_s
{
    int      index;
    int      pgcn;
    int      pgn;
    int      cell_start;
    int      cell_end;
    uint64_t block_start;
    uint64_t block_end;
    uint64_t block_count;

    /* Visual-friendly duration */
    int      hours;
    int      minutes;
    int      seconds;

    /* Exact duration (in 1/90000s) */
    uint64_t duration;

    /* Optional chapter title */
    char     *title;
};

/*
 * A subtitle track.
 * 
 * Required fields when a demuxer creates a subtitle track are:
 * > id
 *     - ID of this track
 *     - must be unique for all tracks within a single job,
 *       since it is used to look up the appropriate in-FIFO with GetFifoForId()
 * > format
 *     - format of the packets the subtitle decoder work-object sends to sub->fifo_raw
 *     - for passthru subtitles, is also the format of the final packets sent to sub->fifo_out
 *     - PICTURESUB for banded 8-bit YAUV pixels; see decvobsub.c documentation for more info
 *     - TEXTSUB for UTF-8 text marked up with <b>, <i>, or <u>
 *     - read by the muxers, and by the subtitle burn-in logic in the hb_sync_video work-object
 * > source
 *     - used to create the appropriate subtitle decoder work-object in do_job()
 * > config.dest
 *     - whether to render the subtitle on the video track (RENDERSUB) or 
 *       to pass it through its own subtitle track in the output container (PASSTHRUSUB)
 *     - all newly created non-VOBSUB tracks should default to PASSTHRUSUB
 *     - all newly created VOBSUB tracks should default to RENDERSUB, for legacy compatibility
 * > lang
 *     - user-readable description of the subtitle track
 *     - may correspond to the language of the track (see the 'iso639_2' field)
 *     - may correspond to the type of track (see the 'type' field; ex: "Closed Captions")
 * > iso639_2
 *     - language code for the subtitle, or "und" if unknown
 */
struct hb_subtitle_s
{
    int  id;
    int  track;
    int  out_track;

    hb_subtitle_config_t config;

    enum subtype { PICTURESUB, TEXTSUB } format;
    enum subsource { VOBSUB, SRTSUB, CC608SUB, /*unused*/CC708SUB, UTF8SUB, TX3GSUB, SSASUB, PGSSUB } source;
    char lang[1024];
    char iso639_2[4];
    uint8_t type; /* Closed Caption, Childrens, Directors etc */
    
    // Color lookup table for VOB subtitle tracks. Each entry is in YCbCr format.
    // Must be filled out by the demuxer for VOB subtitle tracks.
    uint32_t    palette[16];
    uint8_t     palette_set;
    int         width;
    int         height;
    
    // Codec private data for subtitles originating from FFMPEG sources
    uint8_t *   extradata;
    int         extradata_size;

    int hits;     /* How many hits/occurrences of this subtitle */
    int forced_hits; /* How many forced hits in this subtitle */

#ifdef __LIBHB__
    /* Internal data */
    PRIVATE uint32_t codec;         /* Input "codec" */
    PRIVATE uint32_t reg_desc;      /* registration descriptor of source */
    PRIVATE uint32_t stream_type;   /* stream type from source stream */
    PRIVATE uint32_t substream_type;/* substream for multiplexed streams */

    hb_fifo_t * fifo_in;  /* SPU ES */
    hb_fifo_t * fifo_raw; /* Decoded SPU */
    hb_fifo_t * fifo_sync;/* Synced */
    hb_fifo_t * fifo_out; /* Correct Timestamps, ready to be muxed */
    hb_mux_data_t * mux_data;
#endif
};

/*
 * An attachment.
 * 
 * These are usually used for attaching embedded fonts to movies containing SSA subtitles.
 */
struct hb_attachment_s
{
    enum attachtype { FONT_TTF_ATTACH, HB_ART_ATTACH } type;
    char *  name;
    char *  data;
    int     size;
};

struct hb_coverart_s
{
    uint8_t *data;
    uint32_t size;
    enum arttype {
        HB_ART_UNDEFINED,
        HB_ART_BMP,
        HB_ART_GIF,
        HB_ART_PNG,
        HB_ART_JPEG
    } type;
};

struct hb_metadata_s 
{
    char  *name;
    char  *artist;          // Actors
    char  *composer;
    char  *release_date;
    char  *comment;
    char  *album;           // DVD
    char  *album_artist;    // Director
    char  *genre;
    char  *description;
    char  *long_description;
    hb_list_t * list_coverart;
};

struct hb_title_s
{
    enum { HB_DVD_TYPE, HB_BD_TYPE, HB_STREAM_TYPE, HB_FF_STREAM_TYPE } type;
    uint32_t    reg_desc;
    char        path[1024];
    char        name[1024];
    int         index;
    int         playlist;
    int         vts;
    int         ttn;
    int         cell_start;
    int         cell_end;
    uint64_t    block_start;
    uint64_t    block_end;
    uint64_t    block_count;
    int         angle_count;
    void        *opaque_priv;

    /* Visual-friendly duration */
    int         hours;
    int         minutes;
    int         seconds;

    /* Exact duration (in 1/90000s) */
    uint64_t    duration;

    double      aspect;             // aspect ratio for the title's video
    double      container_aspect;   // aspect ratio from container (0 if none)
    int         has_resolution_change;
    int         width;
    int         height;
    int         pixel_aspect_width;
    int         pixel_aspect_height;
    int         color_prim;
    int         color_transfer;
    int         color_matrix;
    int         rate;
    int         rate_base;
    int         crop[4];
    enum { HB_DVD_DEMUXER, HB_MPEG_DEMUXER, HB_NULL_DEMUXER } demuxer;
    int         detected_interlacing;
    int         pcr_pid;                /* PCR PID for TS streams */
    int         video_id;               /* demuxer stream id for video */
    int         video_codec;            /* worker object id of video codec */
    uint32_t    video_stream_type;      /* stream type from source stream */
    int         video_codec_param;      /* codec specific config */
    char        *video_codec_name;
    int         video_bitrate;
    char        *container_name;
    int         data_rate;

    hb_metadata_t *metadata;

    hb_list_t * list_chapter;
    hb_list_t * list_audio;
    hb_list_t * list_subtitle;
    hb_list_t * list_attachment;

#define HB_TITLE_JOBS
#if defined(HB_TITLE_JOBS)
    hb_job_t  * job;
#endif

    uint32_t    flags;
                // set if video stream doesn't have IDR frames
    int         opencl_support;
    int         hwd_support;
#define         HBTF_NO_IDR (1 << 0)
#define         HBTF_SCAN_COMPLETE (1 << 0)
};


struct hb_state_s
{
#define HB_STATE_IDLE     1
#define HB_STATE_SCANNING 2
#define HB_STATE_SCANDONE 4
#define HB_STATE_WORKING  8
#define HB_STATE_PAUSED   16
#define HB_STATE_WORKDONE 32
#define HB_STATE_MUXING   64
#define HB_STATE_SEARCHING 128
    int state;

    union
    {
        struct
        {
            /* HB_STATE_SCANNING */
            int title_cur;
            int title_count;
        } scanning;

        struct
        {
            /* HB_STATE_WORKING */
            float progress;
            int   job_cur;
            int   job_count;
            float rate_cur;
            float rate_avg;
            int   hours;
            int   minutes;
            int   seconds;
            int   sequence_id;
        } working;

        struct
        {
            /* HB_STATE_WORKDONE */
#define HB_ERROR_NONE     0
#define HB_ERROR_CANCELED 1
#define HB_ERROR_UNKNOWN  2
            int error;
        } workdone;

        struct
        {
            /* HB_STATE_MUXING */
            float progress;
        } muxing;
    } param;
};

typedef struct hb_work_info_s
{
    const char * name;
    int          profile;
    int          level;
    int          bitrate;
    int          rate;
    int          rate_base;
    uint32_t     version;
    uint32_t     flags;
    uint32_t     mode;
    union
    {
        struct
        {    // info only valid for video decoders
            int width;
            int height;
            int pixel_aspect_width;
            int pixel_aspect_height;
            int color_prim;
            int color_transfer;
            int color_matrix;
        };
        struct
        {    // info only valid for audio decoders
            uint64_t channel_layout;
            hb_chan_map_t * channel_map;
            int samples_per_frame;
        };
    };
} hb_work_info_t;

struct hb_work_object_s
{
    int                 id;
    char              * name;

#ifdef __LIBHB__
    int              (* init)  ( hb_work_object_t *, hb_job_t * );
    int              (* work)  ( hb_work_object_t *, hb_buffer_t **,
                                 hb_buffer_t ** );
    void             (* close) ( hb_work_object_t * );
    /* the info entry point is used by scan to get bitstream information
     * during a decode (i.e., it should only be called after at least one
     * call to the 'work' entry point). currently it's only called for
     * video streams & can be null for other work objects. */
    int              (* info)  ( hb_work_object_t *, hb_work_info_t * );
    /* the bitstream info entry point is used by scan to get bitstream
     * information from a buffer. it doesn't have to be called during a
     * decode (it can be called even if init & work haven't been).
     * currently it's only called for audio streams & can be null for
     * other work objects. */
    int              (* bsinfo)  ( hb_work_object_t *, const hb_buffer_t *, 
                                   hb_work_info_t * );
    void             (* flush)   ( hb_work_object_t * );

    hb_fifo_t         * fifo_in;
    hb_fifo_t         * fifo_out;
    hb_esconfig_t     * config;

    /* Pointer hb_audio_t so we have access to the info in the audio worker threads. */
    hb_audio_t        * audio;

    /* Pointer hb_subtitle_t so we have access to the info in the subtitle worker threads. */
    hb_subtitle_t     * subtitle;

    hb_work_private_t * private_data;

    hb_thread_t       * thread;
    volatile int      * done;
    int                 status;
    int                 codec_param;
    hb_title_t        * title;

    hb_work_object_t  * next;
    int                 thread_sleep_interval;
#endif
};

extern hb_work_object_t hb_sync_video;
extern hb_work_object_t hb_sync_audio;
extern hb_work_object_t hb_decmpeg2;
extern hb_work_object_t hb_decvobsub;
extern hb_work_object_t hb_encvobsub;
extern hb_work_object_t hb_deccc608;
extern hb_work_object_t hb_decsrtsub;
extern hb_work_object_t hb_decutf8sub;
extern hb_work_object_t hb_dectx3gsub;
extern hb_work_object_t hb_decssasub;
extern hb_work_object_t hb_decpgssub;
extern hb_work_object_t hb_encavcodec;
extern hb_work_object_t hb_encx264;
extern hb_work_object_t hb_enctheora;
extern hb_work_object_t hb_deca52;
extern hb_work_object_t hb_decdca;
extern hb_work_object_t hb_decavcodeca;
extern hb_work_object_t hb_decavcodecv;
extern hb_work_object_t hb_declpcm;
extern hb_work_object_t hb_encfaac;
extern hb_work_object_t hb_enclame;
extern hb_work_object_t hb_encvorbis;
extern hb_work_object_t hb_muxer;
extern hb_work_object_t hb_encca_aac;
extern hb_work_object_t hb_encca_haac;
extern hb_work_object_t hb_encavcodeca;
extern hb_work_object_t hb_reader;

#define HB_FILTER_OK      0
#define HB_FILTER_DELAY   1
#define HB_FILTER_FAILED  2
#define HB_FILTER_DROP    3
#define HB_FILTER_DONE    4

typedef struct hb_oclscale_s
{
#ifdef USE_OPENCL
    // input buffer for running horizontal kernel. output buffer of running horizontal kernel. outpuf buffer of running vertiacla kernel
    cl_mem h_in_buf;
    cl_mem h_out_buf;
    cl_mem v_out_buf;
    // horizontal coefficent buffer for Y U and V plane, hroizontal source index for Y,U and V plane
    cl_mem h_coeff_y;
    cl_mem h_coeff_uv;
    cl_mem h_index_y;
    cl_mem h_index_uv;
    // vertical coefficent buffer for Y U and V plane, vertical source index for Y,U and V plane
    cl_mem v_coeff_y; 
    cl_mem v_coeff_uv;
    cl_mem v_index_y;
    cl_mem v_index_uv;
    // horizontal scaling and vertical scaling kernel handle
    cl_kernel h_kernel;
    cl_kernel v_kernel;
    int use_ocl_mem; // 0 use host memory. 1 use gpu oclmem
#endif        
} hb_oclscale_t;

#ifdef USE_OPENCL
int hb_ocl_scale( cl_mem in_buf, uint8_t *in_data, uint8_t *out_data, int in_w, int in_h, int out_w, int out_h, hb_oclscale_t  *os );
#endif

#ifdef USE_OPENCL
int hb_use_dxva( hb_title_t * title );
// create opencl buffer
#define CREATEBUF( out, flags, size )\
    {\
        out = clCreateBuffer( kenv->context, (flags), (size), NULL, &status );\
        if( CL_SUCCESS != status ) return -1;\
    }

#define OCLCHECK( method, ... )\
    status = method( __VA_ARGS__ ); if( status != CL_SUCCESS ) {\
        printf( # method " error '%d'\n", status ); return status; }

#define CL_FREE( buf )\
{\
    if( buf )\
    {\
        { clReleaseMemObject( buf ); }\
        buf = NULL;\
    }\
}


#endif

typedef struct hb_filter_init_s
{
    hb_job_t    * job;
    int           pix_fmt;
    int           width;
    int           height;
    int           par_width;
    int           par_height;
    int           crop[4];
    int           vrate_base;
    int           vrate;
    int           pfr_vrate_base;
    int           pfr_vrate;
    int           cfr;
#ifdef USE_OPENCL
 	int           use_dxva;
	int           title_width;
	int           title_height;
#endif    

} hb_filter_init_t;

typedef struct hb_filter_info_s
{
    char               human_readable_desc[128];
    hb_filter_init_t   out;
} hb_filter_info_t;

struct hb_filter_object_s
{
    int                     id;
    int                     enforce_order;
    char                  * name;
    char                  * settings;

#ifdef __LIBHB__
    int         (* init)  ( hb_filter_object_t *, hb_filter_init_t * );

    int         (* work)  ( hb_filter_object_t *,
                            hb_buffer_t **, hb_buffer_t ** );

    void        (* close) ( hb_filter_object_t * );
    int         (* info)  ( hb_filter_object_t *, hb_filter_info_t * );

    hb_fifo_t   * fifo_in;
    hb_fifo_t   * fifo_out;

    hb_subtitle_t     * subtitle;

    hb_filter_private_t * private_data;

    hb_thread_t       * thread;
    volatile int      * done;
    int                 status;

    // Filters can drop frames and thus chapter marks
    // These are used to bridge the chapter to the next buffer
    int                 chapter_val;
    int64_t             chapter_time;
#endif
};

enum
{
    // First, filters that may change the framerate (drop or dup frames)
    HB_FILTER_DETELECINE = 1,
    HB_FILTER_DECOMB,
    HB_FILTER_DEINTERLACE,
    HB_FILTER_VFR,
    // Filters that must operate on the original source image are next
    HB_FILTER_DEBLOCK,
    HB_FILTER_DENOISE,
    HB_FILTER_RENDER_SUB,
    HB_FILTER_CROP_SCALE,

    // Finally filters that don't care what order they are in,
    // except that they must be after the above filters
    HB_FILTER_ROTATE,
};

hb_filter_object_t * hb_filter_init( int filter_id );
hb_filter_object_t * hb_filter_copy( hb_filter_object_t * filter );
hb_list_t *hb_filter_list_copy(const hb_list_t *src);
void hb_filter_close( hb_filter_object_t ** );

typedef void hb_error_handler_t( const char *errmsg );

extern void hb_register_error_handler( hb_error_handler_t * handler );

char * hb_strdup_printf(const char *fmt, ...) HB_WPRINTF(1, 2);
char * hb_strncat_dup( const char * s1, const char * s2, size_t n );

int hb_yuv2rgb(int yuv);
int hb_rgb2yuv(int rgb);

const char * hb_subsource_name( int source );

// unparse a set of x264 settings to an HB encopts string
char * hb_x264_param_unparse(const char *x264_preset,  const char *x264_tune,
                             const char *x264_encopts, const char *h264_profile,
                             const char *h264_level, int width, int height);

// x264 preset/tune & h264 profile/level helpers
const char * const * hb_x264_presets();
const char * const * hb_x264_tunes();
const char * const * hb_h264_profiles();
const char * const * hb_h264_levels();

// x264 option name/synonym helper
const char * hb_x264_encopt_name( const char * name );

#endif
