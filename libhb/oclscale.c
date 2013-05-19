/* oclscale.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */

#ifdef USE_OPENCL

#include <math.h>
#include "common.h"
#include "openclwrapper.h"
#define MaxFilterLength 16
#define FILTER_LEN 4

inline double hb_fit_gauss_kernel( double x )
{
    double powNum = -1 * M_PI;

    powNum *= x;

    powNum *= x;

    return exp( powNum );
}

/**
 * Using gaussian algorithm to calculate the scale filter
 */
static void hb_set_gauss_interpolation( float *pcoeff, int *pmappedindex, int targetdatalength, int srcdatalength, int filterLength, float bias )
{
    int i, j;

    float gausskernel[MaxFilterLength];

    int half = filterLength / 2;

    float scalerate = (float)(srcdatalength) / targetdatalength;

    for( i = 0; i < targetdatalength; ++i )
    {
        float flindex = i * scalerate + bias;

        if( flindex > (srcdatalength - 1))
        {
            flindex -= (int)(flindex - (srcdatalength - 1));
        }

        int srcindex = (int)(flindex);

        float t = flindex - srcindex;

        for( j = 0; j < (int)half; j++ )
        {
            gausskernel[j] = (float)hb_fit_gauss_kernel((half - j) - 1 + t );
        }

        for( j = 0; j < (int)half; j++ )
        {
            gausskernel[half + j] = (float)hb_fit_gauss_kernel( j + 1 - t );
        }

        while( srcindex < (int)half - 1 )
        {
            /* -1 0 1  2
            * M1 S P1 P2
            *
            * if srcindex is 0, M1 and S will be the same sample.  To keep the
            * convolution kernel from having to check for edge conditions, move
            * srcindex to 1, then slide down the coefficients
            */
            srcindex += 1;

            gausskernel[0] += gausskernel[1];

            for( j = 1; j < filterLength - 1; j++ )
            {
                gausskernel[j] = gausskernel[j + 1];
            }

            gausskernel[filterLength - 1] = 0;
        }

        while( srcindex >= srcdatalength - half )
        {
            /* If srcindex is near the edge, shift down srcindex and slide up
            * the coefficients
            */
            srcindex -= 1;

            gausskernel[3] += gausskernel[2];

            for( j = filterLength - 2; j > 0; j-- )
            {
                gausskernel[j] = gausskernel[j - 1];
            }

            gausskernel[0] = 0;
        }

        *pmappedindex++ = srcindex - half + 1;

        // Store normalized Gauss kernel

        float sumtemp = 0;

        for( j = 0; j < filterLength; ++j )
        {
            sumtemp += gausskernel[j];
        }

        for( j = 0; j < filterLength; ++j )
        {
            pcoeff[targetdatalength * j + i] = gausskernel[j] / sumtemp;
        }
    }
}

