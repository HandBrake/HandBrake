/* openclkernels.h

   Copyright (c) 2003-2017 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
   
   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */
 
#ifndef USE_EXTERNAL_KERNEL

#define KERNEL( ... )# __VA_ARGS__


char *kernel_src_hscale = KERNEL (

    typedef unsigned char fixed8;

/*******************************************************************************************************
dst:          Horizontal scale destination;
src:          YUV content in opencl buf;
hf_Y:         Horizontal filter coefficients for Y planes;
hf_UV:        Horizontal filter coefficients for UV planes;
hi_Y:         Horizontal filter index for Y planes;
hi_UV:        Horizontal filter index for UV planes;
stride:       Src width;
filter_len:   Length of filter;
********************************************************************************************************/
    kernel void frame_h_scale (
        global fixed8 *src,
        global float *hf_Y,
        global float *hf_UV,
        global int *hi_Y,
        global int *hi_UV,
        global fixed8 *dst,
        int stride, //src_width
        int filter_len
        )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int width = get_global_size( 0 );
        int height = get_global_size( 1 );
        float result_Y = 0, result_U = 0, result_V = 0;
        int i = 0;

        global fixed8 *src_Y = src;
        global fixed8 *src_U = src_Y + stride * height;
        global fixed8 *src_V = src_U + (stride >> 1) * (height >> 1);

        global fixed8 *dst_Y = dst;
        global fixed8 *dst_U = dst_Y + width * height;
        global fixed8 *dst_V = dst_U + (width >> 1) * (height >> 1);

        int xy = y * width + x;
        global fixed8 *rowdata_Y = src_Y + (y * stride);
        for( int i = 0; i < filter_len; i++ )
        {
            result_Y += ( hf_Y[x + i * width] * rowdata_Y[hi_Y[x] + i]);
        }
        dst_Y[xy] = result_Y;

        if( y < (height >> 1) && x < (width >> 1) )
        {
            int xy = y * (width >> 1) + x;
            global fixed8 *rowdata_U = src_U + (y * (stride >> 1));
            global fixed8 *rowdata_V = src_V + (y * (stride >> 1));
            for( i = 0; i < filter_len; i++ )
            {
                result_U += ( hf_UV[x + i * (width >> 1)] * rowdata_U[hi_UV[x] + i]);
                result_V += ( hf_UV[x + i * (width >> 1)] * rowdata_V[hi_UV[x] + i]);
            }
            dst_U[xy] = result_U;
            dst_V[xy] = result_V;
        }
    }
    );

/*******************************************************************************************************
dst:          Vertical scale destination;
src:          YUV content in opencl buf;
hf_Y:         Vertical filter coefficients for Y planes;
hf_UV:        Vertical filter coefficients for UV planes;
hi_Y:         Vertical filter index for Y planes;
hi_UV:        Vertical filter index for UV planes;
stride:       Src height;
filter_len:   Length of filter;
********************************************************************************************************/
char *kernel_src_vscale = KERNEL (

    kernel void frame_v_scale (
        global fixed8 *src,
        global float *vf_Y,
        global float *vf_UV,
        global int *vi_Y,
        global int *vi_UV,
        global fixed8 *dst,
        int src_height,
        int filter_len
        )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int width = get_global_size( 0 );
        int height = get_global_size( 1 );
        float result_Y = 0, result_U = 0, result_V = 0;
        int i = 0;

        global fixed8 *src_Y = src;
        global fixed8 *src_U = src_Y + src_height * width;
        global fixed8 *src_V = src_U + (src_height >> 1) * (width >> 1);

        global fixed8 *dst_Y = dst;
        global fixed8 *dst_U = dst_Y + height * width;
        global fixed8 *dst_V = dst_U + (height >> 1) * (width >> 1);

        int xy = y * width + x;
        for( i = 0; i < filter_len; i++ )
        {
            result_Y += vf_Y[y + i * height] * src_Y[(vi_Y[y] + i) * width + x];
        }
        dst_Y[xy] = result_Y;

        if( y < (height >> 1) && x < (width >> 1) )
        {
            int xy = y * (width >> 1) + x;
            for( i = 0; i < filter_len; i++ )
            {
                result_U += vf_UV[y + i * (height >> 1)] * src_U[(vi_UV[y] + i) * (width >> 1) + x];
                result_V += vf_UV[y + i * (height >> 1)] * src_V[(vi_UV[y] + i) * (width >> 1) + x];
            }
            dst_U[xy] = result_U;
            dst_V[xy] = result_V;
        }
    }
    );

