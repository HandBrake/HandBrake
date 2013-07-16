#ifdef USE_OPENCL
#include "openclwrapper.h"

static int hb_yadif_filter( void **userdata, KernelEnv *kenv )
{
    int parity = (int)userdata[0];
    int tff = (int)userdata[1];
    int cur_linesize_y = (int)userdata[2];
    int cur_linesize_uv = (int)userdata[3];
    int out_linesize_y = (int)userdata[4];
    int out_linesize_uv = (int)userdata[5];
    int mode = (int)userdata[6];
    int width = (int)userdata[7];
    int height = (int)userdata[8];
    cl_mem cl_mem_dst = userdata[9];
    cl_mem cl_mem_prev = userdata[10];
    cl_mem cl_mem_cur = userdata[11];
    cl_mem cl_mem_next = userdata[12];

    cl_int status;
    cl_kernel yadif_kernel = clCreateKernel( kenv->program, "yadif_filter", &status );
    kenv->kernel = yadif_kernel;

    int cl_buf_size = (cur_linesize_y + cur_linesize_uv) * height;

    hb_copy_buffer(cl_mem_cur, cl_mem_dst, 0, 0, cl_buf_size);
    int inparity = parity ^ tff; 

    OCLCHECK( clSetKernelArg, kenv->kernel, 0, sizeof(cl_mem), &cl_mem_dst );
    OCLCHECK( clSetKernelArg, kenv->kernel, 1, sizeof(cl_mem), &cl_mem_prev );
    OCLCHECK( clSetKernelArg, kenv->kernel, 2, sizeof(cl_mem), &cl_mem_cur );
    OCLCHECK( clSetKernelArg, kenv->kernel, 3, sizeof(cl_mem), &cl_mem_next );
    OCLCHECK( clSetKernelArg, kenv->kernel, 4, sizeof(int), &inparity );
    OCLCHECK( clSetKernelArg, kenv->kernel, 5, sizeof(int), &cur_linesize_y );
    OCLCHECK( clSetKernelArg, kenv->kernel, 6, sizeof(int), &cur_linesize_uv );
    OCLCHECK( clSetKernelArg, kenv->kernel, 7, sizeof(int), &out_linesize_y );
    OCLCHECK( clSetKernelArg, kenv->kernel, 8, sizeof(int), &out_linesize_uv );
    OCLCHECK( clSetKernelArg, kenv->kernel, 9, sizeof(int), &mode );

    size_t dims[2];
    dims[0] = width*3/2;
    dims[1] = height/2;

    OCLCHECK( clEnqueueNDRangeKernel, kenv->command_queue, kenv->kernel, 2, NULL, dims, NULL, 0, NULL, NULL );

    return 0;
}

int cl_yadif_filter(
    void* cl_mem_dst,
    void* cl_mem_prev,
    void* cl_mem_cur,
    void* cl_mem_next,
    int parity,
    int tff,
    int cur_linesize_y,
    int cur_linesize_uv,
    int out_linesize_y,
    int out_linesize_uv,
    int mode,
    int width,
    int height)
{
    void *userdata[13];
    userdata[0] = parity;
    userdata[1] = tff;
    userdata[2] = cur_linesize_y;
    userdata[3] = cur_linesize_uv;
    userdata[4] = out_linesize_y;
    userdata[5] = out_linesize_uv;
    userdata[6] = mode;
    userdata[7] = width;
    userdata[8] = height;
    userdata[9] = cl_mem_dst;
    userdata[10] = cl_mem_prev;
    userdata[11] = cl_mem_cur;
    userdata[12] = cl_mem_next;

    int st = hb_register_kernel_wrapper( "yadif_filter", hb_yadif_filter);
    if( !st )
    {
        printf( "register kernel[%s] faild %d\n", "yadif_filter",st );
        return -1;
    }

    if( hb_run_kernel( "yadif_filter", userdata ) )
    {
        printf( "run kernel[yadif_filter] faild\n" );
        return -1;
    }
    return 0;
}

#endif
