/* oclnv12toyuv.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>
 */

#ifdef USE_HWD

#include "opencl.h"
#include "vadxva2.h"
#include "oclnv12toyuv.h"

/**
  * It creates are opencl bufs w is input frame width, h is input frame height
*/
static int hb_nv12toyuv_create_cl_buf( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 );
 
/**
 * It creates are opencl kernel. kernel name is nv12toyuv
*/
static int hb_nv12toyuv_create_cl_kernel( KernelEnv *kenv, hb_va_dxva2_t *dxva2 );
 
/**
 * It set opencl arg, input data,output data, input width, output height
*/
static int hb_nv12toyuv_setkernelarg( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 );
 
/**
 * It initialize nv12 to yuv kernel.
*/
static int hb_init_nv12toyuv_ocl( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 );

/**
 * Run nv12 to yuv kernel.
 */
static int hb_nv12toyuv( void **userdata, KernelEnv *kenv );

/**
 * register nv12 to yuv kernel.
 */
static int hb_nv12toyuv_reg_kernel( void );

/**
 * It creates are opencl bufs w is input frame width, h is input frame height
 */
static int hb_nv12toyuv_create_cl_buf( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 )
{
    if (hb_ocl == NULL)
    {
        hb_error("hb_nv12toyuv_create_cl_kernel: OpenCL support not available");
        return 1;
    }

    cl_int status = CL_SUCCESS;
    int in_bytes = w*h*3/2;
    HB_OCL_BUF_CREATE(hb_ocl, dxva2->cl_mem_nv12, CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, in_bytes);
    HB_OCL_BUF_CREATE(hb_ocl, dxva2->cl_mem_yuv, CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, in_bytes);
    return 0;
}

/**
 * It creates are opencl kernel. kernel name is nv12toyuv
 */
static int hb_nv12toyuv_create_cl_kernel( KernelEnv *kenv, hb_va_dxva2_t *dxva2 )
{
    if (hb_ocl == NULL)
    {
        hb_error("hb_nv12toyuv_create_cl_kernel: OpenCL support not available");
        return 1;
    }

    int ret;
    dxva2->nv12toyuv = hb_ocl->clCreateKernel(kenv->program, "nv12toyuv", &ret);
    return ret;
}

/**
 * It set opencl arg, input data,output data, input width, output height
 */
static int hb_nv12toyuv_setkernelarg( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 )
{
    int arg = 0, status;
    kenv->kernel = dxva2->nv12toyuv;

    if (hb_ocl == NULL)
    {
        hb_error("hb_nv12toyuv_setkernelarg: OpenCL support not available");
        return 1;
    }

    HB_OCL_CHECK(hb_ocl->clSetKernelArg, kenv->kernel, arg++, sizeof(cl_mem), &dxva2->cl_mem_nv12);
    HB_OCL_CHECK(hb_ocl->clSetKernelArg, kenv->kernel, arg++, sizeof(cl_mem), &dxva2->cl_mem_yuv);
    HB_OCL_CHECK(hb_ocl->clSetKernelArg, kenv->kernel, arg++, sizeof(int), &w);
    HB_OCL_CHECK(hb_ocl->clSetKernelArg, kenv->kernel, arg++, sizeof(int), &h);
    return 0;
}

/**
 * It initialize nv12 to yuv kernel.
 */
static int hb_init_nv12toyuv_ocl( KernelEnv *kenv, int w, int h, hb_va_dxva2_t *dxva2 )
{
    if( !dxva2->nv12toyuv )
    {
        if( hb_nv12toyuv_create_cl_buf( kenv, w, h, dxva2 ) )
        {
            hb_log( "OpenCL: nv12toyuv_create_cl_buf fail" );
            return -1;
        }
        if (!dxva2->nv12toyuv_tmp_in) 
		{
            dxva2->nv12toyuv_tmp_in = malloc (w*h*3/2);
		}

        if (!dxva2->nv12toyuv_tmp_out) 
		{
            dxva2->nv12toyuv_tmp_out = malloc (w*h*3/2);
		}

        hb_nv12toyuv_create_cl_kernel( kenv, dxva2 );
    }
    return 0;
}

