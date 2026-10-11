/*
 * gemm_staged_int8_portable — `gemm_staged_int8_portable.comp` in HLSL: the integer staged
 * GEMM without a matrix unit. C[r][c] = acc[r][c] * a_scale[r] * b_scale[c], acc the exact
 * int32 dot product of an int8 row of A and an int8 row of the transposed weights. The
 * accumulator is exact in any order and the two scalings are the matrix kernel's two
 * multiplies in its order, so the output is the matrix kernel's bit for bit. The bytes are
 * read as 32-bit words and sign-extended here, so no 8-bit type is needed. A 32-lane group
 * takes an 8x16 block, a lane one row and four consecutive columns.
 *
 * Operands: u0 the activations (int8, M x K), u1 the weights (int8, N x K), u2 the output
 * (float32, M x N), u3 the row scales, u4 the column scales; K a multiple of 64. Row groups
 * on y, split past 65535 with pc.spare.
 */
#include "nr_d3d.hlsli"

int sbyte(uint w, uint i) { return int(w << (24u - 8u * i)) >> 24; }

int dot4(uint a, uint b) {
    int total = 0;
    [unroll] for (uint i = 0u; i < 4u; i++) total += sbyte(a, i) * sbyte(b, i);
    return total;
}

[numthreads(32, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint r = (gid.y + pc.spare) * 8u + (lid >> 2u);
    uint c = gid.x * 16u + (lid & 3u) * 4u;
    if (r >= pc.m || c >= pc.n) return;
    uint words = pc.k / 4u;
    int4 acc = int4(0, 0, 0, 0);
    for (uint w = 0u; w < words; w++) {
        uint a = bufA.Load(pc.oa.x + (r * words + w) * 4u);
        [unroll] for (uint q = 0u; q < 4u; q++)
            acc[q] += dot4(a, bufB.Load(pc.ob.x + ((c + q) * words + w) * 4u));
    }
    float scale = ld_f32(bufD, pc.od.x, r);
    [unroll] for (uint q2 = 0u; q2 < 4u; q2++)
        st_f32(bufC, pc.oc.x, r * pc.n + c + q2, float(acc[q2]) * scale * ld_f32(bufE, pc.oe.x, c + q2));
}
