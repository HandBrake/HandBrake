/* $Id: decomb.c,v 1.14 2008/04/25 5:00:00 jbrjake Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. 
   
   The yadif algorithm was created by Michael Niedermayer. */
#include "hb.h"
#include "ffmpeg/avcodec.h"
#include "mpeg2dec/mpeg2.h"

#define SUPPRESS_AV_LOG

#define MODE_DEFAULT     4
#define PARITY_DEFAULT   -1

#define MCDEINT_MODE_DEFAULT   -1
#define MCDEINT_QP_DEFAULT      1

#define ABS(a) ((a) > 0 ? (a) : (-(a)))
#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

struct hb_filter_private_s
{
    int              pix_fmt;
    int              width[3];
    int              height[3];

    int              mode;
    int              parity;
    
    int              yadif_ready;

    int              mcdeint_mode;
    int              mcdeint_qp;

    int              mcdeint_outbuf_size;
    uint8_t        * mcdeint_outbuf;
    AVCodecContext * mcdeint_avctx_enc;
    AVFrame        * mcdeint_frame;
    AVFrame        * mcdeint_frame_dec;

    int              comb;
    int              color_equal;
    int              color_diff;
    int              threshold;
    int              prog_equal;
    int              prog_diff;
    int              prog_threshold;
    int              deinterlaced_frames;
    int              passed_frames;

    uint8_t        * ref[4][3];
    int              ref_stride[3];

    AVPicture        pic_in;
    AVPicture        pic_out;
    hb_buffer_t *    buf_out[2];
    hb_buffer_t *    buf_settings;
    
    int              cc_array[3][480][270];
    int              combed_macroblocks;
    int              uncombed_macroblocks;
};

hb_filter_private_t * hb_decomb_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings );

int hb_decomb_work( hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv );

void hb_decomb_close( hb_filter_private_t * pv );

hb_filter_object_t hb_filter_decomb =
{
    FILTER_DECOMB,
    "Decombs selectively with (ffmpeg or yadif/mcdeint or blending)",
    NULL,
    hb_decomb_init,
    hb_decomb_work,
    hb_decomb_close,
};

int cubic_interpolate( int y0, int y1, int y2, int y3 )
{
    /* From http://www.neuron2.net/library/cubicinterp.html */
    int result = ( y0 * -3 ) + ( y1 * 23 ) + ( y2 * 23 ) + ( y3 * -3 );
    result /= 40;
    
    if( result > 255 )
    {
        result = 255;
    }
    else if( result < 0 )
    {
        result = 0;
    }
    
    return result;
}

static void store_ref( const uint8_t ** pic,
                             hb_filter_private_t * pv )
{
    memcpy( pv->ref[3],
            pv->ref[0],
            sizeof(uint8_t *)*3 );

    memmove( pv->ref[0],
             pv->ref[1],
             sizeof(uint8_t *)*3*3 );

    int i;
    for( i = 0; i < 3; i++ )
    {
        const uint8_t * src = pic[i];
        uint8_t * ref = pv->ref[2][i];

        int w = pv->width[i];
        int h = pv->height[i];
        int ref_stride = pv->ref_stride[i];

        int y;
        for( y = 0; y < pv->height[i]; y++ )
        {
            memcpy(ref, src, w);
            src = (uint8_t*)src + w;
            ref = (uint8_t*)ref + ref_stride;
        }
    }
}

static void get_ref( uint8_t ** pic, hb_filter_private_t * pv, int frm )
{
    int i;
    for( i = 0; i < 3; i++ )
    {
        uint8_t * dst = pic[i];
        const uint8_t * ref = pv->ref[frm][i];
        int w = pv->width[i];
        int ref_stride = pv->ref_stride[i];
        
        int y;
        for( y = 0; y < pv->height[i]; y++ )
        {
            memcpy(dst, ref, w);
            dst += w;
            ref += ref_stride;
        }
    }
}

