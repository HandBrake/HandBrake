/* encx264.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdarg.h>
#include "libavutil/avutil.h"
#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/encx264.h"
#include "handbrake/extradata.h"

int  encx264Init( hb_work_object_t *, hb_job_t * );
int  encx264Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encx264Close( hb_work_object_t * );

hb_work_object_t hb_encx264 =
{
    WORK_ENCX264,
    "H.264/AVC encoder (libx264)",
    encx264Init,
    encx264Work,
    encx264Close
};

#define DTS_BUFFER_SIZE 32

#define MASTERING_CHROMA_DEN 50000
#define MASTERING_LUMA_DEN 10000

struct hb_work_private_s
{
    hb_job_t           * job;
    x264_t             * x264;
    x264_picture_t       pic_in;

    int64_t              last_stop;   // Debugging - stop time of previous input frame

    hb_chapter_queue_t * chapter_queue;

    char               * filename;

    // Multiple bit-depth
    const x264_api_t *   api;
};

#define HB_X264_API_COUNT   2
static x264_api_t x264_apis[HB_X264_API_COUNT];

const char *libx264_10bit_names[] = {
    "libx26410b", "libx264_main10", NULL
};

const char *libx264_8bit_names[] = {
    "libx264", "libx2648b", "libx264_main", NULL
};

static void * x264_lib_open(const char *names[])
{
    if (names == NULL)
        return NULL;

    void *h;
    int   ii = 0;
    while (names[ii] != NULL)
    {
        char *name = hb_strdup_printf("%s%s", names[ii], HB_SO_EXT);
        h = hb_dlopen(name);
        free(name);
        if (h != NULL)
        {
            return h;
        }
        ii++;
    }
    return NULL;
}

#if defined(SYS_LINUX)
static void * x264_lib_open_ubuntu_10bit_search(const char *prefix)
{
    void          * h;
    const char    * name = "libx264.so";

    HB_DIR        * dir;
    struct dirent * entry;

    dir = hb_opendir(prefix);
    if (dir == NULL)
    {
        return NULL;
    }

    while ((entry = hb_readdir(dir)) != NULL)
    {
        if (!strncmp(entry->d_name, name, strlen(name)))
        {
            char *path = hb_strdup_printf("%s/%s", prefix, entry->d_name);
            h = hb_dlopen(path);
            free(path);
            if (h != NULL)
            {
                hb_closedir(dir);
                return h;
            }
        }
    }
    hb_closedir(dir);

    return NULL;
}

static void * x264_lib_open_ubuntu_10bit(void)
{
    void *h = NULL;

#if ARCH_X86_64
    h = x264_lib_open_ubuntu_10bit_search("/usr/lib/x86_64-linux-gnu/x264-10bit");
#elif ARCH_X86_32
    h = x264_lib_open_ubuntu_10bit_search("/usr/lib/i386-linux-gnu/x264-10bit");
#endif
    if (h == NULL)
    {
        h = x264_lib_open_ubuntu_10bit_search("/usr/lib/x264-10bit");
    }
    return h;
}
#endif

void hb_x264_global_init(void)
{
#if X264_BUILD < 153
    x264_apis[0].bit_depth                 = x264_bit_depth;
#else
    x264_apis[0].bit_depth                 = X264_BIT_DEPTH;
#endif
    x264_apis[0].param_default             = x264_param_default;
    x264_apis[0].param_default_preset      = x264_param_default_preset;
    x264_apis[0].param_apply_profile       = x264_param_apply_profile;
    x264_apis[0].param_apply_fastfirstpass = x264_param_apply_fastfirstpass;
    x264_apis[0].param_parse               = x264_param_parse;
    x264_apis[0].encoder_open              = x264_encoder_open;
    x264_apis[0].encoder_headers           = x264_encoder_headers;
    x264_apis[0].encoder_encode            = x264_encoder_encode;
    x264_apis[0].encoder_delayed_frames    = x264_encoder_delayed_frames;
    x264_apis[0].encoder_close             = x264_encoder_close;
    x264_apis[0].picture_init              = x264_picture_init;

    if (x264_apis[0].bit_depth == 0)
    {
        // libx264 supports 8 and 10 bit
        x264_apis[0].bit_depth                 = 8;
        x264_apis[1].bit_depth                 = 10;
        x264_apis[1].param_default             = x264_param_default;
        x264_apis[1].param_default_preset      = x264_param_default_preset;
        x264_apis[1].param_apply_profile       = x264_param_apply_profile;
        x264_apis[1].param_apply_fastfirstpass = x264_param_apply_fastfirstpass;
        x264_apis[1].param_parse               = x264_param_parse;
        x264_apis[1].encoder_open              = x264_encoder_open;
        x264_apis[1].encoder_headers           = x264_encoder_headers;
        x264_apis[1].encoder_encode            = x264_encoder_encode;
        x264_apis[1].encoder_delayed_frames    = x264_encoder_delayed_frames;
        x264_apis[1].encoder_close             = x264_encoder_close;
        x264_apis[1].picture_init              = x264_picture_init;
        return;
    }

    // Invalidate other apis
    x264_apis[1].bit_depth = -1;

    // Attempt to dlopen a library for handling the bit-depth that we do
    // not already have.
    void *h;
    if (x264_apis[0].bit_depth == 8)
    {
        h = x264_lib_open(libx264_10bit_names);
#if defined(SYS_LINUX)
        if (h == NULL)
        {
            h = x264_lib_open_ubuntu_10bit();
        }
#endif
    }
    else
    {
        h = x264_lib_open(libx264_8bit_names);
    }
    if (h == NULL)
    {
        return;
    }

    int ii;
    int dll_bitdepth = 0;
#if X264_BUILD < 153
    int *pbit_depth                   = (int*)hb_dlsym(h, "x264_bit_depth");
    if (pbit_depth != NULL)
    {
        dll_bitdepth = *pbit_depth;
    }
#endif
    x264_apis[1].param_default        = hb_dlsym(h, "x264_param_default");
#if X264_BUILD >= 153
    if (x264_apis[1].param_default != NULL)
    {
        x264_param_t defaults;
        x264_apis[1].param_default(&defaults);
        dll_bitdepth = defaults.i_bitdepth;
    }
#endif
    x264_apis[1].param_default_preset = hb_dlsym(h, "x264_param_default_preset");
    x264_apis[1].param_apply_profile  = hb_dlsym(h, "x264_param_apply_profile");
    x264_apis[1].param_apply_fastfirstpass =
                                hb_dlsym(h, "x264_param_apply_fastfirstpass");
    x264_apis[1].param_parse          = hb_dlsym(h, "x264_param_parse");
    // x264 appends the build number to the end of x264_encoder_open
    for (ii = 140; ii < 200; ii++)
    {
        char *name = hb_strdup_printf("x264_encoder_open_%d", ii);
        x264_apis[1].encoder_open = hb_dlsym(h, name);
        free(name);
        if (x264_apis[1].encoder_open != NULL)
        {
            break;
        }
    }
    x264_apis[1].encoder_headers      = hb_dlsym(h, "x264_encoder_headers");
    x264_apis[1].encoder_encode       = hb_dlsym(h, "x264_encoder_encode");
    x264_apis[1].encoder_delayed_frames =
                                hb_dlsym(h, "x264_encoder_delayed_frames");
    x264_apis[1].encoder_close        = hb_dlsym(h, "x264_encoder_close");
    x264_apis[1].picture_init         = hb_dlsym(h, "x264_picture_init");

    if (dll_bitdepth > 0 && dll_bitdepth != x264_apis[0].bit_depth &&
        x264_apis[1].param_default             != NULL &&
        x264_apis[1].param_default_preset      != NULL &&
        x264_apis[1].param_apply_profile       != NULL &&
        x264_apis[1].param_apply_fastfirstpass != NULL &&
        x264_apis[1].param_parse               != NULL &&
        x264_apis[1].encoder_open              != NULL &&
        x264_apis[1].encoder_headers           != NULL &&
        x264_apis[1].encoder_encode            != NULL &&
        x264_apis[1].encoder_delayed_frames    != NULL &&
        x264_apis[1].encoder_close             != NULL &&
        x264_apis[1].picture_init              != NULL)
    {
        x264_apis[1].bit_depth = dll_bitdepth;
    }
}

const x264_api_t * hb_x264_api_get(int bit_depth)
{
    int ii;

    for (ii = 0; ii < HB_X264_API_COUNT; ii++)
    {
        if (-1        != x264_apis[ii].bit_depth &&
            bit_depth == x264_apis[ii].bit_depth)
        {
            return &x264_apis[ii];
        }
    }
    return NULL;
}

#if 0
/*
 * Check whether a valid h264_level is compatible with the given framerate,
 * resolution and interlaced compression/flags combination.
 *
 * width, height, fps_num and fps_den should be greater than zero.
 *
 * interlacing parameters can be set to zero when the information is
 * unavailable, as apply_h264_level() will disable interlacing if necessary.
 *
 * Returns 0 if the level is valid and compatible, 1 otherwise.
 */
