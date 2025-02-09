/* rendersub.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/extradata.h"
#include <ass/ass.h>

#define ABS(a) ((a) > 0 ? (a) : (-(a)))

typedef struct hb_box_s
{
    int x1, y1, x2, y2;
} hb_box_t;

typedef struct hb_box_vec_s
{
    hb_box_t *boxes;
    int count;
    int size;
} hb_box_vec_t;

struct hb_filter_private_s
{
    // Common
    int                pix_fmt_alpha;
    int                hshift;
    int                wshift;
    int                crop[4];
    int                type;
    struct SwsContext *sws;
    int                sws_width;
    int                sws_height;

    hb_buffer_list_t   rendered_sub_list;
    int                changed;

    // VOBSUB && PGSSUB
    hb_buffer_list_t   sub_list; // List of active subs

    // SSA
    ASS_Library       *ssa;
    ASS_Renderer      *renderer;
    ASS_Track         *ssa_track;
    uint8_t            script_initialized;
    hb_box_vec_t       boxes;
    hb_csp_convert_f   rgb2yuv_fn;

    // SRT
    int                line;
    hb_buffer_t       *current_sub;

    hb_blend_object_t *blend;
    unsigned           chroma_coeffs[2][4];

    hb_filter_init_t   input;
    hb_filter_init_t   output;
};

// VOBSUB
static int vobsub_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int vobsub_work(hb_filter_object_t *filter,
                       hb_buffer_t **buf_in,
                       hb_buffer_t **buf_out);

static void vobsub_close(hb_filter_object_t *filter);


// SSA
static int ssa_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int ssa_work(hb_filter_object_t *filter,
                    hb_buffer_t **buf_in,
                    hb_buffer_t **buf_out);

static void ssa_close(hb_filter_object_t *filter);


// SRT
static int textsub_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int cc608sub_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int textsub_work(hb_filter_object_t *filter,
                        hb_buffer_t **buf_in,
                        hb_buffer_t **buf_out);

static void textsub_close(hb_filter_object_t *filter);


// PGS
static int pgssub_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int pgssub_work(hb_filter_object_t *filter,
                       hb_buffer_t **buf_in,
                       hb_buffer_t **buf_out);

static void pgssub_close(hb_filter_object_t *filter);


// Entry points
static int hb_rendersub_init(hb_filter_object_t *filter,
                             hb_filter_init_t *init);

static int hb_rendersub_post_init(hb_filter_object_t *filter, hb_job_t *job);

static int hb_rendersub_work(hb_filter_object_t *filter,
                             hb_buffer_t **buf_in,
                             hb_buffer_t **buf_out);

static void hb_rendersub_close(hb_filter_object_t *filter);

hb_filter_object_t hb_filter_render_sub =
{
    .id            = HB_FILTER_RENDER_SUB,
    .enforce_order = 1,
    .name          = "Subtitle renderer",
    .settings      = NULL,
    .init          = hb_rendersub_init,
    .post_init     = hb_rendersub_post_init,
    .work          = hb_rendersub_work,
    .close         = hb_rendersub_close,
};

static void hb_box_vec_resize(hb_box_vec_t *vec, int size)
{
    hb_box_t *boxes = realloc(vec->boxes, size * sizeof(hb_box_t));
    if (boxes == NULL)
    {
        return; // Error. Should never happen.
    }
    vec->boxes = boxes;
    vec->size = size;
}

static inline int hb_box_intersect(const hb_box_t *a, const hb_box_t *b, int offset)
{
    return ((FFMIN(a->x2, b->x2) + offset - FFMAX(a->x1, b->x1)) >= 0) &&
           ((FFMIN(a->y2, b->y2) + offset - FFMAX(a->y1, b->y1)) >= 0);
}

static inline void hb_box_union(hb_box_t *a, const hb_box_t *b)
{
    if (a->x1 > b->x1) { a->x1 = b->x1; }
    if (a->y1 > b->y1) { a->y1 = b->y1; }
    if (a->x2 < b->x2) { a->x2 = b->x2; }
    if (a->y2 < b->y2) { a->y2 = b->y2; }
}

static inline void hb_box_clear(hb_box_t *box)
{
    box->x1 = box->y1 = box->x2 = box->y2 = 0;
}

static void hb_box_vec_merge(hb_box_vec_t *vec)
{
    if (vec->count < 2)
    {
        return;
    }

    for (int i = 0; i < vec->count - 1; i++)
    {
        hb_box_t *a = &vec->boxes[i];
        for (int j = i + 1; j < vec->count; j++)
        {
            hb_box_t *b = &vec->boxes[j];
            if (hb_box_intersect(a, b, 8))
            {
                hb_box_union(a, b);
                hb_box_clear(b);
            }
        }
    }
}

static void hb_box_vec_compact(hb_box_vec_t *vec)
{
    int j = 0, i = 0;
    while (i < vec->count)
    {
        hb_box_t a = vec->boxes[i];
        if (a.x2 != 0 || a.y2 != 0)
        {
            vec->boxes[j] = a;
            j++;
        }
        i++;
    }
    vec->count = j;
}

static void hb_box_vec_append(hb_box_vec_t *vec, int x1, int y1, int x2, int y2)
{
    if (x1 == x2 || y1 == y2)
    {
        return; // Empty box
    }

    if (vec->size < vec->count + 1)
    {
        hb_box_vec_resize(vec, vec->count + 10);
    }

    if (vec->size < vec->count + 1)
    {
        return; // Error. Should never happen.
    }

    vec->boxes[vec->count].x1 = x1;
    vec->boxes[vec->count].y1 = y1;
    vec->boxes[vec->count].x2 = x2;
    vec->boxes[vec->count].y2 = y2;
    vec->count += 1;

    hb_box_vec_merge(vec);
    hb_box_vec_compact(vec);
}

static void hb_box_vec_clear(hb_box_vec_t *vec)
{
    memset(vec->boxes, 0, sizeof(hb_box_t) * vec->size);
    vec->count = 0;
}

static void hb_box_vec_close(hb_box_vec_t *vec)
{
    free(vec->boxes);
    vec->boxes = NULL;
    vec->count = 0;
    vec->size = 0;
}

static hb_buffer_t * scale_subtitle(hb_filter_private_t *pv,
                                    hb_buffer_t *sub, hb_buffer_t *buf)
{
    hb_buffer_t *scaled;
    double xfactor = 1., yfactor = 1.;

    // Do we need to rescale subtitles?
    if (sub->f.window_width > 0 && sub->f.window_height > 0)
    {
        // TODO: Factor aspect ratio
        // For now, assume subtitle and video PAR is the same.
        xfactor = (double)buf->f.width  / sub->f.window_width;
        yfactor = (double)buf->f.height / sub->f.window_height;
        // The video may have been cropped.  This will make xfactor != yfactor
        // even though video and subtitles are the same PAR.  So use the
        // larger of as the scale factor.
        if (xfactor > yfactor)
        {
            yfactor = xfactor;
        }
        else
        {
            xfactor = yfactor;
        }
    }
    if (ABS(xfactor - 1) > 0.01 || ABS(yfactor - 1) > 0.01)
    {
        uint8_t *in_data[4],  *out_data[4];
        int      in_stride[4], out_stride[4];
        int      width, height;

        width  = sub->f.width  * xfactor;
        height = sub->f.height * yfactor;
        // Note that subtitle frame buffer has alpha and should always be 444P
        scaled = hb_frame_buffer_init(sub->f.fmt, width, height);
        if (scaled == NULL)
        {
            return NULL;
        }

        scaled->f.x = sub->f.x * xfactor;
        scaled->f.y = sub->f.y * yfactor;

        hb_picture_fill(in_data,  in_stride,  sub);
        hb_picture_fill(out_data, out_stride, scaled);

        if (pv->sws        == NULL   ||
            pv->sws_width  != width  ||
            pv->sws_height != height)
        {
            if (pv->sws!= NULL)
            {
                sws_freeContext(pv->sws);
            }
            pv->sws = hb_sws_get_context(
                                sub->f.width, sub->f.height, sub->f.fmt, AVCOL_RANGE_MPEG,
                                scaled->f.width, scaled->f.height, sub->f.fmt, AVCOL_RANGE_MPEG,
                                SWS_LANCZOS|SWS_ACCURATE_RND, SWS_CS_DEFAULT);
            pv->sws_width  = width;
            pv->sws_height = height;
        }
        sws_scale(pv->sws, (const uint8_t* const *)in_data, in_stride,
                  0, sub->f.height, out_data, out_stride);
    }
    else
    {
        scaled = hb_buffer_dup(sub);
    }

    int top, left, margin_top, margin_percent;

    // Percent of height of picture that form a margin that subtitles
    //should not be displayed within.
    margin_percent = 2;

    // If necessary, move the subtitle so it is not in a cropped zone.
    // When it won't fit, we center it so we lose as much on both ends.
    // Otherwise we try to leave a 20px or 2% margin around it.
    margin_top = ((buf->f.height - pv->crop[0] - pv->crop[1]) *
                   margin_percent) / 100;

    if (margin_top > 20)
    {
         // A maximum margin of 20px regardless of height of the picture.
        margin_top = 20;
    }

    if (scaled->f.height > buf->f.height - pv->crop[0] - pv->crop[1] - (margin_top * 2))
    {
        // The subtitle won't fit in the cropped zone, so center
        // it vertically so we fit in as much as we can.
        top = pv->crop[0] + (buf->f.height - pv->crop[0] -
                             pv->crop[1] - scaled->f.height) / 2;
    }
    else if (scaled->f.y < pv->crop[0] + margin_top)
    {
        // The subtitle fits in the cropped zone, but is currently positioned
        // within our top margin, so move it outside of our margin.
        top = pv->crop[0] + margin_top;
    }
    else if (scaled->f.y > buf->f.height - pv->crop[1] - margin_top - scaled->f.height)
    {
        // The subtitle fits in the cropped zone, and is not within the top
        // margin but is within the bottom margin, so move it to be above
        // the margin.
        top = buf->f.height - pv->crop[1] - margin_top - scaled->f.height;
    }
    else
    {
        // The subtitle is fine where it is.
        top = scaled->f.y;
    }

    if (scaled->f.width > buf->f.width - pv->crop[2] - pv->crop[3] - 40)
    {
        left = pv->crop[2] + (buf->f.width - pv->crop[2] -
                              pv->crop[3] - scaled->f.width) / 2;
    }
    else if (scaled->f.x < pv->crop[2] + 20)
    {
        left = pv->crop[2] + 20;
    }
    else if (scaled->f.x > buf->f.width - pv->crop[3] - 20 - scaled->f.width)
    {
        left = buf->f.width - pv->crop[3] - 20 - scaled->f.width;
    }
    else
    {
        left = scaled->f.x;
    }

    scaled->f.x = left;
    scaled->f.y = top;

    return scaled;
}

// Assumes that the input buffer has the same dimensions
// as the original title dimensions
static void render_vobsubs(hb_filter_private_t *pv, hb_buffer_t *buf)
{
    hb_buffer_t *sub, *next;

    // Note that VOBSUBs can overlap in time.
    // I.e. more than one may be rendered to the screen at once.
    for (sub = hb_buffer_list_head(&pv->sub_list); sub; sub = next)
    {
        next = sub->next;

        if ((sub->s.stop != AV_NOPTS_VALUE && sub->s.stop <= buf->s.start) ||
            (next != NULL && sub->s.stop == AV_NOPTS_VALUE && next->s.start <= buf->s.start))
        {
            // Subtitle stop is in the past, delete it
            hb_buffer_list_rem(&pv->sub_list, sub);
            hb_buffer_close(&sub);
        }
        else if (sub->s.start <= buf->s.start)
        {
            // The subtitle has started before this frame and ends
            // after it. Render the subtitle into the frame.
            hb_buffer_t *scaled = scale_subtitle(pv, sub, buf);
            if (scaled)
            {
                hb_buffer_list_append(&pv->rendered_sub_list, scaled);
            }
        }
        else
        {
            // The subtitle starts in the future. No need to continue.
            break;
        }
    }
}

static int vobsub_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    filter->private_data->pix_fmt_alpha = AV_PIX_FMT_YUVA444P;
    return 0;
}

static void vobsub_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (!pv)
    {
        return;
    }

    hb_buffer_list_close(&pv->sub_list);

    free(pv);
    filter->private_data = NULL;
}

static int vobsub_work(hb_filter_object_t *filter,
                       hb_buffer_t **buf_in,
                       hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *sub;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while ((sub = hb_fifo_get(filter->subtitle->fifo_out)))
    {
        if (sub->s.flags & HB_BUF_FLAG_EOF)
        {
            hb_buffer_close(&sub);
            break;
        }
        hb_buffer_list_append(&pv->sub_list, sub);
    }

    render_vobsubs(pv, in);

    *buf_in = NULL;
    *buf_out = pv->blend->work(pv->blend, in, &pv->rendered_sub_list, 1);

    hb_buffer_list_close(&pv->rendered_sub_list);

    return HB_FILTER_OK;
}

#define ALPHA_BLEND(srcA, srcRGB, dstAc, dstRGB, outA) \
    (((srcA * srcRGB + dstRGB * dstAc + (outA >> 1)) / outA))
#define div255(x) (((x + ((x + 128) >> 8)) + 128) >> 8)

static uint8_t inline ssa_alpha(const ASS_Image *frame, const int x, const int y)
{
    const unsigned frame_a = (frame->color) & 0xff;
    const unsigned gliph_a = frame->bitmap[y*frame->stride + x];

    // Alpha for this pixel is the frame opacity (255 - frame_a)
    // multiplied by the gliph alpha (gliph_a) for this pixel
    return (uint8_t)div255((255 - frame_a) * gliph_a);
}

static hb_buffer_t * compose_subsample_ass(hb_filter_private_t *pv, const ASS_Image *frame_list,
                                           const unsigned width, const unsigned height,
                                           const unsigned x, const unsigned y)
{
    const ASS_Image *frame = frame_list;
    const unsigned flat_stride = width << 2;
    uint8_t *compo = calloc(flat_stride * height, sizeof(uint8_t));

    if (!compo)
    {
        return NULL;
    }

    while (frame)
    {
        if (frame->w && frame->h &&
            x <= frame->dst_x && x + width >= frame->dst_x + frame->w &&
            y <= frame->dst_y && y + height >= frame->dst_y + frame->h)
        {
            const int yuv = pv->rgb2yuv_fn(frame->color >> 8);

            const unsigned frame_y = (yuv >> 16) & 0xff;
            const unsigned frame_v = (yuv >> 8 ) & 0xff;
            const unsigned frame_u = (yuv >> 0 ) & 0xff;
            unsigned frame_a, res_alpha, alpha_compo, alpha_in_scaled;

            const unsigned ini_fx = (frame->dst_x - x) * 4 + (frame->dst_y - y) * flat_stride;

            for (int yy = 0, fx = ini_fx; yy < frame->h; yy++, fx = ini_fx + yy * flat_stride)
            {
                for (int xx = 0; xx < frame->w; xx++, fx += 4)
                {
                    frame_a = ssa_alpha(frame, xx, yy);

                    // Skip if transparent
                    if (frame_a)
                    {
                        if (compo[fx+3])
                        {
                            alpha_in_scaled = frame_a * 255;
                            alpha_compo = compo[fx+3] * (255 - frame_a);
                            res_alpha = (alpha_in_scaled + alpha_compo);

                            compo[fx  ] = ALPHA_BLEND(alpha_in_scaled, frame_y, alpha_compo, compo[fx  ], res_alpha);
                            compo[fx+1] = ALPHA_BLEND(alpha_in_scaled, frame_u, alpha_compo, compo[fx+1], res_alpha);
                            compo[fx+2] = ALPHA_BLEND(alpha_in_scaled, frame_v, alpha_compo, compo[fx+2], res_alpha);
                            compo[fx+3] = div255(res_alpha);
                        }
                        else
                        {
                            compo[fx  ] = frame_y;
                            compo[fx+1] = frame_u;
                            compo[fx+2] = frame_v;
                            compo[fx+3] = frame_a;
                        }
                    }
                }
            }
        }
        frame = frame->next;
    }

    hb_buffer_t *sub = hb_frame_buffer_init(pv->pix_fmt_alpha, width, height);
    if (!sub)
    {
        free(compo);
        return NULL;
    }

    sub->f.x = x;
    sub->f.y = y;

    uint8_t *y_out, *u_out, *v_out, *a_out;
    y_out = sub->plane[0].data;
    u_out = sub->plane[1].data;
    v_out = sub->plane[2].data;
    a_out = sub->plane[3].data;

    unsigned int accu_a, accu_b, accu_c, coeff, is_chroma_line;

    for (int yy = 0, ys = 0, fx = 0; yy < height; yy++, ys = yy >> pv->hshift, fx = yy * flat_stride)
    {
        is_chroma_line = yy == ys << pv->hshift;
        for (int xx = 0, xs = 0; xx < width; xx++, xs = xx >> pv->wshift, fx += 4)
        {
            y_out[xx] = compo[ fx ];
            a_out[xx] = compo[fx+3];

            // Are we on a chroma sample?
            if (is_chroma_line && xx == xs << pv->wshift)
            {
                accu_a = accu_b = accu_c = 0;
                for (int yz = 0; yz < (1 << pv->hshift) && (yz + yy < height); yz++)
                {
                    for (int xz = 0; xz < (1 << pv->wshift) && (xz + xx < width); xz++)
                    {
                        // Access compo to avoid cache misses with a_out.
                        coeff = pv->chroma_coeffs[0][xz] *
                                pv->chroma_coeffs[1][yz] *
                                compo[fx + yz * flat_stride + 3];
                        accu_a += coeff * compo[fx + yz * flat_stride + 1];
                        accu_b += coeff * compo[fx + yz * flat_stride + 2];
                        accu_c += coeff;
                    }
                }
                if (accu_c)
                {
                    u_out[xs] = (accu_a + (accu_c >> 1)) / accu_c;
                    v_out[xs] = (accu_b + (accu_c >> 1)) / accu_c;
                }
            }
        }

        if (is_chroma_line)
        {
            u_out += sub->plane[1].stride;
            v_out += sub->plane[2].stride;
        }
        y_out += sub->plane[0].stride;
        a_out += sub->plane[3].stride;
    }

    free(compo);
    return sub;
}

static void clear_ssa_rendered_sub_cache(hb_filter_private_t *pv)
{
    if (hb_buffer_list_count(&pv->rendered_sub_list))
    {
        hb_buffer_list_close(&pv->rendered_sub_list);
        hb_box_vec_clear(&pv->boxes);
    }
}

static void render_ssa_subs(hb_filter_private_t *pv, int64_t start)
{
    int changed;
    ASS_Image *frame_list = ass_render_frame(pv->renderer, pv->ssa_track,
                                             start / 90, &changed);
    if (!frame_list)
    {
        clear_ssa_rendered_sub_cache(pv);
    }
    else if (changed)
    {
        // Re-use cached overlays, whenever possible
        clear_ssa_rendered_sub_cache(pv);

        // Find overlay size and pos of non overlapped boxes
        // (faster than composing at the video dimensions)
        for (ASS_Image *frame = frame_list; frame; frame = frame->next)
        {
            hb_box_vec_append(&pv->boxes,
                               frame->dst_x, frame->dst_y,
                               frame->w + frame->dst_x, frame->h + frame->dst_y);
        }

        for (int i = 0; i < pv->boxes.count; i++)
        {
            // Overlay must be aligned to the chroma plane, pad as needed.
            hb_box_t box = pv->boxes.boxes[i];
            int x = box.x1 - ((box.x1 + pv->crop[2]) & ((1 << pv->wshift) - 1));
            int y = box.y1 - ((box.y1 + pv->crop[0]) & ((1 << pv->hshift) - 1));
            int width  = box.x2 - x;
            int height = box.y2 - y;

            hb_buffer_t *sub = compose_subsample_ass(pv, frame_list, width, height, x, y);
            if (sub)
            {
                sub->f.x += pv->crop[2];
                sub->f.y += pv->crop[0];
                hb_buffer_list_append(&pv->rendered_sub_list, sub);
            }
        }
    }
    pv->changed = changed;
}

static void ssa_log(int level, const char *fmt, va_list args, void *data)
{
    if (level < 5) // Same as default verbosity when no callback is set
    {
        hb_valog(1, "[ass]", fmt, args);
    }
}

static int ssa_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    hb_filter_private_t *pv = filter->private_data;

    switch (pv->input.pix_fmt)
    {
        case AV_PIX_FMT_NV12:
        case AV_PIX_FMT_P010:
        case AV_PIX_FMT_P012:
        case AV_PIX_FMT_P016:
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUV420P10:
        case AV_PIX_FMT_YUV420P12:
        case AV_PIX_FMT_YUV420P16:
            pv->pix_fmt_alpha = AV_PIX_FMT_YUVA420P;
            break;
        case AV_PIX_FMT_NV16:
        case AV_PIX_FMT_P210:
        case AV_PIX_FMT_P212:
        case AV_PIX_FMT_P216:
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUV422P10:
        case AV_PIX_FMT_YUV422P12:
        case AV_PIX_FMT_YUV422P16:
            pv->pix_fmt_alpha = AV_PIX_FMT_YUVA422P;
            break;
        case AV_PIX_FMT_NV24:
        case AV_PIX_FMT_P410:
        case AV_PIX_FMT_P412:
        case AV_PIX_FMT_P416:
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
        case AV_PIX_FMT_YUV444P16:
        default:
            pv->pix_fmt_alpha = AV_PIX_FMT_YUVA444P;
            break;
    }

    pv->ssa = ass_library_init();
    if (!pv->ssa)
    {
        hb_error("decssasub: libass initialization failed\n");
        return 1;
    }

    // Redirect libass output to hb_log
    ass_set_message_cb(pv->ssa, ssa_log, NULL);

    // Load embedded fonts
    hb_list_t *list_attachment = job->list_attachment;
    for (int i = 0; i < hb_list_count(list_attachment); i++)
    {
        hb_attachment_t *attachment = hb_list_item(list_attachment, i);

        if (attachment->type == FONT_TTF_ATTACH ||
            attachment->type == FONT_OTF_ATTACH)
        {
            ass_add_font(pv->ssa,
                         attachment->name,
                         attachment->data,
                         attachment->size);
        }
    }

    ass_set_extract_fonts(pv->ssa, 1);
    ass_set_style_overrides(pv->ssa, NULL);

    pv->renderer = ass_renderer_init(pv->ssa);
    if (!pv->renderer)
    {
        hb_log("decssasub: renderer initialization failed\n");
        return 1;
    }

    ass_set_use_margins(pv->renderer, 0);
    ass_set_hinting(pv->renderer, ASS_HINTING_NONE);
    ass_set_font_scale(pv->renderer, 1.0);
    ass_set_line_spacing(pv->renderer, 1.0);

    // Setup default font family
    //
    // SSA v4.00 requires that "Arial" be the default font
    const char *font = NULL;
    const char *family = "Arial";
    // NOTE: This can sometimes block for several *seconds*.
    //       It seems that process_fontdata() for some embedded fonts is slow.
    ass_set_fonts(pv->renderer, font, family, /*haveFontConfig=*/1, NULL, 1);

    // Setup track state
    pv->ssa_track = ass_new_track(pv->ssa);
    if (!pv->ssa_track)
    {
        hb_log("decssasub: ssa track initialization failed\n");
        return 1;
    }

    // Do not use Read Order to eliminate duplicates
    // we never send the same subtitles sample twice,
    // and some MKVs have duplicated Read Orders
    // and won't render properly when this is enabled.
    ass_set_check_readorder(pv->ssa_track, 0);

    int height = job->title->geometry.height - job->crop[0] - job->crop[1];
    int width = job->title->geometry.width - job->crop[2] - job->crop[3];
    ass_set_frame_size(pv->renderer, width, height);
    ass_set_storage_size(pv->renderer, width, height);

    return 0;
}

