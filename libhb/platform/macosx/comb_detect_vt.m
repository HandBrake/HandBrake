/* comb_detect.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

*/

#include "handbrake/handbrake.h"
#include "cv_utils.h"
#include "metal_utils.h"
#include "vt_common.h"

extern char hb_comb_detect_vt_metallib_data[];
extern unsigned int hb_comb_detect_vt_metallib_len;

struct mtl_comb_detect_params
{
    bool force_exaustive_check;
};

#define MODE_GAMMA        1 // Scale gamma when decombing
#define MODE_FILTER       2 // Filter combing mask
#define MODE_MASK         4 // Output combing masks instead of pictures
#define MODE_COMPOSITE    8 // Overlay combing mask onto picture

#define FILTER_CLASSIC 1
#define FILTER_ERODE_DILATE 2

struct hb_filter_private_s
{
    hb_metal_context_t *mtl;
    const AVPixFmtDescriptor *desc;

    // comb detect parameters
    int  mode;
    int  filter_mode;
    int  spatial_metric;
    float  motion_threshold;
    float  spatial_threshold;
    int    block_threshold;
    int    block_width;
    int    block_height;

    // Computed parameters
    float  gamma_motion_threshold;
    float  gamma_spatial_threshold;
    float  gamma_spatial_threshold6;
    float  spatial_threshold_squared;
    float  spatial_threshold6;
    float  comb32detect_min;
    float  comb32detect_max;

    bool   force_exaustive_check;

    // Mask textures
    id<MTLTexture> mask;
    id<MTLTexture> temp;

    // Comb result
    id<MTLBuffer> combed;

    hb_buffer_t       *ref[3];
    hb_buffer_list_t   out_list;

    // Filter statistics
    int comb_heavy;
    int comb_light;
    int comb_none;
    int frames;
};

static int comb_detect_vt_init(hb_filter_object_t *filter,
                            hb_filter_init_t *init);

static int comb_detect_vt_work(hb_filter_object_t *filter,
                            hb_buffer_t **buf_in,
                            hb_buffer_t **buf_out );

static void comb_detect_vt_close(hb_filter_object_t *filter);

static const char comb_detect_vt_template[] =
    "mode=^"HB_INT_REG"$:spatial-metric=^([012])$:"
    "motion-thresh=^"HB_INT_REG"$:spatial-thresh=^"HB_INT_REG"$:"
    "filter-mode=^([012])$:block-thresh=^"HB_INT_REG"$:"
    "block-width=^"HB_INT_REG"$:block-height=^"HB_INT_REG"$:"
    "disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_comb_detect_vt =
{
    .id                = HB_FILTER_COMB_DETECT_VT,
    .enforce_order     = 1,
    .name              = "Comb Detect (VideoToolbox)",
    .settings          = NULL,
    .init              = comb_detect_vt_init,
    .work              = comb_detect_vt_work,
    .close             = comb_detect_vt_close,
    .settings_template = comb_detect_vt_template,
};

#define PREV 0
#define CURR 1
#define NEXT 2

static void store_buf(hb_filter_private_t *pv, hb_buffer_t *in)
{
    if (pv->ref[PREV])
    {
        hb_buffer_close(&pv->ref[PREV]);
    }
    pv->ref[PREV] = pv->ref[CURR];
    pv->ref[CURR] = pv->ref[NEXT];
    pv->ref[NEXT] = in;
}

