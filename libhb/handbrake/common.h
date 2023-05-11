/* common.h

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_COMMON_H
#define HANDBRAKE_COMMON_H

#include "handbrake/project.h"
#include "handbrake/hbtypes.h"
#include "handbrake/hb_dict.h"
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

/*
 * It seems WinXP doesn't align the stack of new threads to 16 bytes.
 * To prevent crashes in SSE functions, we need to force stack alignment
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
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : (-(a)))
#endif

#define HB_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

#ifndef HB_DEBUG_ASSERT
#define HB_DEBUG_ASSERT(x, y) { if ((x)) { hb_error("ASSERT: %s", y); exit(1); } }
#endif

#define EVEN( a )               ((a) + ((a) & 1))
#define MULTIPLE_MOD(a, b)      (((b) * (int)(((a) + ((b) / 2)) / (b))))
#define MULTIPLE_MOD_UP(a, b)   (((b) * (int)(((a) + ((b) - 1)) / (b))))
#define MULTIPLE_MOD_DOWN(a, b) (((b) * (int)((a) / (b))))

#define HB_DVD_READ_BUFFER_SIZE 2048

#define HB_MIN_WIDTH    32
#define HB_MIN_HEIGHT   32
#define HB_MAX_WIDTH    20480
#define HB_MAX_HEIGHT   20480

typedef enum
{
     HB_ERROR_NONE         = 0,
     HB_ERROR_CANCELED     = 1,
     HB_ERROR_WRONG_INPUT  = 2,
     HB_ERROR_INIT         = 3,
     HB_ERROR_UNKNOWN      = 4,
     HB_ERROR_READ         = 5
} hb_error_code;

#include "ports.h"
#ifdef __LIBHB__
#include "internal.h"
#define PRIVATE
#else
#define PRIVATE const
#endif
#include "audio_remap.h"
#include "libavutil/channel_layout.h"

#if HB_PROJECT_FEATURE_QSV
#include "qsv_libav.h"
#endif

#ifdef __LIBHB__
struct hb_buffer_list_s
{
    hb_buffer_t *head;
    hb_buffer_t *tail;
    int count;
    int size;
};

void hb_buffer_list_append(hb_buffer_list_t *list, hb_buffer_t *buf);
void hb_buffer_list_prepend(hb_buffer_list_t *list, hb_buffer_t *buf);
hb_buffer_t* hb_buffer_list_head(hb_buffer_list_t *list);
hb_buffer_t* hb_buffer_list_rem_head(hb_buffer_list_t *list);
hb_buffer_t* hb_buffer_list_tail(hb_buffer_list_t *list);
hb_buffer_t* hb_buffer_list_rem_tail(hb_buffer_list_t *list);
hb_buffer_t* hb_buffer_list_rem(hb_buffer_list_t *list, hb_buffer_t * b);
hb_buffer_t* hb_buffer_list_clear(hb_buffer_list_t *list);
hb_buffer_t* hb_buffer_list_set(hb_buffer_list_t *list, hb_buffer_t *buf);
void hb_buffer_list_close(hb_buffer_list_t *list);
int hb_buffer_list_count(hb_buffer_list_t *list);
int hb_buffer_list_size(hb_buffer_list_t *list);
#endif // __LIBHB__

hb_list_t * hb_list_init(void);
int         hb_list_count( const hb_list_t * );
void        hb_list_add( hb_list_t *, void * );
void        hb_list_insert( hb_list_t * l, int pos, void * p );
void        hb_list_rem( hb_list_t *, void * );
void      * hb_list_item( const hb_list_t *, int );
void        hb_list_close( hb_list_t ** );

void hb_reduce( int *x, int *y, int num, int den );
void hb_limit_rational( int *x, int *y, int64_t num, int64_t den, int limit );
void hb_reduce64( int64_t *x, int64_t *y, int64_t num, int64_t den );
void hb_limit_rational64( int64_t *x, int64_t *y, int64_t num, int64_t den, int64_t limit );

void hb_job_set_encoder_preset (hb_job_t *job, const char *preset);
void hb_job_set_encoder_tune   (hb_job_t *job, const char *tune);
void hb_job_set_encoder_options(hb_job_t *job, const char *options);
void hb_job_set_encoder_profile(hb_job_t *job, const char *profile);
void hb_job_set_encoder_level  (hb_job_t *job, const char *level);
void hb_job_set_file           (hb_job_t *job, const char *file);

hb_audio_t *hb_audio_copy(const hb_audio_t *src);
hb_list_t *hb_audio_list_copy(const hb_list_t *src);
void hb_audio_close(hb_audio_t **audio);
void hb_audio_config_init(hb_audio_config_t * audiocfg);
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg);
hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i);

int hb_subtitle_add_ssa_header(hb_subtitle_t *subtitle, const char *font,
                               int fs, int width, int height);
hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src);
hb_list_t *hb_subtitle_list_copy(const hb_list_t *src);
void hb_subtitle_close( hb_subtitle_t **sub );
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track);
int hb_import_subtitle_add( const hb_job_t * job,
                const hb_subtitle_config_t * subtitlecfg,
                const char *lang_code, int source );
int hb_srt_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg,
               const char *lang);
int hb_subtitle_can_force( int source );
int hb_subtitle_can_burn( int source );
int hb_subtitle_can_pass( int source, int mux );

int hb_audio_can_apply_drc(uint32_t codec, uint32_t codec_param, int encoder);
int hb_audio_can_apply_drc2(hb_handle_t *h, int title_idx,
                            int audio_idx, int encoder);

hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src);

hb_list_t *hb_attachment_list_copy(const hb_list_t *src);
void hb_attachment_close(hb_attachment_t **attachment);

hb_metadata_t * hb_metadata_init(void);
hb_metadata_t * hb_metadata_copy(const hb_metadata_t *src);
void hb_metadata_close(hb_metadata_t **metadata);
void hb_update_meta_dict(hb_dict_t * dict, const char * key, const char * value);
const char * hb_lookup_meta_key(const char * mux_key);
void hb_metadata_add_coverart( hb_metadata_t *metadata, const uint8_t *data, int size, int type );
void hb_metadata_rem_coverart( hb_metadata_t *metadata, int ii );

hb_chapter_t *hb_chapter_copy(const hb_chapter_t *src);
hb_list_t *hb_chapter_list_copy(const hb_list_t *src);
void hb_chapter_close(hb_chapter_t **chapter);
void hb_chapter_set_title(hb_chapter_t *chapter, const char *title);

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_rate_s.cs when changing this struct
struct hb_rate_s
{
    const char *name;
    int         rate;
};

struct hb_dither_s
{
    const char *description;
    const char *short_name;
    int         method;
};

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_mixdown_s.cs when changing this struct
struct hb_mixdown_s
{
    const char *name;
    const char *short_name;
    int         amixdown;
};

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_encoder_s.cs when changing this struct
struct hb_encoder_s
{
    const char *name;       // note: used in presets
    const char *short_name; // note: used in CLI
    const char *long_name;  // used in log
    int         codec;      // HB_*CODEC_* define
    int         muxers;     // supported muxers
};

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_container_s.cs when changing this struct
struct hb_container_s
{
    const char *name;
    const char *short_name;
    const char *long_name;
    const char *default_extension;
    int         format;
};

struct hb_rational_s
{
    int num;
    int den;
};

static inline hb_rational_t hb_make_q(int num, int den)
{
    hb_rational_t r = { num, den };
    return r;
}

static inline double hb_q2d(hb_rational_t a){
    return a.num / (double) a.den;
}

struct hb_geometry_s
{
    int width;
    int height;
    hb_rational_t par;
};

struct hb_geometry_crop_s
{
    int crop[4];
    int pad[4];
    hb_geometry_t geometry;
};

// hb_geometry_settings_s 'keep' flags
#define HB_KEEP_WIDTH           0x01
#define HB_KEEP_HEIGHT          0x02
#define HB_KEEP_DISPLAY_ASPECT  0x04
#define HB_KEEP_DISPLAY_WIDTH   0x08
#define HB_KEEP_PAD             0x10

// hb_geometry_settings_s 'flags'
#define HB_GEO_SCALE_UP         0x01
#define HB_GEO_SCALE_BEST       0x02

struct hb_geometry_settings_s
{
    int mode;               // Anamorphic mode, see job struct anamorphic
    int keep;               // Specifies settings that shouldn't be changed
    int flags;              // Flags that affect calculations
    int itu_par;            // use dvd dimensions to determine PAR
    int modulus;            // pixel alignment for loose anamorphic
    int crop[4];            // Pixels cropped from source before scaling
    int pad[4];             // Pixels cropped from source before scaling
    int maxWidth;           // max destination storage width
    int maxHeight;          // max destination storage height
    int displayWidth;       // display width, used with !HB_KEEP_DISPLAY_ASPECT
    int displayHeight;      // display height, used with !HB_KEEP_DISPLAY_ASPECT
                            // Note that display size includes pad!
    hb_geometry_t geometry;
};

// Update win/CS/HandBrake.Interop/Interop/HbLib/hb_image_s.cs when changing this struct
struct hb_image_s
{
    int format;
    int max_plane;
    int width;
    int height;
    int color_prim;
    int color_transfer;
    int color_matrix;
    uint8_t *data;

    struct image_plane
    {
        uint8_t *data;
        int width;
        int height;
        int stride;
        int height_stride;
        int size;
    } plane[4];
};

void hb_image_close(hb_image_t **_image);

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_subtitle_config_s.cs when changing this struct
struct hb_subtitle_config_s
{
    enum subdest { RENDERSUB, PASSTHRUSUB } dest;
    int          force;
    int          default_track;
    const char * name;

    /* SRT subtitle tracks only */
    const char * src_filename;
    char         src_codeset[40];
    int64_t      offset;
};

