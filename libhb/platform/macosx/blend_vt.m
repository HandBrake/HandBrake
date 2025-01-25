/* blend_vt.m

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_blend_vt_metallib_data[];
extern unsigned int hb_blend_vt_metallib_len;

struct mtl_blend_params
{
    uint x;
    uint y;
};

struct hb_blend_private_s
{
    hb_metal_context_t *mtl;

    id<MTLTexture> overlays[4];

    const AVPixFmtDescriptor *in_desc;
    const AVPixFmtDescriptor *overlay_desc;

    int subw;
    int subh;
    int overlay_subw;
    int overlay_subh;
};

static int hb_blend_vt_init(hb_blend_object_t *object,
                            int in_width,
                            int in_height,
                            int in_pix_fmt,
                            int in_chroma_location,
                            int in_color_range,
                            int overlay_pix_fmt);

static hb_buffer_t * hb_blend_vt_work(hb_blend_object_t *object,
                                      hb_buffer_t *in,
                                      hb_buffer_list_t *overlays,
                                      int changed);

static void hb_blend_vt_close(hb_blend_object_t *object);

hb_blend_object_t hb_blend_vt =
{
    .name  = "Blend (VideoToolbox)",
    .init  = hb_blend_vt_init,
    .work  = hb_blend_vt_work,
    .close = hb_blend_vt_close,
};

static int hb_blend_vt_init(hb_blend_object_t *object,
                            int in_width,
                            int in_height,
                            int in_pix_fmt,
                            int in_chroma_location,
                            int in_color_range,
                            int overlay_pix_fmt)
{
    object->private_data = calloc(sizeof(struct hb_blend_private_s), 1);
    if (object->private_data == NULL)
    {
        hb_error("blend_vt: calloc failed");
        return -1;
    }
    hb_blend_private_t *pv = object->private_data;

    pv->in_desc      = av_pix_fmt_desc_get(in_pix_fmt);
    pv->overlay_desc = av_pix_fmt_desc_get(overlay_pix_fmt);

    int depth     = pv->in_desc->comp[0].depth == 8 ? 8 : 16;
    int shift     = depth - 8;
    int max_value = (1 << depth) - 1;

    pv->subw  = pv->in_desc->log2_chroma_w;
    pv->subh  = pv->in_desc->log2_chroma_h;
    pv->overlay_subw = pv->overlay_desc->log2_chroma_w;
    pv->overlay_subh = pv->overlay_desc->log2_chroma_h;

    bool needs_subsample = (pv->subw != pv->overlay_subw) || (pv->subh != pv->overlay_subh);

    pv->mtl = hb_metal_context_init(hb_blend_vt_metallib_data,
                                    hb_blend_vt_metallib_len,
                                    NULL, NULL, sizeof(struct mtl_blend_params),
                                    in_width, in_height,
                                    in_pix_fmt, in_color_range);
    if (pv->mtl == NULL)
    {
        hb_error("blend_vt: failed to create Metal device");
        return -1;
    }

    uint32_t plane = 0;
    uint32_t channels = 1;
    MTLFunctionConstantValues *constant_values = [MTLFunctionConstantValues new];

    [constant_values setConstantValue:&plane            type:MTLDataTypeUInt withName:@"plane"];
    [constant_values setConstantValue:&channels         type:MTLDataTypeUInt withName:@"channels"];
    [constant_values setConstantValue:&pv->subw         type:MTLDataTypeUInt withName:@"subw"];
    [constant_values setConstantValue:&pv->subh         type:MTLDataTypeUInt withName:@"subh"];
    [constant_values setConstantValue:&pv->overlay_subw type:MTLDataTypeUInt withName:@"osubw"];
    [constant_values setConstantValue:&pv->overlay_subh type:MTLDataTypeUInt withName:@"osubh"];
    [constant_values setConstantValue:&shift            type:MTLDataTypeUInt withName:@"shift"];
    [constant_values setConstantValue:&max_value        type:MTLDataTypeUInt withName:@"maxv"];
    [constant_values setConstantValue:&needs_subsample  type:MTLDataTypeBool withName:@"subsample"];

    if (hb_metal_add_pipeline(pv->mtl, "blend", constant_values, pv->mtl->pipelines_count))
    {
        hb_error("blend_vt: failed to create Metal pipeline");
        [constant_values release];
        return -1;
    }

    plane = 1;
    channels = 2;
    [constant_values setConstantValue:&plane    type:MTLDataTypeUInt withName:@"plane"];
    [constant_values setConstantValue:&channels type:MTLDataTypeUInt withName:@"channels"];

    if (hb_metal_add_pipeline(pv->mtl, "blend", constant_values, pv->mtl->pipelines_count))
    {
        hb_error("blend_vt: failed to create Metal pipeline");
        [constant_values release];
        return -1;
    }

    [constant_values release];

    for (int pp = 0; pp < pv->overlay_desc->nb_components; pp++)
    {
        int width  = pp == 1 || pp == 2 ? in_width  >> pv->overlay_desc->log2_chroma_w : in_width;
        int height = pp == 1 || pp == 2 ? in_height >> pv->overlay_desc->log2_chroma_h : in_height;

        MTLTextureDescriptor *descriptor = [[MTLTextureDescriptor alloc] init];
        descriptor.textureType = MTLTextureType2D;
        descriptor.pixelFormat = MTLPixelFormatR8Uint;
        descriptor.width       = width;
        descriptor.height      = height;
        descriptor.depth       = 1;
        descriptor.storageMode = MTLStorageModeManaged;
        descriptor.usage       = MTLTextureUsageShaderRead;

        pv->overlays[pp] = [pv->mtl->device newTextureWithDescriptor:descriptor];

        [descriptor release];
    }

    return 0;
}

static void upload_overlays(hb_blend_private_t *pv, hb_buffer_list_t *overlays)
{
    for (hb_buffer_t *overlay = hb_buffer_list_head(overlays); overlay; overlay = overlay->next)
    {
        for (int pp = 0; pp < pv->overlay_desc->nb_components; pp++)
        {
            int x = pp == 1 || pp == 2 ? overlay->f.x >> pv->overlay_desc->log2_chroma_w : overlay->f.x;
            int y = pp == 1 || pp == 2 ? overlay->f.y >> pv->overlay_desc->log2_chroma_h : overlay->f.y;

            [pv->overlays[pp] replaceRegion:MTLRegionMake2D(x, y,
                                                            overlay->plane[pp].width,
                                                            overlay->plane[pp].height)
                                mipmapLevel:0
                                  withBytes:overlay->plane[pp].data
                                bytesPerRow:overlay->plane[pp].stride];
        }
    }
}

static void call_kernel(hb_blend_private_t *pv,
                        id<MTLTexture> dst,
                        int plane,
                        int width, int height,
                        int x, int y)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;

    struct mtl_blend_params *params = (struct mtl_blend_params *)pv->mtl->params_buffer.contents;
    params->x = x;
    params->y = y;

    [encoder setTexture:dst atIndex:0];
    for (int i = 0; i < pv->overlay_desc->nb_components; i++)
    {
        [encoder setTexture:pv->overlays[i] atIndex:i + 1];
    }
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:0];

    if (plane)
    {
        // Chroma plane subsampling can differ
        // always call the kernel in the destination coordinate space
        width  >>= (pv->subw - pv->overlay_subw);
        height >>= (pv->subh - pv->overlay_subh);
    }

    hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[plane],
                                      encoder, width, height);

    [encoder endEncoding];

    [buffer commit];
    [buffer waitUntilCompleted];
}

void blend(hb_blend_private_t *pv, hb_buffer_t *out, hb_buffer_t *overlay, int changed)
{
    CVPixelBufferRef cv_dest = hb_cv_get_pixel_buffer(out);

    if (cv_dest == NULL)
    {
        hb_log("blend_vt: extract_buf failed");
        return;
    }

    for (int i = 0; i < pv->in_desc->nb_components; i++)
    {
        const AVComponentDescriptor *comp = &pv->in_desc->comp[i];
        if (comp->plane < i)
        {
            continue;
        }

        int channels;
        const MTLPixelFormat format = hb_metal_pix_fmt_from_component(comp, 1, &channels);
        if (format == MTLPixelFormatInvalid)
        {
            break;
        }

        CVMetalTextureRef dest = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_dest, i, channels, format);

        id<MTLTexture> tex_dest = CVMetalTextureGetTexture(dest);
        call_kernel(pv, tex_dest, i,
                    overlay->plane[i].width, overlay->plane[i].height,
                    overlay->f.x, overlay->f.y);
        CFRelease(dest);
    }
}

static hb_buffer_t * hb_blend_vt_work(hb_blend_object_t *object,
                                      hb_buffer_t *in,
                                      hb_buffer_list_t *overlays,
                                      int changed)
{
    hb_blend_private_t *pv = object->private_data;
    hb_buffer_t *out = in;

    if (hb_buffer_list_count(overlays) == 0)
    {
        return out;
    }

    @autoreleasepool
    {
        if (changed)
        {
            upload_overlays(pv, overlays);
        }

        for (hb_buffer_t *overlay = hb_buffer_list_head(overlays); overlay; overlay = overlay->next)
        {
            blend(pv, out, overlay, changed);
        }
        return out;
    }
}

static void hb_blend_vt_close(hb_blend_object_t *metric)
{
    hb_blend_private_t *pv = metric->private_data;

    if (pv == NULL)
    {
        return;
    }

    for (int i = 0; i < pv->overlay_desc->nb_components; i++)
    {
        [pv->overlays[i] release];
    }

    hb_metal_context_close(&pv->mtl);

    free(pv);
}