/*******************************************************************************************************
input:    Input buffer;
output:   Output buffer;
w:        Width of frame;
h:        Height of frame;
********************************************************************************************************/
char *kernel_src_nvtoyuv = KERNEL (

    kernel void nv12toyuv ( global char *input, global char* output, int w, int h )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int idx = y * (w >> 1) + x;
        vstore4((vload4( 0, input + (idx << 2))), 0, output + (idx << 2)); //Y
        char2 uv = vload2( 0, input + (idx << 1) + w * h );
        output[idx + w * h] = uv.s0;
        output[idx + w * h + ((w * h) >> 2)] = uv.s1;
    }
    );

/*******************************************************************************************************
dst:           Horizontal scale destination;
src:           YUV content in opencl buf;
yfilter:       Opencl memory of horizontal filter coefficients for luma/alpha planes;
yfilterPos:    Opencl memory of horizontal filter starting positions for each dst[i] for luma/alpha planes;
yfilterSize:   Horizontal filter size for luma/alpha pixels;
cfilter:       Opencl memory of horizontal filter coefficients for chroma planes;
cfilterPos:    Opencl memory of horizontal filter starting positions for each dst[i] for chroma planes;
cfilterSize:   Horizontal filter size for chroma pixels;
dstStride:     Width of destination luma/alpha planes;
dstChrStride:  Width of destination chroma planes;
********************************************************************************************************/

char *kernel_src_hscaleall = KERNEL (

    kernel void hscale_all_opencl (
        global short *dst,
        const global unsigned char *src,
        const global short *yfilter,
        const global int *yfilterPos,
        int yfilterSize,
        const global short *cfilter,
        const global int *cfilterPos,
        int cfilterSize,
        int dstWidth,
        int dstHeight,
        int srcWidth,
        int srcHeight,
        int dstStride,
        int dstChrStride,
        int srcStride,
        int srcChrStride)
    {
        int w = get_global_id(0);
        int h = get_global_id(1);

        int chrWidth = get_global_size(0);
        int chrHeight = get_global_size(1);

        int srcPos1 = h * srcStride + yfilterPos[w];
        int srcPos2 = h * srcStride + yfilterPos[w + chrWidth];
        int srcPos3 = (h + (srcHeight >> 1)) * srcStride + yfilterPos[w];
        int srcPos4 = (h + (srcHeight >> 1)) * srcStride + yfilterPos[w + chrWidth];
        int srcc1Pos = srcStride * srcHeight + (h) * (srcChrStride) + cfilterPos[w];
        int srcc2Pos = srcc1Pos + ((srcChrStride)*(chrHeight));

        int val1 = 0;
        int val2 = 0;
        int val3 = 0;
        int val4 = 0;
        int val5 = 0;
        int val6 = 0;

        int filterPos1 = yfilterSize * w;
        int filterPos2 = yfilterSize * (w + chrWidth);
        int cfilterPos1 = cfilterSize * w;

        int j;
        for (j = 0; j < yfilterSize; j++)
        {
            val1 += src[srcPos1 + j] * yfilter[filterPos1+ j];
            val2 += src[srcPos2 + j] * yfilter[filterPos2 + j];
            val3 += src[srcPos3 + j] * yfilter[filterPos1 + j];
            val4 += src[srcPos4 + j] * yfilter[filterPos2 + j];
            val5 += src[srcc1Pos+j] * cfilter[cfilterPos1 + j];
            val6 += src[srcc2Pos+j] * cfilter[cfilterPos1 + j];
        }
        int dstPos1 = h *dstStride;
        int dstPos2 = (h + chrHeight) * dstStride;

        dst[dstPos1 + w] = ((val1 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val1 >> 7));
        dst[dstPos1 + w + chrWidth] = ((val2 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val2 >> 7));
        dst[dstPos2 + w] = ((val3 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val3 >> 7));
        dst[dstPos2 + w + chrWidth] = ((val4 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val4 >> 7));

        int dstPos3 = h * (dstChrStride) + w + dstStride * dstHeight;
        int dstPos4 = h * (dstChrStride) + w + dstStride * dstHeight + ((dstChrStride) * chrHeight);
        dst[dstPos3] = ((val5 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val5 >> 7));
        dst[dstPos4] = ((val6 >> 7) > ((1 << 15) - 1) ? ((1 << 15) - 1) : (val6 >> 7));
    }
    );