static int check_h264_level(const char *h264_level, int width, int height,
                            int fps_num, int fps_den, int interlaced,
                            int fake_interlaced);
#endif

/*
 * Applies the restrictions of the requested H.264 level to an x264_param_t.
 *
 * Returns -1 if an invalid level (or no level) is specified. GUIs should be
 * capable of always providing a valid level.
 *
 * Does not modify resolution/framerate but warns when they exceed level limits.
 *
 * Based on a x264_param_apply_level() draft and other x264 code.
 */
static int apply_h264_level(const x264_api_t *api, x264_param_t *param,
                            const char *h264_level, const char *x264_profile,
                            int verbose);

/*
 * Applies the restrictions of the requested H.264 profile to an x264_param_t.
 *
 * x264_param_apply_profile wrapper designed to always succeed when a valid
 * H.264 profile is specified (unlike x264's function).
 */
static int apply_h264_profile(const x264_api_t *api, x264_param_t *param,
                              const char *h264_profile, int verbose);

/***********************************************************************
 * hb_work_encx264_init
 ***********************************************************************
 *
 **********************************************************************/
int encx264Init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    x264_param_t        param;
    x264_nal_t        * nal;
    int                 nal_count, bit_depth;

    w->private_data = pv;

    bit_depth = hb_video_encoder_get_depth(job->vcodec);
    pv->api   = hb_x264_api_get(bit_depth);
    if (pv->api == NULL)
    {
        hb_error("encx264: hb_x264_api_get failed, bit-depth %d", bit_depth);
        return 1;
    }

    pv->job = job;
    pv->last_stop        = AV_NOPTS_VALUE;
    pv->chapter_queue    = hb_chapter_queue_init();

    if (pv->api->param_default_preset(&param,
                                  job->encoder_preset, job->encoder_tune) < 0)
    {
        free( pv );
        w->private_data = NULL;
        return 1;
    }

#if X264_BUILD >= 153
    param.i_bitdepth = bit_depth;
#endif

    /* If the PSNR or SSIM tunes are in use, enable the relevant metric */
    if (job->encoder_tune != NULL && *job->encoder_tune)
    {
        char *tmp = strdup(job->encoder_tune);
        char *tok = strtok(tmp,   ",./-+");
        do
        {
            if (!strncasecmp(tok, "psnr", 4))
            {
                param.analyse.b_psnr = 1;
                break;
            }
            if (!strncasecmp(tok, "ssim", 4))
            {
                param.analyse.b_ssim = 1;
                break;
            }
        }
        while ((tok = strtok(NULL, ",./-+")) != NULL);
        free(tmp);
    }

    /* Some HandBrake-specific defaults; users can override them
     * using the encoder_options string. */
    param.i_fps_num = job->vrate.num;
    param.i_fps_den = job->vrate.den;
    if ( job->cfr == 1 )
    {
        param.i_timebase_num   = 0;
        param.i_timebase_den   = 0;
        param.b_vfr_input = 0;
    }
    else
    {
        param.i_timebase_num   = 1;
        param.i_timebase_den   = 90000;
    }

    /* Set min:max keyframe intervals to 1:10 of fps;
     * adjust +0.5 for when fps has remainder to bump
     * { 23.976, 29.976, 59.94 } to { 24, 30, 60 }. */
    param.i_keyint_min = (double)job->orig_vrate.num / job->orig_vrate.den +
                                 0.5;
    param.i_keyint_max = 10 * param.i_keyint_min;
    param.i_log_level  = X264_LOG_INFO;

    /* set up the VUI color model & gamma */
    param.vui.i_colorprim = hb_output_color_prim(job);
    param.vui.i_transfer  = hb_output_color_transfer(job);
    param.vui.i_colmatrix = hb_output_color_matrix(job);
    if (job->chroma_location != AVCHROMA_LOC_UNSPECIFIED)
    {
        param.vui.i_chroma_loc = job->chroma_location - 1;
    }

#if X264_BUILD >= 163
    /* HDR10 Static metadata */
    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        /* Mastering display metadata */
        if (job->mastering.has_primaries && job->mastering.has_luminance)
        {
            param.mastering_display.b_mastering_display = 1;
            param.mastering_display.i_red_x   = hb_rescale_rational(job->mastering.display_primaries[0][0], MASTERING_CHROMA_DEN);
            param.mastering_display.i_red_y   = hb_rescale_rational(job->mastering.display_primaries[0][1], MASTERING_CHROMA_DEN);
            param.mastering_display.i_green_x = hb_rescale_rational(job->mastering.display_primaries[1][0], MASTERING_CHROMA_DEN);
            param.mastering_display.i_green_y = hb_rescale_rational(job->mastering.display_primaries[1][1], MASTERING_CHROMA_DEN);
            param.mastering_display.i_blue_x  = hb_rescale_rational(job->mastering.display_primaries[2][0], MASTERING_CHROMA_DEN);
            param.mastering_display.i_blue_y  = hb_rescale_rational(job->mastering.display_primaries[2][1], MASTERING_CHROMA_DEN);
            param.mastering_display.i_white_x = hb_rescale_rational(job->mastering.white_point[0], MASTERING_CHROMA_DEN);
            param.mastering_display.i_white_y = hb_rescale_rational(job->mastering.white_point[1], MASTERING_CHROMA_DEN);
            param.mastering_display.i_display_max = hb_rescale_rational(job->mastering.max_luminance, MASTERING_LUMA_DEN);
            param.mastering_display.i_display_min = hb_rescale_rational(job->mastering.min_luminance, MASTERING_LUMA_DEN);
        }

        /*  Content light level */
        if (job->coll.max_cll && job->coll.max_fall)
        {
            param.content_light_level.b_cll = 1;
            param.content_light_level.i_max_cll  = job->coll.max_cll;
            param.content_light_level.i_max_fall = job->coll.max_fall;
        }
    }
#endif

    /* place job->encoder_options in an hb_dict_t for convenience */
    hb_dict_t * x264_opts = NULL;
    if (job->encoder_options != NULL && *job->encoder_options)
    {
        x264_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);
    }
    /* iterate through x264_opts and have libx264 parse the options for us */
    int ret;
    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(x264_opts);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(x264_opts, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

        /* Here's where the strings are passed to libx264 for parsing. */
        ret = pv->api->param_parse(&param, key, str);

        /* Let x264 sanity check the options for us */
        if (ret == X264_PARAM_BAD_NAME)
            hb_log( "x264 options: Unknown suboption %s", key );
        if (ret == X264_PARAM_BAD_VALUE)
            hb_log( "x264 options: Bad argument %s=%s", key,
                    str ? str : "(null)" );
        free(str);
    }
    hb_dict_free(&x264_opts);

    /* Reload colorimetry settings in case custom values were set
     * in the encoder_options string */
    job->color_prim_override     = param.vui.i_colorprim;
    job->color_transfer_override = param.vui.i_transfer;
    job->color_matrix_override   = param.vui.i_colmatrix;
    job->chroma_location         = param.vui.i_chroma_loc + 1;

