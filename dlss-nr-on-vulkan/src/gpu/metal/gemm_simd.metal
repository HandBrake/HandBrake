/*
 * gemm_simd — the cooperative-matrix GEMMs on Apple's matrix path.
 *
 * Xe2 has `VK_KHR_cooperative_matrix` with one float shape, 8x16x16 fp16 -> fp32. Apple
 * silicon has `simdgroup_matrix`, one shape, 8x8, and `simdgroup_multiply_accumulate`
 * takes half operands into a float accumulator — the half x half product is exact in
 * float32, so the arithmetic claim is the one the XMX kernels make. An 8x16x16 tile is
 * therefore two 8x8 accumulators fed by two K slices: `TN / 8 * TK / 8` = four
 * multiply-accumulates where the GLSL has one `coopMatMulAdd`, and everything around
 * them — the flags, the strides, the batch on `threadgroup_position_in_grid.z`, the
 * epilogue and the narrow store through threadgroup memory (see the store below for why
 * not on the accumulator), the 8x16 / 16x32 / 64x32 block geometry — is `gemm_resident.comp`,
 * `gemm_staged.comp` and `gemm_coopmat*.comp` unchanged. The simdgroup is 32 wide, as
 * the subgroup is on Xe2; the runtime checks `threadExecutionWidth` and refuses otherwise.
 *
 * The transposed B (`flags & 1`, the key's own (N, K) layout) is the `transpose_matrix`
 * argument of `simdgroup_load`, where the SPIR-V uses a column-major `coopMatLoad`: no
 * transpose is copied on either.
 *
 * The fused epilogues (nr_epilogue.h) all go through the threadgroup stage: the residual
 * (0x20000, window layout 0x80000), the QKV projection's own epilogue (0x100000, a
 * 32-wide block, so the tiled and staged kernels), the half copy (0x200000), the compact
 * head (0x10000, the base kernel). A window-gathered A (0x400000) is staged through
 * threadgroup memory K step by K step, in the staged kernel's own operand tile and in the
 * tiled kernel's stage bytes, and loaded from there.
 */
#include "nr_epilogue.h"

constant uint TM = 8, TN = 16, TK = 16;

/* -- the Metal 4 build: the K loop as one Metal Performance Primitives matmul -------------
 *
 * libmetalmx carries this file twice: compiled as Metal 3.1 (the fallback, every device
 * and OS back to macOS 11) and, where the SDK has it, as Metal 4 with NR_METAL4, loaded on
 * a device and OS in MTLGPUFamilyMetal4 (XMX_METAL4=0 keeps the Metal 3 library). In the
 * Metal 4 build the GEMM kernels hand their K loop to `mpp::tensor_ops::matmul2d` on
 * tensors made in place from the operand addresses — its own operand pipeline, which on
 * an M3 runs the large shapes at up to twice the hand-written staged loop's speed — and
 * keep everything around it: the block geometry, the dispatch, the stage and every fused
 * epilogue. Measured on the M3, matmul2d in multiply mode with the K extent dynamic sums
 * each output in the order the loops below do, eight K at a time from zero, so the two
 * libraries give the same bytes and every fused kernel (which keeps its own simdgroup
 * loops) still equals its unfused passes. The window-gathered A (0x400000) has no tensor
 * form and keeps the hand-written loop. */
#ifdef NR_METAL4
#include <metal_tensor>
#include <MetalPerformancePrimitives/MetalPerformancePrimitives.h>

/* C[row.., col..] of the product over all of K into `dest`: a BM x BN block of a
 * (M x K) A with row stride lda and a B stored (K, N) with row stride ldb — or (N, K),
 * `TRANS`. SG simdgroups share the op. The extents are the whole matrices', so a block
 * past M or N reads nothing out of bounds, and the rows of `dest` past M are never read. */
template <int BM, int BN, int SG, bool TRANS, typename Dest>
inline void mpp_block(device const half *A, uint lda, device const half *B, uint ldb,
                      uint M, uint N, uint K, uint row, uint col, thread Dest &dest) {
    using namespace mpp::tensor_ops;
    constexpr auto desc = matmul2d_descriptor(BM, BN, static_cast<int>(dynamic_extent), false, TRANS, false);
    matmul2d<desc, execution_simdgroups<SG>> op;
    tensor<device half, dextents<int32_t, 2>, tensor_inline> tA(
        const_cast<device half *>(A), dextents<int32_t, 2>(K, M), array<int32_t, 2>({1, (int)lda}));
    auto mA = tA.slice(0, (int)row);
    if (TRANS) {
        tensor<device half, dextents<int32_t, 2>, tensor_inline> tB(
            const_cast<device half *>(B), dextents<int32_t, 2>(K, N), array<int32_t, 2>({1, (int)ldb}));
        auto mB = tB.slice(0, (int)col);
        op.run(mA, mB, dest);
    } else {
        tensor<device half, dextents<int32_t, 2>, tensor_inline> tB(
            const_cast<device half *>(B), dextents<int32_t, 2>(N, K), array<int32_t, 2>({1, (int)ldb}));
        auto mB = tB.slice((int)col, 0);
        op.run(mA, mB, dest);
    }
}