/**
* executive scale using opencl
* get filter args
* create output buffer
* create horizontal filter buffer
* create vertical filter buffer
* create  kernels
*/
int hb_ocl_scale_func( void **data, KernelEnv *kenv )
{
    cl_int status;

    uint8_t *in_frame = data[0];
    uint8_t *out_frame = data[1];
    int in_frame_w = (int)data[2];
    int in_frame_h = (int)data[3];
    int out_frame_w = (int)data[4];
    int out_frame_h = (int)data[5];
    hb_oclscale_t *os = data[6];

    if( os->use_ocl_mem )
        os->h_in_buf = data[0];
    int h_filter_len = FILTER_LEN;
    int v_filter_len = FILTER_LEN;
    //it will make the psnr lower when filter length is 4 in the condition that the video width is shorter than 960 and width is shorter than 544,so we set the filter length to 2
    if( out_frame_w <= 960 && out_frame_h <= 544 ) 
    {
        h_filter_len>>=1;
        v_filter_len>>=1;
    }
    if( !os->h_out_buf )
    {
        hb_log( "OpenCL: Scaling With OpenCL" );
        //malloc filter args
        float *hf_y, *hf_uv, *vf_y, *vf_uv;
        int   *hi_y, *hi_uv, *vi_y, *vi_uv;
        hf_y =  (float*)malloc( sizeof(float)*out_frame_w * h_filter_len );
        hf_uv = (float*)malloc( sizeof(float)*(out_frame_w>>1) * h_filter_len );
        hi_y =  (int*)malloc( sizeof(int)*out_frame_w );
        hi_uv = (int*)malloc( sizeof(int)*(out_frame_w>>1));
        vf_y =  (float*)malloc( sizeof(float)*out_frame_h * v_filter_len );
        vf_uv = (float*)malloc( sizeof(float)*(out_frame_h>>1) * v_filter_len );
        vi_y =  (int*)malloc( sizeof(int)*out_frame_h );
        vi_uv = (int*)malloc( sizeof(int)*(out_frame_h>>1) );
        //get filter args
        hb_set_gauss_interpolation( hf_y, hi_y, out_frame_w, in_frame_w, h_filter_len, 0 );
        hb_set_gauss_interpolation( hf_uv, hi_uv, out_frame_w>>1, in_frame_w>>1, h_filter_len, 0 );
        hb_set_gauss_interpolation( vf_y, vi_y, out_frame_h, in_frame_h, v_filter_len, 0 );
        hb_set_gauss_interpolation( vf_uv, vi_uv, out_frame_h>>1, in_frame_h>>1, v_filter_len, 0 );
        //create output buffer
        if( !os->use_ocl_mem )
        {
            CREATEBUF( os->h_in_buf, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(uint8_t)* in_frame_w * in_frame_h*3/2 );
        }
        CREATEBUF( os->h_out_buf, CL_MEM_WRITE_ONLY, sizeof(uint8_t) * out_frame_w * in_frame_h*3/2 );
        CREATEBUF( os->v_out_buf, CL_MEM_WRITE_ONLY, sizeof(uint8_t) * out_frame_w * out_frame_h*3/2 );
        //create horizontal filter buffer
        CREATEBUF( os->h_coeff_y, CL_MEM_READ_ONLY, sizeof(float) * out_frame_w * h_filter_len );
        CREATEBUF( os->h_coeff_uv, CL_MEM_READ_ONLY, sizeof(float) * (out_frame_w>>1) * h_filter_len );
        CREATEBUF( os->h_index_y, CL_MEM_READ_ONLY, sizeof(int) * out_frame_w );
        CREATEBUF( os->h_index_uv, CL_MEM_READ_ONLY, sizeof(int) * (out_frame_w>>1) );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->h_coeff_y, CL_TRUE, 0, sizeof(float) * out_frame_w * h_filter_len, hf_y, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->h_coeff_uv, CL_TRUE, 0, sizeof(float) * (out_frame_w>>1) * h_filter_len, hf_uv, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->h_index_y, CL_TRUE, 0, sizeof(int) * out_frame_w, hi_y, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->h_index_uv, CL_TRUE, 0, sizeof(int) * (out_frame_w>>1), hi_uv, 0, NULL, NULL );
        //create vertical filter buffer
        CREATEBUF( os->v_coeff_y, CL_MEM_READ_ONLY, sizeof(float) * out_frame_h * v_filter_len );
        CREATEBUF( os->v_coeff_uv, CL_MEM_READ_ONLY, sizeof(float) * (out_frame_h>>1) * v_filter_len );
        CREATEBUF( os->v_index_y, CL_MEM_READ_ONLY, sizeof(int) * out_frame_h );
        CREATEBUF( os->v_index_uv, CL_MEM_READ_ONLY, sizeof(int) * (out_frame_h>>1) );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->v_coeff_y, CL_TRUE, 0, sizeof(float) * out_frame_h * v_filter_len, vf_y, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->v_coeff_uv, CL_TRUE, 0, sizeof(float) * (out_frame_h>>1) * v_filter_len, vf_uv, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->v_index_y, CL_TRUE, 0, sizeof(int) * out_frame_h, vi_y, 0, NULL, NULL );
        OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->v_index_uv, CL_TRUE, 0, sizeof(int) * (out_frame_h>>1), vi_uv, 0, NULL, NULL );
        //create horizontal kernel
        os->h_kernel = clCreateKernel( kenv->program, "frame_h_scale", NULL );
        OCLCHECK( clSetKernelArg, os->h_kernel, 1, sizeof(cl_mem), &os->h_coeff_y );
        OCLCHECK( clSetKernelArg, os->h_kernel, 2, sizeof(cl_mem), &os->h_coeff_uv );
        OCLCHECK( clSetKernelArg, os->h_kernel, 3, sizeof(cl_mem), &os->h_index_y );
        OCLCHECK( clSetKernelArg, os->h_kernel, 4, sizeof(cl_mem), &os->h_index_uv );
        OCLCHECK( clSetKernelArg, os->h_kernel, 6, sizeof(int), &in_frame_w );
        OCLCHECK( clSetKernelArg, os->h_kernel, 7, sizeof(int), &h_filter_len );
        //create vertical kernel
        os->v_kernel = clCreateKernel( kenv->program, "frame_v_scale", NULL );
        OCLCHECK( clSetKernelArg, os->v_kernel, 1, sizeof(cl_mem), &os->v_coeff_y );
        OCLCHECK( clSetKernelArg, os->v_kernel, 2, sizeof(cl_mem), &os->v_coeff_uv );
        OCLCHECK( clSetKernelArg, os->v_kernel, 3, sizeof(cl_mem), &os->v_index_y );
        OCLCHECK( clSetKernelArg, os->v_kernel, 4, sizeof(cl_mem), &os->v_index_uv );
        OCLCHECK( clSetKernelArg, os->v_kernel, 6, sizeof(int), &in_frame_h );
        OCLCHECK( clSetKernelArg, os->v_kernel, 7, sizeof(int), &v_filter_len );
        free( hf_y );
        free( hf_uv );
        free( vf_y );
        free( vf_uv );
        free( hi_y );
        free( hi_uv );
        free( vi_y );
        free( vi_uv );
    }
    //start horizontal scaling kernel

    if( !os->use_ocl_mem )
    {
        if( kenv->isAMD )
        {
            char *mapped = clEnqueueMapBuffer( kenv->command_queue, os->h_in_buf, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0, sizeof(uint8_t) * in_frame_w * in_frame_h*3/2, 0, NULL, NULL, NULL );
            memcpy( mapped, in_frame, sizeof(uint8_t) * in_frame_w * in_frame_h*3/2 );
            clEnqueueUnmapMemObject( kenv->command_queue, os->h_in_buf, mapped, 0, NULL, NULL );
        }
        else
        {
            OCLCHECK( clEnqueueWriteBuffer, kenv->command_queue, os->h_in_buf, CL_TRUE, 0, sizeof(uint8_t) * in_frame_w * in_frame_h * 3/2, in_frame, 0, NULL, NULL );
        }
    }

    kenv->kernel = os->h_kernel;
    size_t dims[2];
    dims[0] = out_frame_w;
    dims[1] = in_frame_h;
    OCLCHECK( clSetKernelArg, kenv->kernel, 0, sizeof(cl_mem), &os->h_in_buf );
    OCLCHECK( clSetKernelArg, kenv->kernel, 5, sizeof(cl_mem), &os->h_out_buf );
    OCLCHECK( clEnqueueNDRangeKernel, kenv->command_queue, kenv->kernel, 2, NULL, dims, NULL, 0, NULL, NULL );
    //start vertical scaling kernel

    kenv->kernel = os->v_kernel;
    dims[0] = out_frame_w;
    dims[1] = out_frame_h;
    OCLCHECK( clSetKernelArg, kenv->kernel, 0, sizeof(cl_mem), &os->h_out_buf );
    OCLCHECK( clSetKernelArg, kenv->kernel, 5, sizeof(cl_mem), &os->v_out_buf );
    OCLCHECK( clEnqueueNDRangeKernel, kenv->command_queue, kenv->kernel, 2, NULL, dims, NULL, 0, NULL, NULL );
    OCLCHECK( clEnqueueReadBuffer, kenv->command_queue, os->v_out_buf, CL_TRUE, 0, sizeof(uint8_t) * out_frame_w * out_frame_h * 3/2, out_frame, 0, NULL, NULL );

    return 1;
}

