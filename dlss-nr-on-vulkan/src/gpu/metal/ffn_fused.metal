/*
 * ffn_fused — a feed-forward in one pass, the hidden layer kept on chip:
 * `ffn_fused.comp` in MSL, simdgroup (`ffn_fused`) and portable (`ffn_fused_portable`).
 *
 * Bit-identical to this runtime's two GEMMs (the expand with the gate and E4M3 publish
 * into a half hidden buffer, then the projection with the residual in its epilogue, or
 * the branched blocks' two grouped GEMMs): each accumulator takes the same terms in the
 * same order the GEMM kernel of the same path gives it — ascending 8-wide K slices on
 * the simdgroup path (every simdgroup GEMM here, tiled or staged, goes up K that way),
 * one term at a time on the portable path — and the publishes and the residual are
 * nr_epilogue.h's.
 *
 * Push: a input (half, m x n), b expand (groups x n x k, group stride sa), qkv_scale slot
 * the projection (groups x k x 32, group stride sb), c output (row stride ldc, or 32;
 * group g at columns 32g), d skip and residual_cos cosine with 0x20000. Grid
 * (ceil(m/128), groups), 256 threads: eight 16-row blocks a threadgroup, one to each
 * simdgroup (on the portable kernel, each 32-thread slice). Flag 0x800000 — the narrow
 * blocks' shape, 32 channels and 128 hidden, set by libmetalmx — loads both 8 KB weight
 * matrices into threadgroup memory once for the eight blocks, `ffn_fused.comp`'s change,
 * rather than every 16 rows fetching them for itself. Other shapes read their weights
 * from device memory as before.
 *
 * Threadgroup memory: the simdgroup kernel 32 KB (16 KB of weights, then 2 KB a
 * simdgroup that holds its expand stage, the gated hidden chunk over the stage's first
 * kilobyte once every value is in registers, and at the end its output stage), the
 * portable one 24 KB (the weights, and a kilobyte of hidden chunk a slice; it stores its
 * output from registers). A surplus block past m leaves the simdgroup kernel after the
 * weights' barrier — everything after is a simdgroup barrier — and idles through the
 * portable kernel's threadgroup barriers.
 *
 * Eight blocks, not the matrix kernel's sixteen: Apple's threadgroup memory is 32 KB, and
 * a simdgroup_float8x8 cannot be published element by element without a trip through
 * threadgroup memory, so each simdgroup needs its 2 KB stage. Measured on an M3 at
 * 1280x768: 32.9 -> 30.7 ms a frame (portable 70.1 -> 56.2). Staging the expand and the
 * output 16 columns at a time, for 1 KB a simdgroup and 24 KB in all, was slower: 36.8.
 *
 * Block 70's input can be made here (flag 0x1000000, the staged shape with the residual):
 * the level above upsampled 2x times the sin plus the skip times the cos, UPSAMPLE_MERGE's
 * expression, which this runtime's reference stored as float32 for the residual and
 * published for the expand; and block 0's stem (0x2000000), the features times the adapter, as
 * the reference's 16x32 GEMM makes it (the simdgroup kernel's two 8-wide K slices from
 * zero, the portable kernel's sixteen multiply-adds). Each simdgroup (slice) makes its
 * sixteen rows once, published to E4M3 (every feed-forward GEMM reads its input
 * published, notes/opendlss-reference.md), into the A it reads for every chunk, and each
 * value again, raw, where its residual needs it. a is then the level above (half) or the features
 * (half, rows x 16), d the skip (half), the 64-bit address in p0-p1 the sin then cos or
 * the adapter (16 x 32), lda the width and ldb the level above's.
 */
#include "nr_epilogue.h"
#ifdef NR_METAL4
#include <metal_tensor>
#include <MetalPerformancePrimitives/MetalPerformancePrimitives.h>
#endif

constant uint F_ROWS = 16u, F_CHUNK = 32u, F_OUT = 32u, F_BLOCKS = 8u;
constant uint F_STAGED = 0x800000u;           /* one group of 32 x 128: weights shared */

/* Both staged weight matrices into threadgroup memory, 2 x 16 bytes of each a thread of
 * 256: the expand's 32 x 128 halves at 0, the projection's 128 x 32 at 4096. */
constant uint F_MERGE = 0x1000000u, F_STEM = 0x2000000u;

inline ulong ffn_aux(constant Push &pc) { return push_address(pc.p0); }