/* Into a BM x BN row-major float stage in threadgroup memory. */
template <int BM, int BN, int SG>
inline void mpp_to_stage(device const half *A, uint lda, device const half *B, uint ldb, bool transposed,
                         uint M, uint N, uint K, uint row, uint col, threadgroup float *stage) {
    tensor<threadgroup float, extents<int32_t, BN, BM>, tensor_inline> tS(
        stage, extents<int32_t, BN, BM>(), array<int32_t, 2>({1, BN}));
    if (transposed) mpp_block<BM, BN, SG, true>(A, lda, B, ldb, M, N, K, row, col, tS);
    else            mpp_block<BM, BN, SG, false>(A, lda, B, ldb, M, N, K, row, col, tS);
}

/* Straight into a float32 C with row stride ldc, for the plain store. */
template <int BM, int BN, int SG>
inline void mpp_to_device(device const half *A, uint lda, device const half *B, uint ldb, bool transposed,
                          uint M, uint N, uint K, uint row, uint col, device float *C, uint ldc) {
    tensor<device float, dextents<int32_t, 2>, tensor_inline> tC(
        C, dextents<int32_t, 2>(N, M), array<int32_t, 2>({1, (int)ldc}));
    auto mC = tC.slice((int)col, (int)row);
    if (transposed) mpp_block<BM, BN, SG, true>(A, lda, B, ldb, M, N, K, row, col, mC);
    else            mpp_block<BM, BN, SG, false>(A, lda, B, ldb, M, N, K, row, col, mC);
}
#endif

/* One 8x16x16 step on 8x8 fragments: acc[jj] += A[kk] * B[kk][jj]. */
template <typename ACC>
inline void mma_tile(thread simdgroup_matrix<ACC, 8, 8> *acc,        /* [2]: the two column halves */
                     thread const simdgroup_half8x8 *a,               /* [2]: the two K slices */
                     thread const simdgroup_half8x8 *b) {             /* [4]: [kk * 2 + jj] */
    for (int kk = 0; kk < 2; kk++)
        for (int jj = 0; jj < 2; jj++)
            simdgroup_multiply_accumulate(acc[jj], a[kk], b[kk * 2 + jj], acc[jj]);
}

inline void load_a_tile(thread simdgroup_half8x8 *a, device const half *A, uint at, uint lda) {
    for (int kk = 0; kk < 2; kk++) simdgroup_load(a[kk], A + at + kk * 8, lda);
}
/* B row-major (K, N) at `at`, or (N, K) at `at` transposed on the way in. */
inline void load_b_tile(thread simdgroup_half8x8 *b, device const half *B, uint at, uint ldb, bool transposed) {
    for (int kk = 0; kk < 2; kk++)
        for (int jj = 0; jj < 2; jj++) {
            if (transposed) simdgroup_load(b[kk * 2 + jj], B + at + (jj * 8) * ldb + kk * 8, ldb, ulong2(0, 0), true);
            else            simdgroup_load(b[kk * 2 + jj], B + at + (kk * 8) * ldb + jj * 8, ldb);
        }
}

/* -- the resident GEMM: 8x16 (RM = RN = 1) and 16x32 (RM = RN = 2) blocks ------------- */

