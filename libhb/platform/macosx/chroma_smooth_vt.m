/* unsharp_vt.m

   Copyright (c) 2002 Rémi Guyomarch <rguyom at pobox.com>
   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_chroma_smooth_vt_metallib_data[];
extern unsigned int hb_chroma_smooth_vt_metallib_len;

#define CHROMA_SMOOTH_STRENGTH_DEFAULT 0.25
#define CHROMA_SMOOTH_SIZE_DEFAULT 7
#define CHROMA_SMOOTH_SIZE_MIN 3
#define CHROMA_SMOOTH_SIZE_MAX 15

typedef struct
{
    float      max_value;
    float      min_value;
    float      strength;  // amount
    int        size;      // pixel context region width (must be odd)

    id<MTLBuffer> matrix;
    id<MTLBuffer> coef_x;
    id<MTLBuffer> coef_y;
} chroma_smooth_vt_plane_context_t;

struct mtl_chroma_smooth_params
{
    uint       channels;
    float      max_value;
    float      min_value;
    float      strength;
    int        size;
};

struct hb_filter_private_s
{
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

    bool                        global;
    chroma_smooth_vt_plane_context_t  plane_ctx[3];

    hb_filter_init_t            input;
    hb_filter_init_t            output;
};

static int chroma_smooth_vt_init(hb_filter_object_t *filter,
                           hb_filter_init_t   *init);

static int chroma_smooth_vt_work(hb_filter_object_t *filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out);

static void chroma_smooth_vt_close(hb_filter_object_t *filter);

static const char chroma_smooth_vt_template[] =
    "cb-strength=^"HB_FLOAT_REG"$:cb-size=^"HB_INT_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-size=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_chroma_smooth_vt =
{
    .id                = HB_FILTER_CHROMA_SMOOTH_VT,
    .enforce_order     = 1,
    .name              = "Chroma Smooth (VideoToolbox)",
    .settings          = NULL,
    .init              = chroma_smooth_vt_init,
    .work              = chroma_smooth_vt_work,
    .close             = chroma_smooth_vt_close,
    .settings_template = chroma_smooth_vt_template,
};

static int chroma_smooth_vt_init(hb_filter_object_t *filter,
                        hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("chroma_smooth_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;
    pv->input = *init;
    pv->desc = av_pix_fmt_desc_get(init->pix_fmt);

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->plane_ctx[c].strength = -1;
        pv->plane_ctx[c].size     = -1;
    }

    // Read user parameters
    if (filter->settings != NULL)
    {
        hb_dict_t *dict = filter->settings;
        double val;

        hb_dict_extract_double(&val, dict, "cb-strength");
        hb_dict_extract_int(&pv->plane_ctx[1].size, dict, "cb-size");
        pv->plane_ctx[1].strength = val;

        hb_dict_extract_double(&val, dict, "cr-strength");
        hb_dict_extract_int(&pv->plane_ctx[2].size, dict, "cr-size");
        pv->plane_ctx[2].strength = val;
    }

    char *function_name = "chroma_smooth_local";

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; defaults.
    for (int c = 2; c < 3; c++)
    {
        chroma_smooth_vt_plane_context_t *prev_ctx = &pv->plane_ctx[c - 1];
        chroma_smooth_vt_plane_context_t *ctx      = &pv->plane_ctx[c];

        if (ctx->strength == -1) ctx->strength = prev_ctx->strength;
        if (ctx->size     == -1) ctx->size     = prev_ctx->size;
        if (ctx->size > 17)
        {
            pv->global = true;
            function_name = "chroma_smooth_global";
        }
    }

    pv->mtl = hb_metal_context_init(hb_chroma_smooth_vt_metallib_data,
                                    hb_chroma_smooth_vt_metallib_len,
                                    function_name,
                                    sizeof(struct mtl_chroma_smooth_params),
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("chroma_smooth_vt: failed to create Metal device");
        return -1;
    }

    for (int c = 0; c < 3; c++)
    {
        chroma_smooth_vt_plane_context_t *ctx = &pv->plane_ctx[c];

        float max = 1.0f;
        ctx->max_value = max - max / 16;
        ctx->min_value = max / 16;

        // Replace unset values with defaults
        if (ctx->strength == -1)
        {
            ctx->strength = CHROMA_SMOOTH_STRENGTH_DEFAULT;
        }
        if (ctx->size     == -1)
        {
            ctx->size     = CHROMA_SMOOTH_SIZE_DEFAULT;
        }

        // Sanitize
        if (ctx->strength < 0) ctx->strength = 0;
        if (ctx->strength > 3) ctx->strength = 3;
        if (ctx->size % 2 == 0) ctx->size--;
        if (ctx->size < CHROMA_SMOOTH_SIZE_MIN) ctx->size = CHROMA_SMOOTH_SIZE_MIN;
        if (ctx->size > CHROMA_SMOOTH_SIZE_MAX) ctx->size = CHROMA_SMOOTH_SIZE_MAX;

        const NSUInteger length = CHROMA_SMOOTH_SIZE_MAX * sizeof(float);
        ctx->coef_x = [pv->mtl->device newBufferWithLength:length
                                                     options:MTLResourceStorageModeManaged];
        ctx->coef_y = [pv->mtl->device newBufferWithLength:length
                                                     options:MTLResourceStorageModeManaged];

        float *blur_x = ctx->coef_x.contents;
        float *blur_y = ctx->coef_y.contents;
        double sum = 0.0;

        for (int x = 0; x < ctx->size; x++)
        {
            double dx = (double)(x - ctx->size / 2) / ctx->size;
            sum += blur_x[x] = exp(-16.0 * (dx * dx));
        }
        for (int x = 0; x < ctx->size; x++)
        {
            blur_x[x] /= sum;
        }

        sum = 0.0;
        for (int y = 0; y < ctx->size; y++)
        {
            double dy = (double)(y - ctx->size / 2) / ctx->size;
            sum += blur_y[y] = exp(-16.0 * (dy * dy));
        }
        for (int y = 0; y < ctx->size; y++)
        {
            blur_y[y] /= sum;
        }

        [ctx->coef_x didModifyRange:NSMakeRange(0, length)];
        [ctx->coef_y didModifyRange:NSMakeRange(0, length)];

        NSUInteger matrix_length = ctx->size * ctx->size * sizeof(float);
        ctx->matrix = [pv->mtl->device newBufferWithLength:matrix_length
                                                   options:MTLResourceStorageModeManaged];

        float *matrix = ctx->matrix.contents;
        for (int y = 0; y < ctx->size; y++)
        {
            for (int x = 0; x < ctx->size; x++)
            {
                double val = blur_x[x] * blur_y[y];
                matrix[y * ctx->size + x] = val;
            }
        }

        [ctx->matrix didModifyRange:NSMakeRange(0, matrix_length)];
    }

    pv->output = *init;

    return 0;
}

static void chroma_smooth_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_metal_context_close(&pv->mtl);

    for (int i = 0; i < 3; i++)
    {
        chroma_smooth_vt_plane_context_t *ctx = &pv->plane_ctx[i];
        [ctx->matrix release];
        [ctx->coef_x release];
        [ctx->coef_y release];
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
    chroma_smooth_vt_plane_context_t *ctx = &pv->plane_ctx[plane];
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;
    struct mtl_chroma_smooth_params *params = (struct mtl_chroma_smooth_params *)pv->mtl->params_buffer.contents;
    *params = (struct mtl_chroma_smooth_params)
    {
        .channels = channels,
        .max_value = ctx->max_value,
        .min_value = ctx->min_value,
        .strength  = ctx->strength,
        .size      = ctx->size,
    };

    [encoder setTexture:dst atIndex:0];
    [encoder setTexture:src atIndex:1];

    int i = 0;
    if (pv->global)
    {
        [encoder setBuffer:ctx->matrix offset:0 atIndex:i++];
    }
    else
    {
        [encoder setBuffer:ctx->coef_x offset:0 atIndex:i++];
        [encoder setBuffer:ctx->coef_y offset:0 atIndex:i++];
    }
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:i++];

    if (pv->global)
    {
        hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[0], encoder, dst.width, dst.height);
    }
    else
    {
        hb_metal_compute_encoder_dispatch_fixed_threadgroup_size(pv->mtl->device, pv->mtl->pipelines[0],
                                                                 encoder, dst.width, dst.height, 16, 16);
    }
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
        hb_log("chroma_smooth_vt: extract_buf failed");
        goto fail;
    }

    ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
    if (ret != kCVReturnSuccess)
    {
        hb_log("chroma_smooth_vt: CVPixelBufferPoolCreatePixelBuffer failed");
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

static int chroma_smooth_vt_work(hb_filter_object_t *filter,
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
