/* motion_metric_vt.m

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_motion_metric_vt_metallib_data[];
extern unsigned int hb_motion_metric_vt_metallib_len;

struct hb_motion_metric_private_s
{
    hb_metal_context_t *mtl;
    id<MTLBuffer>       result;

    const AVPixFmtDescriptor *desc;
};

static int hb_motion_metric_vt_init(hb_motion_metric_object_t *metric,
                                    hb_filter_init_t *init);

static float hb_motion_metric_vt_work(hb_motion_metric_object_t *metric,
                                      hb_buffer_t *buf_a,
                                      hb_buffer_t *buf_b);

static void hb_motion_metric_vt_close(hb_motion_metric_object_t *metric);

hb_motion_metric_object_t hb_motion_metric_vt =
{
    .name  = "Motion metric (VideoToolbox)",
    .init  = hb_motion_metric_vt_init,
    .work  = hb_motion_metric_vt_work,
    .close = hb_motion_metric_vt_close,
};

static int hb_motion_metric_vt_init(hb_motion_metric_object_t *metric,
                                    hb_filter_init_t *init)
{
    metric->private_data = calloc(sizeof(struct hb_motion_metric_private_s), 1);
    if (metric->private_data == NULL)
    {
        hb_error("motion_metric_vt: calloc failed");
        return -1;
    }
    hb_motion_metric_private_t *pv = metric->private_data;

    pv->desc = av_pix_fmt_desc_get(init->pix_fmt);

    pv->mtl = hb_metal_context_init(hb_motion_metric_vt_metallib_data,
                                    hb_motion_metric_vt_metallib_len,
                                    NULL, NULL, 0,
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        hb_error("motion_metric_vt: failed to create Metal device");
        return -1;
    }

    char *function_name = "motion_metric";
    if (@available(macOS 13, *))
    {
        if ([pv->mtl->device supportsFamily:MTLGPUFamilyMetal3])
        {
            // Use simd_sum() to speed up the final reduction pass
            function_name = "motion_metric_simd";
        }
    }
    hb_metal_add_pipeline(pv->mtl, function_name, NULL, 0);

    int w = 16, h = 16;
    NSUInteger length = sizeof(uint32_t) * ((init->geometry.width + w - 1) / w) * ((init->geometry.height + h - 1) / h);

    pv->result = [pv->mtl->device newBufferWithLength:length options:MTLResourceStorageModeShared];
    if (pv->result == nil)
    {
        hb_error("motion_metric_vt: failed to create buffer");
        return -1;
    }

    return 0;
}

static void call_kernel(hb_motion_metric_private_t *pv,
                        id<MTLTexture> a,
                        id<MTLTexture> b)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;

    [encoder setTexture:a atIndex:0];
    [encoder setTexture:b atIndex:1];
    [encoder setBuffer:pv->result offset:0 atIndex:0];

    hb_metal_compute_encoder_dispatch_fixed_threadgroup_size(pv->mtl->device,
                                                             pv->mtl->pipelines[0], encoder,
                                                             a.width, a.height, 16, 16);

    [encoder endEncoding];

    [buffer commit];
    [buffer waitUntilCompleted];
}

static float motion_metric(hb_motion_metric_private_t *pv, hb_buffer_t *a, hb_buffer_t *b)
{
    CVPixelBufferRef cv_a = hb_cv_get_pixel_buffer(a);
    CVPixelBufferRef cv_b = hb_cv_get_pixel_buffer(b);

    if (cv_a == NULL || cv_b == NULL)
    {
        hb_log("motion_metric_vt: extract_buf failed");
        goto fail;
    }

    const AVComponentDescriptor *comp = &pv->desc->comp[0];

    int channels;
    const MTLPixelFormat format = hb_metal_pix_fmt_from_component(comp, 0, &channels);
    if (format == MTLPixelFormatInvalid)
    {
        goto fail;
    }

    CVMetalTextureRef ref_a = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_a, 0, channels, format);
    CVMetalTextureRef ref_b = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_b, 0, channels, format);

    id<MTLTexture> tex_a = CVMetalTextureGetTexture(ref_a);
    id<MTLTexture> tex_b = CVMetalTextureGetTexture(ref_b);

    call_kernel(pv, tex_a, tex_b);

    CFRelease(ref_a);
    CFRelease(ref_b);

    uint32_t *result = pv->result.contents;
    size_t length = pv->result.length / sizeof(uint32_t);

    // Do the final reduction on CPU
    // long and float atomics are still
    // slow or unsupported on too many GPUs
    uint64_t sum = 0;
    for (size_t i = 0; i < length; i++)
    {
        sum += result[i];
    }

    return (float)sum / (a->f.width * a->f.height);
fail:
    return 0;
}

static float hb_motion_metric_vt_work(hb_motion_metric_object_t *metric,
                                      hb_buffer_t *buf_a,
                                      hb_buffer_t *buf_b)
{
    @autoreleasepool
    {
        return motion_metric(metric->private_data, buf_a, buf_b);
    }
}

static void hb_motion_metric_vt_close(hb_motion_metric_object_t *metric)
{
    hb_motion_metric_private_t *pv = metric->private_data;

    if (pv == NULL)
    {
        return;
    }

    [pv->result release];
    hb_metal_context_close(&pv->mtl);

    free(pv);
}