char *kernel_src_hscalefast = KERNEL (

    kernel void hscale_fast_opencl (
        global short *dst,
        const global unsigned char *src,
        int xInc,
        int chrXInc,
        int dstWidth,
        int dstHeight,
        int srcWidth,
        int srcHeight,
        int dstStride,
        int dstChrStride,
        int srcStride,
        int srcChrStride)
    {

        int w = get_global_id(0);
        int h = get_global_id(1);

        int chrWidth = get_global_size(0);
        int chrHeight = get_global_size(1);
        int xpos1 = 0;
        int xpos2 = 0;
        int xx = xpos1 >> 16;
        int xalpha = (xpos1 & 0xFFFF) >> 9;
        dst[h * dstStride + w] = (src[h * srcStride + xx] << 7) + (src[h * srcStride + xx + 1] -src[h * srcStride + xx]) * xalpha;
        int lowpart = h + (chrHeight);
        dst[lowpart * dstStride + w] = (src[lowpart * srcStride + xx] << 7) + (src[lowpart * srcStride + xx + 1] - src[lowpart * srcStride + xx]) * xalpha;

        int inv_i = w * xInc >> 16;
        if( inv_i >= srcWidth - 1)
        {
            dst[h*dstStride + w] = src[h*srcStride + srcWidth-1]*128;
            dst[lowpart*dstStride + w] = src[lowpart*srcStride + srcWidth - 1] * 128;
        }

        int rightpart = w + (chrWidth);
        xx = xpos2 >> 16;
        xalpha = (xpos2 & 0xFFFF) >> 9;
        dst[h * dstStride + rightpart] = (src[h *srcStride + xx] << 7) + (src[h * srcStride + xx + 1] - src[h * srcStride + xx]) * xalpha;
        dst[lowpart * dstStride + rightpart] = (src[lowpart * srcStride + xx] << 7) + (src[lowpart * srcStride + xx + 1] - src[lowpart * srcStride + xx]) * xalpha;
        inv_i = rightpart * xInc >> 16;
        if( inv_i >= srcWidth - 1)
        {
            dst[h * dstStride + rightpart] = src[h * srcStride + srcWidth - 1] * 128;
            dst[lowpart * dstStride + rightpart] = src[lowpart * srcStride + srcWidth - 1] * 128;
        }

        int xpos = 0;
        xpos = chrXInc * w;
        xx = xpos >> 16;
        xalpha = (xpos & 0xFFFF) >> 9;
        src += srcStride * srcHeight;
        dst += dstStride * dstHeight;
        dst[h * (dstChrStride) + w] = (src[h * (srcChrStride) + xx] * (xalpha^127) + src[h * (srcChrStride) + xx + 1] * xalpha);
        inv_i = w * xInc >> 16;
        if( inv_i >= (srcWidth >> 1) - 1)
        {
            dst[h * (dstChrStride) + w] = src[h * (srcChrStride) + (srcWidth >> 1) -1]*128;
        }

        xpos = chrXInc * (w);
        xx = xpos >> 16;
        src += srcChrStride * srcHeight >> 1;
        dst += (dstChrStride * chrHeight);
        dst[h * (dstChrStride) + w] = (src[h * (srcChrStride) + xx] * (xalpha^127) + src[h * (srcChrStride) + xx + 1 ] * xalpha);

        if( inv_i >= (srcWidth >> 1) - 1)
        {
            //v channel:
            dst[h * (dstChrStride) + w] = src[h * (srcChrStride) + (srcWidth >> 1) -1] * 128;
        }
    }
    );

