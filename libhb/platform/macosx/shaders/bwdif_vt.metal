/* bwdif.metal

   Copyright (c) 2003-2025 HandBrake Team
   Copyright (c) 2019 Philip Langdale <philipl@overt.org>

   port of FFmpeg vf_bwdif_cuda.

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

struct params {
    uint channels;
    uint parity;
    uint tff;
    bool is_second_field;
    bool skip_spatial_check;
    bool is_field_end;
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
T tex2D(texture2d<float, access::read> tex, uint x, uint y)
{
    return tex.read(uint2(x, y)).x;
}

template <>
float2 tex2D<float2>(texture2d<float, access::read> tex, uint x, uint y)
{
    return tex.read(uint2(x, y)).xy;
}

/*
 * Bwdiff helpers
 */

constant static const float coef_lf[2] = { 4309, 213 };
constant static const float coef_hf[3] = { 5570, 3801, 1016 };
constant static const float coef_sp[2] = { 5077, 981 };

template<typename T>
T filter_intra(T cur_prefs3, T cur_prefs,
               T cur_mrefs, T cur_mrefs3)
{
    T final = (coef_sp[0] * (cur_mrefs + cur_prefs) -
                 coef_sp[1] * (cur_mrefs3 + cur_prefs3)) / (1 << 13);
    return saturate(final);
}

template<typename T>
T filter_temp(T cur_prefs3, T cur_prefs, T cur_mrefs, T cur_mrefs3,
              T prev2_prefs4, T prev2_prefs2, T prev2_0, T prev2_mrefs2, T prev2_mrefs4,
              T prev_prefs, T prev_mrefs, T next_prefs, T next_mrefs,
              T next2_prefs4, T next2_prefs2, T next2_0, T next2_mrefs2, T next2_mrefs4)
{
    T final;

    T c = cur_mrefs;
    T d = (prev2_0 + next2_0) / 2;
    T e = cur_prefs;

    T temporal_diff0 = abs(prev2_0 - next2_0);
    T temporal_diff1 = (abs(prev_mrefs - c) + abs(prev_prefs - e)) / 2;
    T temporal_diff2 = (abs(next_mrefs - c) + abs(next_prefs - e)) / 2;
    T diff = max3(temporal_diff0 / 2, temporal_diff1, temporal_diff2);

    if (!diff) {
        final = d;
    } else {
        T b = ((prev2_mrefs2 + next2_mrefs2) / 2) - c;
        T f = ((prev2_prefs2 + next2_prefs2) / 2) - e;
        T dc = d - c;
        T de = d - e;
        T mmax = max3(de, dc, min(b, f));
        T mmin = min3(de, dc, max(b, f));
        diff = max3(diff, mmin, -mmax);

        float interpol;
        if (abs(c - e) > temporal_diff0) {
            interpol = (((coef_hf[0] * (prev2_0 + next2_0)
                - coef_hf[1] * (prev2_mrefs2 + next2_mrefs2 + prev2_prefs2 + next2_prefs2)
                + coef_hf[2] * (prev2_mrefs4 + next2_mrefs4 + prev2_prefs4 + next2_mrefs4)) / 4)
                + coef_lf[0] * (c + e) - coef_lf[1] * (cur_mrefs3 + cur_prefs3)) / (1 << 13);
        } else {
            interpol = (coef_sp[0] * (c + e) - coef_sp[1] * (cur_mrefs3 + cur_prefs3)) / (1 << 13);
        }

        if (interpol > d + diff) {
            interpol = d + diff;
        } else if (interpol < d - diff) {
            interpol = d - diff;
        }
        final = saturate(interpol);
    }

    return final;
}

template<typename T>
T bwdif_single(texture2d<float, access::write> dst,
               texture2d<float, accesstype> prev,
               texture2d<float, accesstype> cur,
               texture2d<float, accesstype> next,
               int parity, int tff,
               bool is_field_end, bool is_second_field,
               ushort2 pos)
{
    // Don't modify the primary field
    if (pos.y % 2 == parity) {
        return tex2D<T>(cur, pos.x, pos.y);
    }

    T cur_prefs3 = tex2D<T>(cur, pos.x, pos.y + 3);
    T cur_prefs  = tex2D<T>(cur, pos.x, pos.y + 1);
    T cur_mrefs  = tex2D<T>(cur, pos.x, pos.y - 1);
    T cur_mrefs3 = tex2D<T>(cur, pos.x, pos.y - 3);

    if (is_field_end) {
        return filter_intra(cur_prefs3, cur_prefs, cur_mrefs, cur_mrefs3);
    }

    // Calculate temporal prediction
    texture2d<float, accesstype> prev2 = prev;
    texture2d<float, accesstype> prev1 = is_second_field ? cur : prev;
    texture2d<float, accesstype> next1 = is_second_field ? next : cur;
    texture2d<float, accesstype> next2 = next;

    T prev2_prefs4 = tex2D<T>(prev2, pos.x, pos.y+ 4);
    T prev2_prefs2 = tex2D<T>(prev2, pos.x, pos.y + 2);
    T prev2_0 = tex2D<T>(prev2, pos.x, pos.y + 0);
    T prev2_mrefs2 = tex2D<T>(prev2, pos.x, pos.y - 2);
    T prev2_mrefs4 = tex2D<T>(prev2, pos.x, pos.y - 4);
    T prev_prefs = tex2D<T>(prev1, pos.x, pos.y + 1);
    T prev_mrefs = tex2D<T>(prev1, pos.x, pos.y - 1);
    T next_prefs = tex2D<T>(next1, pos.x, pos.y + 1);
    T next_mrefs = tex2D<T>(next1, pos.x, pos.y - 1);
    T next2_prefs4 = tex2D<T>(next2, pos.x, pos.y + 4);
    T next2_prefs2 = tex2D<T>(next2, pos.x, pos.y + 2);
    T next2_0 = tex2D<T>(next2, pos.x, pos.y + 0);
    T next2_mrefs2 = tex2D<T>(next2, pos.x, pos.y - 2);
    T next2_mrefs4 = tex2D<T>(next2, pos.x, pos.y - 4);

    return filter_temp(cur_prefs3, cur_prefs, cur_mrefs, cur_mrefs3,
                       prev2_prefs4, prev2_prefs2, prev2_0, prev2_mrefs2, prev2_mrefs4,
                       prev_prefs, prev_mrefs, next_prefs, next_mrefs,
                       next2_prefs4, next2_prefs2, next2_0, next2_mrefs2, next2_mrefs4);
}