template <int RM, int RN>
kernel void gemm_resident_t(constant Push &pc [[buffer(0)]],
                            uint3 wg [[threadgroup_position_in_grid]],
                            uint lid [[thread_index_in_threadgroup]]) {
    const uint BM = TM * RM, BN = TN * RN;
    threadgroup float stage[TM * RM * TN * RN];
    uint row = wg.y * BM, col = wg.x * BN;
    if (row >= pc.m || col >= pc.n) return;
    uint flags = operation_flags(pc);
    bool transposed = (flags & 1u) != 0u;
    bool window_a = (flags & 0x400000u) != 0u;
    uint batch = wg.z;
    uint ao = batch * pc.sa, bo = batch * pc.sb, co = batch * pc.sc;
    uint lda = pc.lda != 0u ? pc.lda : pc.k;
    uint ldb = pc.ldb != 0u ? pc.ldb : (transposed ? pc.k : pc.n);
    uint ldc = pc.ldc != 0u ? pc.ldc : pc.n;
    device const half *A = half_ptr(pc.a);
    device const half *B = half_ptr(pc.b);
    device float *C = float_out(pc.c);

#ifdef NR_METAL4
    if (!window_a) {
        uint epilogue4 = (flags >> 8) & 0xFu;
        bool narrow4 = (flags & 0x1000u) != 0u;
        if (epilogue4 == 0u && !narrow4 && (flags & 0x330000u) == 0u) {
            mpp_to_device<BM, BN, 1>(A + ao, lda, B + bo, ldb, transposed, pc.m, pc.n, pc.k,
                                     row, col, C + co, ldc);
            return;
        }
        mpp_to_stage<BM, BN, 1>(A + ao, lda, B + bo, ldb, transposed, pc.m, pc.n, pc.k, row, col, stage);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        if ((flags & 0x200000u) != 0u && epilogue4 == 0u && !narrow4 && (flags & 0x130000u) == 0u) {
            /* the plain float32 store and the half copy, both from the stage */
            for (uint e = lid; e < BM * BN; e += 32u)
                C[co + (row + e / BN) * ldc + col + e % BN] = stage[e];
            store_half_copy(pc, stage, BM, BN, row, col, co, ldc, lid, 32u);
            return;
        }
        if ((flags & 0x100000u) != 0u) {
            qkv_epilogue<false>(pc, stage, BM, BN, row, col, lid, 32u);
            return;
        }
        if ((flags & 0x10000u) != 0u) {
            uint e = lid;
            C[co + (row + e / 4u) * ldc + e % 4u] = stage[(e / 4u) * BN + e % 4u];
            return;
        }
        store_staged(pc, flags, stage, BM, BN, row, col, co, ldc, lid, 32u);
        return;
    }
#endif
    simdgroup_float8x8 acc[RM][RN][2];
    for (int i = 0; i < RM; i++)
        for (int j = 0; j < RN; j++)
            for (int jj = 0; jj < 2; jj++) acc[i][j][jj] = simdgroup_float8x8(0.0f);
    simdgroup_half8x8 a[RM][2];
    simdgroup_half8x8 b[RN][4];

    if (window_a) {
        /* A is the image itself, its window rows gathered K step by K step into the
         * stage's bytes — nothing else uses them until the K loop is done — and loaded
         * from there. One batch, B not transposed: the runtime checks. */
        threadgroup half *abuf = reinterpret_cast<threadgroup half *>(stage);
        const uint STRIDE = TK + 8u;
        for (uint k = 0; k < pc.k; k += TK) {
            simdgroup_barrier(mem_flags::mem_threadgroup);
            for (uint e = lid * 4u; e < BM * TK; e += 128u) {
                uint r = e / TK, c0 = e % TK;
                half4 v = window_load4(pc, flags, window_row_base(pc, row + r), k + c0);
                for (uint q = 0; q < 4u; q++) abuf[r * STRIDE + c0 + q] = v[q];
            }
            simdgroup_barrier(mem_flags::mem_threadgroup);
            for (int i = 0; i < RM; i++)
                for (int kk = 0; kk < 2; kk++)
                    simdgroup_load(a[i][kk], abuf + (i * TM) * STRIDE + kk * 8, STRIDE);
            for (int j = 0; j < RN; j++) load_b_tile(b[j], B, bo + k * ldb + col + j * TN, ldb, false);
            for (int i = 0; i < RM; i++)
                for (int j = 0; j < RN; j++) mma_tile(acc[i][j], a[i], b[j]);
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);   /* the stage is reused below */
    } else if (transposed) {
        /* The transposed test stays outside the K loop, as in the GLSL, so the loads hoist. */
        for (uint k = 0; k < pc.k; k += TK) {
            for (int i = 0; i < RM; i++) load_a_tile(a[i], A, ao + (row + i * TM) * lda + k, lda);
            for (int j = 0; j < RN; j++) load_b_tile(b[j], B, bo + (col + j * TN) * ldb + k, ldb, true);
            for (int i = 0; i < RM; i++)
                for (int j = 0; j < RN; j++) mma_tile(acc[i][j], a[i], b[j]);
        }
    } else {
        for (uint k = 0; k < pc.k; k += TK) {
            for (int i = 0; i < RM; i++) load_a_tile(a[i], A, ao + (row + i * TM) * lda + k, lda);
            for (int j = 0; j < RN; j++) load_b_tile(b[j], B, bo + k * ldb + col + j * TN, ldb, false);
            for (int i = 0; i < RM; i++)
                for (int j = 0; j < RN; j++) mma_tile(acc[i][j], a[i], b[j]);
        }
    }

    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    /* A plain float32 result goes straight from the accumulators. The compact head, the
     * residual and the QKV epilogue need each element's own address, so they take the
     * stage below even without a publish. */
    if (epilogue == 0u && !narrow && (flags & 0x130000u) == 0u) {
        for (int i = 0; i < RM; i++)
            for (int j = 0; j < RN; j++)
                for (int jj = 0; jj < 2; jj++)
                    simdgroup_store(acc[i][j][jj], C + co + (row + i * TM) * ldc + col + j * TN + jj * 8, ldc);
        if ((flags & 0x200000u) == 0u) return;
        /* Bit 0x200000: a half copy too, into `d`, for a value the graph needs both ways. */
        for (int i = 0; i < RM; i++)
            for (int j = 0; j < RN; j++)
                for (int jj = 0; jj < 2; jj++)
                    simdgroup_store(acc[i][j][jj], stage + i * TM * BN + j * TN + jj * 8, BN);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        store_half_copy(pc, stage, BM, BN, row, col, co, ldc, lid, 32u);
        return;
    }
    /* The publish runs on scalars, after an untouched store to threadgroup memory. The
     * GLSL applies it to the cooperative matrix's own components; the Metal equivalent,
     * `thread_elements()`, is a 64-wide vector whose hardware mapping the compiler hides,
     * and a per-element loop over it cost 19 ms a pass against 0.7 for the plain store —
     * 27x, and 80 % of a frame (notes/phase74). Through threadgroup memory the same pass
     * is 0.9 ms, and a narrow output gets its four-halves-a-lane store on the way. */
    for (int i = 0; i < RM; i++)
        for (int j = 0; j < RN; j++)
            for (int jj = 0; jj < 2; jj++)
                simdgroup_store(acc[i][j][jj], stage + i * TM * BN + j * TN + jj * 8, BN);
    simdgroup_barrier(mem_flags::mem_threadgroup);

    if ((flags & 0x100000u) != 0u) {
        /* Q and K normalised and V published here, straight into the three
         * (window, head, token, 32) targets; a 32-wide block, so RN == 2 only. */
        qkv_epilogue<false>(pc, stage, BM, BN, row, col, lid, 32u);
        return;
    }
    if ((flags & 0x10000u) != 0u) {
        /* The 32->4 head: a legal 8x16 tile of which only four columns are written.
         * The runtime keeps this to the base kernel, ldc = 4, plain float32. */
        uint e = lid;
        C[co + (row + e / 4u) * ldc + e % 4u] = stage[(e / 4u) * BN + e % 4u];
        return;
    }
    store_staged(pc, flags, stage, BM, BN, row, col, co, ldc, lid, 32u);
}

