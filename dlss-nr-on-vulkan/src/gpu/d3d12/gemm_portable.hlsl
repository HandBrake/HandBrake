/*
 * gemm_portable — the resident GEMM without a matrix unit, `gemm_portable.comp` in HLSL:
 * FP16 operands promoted to FP32 and accumulated with ordinary multiply-adds, one lane
 * owning four consecutive columns of one row of an 8x16 block per 32-lane group (16x32
 * with -DRM=2 -DRN=2, the `gemm_tiled` slot; 16x64 with -DRM=2 -DRN=4, `gemm_portable_wide`,
 * which libd3dmx gives the GEMMs whose N is whole 64-column blocks — the same sums, twice
 * as many a lane). Same push block, flags (bit 0 a transposed
 * B, bits 8-11 the epilogue, bit 12 a half output), strides, batch on SV_GroupID.z,
 * epilogue and store as the SPIR-V, so the runtime dispatches it with the geometry it uses
 * there. This is the only GEMM on Direct3D 12: HLSL has no shipped cooperative matrix
 * (see libd3dmx.c), so `gemm_resident`, `gemm_tiled` and `gemm_staged` all resolve here.
 *
 * The fused epilogues of `gemm_resident.comp` / `gemm_staged.comp` live here too, on the
 * same flag bits (notes/improve-fusions.md, improve-qkv-epilogue.md):
 *   0x10000  compact head: the 32 -> 4 head writes only its four useful FP32 columns
 *            (base kernel; N = 16, ldc = 4, no publish)
 *   0x20000  the residual `+ skip * cosine` before the publish, the skip in d (half with
 *            0x40000), the cosine in the fifth operand; 0x80000 writes the rows of a
 *            padded window block straight back into the unpadded image
 *   0x100000 the QKV projection's epilogue (tiled build: a 32-wide column block is one
 *            head), see nr_epilogue.hlsli
 *   0x200000 a published (E4M3) half copy of the plain FP32 result into d as well
 *   0x400000 A gathered from the image in window order as it is loaded (tiled build),
 *            half already with 0x8000, else float32 rounded to half on the way in
 *   0x800000 block 0's window residual pooled and published in the epilogue (tiled
 *            build): the published skip into c as half, the pool into the sixth operand
 *   0x8000000 libxmx's packed staged loader (gemm_staged.comp's STAGED_PACKED, a
 *            specialization constant there) as this kernel can take it: the plain form
 *            fetches A sixteen bytes — eight K terms — at a time where the addresses
 *            allow. libd3dmx sets it on every GEMM it dispatches under
 *            XMX_PORTABLE_PACKED=1 (off by default: an M3's two portable paths measured
 *            slower with it, and no Direct3D 12 device has measured it); the sums are the
 *            eight-byte path's bit for bit.
 * Where the SPIR-V stages the accumulator through shared memory for these, this kernel
 * need not: every lane knows which elements it holds, and the residual and the publish
 * are per element. The QKV epilogue normalises whole rows, so it alone stages.
 */
#include "nr_d3d.hlsli"

#ifndef RM
#define RM 1
#endif
#ifndef RN
#define RN 1
#endif
static const uint TM = 8, TN = 16;
static const uint BM = TM * RM, BN = TN * RN;

#if RN == 2
#define QKV_BM BM
#define QKV_BN BN
groupshared float qkv_stage[BM * BN];
#endif
#include "nr_epilogue.hlsli"

/* Four consecutive elements to c, half or float, as one wide store when aligned. */
void store4(uint C, uint at, float4 v, bool narrow) {
    if (narrow) {
        if ((at & 3u) == 0u && (C & 7u) == 0u)
            st_f16x4(bufC, C + at * 2u, v);
        else
            [unroll] for (uint e = 0; e < 4u; e++)
                st_f16(bufC, C, at + e, v[e]);
    } else {
        if ((at & 3u) == 0u && (C & 15u) == 0u)
            st_f32x4(bufC, C + at * 4u, v);
        else
            [unroll] for (uint e = 0; e < 4u; e++)
                st_f32(bufC, C, at + e, v[e]);
    }
}

