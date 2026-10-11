/* dlssnr.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   DLSS Neural Rendering (DLSS-NR) filter, through libdlssnr
   (dlss-nr-on-vulkan). One RGB frame in, one RGB frame out, driven the way
   PCSX2's GSDLSSNR drives the same library: nr_frame_main.c's parameters (a
   profile, then the explicit overrides), the optional control mask, the
   features, the network at the vendor's extent, the head cropped, then the
   composition. With history the previous prediction goes into the
   composition -- not the features, where it would feed back on itself --
   released wherever the picture changed nearby, so nothing that moves leaves
   a copy of itself behind, and dropped altogether on a cut.

   Unlike a game's display filter an encode is offline: every frame goes
   through the network, in order, on the filter thread, and none is skipped.

   The model weights are compiled into libdlssnr by the owner's own build of
   dlss-nr-on-vulkan (`weights/`); this file never ships them. The library is
   one device with process-wide state, so one model is shared by every filter
   instance in the process (an encode and a preview, say) and every pass is
   serialised through `model_mutex`.

   Only available in builds configured with --enable-dlssnr; elsewhere the
   filter refuses to initialise and the job carries on without it.
 */

#include <math.h>
#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

#if HB_PROJECT_FEATURE_DLSSNR
#include <pthread.h>
#include "nr_frame.h"
#include "nr_image.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/mem.h"
#endif

// nr_frame_main.c's PROFILES, by index
#define DLSSNR_PROFILE_STANDARD   0
#define DLSSNR_PROFILE_NATURAL    1
#define DLSSNR_PROFILE_CINEMATIC  2
#define DLSSNR_PROFILE_NEUTRAL    3
#define DLSSNR_PROFILE_VENDOR     4

// signed numbers: the vendor's knobs take -1 for "not set"
#define DLSSNR_SFLOAT_REG "(-?(([0-9]+([.,][0-9]+)?)|([.,][0-9]+)))"
#define DLSSNR_SINT_REG   "(-?[0-9]+)"

/* GSDLSSNR::Settings: nr_frame's command line, as settings. */
typedef struct
{
    double  intensity;        // --intensity
    int     profile;          // --profile: standard, natural, cinematic, neutral, vendor
    int     style_index;      // --style-index, negative keeps the profile's
    double  local_tone;       // --local-tone, negative keeps the profile's
    double  local_structure;  // --local-structure, negative keeps the profile's
    double  skin_structure;   // --skin-structure, negative leaves it unset
    double  auto_mask;        // --auto-mask, negative leaves it unset
    double  detail_strength;  // --detail-strength
    double  colour_strength;  // --colour-strength
    double  detail_radius;    // --detail-radius
    int     frame_index;      // --frame-index
    int     frame_step;       // not nr_frame's: how far --frame-index advances per frame
    char  * control_mask;     // --control-mask, an image file, resized to the frame
    int     history;          // not nr_frame's: feed the previous prediction back in
} dlssnr_settings_t;

struct hb_filter_private_s
{
    hb_filter_init_t    input;
    hb_filter_init_t    output;

    dlssnr_settings_t   settings;

#if HB_PROJECT_FEATURE_DLSSNR
    // the extent the buffers are sized for
    int                 width;
    int                 height;
    int                 network_width;
    int                 network_height;

    // the frame as swscale sees it: planar float RGB (GBRPF32)
    struct SwsContext * to_rgb;
    struct SwsContext * from_rgb;
    uint8_t           * rgb_planes[3];
    int                 rgb_stride;

    // the frame as libdlssnr sees it: interleaved float RGB in [0, 1]
    float             * colour;        // this frame
    float             * composed;      // the composition
    float             * neural;        // the vendor's history, from this frame
    float             * history;       // the vendor's history, from the previous frame
    float             * previous;      // the previous frame's colour
    float             * gate_previous; // what the composition reads as `previous`
    float             * features;      // (network_height, network_width, 16)
    float             * wide;          // the head at the network's extent
    float             * head;          // the head cropped to the frame
    uint8_t           * moved;         // per-pixel change, dilated
    uint8_t           * dilate_scratch;
    int                 have_history;

    // the control mask, as loaded and as resized to the frame
    int                 use_mask;
    float             * mask;
    float             * mask_source;
    int                 mask_source_width;
    int                 mask_source_height;

    nr_frame_params     params;
    int                 frame_count;
    int                 acquired;      // this instance holds a reference to the model
    int                 failed;        // a pass failed: frames pass through, said once
#endif
};

static int dlssnr_init(hb_filter_object_t *filter,
                       hb_filter_init_t   *init);

static int dlssnr_work(hb_filter_object_t *filter,
                       hb_buffer_t ** buf_in,
                       hb_buffer_t ** buf_out);

static void dlssnr_close(hb_filter_object_t *filter);

static const char dlssnr_template[] =
    "intensity=^"HB_FLOAT_REG"$:profile=^"HB_ALL_REG"$:"
    "style-index=^"DLSSNR_SINT_REG"$:"
    "local-tone=^"DLSSNR_SFLOAT_REG"$:local-structure=^"DLSSNR_SFLOAT_REG"$:"
    "skin-structure=^"DLSSNR_SFLOAT_REG"$:auto-mask=^"DLSSNR_SFLOAT_REG"$:"
    "detail-strength=^"HB_FLOAT_REG"$:colour-strength=^"HB_FLOAT_REG"$:"
    "detail-radius=^"HB_FLOAT_REG"$:"
    "frame-index=^"HB_INT_REG"$:frame-step=^"HB_INT_REG"$:"
    "control-mask=^"HB_ALL_REG"$:history=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_dlssnr =
{
    .id                = HB_FILTER_DLSSNR,
    .enforce_order     = 1,
    .name              = "DLSS Neural Rendering (dlssnr)",
    .short_name        = "dlssnr",
    .settings          = NULL,
    .init              = dlssnr_init,
    .work              = dlssnr_work,
    .close             = dlssnr_close,
    .settings_template = dlssnr_template,
};

