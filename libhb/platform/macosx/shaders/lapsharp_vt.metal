/* lapsharp.metal

   Copyright (c) 2003-2024 HandBrake Team

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
    uint   channels;
    float  strength;
    uint   size;
    float  coef;
    int    kernel_id;
};

/*
 * Texture access helpers
 */

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
    texture2d<float, accesstype>    src,
    const float strength,
    const uint size,
    const float coef,
    constant int *mem,
    ushort2 pos)
{
    const int offset_min = -((size - 1) / 2);
    const int offset_max =   (size + 1) / 2;

    T val = tex2D<T>(src, pos.x, pos.y);

    if ((int(pos.y) < offset_max) ||
        (pos.y > dst.get_height() - offset_max) ||
        (int(pos.x) < offset_max) ||
        (pos.x > dst.get_width() - offset_max)) {
        return val;
    }

    T sum = 0;
    for (int x = offset_min; x < offset_max; x++) {
        for (int y = offset_min; y < offset_max; y++) {
            sum += tex2D<T>(src, pos.x + x, pos.y + y) *
                    mem[((y - offset_min) * size) + x - offset_min];
        }
    }

    return saturate(((sum * coef) - val) * strength + val);
}

kernel void lapsharp(
    texture2d<float, access::write> dst [[texture(0)]],
    texture2d<float, accesstype>    src [[texture(1)]],
    constant int *mem  [[buffer(0)]],
    constant params& p [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if ((pos.x >= dst.get_width()) || (pos.y >= dst.get_height())) {
        return;
    }

    if (p.strength == 0) {
        float2 value;
        if (p.channels == 1) {
            value = float2(tex2D<float>(src, pos.x, pos.y));
        } else {
            value = tex2D<float2>(src, pos.x, pos.y);
        }
        dst.write(value.xyyy, pos);
    } else {
        float2 value;
        if (p.channels == 1) {
            value = float2(filter_global<float>(dst, src,
                                                p.strength,
                                                p.size,
                                                p.coef,
                                                mem,
                                                pos));
        } else {
            value = filter_global<float2>(dst, src,
                                          p.strength,
                                          p.size,
                                          p.coef,
                                          mem,
                                          pos);
        }
        dst.write(value.xyyy, pos);
    }
}