static void ssa_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (!pv)
    {
        return;
    }

    if (pv->ssa_track)
    {
        ass_free_track(pv->ssa_track);
    }
    if (pv->renderer)
    {
        ass_renderer_done(pv->renderer);
    }
    if (pv->ssa)
    {
        ass_library_done(pv->ssa);
    }
    hb_box_vec_close(&pv->boxes);

    free(pv);
    filter->private_data = NULL;
}

static void ssa_work_init(hb_filter_private_t *pv, const hb_data_t *sub_data)
{
    // NOTE: The codec extradata is expected to be in MKV format
    // I would like to initialize this in ssa_post_init, but when we are
    // transcoding text subtitles to SSA, the extradata does not
    // get initialized until the decoder is initialized.  Since
    // decoder initialization happens after filter initialization,
    // we need to postpone this.
    ass_process_codec_private(pv->ssa_track, (const char *)sub_data->bytes, sub_data->size);

    switch(pv->ssa_track->YCbCrMatrix)
    {
    case YCBCR_DEFAULT: //No YCbCrMatrix header: VSFilter default
    case YCBCR_FCC_TV:  //FCC is almost the same as 601
    case YCBCR_FCC_PC:
    case YCBCR_BT601_TV:
    case YCBCR_BT601_PC:
        pv->rgb2yuv_fn = hb_rgb2yuv;
        break;
    case YCBCR_BT709_TV:
    case YCBCR_BT709_PC:
    case YCBCR_SMPTE240M_TV:
    case YCBCR_SMPTE240M_PC:  //240M is almost the same as 709
        pv->rgb2yuv_fn = hb_rgb2yuv_bt709;
        break;
    //use video csp
    case YCBCR_UNKNOWN://cannot parse
    case YCBCR_NONE:   //explicitely requested no override
    default:
        pv->rgb2yuv_fn = hb_get_rgb2yuv_function(pv->input.color_matrix);
        break;
    }
}