struct hb_mastering_display_metadata_s
{
    hb_rational_t display_primaries[3][2];
    hb_rational_t white_point[2];
    hb_rational_t min_luminance;
    hb_rational_t max_luminance;
    int has_primaries;
    int has_luminance;
};

struct hb_content_light_metadata_s
{
    unsigned max_cll;
    unsigned max_fall;
};

struct hb_ambient_viewing_environment_metadata_s
{
    hb_rational_t ambient_illuminance;
    hb_rational_t ambient_light_x;
    hb_rational_t ambient_light_y;
};

struct hb_dovi_conf_s
{
    unsigned dv_version_major;
    unsigned dv_version_minor;
    unsigned dv_profile;
    unsigned dv_level;
    unsigned rpu_present_flag;
    unsigned el_present_flag;
    unsigned bl_present_flag;
    unsigned dv_bl_signal_compatibility_id;
};

/*******************************************************************************
 * Lists of rates, mixdowns, encoders etc.
 *******************************************************************************
 *
 * Use hb_*_get_next() to get the next list item (use NULL to get the first).
 *
 * Use hb_*_get_from_name() to get the value corresponding to a name.
 * The name can be either the short or full name.
 * Legacy names are sanitized to currently-supported values whenever possible.
 * Returns 0 or -1 if no value could be found.
 *
 * Use hb_*_get_name() and hb_*_get_short_name() to get the corresponding value.
 * Returns NULL if the value is invalid.
 *
 * Use hb_*_get_long_name() when the name is not descriptive enough for you.
 *
 * hb_*_sanitize_name() are convenience functions for use when dealing
 * with full names (e.g. to translate legacy values while loading a preset).
 *
 * Names are case-insensitive; libhb will ensure that the lists do not contain
 * more than one entry with the same name.
 *
 * Use hb_*_get_limits() to get the minimum/maximum for lists with numerically
 * ordered values.
 *
 * Use hb_*_get_best() to sanitize a value based on other relevant parameters.
 *
 * Use hb_*_get_default() to get the default based on other relevant parameters.
 *
 */

void hb_common_global_init(int);

int              hb_video_framerate_get_from_name(const char *name);
const char*      hb_video_framerate_get_name(int framerate);
const char*      hb_video_framerate_sanitize_name(const char *name);
void             hb_video_framerate_get_limits(int *low, int *high, int *clock);
const hb_rate_t* hb_video_framerate_get_next(const hb_rate_t *last);
int              hb_video_framerate_get_close(hb_rational_t *framerate,
                                              double thresh);

int              hb_audio_samplerate_is_supported(int samplerate,
                                                  uint32_t codec);
int              hb_audio_samplerate_find_closest(int samplerate,
                                                  uint32_t codec);
int              hb_audio_samplerate_get_sr_shift(int samplerate);
int              hb_audio_samplerate_get_from_name(const char *name);
const char*      hb_audio_samplerate_get_name(int samplerate);
const hb_rate_t* hb_audio_samplerate_get_next(const hb_rate_t *last);
const hb_rate_t* hb_audio_samplerate_get_next_for_codec(const hb_rate_t *last,
                                                        uint32_t codec);

int              hb_audio_bitrate_get_best(uint32_t codec, int bitrate, int samplerate, int mixdown);
int              hb_audio_bitrate_get_default(uint32_t codec, int samplerate, int mixdown);
void             hb_audio_bitrate_get_limits(uint32_t codec, int samplerate, int mixdown, int *low, int *high);
const hb_rate_t* hb_audio_bitrate_get_next(const hb_rate_t *last);