#if X264_BUILD >= 163
    /* Reload HDR10 mastering display metadata settings in case custom values were set
     * in the encoder_options string */
    if (param.mastering_display.b_mastering_display)
    {
        job->mastering.display_primaries[0][0] = hb_make_q(param.mastering_display.i_red_x, MASTERING_CHROMA_DEN);
        job->mastering.display_primaries[0][1] = hb_make_q(param.mastering_display.i_red_y, MASTERING_CHROMA_DEN);
        job->mastering.display_primaries[1][0] = hb_make_q(param.mastering_display.i_green_x, MASTERING_CHROMA_DEN);
        job->mastering.display_primaries[1][1] = hb_make_q(param.mastering_display.i_green_y, MASTERING_CHROMA_DEN);
        job->mastering.display_primaries[2][0] = hb_make_q(param.mastering_display.i_blue_x, MASTERING_CHROMA_DEN);
        job->mastering.display_primaries[2][1] = hb_make_q(param.mastering_display.i_blue_y, MASTERING_CHROMA_DEN);

        job->mastering.white_point[0] = hb_make_q(param.mastering_display.i_white_x, MASTERING_CHROMA_DEN);
        job->mastering.white_point[1] = hb_make_q(param.mastering_display.i_white_y, MASTERING_CHROMA_DEN);

        job->mastering.min_luminance = hb_make_q(param.mastering_display.i_display_min, MASTERING_LUMA_DEN);
        job->mastering.max_luminance = hb_make_q(param.mastering_display.i_display_max, MASTERING_LUMA_DEN);
    }

    /* Reload HDR10 content light level metadata settings in case custom values were set
     * in the encoder_options string */
    if (param.content_light_level.b_cll)
    {
        job->coll.max_cll = param.content_light_level.i_max_cll;
        job->coll.max_fall = param.content_light_level.i_max_fall;
    }
#endif

    /* For 25 fps sources, HandBrake's explicit keyints will match the x264 defaults:
     * min-keyint 25 (same as auto), keyint 250. */
    if( param.i_keyint_min != 25 || param.i_keyint_max != 250 )
    {
        int min_auto;

        if ( param.i_fps_num / param.i_fps_den < param.i_keyint_max / 10 )
            min_auto = param.i_fps_num / param.i_fps_den;
        else
            min_auto = param.i_keyint_max / 10;

        char min[40], max[40];
        param.i_keyint_min == X264_KEYINT_MIN_AUTO ?
            snprintf( min, 40, "auto (%d)", min_auto ) :
            snprintf( min, 40, "%d", param.i_keyint_min );

        param.i_keyint_max == X264_KEYINT_MAX_INFINITE ?
            snprintf( max, 40, "infinite" ) :
            snprintf( max, 40, "%d", param.i_keyint_max );

        hb_log( "encx264: min-keyint: %s, keyint: %s", min, max );
    }

    /* Settings which can't be overridden in the encoder_options string
     * (muxer-specific settings, resolution, ratecontrol, etc.). */

    /* Disable annexb. Inserts size into nal header instead of start code. */
    param.b_annexb = 0;

    param.i_width  = job->width;
    param.i_height = job->height;

    param.vui.i_sar_width  = job->par.num;
    param.vui.i_sar_height = job->par.den;

    if (job->vquality > HB_INVALID_VIDEO_QUALITY)
    {
        /* Constant RF */
        param.rc.i_rc_method = X264_RC_CRF;
        param.rc.f_rf_constant = job->vquality;
        hb_log( "encx264: encoding at constant RF %f", param.rc.f_rf_constant );
    }
    else
    {
        /* Average bitrate */
        param.rc.i_rc_method = X264_RC_ABR;
        param.rc.i_bitrate = job->vbitrate;
        hb_log( "encx264: encoding at average bitrate %d", param.rc.i_bitrate );
        if( job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
            job->pass_id == HB_PASS_ENCODE_FINAL )
        {
            pv->filename = hb_get_temporary_filename("x264.log");
        }
        switch( job->pass_id )
        {
            case HB_PASS_ENCODE_ANALYSIS:
                param.rc.b_stat_read  = 0;
                param.rc.b_stat_write = 1;
                param.rc.psz_stat_out = pv->filename;
                break;
            case HB_PASS_ENCODE_FINAL:
                param.rc.b_stat_read  = 1;
                param.rc.b_stat_write = 0;
                param.rc.psz_stat_in  = pv->filename;
                break;
        }
    }

    switch (job->output_pix_fmt)
    {
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUV422P10:
        case AV_PIX_FMT_YUV422P12:
        case AV_PIX_FMT_YUV422P16:
            param.i_csp = X264_CSP_I422;
            break;
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
        case AV_PIX_FMT_YUV444P16:
            param.i_csp = X264_CSP_I444;
            break;
    }

    /* Apply profile and level settings last, if present. */
    if (job->encoder_profile != NULL && *job->encoder_profile)
    {
        if (apply_h264_profile(pv->api, &param, job->encoder_profile, 1))
        {
            free(pv);
            w->private_data = NULL;
            return 1;
        }
    }
    if (job->encoder_level != NULL && *job->encoder_level)
    {
        if (apply_h264_level(pv->api, &param, job->encoder_level,
                             job->encoder_profile, 1) < 0)
        {
            free(pv);
            w->private_data = NULL;
            return 1;
        }
    }

    /* Turbo analysis pass */
    if (job->pass_id == HB_PASS_ENCODE_ANALYSIS && job->fastanalysispass == 1)
    {
        pv->api->param_apply_fastfirstpass( &param );
    }

    /* B-pyramid is enabled by default. */
    job->areBframes = 2;

    if( !param.i_bframe )
    {
        job->areBframes = 0;
    }
    else if( !param.i_bframe_pyramid )
    {
        job->areBframes = 1;
    }

    /* Log the unparsed x264 options string. */
    char *x264_opts_unparsed = hb_x264_param_unparse(pv->api->bit_depth,
                                                     job->encoder_preset,
                                                     job->encoder_tune,
                                                     job->encoder_options,
                                                     job->encoder_profile,
                                                     job->encoder_level,
                                                     job->width,
                                                     job->height);
    if( x264_opts_unparsed != NULL )
    {
        hb_log( "encx264: unparsed options: %s", x264_opts_unparsed );
    }
    free( x264_opts_unparsed );

    hb_deep_log( 2, "encx264: opening libx264 (pass %d)", job->pass_id );
    pv->x264 = pv->api->encoder_open( &param );
    if ( pv->x264 == NULL )
    {
        hb_error("encx264: x264_encoder_open failed.");
        free( pv );
        w->private_data = NULL;
        return 1;
    }

    pv->api->encoder_headers( pv->x264, &nal, &nal_count );

    if (hb_set_h264_extradata(w->extradata,
                              nal[0].p_payload + 4, nal[0].i_payload - 4,
                              nal[1].p_payload + 4, nal[1].i_payload - 4))
    {
        hb_error("encx264: set extradata failed.");
        free( pv );
        w->private_data = NULL;
        return 1;
    }

    pv->api->picture_init( &pv->pic_in );

    pv->pic_in.img.i_csp = param.i_csp;
    if (pv->api->bit_depth > 8)
    {
        pv->pic_in.img.i_csp |= X264_CSP_HIGH_DEPTH;
    }
    pv->pic_in.img.i_plane = 3;

    return 0;
}

void encx264Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if (pv == NULL)
    {
        // Not initialized
        return;
    }

    hb_chapter_queue_close(&pv->chapter_queue);

    pv->api->encoder_close( pv->x264 );
    free( pv->filename );
    free( pv );
    w->private_data = NULL;
}