char *kernel_src_vscalealldither = KERNEL (

    kernel void vscale_all_dither_opencl (
        global unsigned char *dst,
        const global short *src,
        const global short *yfilter,
        int yfilterSize,
        const global short *cfilter,
        int cfilterSize,
        const global int *yfilterPos,
        const global int *cfilterPos,
        int dstWidth,
        int dstHeight,
        int srcWidth,
        int srcHeight,
        int dstStride,
        int dstChrStride,
        int srcStride,
        int srcChrStride)
    {
        const unsigned char hb_dither_8x8_128[8][8] = {
            {  36, 68,  60, 92,  34, 66,  58, 90, },
            { 100,  4, 124, 28,  98,  2, 122, 26, },
            {  52, 84,  44, 76,  50, 82,  42, 74, },
            { 116, 20, 108, 12, 114, 18, 106, 10, },
            {  32, 64,  56, 88,  38, 70,  62, 94, },
            {  96,  0, 120, 24, 102,  6, 126, 30, },
            {  48, 80,  40, 72,  54, 86,  46, 78, },
            { 112, 16, 104,  8, 118, 22, 110, 14, },
        };


        int w = get_global_id(0);
        int h = get_global_id(1);

        int chrWidth = get_global_size(0);
        int chrHeight = get_global_size(1);
        const unsigned char *local_up_dither;
        const unsigned char *local_down_dither;

        local_up_dither = hb_dither_8x8_128[h & 7];
        local_down_dither = hb_dither_8x8_128[(h + chrHeight) & 7];

        //yscale;
        int srcPos1 = (yfilterPos[h]) * srcStride + w;
        int srcPos2 = (yfilterPos[h]) * srcStride + w + (chrWidth);
        int srcPos3 = (yfilterPos[h + chrHeight]) * srcStride + w;
        int srcPos4 = (yfilterPos[h + chrHeight]) * srcStride + w + chrWidth;
        int src1Pos = dstStride * srcHeight + (cfilterPos[h]) * dstChrStride + (w);
        int src2Pos = dstStride * srcHeight + (dstChrStride*(srcHeight>>1)) + (cfilterPos[h]) * dstChrStride + w;

        int val1 = (local_up_dither[w & 7] << 12); //y offset is 0;
        int val2 = (local_up_dither[(w + chrWidth) & 7] << 12);
        int val3 = (local_down_dither[w &7] << 12);
        int val4 = (local_down_dither[(w + chrWidth) & 7] << 12);
        int val5 = (local_up_dither[w & 7] << 12);
        int val6 = (local_up_dither[(w + 3) & 7] << 12);   // 3 is offset of the chrome channel.

        int j;
        int filterPos1 = h * yfilterSize;
        int filterPos2 = ( h + chrHeight ) * yfilterSize;
        for(j = 0; j < yfilterSize; j++)
        {
            val1 += src[srcPos1] * yfilter[filterPos1 + j];
            srcPos1 += srcStride;
            val2 += src[srcPos2] * yfilter[filterPos1 + j];
            srcPos2 += srcStride;
            val3 += src[srcPos3] * yfilter[filterPos2 + j];
            srcPos3 += srcStride;
            val4 += src[srcPos4] * yfilter[filterPos2 + j];
            srcPos4 += srcStride;
            val5 += src[src1Pos] * cfilter[filterPos1 + j];
            val6 += src[src2Pos] * cfilter[filterPos1 + j];
            src1Pos += dstChrStride;
            src2Pos += dstChrStride;
        }
        dst[h * dstStride + w] = (((val1 >> 19)&(~0xFF)) ? ((-(val1 >> 19)) >> 31) : (val1 >> 19));
        dst[h * dstStride + w + chrWidth] = (((val2 >> 19)&(~0xFF)) ? ((-(val2 >> 19)) >> 31) : (val2 >> 19));
        dst[(h + chrHeight) * dstStride + w] = (((val3 >> 19)&(~0xFF)) ? ((-(val3 >> 19)) >> 31) : (val3 >> 19));
        dst[(h + chrHeight) * dstStride + w + chrWidth] = (((val4 >> 19)&(~0xFF)) ? ((-(val4 >> 19)) >> 31) : (val4 >> 19));

        int dst1Pos = dstStride * dstHeight + h*(dstChrStride)+(w);
        int dst2Pos = (dstChrStride * chrHeight) + dst1Pos;
        dst[dst1Pos] = (((val5 >> 19)&(~0xFF)) ? ((-(val5 >> 19)) >> 31) : (val5 >> 19));
        dst[dst2Pos] = (((val6 >> 19)&(~0xFF)) ? ((-(val6 >> 19)) >> 31) : (val6 >> 19));
    }
    );

