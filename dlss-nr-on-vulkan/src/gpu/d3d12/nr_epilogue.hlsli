/*
 * nr_epilogue.hlsli — what the fused GEMM epilogues share, `residual_epilogue.glsl`,
 * `cosine_tree.glsl` and `qkv_epilogue.glsl` in HLSL, plus the half-operation helpers the
 * row pass (`attention.hlsl`) has always used. Included after `nr_d3d.hlsli` by the kernels
 * that finish a GEMM in place: `gemm_portable.hlsl`, `ffn_fused.hlsl`, `attention.hlsl`.
 *
 * The contract is bit-identity with the separate passes each epilogue replaces, on this
 * backend: the residual is added before the publish, exactly as the RESIDUAL kind of
 * `resident.hlsl` adds it (`a + b * d`, the same expression shape so the driver's
 * contraction decision is the same); the cosine normalise is the one fragment tree, each
 * step a single-rounded float32 expression then a half rounding.
 */
#ifndef NR_EPILOGUE_HLSLI
#define NR_EPILOGUE_HLSLI

/* -- cosine_tree.glsl ------------------------------------------------------ */

static const float COSINE_NORM_FLOOR = 0.00006198883056640625;

/* half(l*r), half(l+r), half(l*r + acc) — each a float32 expression with one rounding
 * at the end, matching `_half_multiply` / `_half_add` / `_half_fma`. `precise` stops an
 * FMA contraction, which would drop the float32 rounding between the multiply and the
 * add and give a different answer. */
float hmul(float l, float r) { precise float t = l * r; return half_round(t); }
float hadd(float l, float r) { precise float t = l + r; return half_round(t); }
float hfma(float l, float r, float acc) { precise float t = l * r + acc; return half_round(t); }

/* `h` is one head's 32 channels, already rounded to half. The vendor's fragment tree:
 * eight partial sums of four squares, then two butterfly levels, then the last add —
 * each step a half operation, in this order and no other. */
float cosine_reciprocal(float h[32]) {
    float partial[4][2];
    [unroll] for (uint lane = 0u; lane < 4u; lane++)
        [unroll] for (uint parity = 0u; parity < 2u; parity++) {
            uint ch = lane * 2u + parity;
            float first  = hfma(h[ch + 8u],  h[ch + 8u],  hmul(h[ch],       h[ch]));
            float second = hfma(h[ch + 24u], h[ch + 24u], hmul(h[ch + 16u], h[ch + 16u]));
            partial[lane][parity] = hadd(first, second);
        }
    float two[4][2], one[4][2];
    [unroll] for (uint lane = 0u; lane < 4u; lane++)
        [unroll] for (uint p = 0u; p < 2u; p++) two[lane][p] = hadd(partial[lane][p], partial[lane ^ 2u][p]);
    [unroll] for (uint lane = 0u; lane < 4u; lane++)
        [unroll] for (uint p = 0u; p < 2u; p++) one[lane][p] = hadd(two[lane][p], two[lane ^ 1u][p]);

    float norm = max(hadd(one[0][0], one[0][1]), half_round(COSINE_NORM_FLOOR));
    return half_round(rsqrt(norm));
}

/* The ViT's own (OpenDLSS-NR's `vit.wgsl`, notes/opendlss-reference.md): each pair square
 * summed in float32 — the low one exact, the high one rounded to half first — and rounded
 * once, then a stride 8, 4 and 2 tree of half adds, the last two pairs apart, and the
 * reciprocal square root as a divide. The floor keeps a zero head zero. */
float vit_reciprocal(float h[32]) {
    float r[16];
    [unroll] for (uint c = 0u; c < 16u; c++) r[c] = hfma(h[c], h[c], hmul(h[c + 16u], h[c + 16u]));
    [unroll] for (uint c1 = 0u; c1 < 8u; c1++) r[c1] = hadd(r[c1], r[c1 + 8u]);
    [unroll] for (uint c2 = 0u; c2 < 4u; c2++) r[c2] = hadd(r[c2], r[c2 + 4u]);
    float norm = max(hadd(hadd(r[0], r[2]), hadd(r[1], r[3])), half_round(COSINE_NORM_FLOOR));
    return half_round(1.0 / sqrt(norm));
}

/* half(sqrt(32)): the ViT's query takes it as a half multiply of its own, between the norm
 * and the learned scale. */
static const float VIT_ROOT = 5.65625;

#ifndef NR_D3D_DESC_PUSH
/* -- residual_epilogue.glsl -------------------------------------------------- */

/* Map the first of four consecutive channels into the unpadded image (bit 0x80000: the
 * rows are a window block's, padded, in (window, token) order). Channel counts and
 * four-element stores never cross a pixel boundary. Cropped window padding has no
 * destination and must not read the skip buffer. */