static int comb_detect_vt_init(hb_filter_object_t *filter,
                               hb_filter_init_t   *init)
{
    filter->private_data = calloc(1, sizeof(struct hb_filter_private_s));
    if (filter->private_data == NULL)
    {
        hb_error("comb_detect_vt: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;

    hb_buffer_list_clear(&pv->out_list);

    pv->desc = av_pix_fmt_desc_get(init->pix_fmt);

    pv->frames = 0;
    pv->force_exaustive_check = 1;
    pv->comb_heavy = 0;
    pv->comb_light = 0;
    pv->comb_none = 0;

    pv->mode              = MODE_GAMMA | MODE_FILTER;
    pv->filter_mode       = FILTER_ERODE_DILATE;
    pv->spatial_metric    = 2;
    pv->motion_threshold  = 3;
    pv->spatial_threshold = 3;
    pv->block_threshold   = 40;
    pv->block_width       = 16;
    pv->block_height      = 16;

    if (filter->settings)
    {
        int motion_threshold, spatial_threshold, block_threshold;
        hb_value_t *dict = filter->settings;

        // Get comb detection settings
        hb_dict_extract_int(&pv->mode, dict, "mode");
        hb_dict_extract_int(&pv->spatial_metric, dict, "spatial-metric");
        hb_dict_extract_int(&motion_threshold, dict, "motion-thresh");
        hb_dict_extract_int(&spatial_threshold, dict, "spatial-thresh");
        hb_dict_extract_int(&pv->filter_mode, dict, "filter-mode");
        hb_dict_extract_int(&block_threshold, dict, "block-thresh");
        hb_dict_extract_int(&pv->block_width, dict, "block-width");
        hb_dict_extract_int(&pv->block_height, dict, "block-height");

        pv->motion_threshold  = motion_threshold;
        pv->spatial_threshold = spatial_threshold;
        pv->block_threshold   = block_threshold;
    }

    pv->motion_threshold  /= 255.f;
    pv->spatial_threshold /= 255.f;

    // Compute thresholds
    pv->gamma_motion_threshold    = pv->motion_threshold;
    pv->gamma_spatial_threshold   = pv->spatial_threshold;
    pv->gamma_spatial_threshold6  = 6 * pv->gamma_spatial_threshold;
    pv->spatial_threshold_squared = pv->spatial_threshold * pv->spatial_threshold;
    pv->spatial_threshold6        = 6 * pv->spatial_threshold;
    pv->comb32detect_min = 10 / 255.f;
    pv->comb32detect_max = 15 / 255.f;

    if (pv->block_width > 32)  {pv->block_width  = 32;}
    if (pv->block_height > 32) {pv->block_height = 32;}
    if (pv->block_width < 8)   {pv->block_width  = 8; }
    if (pv->block_height < 8)  {pv->block_height = 8; }

    MTLFunctionConstantValues *constant_values = [MTLFunctionConstantValues new];
    [constant_values setConstantValue:&pv->spatial_metric    type:MTLDataTypeInt withName:@"spatial_metric"];
    [constant_values setConstantValue:&pv->motion_threshold  type:MTLDataTypeFloat withName:@"motion_threshold"];
    [constant_values setConstantValue:&pv->spatial_threshold type:MTLDataTypeFloat withName:@"spatial_threshold"];
    [constant_values setConstantValue:&pv->block_threshold   type:MTLDataTypeInt withName:@"block_threshold"];
    [constant_values setConstantValue:&pv->block_width       type:MTLDataTypeInt withName:@"block_width"];
    [constant_values setConstantValue:&pv->block_height      type:MTLDataTypeInt withName:@"block_height"];

    [constant_values setConstantValue:&pv->gamma_motion_threshold    type:MTLDataTypeFloat withName:@"gamma_motion_threshold"];
    [constant_values setConstantValue:&pv->gamma_spatial_threshold   type:MTLDataTypeFloat withName:@"gamma_spatial_threshold"];
    [constant_values setConstantValue:&pv->gamma_spatial_threshold6  type:MTLDataTypeFloat withName:@"gamma_spatial_threshold6"];
    [constant_values setConstantValue:&pv->spatial_threshold_squared type:MTLDataTypeFloat withName:@"spatial_threshold_squared"];
    [constant_values setConstantValue:&pv->spatial_threshold6 type:MTLDataTypeFloat withName:@"spatial_threshold6"];
    [constant_values setConstantValue:&pv->comb32detect_min   type:MTLDataTypeFloat withName:@"comb32detect_min"];
    [constant_values setConstantValue:&pv->comb32detect_max   type:MTLDataTypeFloat withName:@"comb32detect_max"];

    pv->mtl = hb_metal_context_init(hb_comb_detect_vt_metallib_data,
                                    hb_comb_detect_vt_metallib_len,
                                    pv->mode & MODE_GAMMA ? "comb_detect_gamma" : "comb_detect",
                                    constant_values,
                                    sizeof(struct mtl_comb_detect_params),
                                    init->geometry.width, init->geometry.height,
                                    init->pix_fmt, init->color_range);
    if (pv->mtl == NULL)
    {
        [constant_values release];
        hb_error("comb_detect_vt: failed to create Metal device");
        return -1;
    }

    struct mtl_comb_detect_params *params = (struct mtl_comb_detect_params *)pv->mtl->params_buffer.contents;
    *params = (struct mtl_comb_detect_params) {
        .force_exaustive_check = pv->force_exaustive_check
    };

    if (hb_metal_add_pipeline(pv->mtl, pv->filter_mode == FILTER_ERODE_DILATE ? "filter_erode_dilate" : "filter_classic",
                               constant_values, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }
    if (hb_metal_add_pipeline(pv->mtl, "erode_mask", NULL, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }
    if (hb_metal_add_pipeline(pv->mtl, "dilate_mask", NULL, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }
    char *check_combing_name = pv->mode & MODE_FILTER ? "check_filtered_combing_mask" : "check_combing_mask";
    if (@available(macOS 13, *))
    {
        if ([pv->mtl->device supportsFamily:MTLGPUFamilyMetal3] &&
            ((pv->block_width == 16 && pv->block_height == 16) || (pv->block_width == 32 && pv->block_height == 32)))
        {
            // Use simd_sum() to speed up the final reduction pass
            check_combing_name = pv->mode & MODE_FILTER ? "check_filtered_combing_mask_simd" : "check_combing_mask_simd";
        }
        else if ([pv->mtl->device supportsFamily:MTLGPUFamilyCommon3] &&
                 (pv->block_width * pv->block_height) % 4)
        {
            // Use quad_sum() to speed up the final reduction pass
            check_combing_name = pv->mode & MODE_FILTER ? "check_filtered_combing_mask_quad" : "check_combing_mask_quad";
        }
    }
    if (hb_metal_add_pipeline(pv->mtl,check_combing_name, constant_values, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }
    if (hb_metal_add_pipeline(pv->mtl, "apply_mask", constant_values, pv->mtl->pipelines_count))
    {
        [constant_values release];
        return -1;
    }

    [constant_values release];

    // Allocate buffers to store the mask and the comb result
    MTLTextureDescriptor *descriptor = [[MTLTextureDescriptor alloc] init];
    descriptor.textureType      = MTLTextureType2D;
    descriptor.pixelFormat      = MTLPixelFormatR8Uint;
    descriptor.width            = init->geometry.width;
    descriptor.height           = init->geometry.height;
    descriptor.depth            = 1;
    descriptor.storageMode      = MTLStorageModePrivate;
    descriptor.usage            = MTLResourceUsageRead | MTLResourceUsageWrite;

    pv->mask   = [pv->mtl->device newTextureWithDescriptor:descriptor];
    pv->temp   = [pv->mtl->device newTextureWithDescriptor:descriptor];

    [descriptor release];

    pv->combed = [pv->mtl->device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

    return 0;
}

static void comb_detect_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_log("comb detect: heavy %i | light %i | uncombed %i | total %i",
           pv->comb_heavy, pv->comb_light, pv->comb_none, pv->frames);

    [pv->combed release];
    [pv->temp release];
    [pv->mask release];

    hb_metal_context_close(&pv->mtl);

    for (int i = 0; i < 3; i++)
    {
        hb_buffer_close(&pv->ref[i]);
    }

    free(pv);
    filter->private_data = NULL;
}

static void call_kernel(hb_filter_private_t *pv,
                        id<MTLTexture> prev,
                        id<MTLTexture> cur,
                        id<MTLTexture> next,
                        id<MTLTexture> dest)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;

    int width = cur.width, height = cur.height;
    struct mtl_comb_detect_params *params = (struct mtl_comb_detect_params *)pv->mtl->params_buffer.contents;
    params->force_exaustive_check = pv->force_exaustive_check;

    [encoder setTexture:prev atIndex:0];
    [encoder setTexture:cur  atIndex:1];
    [encoder setTexture:next atIndex:2];
    [encoder setTexture:pv->mask atIndex:3];
    [encoder setTexture:pv->temp atIndex:4];
    if (pv->mode & MODE_MASK || pv->mode & MODE_COMPOSITE)
    {
        [encoder setTexture:dest atIndex:5];
    }

    [encoder setBuffer:pv->combed offset:0 atIndex:0];
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:1];

    hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[0], encoder, width, height);

    if (pv->mode & MODE_FILTER)
    {
        hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[1], encoder, width, height);

        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[2], encoder, width, height);
            hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[3], encoder, width, height);
            hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[2], encoder, width, height);
        }
    }

    if (pv->mode & MODE_FILTER && pv->filter_mode == FILTER_CLASSIC)
    {
        [encoder setTexture:pv->temp atIndex:3];
    }

    hb_metal_compute_encoder_dispatch_fixed_threadgroup_size(pv->mtl->device, pv->mtl->pipelines[4], encoder,
                                                             width, height, pv->block_width, pv->block_height);

    if (pv->mode & MODE_MASK || pv->mode & MODE_COMPOSITE)
    {
        hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[5], encoder, width, height);
    }

    [encoder endEncoding];

    [buffer commit];
    [buffer waitUntilCompleted];
}

