/*
 * ffn_fused_portable — `ffn_fused_portable.comp` in HLSL: a feed-forward in one pass, the
 * gated hidden layer kept in groupshared memory, 32 hidden columns at a time.
 *
 * It must equal this backend's two-GEMM reference bit for bit: the expand through the
 * tiled `gemm_portable.hlsl` with the gate and E4M3 publish into a half hidden buffer,
 * then the projection through `gemm_portable.hlsl` with the residual in its epilogue. So
 * both products are the portable GEMM's lane layout and order (rows r and r+8 of a 16-row
 * block, four consecutive columns cq and cq+16, float4 accumulators taking one K term at a
 * time from zero, `acc += a * b`); the projection's accumulator runs across the hidden
 * chunks in order, the order the reference sums the whole hidden width in.
 *
 * Operands: u0 the input (half, m x n), u1 the expand weights (groups x n x k, group stride
 * sa), u5 the projection weights (groups x k x 32, group stride sb; push offset 120), u2
 * the output (float32, half with 0x1000; row stride ldc, 32 when 0; group g at columns
 * 32g), u3 the residual's skip and u4 its per-channel cosine (flag 0x20000; a half skip
 * with 0x40000). Eight 16-row blocks a 256-lane group on x, one to each 32-lane slice,
 * split by libd3dmx with pc.spare; the feed-forward group on y.
 *
 * As `ffn_fused_portable.comp`: for the narrow blocks' shape (32 channels, 128 hidden;
 * flag 0x800000, set by libd3dmx) both 8 KB weight matrices are loaded into groupshared
 * memory once for the eight blocks instead of once for every 16 rows; other shapes read
 * them from their buffers as before. 16 KB of weights and a kilobyte of hidden chunk a
 * slice, 24 KB. A surplus slice past m idles through the group's barriers.
 *
 * Block 70's input can be made here too (flag 0x1000000, the staged shape with the
 * residual), as `ffn_fused_portable.comp` makes it: the level above upsampled 2x times the
 * sin plus the skip times the cos, in resident.hlsl's UPSAMPLE_MERGE shapes, which this
 * backend's reference stored as float32 for the residual and as half for the expand. Each
 * lane makes the sixty-four half values of A it reads — rows r and r + 8, all 32 channels —
 * once, into registers, and each merged value again where its residual needs it. And block
 * 0's stem (flag 0x2000000): the features times the adapter, the tiled portable GEMM's
 * sixteen multiply-adds from zero four columns at a time. u0 is then the level above (half)
 * or the features (half, rows x 16), u3 the skip (half), u6 the sin then cos (float32) or
 * the adapter (16 x 32 half) at the byte offset in p0, lda the width and ldb the level
 * above's. The residual is then `branch + skip * cosine`, add_gemm_residual's shape with the
 * float32 skip the reference's residual read.
 */
#include "nr_d3d.hlsli"
#include "nr_epilogue.hlsli"

static const uint ROWS = 16u, CHUNK = 32u, OUT = 32u, TM = 8u, TN = 16u, SLICES = 8u;
static const uint EPI_GATE_E4M3 = 3u;              // the expand GEMM's publish

/* as words, two halves each: the staged expand's 32 x 128 at 0, the projection's 128 x 32
 * at 2048; then each slice's chunk of the gated hidden layer, 16 x 32 halves */
groupshared uint weights[4096];
groupshared uint hidden[SLICES * ROWS * CHUNK / 2u];
static const uint PROJECTION = 2048u;

float lo(uint w) { return f16tof32(w & 0xFFFFu); }
float hi(uint w) { return f16tof32(w >> 16); }
bool merging() { return (operation_flags() & 0x1000000u) != 0u; }
bool stemming() { return (operation_flags() & 0x2000000u) != 0u; }

/* Four channels of the merge at one pixel, as UPSAMPLE_MERGE makes each: the scale its own
 * rounded product (`precise`), then `scaled + skip * cos`. `c` is a multiple of four. */
float4 merged4(uint pixel, uint c) {
    uint x = pixel % pc.lda, y = pixel / pc.lda;
    uint above = ((y / 2u) * pc.ldb + x / 2u) * 32u + c;
    uint G = asuint(pc.p0);
    float4 out4;
    [unroll] for (uint e = 0u; e < 4u; e++) {
        precise float scaled = ld_f16(bufA, pc.oa.x, above + e) * ld_f32(bufG, G, c + e);
        out4[e] = scaled + ld_f16(bufD, pc.od.x, pixel * 32u + c + e) * ld_f32(bufG, G, 32u + c + e);
    }
    return out4;
}

