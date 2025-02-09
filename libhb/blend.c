/* blend.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "libavutil/bswap.h"

struct hb_blend_private_s
{
    int hshift;
    int wshift;
    int depth;

    unsigned chroma_coeffs[2][4];

    void (*blend)(const struct hb_blend_private_s *pv, hb_buffer_t *dst,
                  const hb_buffer_t *src, const int shift);
};

static int hb_blend_init(hb_blend_object_t *object,
                         int in_width,
                         int in_height,
                         int in_pix_fmt,
                         int in_chroma_location,
                         int in_color_range,
                         int overlay_pix_fmt);

static hb_buffer_t * hb_blend_work(hb_blend_object_t *object,
                                   hb_buffer_t *in,
                                   hb_buffer_list_t *overlays,
                                   int changed);

static void hb_blend_close(hb_blend_object_t *object);

hb_blend_object_t hb_blend =
{
    .name  = "Blend",
    .init  = hb_blend_init,
    .work  = hb_blend_work,
    .close = hb_blend_close,
};

static void blend_subsample_8on1x(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int x0, y0, x0c, y0c;
    int ox, oy;
    int width, height;
    uint8_t *y_in, *u_in, *v_in, *a_in;
    uint16_t *y_out, *u_out, *v_out;
    const unsigned max_val = (256 << shift) - 1;

    const int left = x0 = src->f.x;
    const int top  = y0 = src->f.y;

    // Coordinates of the first chroma sample affected by the overlay
    x0c = x0 & ~((1 << pv->wshift) - 1);
    y0c = y0 & ~((1 << pv->hshift) - 1);

    width  = (src->f.width  - x0 <= dst->f.width - left) ? src->f.width  : (dst->f.width - left + x0);
    height = (src->f.height - y0 <= dst->f.height - top) ? src->f.height : (dst->f.height - top + y0);

    // This is setting the pointer outside of the array range if y0c < y0
    oy = y0c - y0;

    unsigned is_chroma_line, res_u, res_v, alpha;
    unsigned accu_a, accu_b, accu_c, coeff;
    for (int yy = y0c; oy < height; oy = ++yy - y0)
    {
        y_out = (uint16_t*)(dst->plane[0].data + yy * dst->plane[0].stride);
        u_out = (uint16_t*)(dst->plane[1].data + (yy >> pv->hshift) * dst->plane[1].stride);
        v_out = (uint16_t*)(dst->plane[2].data + (yy >> pv->hshift) * dst->plane[2].stride);

        y_in = src->plane[0].data + oy * src->plane[0].stride;
        u_in = src->plane[1].data + oy * src->plane[1].stride;
        v_in = src->plane[2].data + oy * src->plane[2].stride;
        a_in = src->plane[3].data + oy * src->plane[3].stride;

        ox = x0c - x0;
        is_chroma_line = yy == (yy & ~((1 << pv->hshift) - 1));
        for (int xx = x0c; ox < width; ox = ++xx - x0)
        {
            if (ox >= 0 && oy >= 0)
            {
                alpha = a_in[ox] << shift;
                y_out[xx] = ((uint32_t)y_out[xx] * (max_val - alpha) + ((uint32_t)y_in[ox] << shift) * alpha + (max_val >> 1)) / max_val;
            }

            if (is_chroma_line && xx == (xx & ~((1 << pv->wshift) - 1)))
            {
                // Perform chromaloc-aware subsampling and blending
                accu_a = accu_b = accu_c = 0;
                for (int yz = 0, oyz = oy; yz < (1 << pv->hshift) && oy + yz < height; yz++, oyz++)
                {
                    for (int xz = 0, oxz = ox; xz < (1 << pv->wshift) && ox + xz < width; xz++, oxz++)
                    {
                        // Weight of the current chroma sample
                        coeff = pv->chroma_coeffs[0][xz] * pv->chroma_coeffs[1][yz];
                        res_u = u_out[xx >> pv->wshift];
                        res_v = v_out[xx >> pv->wshift];

                        // Chroma sampled area overlap with bitmap
                        if (oxz >= 0 && oyz >= 0 && ox + xz < width && oy + yz < height)
                        {
                            alpha = (uint32_t)a_in[oxz + yz * src->plane[3].stride] << shift;
                            res_u *= (max_val - alpha);
                            res_u = (res_u + ((uint32_t)(u_in + yz * src->plane[1].stride)[oxz] << shift) * alpha + (max_val>>1)) / max_val;

                            res_v *= (max_val - alpha);
                            res_v = (res_v + ((uint32_t)(v_in + yz * src->plane[2].stride)[oxz] << shift) * alpha + (max_val>>1)) / max_val;
                        }

                        // Accumulate
                        accu_a += coeff * res_u;
                        accu_b += coeff * res_v;
                        accu_c += coeff;
                    }
                }
                if (accu_c)
                {
                    u_out[xx >> pv->wshift] = (accu_a + (accu_c >> 1)) / accu_c;
                    v_out[xx >> pv->wshift] = (accu_b + (accu_c >> 1)) / accu_c;
                }
            }
        }
    }
}

static void blend_subsample_8onbi1x(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int x0, y0, x0c, y0c;
    int ox, oy;
    int width, height;
    uint8_t *y_in, *u_in, *v_in, *a_in;
    uint16_t *y_out, *u_out, *v_out;
    const unsigned max_val = (256 << shift) - 1;

    const int left = x0 = src->f.x;
    const int top  = y0 = src->f.y;

    // Coordinates of the first chroma sample affected by the overlay
    x0c = x0 & ~((1 << pv->wshift) - 1);
    y0c = y0 & ~((1 << pv->hshift) - 1);

    width  = (src->f.width  - x0 <= dst->f.width - left) ? src->f.width  : (dst->f.width - left + x0);
    height = (src->f.height - y0 <= dst->f.height - top) ? src->f.height : (dst->f.height - top + y0);

    // This is setting the pointer outside of the array range if y0c < y0
    oy = y0c - y0;

    unsigned is_chroma_line, res_u, res_v, alpha;
    unsigned accu_a, accu_b, accu_c, coeff;
    for (int yy = y0c; oy < height; oy = ++yy - y0)
    {
        y_out = (uint16_t*)(dst->plane[0].data + yy * dst->plane[0].stride);
        u_out = (uint16_t*)(dst->plane[1].data + (yy >> pv->hshift) * dst->plane[1].stride);
        v_out = u_out;

        y_in = src->plane[0].data + oy * src->plane[0].stride;
        u_in = src->plane[1].data + oy * src->plane[1].stride;
        v_in = src->plane[2].data + oy * src->plane[2].stride;
        a_in = src->plane[3].data + oy * src->plane[3].stride;

        ox = x0c - x0;
        is_chroma_line = yy == (yy & ~((1 << pv->hshift) - 1));
        for (int xx = x0c; ox < width; ox = ++xx - x0)
        {
            if (ox >= 0 && oy >= 0)
            {
                alpha = a_in[ox] << shift;
                y_out[xx] = ((uint32_t)y_out[xx] * (max_val - alpha) + av_bswap16(y_in[ox]) * alpha + (max_val >> 1)) / max_val;
            }

            if (is_chroma_line && xx == (xx & ~((1 << pv->wshift) - 1)))
            {
                // Perform chromaloc-aware subsampling and blending
                accu_a = accu_b = accu_c = 0;
                for (int yz = 0, oyz = oy; yz < (1 << pv->hshift) && oy + yz < height; yz++, oyz++)
                {
                    for (int xz = 0, oxz = ox; xz < (1 << pv->wshift) && ox + xz < width; xz++, oxz++)
                    {
                        // Weight of the current chroma sample
                        coeff = pv->chroma_coeffs[0][xz] * pv->chroma_coeffs[1][yz];
                        res_u = u_out[(xx >> pv->wshift) * 2 + 0];
                        res_v = v_out[(xx >> pv->wshift) * 2 + 1];

                        // Chroma sampled area overlap with bitmap
                        if (oxz >= 0 && oyz >= 0 && ox + xz < width && oy + yz < height)
                        {
                            alpha = a_in[oxz + yz*src->plane[3].stride] << shift;
                            res_u *= (max_val - alpha);
                            res_u = (res_u + av_bswap16((u_in + yz * src->plane[1].stride)[oxz]) * alpha + (max_val>>1)) / max_val;

                            res_v *= (max_val - alpha);
                            res_v = (res_v + av_bswap16((v_in + yz * src->plane[2].stride)[oxz]) * alpha + (max_val>>1)) / max_val;
                        }

                        // Accumulate
                        accu_a += coeff * res_u;
                        accu_b += coeff * res_v;
                        accu_c += coeff;
                    }
                }
                if (accu_c)
                {
                    u_out[(xx >> pv->wshift) * 2 + 0] = (accu_a + (accu_c >> 1)) / accu_c;
                    v_out[(xx >> pv->wshift) * 2 + 1] = (accu_b + (accu_c >> 1)) / accu_c;
                }
            }
        }
    }
}

static void blend_subsample_8on8(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int x0, y0, x0c, y0c;
    int ox, oy;
    int width, height;
    uint8_t *y_in, *y_out;
    uint8_t *u_in, *u_out;
    uint8_t *v_in, *v_out;
    uint8_t *a_in;

    const int left = x0 = src->f.x;
    const int top  = y0 = src->f.y;

    // Coordinates of the first chroma sample affected by the overlay
    x0c = x0 & ~((1 << pv->wshift) - 1);
    y0c = y0 & ~((1 << pv->hshift) - 1);

    width  = (src->f.width  - x0 <= dst->f.width - left) ? src->f.width  : (dst->f.width - left + x0);
    height = (src->f.height - y0 <= dst->f.height - top) ? src->f.height : (dst->f.height - top + y0);

    // This is setting the pointer outside of the array range if y0c < y0
    oy = y0c - y0;

    unsigned is_chroma_line, res_u, res_v, alpha;
    unsigned accu_a, accu_b, accu_c, coeff;
    for (int yy = y0c; oy < height; oy = ++yy - y0)
    {
        y_out = dst->plane[0].data + yy * dst->plane[0].stride;
        u_out = dst->plane[1].data + (yy >> pv->hshift) * dst->plane[1].stride;
        v_out = dst->plane[2].data + (yy >> pv->hshift) * dst->plane[2].stride;

        y_in = src->plane[0].data + oy * src->plane[0].stride;
        u_in = src->plane[1].data + oy * src->plane[1].stride;
        v_in = src->plane[2].data + oy * src->plane[2].stride;
        a_in = src->plane[3].data + oy * src->plane[3].stride;

        ox = x0c - x0;
        is_chroma_line = yy == (yy & ~((1 << pv->hshift) - 1));
        for (int xx = x0c; ox < width; ox = ++xx - x0)
        {
            if (ox >= 0 && oy >= 0)
            {
                y_out[xx] = (y_out[xx] * (255 - a_in[ox]) + y_in[ox] * a_in[ox] + 127) / 255;
            }

            if (is_chroma_line && xx == (xx & ~((1 << pv->wshift) - 1)))
            {
                // Perform chromaloc-aware subsampling and blending
                accu_a = accu_b = accu_c = 0;
                for (int yz = 0, oyz = oy; yz < (1 << pv->hshift) && oy + yz < height; yz++, oyz++)
                {
                    for (int xz = 0, oxz = ox; xz < (1 << pv->wshift) && ox + xz < width; xz++, oxz++)
                    {
                        // Weight of the current chroma sample
                        coeff = pv->chroma_coeffs[0][xz] * pv->chroma_coeffs[1][yz];
                        res_u = u_out[xx >> pv->wshift];
                        res_v = v_out[xx >> pv->wshift];

                        // Chroma sampled area overlap with bitmap
                        if (oxz >= 0 && oyz >= 0 && ox + xz < width && oy + yz < height)
                        {
                            alpha = a_in[oxz + yz*src->plane[3].stride];
                            res_u *= (255 - alpha);
                            res_u = (res_u + (u_in + yz * src->plane[1].stride)[oxz] * alpha + 127) / 255;

                            res_v *= (255 - alpha);
                            res_v = (res_v + (v_in + yz * src->plane[2].stride)[oxz] * alpha + 127) / 255;
                        }

                        // Accumulate
                        accu_a += coeff * res_u;
                        accu_b += coeff * res_v;
                        accu_c += coeff;
                    }
                }
                if (accu_c)
                {
                    u_out[xx >> pv->wshift] = (accu_a + (accu_c >> 1)) / accu_c;
                    v_out[xx >> pv->wshift] = (accu_b + (accu_c >> 1)) / accu_c;
                }
            }
        }
    }
}

static void blend_subsample_8onbi8(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int x0, y0, x0c, y0c;
    int ox, oy;
    int width, height;
    uint8_t *y_in, *y_out;
    uint8_t *u_in, *u_out;
    uint8_t *v_in, *v_out;
    uint8_t *a_in;

    const int left = x0 = src->f.x;
    const int top = y0 = src->f.y;

    // Coordinates of the first chroma sample affected by the overlay
    x0c = x0 & ~((1 << pv->wshift) - 1);
    y0c = y0 & ~((1 << pv->hshift) - 1);

    width  = (src->f.width  - x0 <= dst->f.width - left) ? src->f.width  : (dst->f.width - left + x0);
    height = (src->f.height - y0 <= dst->f.height - top) ? src->f.height : (dst->f.height - top + y0);

    // This is setting the pointer outside of the array range if y0c < y0
    oy = y0c - y0;

    unsigned is_chroma_line, res_u, res_v, alpha;
    unsigned accu_a, accu_b, accu_c, coeff;
    for (int yy = y0c; oy < height; oy = ++yy - y0)
    {
        y_out = dst->plane[0].data + yy * dst->plane[0].stride;
        u_out = dst->plane[1].data + (yy >> pv->hshift) * dst->plane[1].stride;
        v_out = u_out;

        y_in = src->plane[0].data + oy * src->plane[0].stride;
        u_in = src->plane[1].data + oy * src->plane[1].stride;
        v_in = src->plane[2].data + oy * src->plane[2].stride;
        a_in = src->plane[3].data + oy * src->plane[3].stride;

        ox = x0c - x0;
        is_chroma_line = yy == (yy & ~((1 << pv->hshift) - 1));
        for (int xx = x0c; ox < width; ox = ++xx - x0)
        {
            if (ox >= 0 && oy >= 0)
            {
                y_out[xx] = (y_out[xx] * (255 - a_in[ox]) + y_in[ox] * a_in[ox] + 127) / 255;
            }

            if (is_chroma_line && xx == (xx & ~((1 << pv->wshift) - 1)))
            {
                // Perform chromaloc-aware subsampling and blending
                accu_a = accu_b = accu_c = 0;
                for (int yz = 0, oyz = oy; yz < (1 << pv->hshift); yz++, oyz++)
                {
                    for (int xz = 0, oxz = ox; xz < (1 << pv->wshift); xz++, oxz++)
                    {
                        // Weight of the current chroma sample
                        coeff = pv->chroma_coeffs[0][xz] * pv->chroma_coeffs[1][yz];
                        res_u = u_out[(xx >> pv->wshift) * 2 + 0];
                        res_v = v_out[(xx >> pv->wshift) * 2 + 1];

                        // Chroma sampled area overlap with bitmap
                        if (oxz >= 0 && oyz >= 0 && ox + xz < width && oy + yz < height)
                        {
                            alpha = a_in[oxz + yz*src->plane[3].stride];
                            res_u *= (255 - alpha);
                            res_u = (res_u + (u_in + yz * src->plane[1].stride)[oxz] * alpha + 127) / 255;

                            res_v *= (255 - alpha);
                            res_v = (res_v + (v_in + yz * src->plane[2].stride)[oxz] * alpha + 127) / 255;
                        }

                        // Accumulate
                        accu_a += coeff*res_u;
                        accu_b += coeff*res_v;
                        accu_c += coeff;
                    }
                }
                if (accu_c)
                {
                    u_out[(xx >> pv->wshift) * 2 + 0] = (accu_a + (accu_c >> 1)) / accu_c;
                    v_out[(xx >> pv->wshift) * 2 + 1] = (accu_b + (accu_c >> 1)) / accu_c;
                }
            }
        }
    }
}

// blends src YUVA4**P buffer into dst
static void blend8on8(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int ww, hh;
    int x0, y0;
    uint8_t *y_in, *y_out;
    uint8_t *u_in, *u_out;
    uint8_t *v_in, *v_out;
    uint8_t *a_in, alpha;

    const int left = src->f.x;
    const int top  = src->f.y;

    x0 = y0 = 0;
    if (left < 0)
    {
        x0 = -left;
    }
    if (top < 0)
    {
        y0 = -top;
    }

    ww = src->f.width;
    if (src->f.width - x0 > dst->f.width - left)
    {
        ww = dst->f.width - left + x0;
    }
    hh = src->f.height;
    if (src->f.height - y0 > dst->f.height - top)
    {
        hh = dst->f.height - top + y0;
    }
    // Blend luma
    for (int yy = y0; yy < hh; yy++)
    {
        y_in  = src->plane[0].data + yy * src->plane[0].stride;
        y_out = dst->plane[0].data + (yy + top) * dst->plane[0].stride;
        a_in = src->plane[3].data + yy * src->plane[3].stride;
        for (int xx = x0; xx < ww; xx++)
        {
            alpha = a_in[xx];
            // Merge the luminance and alpha with the picture
            y_out[left + xx] =
                ((uint16_t)y_out[left + xx] * (255 - alpha) +
                     (uint16_t)y_in[xx] * alpha) / 255;
        }
    }

    // Blend U & V
    // Assumes source and dest are the same PIX_FMT
    int hshift = 0;
    int wshift = 0;
    if (dst->plane[1].height < dst->plane[0].height)
    {
        hshift = 1;
    }
    if (dst->plane[1].width < dst->plane[0].width)
    {
        wshift = 1;
    }

    for (int yy = y0 >> hshift; yy < hh >> hshift; yy++)
    {
        u_in = src->plane[1].data + yy * src->plane[1].stride;
        u_out = dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride;
        v_in = src->plane[2].data + yy * src->plane[2].stride;
        v_out = dst->plane[2].data + (yy + (top >> hshift)) * dst->plane[2].stride;
        a_in = src->plane[3].data + (yy << hshift) * src->plane[3].stride;

        for (int xx = x0 >> wshift; xx < ww >> wshift; xx++)
        {
            alpha = a_in[xx << wshift];

            // Blend U and alpha
            u_out[(left >> wshift) + xx] =
                ((uint16_t)u_out[(left >> wshift) + xx] * (255 - alpha) +
                 (uint16_t)u_in[xx] * alpha) / 255;

            // Blend V and alpha
            v_out[(left >> wshift) + xx] =
                ((uint16_t)v_out[(left >> wshift) + xx] * (255 - alpha) +
                 (uint16_t)v_in[xx] * alpha) / 255;
        }
    }
}

static void blend8on1x(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int ww, hh;
    int x0, y0;
    int max;

    uint8_t *y_in;
    uint8_t *u_in;
    uint8_t *v_in;
    uint8_t *a_in;

    uint16_t *y_out;
    uint16_t *u_out;
    uint16_t *v_out;
    uint16_t alpha;

    const int left = src->f.x;
    const int top  = src->f.y;

    x0 = y0 = 0;
    if (left < 0)
    {
        x0 = -left;
    }
    if (top < 0)
    {
        y0 = -top;
    }

    ww = src->f.width;
    if (src->f.width - x0 > dst->f.width - left)
    {
        ww = dst->f.width - left + x0;
    }
    hh = src->f.height;
    if (src->f.height - y0 > dst->f.height - top)
    {
        hh = dst->f.height - top + y0;
    }

    max = (256 << shift) -1;

    // Blend luma
    for (int yy = y0; yy < hh; yy++)
    {
        y_in  = src->plane[0].data + yy * src->plane[0].stride;
        y_out = (uint16_t*)(dst->plane[0].data + (yy + top) * dst->plane[0].stride);
        a_in = src->plane[3].data + yy * src->plane[3].stride;
        for (int xx = x0; xx < ww; xx++)
        {
            alpha = a_in[xx] << shift;
            // Merge the luminance and alpha with the picture
            y_out[left + xx] =
                ((uint32_t)y_out[left + xx] * (max - alpha) +
                    ((uint32_t)y_in[xx] << shift) * alpha) / max;
        }
    }

    // Blend U & V
    int hshift = 0;
    int wshift = 0;
    if (dst->plane[1].height < dst->plane[0].height)
    {
        hshift = 1;
    }
    if (dst->plane[1].width < dst->plane[0].width)
    {
        wshift = 1;
    }

    for (int yy = y0 >> hshift; yy < hh >> hshift; yy++)
    {
        u_in = src->plane[1].data + yy * src->plane[1].stride;
        u_out = (uint16_t*)(dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride);
        v_in = src->plane[2].data + yy * src->plane[2].stride;
        v_out = (uint16_t*)(dst->plane[2].data + (yy + (top >> hshift)) * dst->plane[2].stride);
        a_in = src->plane[3].data + (yy << hshift) * src->plane[3].stride;

        for (int xx = x0 >> wshift; xx < ww >> wshift; xx++)
        {
            alpha = a_in[xx << wshift] << shift;

            // Blend U and alpha
            u_out[(left >> wshift) + xx] =
                ((uint32_t)u_out[(left >> wshift) + xx] * (max - alpha) +
                 ((uint32_t)u_in[xx] << shift) * alpha) / max;

            // Blend V and alpha
            v_out[(left >> wshift) + xx] =
                ((uint32_t)v_out[(left >> wshift) + xx] * (max - alpha) +
                 ((uint32_t)v_in[xx] << shift) * alpha) / max;
        }
    }
}

static void blend8onbi8(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int ww, hh;
    int x0, y0;
    uint8_t *y_in, *y_out;
    uint8_t *u_in, *u_out;
    uint8_t *v_in, *v_out;
    uint8_t *a_in, alpha;

    const int left = src->f.x;
    const int top  = src->f.y;

    x0 = y0 = 0;
    if (left < 0)
    {
        x0 = -left;
    }
    if (top < 0)
    {
        y0 = -top;
    }

    ww = src->f.width;
    if (src->f.width - x0 > dst->f.width - left)
    {
        ww = dst->f.width - left + x0;
    }
    hh = src->f.height;
    if (src->f.height - y0 > dst->f.height - top)
    {
        hh = dst->f.height - top + y0;
    }

    // Blend luma
    for (int yy = y0; yy < hh; yy++)
    {
        y_in  = src->plane[0].data + yy * src->plane[0].stride;
        y_out = dst->plane[0].data + (yy + top) * dst->plane[0].stride;
        a_in = src->plane[3].data + yy * src->plane[3].stride;
        for (int xx = x0; xx < ww; xx++)
        {
            alpha = a_in[xx];
            // Merge the luminance and alpha with the picture
            y_out[left + xx] =
                ((uint16_t)y_out[left + xx] * (255 - alpha) +
                    (uint16_t)y_in[xx] * alpha) / 255;
        }
    }

    // Blend U & V
    // Assumes source and dest are the same PIX_FMT
    int hshift = 0;
    int wshift = 0;
    if (dst->plane[1].height < dst->plane[0].height)
    {
        hshift = 1;
    }
    if (dst->plane[1].width < dst->plane[0].width)
    {
        wshift = 1;
    }

    for (int yy = y0 >> hshift; yy < hh >> hshift; yy++)
    {
        u_in = src->plane[1].data + yy * src->plane[1].stride;
        u_out = dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride;
        v_in = src->plane[2].data + yy * src->plane[2].stride;
        v_out = dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride;
        a_in = src->plane[3].data + (yy << hshift) * src->plane[3].stride;

        for (int xx = x0 >> wshift; xx < ww >> wshift; xx++)
        {
            alpha = a_in[xx << wshift];

            // Blend U and alpha
            u_out[((left >> wshift) + xx) * 2] =
                ((uint16_t)u_out[((left >> wshift) + xx) * 2] * (255 - alpha) +
                 (uint16_t)u_in[xx] * alpha) / 255;

            // Blend V and alpha
            v_out[((left >> wshift) + xx) * 2 +1] =
                ((uint16_t)v_out[((left >> wshift) + xx) * 2 + 1] * (255 - alpha) +
                 (uint16_t)v_in[xx] * alpha) / 255;
        }
    }
}

static void blend8onbi1x(const hb_blend_private_t *pv, hb_buffer_t *dst, const hb_buffer_t *src, const int shift)
{
    int ww, hh;
    int x0, y0;
    int max;

    uint8_t *y_in;
    uint8_t *u_in;
    uint8_t *v_in;
    uint8_t *a_in;

    uint16_t *y_out;
    uint16_t *u_out;
    uint16_t *v_out;
    uint16_t alpha;

    const int left = src->f.x;
    const int top  = src->f.y;

    x0 = y0 = 0;
    if (left < 0)
    {
        x0 = -left;
    }
    if (top < 0)
    {
        y0 = -top;
    }

    ww = src->f.width;
    if (src->f.width - x0 > dst->f.width - left)
    {
        ww = dst->f.width - left + x0;
    }
    hh = src->f.height;
    if (src->f.height - y0 > dst->f.height - top)
    {
        hh = dst->f.height - top + y0;
    }

    max = (256 << shift) -1;

    // Blend luma
    for (int yy = y0; yy < hh; yy++)
    {
        y_in  = src->plane[0].data + yy * src->plane[0].stride;
        y_out = (uint16_t*)(dst->plane[0].data + (yy + top) * dst->plane[0].stride);
        a_in  = src->plane[3].data + yy * src->plane[3].stride;
        for (int xx = x0; xx < ww; xx++)
        {
            alpha = a_in[xx] << shift;
            // Merge the luminance and alpha with the picture
            y_out[left + xx] =
                ((uint32_t)y_out[left + xx] * (max - alpha) +
                    ((uint32_t)av_bswap16(y_in[xx])) * alpha) / max;
        }
    }

    // Blend U & V
    int hshift = 0;
    int wshift = 0;
    if (dst->plane[1].height < dst->plane[0].height)
    {
        hshift = 1;
    }
    if (dst->plane[1].width < dst->plane[0].width)
    {
        wshift = 1;
    }

    for (int yy = y0 >> hshift; yy < hh >> hshift; yy++)
    {
        u_in = src->plane[1].data + yy * src->plane[1].stride;
        u_out = (uint16_t *)(dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride);
        v_in = src->plane[2].data + yy * src->plane[2].stride;
        v_out = (uint16_t *)(dst->plane[1].data + (yy + (top >> hshift)) * dst->plane[1].stride);
        a_in = src->plane[3].data + (yy << hshift) * src->plane[3].stride;

        for (int xx = x0 >> wshift; xx < ww >> wshift; xx++)
        {
            alpha = a_in[xx << wshift] << shift;

            // Blend averge U and alpha
            u_out[((left >> wshift) + xx) * 2] =
                ((uint32_t)u_out[((left >> wshift) + xx) * 2] * (max - alpha) +
                 ((uint32_t)av_bswap16(u_in[xx])) * alpha) / max;

            // Blend V and alpha
            v_out[((left >> wshift) + xx) * 2 + 1] =
                ((uint32_t)v_out[((left >> wshift) + xx) * 2 + 1] * (max - alpha) +
                 ((uint32_t)av_bswap16(v_in[xx])) * alpha) / max;
        }
    }
}

static int hb_blend_init(hb_blend_object_t *object,
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
        hb_error("blend: calloc failed");
        return -1;
    }
    hb_blend_private_t *pv = object->private_data;

    const AVPixFmtDescriptor *in_desc = av_pix_fmt_desc_get(in_pix_fmt);
    const AVPixFmtDescriptor *overlay_desc = av_pix_fmt_desc_get(overlay_pix_fmt);

    pv->depth  = in_desc->comp[0].depth;
    pv->wshift = in_desc->log2_chroma_w;
    pv->hshift = in_desc->log2_chroma_h;

    hb_compute_chroma_smoothing_coefficient(pv->chroma_coeffs,
                                            in_pix_fmt,
                                            in_chroma_location);

    const int needs_subsample = in_desc->log2_chroma_w != overlay_desc->log2_chroma_w ||
                                in_desc->log2_chroma_h != overlay_desc->log2_chroma_h;
    const int planes_count = av_pix_fmt_count_planes(in_pix_fmt);

    switch (pv->depth)
    {
        case 8:
            switch (planes_count)
            {
                case 2:
                    pv->blend = needs_subsample ? blend_subsample_8onbi8 : blend8onbi8;
                    break;
                default:
                    pv->blend = needs_subsample ? blend_subsample_8on8 : blend8on8;
                    break;
            }
            break;
        default:
            switch (planes_count)
            {
                case 2:
                    pv->blend = needs_subsample ? blend_subsample_8onbi1x : blend8onbi1x;
                    break;
                default:
                    pv->blend = needs_subsample ? blend_subsample_8on1x : blend8on1x;
                    break;
            }
    }


    return 0;
}

static hb_buffer_t * hb_blend_work(hb_blend_object_t *object,
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

    if (hb_buffer_is_writable(in) == 0)
    {
        out = hb_buffer_dup(in);
        hb_buffer_close(&in);
    }

    for (hb_buffer_t *overlay = hb_buffer_list_head(overlays); overlay; overlay = overlay->next)
    {
        pv->blend(pv, out, overlay, pv->depth - 8);
    }

    return out;
}

static void hb_blend_close(hb_blend_object_t *object)
{
    hb_blend_private_t *pv = object->private_data;

    if (pv == NULL)
    {
        return;
    }

    free(pv);
}