void        hb_video_quality_get_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);
const char* hb_video_quality_get_name(uint32_t codec);
int         hb_video_quality_is_supported(uint32_t codec);

int         hb_video_multipass_is_supported(uint32_t codec);

int                hb_video_encoder_is_supported(int encoder);
int                hb_video_encoder_get_count_of_analysis_passes(int encoder);
int                hb_video_encoder_pix_fmt_is_supported(int encoder, int pix_fmt, const char *profile);
int                hb_video_encoder_get_depth   (int encoder);
const char* const* hb_video_encoder_get_presets (int encoder);
const char* const* hb_video_encoder_get_tunes   (int encoder);
const char* const* hb_video_encoder_get_profiles(int encoder);
const char* const* hb_video_encoder_get_levels  (int encoder);
const int*         hb_video_encoder_get_pix_fmts(int encoder, const char *profile);

void  hb_audio_quality_get_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);
float hb_audio_quality_get_best(uint32_t codec, float quality);
float hb_audio_quality_get_default(uint32_t codec);

void  hb_audio_compression_get_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);
float hb_audio_compression_get_best(uint32_t codec, float compression);
float hb_audio_compression_get_default(uint32_t codec);

int                hb_audio_dither_get_default(void);
int                hb_audio_dither_get_default_method(void); // default method, if enabled && supported
int                hb_audio_dither_is_supported(uint32_t codec, int depth);
int                hb_audio_dither_get_from_name(const char *name);
const char*        hb_audio_dither_get_description(int method);
const hb_dither_t* hb_audio_dither_get_next(const hb_dither_t *last);

int                 hb_mixdown_is_supported(int mixdown, uint32_t codec, uint64_t layout);
int                 hb_mixdown_has_codec_support(int mixdown, uint32_t codec);
int                 hb_mixdown_has_remix_support(int mixdown, uint64_t layout);
int                 hb_mixdown_get_discrete_channel_count(int mixdown);
int                 hb_mixdown_get_low_freq_channel_count(int mixdown);
int                 hb_mixdown_get_best(uint32_t codec, uint64_t layout, int mixdown);
int                 hb_mixdown_get_default(uint32_t codec, uint64_t layout);
hb_mixdown_t*       hb_mixdown_get_from_mixdown(int mixdown);
int                 hb_mixdown_get_from_name(const char *name);
const char*         hb_mixdown_get_name(int mixdown);
const char*         hb_mixdown_get_short_name(int mixdown);
const char*         hb_mixdown_sanitize_name(const char *name);
const hb_mixdown_t* hb_mixdown_get_next(const hb_mixdown_t *last);

void                hb_layout_get_name(char * name, int size, int64_t layout);
int                 hb_layout_get_discrete_channel_count(int64_t layout);
int                 hb_layout_get_low_freq_channel_count(int64_t layout);

int                 hb_video_encoder_get_default(int muxer);
hb_encoder_t*       hb_video_encoder_get_from_codec(int codec);
int                 hb_video_encoder_get_from_name(const char *name);
const char*         hb_video_encoder_get_name(int encoder);
const char*         hb_video_encoder_get_short_name(int encoder);
const char*         hb_video_encoder_get_long_name(int encoder);
const char*         hb_video_encoder_sanitize_name(const char *name);
const hb_encoder_t* hb_video_encoder_get_next(const hb_encoder_t *last);

/*
 * hb_audio_encoder_get_fallback_for_passthru() will sanitize a passthru codec
 * to the matching audio encoder (if any is available).
 *
 * hb_audio_encoder_get_from_name(), hb_audio_encoder_sanitize_name() will
 * sanitize legacy encoder names, but won't convert passthru to an encoder.
 */
int                 hb_audio_encoder_get_fallback_for_passthru(int passthru);
int                 hb_audio_encoder_get_default(int muxer);
hb_encoder_t*       hb_audio_encoder_get_from_codec(int codec);
int                 hb_audio_encoder_get_from_name(const char *name);
const char*         hb_audio_encoder_get_name(int encoder);
const char*         hb_audio_encoder_get_short_name(int encoder);
const char*         hb_audio_encoder_get_long_name(int encoder);
const char*         hb_audio_encoder_sanitize_name(const char *name);
const hb_encoder_t* hb_audio_encoder_get_next(const hb_encoder_t *last);

const char*         hb_audio_decoder_get_name(int codec, int codec_param);

/*
 * Not typically used by the UIs
 * (set hb_job_t.acodec_copy_mask, hb_job_t.acodec_fallback instead).
 */
void hb_autopassthru_apply_settings(hb_job_t *job);
void hb_autopassthru_print_settings(hb_job_t *job);
int  hb_autopassthru_get_encoder(int in_codec, int copy_mask, int fallback, int muxer);

hb_container_t*       hb_container_get_from_format(int format);
int                   hb_container_get_from_name(const char *name);
int                   hb_container_get_from_extension(const char *extension); // not really a container name
const char*           hb_container_get_name(int format);
const char*           hb_container_get_short_name(int format);
const char*           hb_container_get_long_name(int format);
const char*           hb_container_get_default_extension(int format);
const char*           hb_container_sanitize_name(const char *name);
const hb_container_t* hb_container_get_next(const hb_container_t *last);

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_title_set_s.cs when changing this struct
struct hb_title_set_s
{
    hb_list_t   * list_title;
    int           feature;    // Detected DVD feature title
    const char  * path;
};

typedef enum
{
    HB_ANAMORPHIC_NONE,
    HB_ANAMORPHIC_STRICT,
    HB_ANAMORPHIC_LOOSE,
    HB_ANAMORPHIC_CUSTOM,
    HB_ANAMORPHIC_AUTO
} hb_anamorphic_mode_t;

/******************************************************************************
 * hb_job_t: settings to be filled by the UI
 * Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_job_s.cs when changing this struct
 *****************************************************************************/
struct hb_job_s
{
    const char  * json;   // JSON encoded job string

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

    // Video filters
    int             grayscale;      // Black and white encoding
    hb_list_t     * list_filter;

    PRIVATE int             crop[4];
    PRIVATE int             width;
    PRIVATE int             height;
    hb_rational_t           par;

    /* Video settings:
         vcodec:            output codec
         vquality:          output quality (if invalid, bitrate is used instead)
         vbitrate:          output bitrate (Kbps)
         vrate:             output framerate
         cfr:               0 (vfr), 1 (cfr), 2 (pfr) [see render.c]
         pass:              0, 1 or 2 (or -1 for scan)
         areBframes:        boolean to note if b-frames are used */

