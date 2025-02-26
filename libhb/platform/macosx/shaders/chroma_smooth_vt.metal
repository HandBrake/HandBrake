/* chroma_smooth.metal

   Copyright (c) 2003-2025 HandBrake Team

   port of FFmpeg unsharp.cl.

   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */


#include <metal_stdlib>
#include <metal_integer>
#include <metal_texture>

using namespace metal;

struct params {
    uint       channels;
    float      max;
    float      min;
    float      strength;  // strength
    int        size;      // pixel context region width (must be odd)
};

#define accesstype access::sample
constexpr sampler s(coord::pixel);

template <typename T>
T tex2D(texture2d<float, access::sample> tex, int x, int y)
{
    return tex.sample(s, float2(x, y)).x;
}

template <>
float2 tex2D<float2>(texture2d<float, access::sample> tex, int x, int y)
{
    return tex.sample(s, float2(x, y)).xy;
}

template <typename T>
T filter_global(
    texture2d<float, access::write> dst,
    texture2d<float, accesstype> src,
    const short size_x,
    const short size_y,
    const float amount,
    constant float *coef_matrix,
    ushort2 pos)
{
    ushort2 center = ushort2(size_x / 2, size_y / 2);
    T val = tex2D<T>(src, pos.x, pos.y);
    T sum = 0.0f;
    short x, y;

    for (y = 0; y < size_y; y++) {
        for (x = 0; x < size_x; x++) {
            ushort2 posl = pos + ushort2(x, y) - center;
            sum += coef_matrix[y * size_x + x] *
                tex2D<T>(src, posl.x, posl.y);
        }
    }

    return val + (val - sum) * amount;
}

kernel void chroma_smooth_global(
    texture2d<float, access::write> dst [[texture(0)]],
    texture2d<float, accesstype>    src [[texture(1)]],
    constant float *coef_matrix [[buffer(0)]],
    constant params& params     [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if ((pos.x >= dst.get_width()) || (pos.y >= dst.get_height())) {
        return;
    }

    if (params.strength == 0) {
        float2 value;
        if (params.channels == 1) {
            value = float2(tex2D<float>(src, pos.x, pos.y));
        } else {
            value = tex2D<float2>(src, pos.x, pos.y);
        }
        dst.write(value.xyyy, pos);
    } else {
        float2 value;
        if (params.channels == 1) {
            value = float2(filter_global<float>(dst, src,
                                                params.size, params.size,
                                                params.strength,
                                                coef_matrix, pos));
        } else {
            value = filter_global<float2>(dst, src,
                                          params.size, params.size,
                                          params.strength,
                                          coef_matrix, pos);
        }
        dst.write(value.xyyy, pos);
    }
}

template <typename T, size_t S>
T filter_local(
    texture2d<float, access::write> dst,
    texture2d<float, accesstype> src,
    const short size_x,
    const short size_y,
    const float max,
    const float min,
    const float amount,
    constant float *coef_x,
    constant float *coef_y,
    threadgroup T tmp[S][S],
    ushort2 gid,
    ushort2 lid)
{
    constexpr short s = S / 2;
    constexpr short h = S / 4;

    gid *= s;

    short rad_x = size_x / 2;
    short rad_y = size_y / 2;
    short x, y;

    for (y = 0; y <= 1; y++) {
        for (x = 0; x <= 1; x++) {
            tmp[lid.y + s * y][lid.x + s * x] =
                tex2D<T>(src,
                         gid.x + lid.x + (s * x - h),
                         gid.y + lid.y + (s * y - h));
        }
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    T val = tmp[lid.y + h][lid.x + h];

    T horiz[2];
    for (y = 0; y <= 1; y++) {
        horiz[y] = 0.0f;
        for (x = 0; x < size_x; x++)
            horiz[y] += coef_x[x] * tmp[lid.y + y * s][lid.x + h + x - rad_x];
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (y = 0; y <= 1; y++) {
        tmp[lid.y + y * s][lid.x + h] = horiz[y];
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    T sum = 0.0f;
    for (y = 0; y < size_y; y++) {
        sum += coef_y[y] * tmp[lid.y + h + y - rad_y][lid.x + h];
    }

    return clamp(val - (val - sum) * amount, min, max);
}

kernel void chroma_smooth_local(
    texture2d<float, access::write> dst [[texture(0)]],
    texture2d<float, accesstype>    src [[texture(1)]],
    constant float *coef_x [[buffer(0)]],
    constant float *coef_y [[buffer(1)]],
    constant params& p     [[buffer(2)]],
    ushort2 pos   [[thread_position_in_grid]],
    ushort2 gid   [[threadgroup_position_in_grid]],
    ushort2 lid   [[thread_position_in_threadgroup]])
{
    if (p.strength == 0) {
        float2 value = tex2D<float2>(src, pos.x, pos.y);
        dst.write(value.xyyy, pos);
    } else {
        float2 value;
        constexpr size_t size = 32;
        threadgroup float2 tmp[size][size];
        value = filter_local<float2>(dst, src,
                                     p.size, p.size,
                                     p.max, p.min,
                                     p.strength,
                                     coef_x, coef_y,
                                     tmp, gid, lid);
        dst.write(value.xyyy, pos);
    }
}
