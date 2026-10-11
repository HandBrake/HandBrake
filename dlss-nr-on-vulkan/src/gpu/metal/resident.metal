/*
 * resident — the elementwise half of the graph, addressed by pointer. `resident.comp` in
 * MSL; the kinds, the flag bits and the order of every sum are the GLSL's.
 */
#include "nr_metal.h"

/* kinds; SCALE = 4u is the fall-through at the end */
constant uint E4M3 = 0u, GATE = 1u, HALF = 2u, TO_HALF = 3u,
              RESIDUAL = 5u, FROM_HALF = 6u, PARTITION = 7u, REVERSE = 8u, ADD_BIAS = 9u,
              SPLIT_HEADS = 10u, MERGE_HEADS = 11u, POOL2 = 12u, UPSAMPLE2 = 13u,
              SCALE_CHANNEL = 14u, ADD = 15u, PAD_END = 16u,
              GATE_E4M3_HALF = 17u, E4M3_HALF = 18u, GATE_HALF = 19u,
              UPSAMPLE_MERGE = 20u, UPSAMPLE_ADD = 21u, POOL2_SKIP = 22u;

/* An operand held as float16 (bit 15 marks `a`, bit 16 marks `b`); a published value is
 * E4M3 and exact in half, so nothing is lost by the narrow read. */
inline float load_a(constant Push &pc, uint flags, uint index) {
    return (flags & 0x8000u) != 0u ? float(half_ptr(pc.a)[index]) : float_ptr(pc.a)[index];
}
inline float load_b(constant Push &pc, uint flags, uint index) {
    return (flags & 0x10000u) != 0u ? float(half_ptr(pc.b)[index]) : float_ptr(pc.b)[index];
}
inline void store(constant Push &pc, uint flags, uint index, float value) {
    uint epilogue = (flags >> 8) & 0xFu;
    if (epilogue == 2u || epilogue == 3u) value = gate_activation(value);
    if (epilogue == 1u || epilogue == 3u) value = e4m3(value);
    if (epilogue == 4u) value = half_round(value);
    if ((flags & 0x1000u) != 0u) half_out(pc.c)[index] = half(value);
    else                         float_out(pc.c)[index] = value;
}