bool residual_output_index(inout uint index) {
    if ((operation_flags() & 0x80000u) == 0u) return true;
    uint pixel = index / pc.n, window = pixel / 64u, token = pixel % 64u;
    uint y = (window / pc.window_cols) * 8u + token / 8u;
    uint x = (window % pc.window_cols) * 8u + token % 8u;
    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    if (y < top || x < left || y - top >= pc.image_h || x - left >= pc.image_w)
        return false;
    index = ((y - top) * pc.image_w + x - left) * pc.n + index % pc.n;
    return true;
}

/* GEMM residual (bit 0x20000), before the publish: `branch + skip * cosine`, the skip in
 * `d` (half with bit 0x40000), the per-channel cosine in the fifth operand. The same
 * expression shape as the RESIDUAL kind of resident.hlsl, and no `precise`, as there. */
float add_gemm_residual(float branch, uint index) {
    if ((operation_flags() & 0x20000u) == 0u) return branch;
    float skip = (operation_flags() & 0x40000u) != 0u
        ? ld_f16(bufD, pc.od.x, index) : ld_f32(bufD, pc.od.x, index);
    return branch + skip * ld_f32(bufE, pc.oe.x, index % pc.n);
}

/* -- qkv_epilogue.glsl ------------------------------------------------------- */

/* The QKV projection finished where it is computed (bit 0x100000). The includer holds the
 * raw accumulator block in `qkv_stage` (groupshared float, row-major, QKV_BN wide, QKV_BM
 * rows) and defines QKV_BM / QKV_BN before including. The operands, which have no other
 * use in this mode: c the Q target, d the K target, the fifth operand the V target, the
 * sixth the query's per-head float32 scale; image_h = tokens per window, image_w = heads.
 * A column block is exactly one head of one of Q, K or V — 32 wide, 32-aligned. Q and K
 * rows are normalised in place (the row rounded to half, `cosine_reciprocal`, the row
 * scaled by it and Q by its head's scale), then every element is published as E4M3 into
 * (window, head, token, 32) order: what the float32 projection followed by two
 * `cosine_publish` and one `split_heads` write, bit for bit. */
#ifdef QKV_BN
void qkv_store4(RWByteAddressBuffer target, uint base, uint at, float4 out4) {
    /* the offset's low bits are the address's: a resource is 64 KB aligned */
    if ((base & 7u) == 0u) st_f16x4(target, base + at * 2u, out4);
    else [unroll] for (uint i = 0u; i < 4u; i++) st_f16(target, base, at + i, out4[i]);
}

void qkv_epilogue(uint row, uint col, uint lane, uint lanes) {
    uint channels = pc.n / 3u, tokens = pc.image_h, heads = pc.image_w;
    uint part = col / channels, head = (col % channels) / 32u;
    /* Q and K: one invocation per row, as the row pass does — the reciprocal norm through
     * the fragment tree, then the row scaled by it (and Q by its head's scale). */
    if (part < 2u) {
        /* flag 0x2000: the ViT's normalisation, its query taking half(sqrt(32)) and then
         * its learned scale, two half multiplies */
        bool vit = (operation_flags() & 0x2000u) != 0u;
        float scale = part == 0u ? half_round(ld_f32(bufF, pc.of.x, head)) : 1.0;
        for (uint r = lane; r < QKV_BM; r += lanes) {
            float h[32];
            [unroll] for (uint i = 0u; i < 32u; i++) h[i] = half_round(qkv_stage[r * QKV_BN + i]);
            float reciprocal = vit ? vit_reciprocal(h) : cosine_reciprocal(h);
            [unroll] for (uint i = 0u; i < 32u; i++) {
                float value = hmul(h[i], reciprocal);
                if (part == 0u) value = hmul(vit ? hmul(value, VIT_ROOT) : value, scale);
                qkv_stage[r * QKV_BN + i] = value;    // a half value: exact in float
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }
    /* Then every invocation publishes four consecutive elements of one row, so the stores
     * coalesce: a row is one head's 32 channels, contiguous in the target. */
    for (uint e = lane * 4u; e < QKV_BM * QKV_BN; e += lanes * 4u) {
        uint r = row + e / QKV_BN, window = r / tokens, token = r % tokens;
        if (r >= pc.m) continue;                 // a partial last block's rows past M
        uint at = ((window * heads + head) * tokens + token) * 32u + e % QKV_BN;
        float4 out4;
        [unroll] for (uint i = 0u; i < 4u; i++) out4[i] = e4m3(qkv_stage[e + i]);
        /* `part` is uniform over the group: one resource per branch, as HLSL wants it */
        if (part == 0u)      qkv_store4(bufC, pc.oc.x, at, out4);
        else if (part == 1u) qkv_store4(bufD, pc.od.x, at, out4);
        else                 qkv_store4(bufE, pc.oe.x, at, out4);
    }
}
#endif
#endif

#endif