template [[host_name("gemm_resident")]] kernel void gemm_resident_t<1, 1>(constant Push &, uint3, uint);
template [[host_name("gemm_tiled")]]    kernel void gemm_resident_t<2, 2>(constant Push &, uint3, uint);

/* -- the staged GEMM: a 64x32 block, both operands through threadgroup memory ---------- */

/* BM: the rows of the block, 64 by default; the 32-row builds (`gemm_staged32`,
 * `gemm_staged32_deep`) take a bottleneck of 32 tokens or fewer, whose GEMMs wait on their
 * K loop. BK: the K step, 32, or 64 for the deep build. Every row's sums are its own and
 * the fragment loop runs K in the same ascending 8-wide slices either way, so a row comes
 * out of any build bit for bit the same (libxmx.c `small`, notes/improve-b.md). */
constant uint S_WARPS = 4;
constant uint S_BN = 32;

/* Bit 0x800000: block 0's window residual — whose only readers pool it 2x2 and publish
 * it — pooled and published here. A 64-row block is one 8x8 window across all 32
 * channels: each value goes out published as half into `c` (the post block's skip) and
 * back into the stage as it is, and after a barrier the window's sixteen pooled pixels
 * are summed from the stage in POOL2_SKIP's order into `qkv_scale` (gemm_staged.comp). */