/**
 * copy_plane
 * @param dst -
 * @param src -
 * @param dstride -
 * @param sstride -
 * @param h -
 */
static uint8_t *copy_plane( uint8_t *dst, uint8_t* src, int dstride, int sstride,
                            int h )
{
    if ( dstride == sstride )
    {
        memcpy( dst, src, dstride * h );
        return dst + dstride * h;
    }

    int lbytes = dstride <= sstride? dstride : sstride;
    while ( --h >= 0 )
    {
        memcpy( dst, src, lbytes );
        src += sstride;
        dst += dstride;
    }

    return dst;
}

/**
 * Run nv12 to yuv kernel.
 */
static int hb_nv12toyuv( void **userdata, KernelEnv *kenv )
{
    int status;
    int w = (int)userdata[0];
    int h = (int)userdata[1];
    uint8_t *bufi1 = userdata[2];
    int *crop = userdata[3];
    hb_va_dxva2_t *dxva2 = userdata[4];

    uint8_t *bufi2 = userdata[5];
    int p = (int)userdata[6];
    int decomb = (int)userdata[7];
    int detelecine = (int)userdata[8];
    int i;
    if( hb_init_nv12toyuv_ocl( kenv, w, h, dxva2 ) )
	{
        return -1;
	}

    if( hb_nv12toyuv_setkernelarg( kenv, w, h, dxva2 ) )
	{
        return -1;
	}

    if (hb_ocl == NULL)
    {
        hb_error("hb_nv12toyuv: OpenCL support not available");
        return -1;
    }

    int in_bytes = w*h*3/2;
    if( kenv->isAMD )
    {
        void *data = hb_ocl->clEnqueueMapBuffer(kenv->command_queue,
                                                dxva2->cl_mem_nv12,
                                                CL_MAP_WRITE_INVALIDATE_REGION,
                                                CL_TRUE, 0, in_bytes, 0, NULL, NULL, NULL);

        for ( i = 0; i < dxva2->height; i++ )
        {
            memcpy( data + i * dxva2->width, bufi1 + i * p, dxva2->width );
            if ( i < dxva2->height >> 1 )
			{
                memcpy( data + ( dxva2->width * dxva2->height ) + i * dxva2->width, bufi2 + i * p, dxva2->width );
			}		
		}
        hb_ocl->clEnqueueUnmapMemObject(kenv->command_queue, dxva2->cl_mem_nv12,
                                        data, 0, NULL, NULL);
    }
    else
    {
        uint8_t *tmp = (uint8_t*)malloc( dxva2->width * dxva2->height * 3 / 2 );
        for( i = 0; i < dxva2->height; i++ )
        {
            memcpy( tmp + i * dxva2->width, bufi1 + i * p, dxva2->width );
            if( i < dxva2->height >> 1 )
			{
                memcpy( tmp + (dxva2->width * dxva2->height) + i * dxva2->width, bufi2 + i * p, dxva2->width );
			}
        }
        HB_OCL_CHECK(hb_ocl->clEnqueueWriteBuffer, kenv->command_queue,
                     dxva2->cl_mem_nv12, CL_TRUE, 0, in_bytes, tmp, 0, NULL, NULL);
        free( tmp );
    }

    size_t gdim[2] = {w>>1, h>>1};
    HB_OCL_CHECK(hb_ocl->clEnqueueNDRangeKernel, kenv->command_queue,
                 kenv->kernel, 2, NULL, gdim, NULL, 0, NULL, NULL );

    if ((crop[0] || crop[1] || crop[2] || crop[3]) &&
        (decomb == 0) && (detelecine == 0))
    {
        uint8_t * crop_data[4];
        int       crop_stride[4];

        hb_ocl->clEnqueueReadBuffer(kenv->command_queue,  dxva2->cl_mem_yuv,
                                    CL_TRUE, 0, in_bytes, dxva2->nv12toyuv_tmp_out,
                                    0, NULL, NULL);
        hb_buffer_t *in = hb_video_buffer_init( w, h );

        int wmp = in->plane[0].stride;
        int hmp = in->plane[0].height;
        copy_plane(in->plane[0].data, dxva2->nv12toyuv_tmp_out, wmp, w, hmp);
        wmp = in->plane[1].stride;
        hmp = in->plane[1].height;
        copy_plane(in->plane[1].data, dxva2->nv12toyuv_tmp_out + w * h,
                   wmp, w >> 1, hmp);
        wmp = in->plane[2].stride;
        hmp = in->plane[2].height;
        copy_plane(in->plane[2].data, dxva2->nv12toyuv_tmp_out + w * h +
                                      ((w * h) >> 2), wmp, w>>1, hmp);

        hb_picture_crop(crop_data, crop_stride, in, crop[0], crop[2]);
        int i, ww = w - ( crop[2] + crop[3] ), hh = h - ( crop[0] + crop[1] );
        for( i = 0; i< hh >> 1; i++ )
        {
            memcpy( dxva2->nv12toyuv_tmp_in + ((i << 1) + 0) * ww,
                    crop_data[0]+ ((i << 1) + 0) * crop_stride[0], ww );
            memcpy( dxva2->nv12toyuv_tmp_in + ((i << 1) + 1) * ww,
                    crop_data[0]+ ((i << 1) + 1) * crop_stride[0], ww );
            memcpy( dxva2->nv12toyuv_tmp_in + (ww * hh) + i * (ww >> 1),
                    crop_data[1] + i * crop_stride[1], ww >> 1 );
            memcpy( dxva2->nv12toyuv_tmp_in + (ww * hh) + ((ww * hh) >> 2) +
                    i * (ww >> 1),
                    crop_data[2] + i * crop_stride[2], ww >> 1 );
        }

        if( kenv->isAMD )
        {
            void *data = hb_ocl->clEnqueueMapBuffer(kenv->command_queue,
                                                    dxva2->cl_mem_yuv,
                                                    CL_MAP_WRITE_INVALIDATE_REGION,
                                                    CL_TRUE, 0, ww * hh * 3 / 2, 0,
                                                    NULL, NULL, NULL);
            memcpy( data, dxva2->nv12toyuv_tmp_in, ww * hh * 3 / 2 );
            hb_ocl->clEnqueueUnmapMemObject(kenv->command_queue,
                                            dxva2->cl_mem_yuv, data, 0, NULL, NULL);
        }
        else
        {
            HB_OCL_CHECK(hb_ocl->clEnqueueWriteBuffer, kenv->command_queue,
                         dxva2->cl_mem_yuv, CL_TRUE, 0, in_bytes,
                         dxva2->nv12toyuv_tmp_in, 0, NULL, NULL);
        }

        hb_buffer_close( &in );
    }
    return 0;
}
/**
 * register nv12 to yuv kernel.
 */
static int hb_nv12toyuv_reg_kernel( void )
{
    int st = hb_register_kernel_wrapper( "nv12toyuv", hb_nv12toyuv );
    if( !st )
    {
        hb_log( "OpenCL: register kernel[%s] failed", "nv12toyuv" );
        return -1;
    }
    return 0;
}
/**
 * nv12 to yuv interface
 * bufi is input frame of nv12, w is input frame width, h is input frame height
 */
int hb_ocl_nv12toyuv( uint8_t *bufi[], int p, int w, int h, int *crop, hb_va_dxva2_t *dxva2, int decomb, int detelecine )
{
    void *userdata[9];
    userdata[0] = (void*)w;
    userdata[1] = (void*)h;
    userdata[2] = bufi[0];
    userdata[3] = crop;
    userdata[4] = dxva2;
    userdata[5] = bufi[1];
    userdata[6] = (void*)p;
    userdata[7] = decomb;
    userdata[8] = detelecine;

    if( hb_nv12toyuv_reg_kernel() )
	{
        return -1;
	}

    if( hb_run_kernel( "nv12toyuv", userdata ) )
    {
        hb_log( "OpenCL: run kernel[nv12toyuv] failed" );
        return -1;
    }
    return 0;
}

#endif // USE_HWD