    // Changing the constant values requires changes in win/CS/HandBrake.Interop/Interop/HbLib/NativeConstants.cs for Windows GUI
#define HB_VCODEC_INVALID            0x00000000

#define HB_VCODEC_AV1_MASK           0x40000000
#define HB_VCODEC_H264_MASK          0x20000000
#define HB_VCODEC_H265_MASK          0x10000000

#define HB_VCODEC_SVT_AV1_MASK       0x00800000
#define HB_VCODEC_X264_MASK          0x00400000
#define HB_VCODEC_X265_MASK          0x00200000

#define HB_VCODEC_VT_MASK            0x00080000
#define HB_VCODEC_QSV_MASK           0x00040000
#define HB_VCODEC_FFMPEG_MASK        0x00010000

#define HB_VCODEC_THEORA             0x00000001

#define HB_VCODEC_X264_8BIT         (0x00000002 | HB_VCODEC_X264_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_X264              HB_VCODEC_X264_8BIT
#define HB_VCODEC_X264_10BIT        (0x00000003 | HB_VCODEC_X264_MASK | HB_VCODEC_H264_MASK)

#define HB_VCODEC_X265_8BIT         (0x00000004 | HB_VCODEC_X265_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_X265              HB_VCODEC_X265_8BIT
#define HB_VCODEC_X265_10BIT        (0x00000005 | HB_VCODEC_X265_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_X265_12BIT        (0x00000006 | HB_VCODEC_X265_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_X265_16BIT        (0x00000007 | HB_VCODEC_X265_MASK | HB_VCODEC_H265_MASK)

#define HB_VCODEC_FFMPEG_MPEG4      (0x00000008 | HB_VCODEC_FFMPEG_MASK)
#define HB_VCODEC_FFMPEG_MPEG2      (0x00000009 | HB_VCODEC_FFMPEG_MASK)

#define HB_VCODEC_FFMPEG_VP8        (0x0000000A | HB_VCODEC_FFMPEG_MASK)
#define HB_VCODEC_FFMPEG_VP9        (0x0000000B | HB_VCODEC_FFMPEG_MASK)
#define HB_VCODEC_FFMPEG_VP9_10BIT  (0x0000000C | HB_VCODEC_FFMPEG_MASK)

#define HB_VCODEC_FFMPEG_VCE_H264           (0x0000000D | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_FFMPEG_VCE_H265           (0x0000000E | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_FFMPEG_VCE_H265_10BIT     (0x0000000F | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H265_MASK)

#define HB_VCODEC_FFMPEG_MF_H264    (0x00000020 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_FFMPEG_MF_H265    (0x00000021 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H265_MASK)

#define HB_VCODEC_FFMPEG_NVENC_H264         (0x00000030 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_FFMPEG_NVENC_H265         (0x00000031 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_FFMPEG_NVENC_H265_10BIT   (0x00000032 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_FFMPEG_NVENC_AV1          (0x00000033 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_AV1_MASK)
#define HB_VCODEC_FFMPEG_NVENC_AV1_10BIT    (0x00000034 | HB_VCODEC_FFMPEG_MASK | HB_VCODEC_AV1_MASK)

#define HB_VCODEC_SVT_AV1_8BIT       (0x00000041 | HB_VCODEC_SVT_AV1_MASK | HB_VCODEC_AV1_MASK)
#define HB_VCODEC_SVT_AV1            HB_VCODEC_SVT_AV1_8BIT
#define HB_VCODEC_SVT_AV1_10BIT      (0x00000042 | HB_VCODEC_SVT_AV1_MASK | HB_VCODEC_AV1_MASK)

#define HB_VCODEC_VT_H264           (0x00000050 | HB_VCODEC_VT_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_VT_H265           (0x00000051 | HB_VCODEC_VT_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_VT_H265_10BIT     (0x00000052 | HB_VCODEC_VT_MASK | HB_VCODEC_H265_MASK)

#define HB_VCODEC_QSV_H264          (0x00000060 | HB_VCODEC_QSV_MASK | HB_VCODEC_H264_MASK)
#define HB_VCODEC_QSV_H265_8BIT     (0x00000061 | HB_VCODEC_QSV_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_QSV_H265_10BIT    (0x00000062 | HB_VCODEC_QSV_MASK | HB_VCODEC_H265_MASK)
#define HB_VCODEC_QSV_H265          HB_VCODEC_QSV_H265_8BIT

#define HB_VCODEC_QSV_AV1_8BIT      (0x00000070 | HB_VCODEC_QSV_MASK | HB_VCODEC_AV1_MASK)
#define HB_VCODEC_QSV_AV1_10BIT     (0x00000071 | HB_VCODEC_QSV_MASK | HB_VCODEC_AV1_MASK)
#define HB_VCODEC_QSV_AV1           HB_VCODEC_QSV_AV1_8BIT

/* define an invalid CQ value compatible with all CQ-capable codecs */
#define HB_INVALID_VIDEO_QUALITY (-1000.)

    int             vcodec;
    double          vquality;
    int             vbitrate;
    hb_rational_t   vrate;
    // Some parameters that depend on vrate (like keyint) can't change
    // between encoding passes. So orig_vrate is used to store the
    // 1st pass rate.
    hb_rational_t   orig_vrate;
    int             cfr;
    PRIVATE int     pass_id;
    int             multipass;        // Enable multi-pass encode. Boolean
    int             fastanalysispass;
    char           *encoder_preset;
    char           *encoder_tune;
    char           *encoder_options;
    char           *encoder_profile;
    char           *encoder_level;
    int             areBframes;

    // Pixel format from decoder to the end of the filters chain
    int             input_pix_fmt;
    // Pixel format from the end of filters chain to the encoder
    int             output_pix_fmt;
    int             color_prim;
    int             color_transfer;
    int             color_matrix;
    int             color_range;
    int             chroma_location;

