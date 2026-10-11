/*
 * half_probe — does the hardware float16 conversion survive the compiler, and does it round
 * the way the bit-twiddled version does, subnormals and overflow included? The daemon's
 * start-up check (`nr_daemon.check_half_rounding`) runs this as its unary pass with
 * `XMX_UNARY_SPV=half_probe.spv`; `src/bench/half_probe.comp` is the GLSL it mirrors.
 *
 * Three answers, one buffer each: `b` the rounding done by hand on the bits, `c` the
 * `f16tof32(f32tof16(x))` round trip — which is what `half_round` in nr_d3d.hlsli is, and
 * `xmx_half_by_cast()` says so — and `d` the cast `float(half(x))` through the native
 * 16-bit type. A driver that folds either one away moves every vendor rounding point in the
 * graph, which is what the check is for.
 */
#include "nr_d3d.hlsli"

float bit_round(float x) {
    float magnitude = abs(x);
    if (magnitude >= 65520.0) return x < 0.0 ? asfloat(0xff800000u) : asfloat(0x7f800000u);
    if (magnitude < 6.103515625e-05) return round(x * 16777216.0) * 5.9604644775390625e-08;
    int bits = asint(x);
    bits = (bits + (0x0FFF + ((bits >> 13) & 1))) & ~0x1FFF;
    return asfloat(bits);
}
float pack_round(float x) { return f16tof32(f32tof16(x)); }
float cast_round(float x) { return float(half(x)); }

[numthreads(256, 1, 1)]
void main(uint3 gid : SV_GroupID, uint lid : SV_GroupIndex) {
    uint i = (gid.x + pc.spare) * 256u + lid;
    if (i >= pc.m) return;
    float x = ld_f32(bufA, pc.oa.x, i);
    st_f32(bufB, pc.ob.x, i, bit_round(x));
    st_f32(bufC, pc.oc.x, i, pack_round(x));
    st_f32(bufD, pc.od.x, i, cast_round(x));
}
