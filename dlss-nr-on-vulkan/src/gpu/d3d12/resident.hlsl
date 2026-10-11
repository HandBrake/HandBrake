/*
 * resident — the elementwise half of the graph, `resident.comp` in HLSL; the kinds, the
 * flag bits and the order of every sum are the GLSL's. One thread per element, 256 to a
 * group, `pc.spare` the first group of this piece of the dispatch.
 */
#include "nr_d3d.hlsli"

/* kinds; SCALE = 4u is the fall-through at the end */
static const uint E4M3 = 0u, GATE = 1u, HALF = 2u, TO_HALF = 3u,
                  RESIDUAL = 5u, FROM_HALF = 6u, PARTITION = 7u, REVERSE = 8u, ADD_BIAS = 9u,
                  SPLIT_HEADS = 10u, MERGE_HEADS = 11u, POOL2 = 12u, UPSAMPLE2 = 13u,
                  SCALE_CHANNEL = 14u, ADD = 15u, PAD_END = 16u,
                  /* fused: one read of float32 and one write of float16 where the graph
                   * otherwise makes three round trips over the same buffer */
                  GATE_E4M3_HALF = 17u, E4M3_HALF = 18u, GATE_HALF = 19u,
                  UPSAMPLE_MERGE = 20u, UPSAMPLE_ADD = 21u, POOL2_SKIP = 22u;

/* An operand held as float16 (bit 15 marks `a`, bit 16 marks `b`); a published value is
 * E4M3 and exact in half, so nothing is lost by the narrow read. */
float load_a(uint flags, uint index) {
    return (flags & 0x8000u) != 0u ? ld_f16(bufA, pc.oa.x, index) : ld_f32(bufA, pc.oa.x, index);
}
float load_b(uint flags, uint index) {
    return (flags & 0x10000u) != 0u ? ld_f16(bufB, pc.ob.x, index) : ld_f32(bufB, pc.ob.x, index);
}
float load_d(uint index) { return ld_f32(bufD, pc.od.x, index); }

/* The publish a pass ends with: bits 8-11 of `flags` pick the transform and bit 12
 * narrows the output to float16. */