int blend_filter_pixel( int up2, int up1, int current, int down1, int down2 )
{
    /* Low-pass 5-tap filter */
    int result = 0;
    result += -up2;
    result += up1 * 2;
    result += current * 6;
    result += down1 *2;
    result += -down2;
    result /= 8;

    if( result > 255 )
    {
        result = 255;
    }
    if( result < 0 )
    {
        result = 0;
    }
    
    return result;
}

static void blend_filter_line( uint8_t *dst,
                               uint8_t *cur,
                               int plane,
                               int y,
                               hb_filter_private_t * pv )
{
    int w = pv->width[plane];
    int refs = pv->ref_stride[plane];
    int x;

    for( x = 0; x < w; x++)
    {
        int a, b, c, d, e;
        
        a = cur[-2*refs];
        b = cur[-refs];
        c = cur[0];
        d = cur[+refs];
        e = cur[2*refs];
        
        if( y == 0 )
        {
            /* First line, so A and B don't exist.*/
            a = cur[0];
            b = cur[0];
        }
        else if( y == 1 )
        {
            /* Second line, no A. */
            a = cur[-refs];
        }
        else if( y == (pv->height[plane] - 2) )
        {
            /* Second to last line, no E. */
            e = cur[+refs];
        }
        else if( y == (pv->height[plane] -1) )
        {
            /* Last line, no D or E. */
            d = cur[0];
            e = cur[0];
        }
                
        dst[0] = blend_filter_pixel( a, b, c, d, e );

        dst++;
        cur++;
    }
}