char *kernel_src_vscaleallnodither = KERNEL (

    kernel void vscale_all_nodither_opencl (
        global unsigned char *dst,
        const global short *src,
        const global short *yfilter,
        int yfilterSize,
        const global short *cfilter,
        int cfilterSize,
        const global int *yfilterPos,
        const global int *cfilterPos,
        int dstWidth,
        int dstHeight,
        int srcWidth,
        int srcHeight,
        int dstStride,
        int dstChrStride,
        int srcStride,
        int srcChrStride)
    {
        const unsigned char hb_sws_pb_64[8] = {
            64, 64, 64, 64, 64, 64, 64, 64
        };

        int w = get_global_id(0);
        int h = get_global_id(1);

        int chrWidth = get_global_size(0);
        int chrHeight = get_global_size(1);
        const unsigned char *local_up_dither;
        const unsigned char *local_down_dither;

        local_up_dither = hb_sws_pb_64;
        local_down_dither = hb_sws_pb_64;


        //yscale;
        int srcPos1 = (yfilterPos[h]) * srcStride + w;
        int srcPos2 = (yfilterPos[h]) * srcStride + w + (chrWidth);
        int srcPos3 = (yfilterPos[h + chrHeight]) * srcStride + w;
        int srcPos4 = (yfilterPos[h + chrHeight]) * srcStride + w + chrWidth;
        int src1Pos = dstStride * srcHeight + (cfilterPos[h]) * dstChrStride + (w);
        int src2Pos = dstStride * srcHeight + (dstChrStride*(srcHeight>>1)) + (cfilterPos[h]) * dstChrStride + w;

        int val1 = (local_up_dither[w & 7] << 12); //y offset is 0;
        int val2 = (local_up_dither[(w + chrWidth) & 7] << 12);
        int val3 = (local_down_dither[w &7] << 12);
        int val4 = (local_down_dither[(w + chrWidth) & 7] << 12);
        int val5 = (local_up_dither[w & 7] << 12);
        int val6 = (local_up_dither[(w + 3) & 7] << 12);   // 3 is offset of the chrome channel.


        int j;
        int filterPos1 = h * yfilterSize;
        int filterPos2 = ( h + chrHeight ) * yfilterSize;
        for(j = 0; j < yfilterSize; j++)
        {
            val1 += src[srcPos1] * yfilter[filterPos1 + j];
            srcPos1 += srcStride;
            val2 += src[srcPos2] * yfilter[filterPos1 + j];
            srcPos2 += srcStride;
            val3 += src[srcPos3] * yfilter[filterPos2 + j];
            srcPos3 += srcStride;
            val4 += src[srcPos4] * yfilter[filterPos2 + j];
            srcPos4 += srcStride;
            val5 += src[src1Pos] * cfilter[filterPos1 + j];
            val6 += src[src2Pos] * cfilter[filterPos1 + j];
            src1Pos += dstChrStride;
            src2Pos += dstChrStride;
        }
        dst[h * dstStride + w] = (((val1 >> 19)&(~0xFF)) ? ((-(val1 >> 19)) >> 31) : (val1 >> 19));
        dst[h * dstStride + w + chrWidth] = (((val2 >> 19)&(~0xFF)) ? ((-(val2 >> 19)) >> 31) : (val2 >> 19));
        dst[(h + chrHeight) * dstStride + w] = (((val3 >> 19)&(~0xFF)) ? ((-(val3 >> 19)) >> 31) : (val3 >> 19));
        dst[(h + chrHeight) * dstStride + w + chrWidth] = (((val4 >> 19)&(~0xFF)) ? ((-(val4 >> 19)) >> 31) : (val4 >> 19));;

        int dst1Pos = dstStride * dstHeight + h * (dstChrStride) + (w);
        int dst2Pos = (dstChrStride * chrHeight) + dst1Pos;
        dst[dst1Pos] = (((val5 >> 19)&(~0xFF)) ? ((-(val5 >> 19)) >> 31) : (val5 >> 19));
        dst[dst2Pos] = (((val6 >> 19)&(~0xFF)) ? ((-(val6 >> 19)) >> 31) : (val6 >> 19));
    }
    );