static hb_buffer_t *nal_encode( hb_work_object_t *w, x264_picture_t *pic_out,
                                int i_nal, x264_nal_t *nal )
{
    hb_buffer_t *buf = NULL;
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job = pv->job;
    int payload_size = 0;

    for (int i = 0; i < i_nal; i++)
    {
        payload_size += nal[i].i_payload;
    }

    buf = hb_buffer_init(payload_size);
    buf->size = 0;
    buf->s.frametype = 0;

    // use the pts to get the original frame's duration.
    buf->s.duration     = AV_NOPTS_VALUE;
    buf->s.start        = pic_out->i_pts;
    buf->s.stop         = AV_NOPTS_VALUE;
    buf->s.renderOffset = pic_out->i_dts;
    if (!*w->init_delay && pic_out->i_dts < 0)
    {
        *w->init_delay = -pic_out->i_dts;
    }

    /* Determine what type of frame we have. */
    switch (pic_out->i_type)
    {
        case X264_TYPE_IDR:
            buf->s.frametype = HB_FRAME_IDR;
            break;

        case X264_TYPE_P:
            buf->s.frametype = HB_FRAME_P;
            break;

        case X264_TYPE_B:
            buf->s.frametype = HB_FRAME_B;
            break;

        case X264_TYPE_BREF:
            buf->s.frametype = HB_FRAME_BREF;
            break;

        case X264_TYPE_I:
        default:
            buf->s.frametype = HB_FRAME_I;
            break;
    }
    buf->s.flags = 0;

    if (pic_out->b_keyframe)
    {
        buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
        /* if we have a chapter marker pending and this
           frame's presentation time stamp is at or after
           the marker's time stamp, use this as the
           chapter start. */
        hb_chapter_dequeue(pv->chapter_queue, buf);
    }

    /* Encode all the NALs we were given into buf.
       NOTE: This code assumes one video frame per NAL (but there can
             be other stuff like SPS and/or PPS). If there are multiple
             frames we only get the duration of the first which will
             eventually screw up the muxer & decoder. */
    buf->s.flags &= ~HB_FLAG_FRAMETYPE_REF;
    for (int i = 0; i < i_nal; i++)
    {
        int size = nal[i].i_payload;
        if (size < 1)
        {
            continue;
        }

        memcpy(buf->data + buf->size, nal[i].p_payload, size);

        /* H.264 in .mp4 or .mkv */
        switch( nal[i].i_type )
        {
            /* Sequence Parameter Set & Program Parameter Set go in the
             * mp4 header so skip them here
             */
            case NAL_SPS:
            case NAL_PPS:
                if (!job->inline_parameter_sets)
                {
                    continue;
                }
                break;

            case NAL_SLICE_IDR:
            case NAL_SLICE:
            case NAL_SEI:
            default:
                break;
        }

        /*
         * Expose disposable bit to muxer.
         *
         * Also, since libx264 doesn't tell us when B-frames are
         * themselves reference frames, figure it out on our own.
         */
        if (nal[i].i_ref_idc != NAL_PRIORITY_DISPOSABLE)
        {
            if (buf->s.frametype == HB_FRAME_B)
            {
                buf->s.frametype  = HB_FRAME_BREF;
            }
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
        }

        buf->size += size;
    }
    // make sure we found at least one video frame
    if ( buf->size <= 0 )
    {
        // no video - discard the buf
        hb_buffer_close( &buf );
    }
    return buf;
}

static hb_buffer_t * expand_buf(int bit_depth, hb_buffer_t *in, int input_pix_fmt)
{
    hb_buffer_t *buf;
    const int    shift = bit_depth - 8;
    int          output_pix_fmt;

    switch (input_pix_fmt)
    {
        case AV_PIX_FMT_YUV420P:
            output_pix_fmt = AV_PIX_FMT_YUV420P16;
            break;
        case AV_PIX_FMT_YUV422P:
            output_pix_fmt = AV_PIX_FMT_YUV422P16;
            break;
        case AV_PIX_FMT_YUV444P:
        default:
            output_pix_fmt = AV_PIX_FMT_YUV444P16;
            break;
    }

    buf = hb_frame_buffer_init(output_pix_fmt, in->f.width, in->f.height);
    for (int pp = 0; pp < 3; pp++)
    {
        uint8_t  *src =  in->plane[pp].data;
        uint16_t *dst = (uint16_t*)buf->plane[pp].data;
        for (int yy = 0; yy < in->plane[pp].height; yy++)
        {
            for (int xx = 0; xx < in->plane[pp].width; xx++)
            {
                dst[xx] = (uint16_t)src[xx] << shift;
            }
            src +=  in->plane[pp].stride;
            dst += buf->plane[pp].stride / 2;
        }
    }
    return buf;
}

static hb_buffer_t *x264_encode( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t          *job = pv->job;
    hb_buffer_t       *tmp = NULL;

    /* Point x264 at our current buffers Y(UV) data.  */
    if (pv->pic_in.img.i_csp & X264_CSP_HIGH_DEPTH &&
        (job->output_pix_fmt == AV_PIX_FMT_YUV420P ||
         job->output_pix_fmt == AV_PIX_FMT_YUV422P ||
         job->output_pix_fmt == AV_PIX_FMT_YUV444P))
    {
        tmp = expand_buf(pv->api->bit_depth, in, job->output_pix_fmt);
        pv->pic_in.img.i_stride[0] = tmp->plane[0].stride;
        pv->pic_in.img.i_stride[1] = tmp->plane[1].stride;
        pv->pic_in.img.i_stride[2] = tmp->plane[2].stride;
        pv->pic_in.img.plane[0] = tmp->plane[0].data;
        pv->pic_in.img.plane[1] = tmp->plane[1].data;
        pv->pic_in.img.plane[2] = tmp->plane[2].data;
    }
    else
    {
        pv->pic_in.img.i_stride[0] = in->plane[0].stride;
        pv->pic_in.img.i_stride[1] = in->plane[1].stride;
        pv->pic_in.img.i_stride[2] = in->plane[2].stride;
        pv->pic_in.img.plane[0] = in->plane[0].data;
        pv->pic_in.img.plane[1] = in->plane[1].data;
        pv->pic_in.img.plane[2] = in->plane[2].data;
    }

    if( in->s.new_chap > 0 && job->chapter_markers )
    {
        /* chapters have to start with an IDR frame so request that this
           frame be coded as IDR. Since there may be up to 16 frames
           currently buffered in the encoder remember the timestamp so
           when this frame finally pops out of the encoder we'll mark
           its buffer as the start of a chapter. */
        pv->pic_in.i_type = X264_TYPE_IDR;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }
    else
    {
        pv->pic_in.i_type = X264_TYPE_AUTO;
    }

    /* XXX this is temporary debugging code to check that the upstream
     * modules (render & sync) have generated a continuous, self-consistent
     * frame stream with the current frame's start time equal to the
     * previous frame's stop time.
     */
    if (pv->last_stop != AV_NOPTS_VALUE && pv->last_stop != in->s.start)
    {
        hb_log("encx264 input continuity err: last stop %"PRId64"  start %"PRId64,
                pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;

    // Remember info about this frame that we need to pass across
    // the x264_encoder_encode call (since it reorders frames).

    /* Feed the input PTS to x264 so it can figure out proper output PTS */
    pv->pic_in.i_pts = in->s.start;

    x264_picture_t pic_out;
    int i_nal;
    x264_nal_t *nal;

    pv->api->encoder_encode( pv->x264, &nal, &i_nal, &pv->pic_in, &pic_out );
    if ( i_nal > 0 )
    {
        hb_buffer_close(&tmp);
        return nal_encode( w, &pic_out, i_nal, nal );
    }
    hb_buffer_close(&tmp);
    return NULL;
}

int encx264Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;

    *buf_out = NULL;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input. Flush any frames still in the decoder then
        // send the eof downstream to tell the muxer we're done.
        x264_picture_t pic_out;
        int i_nal;
        x264_nal_t *nal;
        hb_buffer_list_t list;

        hb_buffer_list_clear(&list);

        // flush delayed frames
        while ( pv->api->encoder_delayed_frames( pv->x264 ) )
        {
            pv->api->encoder_encode( pv->x264, &nal, &i_nal, NULL, &pic_out );
            if ( i_nal == 0 )
                continue;
            if ( i_nal < 0 )
                break;

            hb_buffer_t *buf = nal_encode( w, &pic_out, i_nal, nal );
            hb_buffer_list_append(&list, buf);
        }
        // add the EOF to the end of the chain
        hb_buffer_list_append(&list, in);

        *buf_out = hb_buffer_list_clear(&list);
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    // Not EOF - encode the packet & wrap it in a NAL
    *buf_out = x264_encode( w, in );
    return HB_WORK_OK;
}

static int apply_h264_profile(const x264_api_t *api, x264_param_t *param,
                              const char *h264_profile, int verbose)
{
    const char * const * profile_names;
    if (api->bit_depth == 10)
    {
        profile_names = hb_h264_profile_names_10bit;
    }
    else
    {
        profile_names = hb_h264_profile_names_8bit;
    }
    if (h264_profile != NULL &&
        strcasecmp(h264_profile, profile_names[0]) != 0)
    {
        /*
         * baseline profile doesn't support interlacing
         */
        if ((param->b_interlaced ||
             param->b_fake_interlaced) &&
            !strcasecmp(h264_profile, "baseline"))
        {
            if (verbose)
            {
                hb_log("apply_h264_profile [warning]: baseline profile doesn't support interlacing, disabling");
            }
            param->b_interlaced = param->b_fake_interlaced = 0;
        }
        /*
         * lossless requires High 4:4:4 Predictive profile
         */
        int qp_bd_offset = 6 * (api->bit_depth - 8);
        if (strcasecmp(h264_profile, "high444") != 0 &&
            ((param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant <= 0) ||
             (param->rc.i_rc_method == X264_RC_CRF && (int)(param->rc.f_rf_constant + qp_bd_offset) <= 0)))
        {
            if (verbose)
            {
                hb_log("apply_h264_profile [warning]: lossless requires high444 profile, disabling");
            }
            if (param->rc.i_rc_method == X264_RC_CQP)
            {
                param->rc.i_qp_constant = 1;
            }
            else
            {
                param->rc.f_rf_constant = 1 - qp_bd_offset;
            }
        }
        return api->param_apply_profile(param, h264_profile);
    }
    else if (!strcasecmp(h264_profile, profile_names[0]))
    {
        // "auto", do nothing
        return 0;
    }
    else
    {
        // error (profile not a string), abort
        hb_error("apply_h264_profile: no profile specified");
        return -1;
    }
}

#if 0
static int check_h264_level(const char *h264_level,
                            int width, int height, int fps_num,
                            int fps_den, int interlaced, int fake_interlaced)
{
    x264_param_t param;
    x264_param_default(&param);
    param.i_width           = width;
    param.i_height          = height;
    param.i_fps_num         = fps_num;
    param.i_fps_den         = fps_den;
    param.b_interlaced      = !!interlaced;
    param.b_fake_interlaced = !!fake_interlaced;
    return (apply_h264_level(&param, h264_level, NULL, 0) != 0);
}
#endif

int apply_h264_level(const x264_api_t *api, x264_param_t *param,
                     const char *h264_level, const char *h264_profile,
                     int verbose)
{
    float f_framerate;
    const x264_level_t *x264_level = NULL;
    int i, i_mb_size, i_mb_rate, i_mb_width, i_mb_height, max_mb_side, ret;

    /*
     * find the x264_level_t corresponding to the requested level
     */
    if (h264_level != NULL &&
        strcasecmp(h264_level, hb_h264_level_names[0]) != 0)
    {
        for (i = 0; hb_h264_level_values[i]; i++)
        {
            if (!strcmp(hb_h264_level_names[i], h264_level))
            {
                int val = hb_h264_level_values[i];
                for (i = 0; x264_levels[i].level_idc; i++)
                {
                    if (x264_levels[i].level_idc == val)
                    {
                        x264_level = &x264_levels[i];
                        break;
                    }
                }
                break;
            }
        }
        if (x264_level == NULL)
        {
            // error (invalid or unsupported level), abort
            hb_error("apply_h264_level: invalid level %s", h264_level);
            return -1;
        }
    }
    else if(h264_level != NULL &&
            !strcasecmp(h264_level, hb_h264_level_names[0]))
    {
        // "auto", do nothing
        return 0;
    }
    else
    {
        // error (level not a string), abort
        hb_error("apply_h264_level: no level specified");
        return -1;
    }

    /*
     * the H.264 profile determines VBV constraints
     */
    enum
    {
        // Main or Baseline (equivalent)
        HB_ENCX264_PROFILE_MAIN,
        HB_ENCX264_PROFILE_HIGH,
        HB_ENCX264_PROFILE_HIGH10,
        HB_ENCX264_PROFILE_HIGH422,
        // Lossless
        HB_ENCX264_PROFILE_HIGH444,
    } hb_encx264_profile;

    /*
     * H.264 profile
     *
     * TODO: we need to guess the profile like x264_sps_init does, otherwise
     * we'll get an error when setting a Main-incompatible VBV and
     * x264_sps_init() guesses Main profile. x264_sps_init() may eventually take
     * VBV into account when guessing profile, at which point this code can be
     * re-enabled.
     */
#if 0
    if (h264_profile != NULL && *h264_profile)
    {
        // if the user explicitly specified a profile, don't guess it
        if (!strcasecmp(h264_profile, "high444"))
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH444;
        }
        else if (!strcasecmp(h264_profile, "high422"))
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH422;
        }
        else if (!strcasecmp(h264_profile, "main") ||
                 !strcasecmp(h264_profile, "baseline"))
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_MAIN;
        }
        else
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH;
        }
    }
    else
