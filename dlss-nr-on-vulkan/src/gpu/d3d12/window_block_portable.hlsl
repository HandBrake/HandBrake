/*
 * window_block_portable — `window_block_portable.comp` in HLSL: a 32-channel window
 * block's attention half, one 8x8 window a 256-lane group, eight 32-lane slices of one
 * window row each.
 *
 * It must equal this backend's three passes bit for bit: the QKV projection on the tiled
 * `gemm_portable.hlsl` with the window gather and the QKV epilogue, the merged
 * `window_attention_portable.hlsl`, and the output projection on the tiled
 * `gemm_portable.hlsl` with the window residual — for block 0 with the pooled epilogue
 * that build has, for block 70 followed by the head on the base `gemm_portable.hlsl`. So
 * every product is the portable GEMM's lane layout and order (a lane one row and four
 * consecutive columns of each 16-column tile, float4 accumulators from zero, `acc += a *
 * b`), the normalisation `cosine_reciprocal`, the softmax window_attention_portable.hlsl's
 * to the expression, and the residual `branch + skip * cosine` in add_gemm_residual's shape
 * (no `precise`, so the driver treats both alike). Not yet run on a Direct3D 12 device.
 *
 * Groupshared memory, 28.25 KB: the window's K and V as halves, two to a word (8 KB), then
 * each slice's 8 x 64 weights (8 KB); each slice's Q (4 KB); a kilobyte of floats a slice
 * for its projections, attention output and block output (8 KB); the row reciprocals take
 * K's first words once every slice is past QK^T, as in window_attention_portable.hlsl.
 *
 * Operands: u0 the image (float32, half with flag 0x8000), also the residual's skip; u1
 * the QKV weights (32 x 96); u2 the target; u3 the attention bias (64 x 64 float32); u4 the
 * residual's cosine; u5 the query's scale; u6 the output projection (32 x 32) at the byte
 * offset in p0; u7 block 0's pool (0x800000) or block 70's head weights (0x1000000, 32 x 16)
 * at the byte offset in p2. m the windows (x, then y past 65535), the geometry in image_h,
 * image_w, window_cols and window_pad; bits 8-12 the publish and a half target; 0x2000000
 * all sixteen head columns.
 */
#include "nr_d3d.hlsli"
#include "nr_epilogue.hlsli"

groupshared uint words[4096];            /* K at 0, V at 1024, weights at 2048 + 256 s */
groupshared uint qwords[1024];           /* slice s's Q at 128 s */
groupshared float region[2048];          /* slice s's floats at 256 s */
static const uint VALUES = 1024u, WEIGHTS = 2048u;

float lo(uint w) { return f16tof32(w & 0xFFFFu); }
float hi(uint w) { return f16tof32(w >> 16); }
float word_half(uint w, uint i) { return (i & 1u) ? hi(w) : lo(w); }
uint pack2(float a, float b) { return f32tof16(a) | (f32tof16(b) << 16); }

bool image_half() { return (operation_flags() & 0x8000u) != 0u; }
float image1(uint at) {
    return image_half() ? ld_f16(bufA, pc.oa.x, at) : ld_f32(bufA, pc.oa.x, at);
}

