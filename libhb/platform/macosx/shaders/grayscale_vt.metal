/* grayscale.metal

   Copyright (c) 2003-2025 HandBrake Team
   Copyright (c) 2021 Paul B Mahol

   port of vf_monochrome FFmpeg.

   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */


#include <metal_stdlib>
#include <metal_integer>
#include <metal_texture>

using namespace metal;

/*
 * Parameters
 */

constant uint plane    [[function_constant(0)]];
constant bool biplanar [[function_constant(1)]];
constant uint subw [[function_constant(2)]];
constant uint subh [[function_constant(3)]];
constant uint cb   [[function_constant(4)]];
constant uint cr   [[function_constant(5)]];
constant uint size [[function_constant(6)]];
constant uint high [[function_constant(7)]];

/*
 * Texture access helpers
 */

template <typename T>
T tex2D(texture2d<float, access::read> tex, ushort x, ushort y)
{
    return tex.read(ushort2(x, y)).x;
}

template <>
float2 tex2D<float2>(texture2d<float, access::read> tex, ushort x, ushort y)
{
    return tex.read(ushort2(x, y)).xy;
}

float2 tex2D(texture2d<float, access::read> tex1,
             texture2d<float, access::read> tex2,
             ushort x, ushort y, bool biplanar)
{
    if (biplanar) {
        return tex1.read(ushort2(x, y)).xy;
    } else {
        float2 value;
        value.x = tex2D<float>(tex1, x, y);
        value.y = tex2D<float>(tex2, x, y);
        return value;
    }
}

/*
 * Monochrome helpers
 */

float envelope(const float x)
{
    const float beta = 0.6f;

    if (x < beta) {
        const float tmp = fabs(x / beta - 1.f);
        return 1.f - tmp * tmp;
    } else {
        const float tmp = (1.f - x) / (1.f - beta);
        return tmp * tmp * (3.f - 2.f * tmp);
    }
}

float filter_sample(float b, float r, float u, float v, float size)
{
    return exp(-saturate(((b - u) * (b - u) + (r - v) * (r - v)) * size));
}

float filter_pixel(
    texture2d<float, access::write> dst,
    texture2d<float, access::read> src_y,
    texture2d<float, access::read> src_u,
    texture2d<float, access::read> src_v,
    ushort2 pos)
{
    const float ihigh = 1.f - high;
    const float isize = 1.f / size;
    const float b = cb * .5f;
    const float r = cr * .5f;

    const int cx = pos.x >> subw;
    const int cy = pos.y >> subh;

    float  y  = tex2D<float>(src_y, pos.x, pos.y);
    float2 uv = tex2D(src_u, src_v, cx, cy, biplanar) - .5f;

    float tt, t, ny;

    ny = filter_sample(b, r, uv.x, uv.y, isize);
    tt = envelope(y);
    t = tt + (1.f - tt) * ihigh;
    ny = (1.f - t) * y + t * ny * y;

    return saturate(ny);
}

/*
 * Kernel dispatch
 */

kernel void monochrome(
    texture2d<float, access::write> dst   [[texture(0)]],
    texture2d<float, access::read>  src_y [[texture(1)]],
    texture2d<float, access::read>  src_u [[texture(2)]],
    texture2d<float, access::read>  src_v [[texture(3)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if ((pos.x >= dst.get_width()) || (pos.y >= dst.get_height())) {
        return;
    }

    if (plane == 0) {
        float value = filter_pixel(dst, src_y, src_u, src_v, pos);
        dst.write(value, pos);
    } else {
        float half_value = 0.5;
        dst.write(half_value, pos);
    }
}