#endif
    {
        // guess the H.264 profile if the user didn't request one
        if (param->rc.i_rc_method == X264_RC_CRF &&
            param->rc.f_rf_constant < 1.0)
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH444;
        }
        else if (param->analyse.b_transform_8x8 ||
                 param->i_cqm_preset != X264_CQM_FLAT)
        {
            if (h264_profile != NULL && !strcasecmp(h264_profile, "high422"))
            {
                hb_encx264_profile = HB_ENCX264_PROFILE_HIGH422;
            }
            else if (api->bit_depth == 10)
            {
                hb_encx264_profile = HB_ENCX264_PROFILE_HIGH10;
            }
            else
            {
                hb_encx264_profile = HB_ENCX264_PROFILE_HIGH;
            }
        }
        else
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_MAIN;
        }
    }

    /*
     * we need at least width and height in order to apply a level correctly
     */
    if (param->i_width <= 0 || param->i_height <= 0)
    {
        // error (invalid width or height), abort
        hb_error("apply_h264_level: invalid resolution (width: %d, height: %d)",
                 param->i_width, param->i_height);
        return -1;
    }

    /*
     * a return value of 1 means there were warnings
     */
    ret = 0;

    /*
     * some levels do not support interlaced encoding
     */
    if (x264_level->frame_only && (param->b_interlaced ||
                                   param->b_fake_interlaced))
    {
        if (verbose)
        {
            hb_log("apply_h264_level [warning]: interlaced flag not supported for level %s, disabling",
                   h264_level);
        }
        ret = 1;
        param->b_interlaced = param->b_fake_interlaced = 0;
    }

    /*
     * frame dimensions and rate (in macroblocks)
     */
    i_mb_width  = (param->i_width  + 15) / 16;
    i_mb_height = (param->i_height + 15) / 16;
    if (param->b_interlaced || param->b_fake_interlaced)
    {
        // interlaced: encoded height must divide cleanly by 32
        i_mb_height = (i_mb_height + 1) & ~1;
    }
    i_mb_size = i_mb_width * i_mb_height;
    if (param->i_fps_den <= 0 || param->i_fps_num <= 0)
    {
        i_mb_rate   = 0;
        f_framerate = 0.0;
    }
    else
    {
        i_mb_rate   = (int64_t)i_mb_size * param->i_fps_num / param->i_fps_den;
        f_framerate = (float)param->i_fps_num / param->i_fps_den;
    }

    /*
     * sanitize ref/frameref
     */
    if (param->i_keyint_max != 1)
    {
        int i_max_dec_frame_buffering =
            MAX(MIN(x264_level->dpb / i_mb_size, 16), 1);
        param->i_frame_reference =
            MIN(i_max_dec_frame_buffering, param->i_frame_reference);
        /*
         * some level and resolution combos may require as little as 1 ref;
         * bframes and b-pyramid are not compatible with this scenario
         */
        if (i_max_dec_frame_buffering < 2)
        {
            param->i_bframe = 0;
        }
        else if (i_max_dec_frame_buffering < 4)
        {
            param->i_bframe_pyramid = X264_B_PYRAMID_NONE;
        }
    }

    /*
     * set and/or sanitize the VBV (if not lossless)
     */
    if (hb_encx264_profile != HB_ENCX264_PROFILE_HIGH444)
    {
        // High profile allows for higher VBV bufsize/maxrate
        int cbp_factor;
        cbp_factor = hb_encx264_profile == HB_ENCX264_PROFILE_HIGH10 ? 12 :
                     hb_encx264_profile == HB_ENCX264_PROFILE_HIGH   ?  5 : 4;
        if (!param->rc.i_vbv_max_bitrate)
        {
            param->rc.i_vbv_max_bitrate = (x264_level->bitrate * cbp_factor) / 4;
        }
        else
        {
            param->rc.i_vbv_max_bitrate =
                MIN(param->rc.i_vbv_max_bitrate,
                    (x264_level->bitrate * cbp_factor) / 4);
        }
        if (!param->rc.i_vbv_buffer_size)
        {
            param->rc.i_vbv_buffer_size = (x264_level->cpb * cbp_factor) / 4;
        }
        else
        {
            param->rc.i_vbv_buffer_size =
                MIN(param->rc.i_vbv_buffer_size,
                    (x264_level->cpb * cbp_factor) / 4);
        }
    }

    /*
     * sanitize mvrange/mv-range
     */
    param->analyse.i_mv_range =
        MIN(param->analyse.i_mv_range,
            x264_level->mv_range >> !!param->b_interlaced);

    /*
     * TODO: check the rest of the limits
     */

    /*
     * things we can do nothing about (too late to change resolution or fps),
     * print warnings if we're not being quiet
     */
    if (x264_level->frame_size < i_mb_size)
    {
        if (verbose)
        {
            hb_log("apply_h264_level [warning]: frame size (%dx%d, %d macroblocks) too high for level %s (max. %d macroblocks)",
                   i_mb_width * 16, i_mb_height * 16, i_mb_size, h264_level,
                   x264_level->frame_size);
        }
        ret = 1;
    }
    else if (x264_level->mbps < i_mb_rate)
    {
        if (verbose)
        {
            hb_log("apply_h264_level [warning]: framerate (%.3f) too high for level %s at %dx%d (max. %.3f)",
                   f_framerate, h264_level, param->i_width, param->i_height,
                   (float)x264_level->mbps / i_mb_size);
        }
        ret = 1;
    }
    /*
     * width or height squared may not exceed 8 * frame_size (in macroblocks)
     * thus neither dimension may exceed sqrt(8 * frame_size)
     */
    max_mb_side = sqrt(x264_level->frame_size * 8);
    if (i_mb_width > max_mb_side)
    {
        if (verbose)
        {
            hb_log("apply_h264_level [warning]: frame too wide (%d) for level %s (max. %d)",
                   param->i_width, h264_level, max_mb_side * 16);
        }
        ret = 1;
    }
    if (i_mb_height > max_mb_side)
    {
        if (verbose)
        {
            hb_log("apply_h264_level [warning]: frame too tall (%d) for level %s (max. %d)",
                   param->i_height, h264_level, max_mb_side * 16);
        }
        ret = 1;
    }

    /*
     * level successfully applied, yay!
     */
    param->i_level_idc = x264_level->level_idc;
    return ret;
}