char *kernel_src_vscalefast = KERNEL (

    kernel void vscale_fast_opencl (
        global unsigned char *dst,
        const global short *src,
        const global int *yfilterPos,
        const global int *cfilterPos,
        int dstWidth,
        int dstHeight,
        int srcWidth,
        int srcHeight,
        int dstStride,
        int dstChrStride,
        int srcStride,
        int srcChrStride)
    {
        const unsigned char hb_sws_pb_64[8] = {
            64, 64, 64, 64, 64, 64, 64, 64
        };

        int w = get_global_id(0);
        int h = get_global_id(1);

        int chrWidth = get_global_size(0);
        int chrHeight = get_global_size(1);

        const unsigned char *local_up_dither;
        const unsigned char *local_down_dither;

        local_up_dither = hb_sws_pb_64;
        local_down_dither = hb_sws_pb_64;


        int rightpart = w + chrWidth;
        int bh = h + chrHeight; // bottom part
        short val1 = (src[(yfilterPos[h]) * dstStride + w] + local_up_dither[(w + 0) & 7]) >> 7; //lum offset is 0;
        short val2 = (src[(yfilterPos[h]) * dstStride + rightpart] + local_up_dither[rightpart & 7]) >> 7;
        short val3 = (src[(yfilterPos[bh]) * dstStride + w] + local_down_dither[w & 7]) >> 7;
        short val4 = (src[(yfilterPos[bh]) * dstStride + rightpart] + local_down_dither[rightpart & 7]) >> 7;
        dst[h * dstStride + w] = ((val1&(~0xFF)) ? ((-val1) >> 31) : (val1));
        dst[h * dstStride + rightpart] = ((val2&(~0xFF)) ? ((-val2) >> 31) : (val2));
        dst[bh * dstStride + w] = ((val3&(~0xFF)) ? ((-val3) >> 31) : (val3));
        dst[bh * dstStride + rightpart] = ((val4&(~0xFF)) ? ((-val4) >> 31) : (val4));

        src += dstStride * srcHeight;
        dst += dstStride * dstHeight;
        val1 = (src[cfilterPos[h] * (dstChrStride) + w] + local_up_dither[ w & 7]) >> 7;
        dst[h * (dstChrStride) + w] = ((val1&(~0xFF)) ? ((-val1) >> 31) : (val1));

        src += dstChrStride * (srcHeight >> 1);
        dst += dstChrStride * chrHeight;
        val1 = (src[cfilterPos[h] * dstChrStride + w] + local_up_dither[ (w + 3) & 7] ) >> 7;
        dst[h * dstChrStride + w] = ((val1&(~0xFF)) ? ((-val1) >> 31) : (val1));

    }
    );