template<typename T>
T bwdif_double(texture2d<float, access::write> dst,
               texture2d<float, accesstype> prev,
               texture2d<float, accesstype> cur,
               texture2d<float, accesstype> next,
               int parity, int tff,
               bool is_field_end, bool is_second_field,
               ushort2 pos)
{
    // Don't modify the primary field
    if (pos.y % 2 == parity) {
        return tex2D<T>(cur, pos.x, pos.y);
    }

    T cur_prefs3 = tex2D<T>(cur, pos.x, pos.y + 3);
    T cur_prefs  = tex2D<T>(cur, pos.x, pos.y + 1);
    T cur_mrefs  = tex2D<T>(cur, pos.x, pos.y - 1);
    T cur_mrefs3 = tex2D<T>(cur, pos.x, pos.y - 3);

    if (is_field_end) {
        T final;
        final.x = filter_intra(cur_prefs3.x, cur_prefs.x, cur_mrefs.x, cur_mrefs3.x);
        final.y = filter_intra(cur_prefs3.y, cur_prefs.y, cur_mrefs.y, cur_mrefs3.y);
        return final;
    }

    // Calculate temporal prediction
    texture2d<float, accesstype> prev2 = prev;
    texture2d<float, accesstype> prev1 = is_second_field ? cur : prev;
    texture2d<float, accesstype> next1 = is_second_field ? next : cur;
    texture2d<float, accesstype> next2 = next;

    T prev2_prefs4 = tex2D<T>(prev2, pos.x, pos.y+ 4);
    T prev2_prefs2 = tex2D<T>(prev2, pos.x, pos.y + 2);
    T prev2_0 = tex2D<T>(prev2, pos.x, pos.y + 0);
    T prev2_mrefs2 = tex2D<T>(prev2, pos.x, pos.y - 2);
    T prev2_mrefs4 = tex2D<T>(prev2, pos.x, pos.y - 4);
    T prev_prefs = tex2D<T>(prev1, pos.x, pos.y + 1);
    T prev_mrefs = tex2D<T>(prev1, pos.x, pos.y - 1);
    T next_prefs = tex2D<T>(next1, pos.x, pos.y + 1);
    T next_mrefs = tex2D<T>(next1, pos.x, pos.y - 1);
    T next2_prefs4 = tex2D<T>(next2, pos.x, pos.y + 4);
    T next2_prefs2 = tex2D<T>(next2, pos.x, pos.y + 2);
    T next2_0 = tex2D<T>(next2, pos.x, pos.y + 0);
    T next2_mrefs2 = tex2D<T>(next2, pos.x, pos.y - 2);
    T next2_mrefs4 = tex2D<T>(next2, pos.x, pos.y - 4);

    T final;
    final.x = filter_temp(cur_prefs3.x, cur_prefs.x, cur_mrefs.x, cur_mrefs3.x,
                          prev2_prefs4.x, prev2_prefs2.x, prev2_0.x, prev2_mrefs2.x, prev2_mrefs4.x,
                          prev_prefs.x, prev_mrefs.x, next_prefs.x, next_mrefs.x,
                          next2_prefs4.x, next2_prefs2.x, next2_0.x, next2_mrefs2.x, next2_mrefs4.x);
    final.y = filter_temp(cur_prefs3.y, cur_prefs.y, cur_mrefs.y, cur_mrefs3.y,
                          prev2_prefs4.y, prev2_prefs2.y, prev2_0.y, prev2_mrefs2.y, prev2_mrefs4.y,
                          prev_prefs.y, prev_mrefs.y, next_prefs.y, next_mrefs.y,
                          next2_prefs4.y, next2_prefs2.y, next2_0.y, next2_mrefs2.y, next2_mrefs4.y);
    return final;
}


/*
 * Kernel dispatch
 */

kernel void deint(
    texture2d<float, access::write> dst [[texture(0)]],
    texture2d<float, accesstype> prev [[texture(1)]],
    texture2d<float, accesstype> cur  [[texture(2)]],
    texture2d<float, accesstype> next [[texture(3)]],
    constant params& p [[buffer(0)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if ((pos.x >= dst.get_width()) || (pos.y >= dst.get_height())) {
        return;
    }

    float2 pred;
    if (p.channels == 1) {
        pred = float2(bwdif_single<float>(dst, prev, cur, next,
                                   p.parity, p.tff,
                                   p.is_field_end, p.is_second_field,
                                   pos));
    } else {
        pred = bwdif_double<float2>(dst, prev, cur, next,
                                    p.parity, p.tff,
                                    p.is_field_end, p.is_second_field,
                                    pos);
    }
    dst.write(pred.xyyy, pos);
}
