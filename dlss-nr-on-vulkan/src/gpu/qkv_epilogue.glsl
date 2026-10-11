/*
 * qkv_epilogue — the QKV projection finished where it is computed.
 *
 * Without it the projection goes to memory as float32, (window, token, 3, head, 32), and
 * three passes read it back: two cosine publishes for Q and K and a split for V, each
 * ending in E4M3 in (window, head, token, 32) order. At block 0 of a 720p frame that is
 * 377 MB written and read three times over, and 19 ms of the block. Here the workgroup
 * already holds a 32-column block of the output — exactly one head of one of Q, K or V,
 * since the column block is 32 wide and 32-aligned — so it can normalise and publish it
 * before anything leaves the chip.
 *
 * Bit-identical to the three passes by construction: the input is the same float32
 * accumulator they would have read back, the reciprocal norm is `cosine_tree.glsl`'s one
 * definition, and the per-element steps are cosine_publish's and the split's, in their
 * order. `src/gpu/test_gemm_qkv.py`.
 *
 * The includer provides `stage` (the raw accumulator block, row-major, BN wide) and its
 * element type as `QKV_STAGE`, BM and BN, `qkv_barrier()`, and a push block with `n`, `c`,
 * `d`, `residual_cos`, `qkv_scale`, `image_h` and `image_w`. The operands, which have no other use in this mode:
 *   c    Q target        d    K target        residual_cos    V target
 *   qkv_scale   the query's per-head scale (float32)
 *   image_h     tokens per window             image_w         heads
 * Flag 0x2000 is the ViT's normalisation (`vit_reciprocal`), whose query takes half(sqrt(32))
 * and then its learned scale, two half multiplies (notes/opendlss-reference.md).
 */

/* No shared memory of its own, and that is load-bearing. Workgroup shared memory is
 * allocated in powers of two on this hardware: the tiled block's `stage` is exactly 2 KB,
 * and a 64-byte array of reciprocals beside it made every tiled GEMM's workgroup take
 * 4 KB — halving how many fit on a core, for every tiled GEMM in the graph whether it used
 * this epilogue or not. That cost 23 ms of a 720p frame before it was found. So each row
 * is normalised in place, in `stage`, and the publish happens in the store loop. */
void qkv_epilogue(uint row, uint col, uint lane, uint lanes) {
    uint channels = pc.n / 3u, tokens = pc.image_h, heads = pc.image_w;
    uint part = col / channels, head = (col % channels) / 32u;
    /* Q and K: one invocation per row, as the row pass does — the reciprocal norm through
     * the fragment tree, then the row scaled by it (and Q by its head's scale). */
    if (part < 2u) {
        bool vit = (operation_flags() & 0x2000u) != 0u;
        float scale = part == 0u ? half_round(FloatBuf(pc.qkv_scale).v[head]) : 1.0;
        for (uint r = lane; r < BM; r += lanes) {
            float h[32];
            for (uint i = 0u; i < 32u; i++) h[i] = half_round(float(stage[r * BN + i]));
            float reciprocal = vit ? vit_reciprocal(h) : cosine_reciprocal(h);
            for (uint i = 0u; i < 32u; i++) {
                float value = hmul(h[i], reciprocal);
                if (part == 0u) value = hmul(vit ? hmul(value, VIT_ROOT) : value, scale);
                stage[r * BN + i] = QKV_STAGE(value);   // a half value: exact in either type
            }
        }
        qkv_barrier();
    }
    uint64_t target = part == 0u ? uint64_t(pc.c)
                    : (part == 1u ? uint64_t(pc.d) : pc.residual_cos);
    bool wide = (uint(target) & 7u) == 0u;
    /* Then every invocation publishes four consecutive elements of one row, so the stores
     * coalesce: a row is one head's 32 channels, contiguous in the target. */
    for (uint e = lane * 4u; e < BM * BN; e += lanes * 4u) {
        uint r = row + e / BN, window = r / tokens, token = r % tokens;
        if (r >= pc.m) continue;                 // a partial last block's rows past M
        uint at = ((window * heads + head) * tokens + token) * 32u + e % BN;
        vec4 out4;
        for (uint i = 0u; i < 4u; i++) out4[i] = e4m3(float(stage[e + i]));
        if (wide) {
            Half4Buf(target).v4[at >> 2] = f16vec4(out4);
        } else {
            for (uint i = 0u; i < 4u; i++) HalfBuf(target).v[at + i] = float16_t(out4[i]);
        }
    }
}
