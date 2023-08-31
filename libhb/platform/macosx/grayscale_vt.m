/* grayscale_vt.m

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/decomb.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_grayscale_vt_metallib_data[];
extern unsigned int hb_grayscale_vt_metallib_len;

struct mtl_grayscale_params
{
    uint plane;
    bool biplanar;
    uint subw;
    uint subh;
    uint cb;
    uint cr;
    uint size;
    uint high;
};

struct hb_filter_private_s
{
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

    int cb;
    int cr;
    int size;
    int high;

    hb_filter_init_t    input;
    hb_filter_init_t    output;
};

static int grayscale_vt_init(hb_filter_object_t *filter,
                             hb_filter_init_t   *init);

static int grayscale_vt_work(hb_filter_object_t *filter,
                             hb_buffer_t **buf_in,
                             hb_buffer_t **buf_out);

static void grayscale_vt_close(hb_filter_object_t *filter);

const char grayscale_vt_template[] =
    "cb=^"HB_FLOAT_REG"$:cr=^"HB_FLOAT_REG"$:size=^"HB_FLOAT_REG"$:high=^"HB_FLOAT_REG"$";

hb_filter_object_t hb_filter_grayscale_vt =
{
    .id                = HB_FILTER_GRAYSCALE_VT,
    .enforce_order     = 1,
    .name              = "Grayscale (VideoToolbox)",
    .settings          = NULL,
    .init              = grayscale_vt_init,
    .work              = grayscale_vt_work,
    .close             = grayscale_vt_close,
    .settings_template = grayscale_vt_template,
};

static int grayscale_vt_init(hb_filter_object_t *filter,
                             hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("grayscale_vt: calloc failed");
        return -1;
    }

    hb_filter_private_t *pv = filter->private_data;
    pv->input = *init;
    pv->desc  = av_pix_fmt_desc_get(init->pix_fmt);

    hb_dict_t *settings = filter->settings;
    double cb = 0, cr = 0, size = 1, high = 0;

    hb_dict_extract_double(&cb, settings, "cb");
    hb_dict_extract_double(&cr, settings, "cr");
    hb_dict_extract_double(&size, settings, "size");
    hb_dict_extract_double(&high, settings, "high");

    pv->cb = cb;
    pv->cr = cr;
    pv->size = size;
    pv->high = high;

    pv->mtl = hb_metal_context_init(hb_grayscale_vt_metallib_data,
                                    hb_grayscale_vt_metallib_len,
                                    "monochrome",
                                    sizeof(struct mtl_grayscale_params),
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("grayscale_vt: failed to create Metal device");
        return -1;
    }

    pv->output = *init;
    return 0;
}

static void grayscale_vt_close(hb_filter_object_t *filter)
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
                        id<MTLTexture> src[3],
                        int plane,
                        int channels)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;
    struct mtl_grayscale_params *params = (struct mtl_grayscale_params *)pv->mtl->params_buffer.contents;
    *params = (struct mtl_grayscale_params)
    {
        .plane = plane,
        .biplanar = channels == 2,
        .subw = pv->desc->log2_chroma_w,
        .subh = pv->desc->log2_chroma_h,
        .cb = pv->cb,
        .cr = pv->cr,
        .size = pv->size,
        .high = pv->high,
    };

    [encoder setTexture:dst atIndex:0];
    for (int i = 0; i < 3; i++)
    {
        if (src[i] != NULL)
        {
            [encoder setTexture:src[i] atIndex:i+1];
        }
    }
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

    CVPixelBufferRef cv_source = hb_cv_get_pixel_buffer(in);

    if (cv_source == NULL)
    {
        hb_log("grayscale_vt: extract_buf failed");
        goto fail;
    }

    CVPixelBufferRef cv_dest = NULL;
    ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
    if (ret != kCVReturnSuccess)
    {
        hb_log("grayscale_vt: CVPixelBufferPoolCreatePixelBuffer failed");
        goto fail;
    }

    CVMetalTextureRef  src[3] = { NULL };
    MTLPixelFormat  format[3] = { 0 };
    id<MTLTexture> tex_src[3] = { NULL };
    int channels;

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        const AVComponentDescriptor *comp = &pv->desc->comp[i];
        if (comp->plane < i)
        {
            continue;
        }

        format[i] = hb_metal_pix_fmt_from_component(comp, &channels);
        if (format[i] == MTLPixelFormatInvalid)
        {
            goto fail;
        }

        src[i]     = hb_metal_texture_from_pixbuf(pv->mtl->cache, cv_source, i, format[i]);
        tex_src[i] = CVMetalTextureGetTexture(src[i]);
    }

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        const AVComponentDescriptor *comp = &pv->desc->comp[i];
        if (comp->plane < i)
        {
            continue;
        }

        CVMetalTextureRef dest = hb_metal_texture_from_pixbuf(pv->mtl->cache, cv_dest, i, format[i]);
        id<MTLTexture> tex_dest = CVMetalTextureGetTexture(dest);

        call_kernel(pv, tex_dest, tex_src, i, channels);

        CFRelease(dest);
    }

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        if (src[i] != NULL)
        {
            CFRelease(src[i]);
        }
    }

    CVBufferPropagateAttachments(cv_source, cv_dest);

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

static int grayscale_vt_work(hb_filter_object_t *filter,
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
