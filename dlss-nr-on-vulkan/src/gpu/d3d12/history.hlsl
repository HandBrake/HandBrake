/*
 * history — the temporal path's five-tap Catmull-Rom reprojection, `history.comp` in
 * HLSL. The order of the sum is part of the answer: the reference accumulates the five
 * weighted taps left to right in float32, and `precise` keeps that from being regrouped
 * or contracted. One thread per pixel, 64 to a group.
 */
#include "nr_d3d.hlsli"

/* Catmull-Rom collapsed onto bilinear taps: the two inner weights are folded into one
 * offset sample, which is why only five taps are needed instead of sixteen. */
void catmull(float normalized, uint dimension,
             out float outer0, out float middle, out float outer3,
             out float weight0, out float weight3, out float gain) {
    precise float pixel = normalized * float(dimension) - 0.5;
    float base_index = floor(pixel);
    float t = clamp(pixel - base_index, 0.0, 1.0);
    float square = t * t, cube = square * t;
    weight0 = -0.5 * t + square - 0.5 * cube;
    float w1 = 1.0 - 2.5 * square + 1.5 * cube;
    float w2 = 0.5 * t + 2.0 * square - 1.5 * cube;
    weight3 = -0.5 * square + 0.5 * cube;
    gain = w1 + w2;
    float base = base_index + 0.5;
    float lower = 0.5, upper = float(dimension) - 0.5;
    outer0 = clamp(base - 1.0, lower, upper);
    middle = clamp(base + w2 / gain, lower, upper);
    outer3 = clamp(base + 2.0, lower, upper);
}

void bilinear(float x, float y, uint height, uint width, uint channels, out float rgb[4]) {
    uint A = pc.oa.x;
    float px = x - 0.5, py = y - 0.5;
    int x0 = int(clamp(floor(px), 0.0, float(width - 1u)));
    int y0 = int(clamp(floor(py), 0.0, float(height - 1u)));
    int x1 = min(x0 + 1, int(width - 1u));
    int y1 = min(y0 + 1, int(height - 1u));
    float tx = clamp(px - float(x0), 0.0, 1.0);
    float ty = clamp(py - float(y0), 0.0, 1.0);
    rgb[0] = 0.0; rgb[1] = 0.0; rgb[2] = 0.0; rgb[3] = 0.0;
    for (uint ch = 0u; ch < channels; ch++) {
        precise float top = ld_f32(bufA, A, (uint(y0) * width + uint(x0)) * channels + ch) * (1.0 - tx)
                          + ld_f32(bufA, A, (uint(y0) * width + uint(x1)) * channels + ch) * tx;
        precise float bottom = ld_f32(bufA, A, (uint(y1) * width + uint(x0)) * channels + ch) * (1.0 - tx)
                             + ld_f32(bufA, A, (uint(y1) * width + uint(x1)) * channels + ch) * tx;
        rgb[ch] = top * (1.0 - ty) + bottom * ty;
    }
}

[numthreads(64, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint pixel = (gid.x + pc.spare) * 64u + lid;
    if (pixel >= pc.m) return;
    uint channels = pc.n, height = pc.k, width = pc.batch;
    uint x = pixel % width, y = pixel / width;
    uint B = pc.ob.x;

    /* `flags` chooses what the second buffer holds: motion, from which the sample
     * coordinate is built here, or the coordinate itself, which lets this stand in for
     * `sample_history` unchanged. The coordinate has to be built exactly as the
     * reference builds it — a last-bit difference moves the tap by ~1e-04 of a pixel,
     * and a steep image gradient turns that into a visible-scale difference. */
    float u, v;
    if (pc.flags != 0u) {
        u = ld_f32(bufB, B, pixel * 2u);
        v = ld_f32(bufB, B, pixel * 2u + 1u);
    } else {
        precise float su = (float(x) + 0.5) / float(width);
        su += ld_f32(bufB, B, pixel * 2u);
        precise float sv = (float(y) + 0.5) / float(height);
        sv += ld_f32(bufB, B, pixel * 2u + 1u);
        u = su; v = sv;
    }

    float xo0, xm, xo3, xw0, xw3, xg, yo0, ym, yo3, yw0, yw3, yg;
    catmull(u, width, xo0, xm, xo3, xw0, xw3, xg);
    catmull(v, height, yo0, ym, yo3, yw0, yw3, yg);

    float weights[5] = { xw0 * yg, xg * yw0, xg * yg, xg * yw3, xw3 * yg };
    float coords[5][2] = { { xo0, ym }, { xm, yo0 }, { xm, ym }, { xm, yo3 }, { xo3, ym } };
    float total[4] = { 0.0, 0.0, 0.0, 0.0 };
    float tap[4];
    for (uint i = 0u; i < 5u; i++) {
        bilinear(coords[i][0], coords[i][1], height, width, channels, tap);
        for (uint ch = 0u; ch < channels; ch++) {
            precise float term = weights[i] * tap[ch];
            total[ch] += term;
        }
    }
    precise float denominator = weights[0] + weights[1];
    denominator += weights[2];
    denominator += weights[3];
    denominator += weights[4];
    for (uint ch = 0u; ch < channels; ch++)
        st_f32(bufC, pc.oc.x, pixel * channels + ch, total[ch] / denominator);
}
