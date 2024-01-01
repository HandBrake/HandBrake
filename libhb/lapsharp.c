/* lapsharp.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"

#define LAPSHARP_STRENGTH_LUMA_DEFAULT   0.2
#define LAPSHARP_STRENGTH_CHROMA_DEFAULT 0.2

#define LAPSHARP_KERNELS 4
#define LAPSHARP_KERNEL_LUMA_DEFAULT   2
#define LAPSHARP_KERNEL_CHROMA_DEFAULT 2

typedef struct
{
    int        bps;
    int        max_value;

    double strength;  // strength
    int    kernel;    // which kernel to use; kernels[kernel]
} lapsharp_plane_context_t;

typedef struct {
    const int   *mem;
    const int    size;
    const double coef;
} kernel_t;

// 4-neighbor Laplacian kernel (lap)
// Sharpens vertical and horizontal edges, less effective on diagonals
// size = 3, coef = 1.0
static const int    kernel_lap[] =
{
 0, -1,  0,
-1,  5, -1,
 0, -1,  0
};

// Isotropic Laplacian kernel (isolap)
// Minimal directionality, sharpens all edges similarly
// size = 3, coef = 1.0 / 5
static const int    kernel_isolap[] =
{
-1, -4, -1,
-4, 25, -4,
-1, -4, -1
};

// Laplacian of Gaussian kernel (log)
// Slight noise and grain rejection
// σ ~= 1
// size = 5, coef = 1.0 / 5
static const int    kernel_log[] =
{
 0,  0, -1,  0,  0,
 0, -1, -2, -1,  0,
-1, -2, 21, -2, -1,
 0, -1, -2, -1,  0,
 0,  0, -1,  0,  0
};

// Isotropic Laplacian of Gaussian kernel (isolog)
// Minimal directionality, plus noise and grain rejection
// σ ~= 1.2
// size = 5, coef = 1.0 / 15
static const int    kernel_isolog[] =
{
 0, -1, -1, -1,  0,
-1, -3, -4, -3, -1,
-1, -4, 55, -4, -1,
-1, -3, -4, -3, -1,
 0, -1, -1, -1,  0
};

static kernel_t kernels[] =
{
    { kernel_lap,    3, 1.0      },
    { kernel_isolap, 3, 1.0 /  5 },
    { kernel_log,    5, 1.0 /  5 },
    { kernel_isolog, 5, 1.0 / 15 }
};

struct hb_filter_private_s
{
    int depth;

    lapsharp_plane_context_t plane_ctx[3];

    hb_filter_init_t         input;
    hb_filter_init_t         output;
};

static int hb_lapsharp_init(hb_filter_object_t *filter,
                            hb_filter_init_t   *init);

static int hb_lapsharp_work(hb_filter_object_t *filter,
                            hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out);

static void hb_lapsharp_close(hb_filter_object_t *filter);

static const char hb_lapsharp_template[] =
    "y-strength=^"HB_FLOAT_REG"$:y-kernel=^"HB_ALL_REG"$:"
    "cb-strength=^"HB_FLOAT_REG"$:cb-kernel=^"HB_ALL_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-kernel=^"HB_ALL_REG"$";

hb_filter_object_t hb_filter_lapsharp =
{
    .id                = HB_FILTER_LAPSHARP,
    .enforce_order     = 1,
    .name              = "Sharpen (lapsharp)",
    .settings          = NULL,
    .init              = hb_lapsharp_init,
    .work              = hb_lapsharp_work,
    .close             = hb_lapsharp_close,
    .settings_template = hb_lapsharp_template,
};

#define DEF_LAPSHARP_FUNC(name, nbits, pixelbits)                                                \
static void name##_##nbits(const uint8_t *frame_src,                                             \
                                 uint8_t *frame_dst,                                             \
                           const int width,                                                      \
                           const int height,                                                     \
                           int stride_src,                                                       \
                           int stride_dst,                                                       \
                           lapsharp_plane_context_t *ctx)                                        \
{                                                                                                \
    const kernel_t *kernel = &kernels[ctx->kernel];                                              \
                                                                                                 \
    const uint##nbits##_t *src = (const uint##nbits##_t *)frame_src;                             \
    uint##nbits##_t       *dst = (uint##nbits##_t *)frame_dst;                                   \
                                                                                                 \
    stride_src /= ctx->bps;                                                                      \
    stride_dst /= ctx->bps;                                                                      \
                                                                                                 \
    /* Sharpen using selected kernel */                                                          \
    const int offset_min    = -((kernel->size - 1) / 2);                                         \
    const int offset_max    =   (kernel->size + 1) / 2;                                          \
    const int stride_border =   (stride_src - width) / 2;                                        \
    const int max_value     = ctx->max_value;                                                    \
                                                                                                 \
    int##pixelbits##_t pixel;                                                                    \
                                                                                                 \
    for (int y = 0; y < height; y++)                                                             \
    {                                                                                            \
        for (int x = 0; x < width; x++)                                                          \
        {                                                                                        \
            if ((y < offset_max) ||                                                              \
                (y > height - offset_max) ||                                                     \
                (x < stride_border + offset_max) ||                                              \
                (x > width + stride_border - offset_max))                                        \
            {                                                                                    \
                *(dst + stride_dst*y + x) = *(src + stride_src*y + x);                           \
                continue;                                                                        \
            }                                                                                    \
                                                                                                 \
            pixel = 0;                                                                           \
            for (int k = offset_min; k < offset_max; k++)                                        \
            {                                                                                    \
                for (int j = offset_min; j < offset_max; j++)                                    \
                {                                                                                \
                    pixel += kernel->mem[((j - offset_min) * kernel->size) +                     \
                             k - offset_min] * *(src + stride_src*(y + j) + (x + k));            \
                }                                                                                \
            }                                                                                    \
            pixel = (int##pixelbits##_t)(((pixel * kernel->coef) - *(src + stride_src*y + x)) *  \
                        ctx->strength) + *(src + stride_src*y + x);                              \
            pixel = pixel < 0 ? 0 : pixel;                                                       \
            pixel = pixel > max_value ? max_value : pixel;                                       \
            *(dst + stride_dst*y + x) = (uint##nbits##_t)(pixel);                                \
        }                                                                                        \
    }                                                                                            \
}                                                                                                \