    int             color_prim_override;
    int             color_transfer_override;
    int             color_matrix_override;
// see https://developer.apple.com/library/content/technotes/tn2162/_index.html
//     https://developer.apple.com/library/content/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-125526
//     libav pixfmt.h
#define HB_COLR_PRI_BT709        1
#define HB_COLR_PRI_UNDEF        2
#define HB_COLR_PRI_BT470M       4
#define HB_COLR_PRI_EBUTECH      5 // use for bt470bg
#define HB_COLR_PRI_SMPTEC       6 // smpte170m
#define HB_COLR_PRI_SMPTE240M    7
#define HB_COLR_PRI_FILM         8 // Illuminant C
#define HB_COLR_PRI_BT2020       9
#define HB_COLR_PRI_SMPTE428     10
#define HB_COLR_PRI_SMPTE431     11
#define HB_COLR_PRI_SMPTE432     12
#define HB_COLR_PRI_JEDEC_P22    22
// 0, 3-4, 7-8, 10-65535: reserved/not implemented
#define HB_COLR_TRA_BT709        1 // also use for bt470m, bt470bg, smpte170m, bt2020_10 and bt2020_12
#define HB_COLR_TRA_UNDEF        2
#define HB_COLR_TRA_GAMMA22      4
#define HB_COLR_TRA_GAMMA28      5
#define HB_COLR_TRA_SMPTE170M    6
#define HB_COLR_TRA_SMPTE240M    7
#define HB_COLR_TRA_LINEAR       8
#define HB_COLR_TRA_LOG          9
#define HB_COLR_TRA_LOG_SQRT     10
#define HB_COLR_TRA_IEC61966_2_4 11
#define HB_COLR_TRA_BT1361_ECG   12
#define HB_COLR_TRA_IEC61966_2_1 13
#define HB_COLR_TRA_BT2020_10    14
#define HB_COLR_TRA_BT2020_12    15
#define HB_COLR_TRA_SMPTEST2084  16
#define HB_COLR_TRA_SMPTE428     17
#define HB_COLR_TRA_ARIB_STD_B67 18 //known as "Hybrid log-gamma"
// 0, 3-6, 8-15, 17-65535: reserved/not implemented
#define HB_COLR_MAT_RGB          0
#define HB_COLR_MAT_BT709        1
#define HB_COLR_MAT_UNDEF        2
#define HB_COLR_MAT_FCC          4
#define HB_COLR_MAT_BT470BG      5
#define HB_COLR_MAT_SMPTE170M    6 // also use for fcc and bt470bg
#define HB_COLR_MAT_SMPTE240M    7
#define HB_COLR_MAT_YCGCO        8
#define HB_COLR_MAT_BT2020_NCL   9
#define HB_COLR_MAT_BT2020_CL    10
#define HB_COLR_MAT_SMPTE2085    11
#define HB_COLR_MAT_CD_NCL       12 // chromaticity derived non-constant lum
#define HB_COLR_MAT_CD_CL        13 // chromaticity derived constant lum
#define HB_COLR_MAT_ICTCP        14 // ITU-R BT.2100-0, ICtCp
// 0, 3-5, 8, 11-65535: reserved/not implemented

    hb_mastering_display_metadata_t mastering;
    hb_content_light_metadata_t coll;
    hb_ambient_viewing_environment_metadata_t ambient;
    hb_dovi_conf_t dovi;

    enum {NONE = 0x0, ALL = 0x3, DOVI = 0x1, HDR_10_PLUS = 0x2} passthru_dynamic_hdr_metadata;


    hb_list_t     * list_chapter;

    /* List of audio settings. */
    hb_list_t     * list_audio;
    int             acodec_copy_mask; // Auto Passthru allowed codecs
    int             acodec_fallback;  // Auto Passthru fallback encoder

    /* Subtitles */
    hb_list_t     * list_subtitle;

    hb_list_t     * list_attachment;

    hb_metadata_t * metadata;

    /*
     * Muxer settings
     *     mux:  output file format
     *     file: file path
     */
#define HB_MUX_MASK      0xFF0001
#define HB_MUX_INVALID   0x000000
#define HB_MUX_MP4V2     0x010000
#define HB_MUX_AV_MP4    0x020000
#define HB_MUX_MASK_MP4  0x030000
#define HB_MUX_LIBMKV    0x100000
#define HB_MUX_AV_MKV    0x200000
#define HB_MUX_AV_WEBM   0x400000
#define HB_MUX_MASK_MKV  0x300000
#define HB_MUX_MASK_AV   0x620000
#define HB_MUX_MASK_WEBM 0x400000

/* default muxer for each container */
#define HB_MUX_MP4       HB_MUX_AV_MP4
#define HB_MUX_MKV       HB_MUX_AV_MKV
#define HB_MUX_WEBM      HB_MUX_AV_WEBM

    int             mux;
    char          * file;

    int             inline_parameter_sets;
                                        // Put h.264/h.265 SPS and PPS
                                        // inline in the stream. This
                                        // is necessary when constructing
                                        // adaptive streaming dash files.
    int             align_av_start;     // align A/V stream start times.
                                        // This is used to work around mp4
                                        // players that do not support edit
                                        // lists. When this option is used
                                        // the resulting stream is not a
                                        // faithful reproduction of the source
                                        // stream and may have blank frames
                                        // added or initial frames dropped.
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

    // QSV-specific settings
    struct
    {
        int decode;
        int async_depth;
#if HB_PROJECT_FEATURE_QSV
        hb_qsv_context *ctx;
#endif
        // shared encoding parameters
        // initialized by the QSV encoder, then used upstream (e.g. by filters)
        // to configure their output so that it matches what the encoder expects
        struct
        {
            int pic_struct;
            int align_width;
            int align_height;
            int is_init_done;
        } enc_info;
    } qsv;

    int hw_decode;

#ifdef __LIBHB__
    /* Internal data */
    hb_handle_t   * h;
    volatile hb_error_code * done_error;
    volatile int  * die;
    volatile int    done;

    uint64_t        st_paused;

    hb_fifo_t     * fifo_mpeg2;   /* MPEG-2 video ES */
    hb_fifo_t     * fifo_raw;     /* Raw pictures */
    hb_fifo_t     * fifo_sync;    /* Raw pictures, framerate corrected */
    hb_fifo_t     * fifo_render;  /* Raw pictures, scaled */
    hb_fifo_t     * fifo_mpeg4;   /* MPEG-4 video ES */

    hb_list_t     * list_work;

    hb_esconfig_t config;

    hb_mux_data_t * mux_data;

    int64_t         reader_pts_offset; // Reader can discard some video.
                                       // Other pipeline stages need to know
                                       // this.  E.g. sync and decsrtsub

    void           *hw_device_ctx;
    int             hw_pix_fmt;
#endif
};