static int ssa_work(hb_filter_object_t *filter,
                    hb_buffer_t **buf_in,
                    hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *sub;

    if (!pv->script_initialized)
    {
        ssa_work_init(pv, filter->subtitle->extradata);
        pv->script_initialized = 1;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while ((sub = hb_fifo_get(filter->subtitle->fifo_out)))
    {
        if (sub->s.flags & HB_BUF_FLAG_EOF)
        {
            hb_buffer_close(&sub);
            break;
        }
        // Parse MKV-SSA packet
        // SSA subtitles always have an explicit stop time, so we
        // do not need to do special processing for stop == AV_NOPTS_VALUE
        ass_process_chunk(pv->ssa_track, (char *)sub->data, sub->size,
                          sub->s.start / 90,
                          (sub->s.stop - sub->s.start) / 90);
        hb_buffer_close(&sub);
    }

    render_ssa_subs(pv, in->s.start);

    *buf_in  = NULL;
    *buf_out = pv->blend->work(pv->blend, in, &pv->rendered_sub_list, pv->changed);

    return HB_FILTER_OK;
}

static int cc608sub_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    // Text subtitles for which we create a dummy ASS header need
    // to have the header rewritten with the correct dimensions.
    int height = job->title->geometry.height - job->crop[0] - job->crop[1];
    int width = job->title->geometry.width - job->crop[2] - job->crop[3];
    int safe_height = 0.8 * job->title->geometry.height;
    // Use fixed width font for CC
    hb_set_ssa_extradata(&filter->subtitle->extradata, HB_FONT_MONO,
                         .08 * safe_height, width, height);
    return ssa_post_init(filter, job);
}