static void yadif_filter_line( uint8_t *dst,
                               uint8_t *prev,
                               uint8_t *cur,
                               uint8_t *next,
                               int plane,
                               int parity,
                               int y,
                               hb_filter_private_t * pv )
{
    uint8_t *prev2 = parity ? prev : cur ;
    uint8_t *next2 = parity ? cur  : next;

    int w = pv->width[plane];
    int refs = pv->ref_stride[plane];
    int x;
    int macroblock_x;
    int macroblock_y = y / 8 ;
    
    
    for( x = 0; x < w; x++)
    {

#if 0       
     /* Buggy experimental code for macroblock-by-macrobock comb detection.*/
        if(plane == 0 && pv->mode == 7)
        {
            if( !(x % 8))
                macroblock_x = x / 8;        
            
            if(pv->cc_array[plane][macroblock_x][macroblock_y] < 0 || pv->cc_array[plane][macroblock_x][macroblock_y] > 64)
            hb_log("[%i][%i] ( %i * %i )macroblock %i x %i is combed: %i", pv->deinterlaced_frames, plane, x, y, macroblock_x, macroblock_y, pv->cc_array[plane][macroblock_x][macroblock_y] );
            
            if(pv->cc_array[plane][macroblock_x][macroblock_y] == 0 && pv->cc_array[plane][macroblock_x+1][macroblock_y] == 0 && pv->cc_array[plane][macroblock_x-1][macroblock_y] == 0 && pv->cc_array[plane][macroblock_x][macroblock_y+1] == 0 && pv->cc_array[plane][macroblock_x][macroblock_y-1] == 0 )
            {
                dst[0] = cur[0];
                pv->uncombed_macroblocks++;
                goto end_of_yadif_filter_pixel;
            }
        }
        pv->combed_macroblocks++;
#endif
        /* Pixel above*/
        int c              = cur[-refs];
        /* Temporal average -- the current pixel location in the previous and next fields */
        int d              = (prev2[0] + next2[0])>>1;
        /* Pixel below */
        int e              = cur[+refs];
        
        /* How the current pixel changes from the field before to the field after */
        int temporal_diff0 = ABS(prev2[0] - next2[0]);
        /* The average of how much the pixels above and below change from the field before to now. */
        int temporal_diff1 = ( ABS(prev[-refs] - cur[-refs]) + ABS(prev[+refs] - cur[+refs]) ) >> 1;
        /* The average of how much the pixels above and below change from now to the next field. */
        int temporal_diff2 = ( ABS(next[-refs] - cur[-refs]) + ABS(next[+refs] - cur[+refs]) ) >> 1;
        /* For the actual difference, use the largest of the previous average diffs. */
        int diff           = MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
        
        /* SAD of how the pixel-1, the pixel, and the pixel+1 change from the line above to below. */ 
        int spatial_score  = ABS(cur[-refs-1] - cur[+refs-1]) + ABS(cur[-refs]-cur[+refs]) +
                                     ABS(cur[-refs+1] - cur[+refs+1]) - 1;         
        int spatial_pred;
         
        /* Spatial pred is either a bilinear or cubic vertical interpolation. */
        if( pv->mode >= 4  )
        {
            spatial_pred = cubic_interpolate( cur[-3*refs], cur[-refs], cur[+refs], cur[3*refs] );
        }
        else
        {
            spatial_pred = (c+e)>>1;
        }

/* EDDI: Edge Directed Deinterlacing Interpolation
   Uses the Martinez-Lim Line Shift Parametric Modeling algorithm...I think.
   Checks 4 different slopes to see if there is more similarity along a diagonal
   than there was vertically. If a diagonal is more similar, then it indicates
   an edge, so interpolate along that instead of a vertical line, using either
   linear or cubic interpolation depending on mode. */
#define YADIF_CHECK(j)\
        {   int score = ABS(cur[-refs-1+j] - cur[+refs-1-j])\
                      + ABS(cur[-refs  +j] - cur[+refs  -j])\
                      + ABS(cur[-refs+1+j] - cur[+refs+1-j]);\
            if( score < spatial_score ){\
                spatial_score = score;\
                if( pv->mode >= 4)\
                {\
                    switch(j)\
                    {\
                        case -1:\
                            spatial_pred = cubic_interpolate(cur[-3 * refs - 3], cur[-refs -1], cur[+refs + 1], cur[3* refs + 3] );\
                        break;\
                        case -2:\
                            spatial_pred = cubic_interpolate( ( ( cur[-3*refs - 4] + cur[-refs - 4] ) / 2 ) , cur[-refs -2], cur[+refs + 2], ( ( cur[3*refs + 4] + cur[refs + 4] ) / 2 ) );\
                        break;\
                        case 1:\
                            spatial_pred = cubic_interpolate(cur[-3 * refs +3], cur[-refs +1], cur[+refs - 1], cur[3* refs -3] );\
                        break;\
                        case 2:\
                            spatial_pred = cubic_interpolate(( ( cur[-3*refs + 4] + cur[-refs + 4] ) / 2 ), cur[-refs +2], cur[+refs - 2], ( ( cur[3*refs - 4] + cur[refs - 4] ) / 2 ) );\
                        break;\
                    }\
                }\
                else\
                {\
                    spatial_pred = ( cur[-refs +j] + cur[+refs -j] ) >>1;\
                }\
                
                YADIF_CHECK(-1) YADIF_CHECK(-2) }} }}
                YADIF_CHECK( 1) YADIF_CHECK( 2) }} }}
                                
        /* Temporally adjust the spatial prediction by comparing against the
           alternate (associated) fields in the previous and next frames. */
        int b = (prev2[-2*refs] + next2[-2*refs])>>1;
        int f = (prev2[+2*refs] + next2[+2*refs])>>1;
        
        /* Find the median value */
        int max = MAX3(d-e, d-c, MIN(b-c, f-e));
        int min = MIN3(d-e, d-c, MAX(b-c, f-e));
        diff = MAX3( diff, min, -max );
        
        if( spatial_pred > d + diff )
        {
            spatial_pred = d + diff;
        }
        else if( spatial_pred < d - diff )
        {
            spatial_pred = d - diff;
        }
        
        dst[0] = spatial_pred;
                        
end_of_yadif_filter_pixel:
        dst++;
        cur++;
        prev++;
        next++;
        prev2++;
        next2++;
    }
}

