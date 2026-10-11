/*
 * gemm_portable — the GEMMs without a matrix unit, `gemm_portable.comp` and
 * `gemm_portable_desc.comp` in MSL: FP16 operands promoted to FP32 and accumulated with
 * ordinary multiply-adds, one lane owning four consecutive columns of one row of an 8x16
 * block per 32-lane threadgroup (16x32 for the tiled build). Same push block, flags,
 * strides, batch, epilogues and store as the simdgroup kernels, so the runtime dispatches
 * both with the same geometry; `XMX_PORTABLE=1` is how they are selected on a device that
 * has the matrix path.
 *
 * The fused epilogues are all here too (nr_epilogue.h): the residual, the window layout,
 * the QKV epilogue (16x32 build), the half copy, the compact head (8x16 build) and the
 * window-gathered A (16x32 build), so the graph a device without simdgroup matrices
 * records is the one the matrix path records.
 */
#include "nr_epilogue.h"

constant uint TM = 8, TN = 16;

/* A threadgroup is one 32-lane block, or two stacked along M where the device's SIMD group is
 * wider than 32 (AMD's 64: a lone 32-lane block leaves half of it idle). Each block computes
 * exactly what it would alone, the QKV epilogue and the pool in a stage of its own; the
 * runtime packs those two only where both blocks are whole, so both meet every barrier. */