/* One channel of the merge at one pixel, as resident.metal's UPSAMPLE_MERGE makes it. */
inline float ffn_merged(constant Push &pc, uint pixel, uint c) {
    uint x = pixel % pc.lda, y = pixel / pc.lda;
    device const float *sincos = float_ptr(ffn_aux(pc));
    float scaled = float(half_ptr(pc.a)[((y / 2u) * pc.ldb + x / 2u) * 32u + c]) * sincos[c];
    float merged = scaled + float(half_ptr(pc.d)[pixel * 32u + c]) * sincos[32u + c];
    return merged;
}

/* Four channels of the stem at one pixel, as the portable 16x32 GEMM sums them. */
inline float4 ffn_stem4(constant Push &pc, uint pixel, uint c) {
    device const half *adapter = half_ptr(ffn_aux(pc));
    float4 acc = float4(0.0f);
    for (uint k = 0u; k < 16u; k++) {
        uint at = k * 32u + c;
        float4 bv = float4(float(adapter[at]), float(adapter[at + 1u]), float(adapter[at + 2u]), float(adapter[at + 3u]));
        float av = float(half_ptr(pc.a)[pixel * 16u + k]);
        acc += av * bv;
    }
    return acc;
}

/* The stem's 16 x 32 block at `row` as the simdgroup 16x32 GEMM makes it: each 8x8 tile
 * from zero, the K slices 0-7 then 8-15, into `stage` (row-major, 32 wide). */
inline void ffn_stem_tiles(constant Push &pc, uint row, threadgroup float *stage) {
    simdgroup_half8x8 feat[2][2], ad[2][4];
    for (uint i = 0u; i < 2u; i++)
        for (uint s = 0u; s < 2u; s++) simdgroup_load(feat[i][s], half_ptr(pc.a) + (row + i * 8u) * 16u + s * 8u, 16);
    for (uint s = 0u; s < 2u; s++)
        for (uint j = 0u; j < 4u; j++) simdgroup_load(ad[s][j], half_ptr(ffn_aux(pc)) + s * 8u * 32u + j * 8u, 32);
    for (uint i = 0u; i < 2u; i++)
        for (uint j = 0u; j < 4u; j++) {
            simdgroup_float8x8 acc = simdgroup_float8x8(0.0f);
            for (uint s = 0u; s < 2u; s++) simdgroup_multiply_accumulate(acc, feat[i][s], ad[s][j], acc);
            simdgroup_store(acc, stage + i * 8u * F_OUT + j * 8u, F_OUT);
        }
}

inline void ffn_load_weights(constant Push &pc, uint group, threadgroup uint4 *w, uint lid) {
    device const uint4 *E = reinterpret_cast<device const uint4 *>(half_ptr(pc.b) + group * pc.sa);
    device const uint4 *P = reinterpret_cast<device const uint4 *>(half_ptr(pc.qkv_scale) + group * pc.sb);
    for (uint t = 0u; t < 2u; t++) {
        w[lid + t * 256u] = E[lid + t * 256u];
        w[512u + lid + t * 256u] = P[lid + t * 256u];
    }
}

/* The output block, raw on `stage` (16 x 32, row-major): the residual and the publish per
 * element, the store four at a time. */
inline void ffn_store(constant Push &pc, uint flags, threadgroup const float *stage,
                      uint row, uint group, uint ldc, uint lid) {
    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    for (uint e = lid * 4u; e < F_ROWS * F_OUT; e += 128u) {
        uint at = (row + e / F_OUT) * ldc + group * F_OUT + e % F_OUT;
        float4 out4;
        for (uint q = 0u; q < 4u; q++) out4[q] = publish(epilogue, add_gemm_residual(pc, flags, stage[e + q], at + q));
        if (!narrow) {
            for (uint q = 0u; q < 4u; q++) float_out(pc.c)[at + q] = out4[q];
        } else if ((at & 3u) == 0u && (pc.c & 7u) == 0u) {
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(out4);
        } else {
            for (uint q = 0u; q < 4u; q++) half_out(pc.c)[at + q] = half(out4[q]);
        }
    }
}

