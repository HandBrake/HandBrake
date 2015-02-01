/* oclscale.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com>   <http://www.multicorewareinc.com/>

 */

#include <math.h>
#include "common.h"
#include "opencl.h"
#include "openclwrapper.h"
#define FILTER_LEN 4

#define _A -0.5f

cl_float cubic(cl_float x)
{
    if (x < 0)
        x = -x;

    if (x < 1)
        return (_A + 2.0f) * (x * x * x) - (_A + 3.0f) * (x * x) + 0 + 1;
    else if (x < 2)
        return (_A) * (x * x * x) - (5.0f * _A) * (x * x) + (8.0f * _A) * x - (4.0f * _A);
    else
        return 0;
}


cl_float *hb_bicubic_weights(cl_float scale, int length)
{
    cl_float *weights = (cl_float*) malloc(length * sizeof(cl_float) * 4);

    int i;    // C rocks
    cl_float *out = weights;
    for (i = 0; i < length; ++i)
    {
        cl_float x = i / scale;
        cl_float dx = x - (int)x;
        *out++ = cubic(-dx - 1.0f);
        *out++ = cubic(-dx);
        *out++ = cubic(-dx + 1.0f);
        *out++ = cubic(-dx + 2.0f);
    }
    return weights;
}

int setupScaleWeights(cl_float xscale, cl_float yscale, int width, int height, hb_oclscale_t *os, KernelEnv *kenv);

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

    cl_mem in_buf = data[0];
    cl_mem out_buf = data[1];
    int crop_top = (intptr_t)data[2];
    int crop_bottom = (intptr_t)data[3];
    int crop_left = (intptr_t)data[4];
    int crop_right = (intptr_t)data[5];
    cl_int in_frame_w = (intptr_t)data[6];
    cl_int in_frame_h = (intptr_t)data[7];
    cl_int out_frame_w = (intptr_t)data[8];
    cl_int out_frame_h = (intptr_t)data[9];
    hb_oclscale_t  *os = data[10];
    hb_buffer_t *in = data[11];
    hb_buffer_t *out = data[12];

    if (hb_ocl == NULL)
    {
        hb_error("hb_ocl_scale_func: OpenCL support not available");
        return 0;
    }

    if (os->initialized == 0)
    {
        hb_log( "Scaling With OpenCL" );
        if (kenv->isAMD != 0)
            hb_log( "Using Zero Copy");
        // create the block kernel
        cl_int status;
        os->m_kernel = hb_ocl->clCreateKernel(kenv->program, "frame_scale", &status);

        os->initialized = 1;
    }

    {
        // Use the new kernel
        cl_event events[5];
        int eventCount = 0;

        if (kenv->isAMD == 0) {
            status = hb_ocl->clEnqueueUnmapMemObject(kenv->command_queue,
                                                     in->cl.buffer, in->data, 0,
                                                     NULL, &events[eventCount++]);
            status = hb_ocl->clEnqueueUnmapMemObject(kenv->command_queue,
                                                     out->cl.buffer, out->data, 0,
                                                     NULL, &events[eventCount++]);
        }

        cl_int srcPlaneOffset0 = in->plane[0].data - in->data;
        cl_int srcPlaneOffset1 = in->plane[1].data - in->data;
        cl_int srcPlaneOffset2 = in->plane[2].data - in->data;
        cl_int srcRowWords0 = in->plane[0].stride;
        cl_int srcRowWords1 = in->plane[1].stride;
        cl_int srcRowWords2 = in->plane[2].stride;
        cl_int dstPlaneOffset0 = out->plane[0].data - out->data;
        cl_int dstPlaneOffset1 = out->plane[1].data - out->data;
        cl_int dstPlaneOffset2 = out->plane[2].data - out->data;
        cl_int dstRowWords0 = out->plane[0].stride;
        cl_int dstRowWords1 = out->plane[1].stride;
        cl_int dstRowWords2 = out->plane[2].stride;

        if (crop_top != 0 || crop_bottom != 0 || crop_left != 0 || crop_right != 0) {
            srcPlaneOffset0 += crop_left + crop_top * srcRowWords0;
            srcPlaneOffset1 += crop_left / 2 + (crop_top / 2) * srcRowWords1;
            srcPlaneOffset2 += crop_left / 2 + (crop_top / 2) * srcRowWords2;
            in_frame_w = in_frame_w - crop_right - crop_left;
            in_frame_h = in_frame_h - crop_bottom - crop_top;
        }

        cl_float xscale = (out_frame_w * 1.0f) / in_frame_w;
        cl_float yscale = (out_frame_h * 1.0f) / in_frame_h;
        setupScaleWeights(xscale, yscale, out_frame_w, out_frame_h, os, kenv);

        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 0, sizeof(cl_mem), &out_buf);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 1, sizeof(cl_mem), &in_buf);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 2, sizeof(cl_float), &xscale);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 3, sizeof(cl_float), &yscale);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 4, sizeof(cl_int), &srcPlaneOffset0);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 5, sizeof(cl_int), &srcPlaneOffset1);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 6, sizeof(cl_int), &srcPlaneOffset2);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 7, sizeof(cl_int), &dstPlaneOffset0);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 8, sizeof(cl_int), &dstPlaneOffset1);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 9, sizeof(cl_int), &dstPlaneOffset2);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 10, sizeof(cl_int), &srcRowWords0);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 11, sizeof(cl_int), &srcRowWords1);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 12, sizeof(cl_int), &srcRowWords2);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 13, sizeof(cl_int), &dstRowWords0);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 14, sizeof(cl_int), &dstRowWords1);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 15, sizeof(cl_int), &dstRowWords2);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 16, sizeof(cl_int), &in_frame_w);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 17, sizeof(cl_int), &in_frame_h);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 18, sizeof(cl_int), &out_frame_w);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 19, sizeof(cl_int), &out_frame_h);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 20, sizeof(cl_mem), &os->bicubic_x_weights);
        HB_OCL_CHECK(hb_ocl->clSetKernelArg, os->m_kernel, 21, sizeof(cl_mem), &os->bicubic_y_weights);

        size_t workOffset[] = { 0, 0, 0 };
        size_t globalWorkSize[] = { 1, 1, 1 };
        size_t localWorkSize[] = { 1, 1, 1 };

        int xgroups = (out_frame_w + 63) / 64;
        int ygroups = (out_frame_h + 15) / 16;

        localWorkSize[0] = 64;
        localWorkSize[1] = 1;
        localWorkSize[2] = 1;
        globalWorkSize[0] = xgroups * 64;
        globalWorkSize[1] = ygroups;
        globalWorkSize[2] = 3;

        HB_OCL_CHECK(hb_ocl->clEnqueueNDRangeKernel, kenv->command_queue,
                     os->m_kernel, 3, workOffset, globalWorkSize, localWorkSize,
                     eventCount, eventCount == 0 ? NULL : &events[0], &events[eventCount]);
        ++eventCount;

        if (kenv->isAMD == 0) {
            in->data  = hb_ocl->clEnqueueMapBuffer(kenv->command_queue, in->cl.buffer,
                                                   CL_FALSE, CL_MAP_READ|CL_MAP_WRITE,
                                                   0, in->alloc,
                                                   eventCount ? 1                       : 0,
                                                   eventCount ? &events[eventCount - 1] : NULL,
                                                   &events[eventCount], &status);
            out->data = hb_ocl->clEnqueueMapBuffer(kenv->command_queue, out->cl.buffer,
                                                   CL_FALSE, CL_MAP_READ|CL_MAP_WRITE,
                                                   0, out->alloc,
                                                   eventCount ? 1                       : 0,
                                                   eventCount ? &events[eventCount - 1] : NULL,
                                                   &events[eventCount + 1], &status);
            eventCount += 2;
        }

        hb_ocl->clFlush(kenv->command_queue);
        hb_ocl->clWaitForEvents(eventCount, &events[0]);
        int i;
        for (i = 0; i < eventCount; ++i)
        {
            hb_ocl->clReleaseEvent(events[i]);
        }
    }

    return 1;
}