/* Audio starts here */
/* Audio Codecs: Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/NativeConstants.cs when changing these consts */
#define HB_ACODEC_INVALID   0x00000000
#define HB_ACODEC_NONE      0x00000001
#define HB_ACODEC_MASK      0x0FFFFF01
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
#define HB_ACODEC_MP2       0x08000000
#define HB_ACODEC_MP3       0x00080000
#define HB_ACODEC_FFFLAC    0x00100000
#define HB_ACODEC_FFFLAC24  0x00200000
#define HB_ACODEC_FDK_AAC   0x00400000
#define HB_ACODEC_FDK_HAAC  0x00800000
#define HB_ACODEC_FFEAC3    0x01000000
#define HB_ACODEC_FFTRUEHD  0x02000000
#define HB_ACODEC_OPUS      0x04000000
#define HB_ACODEC_FF_MASK   0x0FFF2800
#define HB_ACODEC_PASS_FLAG 0x40000000
#define HB_ACODEC_PASS_MASK   (HB_ACODEC_AC3 | HB_ACODEC_DCA | HB_ACODEC_DCA_HD | HB_ACODEC_FFAAC | HB_ACODEC_FFEAC3 | HB_ACODEC_FFFLAC | HB_ACODEC_MP2 | HB_ACODEC_MP3 | HB_ACODEC_FFTRUEHD | HB_ACODEC_OPUS)
#define HB_ACODEC_AUTO_PASS   (HB_ACODEC_PASS_FLAG | HB_ACODEC_PASS_MASK)
#define HB_ACODEC_ANY         (HB_ACODEC_PASS_FLAG | HB_ACODEC_MASK)
#define HB_ACODEC_AAC_PASS    (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFAAC)
#define HB_ACODEC_AC3_PASS    (HB_ACODEC_PASS_FLAG | HB_ACODEC_AC3)
#define HB_ACODEC_DCA_PASS    (HB_ACODEC_PASS_FLAG | HB_ACODEC_DCA)
#define HB_ACODEC_DCA_HD_PASS (HB_ACODEC_PASS_FLAG | HB_ACODEC_DCA_HD)
#define HB_ACODEC_EAC3_PASS   (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFEAC3)
#define HB_ACODEC_FLAC_PASS   (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFFLAC)
#define HB_ACODEC_MP2_PASS    (HB_ACODEC_PASS_FLAG | HB_ACODEC_MP2)
#define HB_ACODEC_MP3_PASS    (HB_ACODEC_PASS_FLAG | HB_ACODEC_MP3)
#define HB_ACODEC_TRUEHD_PASS (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFTRUEHD)
#define HB_ACODEC_OPUS_PASS   (HB_ACODEC_PASS_FLAG | HB_ACODEC_OPUS)

#define HB_SUBSTREAM_BD_TRUEHD  0x72
#define HB_SUBSTREAM_BD_AC3     0x76
#define HB_SUBSTREAM_BD_DTSHD   0x72
#define HB_SUBSTREAM_BD_DTS     0x71

/* define an invalid VBR quality compatible with all VBR-capable codecs */
#define HB_INVALID_AUDIO_QUALITY (-3.)

#define HB_AUDIO_ATTR_NONE              0x00
#define HB_AUDIO_ATTR_NORMAL            0x01
#define HB_AUDIO_ATTR_VISUALLY_IMPAIRED 0x02
#define HB_AUDIO_ATTR_COMMENTARY        0x04
#define HB_AUDIO_ATTR_ALT_COMMENTARY    0x08
#define HB_AUDIO_ATTR_SECONDARY         0x10
#define HB_AUDIO_ATTR_DEFAULT           0x20
// This mask should contain all attributes that are allowed for default
// audio track selection
#define HB_AUDIO_ATTR_REGULAR_MASK      0x21

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_audio_config_s.cs when changing this struct
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
        double   quality; /* Output quality (encoder-specific) */
        double   compression_level;  /* Output compression level (encoder-specific) */
        double   dynamic_range_compression; /* Amount of DRC applied to this track */
        double   gain; /* Gain (in dB), negative is quieter */
        int      normalize_mix_level; /* mix level normalization (boolean) */
        int      dither_method; /* dither algorithm */
        const char * name; /* Output track name */
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
        PRIVATE uint32_t version; /* Bitstream version */
        PRIVATE uint32_t flags; /* Bitstream flags, codec-specific */
        PRIVATE uint32_t mode; /* Bitstream mode, codec-specific */
        PRIVATE int samplerate; /* Input sample rate (Hz) */
        PRIVATE int sample_bit_depth; /* Input samples' bit depth (0 if unknown) */
        PRIVATE int samples_per_frame; /* Number of samples per frame */
        PRIVATE int bitrate; /* Input bitrate (bps) */
        PRIVATE int matrix_encoding; /* Source matrix encoding mode, set by the audio decoder */
        PRIVATE uint64_t channel_layout; /* Source channel layout, set by the audio decoder */
        PRIVATE hb_chan_map_t * channel_map; /* Source channel map, set by the audio decoder */
        PRIVATE int encoder_delay; /* Encoder delay in samples.
                                    * These samples should be dropped
                                    * when decoding */
        PRIVATE hb_rational_t timebase;
        const char * name;
    } in;

    struct
    {
        PRIVATE char description[1024];
        PRIVATE char simple[1024];
        PRIVATE char iso639_2[4];
        PRIVATE uint32_t attributes; /* normal, visually impaired, director's commentary */
    } lang;
};

#ifdef __LIBHB__
// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_audio_s.cs when changing this struct
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

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_chapter_s.cs when changing this struct
struct hb_chapter_s
{
    int      index;

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
 *
 * Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_subtitle_s.cs when changing this struct
 */

#define HB_SUBTITLE_ATTR_UNKNOWN    0x0000
#define HB_SUBTITLE_ATTR_NORMAL     0x0001
#define HB_SUBTITLE_ATTR_LARGE      0x0002
#define HB_SUBTITLE_ATTR_CHILDREN   0x0004
#define HB_SUBTITLE_ATTR_CC         0x0008
#define HB_SUBTITLE_ATTR_FORCED     0x0010
#define HB_SUBTITLE_ATTR_COMMENTARY 0x0020
#define HB_SUBTITLE_ATTR_4_3        0x0040
#define HB_SUBTITLE_ATTR_WIDE       0x0080
#define HB_SUBTITLE_ATTR_LETTERBOX  0x0100
#define HB_SUBTITLE_ATTR_PANSCAN    0x0200
#define HB_SUBTITLE_ATTR_DEFAULT    0x0400

#define HB_SUBTITLE_IMPORT_TAG      0xFF000000
#define HB_SUBTITLE_EMBEDDED_CC_TAG 0xFE000000

struct hb_subtitle_s
{
    int  id;
    int  track;
    int  out_track;

    hb_subtitle_config_t config;

    enum subtype { PICTURESUB, TEXTSUB } format;
    enum subsource {
        VOBSUB,
        CC608SUB,
        CC708SUB, // unused
        UTF8SUB,
        TX3GSUB,
        SSASUB,
        PGSSUB,
        IMPORTSRT,
        IMPORTSSA,
        DVBSUB,
        SRTSUB = IMPORTSRT
    } source;
    const char * name;
    char         lang[1024];
    char         iso639_2[4];
    uint32_t     attributes; /* Closed Caption, Children, Directors etc */

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
    uint32_t        codec;          /* Input "codec" */
    uint32_t        codec_param;    /* Per-codec config info */
    uint32_t        reg_desc;       /* registration descriptor of source */
    uint32_t        stream_type;    /* stream type from source stream */
    uint32_t        substream_type; /* substream for multiplexed streams */
    hb_rational_t   timebase;