static void yadif_filter( uint8_t ** dst,
                          int parity,
                          int tff,
                          hb_filter_private_t * pv )
{

#if 0
    /* Buggy, experimental code for macroblock-by-macroblock decombing.*/
    if( pv->mode == 7 )
    {
        int x, y, block_x, block_y, plane, plane_width, plane_height, offset, cc;
        
        int stride = 0;
        int block = 8;
        int s[16];
        int color_diff = pv->color_diff;
        int color_equal = pv->color_equal;
        
        if ( pv->buf_settings->flags & 16 )
        {
            /* Frame is progressive, be more discerning. */
            color_diff = pv->prog_diff;
            color_equal = pv->prog_equal;
        }
        
        /* Iterate through planes */
        for( plane = 0; plane < 1; plane++ )
        {   
            plane_width =  pv->width[plane];
            plane_height = pv->height[plane];
        
            if( plane == 1 )
            {
                /* Y has already been checked, now offset by Y's dimensions
                   and divide all the other values by 2, since Cr and Cb
                   are half-size compared to Y.                               */
                stride = plane_width * plane_height;
            }
            else if ( plane == 2 )
            {
                /* Y and Cb are done, so the offset needs to be bumped
                   so it's width*height + (width / 2) * (height / 2)  */
                stride *= 5/4;
            }
            /* Grab a horizontal line */
            for(y = 0; y < plane_height; y += block )
            {
                uint8_t *line = &pv->ref[1][plane][ y*plane_width ];

                /* Iterate through it horizontally in blocks */
                for(x = 0; x < plane_width; x += block)
                {
                    /* Clear out the current macroblock mapping from the last frame. */
                    pv->cc_array[plane][x/block][y/block] = 0;
                    int sadA = 0;
                    int sadB = 0;
                    
                    /* Go through the block horizontally */
                    for(block_x = 0; block_x < block; block_x++)
                    {
                        /* Go through the block vertically, collecting pixels */
                        for(block_y = 0; block_y < block*2; block_y++)
                        {
                            s[block_y] = line[x+block_x+(block_y*plane_width)];
                        }

                        /* Now go through the results to check combing. */
                        for(block_y = 0; block_y < block; block_y++)
                        {
                            sadA += abs(s[block_y] - s[block_y+2]);
                            sadB += abs(s[block_y] - s[block_y+1]);
                            
//                            if( abs(s[block_y] - s[block_y+2]) < color_equal && abs(s[block_y] - s[block_y+1]) > color_diff)
//                            {
//                                pv->cc_array[plane][x/block][y/block]++;
//                            }
                        }
                    }
                    
                    if(sadA < sadB)
                    {
                        pv->cc_array[plane][x/block][y/block] = 1;
                    }
                    
                }
            }        
        }
    }

#if 0
/* Visualize macroblocks */    
    int x, y;
    fprintf(stderr, "FRAME %i VISUALIZATION\n", pv->deinterlaced_frames);
    for( y = 0; y < 60; y++ )
    {
        for( x = 0; x < 90; x++ )
        {
            if(pv->cc_array[0][x][y])
                fprintf(stderr, "X");
            else
                fprintf(stderr, "O");
                
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n\n");
#endif
#endif

    int i;
    for( i = 0; i < 3; i++ )
    {
        int w = pv->width[i];
        int h = pv->height[i];
        int ref_stride = pv->ref_stride[i];        
        
        int y;
        for( y = 0; y < h; y++ )
        {
            if( pv->mode == 3)
            {
                uint8_t *prev = &pv->ref[0][i][y*ref_stride];
                uint8_t *cur  = &pv->ref[1][i][y*ref_stride];
                uint8_t *next = &pv->ref[2][i][y*ref_stride];
                uint8_t *dst2 = &dst[i][y*w];

                blend_filter_line( dst2, cur, i, y, pv );
            }
            else if( (y ^ parity) &  1 )
            {
                uint8_t *prev = &pv->ref[0][i][y*ref_stride];
                uint8_t *cur  = &pv->ref[1][i][y*ref_stride];
                uint8_t *next = &pv->ref[2][i][y*ref_stride];
                uint8_t *dst2 = &dst[i][y*w];

                yadif_filter_line( dst2, prev, cur, next, i, parity ^ tff, y, pv );
            }
            else
            {
                memcpy( &dst[i][y*w],
                        &pv->ref[1][i][y*ref_stride],
                        w * sizeof(uint8_t) );              
            }
        }
    }
}

static void mcdeint_filter( uint8_t ** dst,
                            uint8_t ** src,
                            int parity,
                            hb_filter_private_t * pv )
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
        pv->mcdeint_frame->linesize[i] = pv->width[i];
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
        int w    = pv->width[i];
        int h    = pv->height[i];
        int fils = pv->mcdeint_frame_dec->linesize[i];
        int srcs = pv->width[i];

        for( y = 0; y < h; y++ )
        {
            if( (y ^ parity) & 1 )
            {
                for( x = 0; x < w; x++ )
                {
                    if( (x-2)+(y-1)*w >= 0 && (x+2)+(y+1)*w < w*h )
                    {
                        uint8_t * filp =
                            &pv->mcdeint_frame_dec->data[i][x + y*fils];
                        uint8_t * srcp = &src[i][x + y*srcs];

                        int diff0 = filp[-fils] - srcp[-srcs];
                        int diff1 = filp[+fils] - srcp[+srcs];

                        int spatial_score =
                              ABS(srcp[-srcs-1] - srcp[+srcs-1])
                            + ABS(srcp[-srcs  ] - srcp[+srcs  ])
                            + ABS(srcp[-srcs+1] - srcp[+srcs+1]) - 1;

                        int temp = filp[0];

#define MCDEINT_CHECK(j)\
                        {   int score = ABS(srcp[-srcs-1+j] - srcp[+srcs-1-j])\
                                      + ABS(srcp[-srcs  +j] - srcp[+srcs  -j])\
                                      + ABS(srcp[-srcs+1+j] - srcp[+srcs+1-j]);\
                            if( score < spatial_score ) {\
                                spatial_score = score;\
                                diff0 = filp[-fils+j] - srcp[-srcs+j];\
                                diff1 = filp[+fils-j] - srcp[+srcs-j];

                        MCDEINT_CHECK(-1) MCDEINT_CHECK(-2) }} }}
                        MCDEINT_CHECK( 1) MCDEINT_CHECK( 2) }} }}

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