void store(uint flags, uint index, float value) {
    uint epilogue = (flags >> 8) & 0xFu;
    if (epilogue == 2u || epilogue == 3u) value = gate_activation(value);
    if (epilogue == 1u || epilogue == 3u) value = e4m3(value);
    if (epilogue == 4u) value = half_round(value);
    if ((flags & 0x1000u) != 0u) st_f16(bufC, pc.oc.x, index, value);
    else                         st_f32(bufC, pc.oc.x, index, value);
}

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint index = (gid.x + pc.spare) * 256u + lid;
    if (index >= pc.m) return;
    uint flags = operation_flags();
    uint kind = flags & 0xFFu;

    if (kind == GATE_E4M3_HALF || kind == E4M3_HALF || kind == GATE_HALF) {
        float value = ld_f32(bufA, pc.oa.x, index);
        if (kind != E4M3_HALF) value = gate_activation(value);
        if (kind != GATE_HALF) value = e4m3(value);
        // an E4M3 value is exact in half, and the gate already returns a half value,
        // so the narrowing here loses nothing either way
        st_f16(bufC, pc.oc.x, index, value);
        return;
    }
    if (kind == TO_HALF) {
        st_f16(bufC, pc.oc.x, index, ld_f32(bufA, pc.oa.x, index) * pc.p0);
        return;
    }
    if (kind == FROM_HALF) {
        store(flags, index, ld_f16(bufA, pc.oa.x, index) * pc.p0);
        return;
    }
    if (kind == RESIDUAL) {
        /* out = branch + skip * cosine, the cosine one value per channel. Bit 14 makes
         * the branch read through the window reverse, so the attention output goes
         * straight into the residual instead of being copied into image order first. */
        uint at = index;
        if ((flags & 0x4000u) != 0u) {
            uint channels = pc.n, size = pc.batch, width = pc.sb, across = pc.sc;
            uint c = index % channels, rest = index / channels;
            uint py = rest / width + (pc.k >> 16), px = rest % width + (pc.k & 0xFFFFu);
            at = (((py / size) * across + px / size) * size * size
                  + (py % size) * size + px % size) * channels + c;
        }
        store(flags, index, load_a(flags, at) + load_b(flags, index) * load_d(index % pc.n));
        return;
    }
    if (kind == PARTITION || kind == REVERSE) {
        /* NHWC <-> (window, token, channel), with the shifted-window origin folded in
         * rather than materialised as a padded copy. `k` carries the pad as
         * (top << 16) | left; a window that reaches outside the image reads zero, which
         * is what `functional.pad` would have put there. */
        uint channels = pc.n, size = pc.batch;
        uint height = pc.sa, width = pc.sb, across = pc.sc;
        uint pad_top = pc.k >> 16, pad_left = pc.k & 0xFFFFu;
        uint c = index % channels, rest = index / channels;
        if (kind == PARTITION) {
            uint token = rest % (size * size), window = rest / (size * size);
            int y = int((window / across) * size + token / size) - int(pad_top);
            int x = int((window % across) * size + token % size) - int(pad_left);
            store(flags, index, (y < 0 || x < 0 || y >= int(height) || x >= int(width))
                ? 0.0 : load_a(flags, (uint(y) * width + uint(x)) * channels + c));
        } else {
            uint x = rest % width, y = rest / width;
            uint py = y + pad_top, px = x + pad_left;
            uint window = (py / size) * across + (px / size);
            uint token = (py % size) * size + (px % size);
            store(flags, index, load_a(flags, (window * size * size + token) * channels + c));
        }
        return;
    }
    if (kind == POOL2) {
        // 2x2 average, NHWC. n=channels, sb=source width.
        uint channels = pc.n, width = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % (width / 2u), y = rest / (width / 2u);
        uint base = ((2u * y) * width + 2u * x) * channels + c;
        /* The reference adds the four taps column-first — (top-left + bottom-left) +
         * top-right + bottom-right — and float32 addition is not associative, so the
         * order is part of the answer. `precise` keeps the compiler from regrouping. */
        precise float total = load_a(flags, base) + load_a(flags, base + width * channels);
        total += load_a(flags, base + channels);
        total += load_a(flags, base + (width + 1u) * channels);
        store(flags, index, total * 0.25);
        return;
    }
    if (kind == UPSAMPLE2) {
        // nearest 2x, cropped to the target extent. n=channels, sa=target width,
        // sb=source width.
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        store(flags, index, load_a(flags, ((y / 2u) * source + (x / 2u)) * channels + c));
        return;
    }
    if (kind == UPSAMPLE_MERGE) {
        /* Block 70's input in one pass (resident.comp's UPSAMPLE_MERGE): the level above,
         * upsampled 2x (nearest), times the per-channel sin, plus the full-resolution skip
         * times the per-channel cos; float32 to c, the same value published to E4M3 as
         * half to the second output (the fifth operand, push offset 96) — the float32 is
         * the feed-forward's residual, the publication its GEMM's input. The steps are the UPSAMPLE2,
         * SCALE_CHANNEL, RESIDUAL and TO_HALF passes', in their shapes: the scale rounded
         * on its own (`precise`), then `scaled + skip * cos` exactly as RESIDUAL writes
         * `a + b * d` here, so the driver treats the two alike.
         * n=channels, sa=target width, sb=source width; d holds sin then cos. */
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        precise float scaled = load_a(flags, ((y / 2u) * source + (x / 2u)) * channels + c)
                             * load_d(c);
        float merged = scaled + load_b(flags, index) * load_d(channels + c);
        st_f32(bufC, pc.oc.x, index, merged);
        st_f16(bufE, pc.oe.x, index, e4m3(merged));
        return;
    }
    if (kind == POOL2_SKIP) {
        /* Block 0's output into both its consumers in one read (resident.comp's
         * POOL2_SKIP): the 2x2 pool, published (POOL2 with the E4M3 epilogue), and every tap
         * published to half as the post block's skip (E4M3_HALF) into the second output
         * (the fifth operand). The pool adds its taps column-first (`precise`), the skip
         * publishes each tap alone. n=channels, sb=source width. */
        uint channels = pc.n, width = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % (width / 2u), y = rest / (width / 2u);
        uint base = ((2u * y) * width + 2u * x) * channels + c;
        uint taps[4] = { base, base + width * channels, base + channels,
                         base + (width + 1u) * channels };
        float value[4];
        [unroll] for (uint t = 0u; t < 4u; t++) value[t] = load_a(flags, taps[t]);
        precise float total = value[0] + value[1];
        total += value[2];
        total += value[3];
        store(flags, index, total * 0.25);
        [unroll] for (uint t2 = 0u; t2 < 4u; t2++)
            st_f16(bufE, pc.oe.x, taps[t2], e4m3(value[t2]));
        return;
    }
    if (kind == UPSAMPLE_ADD) {
        /* A decoder transition's merge in one pass (resident.comp's UPSAMPLE_ADD): the
         * projected level below upsampled 2x (nearest), plus the skip times the per-channel
         * sine, then the publish — upsample2, scale_channel and add, with their roundings:
         * the product rounded on its own, as scale_channel stored it, then added, neither
         * contracted (`precise`). n=channels, sa=target width, sb=source width, d=sine,
         * b=the skip. With a second output (bit 0x40000000, which libd3dmx sets when the
         * fifth operand is bound: HLSL has no null address to test) the merge goes there
         * published as well — the 32-channel block's, whose feed-forward reads it published
         * and its residual raw (notes/opendlss-reference.md). */
        uint channels = pc.n, target = pc.sa, source = pc.sb;
        uint c = index % channels, rest = index / channels;
        uint x = rest % target, y = rest / target;
        precise float scaled = load_b(flags, index) * load_d(c);
        precise float merged = load_a(flags, ((y / 2u) * source + (x / 2u)) * channels + c) + scaled;
        store(flags, index, merged);
        if ((flags & 0x40000000u) != 0u) st_f16(bufE, pc.oe.x, index, e4m3(merged));
        return;
    }
    if (kind == PAD_END) {
        // extend to a larger extent with zeros. n=channels, sa=height, sb=width,
        // sc=padded width.
        uint channels = pc.n, height = pc.sa, width = pc.sb, padded = pc.sc;
        uint c = index % channels, rest = index / channels;
        uint x = rest % padded, y = rest / padded;
        store(flags, index, (y < height && x < width)
            ? load_a(flags, (y * width + x) * channels + c) : 0.0);
        return;
    }
    if (kind == SCALE_CHANNEL) { store(flags, index, load_a(flags, index) * load_d(index % pc.n)); return; }
    if (kind == ADD)           { store(flags, index, load_a(flags, index) + load_b(flags, index)); return; }
    if (kind == SPLIT_HEADS) {
        // (windows, tokens, 3C) -> one of Q/K/V as (windows, heads, tokens, 32)
        uint channels = pc.n, heads = pc.batch, tokens = pc.sa, part = pc.sb;
        uint dd = index % 32u, rest = index / 32u;
        uint token = rest % tokens; rest /= tokens;
        uint head = rest % heads, window = rest / heads;
        store(flags, index, load_a(flags, (window * tokens + token) * 3u * channels
                                          + part * channels + head * 32u + dd));
        return;
    }
    if (kind == MERGE_HEADS) {
        // (windows, heads, tokens, 32) -> (windows, tokens, C)
        uint channels = pc.n, heads = pc.batch, tokens = pc.sa;
        uint c = index % channels, rest = index / channels;
        uint token = rest % tokens, window = rest / tokens;
        uint at = (window * heads + c / 32u) * tokens + token;
        float value = load_a(flags, at * 32u + c % 32u);
        /* Flag 0x20000: the ViT's value sum, rounded to half, times its row's reciprocal
         * from `d` — the softmax's normalisation, which the ViT applies here rather than
         * to the weights (attention.hlsl, vit_softmax). */
        if ((flags & 0x20000u) != 0u) {
            precise float scaled = half_round(value) * load_d(at);
            value = half_round(scaled);
        }
        store(flags, index, value);
        return;
    }
    if (kind == ADD_BIAS) {
        // scores are (windows*heads, tokens, tokens); the bias is (heads, tokens, tokens)
        uint plane = pc.n * pc.n, inside = index % plane;
        store(flags, index, load_a(flags, index) + load_d(((index / plane) % pc.batch) * plane + inside));
        return;
    }
    float value = load_a(flags, index) * pc.p0;
    if (kind == E4M3)      store(flags, index, e4m3(value));
    else if (kind == GATE) store(flags, index, gate_activation(value));
    else if (kind == HALF) store(flags, index, half_round(value));
    else                   store(flags, index, value);
}
