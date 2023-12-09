/* comb_detect.metal

   Copyright (c) 2003-2023 HandBrake Team

   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <metal_stdlib>
#include <metal_integer>
#include <metal_texture>

using namespace metal;

#define HB_COMB_NONE  0
#define HB_COMB_LIGHT 1
#define HB_COMB_HEAVY 2

/*
 * Parameters
 */

struct params {
    int spatial_metric;

    float motion_threshold;
    float spatial_threshold;
    int   block_threshold;
    int   block_width;
    int   block_height;

    float gamma_motion_threshold;
    float gamma_spatial_threshold;
    float gamma_spatial_threshold6;
    float spatial_threshold_squared;
    float spatial_threshold6;
    float comb32detect_min;
    float comb32detect_max;

    bool  force_exaustive_check;
};

/*
 * Texture access helpers
 */

constexpr sampler s(coord::pixel);

template <typename T>
T tex2D(texture2d<float, access::sample> tex, short2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

template <typename T>
T tex2D(texture2d<float, access::sample> tex, ushort2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

template <typename T>
T tex2D(texture2d<ushort, access::sample> tex, short2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

template <typename T>
T tex2D(texture2d<ushort, access::sample> tex, ushort2 pos)
{
    return tex.sample(s, float2(pos)).x;
}

template <typename T>
T tex2D(texture2d<float, access::read> tex, uint x, uint y)
{
    return tex.read(uint2(x, y)).x;
}

template <typename T>
T tex2D(texture2d<ushort, access::read> tex, ushort2 pos)
{
    return tex.read(pos).x;
}

constexpr sampler szero(coord::pixel,address::clamp_to_zero);
template <typename T>
T tex2Dc(texture2d<ushort, access::sample> tex, short2 pos)
{
    return tex.sample(szero, float2(pos)).x;
}

template <typename T>
T tex2Dc(texture2d<ushort, access::sample> tex, ushort2 pos)
{
    return tex.sample(szero, float2(pos)).x;
}

/*
 * Comb detect helpers
 */

void write_result(
    device  atomic_int *combed,
    uchar block_threshold,
    ushort block_score)
{
    int current = atomic_load_explicit(combed, memory_order_relaxed);
    if (current == HB_COMB_HEAVY) {
        return;
    }

    if (block_score >= (block_threshold / 2)) {
        if (block_score > block_threshold) {
            atomic_store_explicit(combed, HB_COMB_HEAVY, memory_order_relaxed);
        } else {
            atomic_compare_exchange_weak_explicit(combed, &current, HB_COMB_LIGHT, memory_order_relaxed, memory_order_relaxed);
        }
    }
}

template <typename T>
T gamma(T value) {
    return pow(value, 2.2f);
}

template <typename T>
void detect_gamma_combed_segment(
    texture2d<float, access::sample> prev,
    texture2d<float, access::sample> cur,
    texture2d<float, access::sample> next,
    texture2d<ushort, access::write> mask,
    constant params& p,
    ushort2 pos)
{
    // A mishmash of various comb detection tricks
    // picked up from neuron2's Decomb plugin for
    // AviSynth and tritical's IsCombedT and
    // IsCombedTIVTC plugins.

    // Comb scoring algorithm
    const float mthresh  = p.gamma_motion_threshold;
    const float athresh  = p.gamma_spatial_threshold;
    const float athresh6 = p.gamma_spatial_threshold6;

    // These are just to make the buffer locations easier to read.
    const short2 up_2    = short2(pos.x, pos.y -2);
    const short2 up_1    = short2(pos.x, pos.y -1);
    const short2 down_1  = short2(pos.x, pos.y +1);
    const short2 down_2  = short2(pos.x, pos.y +2);

    const T up_diff   = gamma(tex2D<T>(cur, pos)) - gamma(tex2D<T>(cur, up_1));
    const T down_diff = gamma(tex2D<T>(cur, pos)) - gamma(tex2D<T>(cur, down_1));

    mask.write(0, pos);

    if ((up_diff >  athresh && down_diff >  athresh) ||
        (up_diff < -athresh && down_diff < -athresh)) {
        // The pixel above and below are different,
        // and they change in the same "direction" too.
        bool motion = false;
        if (mthresh > 0) {
            // Make sure there's sufficient motion between frame t-1 to frame t+1.
            if (abs(gamma(tex2D<T>(prev, pos))   - gamma(tex2D<T>(cur, pos)))     > mthresh &&
                abs(gamma(tex2D<T>(cur, up_1))   - gamma(tex2D<T>(next, up_1)))   > mthresh &&
                abs(gamma(tex2D<T>(cur, down_1)) - gamma(tex2D<T>(next, down_1))) > mthresh) {
                motion = true;
            }
            if (abs(gamma(tex2D<T>(next, pos))    - gamma(tex2D<T>(cur, pos)))    > mthresh &&
                abs(gamma(tex2D<T>(prev, up_1))   - gamma(tex2D<T>(cur, up_1)))   > mthresh &&
                abs(gamma(tex2D<T>(prev, down_1)) - gamma(tex2D<T>(cur, down_1))) > mthresh) {
                motion = true;
            }
        } else {
            // User doesn't want to check for motion,
            // so move on to the spatial check
            motion = true;
        }

        // If motion, or we can't measure motion yet…
        if (motion || p.force_exaustive_check) {
            // Tritical's noise-resistant combing scorer
            // The check is done on a bob+blur convolution
            const T combing = abs(gamma(tex2D<T>(cur, up_2))
                                  + (4 * gamma(tex2D<T>(cur, pos)))
                                  + gamma(tex2D<T>(cur, down_2))
                                  - (3 * (gamma(tex2D<T>(cur, up_1))
                                          + gamma(tex2D<T>(cur, down_1)))));

            // If the frame is sufficiently combed,
            // then mark it down on the mask as 1.
            if (combing > athresh6) {
                mask.write(1, pos);
            }
        }
    }
}

template <typename T>
void detect_combed_segment(
    texture2d<float, access::sample> prev,
    texture2d<float, access::sample> cur,
    texture2d<float, access::sample> next,
    texture2d<ushort, access::write> mask,
    constant params& p,
    ushort2 pos)
{
    // A mishmash of various comb detection tricks
    // picked up from neuron2's Decomb plugin for
    // AviSynth and tritical's IsCombedT and
    // IsCombedTIVTC plugins.

    // Comb scoring algorithm
    const float mthresh         = p.motion_threshold;
    const float athresh         = p.spatial_threshold;
    const float athresh_squared = p.spatial_threshold_squared;
    const float athresh6        = p.spatial_threshold6;

    // These are just to make the buffer locations easier to read.
    const short2 up_2    = short2(pos.x, pos.y -2);
    const short2 up_1    = short2(pos.x, pos.y -1);
    const short2 down_1  = short2(pos.x, pos.y +1);
    const short2 down_2  = short2(pos.x, pos.y +2);

    const float up_diff   = tex2D<T>(cur, pos) - tex2D<T>(cur, up_1);
    const float down_diff = tex2D<T>(cur, pos) - tex2D<T>(cur, down_1);

    mask.write(0, pos);

    if ((up_diff >  athresh && down_diff >  athresh) ||
        (up_diff < -athresh && down_diff < -athresh)) {
        // The pixel above and below are different,
        // and they change in the same "direction" too.
        bool motion = false;
        if (mthresh > 0) {
            // Make sure there's sufficient motion between frame t-1 to frame t+1.
            if (abs(tex2D<T>(prev, pos)   - tex2D<T>(cur, pos))     > mthresh &&
                abs(tex2D<T>(cur, up_1)   - tex2D<T>(next, up_1))   > mthresh &&
                abs(tex2D<T>(cur, down_1) - tex2D<T>(next, down_1)) > mthresh) {
                motion = true;
            }
            if (abs(tex2D<T>(next, pos)    - tex2D<T>(cur, pos))    > mthresh &&
                abs(tex2D<T>(prev, up_1)   - tex2D<T>(cur, up_1))   > mthresh &&
                abs(tex2D<T>(prev, down_1) - tex2D<T>(cur, down_1)) > mthresh) {
                motion = true;
            }
        } else {
            // User doesn't want to check for motion,
            // so move on to the spatial check
            motion = true;
        }

        // If motion, or we can't measure motion yet…
        if (motion || p.force_exaustive_check) {
            // That means it's time for the spatial check
            // We've got several options here
            if (p.spatial_metric == 0) {
                // Simple 32detect style comb detection.
                if ((abs(tex2D<T>(cur, pos) - tex2D<T>(cur, down_2)) < p.comb32detect_min) &&
                    (abs(tex2D<T>(cur, pos) - tex2D<T>(cur, down_1)) > p.comb32detect_max)) {
                    mask.write(1, pos);
                }
            } else if (p.spatial_metric == 1) {
                // This, for comparison, is what IsCombed uses
                // It's better, but still noise sensitive
                const T combing = (tex2D<T>(cur, up_1)   - tex2D<T>(cur, pos)) *
                                  (tex2D<T>(cur, down_1) - tex2D<T>(cur, pos));

                if (combing > athresh_squared) {
                    mask.write(1, pos);
                }
            } else if (p.spatial_metric == 2) {
                // Tritical's noise-resistant combing scorer
                // The check is done on a bob+blur convolution
                const T combing = abs(tex2D<T>(cur, up_2)
                                        + (4 * tex2D<T>(cur, pos))
                                        + tex2D<T>(cur, down_2)
                                        - (3 * (tex2D<T>(cur, up_1)
                                                + tex2D<T>(cur, down_1))));

                // If the frame is sufficiently combed,
                // then mark it down on the mask as 1.
                if (combing > athresh6) {
                    mask.write(1, pos);
                }
            }
        }
    }
}

/*
 * Kernel dispatch
 */

kernel void apply_mask(
    texture2d<ushort, access::read> mask [[texture(3)]],
    texture2d<half, access::write> dst [[texture(5)]],
    ushort2 pos [[thread_position_in_grid]])
{
    auto value = tex2D<ushort>(mask, pos) ? 1.h : 0.h;
    dst.write(value, pos);
}

kernel void check_filtered_combing_mask_simd(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]],
    ushort2 lid [[thread_position_in_threadgroup]],
    ushort  sid [[simdgroup_index_in_threadgroup]],
    ushort  w   [[dispatch_simdgroups_per_threadgroup]])
{
    threadgroup ushort partial_score[32];

    ushort value = tex2Dc<ushort>(mask, pos);
    partial_score[sid] = simd_sum(value);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (lid.x == 0 && lid.y == 0) {
        ushort block_score = 0;
        for (uchar i = 0; i < w; i++) {
            block_score += partial_score[i];
        }
        write_result(combed, p.block_threshold, block_score);
    }
}

kernel void check_filtered_combing_mask_quad(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]],
    ushort2 lid [[thread_position_in_threadgroup]],
    ushort  qid [[quadgroup_index_in_threadgroup]],
    ushort  w   [[dispatch_quadgroups_per_threadgroup]])
{
    threadgroup ushort partial_score[256];

    ushort value = tex2Dc<ushort>(mask, pos);
    partial_score[qid] = quad_sum(value);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (lid.x == 0 && lid.y == 0) {
        ushort block_score = 0;
        for (uchar i = 0; i < w; i++) {
            block_score += partial_score[i];
        }
        write_result(combed, p.block_threshold, block_score);
    }
}