hb_filter_private_t * hb_decomb_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings )
{
    if( pix_fmt != PIX_FMT_YUV420P )
    {
        return 0;
    }

    hb_filter_private_t * pv = calloc( 1, sizeof(struct hb_filter_private_s) );

    pv->pix_fmt = pix_fmt;

    pv->width[0]  = width;
    pv->height[0] = height;
    pv->width[1]  = pv->width[2]  = width >> 1;
    pv->height[1] = pv->height[2] = height >> 1;

    int buf_size = 3 * width * height / 2;
    pv->buf_out[0] = hb_buffer_init( buf_size );
    pv->buf_out[1] = hb_buffer_init( buf_size );
    pv->buf_settings = hb_buffer_init( 0 );

    pv->deinterlaced_frames = 0;
    pv->passed_frames = 0;
    pv->color_equal = 10;
    pv->color_diff = 15;
    pv->threshold = 9;
    pv->prog_equal = 10;
    pv->prog_diff = 35;
    pv->prog_threshold = 9;
    
    pv->combed_macroblocks = 0;
    pv->uncombed_macroblocks = 0;
    
    pv->yadif_ready    = 0;
    pv->mode     = MODE_DEFAULT;
    pv->parity   = PARITY_DEFAULT;

    pv->mcdeint_mode   = MCDEINT_MODE_DEFAULT;
    pv->mcdeint_qp     = MCDEINT_QP_DEFAULT;

    if( settings )
    {
        sscanf( settings, "%d:%d:%d:%d:%d:%d:%d",
                &pv->mode,
                &pv->color_equal,
                &pv->color_diff,
                &pv->threshold,
                &pv->prog_equal,
                &pv->prog_diff,
                &pv->prog_threshold );
    }
    
    if( pv->mode == 2 || pv->mode == 5 )
    {
        pv->mcdeint_mode = 0;
    }
    
    /* Allocate yadif specific buffers */
    if( pv->mode > 0 )
    {
        int i, j;
        for( i = 0; i < 3; i++ )
        {
            int is_chroma = !!i;
            int w = ((width   + 31) & (~31))>>is_chroma;
            int h = ((height+6+ 31) & (~31))>>is_chroma;

            pv->ref_stride[i] = w;

            for( j = 0; j < 3; j++ )
            {
                pv->ref[j][i] = malloc( w*h*sizeof(uint8_t) ) + 3*w;
            }
        }
    }

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
                    avctx_enc->me_method = ME_UMH;
                case 1:
                    avctx_enc->flags |= CODEC_FLAG_4MV;
                    avctx_enc->dia_size =2;
                case 0:
                    avctx_enc->flags |= CODEC_FLAG_QPEL;
            }

            avcodec_open(avctx_enc, enc);
        }

        pv->mcdeint_frame       = avcodec_alloc_frame();
        pv->mcdeint_outbuf_size = width * height * 10;
        pv->mcdeint_outbuf      = malloc( pv->mcdeint_outbuf_size );
    }

    return pv;
}