static int dlssnr_profile_from_string(const char *name)
{
    if (name == NULL || name[0] == 0)
    {
        return DLSSNR_PROFILE_STANDARD;
    }
    if (!strcasecmp(name, "standard"))  return DLSSNR_PROFILE_STANDARD;
    if (!strcasecmp(name, "natural"))   return DLSSNR_PROFILE_NATURAL;
    if (!strcasecmp(name, "cinematic")) return DLSSNR_PROFILE_CINEMATIC;
    if (!strcasecmp(name, "neutral"))   return DLSSNR_PROFILE_NEUTRAL;
    if (!strcasecmp(name, "vendor"))    return DLSSNR_PROFILE_VENDOR;

    char *end = NULL;
    long  index = strtol(name, &end, 10);
    if (end != name && *end == 0 && index >= 0 && index <= DLSSNR_PROFILE_VENDOR)
    {
        return (int)index;
    }
    hb_log("dlssnr: unknown profile (%s), using standard", name);
    return DLSSNR_PROFILE_STANDARD;
}

/* GSDLSSNR::Settings' defaults: nr_frame's own. */
static void dlssnr_settings_defaults(dlssnr_settings_t *s)
{
    s->intensity       = 1.0;
    s->profile         = DLSSNR_PROFILE_STANDARD;
    s->style_index     = -1;
    s->local_tone      = -1.0;
    s->local_structure = -1.0;
    s->skin_structure  = -1.0;
    s->auto_mask       = -1.0;
    s->detail_strength = 1.0;
    s->colour_strength = 1.0;
    s->detail_radius   = 4.0;
    s->frame_index     = 0;
    s->frame_step      = 0;
    s->control_mask    = NULL;
    s->history         = 1;
}

static void dlssnr_settings_read(dlssnr_settings_t *s, hb_dict_t *dict)
{
    char *profile = NULL;

    if (dict == NULL)
    {
        return;
    }
    hb_dict_extract_double(&s->intensity,       dict, "intensity");
    hb_dict_extract_string(&profile,            dict, "profile");
    hb_dict_extract_int   (&s->style_index,     dict, "style-index");
    hb_dict_extract_double(&s->local_tone,      dict, "local-tone");
    hb_dict_extract_double(&s->local_structure, dict, "local-structure");
    hb_dict_extract_double(&s->skin_structure,  dict, "skin-structure");
    hb_dict_extract_double(&s->auto_mask,       dict, "auto-mask");
    hb_dict_extract_double(&s->detail_strength, dict, "detail-strength");
    hb_dict_extract_double(&s->colour_strength, dict, "colour-strength");
    hb_dict_extract_double(&s->detail_radius,   dict, "detail-radius");
    hb_dict_extract_int   (&s->frame_index,     dict, "frame-index");
    hb_dict_extract_int   (&s->frame_step,      dict, "frame-step");
    hb_dict_extract_string(&s->control_mask,    dict, "control-mask");
    hb_dict_extract_bool  (&s->history,         dict, "history");

    if (profile != NULL)
    {
        s->profile = dlssnr_profile_from_string(profile);
        free(profile);
    }
    if (s->control_mask != NULL && s->control_mask[0] == 0)
    {
        free(s->control_mask);
        s->control_mask = NULL;
    }
}

#if HB_PROJECT_FEATURE_DLSSNR

/* -------------------------------------------------------------------------
 * The model, shared by every filter instance in the process. libdlssnr is one
 * device with process-wide state: `model_mutex` guards everything here and is
 * held for the length of a pass, and of a model open -- seconds, on a driver
 * that compiles pipelines slowly.
 * ---------------------------------------------------------------------- */

static pthread_mutex_t model_mutex = PTHREAD_MUTEX_INITIALIZER;
static nr_frame      * model_frame = NULL;
static int             model_users = 0;
// A model that couldn't open once won't open on the next job either, and the
// attempt costs seconds: say so once and don't try again.
static int             model_open_failed = 0;

static int dlssnr_model_acquire(void)
{
    int result = 0;

    pthread_mutex_lock(&model_mutex);
    if (model_frame == NULL)
    {
        if (model_open_failed)
        {
            hb_log("dlssnr: the model failed to open earlier in this process, "
                   "not trying again");
            result = -1;
        }
        else
        {
            hb_log("dlssnr: opening the model (%s runtime)", nr_frame_runtime());
            // NULL: the weights compiled into libdlssnr
            model_frame = nr_frame_open(NULL);
            if (model_frame == NULL)
            {
                const char *error = nr_frame_error();
                hb_error("dlssnr: nr_frame_open() failed: %s",
                         error != NULL && error[0] ? error : "unknown error");
                nr_frame_shutdown();
                model_open_failed = 1;
                result = -1;
            }
            else
            {
                hb_log("dlssnr: running on %s (%s runtime, %s)",
                       nr_frame_device(model_frame), nr_frame_runtime(),
                       nr_frame_gemm_path(model_frame));
            }
        }
    }
    if (result == 0)
    {
        model_users++;
    }
    pthread_mutex_unlock(&model_mutex);

    return result;
}