static hb_value_t * value_pair(hb_value_t * v1, hb_value_t * v2)
{
    hb_value_t *array = hb_value_array_init();
    hb_value_array_append(array, v1);
    hb_value_array_append(array, v2);
    return array;
}

// TODO: add bit_depth
char * hb_x264_param_unparse(int bit_depth, const char *x264_preset,
                             const char *x264_tune, const char *x264_encopts,
                             const char *h264_profile, const char *h264_level,
                             int width, int height)
{
    int i;
    char *unparsed_opts;
    hb_dict_t *x264_opts;
    x264_param_t defaults, param;
    const x264_api_t *api;

    /*
     * get the global x264 defaults (what we compare against)
     */
    api = hb_x264_api_get(bit_depth);
    api->param_default(&defaults);

    /*
     * apply the defaults, preset and tune
     */
    if (api->param_default_preset(&param, x264_preset, x264_tune) < 0)
    {
        /*
         * Note: GUIs should be able to always specify valid preset/tunes, so
         *       this code will hopefully never be reached
         */
        return strdup("hb_x264_param_unparse: invalid x264 preset/tune");
    }

    /*
     * place additional x264 options in a dictionary
     */
    x264_opts = hb_encopts_to_dict(x264_encopts, HB_VCODEC_X264);

    /*
     * some libx264 options are set via dedicated widgets in the video tab or
     * hardcoded in libhb, and have no effect when present in the advanced x264
     * options string.
     *
     * clear them from x264_opts so as to not apply then during unparse.
     */
    hb_dict_remove(x264_opts, "qp");
    hb_dict_remove(x264_opts, "qp_constant");
    hb_dict_remove(x264_opts, "crf");
    hb_dict_remove(x264_opts, "bitrate");
    hb_dict_remove(x264_opts, "fps");
    hb_dict_remove(x264_opts, "force-cfr");
    hb_dict_remove(x264_opts, "sar");
    hb_dict_remove(x264_opts, "annexb");

    /*
     * apply the additional x264 options
     */
    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(x264_opts);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(x264_opts, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

        // let's not pollute GUI logs with x264_param_parse return codes
        api->param_parse(&param, key, str);
        free(str);
    }

    /*
     * apply the x264 profile, if specified
     */
    if (h264_profile != NULL && *h264_profile)
    {
        // be quiet so at to not pollute GUI logs
        apply_h264_profile(api, &param, h264_profile, 0);
    }

    /*
     * apply the h264 level, if specified
     */
    if (h264_level != NULL && *h264_level)
    {
        // set width/height to avoid issues in apply_h264_level
        param.i_width  = width;
        param.i_height = height;
        // be quiet so at to not pollute GUI logs
        apply_h264_level(api, &param, h264_level, h264_profile, 0);
    }

    /*
     * if x264_encopts is NULL, x264_opts wasn't initialized
     */
    if (x264_opts == NULL && (x264_opts = hb_dict_init()) == NULL)
    {
        return strdup("hb_x264_param_unparse: could not initialize hb_dict_t");
    }

    /*
     * x264 lets you specify some options in multiple ways. For options that we
     * do unparse, clear the forms that don't match how we unparse said option
     * from the x264_opts dictionary.
     *
     * actual synonyms are already handled by hb_encopts_to_dict().
     *
     * "no-deblock" is a special case as it can't be unparsed to "deblock=0"
     *
     * also, don't bother with forms that aren't allowed by the x264 CLI, such
     * as "no-bframes" - there are too many.
     */
    hb_dict_remove(x264_opts, "no-sliced-threads");
    hb_dict_remove(x264_opts, "no-scenecut");
    hb_dict_remove(x264_opts, "no-b-adapt");
    hb_dict_remove(x264_opts, "no-weightb");
    hb_dict_remove(x264_opts, "no-cabac");
    hb_dict_remove(x264_opts, "interlaced"); // we unparse to tff/bff
    hb_dict_remove(x264_opts, "no-interlaced");
    hb_dict_remove(x264_opts, "no-8x8dct");
    hb_dict_remove(x264_opts, "no-mixed-refs");
    hb_dict_remove(x264_opts, "no-fast-pskip");
    hb_dict_remove(x264_opts, "no-dct-decimate");
    hb_dict_remove(x264_opts, "no-psy");
    hb_dict_remove(x264_opts, "no-mbtree");

    /*
     * compare defaults to param and unparse to the x264_opts dictionary
     */
    if (!param.b_sliced_threads != !defaults.b_sliced_threads)
    {
        // can be modified by: tune zerolatency
        hb_dict_set(x264_opts, "sliced-threads",
                    hb_value_bool(!!param.b_sliced_threads));
    }
    else
    {
        hb_dict_remove(x264_opts, "sliced-threads");
    }
    if (param.i_sync_lookahead != defaults.i_sync_lookahead)
    {
        // can be modified by: tune zerolatency
        hb_dict_set(x264_opts, "sync-lookahead",
                    hb_value_int(param.i_sync_lookahead));
    }
    else
    {
        hb_dict_remove(x264_opts, "sync-lookahead");
    }
    if (param.i_level_idc != defaults.i_level_idc)
    {
        // can be modified by: level
        for (i = 0; hb_h264_level_values[i]; i++)
            if (param.i_level_idc == hb_h264_level_values[i])
                hb_dict_set(x264_opts, "level",
                            hb_value_string(hb_h264_level_names[i]));
    }
    else
    {
        hb_dict_remove(x264_opts, "level");
    }
    if (param.i_frame_reference != defaults.i_frame_reference)
    {
        // can be modified by: presets, tunes, level
        hb_dict_set(x264_opts, "ref", hb_value_int(param.i_frame_reference));
    }
    else
    {
        hb_dict_remove(x264_opts, "ref");
    }
    if (param.i_scenecut_threshold != defaults.i_scenecut_threshold)
    {
        // can be modified by: preset ultrafast
        hb_dict_set(x264_opts, "scenecut",
                    hb_value_int(param.i_scenecut_threshold));
    }
    else
    {
        hb_dict_remove(x264_opts, "scenecut");
    }
    if (param.i_bframe != defaults.i_bframe)
    {
        // can be modified by: presets, tunes, profile, level
        hb_dict_set(x264_opts, "bframes", hb_value_int(param.i_bframe));
    }
    else
    {
        hb_dict_remove(x264_opts, "bframes");
    }
    if (param.i_bframe > 0)
    {
        if (param.i_bframe_adaptive != defaults.i_bframe_adaptive)
        {
            // can be modified by: presets
            hb_dict_set(x264_opts, "b-adapt",
                        hb_value_int(param.i_bframe_adaptive));
        }
        else
        {
            hb_dict_remove(x264_opts, "b-adapt");
        }
        if (param.i_bframe > 1 &&
            param.i_bframe_pyramid != defaults.i_bframe_pyramid)
        {
            // can be modified by: level
            if (param.i_bframe_pyramid < X264_B_PYRAMID_NONE)
                param.i_bframe_pyramid = X264_B_PYRAMID_NONE;
            if (param.i_bframe_pyramid > X264_B_PYRAMID_NORMAL)
                param.i_bframe_pyramid = X264_B_PYRAMID_NORMAL;
            for (i = 0; x264_b_pyramid_names[i] != NULL; i++)
                if (param.i_bframe_pyramid == i)
                    hb_dict_set(x264_opts, "b-pyramid",
                                hb_value_string(x264_b_pyramid_names[i]));
        }
        else
        {
            hb_dict_remove(x264_opts, "b-pyramid");
        }
        if (param.analyse.i_direct_mv_pred != defaults.analyse.i_direct_mv_pred)
        {
            // can be modified by: presets
            if (param.analyse.i_direct_mv_pred < X264_DIRECT_PRED_NONE)
                param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE;
            if (param.analyse.i_direct_mv_pred > X264_DIRECT_PRED_AUTO)
                param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
            for (i = 0; x264_direct_pred_names[i] != NULL; i++)
                if (param.analyse.i_direct_mv_pred == i)
                    hb_dict_set(x264_opts, "direct",
                                hb_value_string(x264_direct_pred_names[i]));
        }
        else
        {
            hb_dict_remove(x264_opts, "direct");
        }
        if (!param.analyse.b_weighted_bipred !=
            !defaults.analyse.b_weighted_bipred)
        {
            // can be modified by: preset ultrafast, tune fastdecode
            hb_dict_set(x264_opts, "weightb",
                        hb_value_bool(!!param.analyse.b_weighted_bipred));
        }
        else
        {
            hb_dict_remove(x264_opts, "weightb");
        }
    }
    else
    {
        // no bframes, these options have no effect
        hb_dict_remove(x264_opts, "b-adapt");
        hb_dict_remove(x264_opts, "b-pyramid");
        hb_dict_remove(x264_opts, "direct");
        hb_dict_remove(x264_opts, "weightb");
        hb_dict_remove(x264_opts, "b-bias");
        hb_dict_remove(x264_opts, "open-gop");
    }
    if (!param.b_deblocking_filter != !defaults.b_deblocking_filter)
    {
        // can be modified by: preset ultrafast, tune fastdecode
        hb_dict_set(x264_opts, "no-deblock",
                    hb_value_bool(!param.b_deblocking_filter));
    }
    else
    {
        hb_dict_remove(x264_opts, "no-deblock");
    }
    if (param.b_deblocking_filter &&
        (param.i_deblocking_filter_alphac0 != defaults.i_deblocking_filter_alphac0 ||
         param.i_deblocking_filter_beta    != defaults.i_deblocking_filter_beta))
    {
        // can be modified by: tunes
        hb_dict_set(x264_opts, "deblock",
                    value_pair(hb_value_int(param.i_deblocking_filter_alphac0),
                               hb_value_int(param.i_deblocking_filter_beta)));
    }
    else
    {
        hb_dict_remove(x264_opts, "deblock");
    }
    if (!param.b_cabac != !defaults.b_cabac)
    {
        // can be modified by: preset ultrafast, tune fastdecode, profile
        hb_dict_set(x264_opts, "cabac", hb_value_bool(!!param.b_cabac));
    }
    else
    {
        hb_dict_remove(x264_opts, "cabac");
    }
    if (param.b_interlaced != defaults.b_interlaced)
    {
        if (param.b_tff)
        {
            hb_dict_set(x264_opts, "tff", hb_value_bool(1));
            hb_dict_remove(x264_opts, "bff");
        }
        else
        {
            hb_dict_set(x264_opts, "bff", hb_value_bool(1));
            hb_dict_remove(x264_opts, "tff");
        }
        hb_dict_remove(x264_opts, "fake-interlaced");
    }
    else if (param.b_fake_interlaced != defaults.b_fake_interlaced)
    {
        hb_dict_set(x264_opts, "fake-interlaced", hb_value_bool(1));
        hb_dict_remove(x264_opts, "tff");
        hb_dict_remove(x264_opts, "bff");
    }
    else
    {
        hb_dict_remove(x264_opts, "tff");
        hb_dict_remove(x264_opts, "bff");
        hb_dict_remove(x264_opts, "fake-interlaced");
    }
    if (param.i_cqm_preset == defaults.i_cqm_preset &&
        param.psz_cqm_file == defaults.psz_cqm_file)
    {
        // can be reset to default by: profile
        hb_dict_remove(x264_opts, "cqm");
        hb_dict_remove(x264_opts, "cqm4");
        hb_dict_remove(x264_opts, "cqm8");
        hb_dict_remove(x264_opts, "cqm4i");
        hb_dict_remove(x264_opts, "cqm4p");
        hb_dict_remove(x264_opts, "cqm8i");
        hb_dict_remove(x264_opts, "cqm8p");
        hb_dict_remove(x264_opts, "cqm4iy");
        hb_dict_remove(x264_opts, "cqm4ic");
        hb_dict_remove(x264_opts, "cqm4py");
        hb_dict_remove(x264_opts, "cqm4pc");
    }
    /*
     * Note: param.analyse.intra can only be modified directly or by using
     *       x264 --preset ultrafast, but not via the "analyse" option
     */
    if (param.analyse.inter != defaults.analyse.inter)
    {
        // can be modified by: presets, tune touhou
        if (!param.analyse.inter)
        {
            hb_dict_set(x264_opts, "analyse", hb_value_string("none"));
        }
        else if ((param.analyse.inter & X264_ANALYSE_I4x4)      &&
                 (param.analyse.inter & X264_ANALYSE_I8x8)      &&
                 (param.analyse.inter & X264_ANALYSE_PSUB16x16) &&
                 (param.analyse.inter & X264_ANALYSE_PSUB8x8)   &&
                 (param.analyse.inter & X264_ANALYSE_BSUB16x16))
        {
            hb_dict_set(x264_opts, "analyse", hb_value_string("all"));
        }
        else
        {
            hb_value_t *array = hb_value_array_init();
            if (param.analyse.inter & X264_ANALYSE_I4x4)
            {
                hb_value_array_append(array, hb_value_string("i4x4"));
            }
            if (param.analyse.inter & X264_ANALYSE_I8x8)
            {
                hb_value_array_append(array, hb_value_string("i8x8"));
            }
            if (param.analyse.inter & X264_ANALYSE_PSUB16x16)
            {
                hb_value_array_append(array, hb_value_string("p8x8"));
            }
            if (param.analyse.inter & X264_ANALYSE_PSUB8x8)
            {
                hb_value_array_append(array, hb_value_string("p4x4"));
            }
            if (param.analyse.inter & X264_ANALYSE_BSUB16x16)
            {
                hb_value_array_append(array, hb_value_string("b8x8"));
            }
            hb_dict_set(x264_opts, "analyse", array);
        }
    }
    else
    {
        hb_dict_remove(x264_opts, "analyse");
    }
    if (!param.analyse.b_transform_8x8 != !defaults.analyse.b_transform_8x8)
    {
        // can be modified by: preset ultrafast, profile
        hb_dict_set(x264_opts, "8x8dct",
                    hb_value_bool(!!param.analyse.b_transform_8x8));
    }
    else
    {
        hb_dict_remove(x264_opts, "8x8dct");
    }
    if (param.analyse.i_weighted_pred != defaults.analyse.i_weighted_pred)
    {
        // can be modified by: presets, tune fastdecode, profile
        hb_dict_set(x264_opts, "weightp",
                    hb_value_int(param.analyse.i_weighted_pred));
    }
    else
    {
        hb_dict_remove(x264_opts, "weightp");
    }
    if (param.analyse.i_me_method != defaults.analyse.i_me_method)
    {
        // can be modified by: presets
        if (param.analyse.i_me_method < X264_ME_DIA)
            param.analyse.i_me_method = X264_ME_DIA;
        if (param.analyse.i_me_method > X264_ME_TESA)
            param.analyse.i_me_method = X264_ME_TESA;
        for (i = 0; x264_motion_est_names[i] != NULL; i++)
            if (param.analyse.i_me_method == i)
                hb_dict_set(x264_opts, "me",
                            hb_value_string(x264_motion_est_names[i]));
    }
    else
    {
        hb_dict_remove(x264_opts, "me");
    }
    if (param.analyse.i_me_range != defaults.analyse.i_me_range)
    {
        // can be modified by: presets
        hb_dict_set(x264_opts, "merange",
                    hb_value_int(param.analyse.i_me_range));
    }
    else
    {
        hb_dict_remove(x264_opts, "merange");
    }
    if (param.analyse.i_mv_range != defaults.analyse.i_mv_range)
    {
        // can be modified by: level
        hb_dict_set(x264_opts, "mvrange",
                    hb_value_int(param.analyse.i_mv_range));
    }
    else
    {
        hb_dict_remove(x264_opts, "mvrange");
    }
    if (param.analyse.i_subpel_refine > 9 && (param.rc.i_aq_mode == 0 ||
                                              param.analyse.i_trellis < 2))
    {
        // subme 10 and higher require AQ and trellis 2
        param.analyse.i_subpel_refine = 9;
    }
    if (param.analyse.i_subpel_refine != defaults.analyse.i_subpel_refine)
    {
        // can be modified by: presets
        hb_dict_set(x264_opts, "subme",
                    hb_value_int(param.analyse.i_subpel_refine));
    }
    else
    {
        hb_dict_remove(x264_opts, "subme");
    }
    if (!param.analyse.b_mixed_references !=
        !defaults.analyse.b_mixed_references)
    {
        // can be modified by: presets
        hb_dict_set(x264_opts, "mixed-refs",
                    hb_value_bool(!!param.analyse.b_mixed_references));
    }
    else
    {
        hb_dict_remove(x264_opts, "mixed-refs");
    }
    if (param.analyse.i_trellis != defaults.analyse.i_trellis)
    {
        // can be modified by: presets
        hb_dict_set(x264_opts, "trellis",
                    hb_value_int(param.analyse.i_trellis));
    }
    else
    {
        hb_dict_remove(x264_opts, "trellis");
    }
    if (!param.analyse.b_fast_pskip != !defaults.analyse.b_fast_pskip)
    {
        // can be modified by: preset placebo
        hb_dict_set(x264_opts, "fast-pskip",
                    hb_value_bool(!!param.analyse.b_fast_pskip));
    }
    else
    {
        hb_dict_remove(x264_opts, "fast-pskip");
    }
    if (!param.analyse.b_dct_decimate != !defaults.analyse.b_dct_decimate)
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "dct-decimate",
                    hb_value_bool(!!param.analyse.b_dct_decimate));
    }
    else
    {
        hb_dict_remove(x264_opts, "dct-decimate");
    }
    if (!param.analyse.b_psy != !defaults.analyse.b_psy)
    {
        // can be modified by: tunes
        hb_dict_set(x264_opts, "psy", hb_value_bool(!!param.analyse.b_psy));
    }
    else
    {
        hb_dict_remove(x264_opts, "psy");
    }
    if (param.analyse.b_psy &&
        (param.analyse.f_psy_rd      != defaults.analyse.f_psy_rd ||
         param.analyse.f_psy_trellis != defaults.analyse.f_psy_trellis))
    {
        // can be modified by: tunes
        hb_dict_set(x264_opts, "psy-rd",
                    value_pair(hb_value_double(param.analyse.f_psy_rd),
                               hb_value_double(param.analyse.f_psy_trellis)));
    }
    else
    {
        hb_dict_remove(x264_opts, "psy-rd");
    }
    /*
     * Note: while deadzone is incompatible with trellis, it still has a slight
     *       effect on the output even when trellis is on, so always unparse it.
     */
    if (param.analyse.i_luma_deadzone[0] != defaults.analyse.i_luma_deadzone[0])
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "deadzone-inter",
                    hb_value_int(param.analyse.i_luma_deadzone[0]));
    }
    else
    {
        hb_dict_remove(x264_opts, "deadzone-inter");
    }
    if (param.analyse.i_luma_deadzone[1] != defaults.analyse.i_luma_deadzone[1])
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "deadzone-intra",
                    hb_value_int(param.analyse.i_luma_deadzone[1]));
    }
    else
    {
        hb_dict_remove(x264_opts, "deadzone-intra");
    }
    if (param.rc.i_vbv_buffer_size != defaults.rc.i_vbv_buffer_size)
    {
        // can be modified by: level
        hb_dict_set(x264_opts, "vbv-bufsize",
                    hb_value_int(param.rc.i_vbv_buffer_size));
        if (param.rc.i_vbv_max_bitrate != defaults.rc.i_vbv_max_bitrate)
        {
            // can be modified by: level
            hb_dict_set(x264_opts, "vbv-maxrate",
                        hb_value_int(param.rc.i_vbv_max_bitrate));
        }
        else
        {
            hb_dict_remove(x264_opts, "vbv-maxrate");
        }
    }
    else
    {
        hb_dict_remove(x264_opts, "vbv-bufsize");
        hb_dict_remove(x264_opts, "vbv-maxrate");
    }
    if (param.rc.f_ip_factor != defaults.rc.f_ip_factor)
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "ipratio",
                    hb_value_double(param.rc.f_ip_factor));
    }
    else
    {
        hb_dict_remove(x264_opts, "ipratio");
    }
    if (param.i_bframe > 0 && !param.rc.b_mb_tree &&
        param.rc.f_pb_factor != defaults.rc.f_pb_factor)
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "pbratio",
                    hb_value_double(param.rc.f_pb_factor));
    }
    else
    {
        // pbratio requires bframes and is incompatible with mbtree
        hb_dict_remove(x264_opts, "pbratio");
    }
    if (param.rc.f_qcompress != defaults.rc.f_qcompress)
    {
        // can be modified by: tune grain
        hb_dict_set(x264_opts, "qcomp", hb_value_double(param.rc.f_qcompress));
    }
    else
    {
        hb_dict_remove(x264_opts, "qcomp");
    }
    if (param.rc.i_aq_mode != defaults.rc.i_aq_mode)
    {
        // can be modified by: preset ultrafast, tune psnr
        hb_dict_set(x264_opts, "aq-mode", hb_value_int(param.rc.i_aq_mode));
    }
    else
    {
        hb_dict_remove(x264_opts, "aq-mode");
    }
    if (param.rc.i_aq_mode > 0 &&
        param.rc.f_aq_strength != defaults.rc.f_aq_strength)
    {
        // can be modified by: tunes
        hb_dict_set(x264_opts, "aq-strength",
                    hb_value_double(param.rc.f_aq_strength));
    }
    else
    {
        hb_dict_remove(x264_opts, "aq-strength");
    }
    if (!param.rc.b_mb_tree != !defaults.rc.b_mb_tree)
    {
        // can be modified by: presets, tune zerolatency
        hb_dict_set(x264_opts, "mbtree", hb_value_bool(!!param.rc.b_mb_tree));
    }
    else
    {
        hb_dict_remove(x264_opts, "mbtree");
    }
    if (param.rc.i_lookahead != defaults.rc.i_lookahead)
    {
        // can be modified by: presets, tune zerolatency
        hb_dict_set(x264_opts, "rc-lookahead",
                    hb_value_int(param.rc.i_lookahead));
    }
    else
    {
        hb_dict_remove(x264_opts, "rc-lookahead");
    }
    if (!param.b_vfr_input != !defaults.b_vfr_input)
    {
        // can be modified by: tune zerolatency
        hb_dict_set(x264_opts, "force-cfr", hb_value_bool(!param.b_vfr_input));
    }
    else
    {
        hb_dict_remove(x264_opts, "force-cfr");
    }

    /* convert the x264_opts dictionary to an encopts string */
    unparsed_opts = hb_dict_to_encopts(x264_opts);
    hb_dict_free(&x264_opts);

    /* we're done */
    return unparsed_opts;
}

const char * hb_x264_encopt_name(const char *name)
{
    int i;
    for (i = 0; hb_x264_encopt_synonyms[i][0] != NULL; i++)
        if (!strcmp(name, hb_x264_encopt_synonyms[i][1]))
            return hb_x264_encopt_synonyms[i][0];
    return name;
}
