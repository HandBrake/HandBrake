/* chroma_smooth.c

   Copyright (c) 2002 Rémi Guyomarch <rguyom at pobox.com>
   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"

#define CHROMA_SMOOTH_STRENGTH_DEFAULT 0.25
#define CHROMA_SMOOTH_SIZE_DEFAULT 7
#define CHROMA_SMOOTH_SIZE_MIN 3
#define CHROMA_SMOOTH_SIZE_MAX 63

typedef struct
{
    int        pix_fmt;   // source pixel format
    int        width;     // source video width
    double     strength;  // strength
    int        size;      // pixel context region width (must be odd)

    int        steps;
    int        amount;
    int        scalebits;
    int32_t    halfscale;
} chroma_smooth_plane_context_t;

typedef struct
{
    uint32_t * SC[CHROMA_SMOOTH_SIZE_MAX - 1];
} chroma_smooth_thread_context_t;

typedef chroma_smooth_thread_context_t chroma_smooth_thread_context3_t[3];

struct hb_filter_private_s
{
    chroma_smooth_plane_context_t     plane_ctx[3];
    chroma_smooth_thread_context3_t * thread_ctx;
    int                               threads;
};

static int chroma_smooth_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init);

static int chroma_smooth_init_thread(hb_filter_object_t *filter, int threads);

static int chroma_smooth_work(hb_filter_object_t *filter,
                              hb_buffer_t ** buf_in,
                              hb_buffer_t ** buf_out);
static int chroma_smooth_work_thread(hb_filter_object_t *filter,
                                     hb_buffer_t ** buf_in,
                                     hb_buffer_t ** buf_out, int thread);

static void chroma_smooth_close(hb_filter_object_t *filter);

static const char chroma_smooth_template[] =
    "cb-strength=^"HB_FLOAT_REG"$:cb-size=^"HB_INT_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-size=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_chroma_smooth =
{
    .id                = HB_FILTER_CHROMA_SMOOTH,
    .enforce_order     = 1,
    .name              = "Chroma Smooth",
    .settings          = NULL,
    .init              = chroma_smooth_init,
    .init_thread       = chroma_smooth_init_thread,
    .work              = chroma_smooth_work,
    .work_thread       = chroma_smooth_work_thread,
    .close             = chroma_smooth_close,
    .settings_template = chroma_smooth_template,
};

static void chroma_smooth(const uint8_t *src,
                          uint8_t *dst,
                          const int width,
                          const int height,
                          const int stride,
                          chroma_smooth_plane_context_t * ctx,
                          chroma_smooth_thread_context_t * tctx)
{
    uint32_t **SC = tctx->SC;
    uint32_t SR[CHROMA_SMOOTH_SIZE_MAX - 1],
             Tmp1,
             Tmp2;
    const uint8_t *src2 = src; // avoid gcc warning
    int32_t res;
    int x, y, z;
    int amount        = ctx->amount;
    int steps         = ctx->steps;
    int scalebits     = ctx->scalebits;
    int32_t halfscale = ctx->halfscale;

    if (!amount)
    {
        if (src != dst)
        {
            memcpy(dst, src, stride*height);
        }

        return;
    }

    for (y = 0; y < 2 * steps; y++)
    {
        memset(SC[y], 0, sizeof(SC[y][0]) * (width + 2 * steps));
    }

    for (y = -steps; y < height + steps; y++)
    {
        if (y < height)
        {
            src2 = src;
        }

        memset(SR, 0, sizeof(SR[0]) * (2 * steps - 1));

        for (x = -steps; x < width + steps; x++)
        {
            Tmp1 = x <= 0 ? src2[0] : x >= width ? src2[width - 1] : src2[x];

            for (z = 0; z < steps * 2; z += 2)
            {
                Tmp2 = SR[z + 0] + Tmp1; SR[z + 0] = Tmp1;
                Tmp1 = SR[z + 1] + Tmp2; SR[z + 1] = Tmp2;
            }

            for (z = 0; z < steps * 2; z += 2)
            {
                Tmp2 = SC[z + 0][x + steps] + Tmp1; SC[z + 0][x + steps] = Tmp1;
                Tmp1 = SC[z + 1][x + steps] + Tmp2; SC[z + 1][x + steps] = Tmp2;
            }

            if (x >= steps && y >= steps)
            {
                const uint8_t * srx = src - steps * stride + x - steps;
                uint8_t       * dsx = dst - steps * stride + x - steps;

                res = (int32_t)*srx - ((((int32_t)*srx -
                      (int32_t)((Tmp1 + halfscale) >> scalebits)) * amount) >> 16);
                *dsx = res > 240 ? 240 : res < 16 ? 16 : (uint8_t)res;
            }
        }

        if (y >= 0)
        {
            dst += stride;
            src += stride;
        }
    }
}

static int chroma_smooth_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("Chroma Smooth calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->plane_ctx[c].strength = -1;
        pv->plane_ctx[c].size     = -1;
    }

    // Read user parameters
    if (filter->settings != NULL)
    {
        hb_dict_t * dict = filter->settings;
        hb_dict_extract_double(&pv->plane_ctx[1].strength, dict, "cb-strength");
        hb_dict_extract_int(&pv->plane_ctx[1].size,        dict, "cb-size");

        hb_dict_extract_double(&pv->plane_ctx[2].strength, dict, "cr-strength");
        hb_dict_extract_int(&pv->plane_ctx[2].size,        dict, "cr-size");
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; defaults.
    for (int c = 2; c < 3; c++)
    {
        chroma_smooth_plane_context_t * prev_ctx = &pv->plane_ctx[c - 1];
        chroma_smooth_plane_context_t * ctx      = &pv->plane_ctx[c];

        if (ctx->strength == -1) ctx->strength = prev_ctx->strength;
        if (ctx->size     == -1) ctx->size     = prev_ctx->size;
    }

    for (int c = 0; c < 3; c++)
    {
        chroma_smooth_plane_context_t * ctx = &pv->plane_ctx[c];

        ctx->width = init->geometry.width;

        // Replace unset values with defaults
        if (ctx->strength == -1)
        {
            ctx->strength = CHROMA_SMOOTH_STRENGTH_DEFAULT;
        }
        if (ctx->size     == -1)
        {
            ctx->size     = CHROMA_SMOOTH_SIZE_DEFAULT;
        }

        // Sanitize
        if (ctx->strength < 0) ctx->strength = 0;
        if (ctx->strength > 3) ctx->strength = 3;
        if (ctx->size % 2 == 0) ctx->size--;
        if (ctx->size < CHROMA_SMOOTH_SIZE_MIN) ctx->size = CHROMA_SMOOTH_SIZE_MIN;
        if (ctx->size > CHROMA_SMOOTH_SIZE_MAX) ctx->size = CHROMA_SMOOTH_SIZE_MAX;

        if (c)
        {
            // Chroma
            ctx->amount    = ctx->strength * 65536.0;
            ctx->steps     = ctx->size / 2;
            ctx->scalebits = ctx->steps * 4;
            ctx->halfscale = 1 << (ctx->scalebits - 1);
        }
        else
        {
            // Luma
            ctx->amount    = 0;
            ctx->steps     = 0;
            ctx->scalebits = 0;
            ctx->halfscale = 0;
        }
    }

    if (chroma_smooth_init_thread(filter, 1) < 0)
    {
        chroma_smooth_close(filter);
        return -1;
    }

    return 0;
}

static void chroma_smooth_thread_close(hb_filter_private_t *pv)
{
    int c, z;
    for (c = 0; c < 3; c++)
    {
        chroma_smooth_plane_context_t * ctx = &pv->plane_ctx[c];
        for (int t = 0; t < pv->threads; t++)
        {
            chroma_smooth_thread_context_t * tctx = &pv->thread_ctx[t][c];

            if (c)
            {
                for (z = 0; z < 2 * ctx->steps; z++)
                {
                    free(tctx->SC[z]);
                    tctx->SC[z] = NULL;
                }
            }
        }
    }
    free(pv->thread_ctx);
}

static int chroma_smooth_init_thread(hb_filter_object_t *filter, int threads)
{
    hb_filter_private_t * pv = filter->private_data;

    chroma_smooth_thread_close(pv);
    pv->thread_ctx = calloc(threads, sizeof(chroma_smooth_thread_context3_t));
    pv->threads = threads;
    for (int c = 0; c < 3; c++)
    {
        chroma_smooth_plane_context_t * ctx = &pv->plane_ctx[c];
        int w = hb_image_width(ctx->pix_fmt, ctx->width, c);

        for (int t = 0; t < threads; t++)
        {
            chroma_smooth_thread_context_t * tctx = &pv->thread_ctx[t][c];

            if (c)
            {
                int z;
                for (z = 0; z < 2 * ctx->steps; z++)
                {
                    tctx->SC[z] = malloc(sizeof(*(tctx->SC[z])) *
                                         (w + 2 * ctx->steps));
                    if (tctx->SC[z] == NULL)
                    {
                        hb_error("Chroma Smooth calloc failed");
                        chroma_smooth_close(filter);
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
}

static void chroma_smooth_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    chroma_smooth_thread_close(pv);
    free(pv);
    filter->private_data = NULL;
}

static int chroma_smooth_work_thread(hb_filter_object_t *filter,
                                     hb_buffer_t ** buf_in,
                                     hb_buffer_t ** buf_out, int thread)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    out = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);

    int c;
    for (c = 0; c < 3; c++)
    {
        chroma_smooth_plane_context_t  * ctx  = &pv->plane_ctx[c];
        chroma_smooth_thread_context_t * tctx = &pv->thread_ctx[thread][c];

        chroma_smooth(in->plane[c].data,
                      out->plane[c].data,
                      in->plane[c].width,
                      in->plane[c].height,
                      in->plane[c].stride,
                      ctx, tctx);
    }

    out->s = in->s;
    *buf_out = out;

    return HB_FILTER_OK;
}

static int chroma_smooth_work(hb_filter_object_t *filter,
                              hb_buffer_t ** buf_in,
                              hb_buffer_t ** buf_out)
{
    return chroma_smooth_work_thread(filter, buf_in, buf_out, 0);
}