void hb_decomb_close( hb_filter_private_t * pv )
{
    if( !pv )
    {
        return;
    }
    
    if( pv->mode < 7 )
    {
        hb_log("decomb: deinterlaced %i | unfiltered %i | total %i", pv->deinterlaced_frames, pv->passed_frames, pv->deinterlaced_frames + pv->passed_frames);
    }
    else
    {
        hb_log("decomb macroblock: deinterlaced: %i | unfiltered %i | total %i", pv->combed_macroblocks, pv->uncombed_macroblocks, pv->combed_macroblocks + pv->uncombed_macroblocks);
    }

    /* Cleanup frame buffers */
    if( pv->buf_out[0] )
    {
        hb_buffer_close( &pv->buf_out[0] );
    }
    if( pv->buf_out[1] )
    {
        hb_buffer_close( &pv->buf_out[1] );
    }
    if (pv->buf_settings )
    {
        hb_buffer_close( &pv->buf_settings );
    }

    /* Cleanup yadif specific buffers */
    if( pv->mode > 0 )
    {
        int i;
        for( i = 0; i<3*3; i++ )
        {
            uint8_t **p = &pv->ref[i%3][i/3];
            if (*p)
            {
                free( *p - 3*pv->ref_stride[i/3] );
                *p = NULL;
            }
        }
    }

    /* Cleanup mcdeint specific buffers */
    if( pv->mcdeint_mode >= 0 )
    {
        if( pv->mcdeint_avctx_enc )
        {
            avcodec_close( pv->mcdeint_avctx_enc );
            av_freep( &pv->mcdeint_avctx_enc );
        }
        if( pv->mcdeint_outbuf )
        {
            free( pv->mcdeint_outbuf );
        }
    }

    free( pv );
}