static int analyze_frame(hb_filter_private_t *pv, hb_buffer_t **out)
{
    CVReturn ret = kCVReturnSuccess;

    CVPixelBufferRef cv_dest = NULL;
    CVPixelBufferRef cv_prev = pv->ref[PREV] ? hb_cv_get_pixel_buffer(pv->ref[PREV]) : hb_cv_get_pixel_buffer(pv->ref[CURR]);
    CVPixelBufferRef cv_cur = hb_cv_get_pixel_buffer(pv->ref[CURR]);
    CVPixelBufferRef cv_next = pv->ref[NEXT] ? hb_cv_get_pixel_buffer(pv->ref[NEXT]) : hb_cv_get_pixel_buffer(pv->ref[CURR]);

    if (cv_prev == NULL || cv_cur == NULL || cv_next == NULL)
    {
        hb_log("comb_detect_vt: extract_buf failed");
        goto fail;
    }

    const AVComponentDescriptor *comp = &pv->desc->comp[0];

    int channels;
    const MTLPixelFormat format = hb_metal_pix_fmt_from_component(comp, &channels);
    if (format == MTLPixelFormatInvalid)
    {
        goto fail;
    }

    CVMetalTextureRef dest = NULL;
    id<MTLTexture> tex_dest = nil;

    if (pv->mode & MODE_MASK || pv->mode & MODE_COMPOSITE)
    {
        ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
        if (ret != kCVReturnSuccess)
        {
            hb_log("comb_detect_vt: CVPixelBufferPoolCreatePixelBuffer failed");
            goto fail;
        }
        dest = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_dest, 0, format);
        tex_dest = CVMetalTextureGetTexture(dest);
    }

    CVMetalTextureRef prev = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_prev, 0, format);
    CVMetalTextureRef cur  = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_cur,  0, format);
    CVMetalTextureRef next = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_next, 0, format);

    id<MTLTexture> tex_prev = CVMetalTextureGetTexture(prev);
    id<MTLTexture> tex_cur  = CVMetalTextureGetTexture(cur);
    id<MTLTexture> tex_next = CVMetalTextureGetTexture(next);

    uint32_t *combed = pv->combed.contents;
    *combed = HB_COMB_NONE;

    call_kernel(pv, tex_prev, tex_cur, tex_next, tex_dest);

    CFRelease(prev);
    CFRelease(cur);
    CFRelease(next);

    if (pv->mode & MODE_MASK || pv->mode & MODE_COMPOSITE)
    {
        CFRelease(dest);
        CVBufferPropagateAttachments(cv_cur, cv_dest);

        *out = hb_buffer_wrapper_init();
        (*out)->storage_type = COREMEDIA;
        (*out)->storage      = cv_dest;
        (*out)->f.width           = pv->ref[CURR]->f.width;
        (*out)->f.height          = pv->ref[CURR]->f.height;
        (*out)->f.fmt             = pv->ref[CURR]->f.fmt;
        (*out)->f.color_prim      = pv->ref[CURR]->f.color_prim;
        (*out)->f.color_transfer  = pv->ref[CURR]->f.color_transfer;
        (*out)->f.color_matrix    = pv->ref[CURR]->f.color_matrix;
        (*out)->f.color_range     = pv->ref[CURR]->f.color_range;
        (*out)->f.chroma_location = pv->ref[CURR]->f.chroma_location;
        hb_buffer_copy_props(*out,  pv->ref[CURR]);
    }

    return *combed;

