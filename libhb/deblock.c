/*
 Copyright (C) 2005 Michael Niedermayer <michaelni@gmx.at>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "hb.h"
#include "hbffmpeg.h"

#define PP7_QP_DEFAULT    5
#define PP7_MODE_DEFAULT  2

#define XMIN(a,b) ((a) < (b) ? (a) : (b))
#define XMAX(a,b) ((a) > (b) ? (a) : (b))

typedef short DCTELEM;

//===========================================================================//
static const uint8_t  __attribute__((aligned(8))) pp7_dither[8][8] =
{
    {  0,  48,  12,  60,   3,  51,  15,  63, },
    { 32,  16,  44,  28,  35,  19,  47,  31, },
    {  8,  56,   4,  52,  11,  59,   7,  55, },
    { 40,  24,  36,  20,  43,  27,  39,  23, },
    {  2,  50,  14,  62,   1,  49,  13,  61, },
    { 34,  18,  46,  30,  33,  17,  45,  29, },
    { 10,  58,   6,  54,   9,  57,   5,  53, },
    { 42,  26,  38,  22,  41,  25,  37,  21, },
};

struct hb_filter_private_s
{
    int           pp7_qp;
    int           pp7_mode;
    int           pp7_mpeg2;
    int           pp7_temp_stride;
    uint8_t     * pp7_src;
};

static int hb_deblock_init( hb_filter_object_t * filter,
                            hb_filter_init_t * init );

static int hb_deblock_work( hb_filter_object_t * filter,
                            hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out );

static void hb_deblock_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_deblock =
{
    .id            = HB_FILTER_DEBLOCK,
    .enforce_order = 1,
    .name          = "Deblock (pp7)",
    .settings      = NULL,
    .init          = hb_deblock_init,
    .work          = hb_deblock_work,
    .close         = hb_deblock_close,
};

static inline void pp7_dct_a( DCTELEM * dst, uint8_t * src, int stride )
{
    int i;

    for( i = 0; i < 4; i++ )
    {
        int s0 =  src[0*stride] + src[6*stride];
        int s1 =  src[1*stride] + src[5*stride];
        int s2 =  src[2*stride] + src[4*stride];
        int s3 =  src[3*stride];
        int s  =  s3+s3;

        s3 = s  - s0;
        s0 = s  + s0;
        s  = s2 + s1;
        s2 = s2 - s1;

        dst[0] =   s0 + s;
        dst[2] =   s0 - s;
        dst[1] = 2*s3 + s2;
        dst[3] =   s3 - s2*2;

        src++;
        dst += 4;
    }
}

static void pp7_dct_b( DCTELEM * dst, DCTELEM * src )
{
    int i;

    for( i = 0; i < 4; i++ )
    {
        int s0 = src[0*4] + src[6*4];
        int s1 = src[1*4] + src[5*4];
        int s2 = src[2*4] + src[4*4];
        int s3 = src[3*4];
        int s  = s3+s3;

        s3 = s  - s0;
        s0 = s  + s0;
        s  = s2 + s1;
        s2 = s2 - s1;

        dst[0*4] =   s0 + s;
        dst[2*4] =   s0 - s;
        dst[1*4] = 2*s3 + s2;
        dst[3*4] =   s3 - s2*2;

        src++;
        dst++;
    }
}

#define N   (1<<16)
#define N0  4
#define N1  5
#define N2  10
#define SN0 2
#define SN1 2.2360679775
#define SN2 3.16227766017

static const int pp7_factor[16] =
{
    N/(N0*N0), N/(N0*N1), N/(N0*N0),N/(N0*N2),
    N/(N1*N0), N/(N1*N1), N/(N1*N0),N/(N1*N2),
    N/(N0*N0), N/(N0*N1), N/(N0*N0),N/(N0*N2),
    N/(N2*N0), N/(N2*N1), N/(N2*N0),N/(N2*N2),
};

static int pp7_threshold[99][16];

static void pp7_init_threshold( void )
{
    int qp, i;
    int bias = 0;

    for( qp = 0; qp < 99; qp++ )
    {
        for( i = 0; i < 16; i++ )
        {
            pp7_threshold[qp][i] =
                ((i&1)?SN2:SN0) * ((i&4)?SN2:SN0) *
                 XMAX(1,qp) * (1<<2) - 1 - bias;
        }
    }
}

static int pp7_hard_threshold( DCTELEM * src, int qp )
{
    int i;
    int a;

    a = src[0] * pp7_factor[0];
    for( i = 1; i < 16; i++ )
    {
        unsigned int threshold1 = pp7_threshold[qp][i];
        unsigned int threshold2 = (threshold1<<1);
        int level= src[i];
        if( ((unsigned)(level+threshold1)) > threshold2 )
        {
            a += level * pp7_factor[i];
        }
    }
    return (a + (1<<11)) >> 12;
}

static int pp7_medium_threshold( DCTELEM * src, int qp )
{
    int i;
    int a;

    a = src[0] * pp7_factor[0];
    for( i = 1; i < 16; i++ )
    {
        unsigned int threshold1 = pp7_threshold[qp][i];
        unsigned int threshold2 = (threshold1<<1);
        int level= src[i];
        if( ((unsigned)(level+threshold1)) > threshold2 )
        {
            if( ((unsigned)(level+2*threshold1)) > 2*threshold2 )
            {
                a += level * pp7_factor[i];
            }
            else
            {
                if( level>0 )
                {
                    a += 2*(level - (int)threshold1) * pp7_factor[i];
                }
                else
                {
                    a += 2*(level + (int)threshold1) * pp7_factor[i];
                }
            }
        }
    }
    return (a + (1<<11)) >> 12;
}

static int pp7_soft_threshold( DCTELEM * src, int qp )
{
    int i;
    int a;

    a = src[0] * pp7_factor[0];
    for( i = 1; i < 16; i++ )
    {
        unsigned int threshold1 = pp7_threshold[qp][i];
        unsigned int threshold2 = (threshold1<<1);
        int level= src[i];
        if( ((unsigned)(level+threshold1))>threshold2 )
        {
            if( level>0 )
            {
                a += (level - (int)threshold1) * pp7_factor[i];
            }
            else
            {
                a += (level + (int)threshold1) * pp7_factor[i];
            }
        }
    }
    return (a + (1<<11)) >> 12;
}

static int ( * pp7_requantize )( DCTELEM * src, int qp ) = pp7_hard_threshold;

static void pp7_filter( hb_filter_private_t * pv,
                        uint8_t * dst,
                        uint8_t * src,
                        int width,
                        int height,
                        uint8_t * qp_store,
                        int qp_stride,
                        int is_luma)
{
    int x, y;

    const int  stride = is_luma ? pv->pp7_temp_stride : ((width+16+15)&(~15));
    uint8_t  * p_src  = pv->pp7_src + 8*stride;
    DCTELEM  * block  = (DCTELEM *)(pv->pp7_src);
    DCTELEM  * temp   = (DCTELEM *)(pv->pp7_src + 32);

    if( !src || !dst )
    {
        return;
    }

    for( y = 0; y < height; y++ )
    {
        int index = 8 + 8*stride + y*stride;
        memcpy( p_src + index, src + y*width, width );

        for( x = 0; x < 8; x++ )
        {
            p_src[index         - x - 1] = p_src[index +         x    ];
            p_src[index + width + x    ] = p_src[index + width - x - 1];
        }
    }

    for( y = 0; y < 8; y++ )
    {
        memcpy( p_src + (     7-y)*stride,
                p_src + (     y+8)*stride, stride );
        memcpy( p_src + (height+8+y)*stride,
                p_src + (height-y+7)*stride, stride );
    }

    for( y = 0; y < height; y++ )
    {
        for( x = -8; x < 0; x += 4 )
        {
            const int index = x + y*stride + (8-3)*(1+stride) + 8;
            uint8_t * src   = p_src + index;
            DCTELEM * tp    = temp+4*x;

            pp7_dct_a( tp+4*8, src, stride );
        }

        for( x = 0; x < width; )
        {
            const int qps = 3 + is_luma;
            int end = XMIN(x+8, width);

            int qp;
            if( pv->pp7_qp )
            {
                qp = pv->pp7_qp;
            }
            else
            {
                qp = qp_store[ (XMIN(x, width-1)>>qps) +
                               (XMIN(y, height-1)>>qps) * qp_stride ];

                if( pv->pp7_mpeg2 )
                {
                    qp >>= 1;
                }
            }

            for( ; x < end; x++ )
            {
                const int index = x + y*stride + (8-3)*(1+stride) + 8;
                uint8_t * src   = p_src + index;
                DCTELEM * tp    = temp+4*x;
                int v;

                if( (x&3) == 0 )
                {
                    pp7_dct_a( tp+4*8, src, stride );
                }

                pp7_dct_b( block, tp );

                v = pp7_requantize( block, qp );
                v = (v + pp7_dither[y&7][x&7]) >> 6;
                if( (unsigned)v > 255 )
                {
                    v = (-v) >> 31;
                }
                dst[x + y*width] = v;
            }
        }
    }
}

static int hb_deblock_init( hb_filter_object_t * filter, 
                            hb_filter_init_t * init )
{
    filter->private_data = calloc( sizeof(struct hb_filter_private_s), 1 );
    hb_filter_private_t * pv = filter->private_data;

    pv->pp7_qp    = PP7_QP_DEFAULT;
    pv->pp7_mode  = PP7_MODE_DEFAULT;
    pv->pp7_mpeg2 = 1; /*mpi->qscale_type;*/

    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d", &pv->pp7_qp, &pv->pp7_mode );
    }

    if( pv->pp7_qp < 0 )
    {
        pv->pp7_qp = 0;
    }

    pp7_init_threshold();

    switch( pv->pp7_mode )
    {
        case 0:
            pp7_requantize = pp7_hard_threshold;
            break;
        case 1:
            pp7_requantize = pp7_soft_threshold;
            break;
        case 2:
            pp7_requantize = pp7_medium_threshold;
            break;
    }

    int h = (init->geometry.height + 16 + 15) & (~15);

    pv->pp7_temp_stride = (init->geometry.width + 16 + 15) & (~15);

    pv->pp7_src = (uint8_t*)malloc( pv->pp7_temp_stride*(h+8)*sizeof(uint8_t) );

    return 0;
}

static void hb_deblock_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    free( pv );
    filter->private_data = NULL;
}

static int hb_deblock_work( hb_filter_object_t * filter,
                            hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in, * out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    if( /*TODO: mpi->qscale ||*/ pv->pp7_qp )
    {
        out = hb_video_buffer_init( in->f.width, in->f.height );

        pp7_filter( pv,
                out->plane[0].data,
                in->plane[0].data,
                in->plane[0].stride,
                in->plane[0].height,
                NULL, /* TODO: mpi->qscale*/
                0,    /* TODO: mpi->qstride*/
                1 );

        pp7_filter( pv,
                out->plane[1].data,
                in->plane[1].data,
                in->plane[1].stride,
                in->plane[1].height,
                NULL, /* TODO: mpi->qscale*/
                0,    /* TODO: mpi->qstride*/
                0 );

        pp7_filter( pv,
                out->plane[2].data,
                in->plane[2].data,
                in->plane[2].stride,
                in->plane[2].height,
                NULL, /* TODO: mpi->qscale*/
                0,    /* TODO: mpi->qstride*/
                0 );

        out->s = in->s;

        *buf_out = out;
    }
    else
    {
        *buf_in = NULL;
        *buf_out = in;
    }

    return HB_FILTER_OK;
}