    hb_fifo_t     * fifo_in;        /* SPU ES */
    hb_fifo_t     * fifo_raw;       /* Decoded SPU */
    hb_fifo_t     * fifo_out;       /* Correct Timestamps, ready to be muxed */
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
    enum attachtype { FONT_TTF_ATTACH, FONT_OTF_ATTACH, HB_ART_ATTACH } type;
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
    hb_dict_t * dict;
    hb_list_t * list_coverart;
};

struct hb_title_s
{
    enum { HB_DVD_TYPE, HB_BD_TYPE, HB_STREAM_TYPE, HB_FF_STREAM_TYPE } type;
    uint32_t        reg_desc;
    const char    * path;
    const char    * name;
    int             index;
    int             playlist;
    int             angle_count;
    void          * opaque_priv;

    /* Visual-friendly duration */
    int             hours;
    int             minutes;
    int             seconds;

    /* Exact duration (in 1/90000s) */
    uint64_t        duration;

    int             preview_count;
    int             has_resolution_change;
    enum { HB_ROTATION_0, HB_ROTATION_90, HB_ROTATION_180, HB_ROTATION_270 } rotation;
    hb_geometry_t   geometry;
    hb_rational_t   dar;             // aspect ratio for the title's video
    hb_rational_t   container_dar;   // aspect ratio from container (0 if none)
    int             pix_fmt;
    int             color_prim;
    int             color_transfer;
    int             color_matrix;
    int             color_range;
    int             chroma_location;

    hb_mastering_display_metadata_t mastering;
    hb_content_light_metadata_t     coll;
    hb_ambient_viewing_environment_metadata_t ambient;
    hb_dovi_conf_t  dovi;
    int             hdr_10_plus;

    hb_rational_t   vrate;
    int             crop[4];
    int             loose_crop[4];

    enum {HB_DVD_DEMUXER, HB_TS_DEMUXER, HB_PS_DEMUXER, HB_NULL_DEMUXER} demuxer;
    int             detected_interlacing;
    int             pcr_pid;                /* PCR PID for TS streams */
    int             video_id;               /* demuxer stream id for video */
    int             video_codec;            /* worker object id of video codec */
    uint32_t        video_stream_type;      /* stream type from source stream */
    int             video_codec_param;      /* codec specific config */
    char          * video_codec_name;
    int             video_bitrate;
    hb_rational_t   video_timebase;
    char          * container_name;
    int             data_rate;

    // additional supported video decoders (e.g. HW-accelerated implementations)
    int           video_decode_support;
#define HB_DECODE_SUPPORT_SW             0x01 // software (libavcodec or mpeg2dec)
#define HB_DECODE_SUPPORT_QSV            0x02 // Intel Quick Sync Video
#define HB_DECODE_SUPPORT_NVDEC          0x04
#define HB_DECODE_SUPPORT_VIDEOTOOLBOX   0x08

#define HB_DECODE_SUPPORT_HWACCEL        (HB_DECODE_SUPPORT_NVDEC | HB_DECODE_SUPPORT_VIDEOTOOLBOX)


    hb_metadata_t * metadata;

    hb_list_t     * list_chapter;
    hb_list_t     * list_audio;
    hb_list_t     * list_subtitle;
    hb_list_t     * list_attachment;

    uint32_t        flags;
                // set if video stream doesn't have IDR frames
#define         HBTF_NO_IDR (1 << 0)
#define         HBTF_SCAN_COMPLETE (1 << 1)
#define         HBTF_RAW_VIDEO (1 << 2)
};

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_state_s.cs when changing this struct
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
    int         state;
    int         sequence_id;

    struct
    {
        struct
        {
            /* HB_STATE_SCANNING */
            float progress;
            int preview_cur;
            int preview_count;
            int title_cur;
            int title_count;
        } scanning;

        struct
        {
            /* HB_STATE_WORKING || HB_STATE_SEARCHING || HB_STATE_WORKDONE */
#define HB_PASS_SUBTITLE    -1
#define HB_PASS_ENCODE      0
#define HB_PASS_ENCODE_ANALYSIS  1   // Some code depends on these values being
#define HB_PASS_ENCODE_FINAL     2   // 1 and 2.  Do not change.
            int           pass_id;
            int           pass;
            int           pass_count;
            float         progress;
            float         rate_cur;
            float         rate_avg;
            int64_t       eta_seconds;
            int           hours;
            int           minutes;
            int           seconds;
            uint64_t      paused;
            hb_error_code error;
        } working;

        struct
        {
            /* HB_STATE_MUXING */
            float progress;
        } muxing;
    } param;
};