static void dlssnr_model_release(void)
{
    pthread_mutex_lock(&model_mutex);
    if (model_users > 0 && --model_users == 0 && model_frame != NULL)
    {
        nr_frame_close(model_frame);
        model_frame = NULL;
        // release the device libxmx (or the Metal runtime) holds
        nr_frame_shutdown();
        hb_log("dlssnr: model closed");
    }
    pthread_mutex_unlock(&model_mutex);
}

/* -------------------------------------------------------------------------
 * Parameters: GSDLSSNR's BuildParams, which is nr_frame_main.c's main() --
 * the defaults and the flags, then the profile, then the explicit overrides
 * -- with the gate's release and floor on.
 * ---------------------------------------------------------------------- */

// The temporal gate's knobs, nr_frame_live's defaults. nr_frame_defaults()
// leaves the release and the floor off, and the model's own gate is not
// local: it keeps about 0.6 of the previous prediction across the whole
// frame, so wherever something has just been, that much of it stays in the
// history -- a dark trail behind anything that moves. The release lets go of
// the history where the picture's own pixel changed since the frame before,
// all of it by DLSSNR_RELEASE_LEVELS; the floor holds it where nothing
// changed, falling away by DLSSNR_HOLD_RAMP_LEVELS.
#define DLSSNR_HOLD             1.0f
#define DLSSNR_HOLD_RAMP_LEVELS 4.0f
#define DLSSNR_RELEASE_LEVELS   24.0f

// How far, in pixels, a change to the picture lets go of the history around
// it. The network works from a pixel's surroundings, so its prediction for
// the ground beside a figure carries a halo of the figure's edges, and that
// goes into the history with it. When the figure moves on, those pixels have
// not changed, so a release that looks only at the pixel itself keeps the
// halo, and the floor holds it there pass after pass: a pale outline left
// behind everything that moves. Releasing by the largest change nearby
// instead lets the halo go with the figure that made it.
#define DLSSNR_RELEASE_RADIUS   8

// The mean change of the network's input, 0-1, above which the history is
// dropped rather than released pixel by pixel (nr_frame_live's cut limit): a
// cut, a transition, or a pan so fast that nothing in the history is still
// where it was.
#define DLSSNR_CUT_LIMIT        0.15

// nr_frame_main.c's PROFILES, applied over nr_frame_defaults().
static void dlssnr_apply_profile(nr_frame_params *p, int profile)
{
    switch (profile)
    {
        case DLSSNR_PROFILE_NATURAL:
            p->normalized_style = 1.0f / 128;
            p->local_tone       = 1.0f;
            p->local_structure  = 1.0f;
            break;
        case DLSSNR_PROFILE_CINEMATIC:
            p->normalized_style = 2.0f / 128;
            p->local_tone       = 1.0f;
            p->local_structure  = 1.0f;
            break;
        case DLSSNR_PROFILE_NEUTRAL:
            p->normalized_style = 0.0f;
            p->local_tone       = 0.0f;
            p->local_structure  = 0.0f;
            break;
        case DLSSNR_PROFILE_VENDOR:
            p->normalized_style = 0.0f;
            p->local_tone       = 1.0f;
            p->local_structure  = 1.5f;
            break;
        case DLSSNR_PROFILE_STANDARD:
        default:
            p->normalized_style = 0.0f;
            p->local_tone       = 1.0f;
            p->local_structure  = 1.0f;
            break;
    }
}

static void dlssnr_build_params(nr_frame_params *p, const dlssnr_settings_t *s)
{
    nr_frame_defaults(p);
    p->intensity       = (float)s->intensity;
    p->detail_strength = (float)s->detail_strength;
    p->colour_strength = (float)s->colour_strength;
    p->detail_radius   = (float)s->detail_radius;
    p->frame_index     = s->frame_index;
    p->hold            = DLSSNR_HOLD;
    p->slope           = -255.0f * DLSSNR_HOLD / DLSSNR_HOLD_RAMP_LEVELS;
    p->release         = -255.0f / DLSSNR_RELEASE_LEVELS;
    dlssnr_apply_profile(p, s->profile);
    if (s->style_index >= 0)
    {
        p->normalized_style = (float)s->style_index / 128.0f;
    }
    if (s->local_tone >= 0.0)
    {
        p->local_tone = (float)s->local_tone;
    }
    if (s->local_structure >= 0.0)
    {
        p->local_structure = (float)s->local_structure;
    }
    // --skin-structure / --auto-mask: either one turns the automatic mask on,
    // the other left at -1.
    const int have_skin = s->skin_structure >= 0.0;
    const int have_auto = s->auto_mask >= 0.0;
    if (have_skin || have_auto)
    {
        p->automatic_mask      = 1;
        p->skin_structure      = have_skin ? (float)s->skin_structure : -1.0f;
        p->automatic_structure = have_auto ? (float)s->auto_mask : -1.0f;
    }
}

/* -------------------------------------------------------------------------
 * Pixels: `save_image` and `read_png` in nr_frame_main.c
 * ---------------------------------------------------------------------- */

// clip to [0, 1], then `v * 255 + 0.5` to a byte
static inline uint8_t dlssnr_to_byte(float v)
{
    v = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
    return (uint8_t)(v * 255.0f + 0.5f);
}

