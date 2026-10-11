/*
 * half_probe — does the hardware float16 conversion survive the compiler, and does it round
 * the way the bit-twiddled version does, subnormals and overflow included? The daemon's
 * start-up check (`nr_daemon.check_half_rounding`) runs this as its unary pass with
 * `XMX_UNARY_SPV=half_probe.spv`; `src/bench/half_probe.comp` is the GLSL it mirrors.
 *
 * Three answers, one buffer each: `b` the rounding done by hand on the bits, `c` a pack-and-
 * unpack round trip (`as_type` through a `half2`, the nearest thing Metal has to
 * packHalf2x16), `d` the cast `float(half(x))` — which is what `half_round` in nr_metal.h is
 * where the device rounds it, as `xmx_half_by_cast()` says. Without fast math neither
 * conversion is foldable on Apple GPUs (notes/phase67, phase74); AMD's compiler folds both,
 * and there `half_round` is the bits (`half_cast_probe` below decides).
 */
#include "nr_metal.h"

inline float bit_round(float x) { return half_round_bits(x); }
inline float pack_round(float x) {
    uint packed = as_type<uint>(half2(x, 0.0h));
    return float(as_type<half2>(packed).x);
}
inline float cast_round(float x) { return float(half(x)); }

kernel void half_probe(constant Push &pc [[buffer(0)]],
                       uint i [[thread_position_in_grid]]) {
    if (i >= pc.m) return;
    float x = float_ptr(pc.a)[i];
    float_out(pc.b)[i] = bit_round(x);
    float_out(pc.c)[i] = pack_round(x);
    float_out(pc.d)[i] = cast_round(x);
}

/* libmetalmx's own start-up question (`probe_half_rounding`): does the cast round on this
 * device, or does the compiler fold it away? */
kernel void half_cast_probe(device const float *x [[buffer(0)]], device float *y [[buffer(1)]],
                            uint i [[thread_position_in_grid]]) {
    y[i] = float(half(x[i]));
}