/**
* function describe: this function is used to scaling video frame. it uses the gausi scaling algorithm
*  parameter:
*           inputFrameBuffer: the source video frame opencl buffer
*           outputdata: the destination video frame buffer
*           inputWidth: the width of the source video frame
*           inputHeight: the height of the source video frame
*           outputWidth: the width of destination video frame
*           outputHeight: the height of destination video frame
*/
int hb_ocl_scale( cl_mem in_buf, uint8_t *in_data, uint8_t *out_data, int in_w, int in_h, int out_w, int out_h, hb_oclscale_t  *os )
{
    void *data[7];
    static int init_flag = 0;
    if( init_flag == 0 )
    {
        int st = hb_register_kernel_wrapper( "frame_h_scale", hb_ocl_scale_func );
        if( !st )
        {
            hb_log( "OpenCL: Register kernel[%s] failed", "frame_h_scale" );
            return 0;
        }
        init_flag++;
    }

    if( in_data==NULL )
    {
        data[0] = in_buf;
        os->use_ocl_mem = 1;
    }
    else
    {
        data[0] = in_data;
        os->use_ocl_mem = 0;
    }

    data[1] = out_data;
    data[2] = (void*)in_w;
    data[3] = (void*)in_h;
    data[4] = (void*)out_w;
    data[5] = (void*)out_h;
    data[6] = os;

    if( !hb_run_kernel( "frame_h_scale", data ) )
	{
        hb_log( "OpenCL: Run kernel[%s] failed", "frame_scale" );
	}

    return 0;
}
#endif