fail:
    return -1;
}

static void process_frame(hb_filter_private_t *pv)
{
    int combed = 0;
    hb_buffer_t *out = NULL;

    @autoreleasepool
    {
        combed = analyze_frame(pv, &out);
    }

    switch (combed)
    {
        case HB_COMB_HEAVY:
            pv->comb_heavy++;
            break;

        case HB_COMB_LIGHT:
            pv->comb_light++;
            break;

        case HB_COMB_NONE:
        default:
            pv->comb_none++;
            break;
    }
    pv->frames++;
    pv->ref[CURR]->s.combed = combed;
    if (out)
    {
        hb_buffer_list_append(&pv->out_list, out);
    }
    else
    {
        hb_buffer_list_append(&pv->out_list, hb_vt_buffer_dup(pv->ref[CURR]));
    }

    pv->force_exaustive_check = 0;
}

static int comb_detect_vt_work(hb_filter_object_t *filter,
                               hb_buffer_t **buf_in,
                               hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t         *in = *buf_in;

    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        store_buf(pv, NULL);
        pv->force_exaustive_check = 1;
        process_frame(pv);
        hb_buffer_list_append(&pv->out_list, in);
        *buf_out = hb_buffer_list_clear(&pv->out_list);
        return HB_FILTER_DONE;
    }

    store_buf(pv, in);

    if (pv->ref[CURR] == NULL)
    {
        // Wait for next buffer
        return HB_FILTER_DELAY;
    }

    process_frame(pv);
    *buf_out = hb_buffer_list_clear(&pv->out_list);

    return *buf_out == NULL ? HB_FILTER_FAILED : HB_FILTER_OK;
}
