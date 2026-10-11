/* Map the first of four consecutive channels into the unpadded image.
 * Channel counts and four-element stores never cross a pixel boundary.
 * Cropped window padding has no destination and must not read the skip buffer. */
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

/* GEMM residual, before the existing publication/rounding step.
 * Match resident.comp's expression, including its compiler contraction policy. */
float add_gemm_residual(float branch, uint index) {
    if ((operation_flags() & 0x20000u) == 0u) return branch;
    float skip = (operation_flags() & 0x40000u) != 0u
        ? float(HalfBuf(uint64_t(pc.d)).v[index]) : pc.d.v[index];
    return branch + skip * FloatBuf(pc.residual_cos).v[index % pc.n];
}
