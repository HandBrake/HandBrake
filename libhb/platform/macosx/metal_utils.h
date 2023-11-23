/* utils.h

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_METAL_UTILS_H
#define HB_METAL_UTILS_H

#include <Metal/Metal.h>
#include <CoreVideo/CoreVideo.h>
#include "libavutil/avutil.h"

struct hb_metal_context_s
{
    id<MTLDevice>                device;
    id<MTLLibrary>               library;
    id<MTLCommandQueue>          queue;
    id<MTLBuffer>                params_buffer;
    id<MTLComputePipelineState> *pipelines;
    id<MTLFunction>             *functions;
    size_t                       pipelines_count;

    CVMetalTextureCacheRef   cache;
    CVPixelBufferPoolRef     pool;
};
typedef struct hb_metal_context_s hb_metal_context_t;

hb_metal_context_t * hb_metal_context_init(const char *metallib_data,
                                           size_t metallib_len,
                                           const char *function_name,
                                           size_t params_buffer_len,
                                           int width, int height,
                                           int pix_fmt, int color_range);

void hb_metal_context_close(hb_metal_context_t **_ctx);

void hb_metal_compute_encoder_dispatch(id<MTLDevice> device,
                                       id<MTLComputePipelineState> pipeline,
                                       id<MTLComputeCommandEncoder> encoder,
                                       NSUInteger width, NSUInteger height);

void hb_metal_compute_encoder_dispatch_fixed_threadgroup_size(id<MTLDevice> device,
                                                              id<MTLComputePipelineState> pipeline,
                                                              id<MTLComputeCommandEncoder> encoder,
                                                              NSUInteger width, NSUInteger height,
                                                              NSUInteger w, NSUInteger h);

MTLPixelFormat hb_metal_pix_fmt_from_component(const AVComponentDescriptor *comp, int *channels_out);

CVMetalTextureRef hb_metal_create_texture_from_pixbuf(CVMetalTextureCacheRef textureCache,
                                               CVPixelBufferRef pixbuf,
                                               int plane,
                                               MTLPixelFormat format);

int hb_metal_add_pipeline(hb_metal_context_t *ctx, const char *function_name, size_t index);

#endif /* HB_METAL_UTILS_H */