int hb_decomb_work( hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv )
{
    if( !pv ||
        pix_fmt != pv->pix_fmt ||
        width   != pv->width[0] ||
        height  != pv->height[0] )
    {
        return FILTER_FAILED;
    }

    avpicture_fill( &pv->pic_in, buf_in->data,
                    pix_fmt, width, height );

    /* Use libavcodec deinterlace if mode == 0 */
    if( pv->mode == 0 )
    {
        avpicture_fill( &pv->pic_out, pv->buf_out[0]->data,
                        pix_fmt, width, height );

        /* Check for combing on the input frame */
        int interlaced =  hb_detect_comb(buf_in, width, height, pv->color_equal, pv->color_diff, pv->threshold, pv->prog_equal, pv->prog_diff, pv->prog_threshold);
        
        if(interlaced)
        {
            avpicture_deinterlace( &pv->pic_out, &pv->pic_in,
                                   pix_fmt, width, height );

            pv->deinterlaced_frames++;
            //hb_log("Frame %i is combed (Progressive: %s )", pv->deinterlaced_frames + pv->passed_frames, (buf_in->flags & 16) ? "Y" : "N");
            
            hb_buffer_copy_settings( pv->buf_out[0], buf_in );
            *buf_out = pv->buf_out[0];            
        }
        else
        {
            /* No combing detected, pass input frame through unmolested.*/
            
            pv->passed_frames++;
            
            hb_buffer_copy_settings( pv->buf_out[0], buf_in );
            *buf_out = buf_in;
            
        }

        return FILTER_OK;
    }
    
    /* Determine if top-field first layout */
    int tff;
    if( pv->parity < 0 )
    {
        tff = !!(buf_in->flags & PIC_FLAG_TOP_FIELD_FIRST);
    }
    else
    {
        tff = (pv->parity & 1) ^ 1;
    }

    /* Store current frame in yadif cache */
    store_ref( (const uint8_t**)pv->pic_in.data, pv );
    
    if( pv->mode < 7 )
    {
        /* Note down if the input frame is combed */
        pv->comb = (pv->comb << 1) | hb_detect_comb(buf_in, width, height, pv->color_equal, pv->color_diff, pv->threshold, pv->prog_equal, pv->prog_diff, pv->prog_threshold);
    }

    /* If yadif is not ready, store another ref and return FILTER_DELAY */
    if( pv->yadif_ready == 0 )
    {
        store_ref( (const uint8_t**)pv->pic_in.data, pv );

        hb_buffer_copy_settings( pv->buf_settings, buf_in );

        /* don't let 'work_loop' send a chapter mark upstream */
        buf_in->new_chap  = 0;

        pv->yadif_ready = 1;

        return FILTER_DELAY;
    }

    /* yadif works one frame behind so if the previous frame
     * had combing, deinterlace it otherwise just output it. */
    if( pv->mode == 7 ) // Experimental for macroblock decombing
    {
        /* Perform yadif filtering */
        
        pv->deinterlaced_frames++;
        int frame;
        for( frame = 0; frame <= ( ( pv->mode == 2 || pv->mode == 5 )? 1 : 0 ) ; frame++ )
        {
            int parity = frame ^ tff ^ 1;

            avpicture_fill( &pv->pic_out, pv->buf_out[!(frame^1)]->data,
                            pix_fmt, width, height );

            yadif_filter( pv->pic_out.data, parity, tff, pv );

            if( pv->mcdeint_mode >= 0 )
            {
                /* Perform mcdeint filtering */
                avpicture_fill( &pv->pic_in,  pv->buf_out[(frame^1)]->data,
                                pix_fmt, width, height );

                mcdeint_filter( pv->pic_in.data, pv->pic_out.data, parity, pv );
            }

            *buf_out = pv->buf_out[!(frame^1)];
        }
    }
    else if(  (pv->comb & 2 ) == 0 )
    {
        /* previous frame not interlaced - copy cached input frame to buf_out */
        
        pv->passed_frames++;
        
        avpicture_fill( &pv->pic_out,  pv->buf_out[0]->data, pix_fmt, width, height );
        get_ref( (uint8_t**)pv->pic_out.data, pv, 1 );
        *buf_out = pv->buf_out[0];
    }
    else
    {
        /* Perform yadif filtering */
        
        pv->deinterlaced_frames++;
        int frame;
        for( frame = 0; frame <= ( ( pv->mode == 2 || pv->mode == 5 )? 1 : 0 ) ; frame++ )
        {
            int parity = frame ^ tff ^ 1;

            avpicture_fill( &pv->pic_out, pv->buf_out[!(frame^1)]->data,
                            pix_fmt, width, height );

            yadif_filter( pv->pic_out.data, parity, tff, pv );

            if( pv->mcdeint_mode >= 0 )
            {
                /* Perform mcdeint filtering */
                avpicture_fill( &pv->pic_in,  pv->buf_out[(frame^1)]->data,
                                pix_fmt, width, height );

                mcdeint_filter( pv->pic_in.data, pv->pic_out.data, parity, pv );
            }

            *buf_out = pv->buf_out[!(frame^1)];
        }
    }

    /* Copy buffered settings to output buffer settings */
    hb_buffer_copy_settings( *buf_out, pv->buf_settings );

    /* Replace buffered settings with input buffer settings */
    hb_buffer_copy_settings( pv->buf_settings, buf_in );

    /* don't let 'work_loop' send a chapter mark upstream */
    buf_in->new_chap  = 0;

    return FILTER_OK;
}