typedef struct hb_work_info_s
{
    const char  * name;
    int           profile;
    int           level;
    int           bitrate;
    hb_rational_t rate;
    uint32_t      version;
    uint32_t      flags;
    uint32_t      mode;
    union
    {
        struct
        {    // info only valid for video decoders
            hb_geometry_t geometry;
            int           pix_fmt;
            int           color_prim;
            int           color_transfer;
            int           color_matrix;
            int           color_range;
            int           chroma_location;
            int           video_decode_support;
        };
        struct
        {    // info only valid for audio decoders
            uint64_t channel_layout;
            hb_chan_map_t * channel_map;
            int samples_per_frame;
            int sample_bit_depth;
            int matrix_encoding;
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
    volatile int      * die;
    int                 status;
    int                 frame_count;
    int                 codec_param;
    hb_title_t        * title;

    hb_work_object_t  * next;

    hb_handle_t       * h;
#endif
};

extern hb_work_object_t hb_sync_video;
extern hb_work_object_t hb_sync_audio;
extern hb_work_object_t hb_sync_subtitle;
extern hb_work_object_t hb_decvobsub;
extern hb_work_object_t hb_decsrtsub;
extern hb_work_object_t hb_decutf8sub;
extern hb_work_object_t hb_dectx3gsub;
extern hb_work_object_t hb_decssasub;
extern hb_work_object_t hb_decavsub;
extern hb_work_object_t hb_encavcodec;
extern hb_work_object_t hb_encqsv;
extern hb_work_object_t hb_encvt;
extern hb_work_object_t hb_encx264;
extern hb_work_object_t hb_enctheora;
extern hb_work_object_t hb_encx265;
extern hb_work_object_t hb_encsvtav1;
extern hb_work_object_t hb_decavcodeca;
extern hb_work_object_t hb_decavcodecv;
extern hb_work_object_t hb_declpcm;
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

typedef struct hb_filter_init_s
{
    hb_job_t      * job;
    int             pix_fmt;
    int             hw_pix_fmt;
    int             color_prim;
    int             color_transfer;
    int             color_matrix;
    int             color_range;
    int             chroma_location;
    hb_geometry_t   geometry;
    int             crop[4];
    hb_rational_t   vrate;
    int             cfr;
    int             grayscale;
    hb_rational_t   time_base;
    void          * hw_frames_ctx;
} hb_filter_init_t;

typedef struct hb_filter_info_s
{
    char             * human_readable_desc;
    hb_filter_init_t   output;
} hb_filter_info_t;

struct hb_filter_object_s
{
    int                   id;
    int                   enforce_order;
    int                   skip;
    int                   aliased;
    char                * name;
    hb_dict_t           * settings;

#ifdef __LIBHB__
    int                (* init)       ( hb_filter_object_t *, hb_filter_init_t * );
    int                (* init_thread)( hb_filter_object_t *, int );
    int                (* post_init)  ( hb_filter_object_t *, hb_job_t * );
    int                (* work)       ( hb_filter_object_t *,
                                        hb_buffer_t **, hb_buffer_t ** );
    int                (* work_thread)( hb_filter_object_t *,
                                        hb_buffer_t **, hb_buffer_t **, int );
    void               (* close)      ( hb_filter_object_t * );
    hb_filter_info_t * (* info)       ( hb_filter_object_t * );

    const char          * settings_template;

    hb_fifo_t           * fifo_in;
    hb_fifo_t           * fifo_out;

    hb_subtitle_t       * subtitle;

    hb_filter_private_t * private_data;

    hb_thread_t         * thread;
    volatile int        * done;
    int                   status;

    // Filters can drop frames and thus chapter marks
    // These are used to bridge the chapter to the next buffer
    int                   chapter_val;
    int64_t               chapter_time;

    hb_filter_object_t  * sub_filter;
#endif
};

// Update win/CS/HandBrake.Interop/HandBrakeInterop/HbLib/hb_filter_ids.cs when changing this enum
enum
{
    HB_FILTER_INVALID = 0,
    HB_FILTER_FIRST = 1,

    // First, filters that may change the framerate (drop or dup frames)
    HB_FILTER_DETELECINE,
    HB_FILTER_COMB_DETECT,
    HB_FILTER_DECOMB,
    HB_FILTER_YADIF,
    HB_FILTER_BWDIF,
    HB_FILTER_VFR,
    // Filters that must operate on the original source image are next
    HB_FILTER_DEBLOCK,
    HB_FILTER_DENOISE,
    HB_FILTER_HQDN3D = HB_FILTER_DENOISE,
    HB_FILTER_NLMEANS,
    HB_FILTER_CHROMA_SMOOTH,
    HB_FILTER_ROTATE,
    HB_FILTER_RENDER_SUB,
    HB_FILTER_CROP_SCALE,
    HB_FILTER_LAPSHARP,
    HB_FILTER_UNSHARP,
    HB_FILTER_GRAYSCALE,
    HB_FILTER_PAD,
    HB_FILTER_COLORSPACE,
    HB_FILTER_FORMAT,
    HB_FILTER_RPU,

    // Finally filters that don't care what order they are in,
    // except that they must be after the above filters
    HB_FILTER_AVFILTER,

    HB_FILTER_LAST,
    // wrapper filter for frame based multi-threading of simple filters
    HB_FILTER_MT_FRAME
};

hb_filter_object_t * hb_filter_get( int filter_id );
hb_filter_object_t * hb_filter_init( int filter_id );
hb_filter_object_t * hb_filter_copy( hb_filter_object_t * filter );
hb_list_t          * hb_filter_list_copy(const hb_list_t *src);
hb_dict_t          * hb_filter_dict_find(const hb_value_array_t * list,
                                         int filter_id);
hb_filter_object_t * hb_filter_find(const hb_list_t *list, int filter_id);
void                 hb_filter_close( hb_filter_object_t ** );
void                 hb_filter_info_close( hb_filter_info_t ** );
hb_dict_t          * hb_parse_filter_settings(const char * settings);
char               * hb_parse_filter_settings_json(const char * settings_str);
char               * hb_filter_settings_string(int filter_id,
                                               hb_value_t * value);
char               * hb_filter_settings_string_json(int filter_id,
                                                    const char * json);

typedef void hb_error_handler_t( const char *errmsg );

extern void hb_register_error_handler( hb_error_handler_t * handler );

void hb_str_to_locale(char *buffer);
void hb_str_from_locale(char *buffer);

char * hb_strdup_vaprintf( const char * fmt, va_list args );
char * hb_strdup_printf(const char *fmt, ...) HB_WPRINTF(1, 2);
char * hb_strncat_dup( const char * s1, const char * s2, size_t n );

// free array of strings
void    hb_str_vfree( char ** strv );
// count number of strings in array of strings
int     hb_str_vlen(char ** strv);
// split string into array of strings
char ** hb_str_vsplit( const char * str, char delem );

int hb_yuv2rgb(int yuv);
int hb_rgb2yuv(int rgb);
int hb_rgb2yuv_bt709(int rgb);

const char * hb_subsource_name( int source );

// unparse a set of x264 settings to an HB encopts string
char * hb_x264_param_unparse(int bit_depth, const char *x264_preset,
                             const char *x264_tune, const char *x264_encopts,
                             const char *h264_profile, const char *h264_level,
                             int width, int height);

// x264 option name/synonym helper
const char * hb_x264_encopt_name( const char * name );

#if HB_PROJECT_FEATURE_X265
// x265 option name/synonym helper
const char * hb_x265_encopt_name( const char * name );
#endif

int hb_output_color_prim(hb_job_t * job);
int hb_output_color_transfer(hb_job_t * job);
int hb_output_color_matrix(hb_job_t * job);

int hb_get_bit_depth(int format);
int hb_get_chroma_sub_sample(int format, int *h_shift, int *v_shift);
int hb_get_best_pix_fmt(hb_job_t * job);
int hb_get_best_hw_pix_fmt(hb_job_t * job);

#define HB_NEG_FLOAT_REG "(([-])?(([0-9]+([.,][0-9]+)?)|([.,][0-9]+))"
#define HB_FLOAT_REG     "(([0-9]+([.,][0-9]+)?)|([.,][0-9]+))"
#define HB_NEG_INT_REG   "(([-]?[0-9]+)"
#define HB_INT_REG       "([0-9]+)"
#define HB_RATIONAL_REG  "([0-9]+/[0-9]+)"
#define HB_BOOL_REG      "(yes|no|true|false|[01])"
#define HB_ALL_REG       "(.*)"

#endif // HANDBRAKE_COMMON_H
