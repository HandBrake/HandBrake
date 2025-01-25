/* grayscale_vt.m

   Copyright (c) 2003-2025 HandBrake Team
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

struct hb_filter_private_s
{
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

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

    pv->mtl = hb_metal_context_init(hb_grayscale_vt_metallib_data,
                                    hb_grayscale_vt_metallib_len,
                                    NULL, NULL, 0,
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("grayscale_vt: failed to create Metal device");
        return -1;
    }

    bool biplanar = 1;
    uint32_t plane = 0;
    uint32_t subw = pv->desc->log2_chroma_w;
    uint32_t subh = pv->desc->log2_chroma_h;
    uint32_t cb_i = cb;
    uint32_t cr_i = cr;
    uint32_t size_i = size;
    uint32_t high_i = high;

    MTLFunctionConstantValues *constant_values = [MTLFunctionConstantValues new];

    [constant_values setConstantValue:&plane    type:MTLDataTypeUInt withName:@"plane"];
    [constant_values setConstantValue:&biplanar type:MTLDataTypeBool withName:@"biplanar"];
    [constant_values setConstantValue:&subw     type:MTLDataTypeUInt withName:@"subw"];
    [constant_values setConstantValue:&subh     type:MTLDataTypeUInt withName:@"subh"];
    [constant_values setConstantValue:&cb_i     type:MTLDataTypeUInt withName:@"cb"];
    [constant_values setConstantValue:&cr_i     type:MTLDataTypeUInt withName:@"cr"];
    [constant_values setConstantValue:&size_i   type:MTLDataTypeUInt withName:@"size"];
    [constant_values setConstantValue:&high_i   type:MTLDataTypeUInt withName:@"high"];

    if (hb_metal_add_pipeline(pv->mtl, "monochrome", constant_values, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }

    plane = 1;
    [constant_values setConstantValue:&plane type:MTLDataTypeUInt withName:@"plane"];

    if (hb_metal_add_pipeline(pv->mtl, "monochrome", constant_values, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }

    [constant_values release];

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
                        int plane)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;

    [encoder setTexture:dst atIndex:0];
    for (int i = 0; i < 3; i++)
    {
        if (src[i] != NULL)
        {
            [encoder setTexture:src[i] atIndex:i+1];
        }
    }

    hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[plane], encoder, dst.width, dst.height);

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
        hb_log("grayscale_vt: extract_buf failed");
        goto fail;
    }

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

        format[i] = hb_metal_pix_fmt_from_component(comp, 0, &channels);
        if (format[i] == MTLPixelFormatInvalid)
        {
            goto fail;
        }

        src[i]     = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_src, i, channels, format[i]);
        tex_src[i] = CVMetalTextureGetTexture(src[i]);
    }

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        const AVComponentDescriptor *comp = &pv->desc->comp[i];
        if (comp->plane < i)
        {
            continue;
        }

        CVMetalTextureRef dest = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_dest, i, channels, format[i]);
        id<MTLTexture> tex_dest = CVMetalTextureGetTexture(dest);

        call_kernel(pv, tex_dest, tex_src, i);

        CFRelease(dest);
    }

    for (int i = 0; i < pv->desc->nb_components; i++)
    {
        if (src[i] != NULL)
        {
            CFRelease(src[i]);
        }
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