static int textsub_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    // Text subtitles for which we create a dummy ASS header need
    // to have the header rewritten with the correct dimensions.
    int height = job->title->geometry.height - job->crop[0] - job->crop[1];
    int width = job->title->geometry.width - job->crop[2] - job->crop[3];
    hb_set_ssa_extradata(&filter->subtitle->extradata, HB_FONT_SANS,
                         .066 * job->title->geometry.height,
                         width, height);
    return ssa_post_init(filter, job);
}

static void textsub_close(hb_filter_object_t *filter)
{
    return ssa_close(filter);
}

static void process_sub(hb_filter_private_t *pv, hb_buffer_t *sub)
{
    ass_process_chunk(pv->ssa_track, (char *)sub->data, sub->size,
                      sub->s.start, sub->s.stop - sub->s.start);
}

static int textsub_work(hb_filter_object_t *filter,
                        hb_buffer_t **buf_in,
                        hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *sub;

    if (!pv->script_initialized)
    {
        ssa_work_init(pv, filter->subtitle->extradata);
        pv->script_initialized = 1;
    }

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    int in_start_ms = in->s.start / 90;

    // Get any pending subtitles and add them to the active
    // subtitle list
    while ((sub = hb_fifo_get(filter->subtitle->fifo_out)))
    {
        if (sub->s.flags & HB_BUF_FLAG_EOF)
        {
            hb_buffer_close(&sub);
            if (pv->current_sub != NULL)
            {
                // Make us some duration for final sub
                pv->current_sub->s.stop = pv->current_sub->s.start +
                                          90000LL * 10;
                process_sub(pv, pv->current_sub);
                hb_buffer_close(&pv->current_sub);
            }
            break;
        }

        // libass expects times in ms.  So to make the math easy,
        // convert to ms immediately.
        sub->s.start /= 90;
        if (sub->s.stop != AV_NOPTS_VALUE)
        {
            sub->s.stop /= 90;
        }

        // Subtitle formats such as CC can have stop times
        // that are not known until an "erase display" command
        // is encountered in the stream.  For these formats
        // current_sub is the currently active subtitle for which
        // we do not yet know the stop time.  We do not currently
        // support overlapping subtitles of this type.
        if (pv->current_sub != NULL)
        {
            // Next sub start time tells us the stop time of the
            // current sub when it is not known in advance.
            pv->current_sub->s.stop = sub->s.start;
            process_sub(pv, pv->current_sub);
            hb_buffer_close(&pv->current_sub);
        }
        if (sub->s.flags & HB_BUF_FLAG_EOS)
        {
            // marker used to "clear" previous sub that had
            // an unknown duration
            hb_buffer_close(&sub);
        }
        else if (sub->s.stop == AV_NOPTS_VALUE)
        {
            // We don't know the duration of this sub.  So we will
            // apply it to every video frame until we see a "clear" sub.
            pv->current_sub = sub;
            pv->current_sub->s.stop = pv->current_sub->s.start;
        }
        else
        {
            // Duration of this subtitle is known, so we can just
            // process it normally.
            process_sub(pv, sub);
            hb_buffer_close(&sub);
        }
    }
    if (pv->current_sub != NULL && pv->current_sub->s.start <= in_start_ms)
    {
        // We don't know the duration of this subtitle, but we know
        // that it started before the current video frame and that
        // it is still active.  So render it on this video frame.
        pv->current_sub->s.start = pv->current_sub->s.stop;
        pv->current_sub->s.stop = in_start_ms + 1;
        process_sub(pv, pv->current_sub);
    }

    render_ssa_subs(pv, in->s.start);

    *buf_in  = NULL;
    *buf_out = pv->blend->work(pv->blend, in, &pv->rendered_sub_list, pv->changed);

    return HB_FILTER_OK;
}