int setupScaleWeights(cl_float xscale, cl_float yscale, int width, int height, hb_oclscale_t *os, KernelEnv *kenv)
{
    cl_int status;

    if (hb_ocl == NULL)
    {
        hb_error("setupScaleWeights: OpenCL support not available");
        return 1;
    }

    if (os->xscale != xscale || os->width < width)
    {
        cl_float *xweights = hb_bicubic_weights(xscale, width);
        HB_OCL_BUF_FREE  (hb_ocl, os->bicubic_x_weights);
        HB_OCL_BUF_CREATE(hb_ocl, os->bicubic_x_weights, CL_MEM_READ_ONLY,
                          sizeof(cl_float) * width * 4);
        HB_OCL_CHECK(hb_ocl->clEnqueueWriteBuffer, kenv->command_queue, os->bicubic_x_weights,
                     CL_TRUE, 0, sizeof(cl_float) * width * 4, xweights, 0, NULL, NULL);
        os->width = width;
        os->xscale = xscale;
        free(xweights);
    }

    if ((os->yscale != yscale) || (os->height < height))
    {
        cl_float *yweights = hb_bicubic_weights(yscale, height);
        HB_OCL_BUF_FREE  (hb_ocl, os->bicubic_y_weights);
        HB_OCL_BUF_CREATE(hb_ocl, os->bicubic_y_weights, CL_MEM_READ_ONLY,
                          sizeof(cl_float) * height * 4);
        HB_OCL_CHECK(hb_ocl->clEnqueueWriteBuffer, kenv->command_queue, os->bicubic_y_weights,
                     CL_TRUE, 0, sizeof(cl_float) * height * 4, yweights, 0, NULL, NULL);
        os->height = height;
        os->yscale = yscale;
        free(yweights);
    }
    return 0;
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


static int s_scale_init_flag = 0;

int do_scale_init()
{
    if ( s_scale_init_flag==0 )
    {
        int st = hb_register_kernel_wrapper( "frame_scale", hb_ocl_scale_func );
        if( !st )
        {
            hb_log( "register kernel[%s] failed", "frame_scale" );
            return 0;
        }
        s_scale_init_flag++;
    }
    return 1;
}


int hb_ocl_scale(hb_buffer_t *in, hb_buffer_t *out, int *crop, hb_oclscale_t *os)
{
    void *data[13];

    if (do_scale_init() == 0)
        return 0;

    data[0] = in->cl.buffer;
    data[1] = out->cl.buffer;
    data[2] = (void*)(intptr_t)(crop[0]);
    data[3] = (void*)(intptr_t)(crop[1]);
    data[4] = (void*)(intptr_t)(crop[2]);
    data[5] = (void*)(intptr_t)(crop[3]);
    data[6] = (void*)(intptr_t)(in->f.width);
    data[7] = (void*)(intptr_t)(in->f.height);
    data[8] = (void*)(intptr_t)(out->f.width);
    data[9] = (void*)(intptr_t)(out->f.height);
    data[10] = os;
    data[11] = in;
    data[12] = out;

    if( !hb_run_kernel( "frame_scale", data ) )
        hb_log( "run kernel[%s] failed", "frame_scale" );
    return 0;
}
