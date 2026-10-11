/*
 * gemm_int8 — the integer GEMM, `gemm_staged_int8.comp`'s contract in MSL:
 * C[r][c] = acc[r][c] * a_scale[r] * b_scale[c], acc the exact int32 dot product of an int8
 * row of A (M x K) and an int8 row of the weights stored transposed (N x K). The
 * accumulator is exact in any order and the two scalings are the matrix kernel's two
 * multiplies in its order, so this is its output bit for bit on any device
 * (`src/gpu/test_gemm_int8_staged.py`). Apple's simdgroup matrices have no integer type, so
 * there is one kernel, under both names a caller may hand over: a 32-thread threadgroup an
 * 8x16 block, a thread one row and four consecutive columns (`gemm_staged_int8_portable.comp`).
 *
 * Push: a the activations, b the weights, c the output (float32), d the row scales,
 * residual_cos the column scales; K a multiple of 64. Grid (N / 16, ceil(M / 8)).
 */
#include "nr_metal.h"

kernel void gemm_staged_int8(constant Push &pc [[buffer(0)]],
                             uint3 wg [[threadgroup_position_in_grid]],
                             uint lid [[thread_index_in_threadgroup]]) {
    uint r = wg.y * 8u + (lid >> 2u);
    uint c = wg.x * 16u + (lid & 3u) * 4u;
    if (r >= pc.m || c >= pc.n) return;
    device const packed_char4 *A = reinterpret_cast<device const packed_char4 *>(pc.a);
    device const packed_char4 *B = reinterpret_cast<device const packed_char4 *>(pc.b);
    uint words = pc.k / 4u;
    int4 acc = int4(0);
    for (uint w = 0u; w < words; w++) {
        int4 a = int4(char4(A[r * words + w]));
        for (uint q = 0u; q < 4u; q++) {
            int4 b = int4(char4(B[(c + q) * words + w]));
            acc[q] += a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }
    }
    float scale = float_ptr(pc.d)[r];
    device const float *column = float_ptr(pc.residual_cos);
    for (uint q = 0u; q < 4u; q++)
        float_out(pc.c)[r * pc.n + c + q] = float(acc[q]) * scale * column[c + q];
}