static void render_pgs_subs(hb_filter_private_t *pv, hb_buffer_t *buf)
{
    hb_buffer_t *sub, *next, *active = NULL;

    // Each PGS subtitle supersedes anything that preceded it.
    // Find the active subtitle (if there is one), and delete
    // everything before it.
    for (sub = hb_buffer_list_head(&pv->sub_list); sub; sub = sub->next)
    {
        if (sub->s.start > buf->s.start)
        {
            break;
        }
        active = sub;
    }

    for (sub = hb_buffer_list_head(&pv->sub_list); sub; sub = next)
    {
        if (active == NULL || sub == active)
        {
            break;
        }
        next = sub->next;
        hb_buffer_list_rem(&pv->sub_list, sub);
        hb_buffer_close(&sub);
    }

    // Some PGS subtitles have no content and only serve to clear
    // the screen. If any of these are at the front of our list,
    // we can now get rid of them.
    for (sub = hb_buffer_list_head(&pv->sub_list); sub; sub = next)
    {
        if (sub->f.width != 0 && sub->f.height != 0)
        {
            break;
        }
        next = sub->next;
        hb_buffer_list_rem(&pv->sub_list, sub);
        hb_buffer_close(&sub);
    }

    // Check to see if there's an active subtitle, and apply it.
    for (sub = hb_buffer_list_head(&pv->sub_list); sub; sub = sub->next)
    {
        if (sub->s.start <= buf->s.start)
        {
            hb_buffer_t *scaled = scale_subtitle(pv, sub, buf);
            if (scaled)
            {
                hb_buffer_list_append(&pv->rendered_sub_list, scaled);
            }
        }
    }
}

