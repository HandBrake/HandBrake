/*
 Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>

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
#include "mpeg2dec/mpeg2.h"
#include "mcdeint.h"

#define SUPPRESS_AV_LOG

#define ABS(a) ((a) > 0 ? (a) : (-(a)))

void mcdeint_init( mcdeint_private_t * pv,
                   int mode,
                   int qp,
                   int width,
                   int height )
{
    pv->mcdeint_mode = mode;
    pv->mcdeint_qp = qp;
    
    /* Allocate mcdeint specific buffers */
    if( pv->mcdeint_mode >= 0 )
    {
        avcodec_init();
        avcodec_register_all();

        AVCodec * enc = avcodec_find_encoder( CODEC_ID_SNOW );

        int i;
        for (i = 0; i < 3; i++ )
        {
            AVCodecContext * avctx_enc;

            avctx_enc = pv->mcdeint_avctx_enc = avcodec_alloc_context();

            avctx_enc->width                    = width;
            avctx_enc->height                   = height;
            avctx_enc->time_base                = (AVRational){1,25};  // meaningless
            avctx_enc->gop_size                 = 300;
            avctx_enc->max_b_frames             = 0;
            avctx_enc->pix_fmt                  = PIX_FMT_YUV420P;
            avctx_enc->flags                    = CODEC_FLAG_QSCALE | CODEC_FLAG_LOW_DELAY;
            avctx_enc->strict_std_compliance    = FF_COMPLIANCE_EXPERIMENTAL;
            avctx_enc->global_quality           = 1;
            avctx_enc->flags2                   = CODEC_FLAG2_MEMC_ONLY;
            avctx_enc->me_cmp                   = FF_CMP_SAD; //SSE;
            avctx_enc->me_sub_cmp               = FF_CMP_SAD; //SSE;
            avctx_enc->mb_cmp                   = FF_CMP_SSE;

            switch( pv->mcdeint_mode )
            {
                case 3:
                    avctx_enc->refs = 3;
                case 2:
                    avctx_enc->me_method = ME_ITER;
                case 1:
                    avctx_enc->flags |= CODEC_FLAG_4MV;
                    avctx_enc->dia_size =2;
                case 0:
                    avctx_enc->flags |= CODEC_FLAG_QPEL;
            }

            hb_avcodec_open(avctx_enc, enc, 0);
        }

        pv->mcdeint_frame       = avcodec_alloc_frame();
        pv->mcdeint_outbuf_size = width * height * 10;
        pv->mcdeint_outbuf      = malloc( pv->mcdeint_outbuf_size );
    }

}

void mcdeint_close( mcdeint_private_t * pv )
{
    /* Cleanup mcdeint specific buffers */
    if( pv->mcdeint_mode >= 0 )
    {
        if( pv->mcdeint_avctx_enc )
        {
            hb_avcodec_close( pv->mcdeint_avctx_enc );
            av_freep( &pv->mcdeint_avctx_enc );
        }
        if( pv->mcdeint_outbuf )
        {
            free( pv->mcdeint_outbuf );
        }
    }
}

void mcdeint_filter( uint8_t ** dst,
                     uint8_t ** src,
                     int parity,
                     int * width,
                     int * height,
                     mcdeint_private_t * pv )
{
    int x, y, i;
    int out_size;

#ifdef SUPPRESS_AV_LOG
    /* TODO: temporarily change log level to suppress obnoxious debug output */
    int loglevel = av_log_get_level();
    av_log_set_level( AV_LOG_QUIET );
#endif

    for( i=0; i<3; i++ )
    {
        pv->mcdeint_frame->data[i] = src[i];
        pv->mcdeint_frame->linesize[i] = width[i];
    }
    pv->mcdeint_avctx_enc->me_cmp     = FF_CMP_SAD;
    pv->mcdeint_avctx_enc->me_sub_cmp = FF_CMP_SAD;
    pv->mcdeint_frame->quality        = pv->mcdeint_qp * FF_QP2LAMBDA;

    out_size = avcodec_encode_video( pv->mcdeint_avctx_enc,
                                     pv->mcdeint_outbuf,
                                     pv->mcdeint_outbuf_size,
                                     pv->mcdeint_frame );

    pv->mcdeint_frame_dec = pv->mcdeint_avctx_enc->coded_frame;

    for( i = 0; i < 3; i++ )
    {
        int w    = width[i];
        int h    = height[i];
        int fils = pv->mcdeint_frame_dec->linesize[i];
        int srcs = width[i];

        for( y = 0; y < h; y++ )
        {
            if( (y ^ parity) & 1 )
            {
                for( x = 0; x < w; x++ )
                {
                    if( (x-1)+(y-1)*w >= 0 && (x+1)+(y+1)*w < w*h )
                    {
                        uint8_t * filp =
                            &pv->mcdeint_frame_dec->data[i][x + y*fils];
                        uint8_t * srcp = &src[i][x + y*srcs];

                        int diff0 = filp[-fils] - srcp[-srcs];
                        int diff1 = filp[+fils] - srcp[+srcs];
                        int spatial_score;
                        
                        spatial_score =
                            ABS(srcp[-srcs-1] - srcp[+srcs-1]) +
                            ABS(srcp[-srcs  ] - srcp[+srcs  ]) +
                            ABS(srcp[-srcs+1] - srcp[+srcs+1]) - 1;

                        int temp = filp[0];

#define MCDEINT_CHECK(j)\
                        {   int score = ABS(srcp[-srcs-1+j] - srcp[+srcs-1-j])\
                                      + ABS(srcp[-srcs  +j] - srcp[+srcs  -j])\
                                      + ABS(srcp[-srcs+1+j] - srcp[+srcs+1-j]);\
                            if( score < spatial_score ) {\
                                spatial_score = score;\
                                diff0 = filp[-fils+j] - srcp[-srcs+j];\
                                diff1 = filp[+fils-j] - srcp[+srcs-j];

                        if( x >= 2 && x <= w - 3 )
                        {
                            MCDEINT_CHECK(-1)
                            if( x >= 3 && x <= w - 4 )
                            {
                                MCDEINT_CHECK(-2) }} }}
                            }
                        }
                        if( x >= 2 && x <= w - 3 )
                        {
                            MCDEINT_CHECK(1)
                            if( x >= 3 && x <= w - 4 )
                            {
                                MCDEINT_CHECK(2) }} }}
                            }
                        }

                        if(diff0 + diff1 > 0)
                        {
                            temp -= (diff0 + diff1 -
                                     ABS( ABS(diff0) - ABS(diff1) ) / 2) / 2;
                        }
                        else
                        {
                            temp -= (diff0 + diff1 +
                                     ABS( ABS(diff0) - ABS(diff1) ) / 2) / 2;
                        }

                        filp[0] = dst[i][x + y*w] =
                            temp > 255U ? ~(temp>>31) : temp;
                    }
                    else
                    {
                        dst[i][x + y*w] =
                            pv->mcdeint_frame_dec->data[i][x + y*fils];
                    }
                }
            }
        }

        for( y = 0; y < h; y++ )
        {
            if( !((y ^ parity) & 1) )
            {
                for( x = 0; x < w; x++ )
                {
                    pv->mcdeint_frame_dec->data[i][x + y*fils] =
                        dst[i][x + y*w]= src[i][x + y*srcs];
                }
            }
        }
    }

#ifdef SUPPRESS_AV_LOG
    /* TODO: restore previous log level */
    av_log_set_level(loglevel);
#endif
}