// `byte / 255`, a division rather than a multiply by the reciprocal, so the
// float the network sees is the one the command gives it
static inline float dlssnr_from_byte(unsigned v)
{
    return (float)(v & 0xffu) / 255.0f;
}

static inline float dlssnr_clip01(float v)
{
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

/* -------------------------------------------------------------------------
 * nr_frame_main.c's resize(): `nr_image.bilinear`, one axis at a time
 * through nr_resize_axis, the axis plan mapping pixel centres onto the source.
 * ---------------------------------------------------------------------- */

static float * dlssnr_resize_bilinear(const float *source, int height, int width,
                                      int target_height, int target_width)
{
    const int channels = 3;
    size_t    count    = (size_t)height * width * channels;
    float   * current  = malloc(count * sizeof(float));
    float   * next     = NULL;
    int32_t * low      = NULL;
    int32_t * high     = NULL;
    float   * weight   = NULL;
    int       h = height, w = width;

    if (current == NULL)
    {
        return NULL;
    }
    memcpy(current, source, count * sizeof(float));

    for (int axis = 0; axis < 2; axis++)
    {
        const int extent = axis == 0 ? h : w;
        const int n      = axis == 0 ? target_height : target_width;
        if (extent == n)
        {
            continue;
        }
        low    = realloc(low,    n * sizeof(int32_t));
        high   = realloc(high,   n * sizeof(int32_t));
        weight = realloc(weight, n * sizeof(float));
        if (low == NULL || high == NULL || weight == NULL)
        {
            goto fail;
        }
        for (int i = 0; i < n; i++)
        {
            const float centre  = ((float)i + 0.5f) * ((float)extent / (float)n) - 0.5f;
            const float floored = floorf(centre);
            const int   lo      = (int)(floored < 0.0f ? 0.0f :
                                        floored > (float)(extent - 1) ? (float)(extent - 1) :
                                        floored);
            low[i]  = lo;
            high[i] = lo + 1 > extent - 1 ? extent - 1 : lo + 1;
            const float frac = centre - (float)lo;
            weight[i] = frac < 0.0f ? 0.0f : frac > 1.0f ? 1.0f : frac;
        }
        const int nh = axis == 0 ? n : h;
        const int nw = axis == 0 ? w : n;
        next = malloc((size_t)nh * nw * channels * sizeof(float));
        if (next == NULL)
        {
            goto fail;
        }
        nr_resize_axis(current, (ptrdiff_t)w * channels, channels, 1,
                       nh, nw, channels, axis, low, high, weight, next);
        free(current);
        current = next;
        next = NULL;
        h = nh;
        w = nw;
    }
    free(low);
    free(high);
    free(weight);
    return current;

fail:
    free(current);
    free(next);
    free(low);
    free(high);
    free(weight);
    return NULL;
}

/* -------------------------------------------------------------------------
 * The control mask: an image file, any format libavcodec reads, reduced to
 * float RGB as read_png() reduces a PNG to 8-bit RGB. Red scales the blend in
 * the composition, green the tone and blue the structure that reach the
 * network.
 * ---------------------------------------------------------------------- */

static float * dlssnr_load_mask(const char *path, int *width, int *height)
{
    AVFormatContext   * ic     = NULL;
    AVCodecContext    * ctx    = NULL;
    AVFrame           * frame  = NULL;
    AVPacket          * pkt    = NULL;
    struct SwsContext * sws    = NULL;
    const AVCodec     * codec  = NULL;
    float             * result = NULL;
    uint8_t           * planes[3] = { NULL, NULL, NULL };
    int                 stride = 0;
    int                 stream, got = 0;

    if (avformat_open_input(&ic, path, NULL, NULL) < 0)
    {
        hb_error("dlssnr: could not open the control mask %s", path);
        goto done;
    }
    if (avformat_find_stream_info(ic, NULL) < 0)
    {
        hb_error("dlssnr: could not read the control mask %s", path);
        goto done;
    }
    stream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream < 0 || codec == NULL)
    {
        hb_error("dlssnr: no picture in the control mask %s", path);
        goto done;
    }
    ctx = avcodec_alloc_context3(codec);
    if (ctx == NULL ||
        avcodec_parameters_to_context(ctx, ic->streams[stream]->codecpar) < 0 ||
        avcodec_open2(ctx, codec, NULL) < 0)
    {
        hb_error("dlssnr: could not decode the control mask %s", path);
        goto done;
    }
    pkt   = av_packet_alloc();
    frame = av_frame_alloc();
    if (pkt == NULL || frame == NULL)
    {
        goto done;
    }
    while (!got && av_read_frame(ic, pkt) >= 0)
    {
        if (pkt->stream_index == stream && avcodec_send_packet(ctx, pkt) >= 0 &&
            avcodec_receive_frame(ctx, frame) == 0)
        {
            got = 1;
        }
        av_packet_unref(pkt);
    }
    if (!got)
    {
        avcodec_send_packet(ctx, NULL);
        got = avcodec_receive_frame(ctx, frame) == 0;
    }
    if (!got || frame->width <= 0 || frame->height <= 0)
    {
        hb_error("dlssnr: could not decode the control mask %s", path);
        goto done;
    }

    const int w = frame->width, h = frame->height;
    stride = FFALIGN(w * (int)sizeof(float), 64);
    for (int c = 0; c < 3; c++)
    {
        planes[c] = av_malloc((size_t)stride * h);
        if (planes[c] == NULL)
        {
            goto done;
        }
    }
    int src_range = frame->color_range == AVCOL_RANGE_JPEG ||
                    (av_pix_fmt_desc_get(frame->format)->flags & AV_PIX_FMT_FLAG_RGB);
    sws = hb_sws_get_context(w, h, frame->format, src_range,
                             w, h, AV_PIX_FMT_GBRPF32, 1,
                             SWS_BILINEAR | SWS_ACCURATE_RND,
                             hb_sws_get_colorspace(frame->colorspace));
    if (sws == NULL)
    {
        hb_error("dlssnr: could not convert the control mask %s", path);
        goto done;
    }
    int strides[3] = { stride, stride, stride };
    sws_scale(sws, (const uint8_t * const *)frame->data, frame->linesize, 0, h,
              planes, strides);

    result = malloc((size_t)w * h * 3 * sizeof(float));
    if (result == NULL)
    {
        goto done;
    }
    for (int y = 0; y < h; y++)
    {
        const float *g = (const float *)(planes[0] + (size_t)y * stride);
        const float *b = (const float *)(planes[1] + (size_t)y * stride);
        const float *r = (const float *)(planes[2] + (size_t)y * stride);
        float       *o = result + (size_t)y * w * 3;
        for (int x = 0; x < w; x++)
        {
            // as read_png(): byte / 255 of R, G, B
            o[3 * x + 0] = dlssnr_from_byte(dlssnr_to_byte(r[x]));
            o[3 * x + 1] = dlssnr_from_byte(dlssnr_to_byte(g[x]));
            o[3 * x + 2] = dlssnr_from_byte(dlssnr_to_byte(b[x]));
        }
    }
    *width  = w;
    *height = h;
    hb_log("dlssnr: control mask %s (%dx%d)", path, w, h);

done:
    for (int c = 0; c < 3; c++)
    {
        av_free(planes[c]);
    }
    if (sws != NULL)
    {
        sws_freeContext(sws);
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    avformat_close_input(&ic);
    return result;
}

// Resizes the mask when the frame's extent changes: nr_frame refuses a mask of
// another shape, so this resizes it the way nr_frame's --size resizes a picture.
static int dlssnr_prepare_mask(hb_filter_private_t *pv)
{
    if (pv->mask_source == NULL)
    {
        pv->use_mask = 0;
        return 0;
    }
    free(pv->mask);
    pv->mask = dlssnr_resize_bilinear(pv->mask_source,
                                      pv->mask_source_height, pv->mask_source_width,
                                      pv->height, pv->width);
    pv->use_mask = pv->mask != NULL;
    return pv->mask != NULL ? 0 : -1;
}

/* -------------------------------------------------------------------------
 * The history: GSDLSSNR's ReleaseHistory. Drops the history on a cut, and
 * otherwise lets go of it where the picture changed nearby since the frame
 * before.
 *
 * The history goes to the composition only, never to the features, though
 * nr_frame takes it in both. Handed to the network, it closes a loop: the
 * network darkens and sharpens what it is shown, the result is the next
 * history, and on a picture holding still the darkening compounds pass after
 * pass. The composition alone only averages the model's successive answers,
 * which is what the history is for, and cannot drift.
 *
 * The composition releases and floors its blend by the change at each pixel
 * alone, measured against `previous` -- the one thing it reads `previous`
 * for. So it is handed a stand-in that differs from the colour by the change
 * nearby instead, and its gate then lets go of the halo around a figure as
 * well as the figure.
 * ---------------------------------------------------------------------- */

static inline void dlssnr_max_into(uint8_t *out, const uint8_t *in, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (in[i] > out[i])
        {
            out[i] = in[i];
        }
    }
}

