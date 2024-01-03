/* deinterlace_vt.m

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/decomb.h"
#include "cv_utils.h"
#include "metal_utils.h"

extern char hb_yadif_vt_metallib_data[];
extern unsigned int hb_yadif_vt_metallib_len;

extern char hb_bwdif_vt_metallib_data[];
extern unsigned int hb_bwdif_vt_metallib_len;

struct mtl_deint_params
{
    uint channels;
    uint parity;
    uint tff;
    bool is_second_field;
    bool skip_spatial_check;
    bool is_field_end;
};

#define PREV 0
#define CURR 1
#define NEXT 2

struct hb_filter_private_s
{
    hb_metal_context_t       *mtl;
    const AVPixFmtDescriptor *desc;

    int mode;
    int parity;

    hb_buffer_t        *ref[3];
    hb_buffer_list_t    out_list;

    hb_filter_init_t    input;
    hb_filter_init_t    output;
};

static int yadif_vt_init(hb_filter_object_t *filter,
                         hb_filter_init_t   *init);
static int bwdif_vt_init(hb_filter_object_t *filter,
                         hb_filter_init_t   *init);
static int deinterlace_vt_init(hb_filter_object_t *filter,
                               hb_filter_init_t   *init);

static int deinterlace_vt_work(hb_filter_object_t *filter,
                               hb_buffer_t **buf_in,
                               hb_buffer_t **buf_out);

static void deinterlace_vt_close(hb_filter_object_t *filter);

/*
 *  mode   - deinterlace mode
 *  parity - field parity
 *  Modes:
 *      1 = Enabled ("send_frame")
 *      2 = Spatial [Yadif only]
 *      4 = Bob ("send_field")
 *      8 = Selective
 *  Parity:
 *      0  = Top Field First
 *      1  = Bottom Field First
 *     -1  = Automatic detection of field parity
 */

const char deinterlace_vt_template[] = "mode=^"HB_INT_REG"$:parity=^([01])$";

hb_filter_object_t hb_filter_yadif_vt =
{
    .id                = HB_FILTER_YADIF_VT,
    .enforce_order     = 1,
    .name              = "Yadif (VideoToolbox)",
    .settings          = NULL,
    .init              = yadif_vt_init,
    .work              = deinterlace_vt_work,
    .close             = deinterlace_vt_close,
    .settings_template = deinterlace_vt_template,
};

hb_filter_object_t hb_filter_bwdif_vt =
{
    .id                = HB_FILTER_BWDIF_VT,
    .enforce_order     = 1,
    .name              = "Bwdif (VideoToolbox)",
    .settings          = NULL,
    .init              = bwdif_vt_init,
    .work              = deinterlace_vt_work,
    .close             = deinterlace_vt_close,
    .settings_template = deinterlace_vt_template,
};

static int yadif_vt_init(hb_filter_object_t *filter,
                         hb_filter_init_t   *init)
{
    return deinterlace_vt_init(filter, init);
}

static int bwdif_vt_init(hb_filter_object_t *filter,
                         hb_filter_init_t   *init)
{
    return deinterlace_vt_init(filter, init);
}

static int deinterlace_vt_init(hb_filter_object_t *filter,
                               hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("deinterlace_vt: calloc failed");
        return -1;
    }

    hb_filter_private_t *pv = filter->private_data;
    pv->input = *init;
    hb_buffer_list_clear(&pv->out_list);

    hb_dict_t *settings = filter->settings;
    int mode = 3, parity = -1;

    hb_dict_extract_int(&mode,   settings, "mode");
    hb_dict_extract_int(&parity, settings, "parity");

    pv->mode   = mode;
    pv->parity = parity;
    pv->desc   = av_pix_fmt_desc_get(init->pix_fmt);

    if (filter->id == HB_FILTER_YADIF_VT)
    {
        pv->mtl = hb_metal_context_init(hb_yadif_vt_metallib_data,
                                        hb_yadif_vt_metallib_len,
                                        "deint",
                                        sizeof(struct mtl_deint_params),
                                        init->geometry.width, init->geometry.height,
                                        init->pix_fmt, init->color_range);
    }
    else if (filter->id == HB_FILTER_BWDIF_VT)
    {
        pv->mtl = hb_metal_context_init(hb_bwdif_vt_metallib_data,
                                        hb_bwdif_vt_metallib_len,
                                        "deint",
                                        sizeof(struct mtl_deint_params),
                                        init->geometry.width, init->geometry.height,
                                        init->pix_fmt, init->color_range);
    }

    if (pv->mtl == NULL)
    {
        hb_error("deinterlace_vt: failed to create Metal device");
        return -1;
    }

    pv->output = *init;
    return 0;
}

static void deinterlace_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_metal_context_close(&pv->mtl);

    for (int i = 0; i < 3; i++)
    {
        hb_buffer_close(&pv->ref[i]);
    }

    free(pv);
    filter->private_data = NULL;
}