char *kernel_src_scale = KERNEL (

__kernel __attribute__((reqd_work_group_size(64, 1, 1))) void frame_scale(__global uchar *dst,
                          __global const uchar *src,
                          const float xscale,
                          const float yscale,
                          const int srcPlaneOffset0,
                          const int srcPlaneOffset1,
                          const int srcPlaneOffset2,
                          const int dstPlaneOffset0,
                          const int dstPlaneOffset1,
                          const int dstPlaneOffset2,
                          const int srcRowWords0,
                          const int srcRowWords1,
                          const int srcRowWords2,
                          const int dstRowWords0,
                          const int dstRowWords1,
                          const int dstRowWords2,
                          const int srcWidth,
                          const int srcHeight,
                          const int dstWidth,
                          const int dstHeight,
                          __global const float4* restrict xweights,
                          __global const float4* restrict yweights
                          )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);

    // Abort work items outside the dst image bounds.

    if ((get_group_id(0) * 64 >= (dstWidth >> ((z == 0) ? 0 : 1))) || (get_group_id(1) * 16 >= (dstHeight >> ((z == 0) ? 0 : 1))))
      return;

    const int srcPlaneOffset = (z == 0) ? srcPlaneOffset0 : ((z == 1) ? srcPlaneOffset1 : srcPlaneOffset2);
    const int dstPlaneOffset = (z == 0) ? dstPlaneOffset0 : ((z == 1) ? dstPlaneOffset1 : dstPlaneOffset2);
    const int srcRowWords = (z == 0) ? srcRowWords0: ((z == 1) ? srcRowWords1 : srcRowWords2);
    const int dstRowWords = (z == 0) ? dstRowWords0: ((z == 1) ? dstRowWords1 : dstRowWords2);

    __local uchar pixels[64 * 36];
    const int localRowPixels = 64;
    const int groupHeight = 16;     // src pixel height output by the workgroup
    const int ypad = 2;
    const int localx = get_local_id(0);

    const int globalStartRow = floor((get_group_id(1) * groupHeight) / yscale);
    const int globalRowCount = ceil(groupHeight / yscale) + 2 * ypad;

    float4 weights = xweights[x];
    int4 woffs = floor(x / xscale);
    woffs += (int4)(-1, 0, 1, 2);
    woffs = clamp(woffs, 0, (srcWidth >> ((z == 0) ? 0 : 1)) - 1);
    const int maxy = (srcHeight >> ((z == 0) ? 0 : 1)) - 1;

    // Scale x from global into LDS

    for (int i = 0; i <= globalRowCount; ++i) {
        int4 offs = srcPlaneOffset + clamp(globalStartRow - ypad + i, 0, maxy) * srcRowWords;
        offs += woffs;
        pixels[localx + i * localRowPixels] = convert_uchar(clamp(round(dot(weights,
                       (float4)(src[offs.x], src[offs.y], src[offs.z], src[offs.w]))), 0.0f, 255.0f));
    }
 
    barrier(CLK_LOCAL_MEM_FENCE);

    // Scale y from LDS into global

    if (x >= dstWidth >> ((z == 0) ? 0 : 1))
        return;

    int off = dstPlaneOffset + x + (get_group_id(1) * groupHeight) * dstRowWords;

    for (int i = 0; i < groupHeight; ++i) {
        if (y >= dstHeight >> ((z == 0) ? 0 : 1))
          break;
        int localy = floor((get_group_id(1) * groupHeight + i) / yscale);
        localy = localy - globalStartRow + ypad;
        int loff = localx + localy * localRowPixels;
        dst[off] = convert_uchar(clamp(round(dot(yweights[get_group_id(1) * groupHeight + i],
                                (float4)(pixels[loff - localRowPixels], pixels[loff], pixels[loff + localRowPixels]
                                , pixels[loff + localRowPixels * 2]))), 0.0f, 255.0f));
        off += dstRowWords;
    }
}
);