kernel void ffn_fused(constant Push &pc [[buffer(0)]],
                      uint3 wg [[threadgroup_position_in_grid]],
                      uint lid [[thread_index_in_threadgroup]],
                      uint sg [[simdgroup_index_in_threadgroup]],
                      uint sl [[thread_index_in_simdgroup]]) {
    threadgroup float mem[8192];                  /* 32 KB, see the header */
    uint group = wg.y;
    uint flags = operation_flags(pc);
    bool staged = (flags & F_STAGED) != 0u;
    if (staged) ffn_load_weights(pc, group, reinterpret_cast<threadgroup uint4 *>(mem), lid);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    uint row = wg.x * (F_ROWS * F_BLOCKS) + sg * F_ROWS;
    if (row >= pc.m) return;                      /* only simdgroup barriers follow */
    uint cin = staged ? 32u : pc.n, width = staged ? 128u : pc.k;
    uint ldc = pc.ldc != 0u ? pc.ldc : F_OUT;
    device const half *A = half_ptr(pc.a);
    device const half *E = half_ptr(pc.b) + group * pc.sa;
    device const half *P = half_ptr(pc.qkv_scale) + group * pc.sb;
    threadgroup const half *SE = reinterpret_cast<threadgroup const half *>(mem);
    threadgroup const half *SP = SE + 4096u;
    threadgroup float *stage = mem + 4096u + sg * 512u;           /* 16 x 32 float */
    threadgroup half *hidden = reinterpret_cast<threadgroup half *>(stage);

    simdgroup_float8x8 result[2][4];
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 4; j++) result[i][j] = simdgroup_float8x8(0.0f);
    bool made = (flags & (F_MERGE | F_STEM)) != 0u;
#ifdef NR_METAL4
    if (staged) {
        /* The Metal 4 build: both products on matmul2d, which sums each output in the order
         * the simdgroup loops below do (eight K at a time from zero, gemm_simd.metal), and
         * the projection accumulating across the hidden chunks in a register tensor in
         * multiply-accumulate mode, which continues that order — so the same bits. The
         * expand's result is gated and published from registers straight into the hidden
         * chunk (the stage's first kilobyte); a made input waits in the second. */
        using namespace mpp::tensor_ops;
        constexpr auto ed = matmul2d_descriptor(16, 32, 32, false, false, false);
        matmul2d<ed, execution_simdgroups<1>> expand;
        constexpr auto pd = matmul2d_descriptor(16, 32, 32, false, false, false,
                                                matmul2d_descriptor::mode::multiply_accumulate);
        matmul2d<pd, execution_simdgroups<1>> project;
        threadgroup half *input = hidden + 512u;             /* the second kilobyte */
        if (made) {
            float values[F_ROWS * F_OUT / 32u];
            if ((flags & F_MERGE) != 0u) {
                for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) {
                    uint e = sl + t * 32u;
                    values[t] = ffn_merged(pc, row + e / F_OUT, e % F_OUT);
                }
            } else {
                ffn_stem_tiles(pc, row, stage);
                simdgroup_barrier(mem_flags::mem_threadgroup);
                for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) values[t] = stage[sl + t * 32u];
                simdgroup_barrier(mem_flags::mem_threadgroup);
            }
            for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) input[sl + t * 32u] = half(e4m3(values[t]));
            simdgroup_barrier(mem_flags::mem_threadgroup);
        }
        tensor<device half, extents<int32_t, 32, 16>, tensor_inline> tA(
            const_cast<device half *>(A + row * 32u), extents<int32_t, 32, 16>(), array<int32_t, 2>({1, 32}));
        tensor<threadgroup half, extents<int32_t, 32, 16>, tensor_inline> tM(
            input, extents<int32_t, 32, 16>(), array<int32_t, 2>({1, 32}));
        tensor<threadgroup half, extents<int32_t, 128, 32>, tensor_inline> tE(
            const_cast<threadgroup half *>(SE), extents<int32_t, 128, 32>(), array<int32_t, 2>({1, 128}));
        tensor<threadgroup half, extents<int32_t, 32, 128>, tensor_inline> tP(
            const_cast<threadgroup half *>(SP), extents<int32_t, 32, 128>(), array<int32_t, 2>({1, 32}));
        tensor<threadgroup float, extents<int32_t, 32, 16>, tensor_inline> tS(
            stage, extents<int32_t, 32, 16>(), array<int32_t, 2>({1, 32}));
        tensor<threadgroup half, extents<int32_t, 32, 16>, tensor_inline> tH(
            hidden, extents<int32_t, 32, 16>(), array<int32_t, 2>({1, 32}));
        auto mE0 = tE.template slice<32, 32>(0, 0);
        auto mP0 = tP.template slice<32, 32>(0, 0);
        auto acc = project.template get_destination_cooperative_tensor<decltype(tH), decltype(mP0), float>();
        auto gate = expand.template get_destination_cooperative_tensor<decltype(tA), decltype(mE0), float>();
        for (uint16_t i = 0; i < acc.get_capacity(); ++i)
            if (acc.is_valid_element(i)) acc[i] = 0.0f;
        for (uint chunk = 0u; chunk < 128u; chunk += F_CHUNK) {
            auto mE = tE.template slice<32, 32>((int)chunk, 0);
            if (made) expand.run(tM, mE, gate);
            else      expand.run(tA, mE, gate);
            for (uint16_t i = 0; i < gate.get_capacity(); ++i)
                if (gate.is_valid_element(i)) {
                    auto at = gate.get_multidimensional_index(i);
                    hidden[at[1] * 32 + at[0]] = half(publish(3u, gate[i]));
                }
            simdgroup_barrier(mem_flags::mem_threadgroup);
            auto mP = tP.template slice<32, 32>(0, (int)chunk);
            project.run(tH, mP, acc);
            simdgroup_barrier(mem_flags::mem_threadgroup);  /* the next chunk overwrites it */
        }
        acc.store(tS);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        if (!made) {
            ffn_store(pc, flags, stage, row, group, ldc, sl);
            return;
        }
    } else
