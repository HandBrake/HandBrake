/*
 * history — the temporal path's five-tap Catmull-Rom reprojection, `history.comp` in MSL.
 * The order of every sum is the reference's; the build has no contraction to regroup it.
 */
#include "nr_metal.h"

inline void catmull(float normalized, uint dimension,
                    thread float &outer0, thread float &middle, thread float &outer3,
                    thread float &weight0, thread float &weight3, thread float &gain) {
    float pixel = normalized * float(dimension) - 0.5f;
    float base_index = floor(pixel);
    float t = clamp(pixel - base_index, 0.0f, 1.0f);
    float square = t * t, cube = square * t;
    weight0 = -0.5f * t + square - 0.5f * cube;
    float w1 = 1.0f - 2.5f * square + 1.5f * cube;
    float w2 = 0.5f * t + 2.0f * square - 1.5f * cube;
    weight3 = -0.5f * square + 0.5f * cube;
    gain = w1 + w2;
    float base = base_index + 0.5f;
    float lower = 0.5f, upper = float(dimension) - 0.5f;
    outer0 = clamp(base - 1.0f, lower, upper);
    middle = clamp(base + w2 / gain, lower, upper);
    outer3 = clamp(base + 2.0f, lower, upper);
}

inline void bilinear(device const float *a, float x, float y, uint height, uint width,
                     uint channels, thread float *rgb) {
    float px = x - 0.5f, py = y - 0.5f;
    int x0 = int(clamp(floor(px), 0.0f, float(width - 1u)));
    int y0 = int(clamp(floor(py), 0.0f, float(height - 1u)));
    int x1 = min(x0 + 1, int(width - 1u));
    int y1 = min(y0 + 1, int(height - 1u));
    float tx = clamp(px - float(x0), 0.0f, 1.0f);
    float ty = clamp(py - float(y0), 0.0f, 1.0f);
    for (uint ch = 0u; ch < channels; ch++) {
        float top = a[(uint(y0) * width + uint(x0)) * channels + ch] * (1.0f - tx)
                  + a[(uint(y0) * width + uint(x1)) * channels + ch] * tx;
        float bottom = a[(uint(y1) * width + uint(x0)) * channels + ch] * (1.0f - tx)
                     + a[(uint(y1) * width + uint(x1)) * channels + ch] * tx;
        rgb[ch] = top * (1.0f - ty) + bottom * ty;
    }
}

kernel void history(constant Push &pc [[buffer(0)]],
                    uint pixel [[thread_position_in_grid]]) {
    if (pixel >= pc.m) return;
    uint channels = pc.n, height = pc.k, width = pc.batch;
    uint x = pixel % width, y = pixel / width;
    device const float *a = float_ptr(pc.a);
    device const float *b = float_ptr(pc.b);
    device float *c = float_out(pc.c);

    /* `flags` chooses what the second buffer holds: motion, or the coordinate itself. */
    float u, v;
    if (pc.flags != 0u) {
        u = b[pixel * 2u];
        v = b[pixel * 2u + 1u];
    } else {
        float su = (float(x) + 0.5f) / float(width);
        su += b[pixel * 2u];
        float sv = (float(y) + 0.5f) / float(height);
        sv += b[pixel * 2u + 1u];
        u = su; v = sv;
    }

    float xo0, xm, xo3, xw0, xw3, xg, yo0, ym, yo3, yw0, yw3, yg;
    catmull(u, width, xo0, xm, xo3, xw0, xw3, xg);
    catmull(v, height, yo0, ym, yo3, yw0, yw3, yg);

    float weights[5] = { xw0 * yg, xg * yw0, xg * yg, xg * yw3, xw3 * yg };
    float coords[5][2] = { { xo0, ym }, { xm, yo0 }, { xm, ym }, { xm, yo3 }, { xo3, ym } };
    float total[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float tap[4];
    for (uint i = 0u; i < 5u; i++) {
        bilinear(a, coords[i][0], coords[i][1], height, width, channels, tap);
        for (uint ch = 0u; ch < channels; ch++) {
            float term = weights[i] * tap[ch];
            total[ch] += term;
        }
    }
    float denominator = weights[0] + weights[1];
    denominator += weights[2];
    denominator += weights[3];
    denominator += weights[4];
    for (uint ch = 0u; ch < channels; ch++)
        c[pixel * channels + ch] = total[ch] / denominator;
}