static void call_kernel(hb_filter_private_t *pv,
                        id<MTLTexture> dst,
                        id<MTLTexture> prev,
                        id<MTLTexture> cur,
                        id<MTLTexture> next,
                        int channels,
                        int parity,
                        int tff)
{
    id<MTLCommandBuffer> buffer = pv->mtl->queue.commandBuffer;
    id<MTLComputeCommandEncoder> encoder = buffer.computeCommandEncoder;
    struct mtl_deint_params *params = (struct mtl_deint_params *)pv->mtl->params_buffer.contents;
    *params = (struct mtl_deint_params)
    {
        .channels = channels,
        .parity = parity,
        .tff = tff,
        .is_second_field = !(parity ^ tff),
        .skip_spatial_check = (pv->mode & MODE_YADIF_SPATIAL) == 0,
        .is_field_end = false
    };

    [encoder setTexture:dst  atIndex:0];
    [encoder setTexture:prev atIndex:1];
    [encoder setTexture:cur  atIndex:2];
    [encoder setTexture:next atIndex:3];
    [encoder setBuffer:pv->mtl->params_buffer offset:0 atIndex:0];

    hb_metal_compute_encoder_dispatch(pv->mtl->device, pv->mtl->pipelines[0], encoder, dst.width, dst.height);

    [encoder endEncoding];

    [buffer commit];
    [buffer waitUntilCompleted];
}

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

static hb_buffer_t * filter_frame(hb_filter_private_t *pv, int parity, int tff)
{
    CVReturn ret = kCVReturnSuccess;
    hb_buffer_t *out = NULL;

    CVPixelBufferRef cv_dest = NULL;
    CVPixelBufferRef cv_prev = pv->ref[PREV] ? hb_cv_get_pixel_buffer(pv->ref[PREV]) : hb_cv_get_pixel_buffer(pv->ref[CURR]);
    CVPixelBufferRef cv_cur = hb_cv_get_pixel_buffer(pv->ref[CURR]);
    CVPixelBufferRef cv_next = pv->ref[NEXT] ? hb_cv_get_pixel_buffer(pv->ref[NEXT]) : hb_cv_get_pixel_buffer(pv->ref[CURR]);

    if (cv_prev == NULL || cv_cur == NULL || cv_next == NULL)
    {
        hb_log("bwdif_vt: extract_buf failed");
        goto fail;
    }

    ret = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->mtl->pool, &cv_dest);
    if (ret != kCVReturnSuccess)
    {
        hb_log("bwdif_vt: CVPixelBufferPoolCreatePixelBuffer failed");
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

        CVMetalTextureRef prev = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_prev, i, format);
        CVMetalTextureRef cur  = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_cur,  i, format);
        CVMetalTextureRef next = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_next, i, format);
        CVMetalTextureRef dest = hb_metal_create_texture_from_pixbuf(pv->mtl->cache, cv_dest, i, format);

        id<MTLTexture> tex_prev = CVMetalTextureGetTexture(prev);
        id<MTLTexture> tex_cur  = CVMetalTextureGetTexture(cur);
        id<MTLTexture> tex_next = CVMetalTextureGetTexture(next);
        id<MTLTexture> tex_dest = CVMetalTextureGetTexture(dest);

        call_kernel(pv, tex_dest, tex_prev, tex_cur, tex_next, channels, parity, tff);

        CFRelease(prev);
        CFRelease(cur);
        CFRelease(next);
        CFRelease(dest);
    }

    CVBufferPropagateAttachments(cv_cur, cv_dest);

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
    hb_buffer_copy_props(out, pv->ref[CURR]);

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

static void process_frame(hb_filter_private_t *pv)
{
    if ((pv->mode & MODE_DECOMB_SELECTIVE) &&
         pv->ref[CURR]->s.combed == HB_COMB_NONE)
    {
        // Input buffer is not combed, just make a dup of it
        hb_buffer_t *buf = hb_buffer_dup(pv->ref[CURR]);
        hb_buffer_list_append(&pv->out_list, buf);
    }
    else
    {
        // Determine if top-field first layout
        int tff;
        if (pv->parity < 0)
        {
            uint16_t flags = pv->ref[CURR]->s.flags;
            tff = ((flags & PIC_FLAG_PROGRESSIVE_FRAME) == 0) ?
                  !!(flags & PIC_FLAG_TOP_FIELD_FIRST) : 1;
        }
        else
        {
            tff = (pv->parity & 1) ^ 1;
        }

        // Deinterlace both fields if Bob
        int num_frames = 1;
        if (pv->mode & MODE_XXDIF_BOB)
        {
            num_frames = 2;
        }

        // Perform filtering
        for (int frame = 0; frame < num_frames; frame++)
        {
            hb_buffer_t *buf;
            int parity = frame ^ tff ^ 1;

            @autoreleasepool
            {
                buf = filter_frame(pv, parity, tff);
            }
            hb_buffer_list_append(&pv->out_list, buf);
        }

        // If this frame was deinterlaced and Bob mode is engaged,
        // halve the duration of the saved timestamps
        if (pv->mode & MODE_XXDIF_BOB)
        {
            hb_buffer_t *first  = hb_buffer_list_head(&pv->out_list);
            hb_buffer_t *second = hb_buffer_list_tail(&pv->out_list);
            first->s.stop  -= (first->s.stop - first->s.start) / 2LL;
            second->s.start = first->s.stop;
            second->s.new_chap = 0;
        }
    }
}

static int deinterlace_vt_work(hb_filter_object_t *filter,
                               hb_buffer_t **buf_in,
                               hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;
    *buf_in = NULL;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        store_buf(pv, NULL);
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