kernel void check_filtered_combing_mask(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (pos.x % p.block_width > 0 || pos.y % p.block_height > 0) {
        return;
    }

    ushort block_score = 0;

    for (uchar x = 0; x < p.block_width; x++) {
        for (uchar y = 0; y < p.block_height; y++) {
            ushort2 block_pos = ushort2(pos.x + x, pos.y + y);
            block_score += tex2Dc<ushort>(mask, block_pos);
        }
    }

    write_result(combed, p.block_threshold, block_score);
}

kernel void check_combing_mask_simd(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]],
    ushort2 lid [[thread_position_in_threadgroup]],
    ushort  sid [[simdgroup_index_in_threadgroup]],
    ushort  w   [[dispatch_simdgroups_per_threadgroup]])
{
    threadgroup ushort partial_score[32];
    const short2 left  = short2(pos.x -1, pos.y);
    const short2 right = short2(pos.x +1, pos.y);

    ushort value = tex2Dc<ushort>(mask, left) & tex2Dc<ushort>(mask, pos) & tex2Dc<ushort>(mask, right);
    partial_score[sid] = simd_sum(value);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (lid.x == 0 && lid.y == 0) {
        ushort block_score = 0;
        for (uchar i = 0; i < w; i++) {
            block_score += partial_score[i];
        }
        write_result(combed, p.block_threshold, block_score);
    }
}