DEF_LAPSHARP_FUNC(lapsharp, 16, 32)
DEF_LAPSHARP_FUNC(lapsharp, 8, 16)

#define hb_lapsharp(...)                               \
    switch (pv->depth)                                 \
    {                                                  \
        case  8: lapsharp_8(__VA_ARGS__); break;       \
        default: lapsharp_16(__VA_ARGS__); break;      \
    }                                                  \


static int hb_lapsharp_init(hb_filter_object_t *filter,
                            hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("lapsharp: calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;

    char *kernel_string[3];

    pv->input = *init;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->depth = desc->comp[0].depth;

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->plane_ctx[c].strength = -1;
        pv->plane_ctx[c].kernel   = -1;
        kernel_string[c]          = NULL;
    }

    // Read user parameters
    if (filter->settings != NULL)
    {
        hb_dict_t * dict = filter->settings;
        hb_dict_extract_double(&pv->plane_ctx[0].strength, dict, "y-strength");
        hb_dict_extract_string(&kernel_string[0],          dict, "y-kernel");

        hb_dict_extract_double(&pv->plane_ctx[1].strength, dict, "cb-strength");
        hb_dict_extract_string(&kernel_string[1],          dict, "cb-kernel");

        hb_dict_extract_double(&pv->plane_ctx[2].strength, dict, "cr-strength");
        hb_dict_extract_string(&kernel_string[2],          dict, "cr-kernel");
    }

    // Convert kernel user string to internal id
    for (int c = 0; c < 3; c++)
    {
        lapsharp_plane_context_t * ctx = &pv->plane_ctx[c];

        ctx->bps = pv->depth > 8 ? 2 : 1;
        ctx->max_value = (1 << pv->depth) - 1;
        ctx->kernel = -1;

        if (kernel_string[c] == NULL)
        {
            continue;
        }

        if (!strcasecmp(kernel_string[c], "lap"))
        {
            ctx->kernel = 0;
        }
        else if (!strcasecmp(kernel_string[c], "isolap"))
        {
            ctx->kernel = 1;
        }
        else if (!strcasecmp(kernel_string[c], "log"))
        {
            ctx->kernel = 2;
        }
        else if (!strcasecmp(kernel_string[c], "isolog"))
        {
            ctx->kernel = 3;
        }

        free(kernel_string[c]);
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        lapsharp_plane_context_t * prev_ctx = &pv->plane_ctx[c - 1];
        lapsharp_plane_context_t * ctx      = &pv->plane_ctx[c];

        if (ctx->strength == -1) ctx->strength = prev_ctx->strength;
        if (ctx->kernel   == -1) ctx->kernel   = prev_ctx->kernel;
    }

    for (int c = 0; c < 3; c++)
    {
        lapsharp_plane_context_t * ctx = &pv->plane_ctx[c];

        // Replace unset values with defaults
        if (ctx->strength == -1)
        {
            ctx->strength = c ? LAPSHARP_STRENGTH_CHROMA_DEFAULT :
                                LAPSHARP_STRENGTH_LUMA_DEFAULT;
        }
        if (ctx->kernel   == -1)
        {
            ctx->kernel   = c ? LAPSHARP_KERNEL_CHROMA_DEFAULT :
                                LAPSHARP_KERNEL_LUMA_DEFAULT;
        }

        // Sanitize
        if (ctx->strength < 0)   ctx->strength = 0;
        if (ctx->strength > 1.5) ctx->strength = 1.5;
        if ((ctx->kernel < 0) || (ctx->kernel >= LAPSHARP_KERNELS))
        {
            ctx->kernel = c ? LAPSHARP_KERNEL_CHROMA_DEFAULT : LAPSHARP_KERNEL_LUMA_DEFAULT;
        }
    }
    pv->output = *init;

    return 0;
}

static void hb_lapsharp_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    free(pv);
    filter->private_data = NULL;
}

static int hb_lapsharp_work(hb_filter_object_t *filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    hb_frame_buffer_mirror_stride(in);
    out = hb_frame_buffer_init(pv->output.pix_fmt, in->f.width, in->f.height);
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;

    int c;
    for (c = 0; c < 3; c++)
    {
        lapsharp_plane_context_t * ctx = &pv->plane_ctx[c];
        hb_lapsharp(in->plane[c].data,
                    out->plane[c].data,
                    in->plane[c].width,
                    in->plane[c].height,
                    in->plane[c].stride,
                    out->plane[c].stride,
                    ctx);
    }

    hb_buffer_copy_props(out, in);
    *buf_out = out;

    return HB_FILTER_OK;
}