// a square dilation of radius r: across into `scratch`, then down back into `image`
static void dlssnr_dilate(uint8_t *image, uint8_t *scratch, int width, int height, int r)
{
    for (int y = 0; y < height; y++)
    {
        const uint8_t *row = image   + (size_t)y * width;
        uint8_t       *out = scratch + (size_t)y * width;
        memcpy(out, row, (size_t)width);
        for (int o = 1; o <= r; o++)
        {
            if (width - o <= 0)
            {
                break;
            }
            dlssnr_max_into(out, row + o, width - o);
            dlssnr_max_into(out + o, row, width - o);
        }
    }
    for (int y = 0; y < height; y++)
    {
        uint8_t  *out = image + (size_t)y * width;
        const int y0  = y - r < 0 ? 0 : y - r;
        const int y1  = y + r > height - 1 ? height - 1 : y + r;
        memcpy(out, scratch + (size_t)y0 * width, (size_t)width);
        for (int q = y0 + 1; q <= y1; q++)
        {
            dlssnr_max_into(out, scratch + (size_t)q * width, width);
        }
    }
}

// returns 1 when the history is usable this frame, 0 when it is dropped
static int dlssnr_release_history(hb_filter_private_t *pv)
{
    const int     w      = pv->width;
    const int     h      = pv->height;
    const size_t  pixels = (size_t)w * h;
    const float * c      = pv->colour;
    const float * b      = pv->previous;
    double        cut    = 0.0;

    for (size_t i = 0; i < pixels * 3; i++)
    {
        cut += fabs((double)c[i] - b[i]);
    }
    if (pixels && cut / (double)(pixels * 3) > DLSSNR_CUT_LIMIT)
    {
        hb_deep_log(2, "dlssnr: cut at frame %d (change %.3f), history dropped",
                    pv->frame_count, cut / (double)(pixels * 3));
        return 0;
    }

    for (size_t i = 0; i < pixels; i++)
    {
        float m = 0.0f;
        for (int k = 0; k < 3; k++)
        {
            const float d = fabsf(c[3 * i + k] - b[3 * i + k]);
            m = d > m ? d : m;
        }
        pv->moved[i] = dlssnr_to_byte(m);
    }
    dlssnr_dilate(pv->moved, pv->dilate_scratch, w, h, DLSSNR_RELEASE_RADIUS);

    for (size_t i = 0; i < pixels; i++)
    {
        const float m = dlssnr_from_byte(pv->moved[i]);
        for (int k = 0; k < 3; k++)
        {
            const float ci = c[3 * i + k];
            pv->gate_previous[3 * i + k] = ci >= m ? ci - m : ci + m;
        }
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * Buffers, sized for the frame's extent
 * ---------------------------------------------------------------------- */

static void dlssnr_free_buffers(hb_filter_private_t *pv)
{
    if (pv->to_rgb != NULL)
    {
        sws_freeContext(pv->to_rgb);
        pv->to_rgb = NULL;
    }
    if (pv->from_rgb != NULL)
    {
        sws_freeContext(pv->from_rgb);
        pv->from_rgb = NULL;
    }
    for (int c = 0; c < 3; c++)
    {
        av_freep(&pv->rgb_planes[c]);
    }
    free(pv->colour);         pv->colour = NULL;
    free(pv->composed);       pv->composed = NULL;
    free(pv->neural);         pv->neural = NULL;
    free(pv->history);        pv->history = NULL;
    free(pv->previous);       pv->previous = NULL;
    free(pv->gate_previous);  pv->gate_previous = NULL;
    free(pv->features);       pv->features = NULL;
    free(pv->wide);           pv->wide = NULL;
    free(pv->head);           pv->head = NULL;
    free(pv->moved);          pv->moved = NULL;
    free(pv->dilate_scratch); pv->dilate_scratch = NULL;
    free(pv->mask);           pv->mask = NULL;
    pv->use_mask     = 0;
    pv->have_history = 0;
    pv->width = pv->height = 0;
}

static int dlssnr_alloc_buffers(hb_filter_private_t *pv, int width, int height)
{
    dlssnr_free_buffers(pv);

    const size_t pixels = (size_t)width * height;

    pv->width  = width;
    pv->height = height;
    // the network's extent, as the vendor pads it, at the parameters' floor
    nr_frame_geometry_min(height, width, pv->params.min_extent,
                          &pv->network_height, &pv->network_width);
    const size_t network = (size_t)pv->network_height * pv->network_width;

    pv->rgb_stride = FFALIGN(width * (int)sizeof(float), 64);
    for (int c = 0; c < 3; c++)
    {
        pv->rgb_planes[c] = av_malloc((size_t)pv->rgb_stride * height);
        if (pv->rgb_planes[c] == NULL)
        {
            goto fail;
        }
    }

    const int src_full = pv->input.color_range == AVCOL_RANGE_JPEG;
    const int colorspace = hb_sws_get_colorspace(pv->input.color_matrix);
    pv->to_rgb = hb_sws_get_context(width, height, pv->input.pix_fmt, src_full,
                                    width, height, AV_PIX_FMT_GBRPF32, 1,
                                    SWS_LANCZOS | SWS_ACCURATE_RND, colorspace);
    pv->from_rgb = hb_sws_get_context(width, height, AV_PIX_FMT_GBRPF32, 1,
                                      width, height, pv->output.pix_fmt, src_full,
                                      SWS_LANCZOS | SWS_ACCURATE_RND, colorspace);
    if (pv->to_rgb == NULL || pv->from_rgb == NULL)
    {
        hb_error("dlssnr: swscale could not convert %s to float RGB and back",
                 av_get_pix_fmt_name(pv->input.pix_fmt));
        goto fail;
    }

    pv->colour         = malloc(pixels * 3 * sizeof(float));
    pv->composed       = malloc(pixels * 3 * sizeof(float));
    pv->neural         = malloc(pixels * 3 * sizeof(float));
    pv->history        = malloc(pixels * 3 * sizeof(float));
    pv->previous       = malloc(pixels * 3 * sizeof(float));
    pv->gate_previous  = malloc(pixels * 3 * sizeof(float));
    pv->features       = malloc(network * 16 * sizeof(float));
    pv->wide           = malloc(network * 4 * sizeof(float));
    pv->head           = malloc(pixels * 4 * sizeof(float));
    pv->moved          = malloc(pixels);
    pv->dilate_scratch = malloc(pixels);
    if (pv->colour == NULL || pv->composed == NULL || pv->neural == NULL ||
        pv->history == NULL || pv->previous == NULL || pv->gate_previous == NULL ||
        pv->features == NULL || pv->wide == NULL || pv->head == NULL ||
        pv->moved == NULL || pv->dilate_scratch == NULL)
    {
        goto fail;
    }

    if (dlssnr_prepare_mask(pv) != 0)
    {
        goto fail;
    }

    hb_log("dlssnr: %dx%d frame, %dx%d network field, %s, history %s",
           width, height, pv->network_width, pv->network_height,
           av_get_pix_fmt_name(pv->input.pix_fmt),
           pv->settings.history ? "on" : "off");
    return 0;

fail:
    hb_error("dlssnr: out of memory sizing buffers for %dx%d", width, height);
    dlssnr_free_buffers(pv);
    return -1;
}

// The planes as the buffer carries them. A decoder's frame has its planes
// wherever libavcodec put them, so hb_picture_fill(), which lays them out
// end to end from buf->data, hands back nothing useful for it.
static void dlssnr_planes(uint8_t *data[4], int stride[4], hb_buffer_t *b)
{
    for (int c = 0; c < 4; c++)
    {
        data[c]   = c <= b->f.max_plane ? b->plane[c].data   : NULL;
        stride[c] = c <= b->f.max_plane ? b->plane[c].stride : 0;
    }
}

// min, max and mean of a float array, for the first frame's diagnostics
static void dlssnr_stats(const char *name, const float *a, size_t n, int frame)
{
    double mn = 1e30, mx = -1e30, sum = 0;
    size_t bad = 0;
    for (size_t i = 0; i < n; i++)
    {
        float v = a[i];
        if (isnan(v) || isinf(v)) { bad++; continue; }
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        sum += v;
    }
    hb_deep_log(2, "dlssnr: frame %d %s: %zu values, %zu nan/inf, min %g max %g mean %g",
                frame, name, n, bad, mn, mx, n > bad ? sum / (double)(n - bad) : 0.0);
}

/* -------------------------------------------------------------------------
 * One frame: the three stages of nr_frame_main.c, in order
 * ---------------------------------------------------------------------- */

static int dlssnr_process(hb_filter_private_t *pv, hb_buffer_t *in, hb_buffer_t *out)
{
    const int    w = in->f.width;
    const int    h = in->f.height;
    const size_t pixels = (size_t)w * h;

    if (w != pv->width || h != pv->height)
    {
        if (dlssnr_alloc_buffers(pv, w, h) != 0)
        {
            return -1;
        }
    }

    // 1. the picture as float RGB in [0, 1]
    uint8_t * in_data[4];
    int       in_stride[4];
    int       rgb_stride[3] = { pv->rgb_stride, pv->rgb_stride, pv->rgb_stride };
    dlssnr_planes(in_data, in_stride, in);
    sws_scale(pv->to_rgb, (const uint8_t * const *)in_data, in_stride, 0, h,
              pv->rgb_planes, rgb_stride);
    for (int y = 0; y < h; y++)
    {
        const float *g = (const float *)(pv->rgb_planes[0] + (size_t)y * pv->rgb_stride);
        const float *b = (const float *)(pv->rgb_planes[1] + (size_t)y * pv->rgb_stride);
        const float *r = (const float *)(pv->rgb_planes[2] + (size_t)y * pv->rgb_stride);
        float       *c = pv->colour + (size_t)y * w * 3;
        for (int x = 0; x < w; x++)
        {
            c[3 * x + 0] = dlssnr_clip01(r[x]);
            c[3 * x + 1] = dlssnr_clip01(g[x]);
            c[3 * x + 2] = dlssnr_clip01(b[x]);
        }
    }

    if (pv->frame_count == 0)
    {
        dlssnr_stats("colour in", pv->colour, pixels * 3, pv->frame_count);
    }

    // 2. this frame's parameters
    nr_frame_params params = pv->params;
    params.frame_index = pv->settings.frame_index +
                         pv->frame_count * pv->settings.frame_step;

    // 3. the history, released where the picture moved, dropped on a cut
    int use_history = pv->settings.history && pv->have_history &&
                      dlssnr_release_history(pv);

    // 4. features, the network, the composition -- one pass at a time in the
    //    model, whose graph is prepared for one extent
    const float *mask = pv->use_mask ? pv->mask : NULL;
    const char  *failure = NULL;
    pthread_mutex_lock(&model_mutex);
    nr_frame *frame = model_frame;
    if (frame == NULL)
    {
        failure = "the model is not open";
    }
    // No history: see dlssnr_release_history().
    else if (nr_frame_features_masked(frame, pv->colour, h, w, NULL, mask, &params,
                                      pv->features) != 0)
    {
        failure = "nr_frame_features_masked() failed";
    }
    else if (nr_frame_run_features(frame, pv->features, pv->network_height,
                                   pv->network_width, pv->wide) != 0)
    {
        failure = "nr_frame_run_features() failed";
    }
    else
    {
        // geometry.crop(head)
        for (int y = 0; y < h; y++)
        {
            memcpy(pv->head + (size_t)y * w * 4,
                   pv->wide + (size_t)y * pv->network_width * 4,
                   (size_t)w * 4 * sizeof(float));
        }
        const float *hist = use_history ? pv->history : NULL;
        const float *prev = use_history ? pv->gate_previous : NULL;
        // `_neural` for the prediction as well as the picture: the picture is
        // what goes out, the prediction is what the next frame's history is
        if (nr_frame_compose_neural(frame, pv->head, pv->colour, h, w, hist, prev,
                                    mask, &params, pv->composed, pv->neural) != 0)
        {
            failure = "nr_frame_compose_neural() failed";
        }
    }
    if (failure != NULL)
    {
        const char *error = nr_frame_error();
        hb_error("dlssnr: %s: %s", failure,
                 error != NULL && error[0] ? error : "unknown error");
    }
    pthread_mutex_unlock(&model_mutex);
    if (failure != NULL)
    {
        pv->have_history = 0;
        return -1;
    }

    if (pv->frame_count == 0)
    {
        dlssnr_stats("head", pv->head, pixels * 4, pv->frame_count);
        dlssnr_stats("composed", pv->composed, pixels * 3, pv->frame_count);
    }

    // 5. the composition back into the pipeline's pixel format
    for (int y = 0; y < h; y++)
    {
        float       *g = (float *)(pv->rgb_planes[0] + (size_t)y * pv->rgb_stride);
        float       *b = (float *)(pv->rgb_planes[1] + (size_t)y * pv->rgb_stride);
        float       *r = (float *)(pv->rgb_planes[2] + (size_t)y * pv->rgb_stride);
        const float *o = pv->composed + (size_t)y * w * 3;
        for (int x = 0; x < w; x++)
        {
            r[x] = dlssnr_clip01(o[3 * x + 0]);
            g[x] = dlssnr_clip01(o[3 * x + 1]);
            b[x] = dlssnr_clip01(o[3 * x + 2]);
        }
    }
    uint8_t * out_data[4];
    int       out_stride[4];
    dlssnr_planes(out_data, out_stride, out);
    int rows = sws_scale(pv->from_rgb, (const uint8_t * const *)pv->rgb_planes, rgb_stride,
                         0, h, out_data, out_stride);
    if (pv->frame_count == 0)
    {
        hb_deep_log(2, "dlssnr: frame 0 back to %s: %d rows, in Y[%d][%d] %d -> out %d, "
                    "in stride %d out stride %d",
                    av_get_pix_fmt_name(pv->output.pix_fmt), rows, h / 2, w / 2,
                    in_data[0][(size_t)(h / 2) * in_stride[0] + w / 2],
                    out_data[0][(size_t)(h / 2) * out_stride[0] + w / 2],
                    in_stride[0], out_stride[0]);
    }

    // 6. what the next frame needs: this prediction as its history, this
    //    picture as its previous -- swapped in, not copied
    if (pv->settings.history)
    {
        float *t;
        t = pv->history;  pv->history  = pv->neural;  pv->neural   = t;
        t = pv->previous; pv->previous = pv->colour;  pv->colour   = t;
        pv->have_history = 1;
    }
    (void)pixels;
    pv->frame_count++;
    return 0;
}

#endif // HB_PROJECT_FEATURE_DLSSNR

/* -------------------------------------------------------------------------
 * The filter
 * ---------------------------------------------------------------------- */

static int dlssnr_init(hb_filter_object_t *filter,
                       hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("dlssnr: calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;

    pv->input = *init;
    dlssnr_settings_defaults(&pv->settings);
    dlssnr_settings_read(&pv->settings, filter->settings);

#if HB_PROJECT_FEATURE_DLSSNR
    dlssnr_build_params(&pv->params, &pv->settings);

    if (pv->settings.control_mask != NULL)
    {
        pv->mask_source = dlssnr_load_mask(pv->settings.control_mask,
                                           &pv->mask_source_width,
                                           &pv->mask_source_height);
        if (pv->mask_source == NULL)
        {
            // said already; the pass runs without it
        }
    }

    if (dlssnr_model_acquire() != 0)
    {
        return -1;
    }
    pv->acquired = 1;

    hb_log("dlssnr: profile %d, intensity %.3f, detail %.3f, colour %.3f, "
           "radius %.2f, style %d, tone %.2f, structure %.2f, frame index %d (+%d), "
           "automatic mask %s, control mask %s, history %s",
           pv->settings.profile, pv->params.intensity, pv->params.detail_strength,
           pv->params.colour_strength, pv->params.detail_radius,
           (int)lroundf(pv->params.normalized_style * 128.0f),
           pv->params.local_tone, pv->params.local_structure,
           pv->settings.frame_index, pv->settings.frame_step,
           pv->params.automatic_mask ? "on" : "off",
           pv->mask_source != NULL ? pv->settings.control_mask : "none",
           pv->settings.history ? "on" : "off");

    pv->output = *init;
    return 0;
#else
    hb_error("dlssnr: this build of HandBrake was configured without DLSS-NR "
             "(--enable-dlssnr)");
    return -1;
#endif
}

static void dlssnr_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }
#if HB_PROJECT_FEATURE_DLSSNR
    dlssnr_free_buffers(pv);
    free(pv->mask_source);
    if (pv->acquired)
    {
        dlssnr_model_release();
    }
#endif
    free(pv->settings.control_mask);
    free(pv);
    filter->private_data = NULL;
}

static int dlssnr_work(hb_filter_object_t *filter,
                       hb_buffer_t ** buf_in,
                       hb_buffer_t ** buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

#if HB_PROJECT_FEATURE_DLSSNR
    if (!pv->failed)
    {
        hb_buffer_t *out = hb_frame_buffer_init(pv->output.pix_fmt,
                                                in->f.width, in->f.height);
        if (out == NULL)
        {
            hb_error("dlssnr: could not allocate an output frame");
            pv->failed = 1;
        }
        else
        {
            out->f.color_prim      = pv->output.color_prim;
            out->f.color_transfer  = pv->output.color_transfer;
            out->f.color_matrix    = pv->output.color_matrix;
            out->f.color_range     = pv->output.color_range;
            out->f.chroma_location = pv->output.chroma_location;

            if (dlssnr_process(pv, in, out) == 0)
            {
                hb_buffer_copy_props(out, in);
                *buf_out = out;
                return HB_FILTER_OK;
            }
            hb_buffer_close(&out);
            hb_error("dlssnr: the pass failed at frame %d; the rest of the "
                     "video passes through untouched", pv->frame_count);
            pv->failed = 1;
        }
    }
#endif

    // the frame as it came
    *buf_out = in;
    *buf_in = NULL;
    return HB_FILTER_OK;
}