#endif
    {
    /* an input made here: its sixteen rows, published to E4M3 as every feed-forward GEMM
     * reads its input, through the stage into the fragments every chunk reads them from */
    simdgroup_half8x8 held[4][2];
    if (made) {
        float values[F_ROWS * F_OUT / 32u];
        if ((flags & F_MERGE) != 0u) {
            for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) {
                uint e = sl + t * 32u;
                values[t] = ffn_merged(pc, row + e / F_OUT, e % F_OUT);
            }
        } else {
            ffn_stem_tiles(pc, row, stage);
            simdgroup_barrier(mem_flags::mem_threadgroup);
            for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) values[t] = stage[sl + t * 32u];
            simdgroup_barrier(mem_flags::mem_threadgroup);
        }
        for (uint t = 0u; t < F_ROWS * F_OUT / 32u; t++) hidden[sl + t * 32u] = half(e4m3(values[t]));
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint s = 0u; s < 4u; s++)
            for (uint i = 0u; i < 2u; i++) simdgroup_load(held[s][i], hidden + i * 8u * F_OUT + s * 8u, F_OUT);
        simdgroup_barrier(mem_flags::mem_threadgroup);   /* the first chunk overwrites it */
    }
    for (uint chunk = 0u; chunk < width; chunk += F_CHUNK) {
        simdgroup_float8x8 h[2][4];
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 4; j++) h[i][j] = simdgroup_float8x8(0.0f);
        for (uint kk = 0u; kk < cin; kk += 8u) {
            simdgroup_half8x8 x[2], w[4];
            for (uint i = 0u; i < 2u; i++) {
                if (made) x[i] = held[kk / 8u][i];
                else      simdgroup_load(x[i], A + (row + i * 8u) * cin + kk, cin);
            }
            for (uint j = 0u; j < 4u; j++) {
                if (staged) simdgroup_load(w[j], SE + kk * width + chunk + j * 8u, width);
                else        simdgroup_load(w[j], E + kk * width + chunk + j * 8u, width);
            }
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 4; j++) simdgroup_multiply_accumulate(h[i][j], x[i], w[j], h[i][j]);
        }
        for (uint i = 0u; i < 2u; i++)
            for (uint j = 0u; j < 4u; j++) simdgroup_store(h[i][j], stage + i * 8u * F_CHUNK + j * 8u, F_CHUNK);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        /* the gate and publish, every value into registers before the halves overwrite the
         * stage's first kilobyte */
        half gated[F_ROWS * F_CHUNK / 32u];
        for (uint t = 0u; t < F_ROWS * F_CHUNK / 32u; t++) gated[t] = half(publish(3u, stage[sl + t * 32u]));
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint t = 0u; t < F_ROWS * F_CHUNK / 32u; t++) hidden[sl + t * 32u] = gated[t];
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint kk = 0u; kk < F_CHUNK; kk += 8u) {
            simdgroup_half8x8 g[2], p[4];
            for (uint i = 0u; i < 2u; i++) simdgroup_load(g[i], hidden + i * 8u * F_CHUNK + kk, F_CHUNK);
            for (uint j = 0u; j < 4u; j++) {
                if (staged) simdgroup_load(p[j], SP + (chunk + kk) * F_OUT + j * 8u, F_OUT);
                else        simdgroup_load(p[j], P + (chunk + kk) * F_OUT + j * 8u, F_OUT);
            }
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 4; j++) simdgroup_multiply_accumulate(result[i][j], g[i], p[j], result[i][j]);
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);    /* the next chunk overwrites the stage */
    }
    for (uint i = 0u; i < 2u; i++)
        for (uint j = 0u; j < 4u; j++) simdgroup_store(result[i][j], stage + i * 8u * F_OUT + j * 8u, F_OUT);
    simdgroup_barrier(mem_flags::mem_threadgroup);
    if (!made) {
        ffn_store(pc, flags, stage, row, group, ldc, sl);
        return;
    }
    }
    /* the made input again as the residual's float32 skip: the result's sixteen values of
     * this lane into registers, then the stem through the same stage, or the merge made
     * again; the residual in add_gemm_residual's form */
    float branch[16], skip[16];
    for (uint t = 0u; t < 16u; t++) branch[t] = stage[sl * 4u + (t / 4u) * 128u + t % 4u];
    if ((flags & F_STEM) != 0u) {
        simdgroup_barrier(mem_flags::mem_threadgroup);
        ffn_stem_tiles(pc, row, stage);
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint t = 0u; t < 16u; t++) skip[t] = stage[sl * 4u + (t / 4u) * 128u + t % 4u];
    } else {
        for (uint t = 0u; t < 16u; t++) {
            uint e = sl * 4u + (t / 4u) * 128u + t % 4u;
            skip[t] = ffn_merged(pc, row + e / F_OUT, e % F_OUT);
        }
    }
    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    device const float *cosine = float_ptr(pc.residual_cos);
    for (uint n = 0u; n < 4u; n++) {
        uint e = sl * 4u + n * 128u;
        uint at = (row + e / F_OUT) * ldc + e % F_OUT;
        float4 out4;
        for (uint q = 0u; q < 4u; q++)
            out4[q] = publish(epilogue, branch[n * 4u + q] + skip[n * 4u + q] * cosine[e % F_OUT + q]);
        if (!narrow) {
            for (uint q = 0u; q < 4u; q++) float_out(pc.c)[at + q] = out4[q];
        } else if ((at & 3u) == 0u && (pc.c & 7u) == 0u) {
            reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(out4);
        } else {
            for (uint q = 0u; q < 4u; q++) half_out(pc.c)[at + q] = half(out4[q]);
        }
    }
}

