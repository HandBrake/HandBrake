/* motion_metric_vt.metal

   Copyright (c) 2003-2025 HandBrake Team

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
 * Texture access helpers
 */

constexpr sampler s(coord::pixel,address::clamp_to_zero);

template <typename T>
T tex2D(texture2d<float, access::sample> tex, ushort2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

template <typename T>
T tex2D(texture2d<half, access::sample> tex, ushort2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

/*
 * Motion Metric helpers
 */

template <typename T>
T gamma(T value) {
    return 4095 * pow(value, 2.2f);
}

/*
 * Kernel dispatch
 */

kernel void motion_metric_simd(
    texture2d<float, access::sample> a [[texture(0)]],
    texture2d<float, access::sample> b [[texture(1)]],
    device  uint *result [[buffer(0)]],
    ushort2 pos [[thread_position_in_grid]],
    ushort2 lid [[thread_position_in_threadgroup]],
    ushort  sid [[simdgroup_index_in_threadgroup]],
    ushort  w   [[dispatch_simdgroups_per_threadgroup]])
{
    threadgroup float partial_sums[32];

    int diff = gamma(tex2D<float>(a, pos)) - gamma(tex2D<float>(b, pos));
    uint squared = diff * diff;
    partial_sums[sid] = simd_sum(squared);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (lid.x == 0 && lid.y == 0) {
        uint sum = 0;
        for (ushort i = 0; i < w; i++) {
            sum += partial_sums[i];
        }
        result[pos.x / 16 + pos.y * (a.get_width() / 16) / 16] = sum;
    }
}

kernel void motion_metric(
    texture2d<float, access::sample> a [[texture(0)]],
    texture2d<float, access::sample> b [[texture(1)]],
    device  uint *result [[buffer(0)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (pos.x % 16 || pos.y % 16) {
        return;
    }

    uint sum = 0;
    for (uchar x = 0; x < 16; x++) {
        for (uchar y = 0; y < 16; y++) {
            int diff = gamma(tex2D<float>(a, ushort2(pos.x + x, pos.y + y))) -
                       gamma(tex2D<float>(b, ushort2(pos.x + x, pos.y + y)));
            uint squared = diff * diff;
            sum += squared;
        }
    }

    result[pos.x / 16 + pos.y * (a.get_width() / 16) / 16] = sum;
}