inline void store_pooled(constant Push &pc, uint flags, threadgroup float *stage,
                         uint row, uint thread_id) {
    uint epilogue = (flags >> 8) & 0xFu;
    for (uint e = thread_id * 4u; e < 64u * S_BN; e += 128u * 4u) {
        uint at = (row + e / S_BN) * S_BN + e % S_BN;
        if (!residual_output_index(pc, flags, at)) continue;
        half4 published;
        for (uint q = 0; q < 4u; ++q) {
            float out = publish(epilogue, add_gemm_residual(pc, flags, stage[e + q], at + q));
            stage[e + q] = out;
            published[q] = half(e4m3(out));
        }
        reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = published;
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    uint window = row / 64u;
    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    for (uint e = thread_id * 4u; e < 16u * S_BN; e += 128u * 4u) {
        uint pixel = e / S_BN, c = e % S_BN;
        uint token = (pixel / 4u) * 16u + (pixel % 4u) * 2u;
        uint y = (window / pc.window_cols) * 8u + token / 8u;
        uint x = (window % pc.window_cols) * 8u + token % 8u;
        if (y < top || x < left || y - top >= pc.image_h || x - left >= pc.image_w) continue;
        uint at = (((y - top) / 2u) * (pc.image_w / 2u) + (x - left) / 2u) * S_BN + c;
        half4 pooled;
        for (uint i = 0; i < 4u; ++i) {
            float total = stage[token * S_BN + c + i] + stage[(token + 8u) * S_BN + c + i];
            total += stage[(token + 1u) * S_BN + c + i];
            total += stage[(token + 9u) * S_BN + c + i];
            pooled[i] = half(e4m3(total * 0.25f));
        }
        reinterpret_cast<device half4 *>(half_out(pc.qkv_scale))[at >> 2] = pooled;
    }
}

template <uint S_BM, uint S_BK>
kernel void gemm_staged_t(constant Push &pc [[buffer(0)]],
                          uint3 wg [[threadgroup_position_in_grid]],
                          uint thread_id [[thread_index_in_threadgroup]],
                          uint warp [[simdgroup_index_in_threadgroup]]) {
    const uint S_WM = S_BM / S_WARPS, S_WN = S_BN;
    const uint S_RM = S_WM / TM, S_RN = S_WN / TN;
    const uint S_SA = S_BK + 8, S_SB = S_BN + 8;     /* eight halves of pad against bank conflicts */
    const uint KC = S_BK / 32u;                       /* 32-column chunks of an A row per K step */
    /* 16-byte aligned: the packed loader below stores into them as uint4 */
    alignas(16) threadgroup half buf_a[S_BM * S_SA];
    alignas(16) threadgroup half buf_b[S_BK * S_SB];
    threadgroup float stage[S_BM * S_BN];

    uint row = wg.y * S_BM, col = wg.x * S_BN;
    /* The last block may be partial: its rows past M read the last real row, so nothing
     * is read out of bounds, and are never stored. A real row's arithmetic is the same. */
    bool partial = row + S_BM > pc.m;
    uint last = pc.m - 1u;
    uint flags = operation_flags(pc);
    uint batch = wg.z;
    uint ao = batch * pc.sa, bo = batch * pc.sb, co = batch * pc.sc;
    uint lda = pc.lda != 0u ? pc.lda : pc.k;
    bool transposed = (flags & 1u) != 0u;
    bool window_a = (flags & 0x400000u) != 0u;
    uint ldb = pc.ldb != 0u ? pc.ldb : (transposed ? pc.k : pc.n);
    uint ldc = pc.ldc != 0u ? pc.ldc : pc.n;
    device const half *A = half_ptr(pc.a);
    device const half *B = half_ptr(pc.b);
    device float *C = float_out(pc.c);

#ifdef NR_METAL4
    {
        uint epilogue4 = (flags >> 8) & 0xFu;
        bool narrow4 = (flags & 0x1000u) != 0u;
        if (window_a) {
            /* The window-gathered A has no tensor form: each K step's 64 rows are gathered
             * into the operand tile as before, and matmul2d in multiply-accumulate mode adds
             * that step's product into a cooperative tensor held in registers — measured to
             * continue the accumulation in the same order as one run over all of K. (Into a
             * threadgroup destination it does not: the stage's round trip regroups the sums,
             * and a few values an E4M3 quantum apart came out.) Never a transposed B. */
            using namespace mpp::tensor_ops;
            constexpr auto desc = matmul2d_descriptor(S_BM, S_BN, S_BK, false, false, false,
                                                      matmul2d_descriptor::mode::multiply_accumulate);
            matmul2d<desc, execution_simdgroups<S_WARPS>> op;
            tensor<threadgroup float, extents<int32_t, S_BN, S_BM>, tensor_inline> tS(
                stage, extents<int32_t, S_BN, S_BM>(), array<int32_t, 2>({1, (int)S_BN}));
            tensor<threadgroup half, extents<int32_t, S_BK, S_BM>, tensor_inline> tA(
                buf_a, extents<int32_t, S_BK, S_BM>(), array<int32_t, 2>({1, (int)S_SA}));
            tensor<device half, dextents<int32_t, 2>, tensor_inline> tB(
                const_cast<device half *>(B + bo), dextents<int32_t, 2>(pc.n, pc.k),
                array<int32_t, 2>({1, (int)ldb}));
            auto acc4 = op.template get_destination_cooperative_tensor<decltype(tA), decltype(tB), float>();
            for (uint16_t i = 0; i < acc4.get_capacity(); ++i)
                if (acc4.is_valid_element(i)) acc4[i] = 0.0f;
            uint gr = thread_id / 4u, gc = (thread_id % 4u) * 8u;
            /* this thread's rows' bases, found once rather than at every K step */
            const uint ROWS_PER = S_BM / ((32u * S_WARPS) / 4u);
            int bases[ROWS_PER];
            for (uint t = 0; t < ROWS_PER; t++) bases[t] = window_row_base(pc, row + gr + t * ((32u * S_WARPS) / 4u));
            /* Two operand tiles, the second in the stage's bytes (free until the result is
             * stored): the next step's rows are gathered while this step's product runs, one
             * barrier a step instead of two. The products and their order are unchanged. */
            threadgroup half *second = reinterpret_cast<threadgroup half *>(stage);
            tensor<threadgroup half, extents<int32_t, S_BK, S_BM>, tensor_inline> tA2(
                second, extents<int32_t, S_BK, S_BM>(), array<int32_t, 2>({1, (int)S_SA}));
            auto gather = [&](uint k0, threadgroup half *dst) {
                for (uint q = 0; q < KC; q++)
                    for (uint r = gr, t = 0; r < S_BM; r += (32u * S_WARPS) / 4u, t++) {
                        uint c0 = q * 32u + gc;
                        half4 lo = window_load4(pc, flags, bases[t], k0 + c0);
                        half4 hi = window_load4(pc, flags, bases[t], k0 + c0 + 4u);
                        for (uint e = 0; e < 4u; e++) dst[r * S_SA + c0 + e] = lo[e];
                        for (uint e = 0; e < 4u; e++) dst[r * S_SA + c0 + 4u + e] = hi[e];
                    }
            };
            gather(0u, buf_a);
            threadgroup_barrier(mem_flags::mem_threadgroup);
            uint step = 0u;
            for (uint k0 = 0; k0 < pc.k; k0 += S_BK, step ^= 1u) {
                if (k0 + S_BK < pc.k) gather(k0 + S_BK, step ? buf_a : second);
                auto mB = tB.template slice<S_BN, S_BK>((int)col, (int)k0);
                if (step) op.run(tA2, mB, acc4);
                else      op.run(tA, mB, acc4);
                threadgroup_barrier(mem_flags::mem_threadgroup);
            }
            acc4.store(tS);
        } else if (epilogue4 == 0u && !narrow4 && (flags & 0x920000u) == 0u) {
            mpp_to_device<S_BM, S_BN, S_WARPS>(A + ao, lda, B + bo, ldb, transposed, pc.m, pc.n, pc.k,
                                               row, col, C + co, ldc);
            return;
        } else {
            mpp_to_stage<S_BM, S_BN, S_WARPS>(A + ao, lda, B + bo, ldb, transposed, pc.m, pc.n, pc.k,
                                              row, col, stage);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if ((flags & 0x100000u) != 0u) {
            qkv_epilogue<true>(pc, stage, S_BM, S_BN, row, col, thread_id, 32u * S_WARPS);
            return;
        }
        if (S_BM == 64u && (flags & 0x800000u) != 0u) {
            store_pooled(pc, flags, stage, row, thread_id);
            return;
        }
        store_staged(pc, flags, stage, S_BM, S_BN, row, col, co, ldc, thread_id, 32u * S_WARPS, last);
        return;
    }
#endif
    /* Four threads to a row of the tile, eight halves each. */
    uint lr = thread_id / 4u, lc = (thread_id % 4u) * 8u;

    simdgroup_float8x8 acc[S_RM][S_RN][2];
    for (uint i = 0; i < S_RM; i++)
        for (uint j = 0; j < S_RN; j++)
            for (int jj = 0; jj < 2; jj++) acc[i][j][jj] = simdgroup_float8x8(0.0f);
    simdgroup_half8x8 fa[S_RM][2];
    simdgroup_half8x8 fb[S_RN][4];

    /* Eight halves per thread as two 64-bit loads when the row stride allows it. */
    device const half4 *a4 = reinterpret_cast<device const half4 *>(A);
    device const half4 *b4 = reinterpret_cast<device const half4 *>(B);
    bool wide_a = (pc.a & 7u) == 0u && (lda & 3u) == 0u && (ao & 3u) == 0u;
    bool wide_b = (pc.b & 7u) == 0u && (ldb & 3u) == 0u && (bo & 3u) == 0u;
    /* Or as one 128-bit load and one 128-bit threadgroup store, libxmx's packed staged loader
     * (gemm_staged.comp, STAGED_PACKED): the thread's eight halves are one uint4 in global
     * memory and one in the tile, since S_SA and S_SB are multiples of eight and the thread's
     * column is. The same bits in the same places, so bit-identical to the loops below. */
    device const uint4 *a8 = reinterpret_cast<device const uint4 *>(A);
    device const uint4 *b8 = reinterpret_cast<device const uint4 *>(B);
    threadgroup uint4 *packed_a = reinterpret_cast<threadgroup uint4 *>(buf_a);
    threadgroup uint4 *packed_b = reinterpret_cast<threadgroup uint4 *>(buf_b);
    bool packed_a_ok = (pc.a & 15u) == 0u && (lda & 7u) == 0u && (ao & 7u) == 0u;
    bool packed_b_ok = (pc.b & 15u) == 0u && (ldb & 7u) == 0u && (bo & 7u) == 0u;
    bool packed = staged_packed && !window_a && !transposed && packed_a_ok && packed_b_ok;
    /* a window-gathered A's row bases, found once rather than at every K step */
    const uint ROWS_PER = S_BM / ((32u * S_WARPS) / 4u);
    int bases[ROWS_PER];
    if (window_a)
        for (uint t = 0; t < ROWS_PER; t++) bases[t] = window_row_base(pc, row + lr + t * ((32u * S_WARPS) / 4u));

    for (uint k0 = 0; k0 < pc.k; k0 += S_BK) {
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (packed) {
            /* every global load of the step issued before any store, as the GLSL does */
            const uint STEP = (32u * S_WARPS) / 4u;
            const uint A_PER = S_BM / STEP, B_PER = S_BK / STEP;
            uint4 ra[A_PER * KC], rb[B_PER];
            for (uint p = 0; p < A_PER; p++)
                for (uint q = 0; q < KC; q++) {
                    uint at = ao + min(row + lr + p * STEP, last) * lda + k0 + q * 32u + lc;
                    ra[p * KC + q] = a8[at >> 3];
                }
            for (uint p = 0; p < B_PER; p++) {
                uint at = bo + (k0 + lr + p * STEP) * ldb + col + lc;
                rb[p] = b8[at >> 3];
            }
            for (uint p = 0; p < A_PER; p++)
                for (uint q = 0; q < KC; q++) {
                    uint r = lr + p * STEP, c = q * 32u + lc;
                    packed_a[(r * S_SA + c) >> 3] = ra[p * KC + q];
                }
            for (uint p = 0; p < B_PER; p++) {
                uint kk = lr + p * STEP;
                packed_b[(kk * S_SB + lc) >> 3] = rb[p];
            }
            threadgroup_barrier(mem_flags::mem_threadgroup);
        } else {
        for (uint q = 0; q < KC; q++)
        for (uint r = lr, t = 0; r < S_BM; r += (32u * S_WARPS) / 4u, t++) {
            uint c0 = q * 32u + lc;                  /* this chunk's columns */
            if (window_a) {
                /* Bit 0x400000: A gathered from the image in window order as it is
                 * loaded — the shifted-window partition done here rather than in a pass
                 * of its own; a token outside the image reads zero. */
                int base = bases[t];
                half4 lo = window_load4(pc, flags, base, k0 + c0), hi = window_load4(pc, flags, base, k0 + c0 + 4u);
                for (uint e = 0; e < 4u; e++) buf_a[r * S_SA + c0 + e] = lo[e];
                for (uint e = 0; e < 4u; e++) buf_a[r * S_SA + c0 + 4u + e] = hi[e];
                continue;
            }
            uint at = ao + min(row + r, last) * lda + k0 + c0;
            if (wide_a) {
                half4 lo = a4[at >> 2], hi = a4[(at >> 2) + 1u];
                for (uint e = 0; e < 4u; e++) buf_a[r * S_SA + c0 + e] = lo[e];
                for (uint e = 0; e < 4u; e++) buf_a[r * S_SA + c0 + 4u + e] = hi[e];
            } else {
                for (uint e = 0; e < 8u; e++) buf_a[r * S_SA + c0 + e] = A[at + e];
            }
        }
        if (transposed) {
            for (uint q = 0; q < KC; q++)
            for (uint nn = lr; nn < S_BN; nn += (32u * S_WARPS) / 4u) {
                uint c0 = q * 32u + lc;
                uint at = bo + (col + nn) * ldb + k0 + c0;
                if (wide_b) {
                    half4 lo = b4[at >> 2], hi = b4[(at >> 2) + 1u];
                    for (uint e = 0; e < 4u; e++) buf_b[(c0 + e) * S_SB + nn] = lo[e];
                    for (uint e = 0; e < 4u; e++) buf_b[(c0 + 4u + e) * S_SB + nn] = hi[e];
                } else {
                    for (uint e = 0; e < 8u; e++) buf_b[(c0 + e) * S_SB + nn] = B[at + e];
                }
            }
        } else {
            for (uint kk = lr; kk < S_BK; kk += (32u * S_WARPS) / 4u) {
                uint at = bo + (k0 + kk) * ldb + col + lc;
                if (wide_b) {
                    half4 lo = b4[at >> 2], hi = b4[(at >> 2) + 1u];
                    for (uint e = 0; e < 4u; e++) buf_b[kk * S_SB + lc + e] = lo[e];
                    for (uint e = 0; e < 4u; e++) buf_b[kk * S_SB + lc + 4u + e] = hi[e];
                } else {
                    for (uint e = 0; e < 8u; e++) buf_b[kk * S_SB + lc + e] = B[at + e];
                }
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        }   /* !packed */

        for (uint kk = 0; kk < S_BK; kk += TK) {
            for (uint i = 0; i < S_RM; i++)
                for (int s = 0; s < 2; s++)
                    simdgroup_load(fa[i][s], buf_a + (warp * S_WM + i * TM) * S_SA + kk + s * 8, S_SA);
            for (uint j = 0; j < S_RN; j++)
                for (int s = 0; s < 2; s++)
                    for (int jj = 0; jj < 2; jj++)
                        simdgroup_load(fb[j][s * 2 + jj], buf_b + (kk + s * 8) * S_SB + j * TN + jj * 8, S_SB);
            for (uint i = 0; i < S_RM; i++)
                for (uint j = 0; j < S_RN; j++) mma_tile(acc[i][j], fa[i], fb[j]);
        }
    }

    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    /* A fused residual, the QKV epilogue or the pool needs each element's own address, so
     * it never takes the raw store; it goes through the stage like a publish. */
    if (epilogue == 0u && !narrow && (flags & 0x920000u) == 0u && !partial) {
        for (uint i = 0; i < S_RM; i++)
            for (uint j = 0; j < S_RN; j++)
                for (int jj = 0; jj < 2; jj++)
                    simdgroup_store(acc[i][j][jj],
                                    C + co + (row + warp * S_WM + i * TM) * ldc + col + j * TN + jj * 8, ldc);
        return;
    }
    /* The publish on scalars after an untouched store to threadgroup memory: the whole
     * workgroup, not one simdgroup, then covers the tile four elements a lane. */
    for (uint i = 0; i < S_RM; i++)
        for (uint j = 0; j < S_RN; j++)
            for (int jj = 0; jj < 2; jj++)
                simdgroup_store(acc[i][j][jj], stage + (warp * S_WM + i * TM) * S_BN + j * TN + jj * 8, S_BN);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if ((flags & 0x100000u) != 0u) {
        qkv_epilogue<true>(pc, stage, S_BM, S_BN, row, col, thread_id, 32u * S_WARPS);
        return;
    }
    if (S_BM == 64u && (flags & 0x800000u) != 0u) {
        store_pooled(pc, flags, stage, row, thread_id);
        return;
    }
    store_staged(pc, flags, stage, S_BM, S_BN, row, col, co, ldc, thread_id, 32u * S_WARPS, last);
}

template [[host_name("gemm_staged")]]        kernel void gemm_staged_t<64, 32>(constant Push &, uint3, uint, uint);
template [[host_name("gemm_staged32")]]      kernel void gemm_staged_t<32, 32>(constant Push &, uint3, uint, uint);
template [[host_name("gemm_staged32_deep")]] kernel void gemm_staged_t<32, 64>(constant Push &, uint3, uint, uint);

/* -- the descriptor-bound GEMMs: gemm_coopmat, gemm_batched, gemm_f16acc --------------- */

struct DescPush { uint M, N, K, sa, sb, sc, bt; };

/* ACC float is configuration 1 (fp16 x fp16 -> fp32); ACC half is configuration 0, what
 * NVIDIA's own kernels accumulate in (`gemm_coopmat_f16acc.comp`). */
template <typename ACC, bool BATCHED>
kernel void gemm_desc_t(device const half *A [[buffer(0)]],
                        device const half *B [[buffer(1)]],
                        device ACC *C [[buffer(2)]],
                        constant DescPush &pc [[buffer(3)]],
                        uint3 wg [[threadgroup_position_in_grid]]) {
    uint row = wg.y * TM, col = wg.x * TN;
    if (row >= pc.M || col >= pc.N) return;
    uint ao = 0u, bo = 0u, co = 0u;
    bool transposed = false;
    if (BATCHED) {
        uint batch = wg.z;
        ao = batch * pc.sa; bo = batch * pc.sb; co = batch * pc.sc;
        transposed = pc.bt != 0u;
    }
    simdgroup_matrix<ACC, 8, 8> acc[2];
    acc[0] = simdgroup_matrix<ACC, 8, 8>(ACC(0)); acc[1] = simdgroup_matrix<ACC, 8, 8>(ACC(0));
    simdgroup_half8x8 a[2], b[4];
    if (transposed) {
        for (uint k = 0; k < pc.K; k += TK) {
            load_a_tile(a, A, ao + row * pc.K + k, pc.K);
            load_b_tile(b, B, bo + col * pc.K + k, pc.K, true);
            mma_tile(acc, a, b);
        }
    } else {
        for (uint k = 0; k < pc.K; k += TK) {
            load_a_tile(a, A, ao + row * pc.K + k, pc.K);
            load_b_tile(b, B, bo + k * pc.N + col, pc.N, false);
            mma_tile(acc, a, b);
        }
    }
    simdgroup_store(acc[0], C + co + row * pc.N + col, pc.N);
    simdgroup_store(acc[1], C + co + row * pc.N + col + 8, pc.N);
}

template [[host_name("gemm_coopmat")]] kernel void gemm_desc_t<float, false>(device const half *, device const half *, device float *, constant DescPush &, uint3);
template [[host_name("gemm_batched")]] kernel void gemm_desc_t<float, true>(device const half *, device const half *, device float *, constant DescPush &, uint3);
template [[host_name("gemm_f16acc")]]  kernel void gemm_desc_t<half, false>(device const half *, device const half *, device half *, constant DescPush &, uint3);