/* Four channels of the stem at one pixel, as the tiled portable GEMM sums them. */
float4 stem4(uint pixel, uint c) {
    uint G = asuint(pc.p0);
    float4 acc = float4(0.0, 0.0, 0.0, 0.0);
    for (uint k = 0u; k < 16u; k++) {
        uint at = k * 32u + c;
        float4 bv = float4(ld_f16(bufG, G, at), ld_f16(bufG, G, at + 1u),
                           ld_f16(bufG, G, at + 2u), ld_f16(bufG, G, at + 3u));
        float av = ld_f16(bufA, pc.oa.x, pixel * 16u + k);
        acc += av * bv;
    }
    return acc;
}

float4 made4(uint pixel, uint c) { return merging() ? merged4(pixel, c) : stem4(pixel, c); }

/* four consecutive staged weights from an even half index: two words */
float4 staged4(uint at) {
    uint w0 = weights[at >> 1u], w1 = weights[(at >> 1u) + 1u];
    return float4(lo(w0), hi(w0), lo(w1), hi(w1));
}

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint index : SV_GroupIndex) {
    uint group = gid.y;
    uint cin_all = pc.n, width_all = pc.k;
    uint expand_base = group * pc.sa, project_base = group * pc.sb;
    uint A = pc.oa.x, Bx = pc.ob.x, P = pc.of.x, C = pc.oc.x;
    bool staged = (operation_flags() & 0x800000u) != 0u;
    if (staged) {
        [unroll] for (uint t = 0u; t < 2u; t++) {
            uint at = (index + t * 256u) * 16u;
            uint4 e4 = bufB.Load4(Bx + expand_base * 2u + at);
            uint4 p4 = bufF.Load4(P + project_base * 2u + at);
            [unroll] for (uint w = 0u; w < 4u; w++) {
                weights[at / 4u + w] = e4[w];
                weights[PROJECTION + at / 4u + w] = p4[w];
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint slice = index >> 5u, lane = index & 31u;
    uint row = ((gid.x + pc.spare) * SLICES + slice) * ROWS;
    bool live_rows = row < pc.m;                 // uniform across the slice, not the group
    uint cin = staged ? 32u : cin_all, width = staged ? 128u : width_all;
    uint ldc = pc.ldc != 0u ? pc.ldc : OUT;
    uint r = lane >> 2u, cq = (lane & 3u) * 4u;
    uint mine = slice * ROWS * CHUNK / 2u;       // this slice's hidden chunk, in words

    float4 result[2][2];
    [unroll] for (uint i0 = 0; i0 < 2u; i0++)
        [unroll] for (uint j0 = 0; j0 < 2u; j0++)
            result[i0][j0] = float4(0.0, 0.0, 0.0, 0.0);

    /* an input made here: this lane's rows of A, published to E4M3 as every feed-forward
     * GEMM reads its input (the residual takes it raw, made again), two to a word */
    bool made = merging() || stemming();
    uint held[2][16];
    [unroll] for (uint ih = 0; ih < 2u; ih++)
        [unroll] for (uint wh = 0; wh < 16u; wh++) held[ih][wh] = 0u;
    if (made && live_rows) {
        [unroll] for (uint im = 0; im < 2u; im++)
            [unroll] for (uint cm = 0; cm < 32u; cm += 4u) {
                float4 v = made4(row + r + im * TM, cm);
                v = float4(e4m3(v.x), e4m3(v.y), e4m3(v.z), e4m3(v.w));
                held[im][cm / 2u] = f32tof16(v.x) | (f32tof16(v.y) << 16);
                held[im][cm / 2u + 1u] = f32tof16(v.z) | (f32tof16(v.w) << 16);
            }
    }

    for (uint chunk = 0u; chunk < width; chunk += CHUNK) {
        if (live_rows) {
            /* the expand's 16x32 block for these hidden columns, as the tiled portable GEMM */
            float4 h[2][2];
            [unroll] for (uint i1 = 0; i1 < 2u; i1++)
                [unroll] for (uint j1 = 0; j1 < 2u; j1++)
                    h[i1][j1] = float4(0.0, 0.0, 0.0, 0.0);
            for (uint k = 0; k < cin; k++) {
                float4 bv[2];
                [unroll] for (uint j = 0; j < 2u; j++) {
                    uint at = k * width + chunk + cq + j * TN;
                    if (staged)
                        bv[j] = staged4(at);
                    else
                        bv[j] = float4(ld_f16(bufB, Bx, expand_base + at), ld_f16(bufB, Bx, expand_base + at + 1u),
                                       ld_f16(bufB, Bx, expand_base + at + 2u), ld_f16(bufB, Bx, expand_base + at + 3u));
                }
                [unroll] for (uint i = 0; i < 2u; i++) {
                    float av = made ? ((k & 1u) ? hi(held[i][k >> 1u]) : lo(held[i][k >> 1u]))
                                    : ld_f16(bufA, A, (row + r + i * TM) * cin + k);
                    [unroll] for (uint j2 = 0; j2 < 2u; j2++)
                        h[i][j2] += av * bv[j2];
                }
            }
            /* the expand's publish, stored as the half buffer the reference writes holds it */
            [unroll] for (uint i2 = 0; i2 < 2u; i2++)
                [unroll] for (uint j3 = 0; j3 < 2u; j3++)
                    [unroll] for (uint e = 0; e < 4u; e += 2u) {
                        uint at = (r + i2 * TM) * CHUNK + cq + j3 * TN + e;
                        hidden[mine + at / 2u] =
                            f32tof16(publish(EPI_GATE_E4M3, h[i2][j3][e]))
                            | (f32tof16(publish(EPI_GATE_E4M3, h[i2][j3][e + 1u])) << 16);
                    }
        }
        GroupMemoryBarrierWithGroupSync();
        if (live_rows) {
            /* the projection over these 32 hidden rows, continuing the accumulator */
            for (uint kk = 0; kk < CHUNK; kk++) {
                float4 pv[2];
                [unroll] for (uint j = 0; j < 2u; j++) {
                    uint at = (chunk + kk) * OUT + cq + j * TN;
                    if (staged)
                        pv[j] = staged4(PROJECTION * 2u + at);
                    else
                        pv[j] = float4(ld_f16(bufF, P, project_base + at), ld_f16(bufF, P, project_base + at + 1u),
                                       ld_f16(bufF, P, project_base + at + 2u), ld_f16(bufF, P, project_base + at + 3u));
                }
                [unroll] for (uint i = 0; i < 2u; i++) {
                    uint at = (r + i * TM) * CHUNK + kk;
                    uint w = hidden[mine + at / 2u];
                    float av = (at & 1u) ? hi(w) : lo(w);
                    [unroll] for (uint j4 = 0; j4 < 2u; j4++)
                        result[i][j4] += av * pv[j4];
                }
            }
        }
        GroupMemoryBarrierWithGroupSync();       // the next chunk overwrites the hidden chunk
    }
    if (!live_rows) return;                      // no barrier follows

    uint epilogue = (operation_flags() >> 8) & 0xFu;
    bool narrow_out = (operation_flags() & 0x1000u) != 0u;
    [unroll] for (uint i3 = 0; i3 < 2u; i3++)
        [unroll] for (uint j5 = 0; j5 < 2u; j5++) {
            float4 v = result[i3][j5];
            uint at = (row + r + i3 * TM) * ldc + group * OUT + cq + j5 * TN;
            if (made) {
                /* the made input again as the residual's float32 skip */
                float4 skip = made4(row + r + i3 * TM, cq + j5 * TN);
                [unroll] for (uint e = 0; e < 4u; e++)
                    v[e] = publish(epilogue, v[e] + skip[e] * ld_f32(bufE, pc.oe.x, cq + j5 * TN + e));
            } else {
                [unroll] for (uint e = 0; e < 4u; e++)
                    v[e] = publish(epilogue, add_gemm_residual(v[e], at + e));
            }
            if (!narrow_out) {
                [unroll] for (uint q = 0; q < 4u; q++) st_f32(bufC, C, at + q, v[q]);
            } else if ((at & 3u) == 0u && (C & 7u) == 0u) {
                st_f16x4(bufC, C + at * 2u, v);
            } else {
                [unroll] for (uint q = 0; q < 4u; q++) st_f16(bufC, C, at + q, v[q]);
            }
        }
}