[numthreads(32, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint row = (gid.y + pc.spare) * BM, col = gid.x * BN;
    if (row >= pc.m || col >= pc.n) return;
    uint flags = operation_flags();
    bool transposed = (flags & 1u) != 0u;
    uint batch = gid.z;
    uint ao = batch * pc.sa, bo = batch * pc.sb, co = batch * pc.sc;
    uint lda = pc.lda != 0u ? pc.lda : pc.k;
    uint ldb = pc.ldb != 0u ? pc.ldb : (transposed ? pc.k : pc.n);
    uint ldc = pc.ldc != 0u ? pc.ldc : pc.n;
    uint A = pc.oa.x, B = pc.ob.x, C = pc.oc.x;

    /* Lane l owns row l/4 of every 8-row tile and columns 4*(l%4)..+3 of every 16-column
     * tile: the four lanes of a row share its A element, and a lane's four B elements are
     * consecutive in a row-major B. */
    uint r = lid >> 2u, cq = (lid & 3u) * 4u;

    /* Bit 0x400000: A is a window block's rows, gathered from the image as they are
     * loaded — the shifted-window partition done here instead of in a pass of its own
     * (gemm_staged.comp's WINDOW_A loader). `sa` and `sb` are the image's width and
     * height, `sc` the windows per row, `window_pad` the pad as (top << 16) | left; a
     * token outside the image reads zero, as the partition wrote, and a float32 image is
     * rounded to half on the way in, as the partition's narrow store did — or, with bit
     * 0x4000, published to E4M3 as it is loaded: a 32-channel block's feed-forward output,
     * which the projection reads published and the residual raw. The batch strides have
     * no other use with the one batch this mode allows. */
    bool window_a = (flags & 0x400000u) != 0u;
    bool image_half = (flags & 0x8000u) != 0u;
    bool image_publish = (flags & 0x4000u) != 0u;
    uint abase[RM];
    bool inside[RM];
    [unroll] for (uint i0 = 0; i0 < RM; i0++) {
        uint token_row = row + r + i0 * TM;
        if (window_a) {
            uint window = token_row / 64u, token = token_row % 64u;
            int y = int((window / pc.sc) * 8u + token / 8u) - int(pc.window_pad >> 16);
            int x = int((window % pc.sc) * 8u + token % 8u) - int(pc.window_pad & 0xffffu);
            inside[i0] = !(y < 0 || x < 0 || y >= int(pc.sb) || x >= int(pc.sa));
            abase[i0] = inside[i0] ? (uint(y) * pc.sa + uint(x)) * pc.k : 0u;
        } else {
            inside[i0] = true;
            abase[i0] = ao + token_row * lda;
        }
    }

    float4 acc[RM][RN];
    [unroll] for (uint i1 = 0; i1 < RM; i1++)
        [unroll] for (uint j1 = 0; j1 < RN; j1++)
            acc[i1][j1] = float4(0.0, 0.0, 0.0, 0.0);

    /* Eight-byte operand fetches where the strides and offsets allow: the same halves, fewer
     * loads, and every K term still goes into the accumulator alone and in order, so the
     * sums are bit for bit the four-load paths' (gemm_portable.comp, the same change). */
    bool wide_b = ((bo | ldb) & 3u) == 0u && (B & 7u) == 0u;
    bool wide_ab = wide_b && ((ao | lda | pc.k) & 3u) == 0u && (A & 7u) == 0u;
    bool packed_a = (flags & 0x8000000u) != 0u && wide_b && ((ao | lda | pc.k) & 7u) == 0u
                    && (A & 15u) == 0u;
    if (window_a) {
        /* the same multiply-adds as the plain path below, the A element gathered */
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            [unroll] for (uint j = 0; j < RN; j++) {
                uint at = bo + k * ldb + col + cq + j * TN;
                bv[j] = wide_b ? ld_f16x4(bufB, B + at * 2u)
                               : float4(ld_f16(bufB, B, at), ld_f16(bufB, B, at + 1u),
                                        ld_f16(bufB, B, at + 2u), ld_f16(bufB, B, at + 3u));
            }
            [unroll] for (uint i = 0; i < RM; i++) {
                float x = !inside[i] ? 0.0
                        : (image_half ? ld_f16(bufA, A, abase[i] + k) : ld_f32(bufA, A, abase[i] + k));
                float av = image_publish ? e4m3(x) : half_round(x);
                [unroll] for (uint j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    } else if (transposed && wide_ab) {
        /* the transposed form, both operands fetched four K terms at a time */
        for (uint k0 = 0; k0 < pc.k; k0 += 4u) {
            float4 as4[RM], bt[RN][4];
            [unroll] for (uint i = 0; i < RM; i++)
                as4[i] = ld_f16x4(bufA, A + (abase[i] + k0) * 2u);
            [unroll] for (uint j = 0; j < RN; j++)
                [unroll] for (uint q = 0; q < 4u; q++)
                    bt[j][q] = ld_f16x4(bufB, B + (bo + (col + cq + j * TN + q) * ldb + k0) * 2u);
            [unroll] for (uint kk = 0; kk < 4u; kk++) {
                float4 bv[RN];
                [unroll] for (uint j = 0; j < RN; j++)
                    bv[j] = float4(bt[j][0][kk], bt[j][1][kk], bt[j][2][kk], bt[j][3][kk]);
                [unroll] for (uint i = 0; i < RM; i++) {
                    float av = as4[i][kk];
                    [unroll] for (uint j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else if (transposed) {
        /* B is stored (N, K): a lane's four columns are four rows of it. */
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            [unroll] for (uint j = 0; j < RN; j++) {
                uint at = bo + (col + cq + j * TN) * ldb + k;
                bv[j] = float4(ld_f16(bufB, B, at), ld_f16(bufB, B, at + ldb),
                               ld_f16(bufB, B, at + 2u * ldb), ld_f16(bufB, B, at + 3u * ldb));
            }
            [unroll] for (uint i = 0; i < RM; i++) {
                float av = ld_f16(bufA, A, abase[i] + k);
                [unroll] for (uint j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    } else if (!transposed && packed_a) {
        /* the plain form with A fetched eight K terms at a time (0x8000000), a lane's four B
         * columns one eight-byte load as below; each K term still goes into the accumulator
         * alone and in order */
        for (uint k0 = 0; k0 < pc.k; k0 += 8u) {
            float4 as8[RM][2];
            [unroll] for (uint i = 0; i < RM; i++)
                ld_f16x8(bufA, A + (abase[i] + k0) * 2u, as8[i][0], as8[i][1]);
            [unroll] for (uint kk = 0; kk < 8u; kk++) {
                float4 bv[RN];
                [unroll] for (uint j = 0; j < RN; j++)
                    bv[j] = ld_f16x4(bufB, B + (bo + (k0 + kk) * ldb + col + cq + j * TN) * 2u);
                [unroll] for (uint i = 0; i < RM; i++) {
                    float av = as8[i][kk >> 2][kk & 3u];
                    [unroll] for (uint j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else if (wide_ab) {
        /* the plain form, a lane's A row four K terms at a time and its four B columns as
         * one load */
        for (uint k0 = 0; k0 < pc.k; k0 += 4u) {
            float4 as4[RM];
            [unroll] for (uint i = 0; i < RM; i++)
                as4[i] = ld_f16x4(bufA, A + (abase[i] + k0) * 2u);
            [unroll] for (uint kk = 0; kk < 4u; kk++) {
                float4 bv[RN];
                [unroll] for (uint j = 0; j < RN; j++)
                    bv[j] = ld_f16x4(bufB, B + (bo + (k0 + kk) * ldb + col + cq + j * TN) * 2u);
                [unroll] for (uint i = 0; i < RM; i++) {
                    float av = as4[i][kk];
                    [unroll] for (uint j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else {
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            [unroll] for (uint j = 0; j < RN; j++) {
                uint at = bo + k * ldb + col + cq + j * TN;
                bv[j] = float4(ld_f16(bufB, B, at), ld_f16(bufB, B, at + 1u),
                               ld_f16(bufB, B, at + 2u), ld_f16(bufB, B, at + 3u));
            }
            [unroll] for (uint i = 0; i < RM; i++) {
                float av = ld_f16(bufA, A, abase[i] + k);
                [unroll] for (uint j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    }

    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
#if RN == 2
    /* Bit 0x100000: Q and K normalised and V published here, straight into the three
     * (window, head, token, 32) targets. The rows have to be whole for the norm, so the
     * block goes through groupshared memory first. */
    if ((flags & 0x100000u) != 0u) {
        [unroll] for (uint i = 0; i < RM; i++)
            [unroll] for (uint j = 0; j < RN; j++)
                [unroll] for (uint q = 0; q < 4u; q++)
                    qkv_stage[(r + i * TM) * BN + cq + j * TN + q] = acc[i][j][q];
        GroupMemoryBarrierWithGroupSync();
        qkv_epilogue(row, col, lid, 32u);
        return;
    }
    /* Bit 0x800000 (with the window residual): block 0's output, read only pooled 2x2 and
     * published into the encoder or published as the last block's skip — both made here, as
     * gemm_portable.comp's 16x32 build makes them. The 16-row block is two rows of one 8x8
     * window across all 32 channels: each value goes out published as half into c and into
     * the stage as it is, and after the barrier its four pooled pixels are summed from the
     * stage in POOL2_SKIP's order, into the sixth operand. */
    if ((flags & 0x800000u) != 0u) {
        [unroll] for (uint i = 0; i < RM; i++)
            [unroll] for (uint j = 0; j < RN; j++) {
                uint at = co + (row + r + i * TM) * ldc + col + cq + j * TN;
                if (!residual_output_index(at)) continue;
                float4 published;
                [unroll] for (uint q = 0; q < 4u; q++) {
                    float v = add_gemm_residual(acc[i][j][q], at + q);
                    qkv_stage[(i * TM + r) * BN + j * TN + cq + q] = v;
                    published[q] = e4m3(v);
                }
                st_f16x4(bufC, C + at * 2u, published);
            }
        GroupMemoryBarrierWithGroupSync();
        uint pixel = lid / 8u, c = (lid % 8u) * 4u;
        uint window = row / 64u, pair = (row % 64u) / 8u;     // the block's first window row
        uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
        uint y = (window / pc.window_cols) * 8u + pair, x = (window % pc.window_cols) * 8u + 2u * pixel;
        if (y < top || x < left || y - top >= pc.image_h || x - left >= pc.image_w) return;
        uint at = (((y - top) / 2u) * (pc.image_w / 2u) + (x - left) / 2u) * BN + c;
        uint upper = (2u * pixel) * BN + c, lower = upper + 8u * BN;
        float4 pooled;
        [unroll] for (uint e = 0; e < 4u; e++) {
            precise float total = qkv_stage[upper + e] + qkv_stage[lower + e];
            total += qkv_stage[upper + BN + e];
            total += qkv_stage[lower + BN + e];
            pooled[e] = e4m3(total * 0.25);
        }
        st_f16x4(bufF, pc.of.x + at * 2u, pooled);
        return;
    }
#endif
#if RM == 1 && RN == 1
    /* Bit 0x10000: the 32->4 head still computes a legal 8x16 block, but only writes its
     * four useful columns (ldc = 4, no publish): the lanes holding columns 0..3. */
    if ((flags & 0x10000u) != 0u) {
        if (cq == 0u)
            [unroll] for (uint q = 0; q < 4u; q++)
                st_f32(bufC, C, co + (row + r) * ldc + q, acc[0][0][q]);
        return;
    }
#endif
    /* Bit 0x20000: the residual lands before the publish, exactly as the two-pass path
     * adds it before rounding; 0x80000 maps the window-ordered rows into the image and
     * skips the padding. Four consecutive elements a lane, which never cross a pixel. */
    if ((flags & 0x20000u) != 0u) {
        [unroll] for (uint i = 0; i < RM; i++)
            [unroll] for (uint j = 0; j < RN; j++) {
                uint at = co + (row + r + i * TM) * ldc + col + cq + j * TN;
                if (!residual_output_index(at)) continue;
                float4 v;
                [unroll] for (uint q = 0; q < 4u; q++)
                    v[q] = publish(epilogue, add_gemm_residual(acc[i][j][q], at + q));
                store4(C, at, v, narrow);
            }
        return;
    }
    [unroll] for (uint i = 0; i < RM; i++)
        [unroll] for (uint j = 0; j < RN; j++) {
            float4 v = acc[i][j];
            if (epilogue != 0u)
                [unroll] for (uint e = 0; e < 4u; e++)
                    v[e] = publish(epilogue, v[e]);
            uint at = co + (row + r + i * TM) * ldc + col + cq + j * TN;
            store4(C, at, v, narrow);
            /* Bit 0x200000: a published copy too, into `d`, as E4M3 halves — block 0's
             * stem, which its feed-forward's residual takes raw and its GEMM published.
             * Plain FP32 output only. */
            if ((flags & 0x200000u) != 0u) {
                uint D = pc.od.x;
                float4 p = float4(e4m3(v.x), e4m3(v.y), e4m3(v.z), e4m3(v.w));
                if ((at & 3u) == 0u && (D & 7u) == 0u)
                    st_f16x4(bufD, D + at * 2u, p);
                else
                    [unroll] for (uint e = 0; e < 4u; e++)
                        st_f16(bufD, D, at + e, p[e]);
            }
        }
}
