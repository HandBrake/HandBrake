#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_intel_subgroup_matrix_multiply_accumulate : enable
#ifndef RM
#define RM 1
#endif
#ifndef RN
#define RN 1
#endif

/* Intel's SG16 interface: each lane supplies one K coordinate of each A row,
 * and the packed 16 half values of one B column. Accumulators remain FP32. */
__attribute__((intel_reqd_sub_group_size(16)))
__kernel void gemm(__global const half *A, __global const half *B,
                   __global float *C, uint M, uint N, uint K)
{
    uint lane = get_sub_group_local_id();
    uint row = get_group_id(1) * (8 * RM);
    uint col = get_group_id(0) * (16 * RN);
    float8 accum[RM][RN];
    for (uint r = 0; r < RM; r++)
        for (uint c = 0; c < RN; c++) accum[r][c] = (float8)(0.0f);
    for (uint k = 0; k < K; k += 16) {
        short8 a[RM];
        int8 b[RN];
        for (uint r = 0; r < RM; r++)
            for (uint i = 0; i < 8; i++) a[r][i] = as_short(A[(row + r*8 + i)*K + k + lane]);
        for (uint c = 0; c < RN; c++)
            for (uint i = 0; i < 8; i++)
                b[c][i] = as_int((half2)(B[(k + i*2)*N + col + c*16 + lane],
                                        B[(k + i*2 + 1)*N + col + c*16 + lane]));
        for (uint r = 0; r < RM; r++)
            for (uint c = 0; c < RN; c++)
                accum[r][c] = intel_sub_group_f16_f16_matrix_mad_k16(a[r], b[c], accum[r][c]);
    }
    for (uint r = 0; r < RM; r++)
        for (uint c = 0; c < RN; c++)
            for (uint i = 0; i < 8; i++) C[(row + r*8 + i)*N + col + c*16 + lane] = accum[r][c][i];
}