template <int RM, int RN>
kernel void gemm_portable_t(constant Push &pc [[buffer(0)]],
                            uint3 wg [[threadgroup_position_in_grid]],
                            uint3 tpg [[threads_per_threadgroup]],
                            uint tid [[thread_index_in_threadgroup]]) {
    const uint BM = TM * RM, BN = TN * RN;
    /* only the 16x32 build's QKV epilogue and pool read it: a stage a block */
    threadgroup float stages[RN == 2 ? 2 * TM * RM * TN * RN : 1];
    threadgroup float *stage = stages + (RN == 2 ? (tid >> 5) * (TM * RM * TN * RN) : 0u);
    uint lid = tid & 31u;
    uint row = (wg.y * (tpg.x >> 5) + (tid >> 5)) * BM, col = wg.x * BN;
    if (row >= pc.m || col >= pc.n) return;
    uint flags = operation_flags(pc);
    uint batch = wg.z;
    uint ao = batch * pc.sa, bo = batch * pc.sb, co = batch * pc.sc;
    uint lda = pc.lda != 0u ? pc.lda : pc.k;
    uint ldb = pc.ldb != 0u ? pc.ldb : ((flags & 1u) != 0u ? pc.k : pc.n);
    uint ldc = pc.ldc != 0u ? pc.ldc : pc.n;
    device const half *A = half_ptr(pc.a);
    device const half *B = half_ptr(pc.b);

    uint r = lid >> 2u, cq = (lid & 3u) * 4u;

    float4 acc[RM][RN];
    for (int i = 0; i < RM; i++)
        for (int j = 0; j < RN; j++)
            acc[i][j] = float4(0.0f);

    if ((flags & 0x400000u) != 0u) {
        /* A gathered from the image in window order (the 16x32 build; the runtime routes
         * it here without the matrix path): zero outside the image, a float32 image
         * rounded to half on the way in, as the partition's narrow store did. */
        int base[RM];
        for (int i = 0; i < RM; i++) base[i] = window_row_base(pc, row + r + i * TM);
        bool wide_b = (ldb & 3u) == 0u && (pc.b & 7u) == 0u;
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            for (int j = 0; j < RN; j++) {
                uint at = k * ldb + col + cq + j * TN;
                bv[j] = wide_b ? float4(reinterpret_cast<device const half4 *>(B)[at >> 2])
                               : float4(float(B[at]), float(B[at + 1u]), float(B[at + 2u]), float(B[at + 3u]));
            }
            for (int i = 0; i < RM; i++) {
                float av = window_load(pc, flags, base[i], k);
                for (int j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    } else if (portable_packed && (flags & 1u) == 0u && (ldb & 3u) == 0u && (lda & 7u) == 0u
               && (pc.k & 7u) == 0u && (bo & 3u) == 0u && (ao & 7u) == 0u
               && (pc.a & 15u) == 0u && (pc.b & 7u) == 0u) {
        /* The plain form with A fetched sixteen bytes at a time — eight K terms, libxmx's
         * packed staged loader as this kernel can take it (PORTABLE_PACKED, nr_metal.h); a
         * lane's four B columns stay one eight-byte fetch. Each K term still goes into the
         * accumulator alone and in order, so the sums are the eight-byte path's bit for bit.
         * Off by default: on an M3 it was 13 % slower at 320x320, the compiler combining the
         * eight-byte fetches already (HANDOFF, 2026-10-10). */
        device const uint4 *A8 = reinterpret_cast<device const uint4 *>(A);
        device const half4 *B4 = reinterpret_cast<device const half4 *>(B);
        for (uint k0 = 0; k0 < pc.k; k0 += 8u) {
            float4 as8[RM][2];
            for (int i = 0; i < RM; i++) {
                uint4 w = A8[(ao + (row + r + i * TM) * lda + k0) >> 3];
                as8[i][0] = float4(as_type<half4>(w.xy));
                as8[i][1] = float4(as_type<half4>(w.zw));
            }
            for (uint kk = 0; kk < 8u; kk++) {
                float4 bv[RN];
                for (int j = 0; j < RN; j++)
                    bv[j] = float4(B4[(bo + (k0 + kk) * ldb + col + cq + j * TN) >> 2]);
                for (int i = 0; i < RM; i++) {
                    float av = as8[i][kk >> 2][kk & 3u];
                    for (int j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else if ((flags & 1u) != 0u && (ldb & 3u) == 0u && (lda & 3u) == 0u && (pc.k & 3u) == 0u
               && ((bo | ao) & 3u) == 0u && ((pc.a | pc.b) & 7u) == 0u) {
        /* The transposed form with the operands fetched eight bytes at a time along K:
         * the same terms added one at a time in the same order (gemm_portable.comp). */
        device const half4 *A4 = reinterpret_cast<device const half4 *>(A);
        device const half4 *B4 = reinterpret_cast<device const half4 *>(B);
        for (uint k0 = 0; k0 < pc.k; k0 += 4u) {
            float4 as4[RM], bt[RN][4];
            for (int i = 0; i < RM; i++)
                as4[i] = float4(A4[(ao + (row + r + i * TM) * lda + k0) >> 2]);
            for (int j = 0; j < RN; j++)
                for (uint q = 0; q < 4u; q++)
                    bt[j][q] = float4(B4[(bo + (col + cq + j * TN + q) * ldb + k0) >> 2]);
            for (uint kk = 0; kk < 4u; kk++) {
                float4 bv[RN];
                for (int j = 0; j < RN; j++)
                    bv[j] = float4(bt[j][0][kk], bt[j][1][kk], bt[j][2][kk], bt[j][3][kk]);
                for (int i = 0; i < RM; i++) {
                    float av = as4[i][kk];
                    for (int j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else if ((flags & 1u) != 0u) {
        /* B is stored (N, K): a lane's four columns are four rows of it. */
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            for (int j = 0; j < RN; j++) {
                uint at = bo + (col + cq + j * TN) * ldb + k;
                bv[j] = float4(float(B[at]), float(B[at + ldb]),
                               float(B[at + 2u * ldb]), float(B[at + 3u * ldb]));
            }
            for (int i = 0; i < RM; i++) {
                float av = float(A[ao + (row + r + i * TM) * lda + k]);
                for (int j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    } else if ((ldb & 3u) == 0u && (lda & 3u) == 0u && (pc.k & 3u) == 0u
               && ((bo | ao) & 3u) == 0u && ((pc.a | pc.b) & 7u) == 0u) {
        /* The same sums with the operands fetched eight bytes at a time: a lane's four B
         * columns are adjacent in a row-major B and its A row's next four K terms too.
         * Each K term still goes into the accumulator alone, in order. */
        device const half4 *A4 = reinterpret_cast<device const half4 *>(A);
        device const half4 *B4 = reinterpret_cast<device const half4 *>(B);
        for (uint k0 = 0; k0 < pc.k; k0 += 4u) {
            float4 as4[RM];
            for (int i = 0; i < RM; i++)
                as4[i] = float4(A4[(ao + (row + r + i * TM) * lda + k0) >> 2]);
            for (uint kk = 0; kk < 4u; kk++) {
                float4 bv[RN];
                for (int j = 0; j < RN; j++)
                    bv[j] = float4(B4[(bo + (k0 + kk) * ldb + col + cq + j * TN) >> 2]);
                for (int i = 0; i < RM; i++) {
                    float av = as4[i][kk];
                    for (int j = 0; j < RN; j++)
                        acc[i][j] += av * bv[j];
                }
            }
        }
    } else {
        for (uint k = 0; k < pc.k; k++) {
            float4 bv[RN];
            for (int j = 0; j < RN; j++) {
                uint at = bo + k * ldb + col + cq + j * TN;
                bv[j] = float4(float(B[at]), float(B[at + 1u]),
                               float(B[at + 2u]), float(B[at + 3u]));
            }
            for (int i = 0; i < RM; i++) {
                float av = float(A[ao + (row + r + i * TM) * lda + k]);
                for (int j = 0; j < RN; j++)
                    acc[i][j] += av * bv[j];
            }
        }
    }

    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;

    if (RN == 2 && (flags & 0x100000u) != 0u) {
        /* The QKV projection's epilogue: the raw block row-major on the stage, as the
         * matrix kernels stage it, then the same epilogue they finish through. */
        for (int i = 0; i < RM; i++)
            for (int j = 0; j < RN; j++)
                for (uint e = 0; e < 4u; e++)
                    stage[(i * TM + r) * BN + j * TN + cq + e] = acc[i][j][e];
        threadgroup_barrier(mem_flags::mem_threadgroup);
        qkv_epilogue<true>(pc, stage, BM, BN, row, col, lid, 32u);
        return;
    }
    if (RN == 2 && (flags & 0x800000u) != 0u) {
        /* Bit 0x800000 (with the window residual): block 0's output, read only pooled 2x2
         * and published, both made here as gemm_simd's staged kernel makes them. A 16-row
         * block is two rows of one 8x8 window across all 32 channels: each value goes out
         * published as half into `c` and into the stage as it is, and after the barrier the
         * block's four pooled pixels are summed from the stage in POOL2_SKIP's order into
         * `qkv_scale` (gemm_portable.comp). */
        for (int i = 0; i < RM; i++)
            for (int j = 0; j < RN; j++) {
                uint at = co + (row + r + i * TM) * ldc + col + cq + j * TN;
                if (!residual_output_index(pc, flags, at)) continue;
                float4 v = acc[i][j];
                half4 published;
                for (uint e = 0; e < 4u; e++) {
                    v[e] = publish(epilogue, add_gemm_residual(pc, flags, v[e], at + e));
                    stage[(i * TM + r) * BN + j * TN + cq + e] = v[e];
                    published[e] = half(e4m3(v[e]));
                }
                reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = published;
            }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        uint pixel = lid / 8u, c = (lid % 8u) * 4u;
        uint window = row / 64u, pair = (row % 64u) / 8u;     /* the block's first window row */
        uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
        uint y = (window / pc.window_cols) * 8u + pair, x = (window % pc.window_cols) * 8u + 2u * pixel;
        if (y < top || x < left || y - top >= pc.image_h || x - left >= pc.image_w) return;
        uint at = (((y - top) / 2u) * (pc.image_w / 2u) + (x - left) / 2u) * BN + c;
        uint upper = (2u * pixel) * BN + c, lower = upper + 8u * BN;
        half4 pooled;
        for (uint e = 0; e < 4u; e++) {
            float total = stage[upper + e] + stage[lower + e];
            total += stage[upper + BN + e];
            total += stage[lower + BN + e];
            pooled[e] = half(e4m3(total * 0.25f));
        }
        reinterpret_cast<device half4 *>(half_out(pc.qkv_scale))[at >> 2] = pooled;
        return;
    }
    if (RM == 1 && RN == 1 && (flags & 0x10000u) != 0u) {
        /* The 32->4 head: only the four useful columns, at a row stride of 4. */
        if (cq == 0u)
            for (uint e = 0; e < 4u; e++)
                float_out(pc.c)[co + (row + r) * ldc + e] = acc[0][0][e];
        return;
    }

    bool residual = (flags & 0x20000u) != 0u;
    bool half_copy = (flags & 0x200000u) != 0u;
    for (int i = 0; i < RM; i++)
        for (int j = 0; j < RN; j++) {
            float4 v = acc[i][j];
            uint at = co + (row + r + i * TM) * ldc + col + cq + j * TN;
            if (residual && !residual_output_index(pc, flags, at)) continue;
            for (int e = 0; e < 4; e++) {
                if (residual) v[e] = add_gemm_residual(pc, flags, v[e], at + e);
                if (epilogue != 0u) v[e] = publish(epilogue, v[e]);
            }
            if (narrow) {
                /* Four consecutive halves per lane, as one 64-bit store when aligned. */
                if ((at & 3u) == 0u && (pc.c & 7u) == 0u)
                    reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(v);
                else
                    for (int e = 0; e < 4; e++)
                        half_out(pc.c)[at + e] = half(v[e]);
            } else {
                if ((at & 3u) == 0u && (pc.c & 15u) == 0u)
                    reinterpret_cast<device float4 *>(float_out(pc.c))[at >> 2] = v;
                else
                    for (int e = 0; e < 4; e++)
                        float_out(pc.c)[at + e] = v[e];
            }
            /* the half copy, published: block 0's stem, raw for the residual, published
             * for the feed-forward's GEMM */
            if (half_copy) {
                float4 p = float4(e4m3(v[0]), e4m3(v[1]), e4m3(v[2]), e4m3(v[3]));
                if ((at & 3u) == 0u && (pc.d & 7u) == 0u)
                    reinterpret_cast<device half4 *>(half_out(pc.d))[at >> 2] = half4(p);
                else
                    for (int e = 0; e < 4; e++)
                        half_out(pc.d)[at + e] = half(p[e]);
            }
        }
}

template [[host_name("gemm_portable")]]       kernel void gemm_portable_t<1, 1>(constant Push &, uint3, uint3, uint);
template [[host_name("gemm_portable_tiled")]] kernel void gemm_portable_t<2, 2>(constant Push &, uint3, uint3, uint);
/* 16x64, for the GEMMs whose N is a multiple of 64 and whose store is the generic one */
template [[host_name("gemm_portable_wide")]]  kernel void gemm_portable_t<2, 4>(constant Push &, uint3, uint3, uint);

/* The descriptor-bound benchmark and test path: three buffers and a small push block,
 * `gemm_portable_desc.comp` (BATCHED for the second). The runtime always hands the seven
 * words; the plain kernel reads the first three. */
struct DescPush { uint M, N, K, sa, sb, sc, bt; };

template <bool BATCHED>
kernel void gemm_portable_desc_t(device const half *A [[buffer(0)]],
                                 device const half *B [[buffer(1)]],
                                 device float *C [[buffer(2)]],
                                 constant DescPush &pc [[buffer(3)]],
                                 uint3 wg [[threadgroup_position_in_grid]],
                                 uint lid [[thread_index_in_threadgroup]]) {
    uint row = wg.y * TM, col = wg.x * TN;
    if (row >= pc.M || col >= pc.N) return;
    uint ao = 0u, bo = 0u, co = 0u;
    bool transposed = false;
    if (BATCHED) {
        uint batch = wg.z;
        ao = batch * pc.sa; bo = batch * pc.sb; co = batch * pc.sc;
        transposed = pc.bt != 0u;
    }
    uint r = lid >> 2u, cq = (lid & 3u) * 4u;
    float4 acc = float4(0.0f);
    uint arow = ao + (row + r) * pc.K;
    if (transposed) {
        uint b0 = bo + (col + cq) * pc.K;
        for (uint k = 0; k < pc.K; k++) {
            float4 bv = float4(float(B[b0 + k]), float(B[b0 + pc.K + k]),
                               float(B[b0 + 2u * pc.K + k]), float(B[b0 + 3u * pc.K + k]));
            acc += float(A[arow + k]) * bv;
        }
    } else {
        for (uint k = 0; k < pc.K; k++) {
            uint at = bo + k * pc.N + col + cq;
            float4 bv = float4(float(B[at]), float(B[at + 1u]), float(B[at + 2u]), float(B[at + 3u]));
            acc += float(A[arow + k]) * bv;
        }
    }
    uint at = co + (row + r) * pc.N + col + cq;
    C[at] = acc.x; C[at + 1u] = acc.y; C[at + 2u] = acc.z; C[at + 3u] = acc.w;
}

template [[host_name("gemm_portable_desc")]]    kernel void gemm_portable_desc_t<false>(device const half *, device const half *, device float *, constant DescPush &, uint3, uint);
template [[host_name("gemm_portable_batched")]] kernel void gemm_portable_desc_t<true>(device const half *, device const half *, device float *, constant DescPush &, uint3, uint);