kernel void check_combing_mask_quad(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]],
    ushort2 lid [[thread_position_in_threadgroup]],
    ushort  qid [[quadgroup_index_in_threadgroup]],
    ushort  w   [[dispatch_quadgroups_per_threadgroup]])
{
    threadgroup ushort partial_score[256];
    const short2 left  = short2(pos.x -1, pos.y);
    const short2 right = short2(pos.x +1, pos.y);

    ushort value = tex2Dc<ushort>(mask, left) & tex2Dc<ushort>(mask, pos) & tex2Dc<ushort>(mask, right);
    partial_score[qid] = quad_sum(value);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (lid.x == 0 && lid.y == 0) {
        ushort block_score = 0;
        for (uchar i = 0; i < w; i++) {
            block_score += partial_score[i];
        }
        write_result(combed, p.block_threshold, block_score);
    }
}

kernel void check_combing_mask(
    texture2d<ushort, access::sample> mask [[texture(3)]],
    device  atomic_int *combed [[buffer(0)]],
    constant params& p         [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (pos.x % p.block_width > 0 || pos.y % p.block_height > 0) {
        return;
    }

    ushort block_score = 0;

    for (uchar x = 0; x < p.block_width; x++) {
        for (uchar y = 0; y < p.block_height; y++) {
            const ushort2 block_pos = ushort2(pos.x + x, pos.y + y);
            const short2 left  = short2(pos.x -1 +x, pos.y +y);
            const short2 right = short2(pos.x +1 +x, pos.y +y);
            block_score += tex2Dc<ushort>(mask, left) & tex2Dc<ushort>(mask, block_pos) & tex2Dc<ushort>(mask, right);
        }
    }
    write_result(combed, p.block_threshold, block_score);
}

kernel void dilate_mask(
    texture2d<ushort, access::sample>    src [[texture(3)]],
    texture2d<ushort, access::write> dst [[texture(4)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (tex2D<ushort>(src, pos)) {
        dst.write(1, pos);
        return;
    }

    const short2 up         = short2(pos.x, pos.y -1);
    const short2 up_left    = short2(pos.x -1, pos.y -1);
    const short2 up_right   = short2(pos.x +1, pos.y -1);
    const short2 down       = short2(pos.x, pos.y +1);
    const short2 down_left  = short2(pos.x -1,  pos.y +1);
    const short2 down_right = short2(pos.x +1, pos.y +1);
    const short2 left       = short2(pos.x -1, pos.y);
    const short2 right      = short2(pos.x +1, pos.y);

    constexpr uchar dilation_threshold = 4;
    const uchar count = tex2D<ushort>(src, up_left)   + tex2D<ushort>(src, up)   + tex2D<ushort>(src, up_right) +
                        tex2D<ushort>(src, left)      +                            tex2D<ushort>(src, right)    +
                        tex2D<ushort>(src, down_left) + tex2D<ushort>(src, down) + tex2D<ushort>(src, down_right);

    dst.write(count >= dilation_threshold, pos);

}

kernel void erode_mask(
    texture2d<ushort, access::sample> src [[texture(4)]],
    texture2d<ushort, access::write>  dst [[texture(3)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if (tex2D<ushort>(src, pos) == 0) {
        dst.write(0, pos);
        return;
    }

    const short2 up         = short2(pos.x, pos.y -1);
    const short2 up_left    = short2(pos.x -1, pos.y -1);
    const short2 up_right   = short2(pos.x +1, pos.y -1);
    const short2 down       = short2(pos.x, pos.y +1);
    const short2 down_left  = short2(pos.x -1,  pos.y +1);
    const short2 down_right = short2(pos.x +1, pos.y +1);
    const short2 left       = short2(pos.x -1, pos.y);
    const short2 right      = short2(pos.x +1, pos.y);

    constexpr uchar erosion_threshold = 2;
    const uchar count = tex2D<ushort>(src, up_left)   + tex2D<ushort>(src, up)   + tex2D<ushort>(src, up_right) +
                        tex2D<ushort>(src, left)      +                            tex2D<ushort>(src, right)    +
                        tex2D<ushort>(src, down_left) + tex2D<ushort>(src, down) + tex2D<ushort>(src, down_right);

    dst.write(count >= erosion_threshold, pos);
}

kernel void filter_classic(
    texture2d<ushort, access::sample> src [[texture(3)]],
    texture2d<ushort, access::write>  dst [[texture(4)]],
    ushort2 pos [[thread_position_in_grid]])
{
    const short2 left  = short2(pos.x -1, pos.y);
    const short2 right = short2(pos.x +1, pos.y);

    const uchar h_count = tex2D<ushort>(src, left) & tex2D<ushort>(src, pos) & tex2D<ushort>(src, right);

    dst.write(h_count, pos);
}

kernel void filter_erode_dilate(
    texture2d<ushort, access::sample> src [[texture(3)]],
    texture2d<ushort, access::write>  dst [[texture(4)]],
    ushort2 pos [[thread_position_in_grid]])
{
    const short2 up    = short2(pos.x, pos.y -1);
    const short2 down  = short2(pos.x, pos.y +1);
    const short2 left  = short2(pos.x -1, pos.y);
    const short2 right = short2(pos.x +1, pos.y);

    const uchar h_count = tex2D<ushort>(src, left) & tex2D<ushort>(src, pos) & tex2D<ushort>(src, right);
    const uchar v_count = tex2D<ushort>(src, up)   & tex2D<ushort>(src, pos) & tex2D<ushort>(src, down);

    dst.write(h_count & v_count, pos);
}

kernel void comb_detect(
    texture2d<float, access::sample> prev [[texture(0)]],
    texture2d<float, access::sample> cur  [[texture(1)]],
    texture2d<float, access::sample> next [[texture(2)]],
    texture2d<ushort, access::write> mask [[texture(3)]],
    constant params& p  [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    detect_combed_segment<float>(prev, cur, next, mask, p, pos);
}

kernel void comb_detect_gamma(
    texture2d<float, access::sample> prev [[texture(0)]],
    texture2d<float, access::sample> cur  [[texture(1)]],
    texture2d<float, access::sample> next [[texture(2)]],
    texture2d<ushort, access::write> mask [[texture(3)]],
    constant params& p  [[buffer(1)]],
    ushort2 pos [[thread_position_in_grid]])
{
    detect_gamma_combed_segment<float>(prev, cur, next, mask, p, pos);
}

