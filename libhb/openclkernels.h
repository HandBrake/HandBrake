#ifndef USE_EXTERNAL_KERNEL

#define KERNEL( ... )# __VA_ARGS__

char *kernel_src_hscale = KERNEL(

    typedef unsigned char  fixed8;

    kernel void frame_h_scale(
        global fixed8 *src,
        global float   *hf_Y,
        global float   *hf_UV,
        global int      *hi_Y,
        global int      *hi_UV,
        global fixed8 *dst,
        int                     stride, //src_width
        int                     filter_len
        )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int width = get_global_size( 0 );
        int height = get_global_size( 1 );
        float result_Y = 0, result_U = 0, result_V = 0;
        int i = 0;

        global fixed8 *src_Y = src;
        global fixed8 *src_U = src_Y+stride*height;
        global fixed8 *src_V = src_U+(stride>>1)*(height>>1);

        global fixed8 *dst_Y = dst;
        global fixed8 *dst_U = dst_Y+width*height;
        global fixed8 *dst_V = dst_U+(width>>1)*(height>>1);

        int xy = y * width + x;
        global fixed8 *rowdata_Y = src_Y+(y * stride);
        for( int i = 0; i<filter_len; i++ )
        {
            result_Y += ( hf_Y[x+i*width] * rowdata_Y[hi_Y[x] + i]);
        }
        dst_Y[xy] = result_Y;

        if( y<(height>>1) && x<(width>>1) )
        {
            int xy = y * (width>>1) + x;
            global fixed8 *rowdata_U = src_U+(y * (stride>>1));
            global fixed8 *rowdata_V = src_V+(y * (stride>>1));
            for( i = 0; i<filter_len; i++ )
            {
                result_U += ( hf_UV[x+i*(width>>1)] * rowdata_U[hi_UV[x] + i]);
                result_V += ( hf_UV[x+i*(width>>1)] * rowdata_V[hi_UV[x] + i]);
            }
            dst_U[xy] = result_U;
            dst_V[xy] = result_V;
        }
    }
    );

char *kernel_src_vscale = KERNEL(

    kernel void frame_v_scale(
        global fixed8 *src,
        global float   *vf_Y,
        global float   *vf_UV,
        global int      *vi_Y,
        global int      *vi_UV,
        global fixed8 *dst,
        int                      src_height,
        int                     filter_len
        )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int width = get_global_size( 0 );
        int height = get_global_size( 1 );
        float result_Y = 0, result_U = 0, result_V = 0;
        int i = 0;

        global fixed8 *src_Y = src;
        global fixed8 *src_U = src_Y+src_height*width;
        global fixed8 *src_V = src_U+(src_height>>1)*(width>>1);

        global fixed8 *dst_Y = dst;
        global fixed8 *dst_U = dst_Y+height*width;
        global fixed8 *dst_V = dst_U+(height>>1)*(width>>1);

        int xy = y * width + x;
        for( i = 0; i<filter_len; i++ )
        {
            result_Y += vf_Y[y+i*height] * src_Y[(vi_Y[y]+i)*width + x];
        }
        dst_Y[xy] = result_Y;

        if( y<(height>>1) && x<(width>>1) )
        {
            int xy = y * (width>>1) + x;
            for( i = 0; i<filter_len; i++ )
            {
                result_U += vf_UV[y+i*(height>>1)] * src_U[(vi_UV[y] + i) * (width>>1) + x];
                result_V += vf_UV[y+i*(height>>1)] * src_V[(vi_UV[y] + i) * (width>>1) + x];
            }
            dst_U[xy] = result_U;
            dst_V[xy] = result_V;
        }
    }
    );

char *kernel_src_nvtoyuv = KERNEL(

    kernel void nv12toyuv( global char *input, global char* output, int w, int h )
    {
        int x = get_global_id( 0 );
        int y = get_global_id( 1 );
        int idx = y * (w>>1) + x;
        vstore4((vload4( 0, input+(idx<<2))), 0, output+(idx<<2)); //Y
        char2 uv = vload2( 0, input+(idx<<1)+w*h );
        output[idx+w*h] = uv.s0;
        output[idx+w*h+((w*h)>>2)] = uv.s1;
    }
    );

#endif