kernel void resident(constant Push &pc [[buffer(0)]],
                     uint index [[thread_position_in_grid]]) {
    if (index >= pc.m) return;
    uint flags = operation_flags(pc);
    uint kind = flags & 0xFFu;
    device const float *a = float_ptr(pc.a);
    device const float *d = float_ptr(pc.d);

    if (kind == GATE_E4M3_HALF || kind == E4M3_HALF || kind == GATE_HALF) {
        float value = a[index];
        if (kind != E4M3_HALF) value = gate_activation(value);
        if (kind != GATE_HALF) value = e4m3(value);
        half_out(pc.c)[index] = half(value);
        return;
    }
    if (kind == TO_HALF) {
        half_out(pc.c)[index] = half(a[index] * pc.p0);
        return;
    }
    if (kind == FROM_HALF) {
        store(pc, flags, index, float(half_ptr(pc.a)[index]) * pc.p0);
        return;
    }
    if (kind == RESIDUAL) {
        /* out = branch + skip * cosine, the cosine one value per channel. Bit 14 reads
         * the branch through the window reverse. */
        uint at = index;
        if ((flags & 0x4000u) != 0u) {
            uint channels = pc.n, size = pc.batch, width = pc.sb, across = pc.sc;
            uint c = index % channels, rest = index / channels;
            uint py = rest / width + (pc.k >> 16), px = rest % width + (pc.k & 0xFFFFu);
            at = (((py / size) * across + px / size) * size * size
                  + (py % size) * size + px % size) * channels + c;
        }
        store(pc, flags, index, load_a(pc, flags, at) + load_b(pc, flags, index) * d[index % pc.n]);
        return;
    }
    if (kind == PARTITION || kind == REVERSE) {
        /* NHWC <-> (window, token, channel) with the shifted-window origin folded in;
         * `k` carries the pad as (top << 16) | left, and outside the image reads zero. */
        uint channels = pc.n, size = pc.batch;
        uint height = pc.sa, width = pc.sb, across = pc.sc;
        uint pad_top = pc.k >> 16, pad_left = pc.k & 0xFFFFu;
        uint c = index % channels, rest = index / channels;
        if (kind == PARTITION) {
            uint token = rest % (size * size), window = rest / (size * size);
            int y = int((window / across) * size + token / size) - int(pad_top);
            int x = int((window % across) * size + token % size) - int(pad_left);
            store(pc, flags, index, (y < 0 || x < 0 || y >= int(height) || x >= int(width))
                ? 0.0f : load_a(pc, flags, (uint(y) * width + uint(x)) * channels + c));
        } else {
            uint x = rest % width, y = rest / width;
            uint py = y + pad_top, px = x + pad_left;
            uint window = (py / size) * across + (px / size);
            uint token = (py % size) * size + (px % size);
            store(pc, flags, index, load_a(pc, flags, (window * size * size + token) * channels + c));
        }
        return;
    }
    if (kind == POOL2) {
        // 2x2 average, NHWC; the reference adds column-first and float32 is not associative
        uint channels = pc.n, width = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % (width / 2u), y = rest / (width / 2u);
        uint base = ((2u * y) * width + 2u * x) * channels + c;
        float total = load_a(pc, flags, base) + load_a(pc, flags, base + width * channels);
        total += load_a(pc, flags, base + channels);
        total += load_a(pc, flags, base + (width + 1u) * channels);
        store(pc, flags, index, total * 0.25f);
        return;
    }
    if (kind == UPSAMPLE2) {
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        store(pc, flags, index, load_a(pc, flags, ((y / 2u) * source + (x / 2u)) * channels + c));
        return;
    }
    if (kind == UPSAMPLE_MERGE) {
        /* Block 70's input in one pass: the level above upsampled 2x (nearest), times the
         * per-channel sin, plus the skip times the per-channel cos, stored float32 and, as
         * the second output (offset 96), published to E4M3 as halves — the float32 is the
         * feed-forward's residual, the publication its GEMM's input. The steps are
         * upsample2, scale_channel, residual and e4m3_half, each rounded where they round:
         * the scale on its own, the residual in RESIDUAL's own `a + b * d` (nothing is
         * contracted in this build). n=channels, sa=target width, sb=source width; d holds
         * sin then cos. */
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        float scaled = load_a(pc, flags, ((y / 2u) * source + (x / 2u)) * channels + c) * d[c];
        float merged = scaled + load_b(pc, flags, index) * d[channels + c];
        float_out(pc.c)[index] = merged;
        half_out(pc.residual_cos)[index] = half(e4m3(merged));
        return;
    }
    if (kind == POOL2_SKIP) {
        /* Block 0's output into both its consumers in one read: the 2x2 pool, published
         * (POOL2 with the E4M3 epilogue), and every tap published to half as the post
         * block's skip (E4M3_HALF), the second output at offset 96. Their roundings are
         * theirs: the pool adds its taps column-first, the skip publishes each tap alone.
         * n=channels, sb=source width. */
        uint channels = pc.n, width = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % (width / 2u), y = rest / (width / 2u);
        uint base = ((2u * y) * width + 2u * x) * channels + c;
        uint taps[4] = { base, base + width * channels, base + channels, base + (width + 1u) * channels };
        float value[4];
        for (uint t = 0u; t < 4u; t++) value[t] = load_a(pc, flags, taps[t]);
        float total = value[0] + value[1];
        total += value[2];
        total += value[3];
        store(pc, flags, index, total * 0.25f);
        for (uint t = 0u; t < 4u; t++) half_out(pc.residual_cos)[taps[t]] = half(e4m3(value[t]));
        return;
    }
    if (kind == UPSAMPLE_ADD) {
        /* A decoder transition's merge in one pass: the projected level below upsampled 2x
         * (nearest), plus the skip times the per-channel sine, then the publish — what
         * upsample2, scale_channel and add wrote. The product is rounded on its own and then
         * added (nothing is contracted in this build). n=channels, sa=target width, sb=source
         * width, d=sine, b=the skip. With a second output (offset 96) the merge goes there
         * published as well: the 32-channel block's, whose feed-forward reads it published
         * and its residual raw (notes/opendlss-reference.md). */
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        float scaled = load_b(pc, flags, index) * d[c];
        float merged = load_a(pc, flags, ((y / 2u) * source + (x / 2u)) * channels + c) + scaled;
        store(pc, flags, index, merged);
        if (pc.residual_cos != 0ul) half_out(pc.residual_cos)[index] = half(e4m3(merged));
        return;
    }
    if (kind == PAD_END) {
        uint channels = pc.n, height = pc.sa, width = pc.sb, padded = pc.sc;
        uint c = index % channels, rest = index / channels;
        uint x = rest % padded, y = rest / padded;
        store(pc, flags, index, (y < height && x < width)
            ? load_a(pc, flags, (y * width + x) * channels + c) : 0.0f);
        return;
    }
    if (kind == SCALE_CHANNEL) { store(pc, flags, index, load_a(pc, flags, index) * d[index % pc.n]); return; }
    if (kind == ADD)           { store(pc, flags, index, load_a(pc, flags, index) + load_b(pc, flags, index)); return; }
    if (kind == SPLIT_HEADS) {
        // (windows, tokens, 3C) -> one of Q/K/V as (windows, heads, tokens, 32)
        uint channels = pc.n, heads = pc.batch, tokens = pc.sa, part = pc.sb;
        uint dd = index % 32u, rest = index / 32u;
        uint token = rest % tokens; rest /= tokens;
        uint head = rest % heads, window = rest / heads;
        store(pc, flags, index, load_a(pc, flags, (window * tokens + token) * 3u * channels
                                                  + part * channels + head * 32u + dd));
        return;
    }
    if (kind == MERGE_HEADS) {
        // (windows, heads, tokens, 32) -> (windows, tokens, C)
        uint channels = pc.n, heads = pc.batch, tokens = pc.sa;
        uint c = index % channels, rest = index / channels;
        uint token = rest % tokens, window = rest / tokens;
        uint at = (window * heads + c / 32u) * tokens + token;
        float value = load_a(pc, flags, at * 32u + c % 32u);
        /* Flag 0x20000: the ViT's value sum, rounded to half, times its row's reciprocal
         * from `d` — the softmax's normalisation, which the ViT applies here rather than
         * to the weights (attention.metal, vit_softmax). */
        if ((flags & 0x20000u) != 0u) value = half_round(half_round(value) * d[at]);
        store(pc, flags, index, value);
        return;
    }
    if (kind == ADD_BIAS) {
        uint plane = pc.n * pc.n, inside = index % plane;
        store(pc, flags, index, load_a(pc, flags, index) + d[((index / plane) % pc.batch) * plane + inside]);
        return;
    }
    float value = load_a(pc, flags, index) * pc.p0;
    if (kind == E4M3)      store(pc, flags, index, e4m3(value));
    else if (kind == GATE) store(pc, flags, index, gate_activation(value));
    else if (kind == HALF) store(pc, flags, index, half_round(value));
    else                   store(pc, flags, index, value);
}