static int pgssub_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    filter->private_data->pix_fmt_alpha = AV_PIX_FMT_YUVA444P;
    return 0;
}

static void pgssub_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (!pv)
    {
        return;
    }

    hb_buffer_list_close(&pv->sub_list);

    free(pv);
    filter->private_data = NULL;
}

static int pgssub_work(hb_filter_object_t *filter,
                       hb_buffer_t **buf_in,
                       hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *sub;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while ((sub = hb_fifo_get(filter->subtitle->fifo_out)))
    {
        if (sub->s.flags & HB_BUF_FLAG_EOF)
        {
            hb_buffer_close(&sub);
            break;
        }
        hb_buffer_list_append(&pv->sub_list, sub);
    }

    render_pgs_subs(pv, in);

    *buf_in = NULL;
    *buf_out = pv->blend->work(pv->blend, in, &pv->rendered_sub_list, 1);

    hb_buffer_list_close(&pv->rendered_sub_list);

    return HB_FILTER_OK;
}

static hb_blend_object_t * hb_blend_init(hb_filter_init_t init, int sub_pix_fmt)
{
    hb_blend_object_t *blend;
    switch (init.hw_pix_fmt)
    {
#if defined(__APPLE__)
        case AV_PIX_FMT_VIDEOTOOLBOX:
            blend = &hb_blend_vt;
            break;
#endif
        default:
            blend = &hb_blend;
            break;
    }

    hb_blend_object_t *blend_copy = malloc(sizeof(hb_blend_object_t));
    if (blend_copy == NULL)
    {
        hb_error("render_sub: blend malloc failed");
        return NULL;
    }

    memcpy(blend_copy, blend, sizeof(hb_blend_object_t));
    if (blend_copy->init(blend_copy, init.geometry.width, init.geometry.height,
                         init.pix_fmt, init.chroma_location,
                         init.color_range, sub_pix_fmt))
    {
        free(blend_copy);
        hb_error("render_sub: blend init failed");
        return NULL;
    }

    return blend_copy;
}