char *kernel_src_yadif_filter = KERNEL(
    void filter_v6(
        global unsigned char *dst,
        global unsigned char *prev, 
        global unsigned char *cur, 
        global unsigned char *next,
        int x,
        int y,
        int width,
        int height,
        int parity,
        int inlinesize,
        int outlinesize,
        int inmode,
        int uvflag
    )
    {

        int flag = uvflag * (y >=height) * height;  
        int prefs = select(-(inlinesize), inlinesize,((y+1) - flag) <height);
        int mrefs = select(inlinesize, -(inlinesize),y - flag);
        int mode  = select(inmode,2,(y - flag==1) || (y - flag + 2==height));
        int score;
    
        global unsigned char *prev2 = parity ? prev : cur ;
        global unsigned char *next2 = parity ? cur  : next;
        int index = x + y * inlinesize;
        int outindex = x + y * outlinesize;
        int c = cur[index + mrefs]; 
        int d = (prev2[index] + next2[index])>>1; 
        int e = cur[index + prefs]; 
        int temporal_diff0 = abs((prev2[index]) - (next2[index])); 
        int temporal_diff1 =(abs(prev[index + mrefs] - c) + abs(prev[index + prefs] - e) )>>1; 
        int temporal_diff2 =(abs(next[index + mrefs] - c) + abs(next[index + prefs] - e) )>>1; 
        int diff = max(max(temporal_diff0>>1, temporal_diff1), temporal_diff2); 
        int spatial_pred = (c+e)>>1; 
        int spatial_score = abs(cur[index + mrefs-1] - cur[index + prefs-1]) + abs(c-e) + abs(cur[index + mrefs+1] - cur[index + prefs+1]) - 1; 
        //check -1
        score = abs(cur[index + mrefs-2] - cur[index + prefs])
            + abs(cur[index + mrefs-1] - cur[index + prefs+1])
            + abs(cur[index + mrefs] - cur[index + prefs+2]);
        if (score < spatial_score)
        {
            spatial_score= score;
            spatial_pred= (cur[index + mrefs-1] + cur[index + prefs+1])>>1;
        }
        //check -2
        score = abs(cur[index + mrefs-3] - cur[index + prefs+1])
            + abs(cur[index + mrefs-2] - cur[index + prefs+2])
            + abs(cur[index + mrefs-1] - cur[index + prefs+3]);
        if (score < spatial_score)
        {
            spatial_score= score;
            spatial_pred= (cur[index + mrefs-2] + cur[index + prefs+2])>>1;
        }
        //check 1
        score = abs(cur[index + mrefs] - cur[index + prefs-2])
            + abs(cur[index + mrefs+1] - cur[index + prefs-1])
            + abs(cur[index + mrefs+2] - cur[index + prefs]);
        if (score < spatial_score)
        {
            spatial_score= score;
            spatial_pred= (cur[index + mrefs+1] + cur[index + prefs-1])>>1;
        }
        //check 2
        score = abs(cur[index + mrefs+1] - cur[index + prefs-3])
            + abs(cur[index + mrefs+2] - cur[index + prefs-2])
            + abs(cur[index + mrefs+3] - cur[index + prefs-1]);
        if (score < spatial_score)
        {
            spatial_score= score;
            spatial_pred= (cur[index + mrefs+2] + cur[index + prefs-2])>>1;
        }
        if (mode < 2)
        { 
            int b = (prev2[index + (mrefs<<1)] + next2[index + (mrefs<<1)])>>1; 
            int f = (prev2[index + (prefs<<1)] + next2[index + (prefs<<1)])>>1; 
            int diffmax = max(max(d-e, d-c), min(b-c, f-e)); 
            int diffmin = min(min(d-e, d-c), max(b-c, f-e)); 

            diff = max(max(diff, diffmin), -diffmax); 
        } 
        if (spatial_pred > d + diff) 
        {
            spatial_pred = d + diff; 
        }
        else if (spatial_pred < d - diff) 
        {
            spatial_pred = d - diff; 
        }

        dst[outindex] = spatial_pred; 
    }

    kernel void yadif_filter(
        global unsigned char *dst,
        global unsigned char *prev,
        global unsigned char *cur,
        global unsigned char *next,
        int parity,
        int inlinesizeY,
        int inlinesizeUV,
        int outlinesizeY,
        int outlinesizeUV,
        int mode)
    {
        int x=get_global_id(0);
        int y=(get_global_id(1)<<1) + (!parity);
        int width=(get_global_size(0)<<1)/3;
        int height=get_global_size(1)<<1;
    

        global unsigned char *dst_Y=dst;
        global unsigned char *dst_U=dst_Y+height*outlinesizeY;

        global unsigned char *prev_Y=prev;
        global unsigned char *prev_U=prev_Y+height*inlinesizeY;

        global unsigned char *cur_Y=cur;
        global unsigned char *cur_U=cur_Y+height*inlinesizeY;

        global unsigned char *next_Y=next;
        global unsigned char *next_U=next_Y+height*inlinesizeY;

        if(x < width)
        {
            filter_v6(dst_Y,prev_Y,cur_Y,next_Y,x,y,width,height,parity,inlinesizeY,outlinesizeY,mode,0);
        }
        else
        {
            x = x - width;
            filter_v6(dst_U,prev_U,cur_U,next_U,x,y,width>>1,height>>1,parity,inlinesizeUV,outlinesizeUV,mode,1);
        }
    }
    );

#endif