/* window_attention_portable.hlsl's weights, the bias from u3 */
void weights(float l0, float l1, uint bias_at, out float w0, out float w1) {
    float logits[2] = { l0, l1 };
    float affine[2];
    [unroll] for (uint j = 0u; j < 2u; j++) {
        float logit = logits[j];
        logit += ld_f32(bufD, pc.od.x, bias_at + j);
        precise float scaled = half_round(logit) * 0.044921875;
        scaled += 1.30078125;
        affine[j] = clamp(scaled, 1.03125, 1.5693359375);
    }
    uint transformed = (pack2(affine[0], affine[1]) << 5) + 0x7FF88000u;
    w0 = lo(transformed);
    w1 = hi(transformed);
}

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint index : SV_GroupIndex) {
    uint window = gid.x + gid.y * 65535u;
    if (window >= pc.m) return;                  // uniform across the group
    uint slice = index >> 5u, lane = index & 31u;
    uint r = lane >> 2u, cq = (lane & 3u) * 4u;
    uint mine = slice * 256u;                    // this slice's floats
    uint top = pc.window_pad >> 16, left = pc.window_pad & 0xffffu;
    int y = int((window / pc.window_cols) * 8u + slice) - int(top);
    int x = int((window % pc.window_cols) * 8u + r) - int(left);
    bool inside = y >= 0 && y < int(pc.image_h) && x >= 0 && x < int(pc.image_w);
    uint pixel = inside ? (uint(y) * pc.image_w + uint(x)) * 32u : 0u;
    uint Bq = pc.ob.x, G = asuint(pc.p0), H = asuint(pc.p2);

    /* Q, K and V as the tiled portable GEMM computes the gathered projection */
    float4 acc[6];
    [unroll] for (uint j0 = 0u; j0 < 6u; j0++) acc[j0] = float4(0.0, 0.0, 0.0, 0.0);
    for (uint k = 0u; k < 32u; k++) {
        float4 bv[6];
        [unroll] for (uint j = 0u; j < 6u; j++) {
            uint at = k * 96u + cq + j * 16u;
            bv[j] = float4(ld_f16(bufB, Bq, at), ld_f16(bufB, Bq, at + 1u),
                           ld_f16(bufB, Bq, at + 2u), ld_f16(bufB, Bq, at + 3u));
        }
        /* published to E4M3 on the way in, as the gathered projection's 0x4000 does: the
         * projection reads the feed-forward output published, the residual raw */
        float av = !inside ? 0.0 : e4m3(image_half() ? ld_f16(bufA, pc.oa.x, pixel + k)
                                                     : ld_f32(bufA, pc.oa.x, pixel + k));
        [unroll] for (uint j1 = 0u; j1 < 6u; j1++)
            acc[j1] += av * bv[j1];
    }
    /* the QKV epilogue a part at a time through the slice's floats */
    float scale = half_round(ld_f32(bufF, pc.of.x, 0u));
    [unroll] for (uint part = 0u; part < 3u; part++) {
        [unroll] for (uint j2 = 0u; j2 < 2u; j2++)
            [unroll] for (uint e = 0u; e < 4u; e++)
                region[mine + r * 32u + cq + j2 * 16u + e] = acc[part * 2u + j2][e];
        GroupMemoryBarrierWithGroupSync();
        if (part < 2u && lane < 8u) {
            float h[32];
            [unroll] for (uint i = 0u; i < 32u; i++) h[i] = half_round(region[mine + lane * 32u + i]);
            float norm = cosine_reciprocal(h);
            [unroll] for (uint i2 = 0u; i2 < 32u; i2++) {
                float value = hmul(h[i2], norm);
                if (part == 0u) value = hmul(value, scale);
                region[mine + lane * 32u + i2] = value;
            }
        }
        GroupMemoryBarrierWithGroupSync();
        for (uint w = lane; w < 128u; w += 32u) {
            uint two = pack2(e4m3(region[mine + 2u * w]), e4m3(region[mine + 2u * w + 1u]));
            if (part == 0u)      qwords[slice * 128u + w] = two;
            else if (part == 1u) words[slice * 128u + w] = two;
            else                 words[VALUES + slice * 128u + w] = two;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    /* window_attention_portable.hlsl for this slice's eight rows, one head */
    uint wmine = WEIGHTS + slice * 256u;
    {
        float4 sc[4];
        [unroll] for (uint j3 = 0u; j3 < 4u; j3++) sc[j3] = float4(0.0, 0.0, 0.0, 0.0);
        for (uint c = 0u; c < 32u; c++) {
            float4 kv[4];
            [unroll] for (uint j = 0u; j < 4u; j++) {
                uint at = (cq + j * 16u) * 32u + c;
                kv[j] = float4(word_half(words[at >> 1u], at), word_half(words[(at + 32u) >> 1u], at),
                               word_half(words[(at + 64u) >> 1u], at), word_half(words[(at + 96u) >> 1u], at));
            }
            uint qi = r * 32u + c;
            float qv = word_half(qwords[slice * 128u + (qi >> 1u)], qi);
            [unroll] for (uint j4 = 0u; j4 < 4u; j4++)
                sc[j4] += qv * kv[j4];
        }
        [unroll] for (uint j5 = 0u; j5 < 4u; j5++)
            [unroll] for (uint e = 0u; e < 4u; e += 2u) {
                uint key = cq + j5 * 16u + e;
                float w0, w1;
                weights(sc[j5][e], sc[j5][e + 1u], (slice * 8u + r) * 64u + key, w0, w1);
                words[wmine + (r * 64u + key) / 2u] = pack2(w0, w1);
            }
    }
    GroupMemoryBarrierWithGroupSync();
    if (lane < 8u) {
        float total = 0.0;
        for (uint i = 0u; i < 64u; i += 2u) {
            uint two = words[wmine + (lane * 64u + i) / 2u];
            total += lo(two);
            total += hi(two);
        }
        precise float reciprocal = 1.0 / half_round(total);
        words[slice * 8u + lane] = asuint(half_round(reciprocal));
    }
    GroupMemoryBarrierWithGroupSync();
    for (uint p = lane * 2u; p < 8u * 64u; p += 64u) {
        float s = asfloat(words[slice * 8u + p / 64u]);
        uint pair = words[wmine + p / 2u];
        precise float first = lo(pair) * s;
        precise float second = hi(pair) * s;
        words[wmine + p / 2u] = pack2(e4m3(half_round(first)), e4m3(half_round(second)));
    }
    GroupMemoryBarrierWithGroupSync();
    float4 ctx[2];
    [unroll] for (uint j6 = 0u; j6 < 2u; j6++) ctx[j6] = float4(0.0, 0.0, 0.0, 0.0);
    for (uint key = 0u; key < 64u; key++) {
        float4 vv[2];
        [unroll] for (uint j = 0u; j < 2u; j++) {
            uint at = VALUES + (key * 32u + cq + j * 16u) / 2u;
            uint w0 = words[at], w1 = words[at + 1u];
            vv[j] = float4(lo(w0), hi(w0), lo(w1), hi(w1));
        }
        uint pi = r * 64u + key;
        float pk = word_half(words[wmine + (pi >> 1u)], pi);
        [unroll] for (uint j7 = 0u; j7 < 2u; j7++)
            ctx[j7] += pk * vv[j7];
    }
    /* the merged store's publish, into the slice's floats: the projection's A */
    [unroll] for (uint j8 = 0u; j8 < 2u; j8++)
        [unroll] for (uint e = 0u; e < 4u; e++)
            region[mine + r * 32u + cq + j8 * 16u + e] = e4m3(ctx[j8][e]);
    GroupMemoryBarrierWithGroupSync();

    /* the output projection as the tiled portable GEMM computes it */
    float4 projected[2];
    [unroll] for (uint j9 = 0u; j9 < 2u; j9++) projected[j9] = float4(0.0, 0.0, 0.0, 0.0);
    for (uint k2 = 0u; k2 < 32u; k2++) {
        float4 bv[2];
        [unroll] for (uint j = 0u; j < 2u; j++) {
            uint at = k2 * 32u + cq + j * 16u;
            bv[j] = float4(ld_f16(bufG, G, at), ld_f16(bufG, G, at + 1u),
                           ld_f16(bufG, G, at + 2u), ld_f16(bufG, G, at + 3u));
        }
        float av = region[mine + r * 32u + k2];
        [unroll] for (uint ja = 0u; ja < 2u; ja++)
            projected[ja] += av * bv[ja];
    }
    GroupMemoryBarrierWithGroupSync();           // the attention output read by every lane

    uint epilogue = (operation_flags() >> 8) & 0xFu;
    bool narrow = (operation_flags() & 0x1000u) != 0u;
    bool pooling = (operation_flags() & 0x800000u) != 0u;
    bool head = (operation_flags() & 0x1000000u) != 0u;
    uint C = pc.oc.x;
    if (inside) {
        [unroll] for (uint jb = 0u; jb < 2u; jb++) {
            uint c0 = cq + jb * 16u;
            float4 out4;
            [unroll] for (uint e = 0u; e < 4u; e++) {
                float total = projected[jb][e] + image1(pixel + c0 + e) * ld_f32(bufE, pc.oe.x, c0 + e);
                out4[e] = publish(epilogue, total);
            }
            uint at = pixel + c0;
            if (head) {
                [unroll] for (uint e2 = 0u; e2 < 4u; e2++)
                    region[mine + r * 32u + c0 + e2] = half_round(out4[e2]);
            } else if (pooling) {
                float4 published;
                [unroll] for (uint e3 = 0u; e3 < 4u; e3++) {
                    region[mine + r * 32u + c0 + e3] = out4[e3];
                    published[e3] = e4m3(out4[e3]);
                }
                if ((C & 7u) == 0u) st_f16x4(bufC, C + at * 2u, published);
                else [unroll] for (uint e4 = 0u; e4 < 4u; e4++) st_f16(bufC, C, at + e4, published[e4]);
            } else if (narrow) {
                if ((C & 7u) == 0u) st_f16x4(bufC, C + at * 2u, out4);
                else [unroll] for (uint e5 = 0u; e5 < 4u; e5++) st_f16(bufC, C, at + e5, out4[e5]);
            } else {
                if ((C & 15u) == 0u) st_f32x4(bufC, C + at * 4u, out4);
                else [unroll] for (uint e6 = 0u; e6 < 4u; e6++) st_f32(bufC, C, at + e6, out4[e6]);
            }
        }
    }
    if (head) {
        GroupMemoryBarrierWithGroupSync();
        /* the head as the base portable GEMM computes it: 32 -> 16 columns, one tile */
        float4 hacc = float4(0.0, 0.0, 0.0, 0.0);
        for (uint k3 = 0u; k3 < 32u; k3++) {
            uint at = k3 * 16u + cq;
            float4 bv = float4(ld_f16(bufH, H, at), ld_f16(bufH, H, at + 1u),
                               ld_f16(bufH, H, at + 2u), ld_f16(bufH, H, at + 3u));
            float av = region[mine + r * 32u + k3];
            hacc += av * bv;
        }
        if (!inside) return;
        uint px = uint(y) * pc.image_w + uint(x);
        if ((operation_flags() & 0x2000000u) == 0u) {
            if (cq == 0u)                        // the compact head: the four columns used
                [unroll] for (uint e = 0u; e < 4u; e++) st_f32(bufC, C, px * 4u + e, hacc[e]);
        } else {
            [unroll] for (uint e = 0u; e < 4u; e++) st_f32(bufC, C, px * 16u + cq + e, hacc[e]);
        }
        return;
    }
    if (!pooling) return;
    GroupMemoryBarrierWithGroupSync();
    /* the pool, POOL2's taps in its order: (y, x), (y + 1, x), (y, x + 1), (y + 1, x + 1) */
    for (uint q = index; q < 16u * 32u; q += 256u) {
        uint pp = q / 32u, c = q % 32u, py = pp / 4u, pxx = pp % 4u;
        int yy = int((window / pc.window_cols) * 8u + 2u * py) - int(top);
        int xx = int((window % pc.window_cols) * 8u + 2u * pxx) - int(left);
        if (yy < 0 || xx < 0 || yy >= int(pc.image_h) || xx >= int(pc.image_w)) continue;
        uint upper = (2u * py) * 256u + (2u * pxx) * 32u + c, lower = upper + 256u;
        precise float total = region[upper] + region[lower];
        total += region[upper + 32u];
        total += region[lower + 32u];
        uint at = ((uint(yy) / 2u) * (pc.image_w / 2u) + uint(xx) / 2u) * 32u + c;
        st_f16(bufH, H, at, e4m3(total * 0.25));
    }
}