void hb_blend_close(hb_blend_object_t **_b)
{
    hb_blend_object_t *b = *_b;
    if (b == NULL)
    {
        return;
    }
    b->close(b);
    free(b);
    *_b = NULL;
}

static int hb_rendersub_init(hb_filter_object_t *filter,
                             hb_filter_init_t *init)
{
    filter->private_data = calloc(1, sizeof(struct hb_filter_private_s));
    if (filter->private_data == NULL)
    {
        hb_error("rendersub: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;
    hb_subtitle_t *subtitle;

    pv->input = *init;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->wshift = desc->log2_chroma_w;
    pv->hshift = desc->log2_chroma_h;

    hb_compute_chroma_smoothing_coefficient(pv->chroma_coeffs,
                                            init->pix_fmt,
                                            init->chroma_location);

    // Find the subtitle we need
    for (int ii = 0; ii < hb_list_count(init->job->list_subtitle); ii++)
    {
        subtitle = hb_list_item(init->job->list_subtitle, ii);
        if (subtitle && subtitle->config.dest == RENDERSUB)
        {
            // Found it
            filter->subtitle = subtitle;
            pv->type = subtitle->source;
            break;
        }
    }
    if (filter->subtitle == NULL)
    {
        hb_log("rendersub: no subtitle marked for burn");
        return 1;
    }
    pv->output = *init;

    return 0;
}

static int hb_rendersub_post_init(hb_filter_object_t *filter, hb_job_t *job)
{
    int ret = 0;
    hb_filter_private_t *pv = filter->private_data;

    pv->crop[0] = job->crop[0];
    pv->crop[1] = job->crop[1];
    pv->crop[2] = job->crop[2];
    pv->crop[3] = job->crop[3];

    switch (pv->type)
    {
        case VOBSUB:
        {
            ret = vobsub_post_init(filter, job);
            break;
        }

        case SSASUB:
        {
            ret =  ssa_post_init(filter, job);
            break;
        }

        case IMPORTSRT:
        case IMPORTSSA:
        case UTF8SUB:
        case TX3GSUB:
        {
            ret = textsub_post_init(filter, job);
            break;
        }

        case CC608SUB:
        {
            ret = cc608sub_post_init(filter, job);
            break;
        }

        case DVBSUB:
        case PGSSUB:
        {
            ret = pgssub_post_init(filter, job);
            break;
        }

        default:
        {
            hb_log("rendersub: unsupported subtitle format %d", pv->type);
            return 1;
        }
    }

    if (ret > 0)
    {
        return 1;
    }

    pv->blend = hb_blend_init(pv->input, pv->pix_fmt_alpha);

    if (pv->blend == NULL)
    {
        hb_log("rendersub: blend initialization failed");
        return 1;
    }

    return 0;
}

static int hb_rendersub_work(hb_filter_object_t *filter,
                             hb_buffer_t **buf_in,
                             hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    switch (pv->type)
    {
        case VOBSUB:
        {
            return vobsub_work(filter, buf_in, buf_out);
        }

        case SSASUB:
        {
            return ssa_work(filter, buf_in, buf_out);
        }

        case IMPORTSRT:
        case IMPORTSSA:
        case CC608SUB:
        case UTF8SUB:
        case TX3GSUB:
        {
            return textsub_work(filter, buf_in, buf_out);
        }

        case DVBSUB:
        case PGSSUB:
        {
            return pgssub_work(filter, buf_in, buf_out);
        }

        default:
        {
            hb_error("rendersub: unsupported subtitle format %d", pv->type);
            return 1;
        }
    }
}

static void hb_rendersub_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    if (pv->sws != NULL)
    {
        sws_freeContext(pv->sws);
    }

    hb_buffer_list_close(&pv->rendered_sub_list);
    hb_blend_close(&pv->blend);

    switch (pv->type)
    {
        case VOBSUB:
        {
            vobsub_close(filter);
        } break;

        case SSASUB:
        {
            ssa_close(filter);
        } break;

        case IMPORTSRT:
        case IMPORTSSA:
        case CC608SUB:
        case UTF8SUB:
        case TX3GSUB:
        {
            textsub_close(filter);
        } break;

        case DVBSUB:
        case PGSSUB:
        {
            pgssub_close(filter);
        } break;

        default:
        {
            hb_error("rendersub: unsupported subtitle format %d", pv->type);
        } break;
    }
}

