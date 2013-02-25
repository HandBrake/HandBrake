
#ifdef USE_OPENCL
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "scale.h"
#include "openclwrapper.h"

#define OCLCHECK( method, ...) \
    status = method(__VA_ARGS__); if(status != CL_SUCCESS) { \
    hb_log(" error %s %d",# method, status); assert(0); return status; }
    
#define CREATEBUF( out, flags, size, ptr)\
    out = clCreateBuffer( kenv->context, (flags), (size), ptr, &status );\
    if( status != CL_SUCCESS ) { hb_log( "clCreateBuffer faild %d", status ); return -1; }

 #define CL_PARAM_NUM 20

/****************************************************************************************************************************/
/*************************Combine the hscale and yuv2plane into scaling******************************************************/
/****************************************************************************************************************************/
static int CreateCLBuffer( ScaleContext *c, KernelEnv *kenv )
{
    cl_int status;
    
    if(!c->hyscale_fast || !c->hcscale_fast)
    {
        CREATEBUF(c->cl_hLumFilter,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, c->dstW*c->hLumFilterSize*sizeof(cl_short),c->hLumFilter);
        CREATEBUF(c->cl_hLumFilterPos,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, c->dstW*sizeof(cl_int),c->hLumFilterPos);
        CREATEBUF(c->cl_hChrFilter,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, c->chrDstW*c->hChrFilterSize*sizeof(cl_short),c->hChrFilter);
        CREATEBUF(c->cl_hChrFilterPos,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, c->chrDstW*sizeof(cl_int),c->hChrFilterPos);
    }
    if( c->vLumFilterSize > 1 && c->vChrFilterSize > 1 )
    {
        CREATEBUF(c->cl_vLumFilter,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,c->dstH*c->vLumFilterSize*sizeof(cl_short),c->vLumFilter);
        CREATEBUF(c->cl_vChrFilter,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,c->chrDstH*c->vChrFilterSize*sizeof(cl_short),c->vChrFilter);
    }
    CREATEBUF(c->cl_vLumFilterPos,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,c->dstH*sizeof(cl_int),c->vLumFilterPos);
    CREATEBUF(c->cl_vChrFilterPos,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,c->chrDstH*sizeof(cl_int),c->vChrFilterPos);
    
    return 1;
}

int av_scale_frame_func( void **userdata, KernelEnv *kenv )
{
    ScaleContext *c = (ScaleContext *)userdata[0];

    c->cl_src = (cl_mem)userdata[2];
    c->cl_dst = (cl_mem)userdata[1];

    /*frame size*/
    int *tmp = (int *)userdata[3];
    int srcStride = tmp[0];
    int srcChrStride = tmp[1];
    int srcW = c->srcW;
    int srcH = c->srcH;
    
    tmp = (int *)userdata[4];
    int dstStride = tmp[0];
    int dstChrStride = tmp[1];
    int dstW = c->dstW;
    int dstH = c->dstH;
    
     /* local variable */
    cl_int status;
    size_t global_work_size[2];

    int intermediaSize;

    int st = CreateCLBuffer(c,kenv);
    if( !st )
    {
        hb_log( "CreateBuffer[%s] faild %d", "scale_opencl",st );
        return -1;
    }

    intermediaSize = dstStride * srcH + dstChrStride * srcH;

    CREATEBUF(c->cl_intermediaBuf,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,intermediaSize*sizeof(cl_short),NULL);

    static int init_chr_status = 0;
    static cl_kernel chr_kernel;

    if(init_chr_status == 0){

        if(!(c->flags & 1))
        {
            chr_kernel = clCreateKernel( kenv->program, "hscale_all_opencl", NULL );
            //Set the Kernel Argument;
            OCLCHECK(clSetKernelArg,chr_kernel,2,sizeof(cl_mem),(void*)&c->cl_hLumFilter);
            OCLCHECK(clSetKernelArg,chr_kernel,3,sizeof(cl_mem),(void*)&c->cl_hLumFilterPos);
            OCLCHECK(clSetKernelArg,chr_kernel,4,sizeof(int),(void*)&c->hLumFilterSize);
            OCLCHECK(clSetKernelArg,chr_kernel,5,sizeof(cl_mem),(void*)&c->cl_hChrFilter);
            OCLCHECK(clSetKernelArg,chr_kernel,6,sizeof(cl_mem),(void*)&c->cl_hChrFilterPos);
            OCLCHECK(clSetKernelArg,chr_kernel,7,sizeof(int),(void*)&c->hChrFilterSize);
        }
    
        /*Set the arguments*/
        OCLCHECK(clSetKernelArg,chr_kernel,8,sizeof(dstW),(void*)&dstW);
        OCLCHECK(clSetKernelArg,chr_kernel,9,sizeof(srcH),(void*)&srcH);
        OCLCHECK(clSetKernelArg,chr_kernel,10,sizeof(srcW),(void*)&srcW);
        OCLCHECK(clSetKernelArg,chr_kernel,11,sizeof(srcH),(void*)&srcH);
        OCLCHECK(clSetKernelArg,chr_kernel,12,sizeof(dstStride),(void*)&dstStride);
        OCLCHECK(clSetKernelArg,chr_kernel,13,sizeof(dstChrStride),(void*)&dstChrStride);
        OCLCHECK(clSetKernelArg,chr_kernel,14,sizeof(srcStride),(void*)&srcStride);
        OCLCHECK(clSetKernelArg,chr_kernel,15,sizeof(srcChrStride),(void*)&srcChrStride);
        init_chr_status = 1;
    }

    kenv->kernel = chr_kernel;
    OCLCHECK(clSetKernelArg,chr_kernel,0,sizeof(cl_mem),(void*)&c->cl_intermediaBuf);
    OCLCHECK(clSetKernelArg,chr_kernel,1,sizeof(cl_mem),(void*)&c->cl_src);
    /*Run the Kernel*/
    global_work_size[0] = c->chrDstW;//dstW >> 1; //must times 256;
    global_work_size[1] = c->chrSrcH;

    OCLCHECK(clEnqueueNDRangeKernel,kenv->command_queue, kenv->kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);

    static int init_lum_status = 0;
    static cl_kernel lum_kernel;

    if( init_lum_status == 0 ){
        //Vertical:
        /*Create Kernel*/
        if( c->vLumFilterSize > 1 && c->vChrFilterSize > 1 )
            lum_kernel = clCreateKernel( kenv->program, "vscale_all_nodither_opencl", NULL );
        else
            lum_kernel = clCreateKernel( kenv->program, "vscale_fast_opencl", NULL );

        if( c->vLumFilterSize > 1 && c->vChrFilterSize > 1 )
        {
            OCLCHECK(clSetKernelArg,lum_kernel,2,sizeof(cl_mem),(void*)&c->cl_vLumFilter);
            OCLCHECK(clSetKernelArg,lum_kernel,3,sizeof(int),(void*)&c->vLumFilterSize);
            OCLCHECK(clSetKernelArg,lum_kernel,4,sizeof(cl_mem),(void*)&c->cl_vChrFilter);
            OCLCHECK(clSetKernelArg,lum_kernel,5,sizeof(int),(void*)&c->vChrFilterSize);
            OCLCHECK(clSetKernelArg,lum_kernel,6,sizeof(cl_mem),(void*)&c->cl_vLumFilterPos);
            OCLCHECK(clSetKernelArg,lum_kernel,7,sizeof(cl_mem),(void*)&c->cl_vChrFilterPos);
            OCLCHECK(clSetKernelArg,lum_kernel,8,sizeof(dstW),(void*)&dstW);
            OCLCHECK(clSetKernelArg,lum_kernel,9,sizeof(dstH),(void*)&dstH);
            OCLCHECK(clSetKernelArg,lum_kernel,10,sizeof(srcW),(void*)&srcW);
            OCLCHECK(clSetKernelArg,lum_kernel,11,sizeof(srcH),(void*)&srcH);
            OCLCHECK(clSetKernelArg,lum_kernel,12,sizeof(dstStride),(void*)&dstStride);
            OCLCHECK(clSetKernelArg,lum_kernel,13,sizeof(dstChrStride),(void*)&dstChrStride);
            OCLCHECK(clSetKernelArg,lum_kernel,14,sizeof(dstStride),(void*)&dstStride);
            OCLCHECK(clSetKernelArg,lum_kernel,15,sizeof(dstChrStride),(void*)&dstChrStride);
        }else{
    
            OCLCHECK(clSetKernelArg,lum_kernel,2,sizeof(cl_mem),(void*)&c->cl_vLumFilterPos);
            OCLCHECK(clSetKernelArg,lum_kernel,3,sizeof(cl_mem),(void*)&c->cl_vChrFilterPos);
            OCLCHECK(clSetKernelArg,lum_kernel,4,sizeof(dstW),(void*)&dstW);
            OCLCHECK(clSetKernelArg,lum_kernel,5,sizeof(dstH),(void*)&dstH);
            OCLCHECK(clSetKernelArg,lum_kernel,6,sizeof(srcW),(void*)&srcW);
            OCLCHECK(clSetKernelArg,lum_kernel,7,sizeof(srcH),(void*)&srcH);
            OCLCHECK(clSetKernelArg,lum_kernel,8,sizeof(dstStride),(void*)&dstStride);
            OCLCHECK(clSetKernelArg,lum_kernel,9,sizeof(dstChrStride),(void*)&dstChrStride);
            OCLCHECK(clSetKernelArg,lum_kernel,10,sizeof(dstStride),(void*)&dstStride);
            OCLCHECK(clSetKernelArg,lum_kernel,11,sizeof(dstChrStride),(void*)&dstChrStride);
        }
        init_lum_status = 1;
    }
    
    kenv->kernel = lum_kernel;
    OCLCHECK(clSetKernelArg,kenv->kernel,0,sizeof(cl_mem),(void*)&c->cl_dst);
    OCLCHECK(clSetKernelArg,kenv->kernel,1,sizeof(cl_mem),(void*)&c->cl_intermediaBuf);
    
    /*Run the Kernel*/
    global_work_size[0] = c->chrDstW;
    global_work_size[1] = c->chrDstH;

    OCLCHECK(clEnqueueNDRangeKernel,kenv->command_queue,kenv->kernel,2,NULL,global_work_size,NULL,0,NULL,NULL);

    clReleaseMemObject( c->cl_intermediaBuf );
    
    return 1;
}

void av_scale_frame(ScaleContext *c,
                        void *dst,
                        void *src,
                        int *srcStride,
                        int *dstStride,
                        int *should_dither)
{
    
    static int regflg = 0;
    void *userdata[CL_PARAM_NUM];
    userdata[0] = (void *)c;
    userdata[1] = (void *)dst;
    userdata[2] = (void *)src;
    userdata[3] = (void *)srcStride;
    userdata[4] = (void *)dstStride;
    userdata[5] = (void *)should_dither;

    if( regflg==0 )
    {
        int st = hb_register_kernel_wrapper( "scale_opencl", av_scale_frame_func);
        if( !st )
        {
            hb_log( "register kernel[%s] faild %d", "scale_opencl",st );
            return;
        }
        regflg++;
    }
 
    if( !hb_run_kernel( "scale_opencl", userdata ))
    {
        hb_log("run kernel function[%s] faild", "scale_opencl_func" );
        return;
    }     
}

#endif
