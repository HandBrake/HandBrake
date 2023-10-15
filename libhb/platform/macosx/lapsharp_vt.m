/* lapsharp_vt.m

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_lapsharp_vt_metallib_data[];
extern unsigned int hb_lapsharp_vt_metallib_len;

#define LAPSHARP_STRENGTH_LUMA_DEFAULT   0.2
#define LAPSHARP_STRENGTH_CHROMA_DEFAULT 0.2

#define LAPSHARP_KERNELS 4
#define LAPSHARP_KERNEL_LUMA_DEFAULT   2
#define LAPSHARP_KERNEL_CHROMA_DEFAULT 2

typedef struct
{
    uint   channels;
    float  strength;  // strength
    uint   size;
    float  coef;
    int    kernel;
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
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

    lapsharp_plane_context_t    plane_ctx[3];
    id<MTLBuffer>               mem[3];

    hb_filter_init_t            input;
    hb_filter_init_t            output;
};

static int lapsharp_vt_init(hb_filter_object_t *filter,
                           hb_filter_init_t   *init);

static int lapsharp_vt_work(hb_filter_object_t *filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out);

static void lapsharp_vt_close(hb_filter_object_t *filter);

static const char lapsharp_vt_template[] =
    "y-strength=^"HB_FLOAT_REG"$:y-size=^"HB_INT_REG"$:"
    "cb-strength=^"HB_FLOAT_REG"$:cb-size=^"HB_INT_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-size=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_lapsharp_vt =
{
    .id                = HB_FILTER_LAPSHARP_VT,
    .enforce_order     = 1,
    .name              = "Lapsharp (VideoToolbox)",
    .settings          = NULL,
    .init              = lapsharp_vt_init,
    .work              = lapsharp_vt_work,
    .close             = lapsharp_vt_close,
    .settings_template = lapsharp_vt_template,
};

static int lapsharp_vt_init(hb_filter_object_t *filter,
                        hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("lapsharp_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;
    pv->input = *init;
    pv->desc = av_pix_fmt_desc_get(init->pix_fmt);

    pv->mtl = hb_metal_context_init(hb_lapsharp_vt_metallib_data,
                                    hb_lapsharp_vt_metallib_len,
                                    "lapsharp",
                                    sizeof(lapsharp_plane_context_t),
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("lapsharp_vt: failed to create Metal device");
        return -1;
    }

    char *kernel_string[3];

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
        hb_dict_t *dict = filter->settings;
        double val;
        hb_dict_extract_double(&val, dict, "y-strength");
        hb_dict_extract_string(&kernel_string[0], dict, "y-kernel");
        pv->plane_ctx[0].strength = val;

        hb_dict_extract_double(&val, dict, "cb-strength");
        hb_dict_extract_string(&kernel_string[1], dict, "cb-kernel");
        pv->plane_ctx[1].strength = val;

        hb_dict_extract_double(&val, dict, "cr-strength");
        hb_dict_extract_string(&kernel_string[2], dict, "cr-kernel");
        pv->plane_ctx[2].strength = val;
    }

    for (int c = 0; c < 3; c++)
    {
        lapsharp_plane_context_t * ctx = &pv->plane_ctx[c];

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

        kernel_t *kernel = &kernels[ctx->kernel];
        NSUInteger length = kernel->size * kernel->size * sizeof(int);

        pv->mem[c] = [pv->mtl->device newBufferWithLength:length
                                                  options:MTLResourceStorageModeManaged];
        memcpy(pv->mem[c].contents, kernel->mem, length);
        [pv->mem[c] didModifyRange:NSMakeRange(0, length)];

        ctx->size = kernel->size;
        ctx->coef = kernel->coef;
    }

    pv->output = *init;

    return 0;
}

static void lapsharp_vt_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_metal_context_close(&pv->mtl);

    for (int i = 0; i < 3; i++)
    {
        [pv->mem[i] release];
    }

    free(pv);
    filter->private_data = NULL;
}

static void call_kernel(hb_filter_private_t *pv,
                        id<MTLTexture> dst,
                        id<MTLTexture> src,
                        int channels,
                        int plane)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;
    lapsharp_plane_context_t *params = (lapsharp_plane_context_t *)pv->mtl->params_buffer.contents;
    *params = pv->plane_ctx[plane];
    params->channels = channels;

    [encoder setTexture:dst atIndex:0];
    [encoder setTexture:src atIndex:1];
    [encoder setBuffer:pv->mem[plane] offset:0 atIndex:0];
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:1];

    hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipeline, encoder, dst.width, dst.height);
    [encoder endEncoding];

    [buffer commit];
    [buffer waitUntilCompleted];
}

static hb_buffer_t * filter_frame(hb_filter_private_t *pv, hb_buffer_t *in)
{
    CVReturn ret = kCVReturnSuccess;
    hb_buffer_t *out = NULL;

    CVPixelBufferRef cv_dest = NULL;
    CVPixelBufferRef cv_src = hb_cv_get_pixel_buffer(in);

    if (cv_src == NULL)
    {
        hb_log("lapsharp_vt: extract_buf failed");
        goto fail;
    }

    ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
    if (ret != kCVReturnSuccess)
    {
        hb_log("lapsharp_vt: CVPixelBufferPoolCreatePixelBuffer failed");
        goto fail;
    }

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        const AVComponentDescriptor *comp = &pv->desc->comp[i];
        if (comp->plane < i)
        {
            continue;
        }

        int channels;
        const MTLPixelFormat format = hb_metal_pix_fmt_from_component(comp, &channels);
        if (format == MTLPixelFormatInvalid)
        {
            goto fail;
        }

        CVMetalTextureRef src  = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_src, i, format);
        CVMetalTextureRef dest = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_dest, i, format);

        id<MTLTexture> tex_src  = CVMetalTextureGetTexture(src);
        id<MTLTexture> tex_dest = CVMetalTextureGetTexture(dest);

        call_kernel(pv, tex_dest, tex_src, channels, i);

        CFRelease(src);
        CFRelease(dest);
    }

    CVBufferPropagateAttachments(cv_src, cv_dest);

    out = hb_buffer_wrapper_init();
    out->storage_type      = COREMEDIA;
    out->storage           = cv_dest;
    out->f.width           = pv->output.geometry.width;
    out->f.height          = pv->output.geometry.height;
    out->f.fmt             = pv->output.pix_fmt;
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;
    hb_buffer_copy_props(out, in);

    return out;

fail:
    if (out != NULL)
    {
        hb_buffer_close(&out);
    }
    else if (cv_dest)
    {
        CVPixelBufferRelease(cv_dest);
    }
    return NULL;
}

static int lapsharp_vt_work(hb_filter_object_t *filter,
                            hb_buffer_t **buf_in,
                            hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    @autoreleasepool
    {
        *buf_out = filter_frame(pv, in);
    }

    return *buf_out == NULL ? HB_FILTER_FAILED : HB_FILTER_OK;
}