kernel void ffn_fused_portable(constant Push &pc [[buffer(0)]],
                               uint3 wg [[threadgroup_position_in_grid]],
                               uint lid [[thread_index_in_threadgroup]]) {
    threadgroup uint weights[4096];               /* 16 KB: the staged shape's weights */
    threadgroup half hidden[F_BLOCKS * F_ROWS * F_CHUNK];   /* 8 KB: a chunk a slice */
    uint group = wg.y;
    uint flags = operation_flags(pc);
    bool staged = (flags & F_STAGED) != 0u;
    if (staged) ffn_load_weights(pc, group, reinterpret_cast<threadgroup uint4 *>(weights), lid);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    uint slice = lid >> 5u, lane = lid & 31u;
    uint row = wg.x * (F_ROWS * F_BLOCKS) + slice * F_ROWS;
    bool live_rows = row < pc.m;                  /* uniform across the slice only */
    uint cin = staged ? 32u : pc.n, width = staged ? 128u : pc.k;
    uint ldc = pc.ldc != 0u ? pc.ldc : F_OUT;
    device const half *A = half_ptr(pc.a);
    device const half *E = half_ptr(pc.b) + group * pc.sa;
    device const half *P = half_ptr(pc.qkv_scale) + group * pc.sb;
    threadgroup const half *SE = reinterpret_cast<threadgroup const half *>(weights);
    threadgroup const half *SP = SE + 4096u;
    threadgroup half *mine = hidden + slice * F_ROWS * F_CHUNK;
    /* the portable 16x32 GEMM's lane: rows r and r + 8, columns cq.. and 16 + cq.. */
    uint r = lane >> 2u, cq = (lane & 3u) * 4u;

    float4 result[2][2];
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) result[i][j] = float4(0.0f);
    /* an input made here: this lane's rows of A, published to E4M3, two to a word */
    bool made = (flags & (F_MERGE | F_STEM)) != 0u;
    uint held[2][16];
    if (made && live_rows) {
        for (uint i = 0u; i < 2u; i++)
            for (uint c = 0u; c < 32u; c += 4u) {
                float4 v;
                if ((flags & F_MERGE) != 0u)
                    for (uint q = 0u; q < 4u; q++) v[q] = ffn_merged(pc, row + r + i * 8u, c + q);
                else
                    v = ffn_stem4(pc, row + r + i * 8u, c);
                for (uint q = 0u; q < 4u; q++) v[q] = e4m3(v[q]);
                held[i][c / 2u] = as_type<uint>(half2(v.xy));
                held[i][c / 2u + 1u] = as_type<uint>(half2(v.zw));
            }
    }
    for (uint chunk = 0u; chunk < width; chunk += F_CHUNK) {
        if (live_rows) {
            float4 h[2][2];
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++) h[i][j] = float4(0.0f);
            for (uint k = 0u; k < cin; k++) {
                float4 bv[2];
                for (uint j = 0u; j < 2u; j++) {
                    uint at = k * width + chunk + cq + j * 16u;
                    bv[j] = staged ? float4(float(SE[at]), float(SE[at + 1u]), float(SE[at + 2u]), float(SE[at + 3u]))
                                   : float4(float(E[at]), float(E[at + 1u]), float(E[at + 2u]), float(E[at + 3u]));
                }
                for (uint i = 0u; i < 2u; i++) {
                    float av = made ? float(as_type<half2>(held[i][k >> 1u])[k & 1u])
                                    : float(A[(row + r + i * 8u) * cin + k]);
                    for (uint j = 0u; j < 2u; j++) h[i][j] += av * bv[j];
                }
            }
            for (uint i = 0u; i < 2u; i++)
                for (uint j = 0u; j < 2u; j++)
                    for (uint e = 0u; e < 4u; e++)
                        mine[(r + i * 8u) * F_CHUNK + cq + j * 16u + e] = half(publish(3u, h[i][j][e]));
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        if (live_rows) {
            for (uint k = 0u; k < F_CHUNK; k++) {
                float4 bv[2];
                for (uint j = 0u; j < 2u; j++) {
                    uint at = (chunk + k) * F_OUT + cq + j * 16u;
                    bv[j] = staged ? float4(float(SP[at]), float(SP[at + 1u]), float(SP[at + 2u]), float(SP[at + 3u]))
                                   : float4(float(P[at]), float(P[at + 1u]), float(P[at + 2u]), float(P[at + 3u]));
                }
                for (uint i = 0u; i < 2u; i++) {
                    float av = float(mine[(r + i * 8u) * F_CHUNK + k]);
                    for (uint j = 0u; j < 2u; j++) result[i][j] += av * bv[j];
                }
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);   /* the next chunk overwrites it */
    }
    if (!live_rows) return;
    uint epilogue = (flags >> 8) & 0xFu;
    bool narrow = (flags & 0x1000u) != 0u;
    for (uint i = 0u; i < 2u; i++)
        for (uint j = 0u; j < 2u; j++) {
            float4 v = result[i][j];
            uint at = (row + r + i * 8u) * ldc + group * F_OUT + cq + j * 16u;
            if (made) {
                /* the made input again as the float32 skip, add_gemm_residual's form */
                float4 skip;
                if ((flags & F_MERGE) != 0u)
                    for (uint q = 0u; q < 4u; q++) skip[q] = ffn_merged(pc, row + r + i * 8u, cq + j * 16u + q);
                else
                    skip = ffn_stem4(pc, row + r + i * 8u, cq + j * 16u);
                for (uint q = 0u; q < 4u; q++)
                    v[q] = publish(epilogue, v[q] + skip[q] * float_ptr(pc.residual_cos)[cq + j * 16u + q]);
            } else {
                for (uint q = 0u; q < 4u; q++) v[q] = publish(epilogue, add_gemm_residual(pc, flags, v[q], at + q));
            }
            if (!narrow) {
                for (uint q = 0u; q < 4u; q++) float_out(pc.c)[at + q] = v[q];
            } else if ((at & 3u) == 0u && (pc.c & 7u) == 0u) {
                reinterpret_cast<device half4 *>(half_out(pc.c))[at >> 2] = half4(v);
            } else {
                for (uint q = 0u; q < 4u; q++) half_out(pc.c)[at + q] = half(v[q]);
            }
        }
}
