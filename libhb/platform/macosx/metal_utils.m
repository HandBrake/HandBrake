/* utils.m

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "metal_utils.h"
#include "cv_utils.h"

hb_metal_context_t * hb_metal_context_init(const char *metallib_data,
                                           size_t metallib_len,
                                           const char *function_name,
                                           size_t params_buffer_len,
                                           int width, int height,
                                           int pix_fmt, int color_range)
{
    hb_metal_context_t *ctx = calloc(sizeof(struct hb_metal_context_s), 1);
    if (ctx == NULL)
    {
        goto fail;
    }

    NSError *err = nil;

    NSArray<id<MTLDevice>> *devices = MTLCopyAllDevices();
    ctx->device = [devices.lastObject retain];
    [devices release];
    if (!ctx->device)
    {
        hb_error("metal: unable to find Metal device");
        goto fail;
    }

    if (@available(macOS 10.14, *))
    {
        NSUInteger maxBufferLength = ctx->device.maxBufferLength;
        NSUInteger textureSize = width * height * sizeof(float);

        if (textureSize > maxBufferLength || width > 16384 || height > 16384)
        {
            hb_error("metal: unsupported texture size: %d x %d", width, height);
            goto fail;
        }
    }

    dispatch_data_t libData = dispatch_data_create(metallib_data, metallib_len, nil, nil);
    ctx->library = [ctx->device newLibraryWithData:libData error:&err];
    dispatch_release(libData);
    libData = nil;
    if (err)
    {
        hb_error("metal: failed to load Metal library: %s", err.description.UTF8String);
        goto fail;
    }

    ctx->function = [ctx->library newFunctionWithName:@(function_name)];
    if (!ctx->function)
    {
        hb_error("metal: failed to create Metal function");
        goto fail;
    }

    ctx->queue = ctx->device.newCommandQueue;
    if (!ctx->queue)
    {
        hb_error("metal: failed to create Metal command queue");
        goto fail;
    }

    ctx->pipeline = [ctx->device newComputePipelineStateWithFunction:ctx->function error:&err];
    if (!ctx->pipeline)
    {
        hb_error("metal: failed to create Metal compute pipeline: %s", err.description.UTF8String);
        goto fail;
    }

    ctx->params_buffer = [ctx->device newBufferWithLength:params_buffer_len
                                                  options:MTLResourceStorageModeShared];
    if (!ctx->params_buffer)
    {
        hb_error("metal: failed to create Metal buffer for parameters");
        goto fail;
    }

    CVReturn ret = CVMetalTextureCacheCreate(NULL, NULL, ctx->device, NULL, &ctx->cache);
    if (ret != kCVReturnSuccess)
    {
        hb_error("metal: failed to create CVMetalTextureCache: %d", ret);
        goto fail;
    }

    ctx->pool = hb_cv_create_pixel_buffer_pool(width, height, pix_fmt, color_range);
    if (ctx->pool == NULL)
    {
        hb_log("metal: failed to create CVPixelBufferPool failed");
        goto fail;
    }

    return ctx;

fail:
    if (ctx)
    {
        hb_metal_context_close(&ctx);
    }
    return NULL;
}

void hb_metal_context_close(hb_metal_context_t **_ctx)
{
    hb_metal_context_t *ctx = *_ctx;
    if (ctx)
    {
        [ctx->params_buffer release];
        [ctx->function release];
        [ctx->pipeline release];
        [ctx->queue release];
        [ctx->library release];
        [ctx->device release];

        if (ctx->cache)
        {
            CFRelease(ctx->cache);
        }
        if (ctx->pool)
        {
            CVPixelBufferPoolRelease(ctx->pool);
        }

    }
    free(ctx);
    *_ctx = NULL;
}

void hb_metal_compute_encoder_dispatch(id<MTLDevice> device,
                                       id<MTLComputePipelineState> pipeline,
                                       id<MTLComputeCommandEncoder> encoder,
                                       NSUInteger width, NSUInteger height)
{
    [encoder setComputePipelineState:pipeline];
    NSUInteger w = pipeline.threadExecutionWidth;
    NSUInteger h = pipeline.maxTotalThreadsPerThreadgroup / w;
    MTLSize threadsPerThreadgroup = MTLSizeMake(w, h, 1);
    BOOL fallback = YES;

    if (@available(macOS 10.15, iOS 11, tvOS 14.5, *))
    {
        if ([device supportsFamily:MTLGPUFamilyCommon3])
        {
            MTLSize threadsPerGrid = MTLSizeMake(width, height, 1);
            [encoder dispatchThreads:threadsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];
            fallback = NO;
        }
    }
    if (fallback)
    {
        MTLSize threadgroups = MTLSizeMake((width + w - 1) / w,
                                           (height + h - 1) / h,
                                           1);
        [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
    }
}

void hb_metal_compute_encoder_dispatch_fixed_threadgroup_size(id<MTLDevice> device,
                                                              id<MTLComputePipelineState> pipeline,
                                                              id<MTLComputeCommandEncoder> encoder,
                                                              NSUInteger width, NSUInteger height,
                                                              NSUInteger w, NSUInteger h)
{
    [encoder setComputePipelineState:pipeline];
    MTLSize threadsPerThreadgroup = MTLSizeMake(w, h, 1);
    MTLSize threadgroups = MTLSizeMake((width + w - 1) / w,
                                       (height + h - 1) / h,
                                       1);
    [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
}

MTLPixelFormat hb_metal_pix_fmt_from_component(const AVComponentDescriptor *comp, int *channels_out)
{
    MTLPixelFormat format;
    int pixel_size = (comp->depth + comp->shift) / 8;
    int channels = comp->step / pixel_size;
    if (pixel_size > 2 || channels > 2)
    {
        hb_log("metal: unsupported pixel format");
        return MTLPixelFormatInvalid;
    }
    switch (pixel_size)
    {
        case 1:
            format = channels == 1 ? MTLPixelFormatR8Unorm : MTLPixelFormatRG8Unorm;
            break;
        case 2:
            format = channels == 1 ? MTLPixelFormatR16Unorm : MTLPixelFormatRG16Unorm;
            break;
        default:
            hb_log("metal: unsupported pixel format");
            return MTLPixelFormatInvalid;
    }

    *channels_out = channels;
    return format;
}

CVMetalTextureRef hb_metal_texture_from_pixbuf(CVMetalTextureCacheRef textureCache,
                                               CVPixelBufferRef pixbuf,
                                               int plane,
                                               MTLPixelFormat format)
{
    CVMetalTextureRef tex = NULL;
    CVReturn ret;

    ret = CVMetalTextureCacheCreateTextureFromImage(
        NULL,
        textureCache,
        pixbuf,
        NULL,
        format,
        CVPixelBufferGetWidthOfPlane(pixbuf, plane),
        CVPixelBufferGetHeightOfPlane(pixbuf, plane),
        plane,
        &tex
    );
    if (ret != kCVReturnSuccess)
    {
        hb_log("metal: failed to create CVMetalTexture from image: %d", ret);
        return NULL;
    }

    return tex;
}
