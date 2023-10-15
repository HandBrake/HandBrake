/* pad_vt.m

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/colormap.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_pad_vt_metallib_data[];
extern unsigned int hb_pad_vt_metallib_len;

struct mtl_pad_params
{
    uint    plane;
    uint    channels;
    float   color_y;
    float   color_u;
    float   color_v;
    uint    x;
    uint    y;
};

struct hb_filter_private_s
{
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

    float color[3];
    int   x;
    int   y;

    hb_filter_init_t    input;
    hb_filter_init_t    output;
};

static int pad_vt_init(hb_filter_object_t   *filter,
                           hb_filter_init_t *init);

static int pad_vt_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);

static void pad_vt_close(hb_filter_object_t *filter);

const char pad_vt_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:color=^"HB_ALL_REG"$:"
    "x=^"HB_INT_REG"$:y=^"HB_INT_REG"$:"
    "top=^"HB_INT_REG"$:bottom=^"HB_INT_REG"$:"
    "left=^"HB_INT_REG"$:right=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_pad_vt =
{
    .id                = HB_FILTER_PAD_VT,
    .enforce_order     = 1,
    .name              = "Pad (VideoToolbox)",
    .settings          = NULL,
    .init              = pad_vt_init,
    .work              = pad_vt_work,
    .close             = pad_vt_close,
    .settings_template = pad_vt_template,
};

static int pad_vt_init(hb_filter_object_t *filter,
                        hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("pad_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;
    pv->input = *init;
    pv->desc = av_pix_fmt_desc_get(init->pix_fmt);

    hb_dict_t *settings = filter->settings;
    int      width  = -1, height = -1;
    int      top = -1, bottom = -1, left = -1, right = -1;
    int      x = -1, y = -1;
    char     *color = NULL;

    hb_dict_extract_int(&top, settings, "top");
    hb_dict_extract_int(&bottom, settings, "bottom");
    hb_dict_extract_int(&left, settings, "left");
    hb_dict_extract_int(&right, settings, "right");

    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");
    hb_dict_extract_string(&color, settings, "color");
    hb_dict_extract_int(&x, settings, "x");
    hb_dict_extract_int(&y, settings, "y");

    if (x < 0)
    {
        x = left;
    }
    if (y < 0)
    {
        y = top;
    }
    // Avoid odd offsets when chroma
    // is half the luma
    if (x % 2)
    {
        x -= pv->desc->log2_chroma_w;
    }
    if (y % 2)
    {
        y -= pv->desc->log2_chroma_h;
    }
    if (top >= 0 && bottom >= 0 && height < 0)
    {
        height = init->geometry.height + top + bottom;
    }
    if (left >= 0 && right >= 0 && width < 0)
    {
        width = init->geometry.width + left + right;
    }
    if (color != NULL)
    {
        char *end;
        int rgb = strtol(color, &end, 0);
        if (end == color)
        {
            // Not a numeric value, lookup by name
            rgb = hb_rgb_lookup_by_name(color);
        }
        free(color);

        rgb = hb_cv_match_rgb_to_colorspace(rgb, init->color_prim, init->color_transfer, init->color_matrix);

        int yuv;
        switch (init->color_matrix)
        {
            case HB_COLR_MAT_SMPTE170M:
                yuv = hb_rgb2yuv(rgb);
                break;
            case HB_COLR_MAT_BT2020_NCL:
            case HB_COLR_MAT_BT2020_CL:
                yuv = hb_rgb2yuv_bt2020(rgb);
                break;
            case HB_COLR_MAT_BT709:
            default:
                yuv = hb_rgb2yuv_bt709(rgb);
                break;
        }

        pv->color[0] = ((yuv >> 16) & 0xff) / 255.f;
        pv->color[1] = ((yuv >> 8 ) & 0xff) / 255.f;
        pv->color[2] = ((yuv >> 0 ) & 0xff) / 255.f;
    }

    if (width < init->geometry.width)
    {
        width = init->geometry.width;
    }
    if (height < init->geometry.height)
    {
        height = init->geometry.height;
    }

    pv->x = x;
    pv->y = y;

    pv->mtl = hb_metal_context_init(hb_pad_vt_metallib_data,
                                    hb_pad_vt_metallib_len,
                                    "pad",
                                    sizeof(struct mtl_pad_params),
                                    width, height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("pad_vt: failed to create Metal device");
        return -1;
    }

    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    return 0;
}

static void pad_vt_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_metal_context_close(&pv->mtl);

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
    struct mtl_pad_params *params = (struct mtl_pad_params *)pv->mtl->params_buffer.contents;
    *params = (struct mtl_pad_params)
    {
        .plane = plane,
        .channels = channels,
        .color_y = pv->color[0],
        .color_u = pv->color[1],
        .color_v = pv->color[2],
        .x = plane ? pv->x >> pv->desc->log2_chroma_w : pv->x,
        .y = plane ? pv->y >> pv->desc->log2_chroma_h : pv->y,
    };

    [encoder setTexture:dst atIndex:0];
    [encoder setTexture:src atIndex:1];
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:0];

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
        hb_log("pad_vt: extract_buf failed");
        goto fail;
    }

    ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
    if (ret != kCVReturnSuccess)
    {
        hb_log("pad_vt: CVPixelBufferPoolCreatePixelBuffer failed");
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

static int pad_vt_work(hb_filter_object_t *filter,
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
