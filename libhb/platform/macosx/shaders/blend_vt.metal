/* blend_vt.metal

   Copyright (c) 2003-2025 HandBrake Team

   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <metal_stdlib>
#include <metal_integer>
#include <metal_texture>

/*
 * Parameters
 */

constant uint16_t plane      [[function_constant(0)]];
constant uint16_t channels   [[function_constant(1)]];
constant uint16_t subw       [[function_constant(2)]];
constant uint16_t subh       [[function_constant(3)]];
constant uint16_t osubw      [[function_constant(4)]];
constant uint16_t osubh      [[function_constant(5)]];
constant uint16_t shift      [[function_constant(6)]];
constant uint32_t maxv       [[function_constant(7)]];
constant bool subsample      [[function_constant(8)]];

struct params {
    uint16_t x;
    uint16_t y;
    uint16_t xc;
    uint16_t yc;
    uint16_t width;
    uint16_t height;
};

using namespace metal;

/*
 * Blend helpers
 */

template <typename T>
T blend_pixel(T y_out, T y_in, T a_in)
{
    return ((uint32_t)y_out * (maxv - a_in) + (uint32_t)y_in * a_in) / maxv;
}

template <typename T>
T pos_dst_y(T pos, uint16_t x, uint16_t y)
{
    return T(pos.x + x, pos.y + y);
}

template <typename T>
T pos_dst_u(T pos, uint16_t x, uint16_t y)
{
    return T((pos.x + (x >> subw)) * channels,
              pos.y + (y >> subh));
}

template <typename T>
T pos_dst_v(T pos, uint16_t x, uint16_t y)
{
    return T((pos.x + (x >> subw)) * channels + 1,
              pos.y + (y >> subh));
}

template <typename T>
T pos_uv_subsample(T pos, uint16_t x, uint16_t y)
{
    uint16_t uvsubw = subw - osubw;
    uint16_t uvsubh = subh - osubh;

    return T((pos.x << uvsubw) + (x >> osubw),
             (pos.y << uvsubh) + (y >> osubh));
}

template <typename T>
T pos_a(T pos, uint16_t x, uint16_t y)
{
    return ushort2((pos.x << subw) + x, (pos.y << subh) + y);
}

template <typename T>
T blend_pixel_y(
    texture2d<T, access::read_write> dst,
    texture2d<T, access::read> overlay_y,
    texture2d<T, access::read> overlay_a,
    constant params& p,
    ushort2 pos)
{
    ushort2 pos_ya = pos_dst_y(pos, p.x, p.y);

    T y_in  = overlay_y.read(pos_ya).x << shift;
    T a_in  = overlay_a.read(pos_ya).x << shift;
    T y_out = dst.read(pos_ya).x;

    return blend_pixel(y_out, y_in, a_in);
}

template <typename T, typename V>
V blend_pixel_uv(
    texture2d<T, access::read_write> dst,
    texture2d<T, access::read> overlay_u,
    texture2d<T, access::read> overlay_v,
    texture2d<T, access::read> overlay_a,
    constant params& p,
    ushort2 pos)
{
    ushort2 pos_uv = ushort2(pos.x + (p.x >> osubw), pos.y + (p.y >> osubh));
    T u_in = overlay_u.read(pos_uv).x << shift;
    T v_in = overlay_v.read(pos_uv).x << shift;

    T u_out = dst.read(pos_dst_u(pos, p.x, p.y)).x;
    T v_out = dst.read(pos_dst_v(pos, p.x, p.y)).x;
    T a_in = overlay_a.read(pos_a(pos, p.x, p.y)).x << shift;

    return V(blend_pixel(u_out, u_in, a_in),
             blend_pixel(v_out, v_in, a_in));
}

template <typename T, typename V>
V blend_subsample_pixel_uv(
    texture2d<T, access::read_write> dst,
    texture2d<T, access::read> overlay_u,
    texture2d<T, access::read> overlay_v,
    texture2d<T, access::read> overlay_a,
    constant uint *chroma_coeffs,
    constant params& p,
    ushort2 pos)
{
    T u_out = dst.read(pos_dst_u(pos, p.xc, p.yc)).x;
    T v_out = dst.read(pos_dst_v(pos, p.xc, p.yc)).x;

    // Perform chromaloc-aware subsampling and blending
    uint32_t accu_a = 0, accu_b = 0, accu_c = 0;
    ushort2 pos_uv = pos_uv_subsample(pos, p.xc, p.yc);

    for (uint16_t yz = 0; yz < 1 << subh; yz++) {
        for (uint16_t xz = 0; xz < 1 << subw; xz++) {
            // Weight of the current chroma sample
            uint32_t coeff = *(chroma_coeffs + xz) * *(chroma_coeffs + 4 + yz);
            uint32_t res_u = u_out;
            uint32_t res_v = v_out;

            // Chroma sampled area overlap with bitmap
            if ((pos.x > 0 || p.xc == p.x) && pos.x < p.width &&
                (pos.y > 0 || p.yc == p.y) && pos.y < p.height) {
                ushort2 offset = ushort2(xz, yz);
                T a_in = overlay_a.read(pos_a(pos, p.xc, p.yc) + offset).x << shift;
                T u_in = overlay_u.read(pos_uv + offset).x << shift;
                T v_in = overlay_v.read(pos_uv + offset).x << shift;

                res_u *= (maxv - a_in);
                res_u  = (res_u + ((uint32_t)u_in) * a_in + (maxv >> 1)) / maxv;

                res_v *= (maxv - a_in);
                res_v  = (res_v + ((uint32_t)v_in) * a_in + (maxv >> 1)) / maxv;
            }

            // Accumulate
            accu_a += coeff * res_u;
            accu_b += coeff * res_v;
            accu_c += coeff;
        }
    }

    if (accu_c) {
        return V((accu_a + (accu_c >> 1)) / accu_c,
                 (accu_b + (accu_c >> 1)) / accu_c);
    }
    else {
        return V(u_out, v_out);
    }
}

/*
 * Kernel dispatch
 */

kernel void blend(
    texture2d<ushort, access::read_write> dst [[texture(0)]],
    texture2d<ushort, access::read> overlay_y [[texture(1)]],
    texture2d<ushort, access::read> overlay_u [[texture(2)]],
    texture2d<ushort, access::read> overlay_v [[texture(3)]],
    texture2d<ushort, access::read> overlay_a [[texture(4)]],
    constant uint *chroma_coeffs [[buffer(0)]],
    constant params& p           [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (plane == 0) {
        ushort value = blend_pixel_y(dst, overlay_y, overlay_a,
                                     p, pos);
        dst.write(value, pos_dst_y(pos, p.x, p.y));
    }
    else {
        ushort2 value;
        if (subsample) {
            value = blend_subsample_pixel_uv<ushort, ushort2>(dst, overlay_u,
                                                              overlay_v, overlay_a,
                                                              chroma_coeffs, p, pos);
        }
        else {
            value = blend_pixel_uv<ushort, ushort2>(dst, overlay_u,
                                                    overlay_v, overlay_a,
                                                    p, pos);
        }
        dst.write(value.x, pos_dst_u(pos, p.x, p.y));
        dst.write(value.y, pos_dst_v(pos, p.x, p.y));
    }
}
