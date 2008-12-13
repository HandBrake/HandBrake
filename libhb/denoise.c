/*
 Copyright (C) 2003 Daniel Moreno <comac@comac.darktech.org>

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

#define HQDN3D_SPATIAL_LUMA_DEFAULT    4.0f
#define HQDN3D_SPATIAL_CHROMA_DEFAULT  3.0f
#define HQDN3D_TEMPORAL_LUMA_DEFAULT   6.0f

#define ABS(A) ( (A) > 0 ? (A) : -(A) )

struct hb_filter_private_s
{
    int              pix_fmt;
    int              width[3];
    int              height[3];

    int              hqdn3d_coef[4][512*16];
    unsigned int   * hqdn3d_line;
	unsigned short * hqdn3d_frame[3];

    AVPicture        pic_in;
    AVPicture        pic_out;
    hb_buffer_t    * buf_out;
};

hb_filter_private_t * hb_denoise_init( int pix_fmt,
                                       int width,
                                       int height,
                                       char * settings );

int hb_denoise_work( const hb_buffer_t * buf_in,
                     hb_buffer_t ** buf_out,
                     int pix_fmt,
                     int width,
                     int height,
                     hb_filter_private_t * pv );

void hb_denoise_close( hb_filter_private_t * pv );

hb_filter_object_t hb_filter_denoise =
{
    FILTER_DENOISE,
    "Denoise (hqdn3d)",
    NULL,
    hb_denoise_init,
    hb_denoise_work,
    hb_denoise_close,
};

static void hqdn3d_precalc_coef( int * ct,
                                 double dist25 )
{
    int i;
    double gamma, simil, c;

    gamma = log( 0.25 ) / log( 1.0 - dist25/255.0 - 0.00001 );

    for( i = -255*16; i <= 255*16; i++ )
    {
        simil = 1.0 - ABS(i) / (16*255.0);
        c = pow( simil, gamma ) * 65536.0 * (double)i / 16.0;
        ct[16*256+i] = (c<0) ? (c-0.5) : (c+0.5);
    }

    ct[0] = (dist25 != 0);
}

static inline unsigned int hqdn3d_lowpass_mul( unsigned int prev_mul,
                                               unsigned int curr_mul,
                                               int * coef )
{
    int diff_mul = prev_mul - curr_mul;
    int d = ((diff_mul+0x10007FF)>>12);
    return curr_mul + coef[d];
}

static void hqdn3d_denoise_temporal( unsigned char * frame_src,
                                     unsigned char * frame_dst,
                                     unsigned short * frame_ant,
                                     int w, int h,
                                     int * temporal)
{
    int x, y;
    unsigned int pixel_dst;

    for( y = 0; y < h; y++ )
    {
        for( x = 0; x < w; x++ )
        {
            pixel_dst = hqdn3d_lowpass_mul( frame_ant[x]<<8,
                                            frame_src[x]<<16,
                                            temporal );

            frame_ant[x] = ((pixel_dst+0x1000007F)>>8);
            frame_dst[x] = ((pixel_dst+0x10007FFF)>>16);
        }

        frame_src += w;
        frame_dst += w;
        frame_ant += w;
    }
}

static void hqdn3d_denoise_spatial( unsigned char * frame_src,
                                    unsigned char * frame_dst,
                                    unsigned int * line_ant,
                                    int w, int h,
                                    int * horizontal,
                                    int * vertical )
{
    int x, y;
    int line_offset_src = 0, line_offset_dst = 0;
    unsigned int pixel_ant;
    unsigned int pixel_dst;

    /* First pixel has no left nor top neighbor. */
    pixel_dst = line_ant[0] = pixel_ant = frame_src[0]<<16;
    frame_dst[0] = ((pixel_dst+0x10007FFF)>>16);

    /* First line has no top neighbor, only left. */
    for( x = 1; x < w; x++ )
    {
        pixel_dst = line_ant[x] = hqdn3d_lowpass_mul(pixel_ant,
                                                     frame_src[x]<<16,
                                                     horizontal);

        frame_dst[x] = ((pixel_dst+0x10007FFF)>>16);
    }

    for( y = 1; y < h; y++ )
    {
        unsigned int pixel_ant;
        line_offset_src += w, line_offset_dst += w;

        /* First pixel on each line doesn't have previous pixel */
        pixel_ant = frame_src[line_offset_src]<<16;

        pixel_dst = line_ant[0] = hqdn3d_lowpass_mul( line_ant[0],
                                                      pixel_ant,
                                                      vertical);

        frame_dst[line_offset_dst] = ((pixel_dst+0x10007FFF)>>16);

        /* The rest of the pixels in the line are normal */
        for( x = 1; x < w; x++ )
        {
            unsigned int pixel_dst;

            pixel_ant = hqdn3d_lowpass_mul( pixel_ant,
                                            frame_src[line_offset_src+x]<<16,
                                            horizontal );
            pixel_dst = line_ant[x] = hqdn3d_lowpass_mul( line_ant[x],
                                                          pixel_ant,
                                                          vertical );

            frame_dst[line_offset_dst+x]= ((pixel_dst+0x10007FFF)>>16);
        }
    }
}

static void hqdn3d_denoise( unsigned char * frame_src,
                            unsigned char * frame_dst,
                            unsigned int * line_ant,
                            unsigned short ** frame_ant_ptr,
                            int w,
                            int h,
                            int * horizontal,
                            int * vertical,
                            int * temporal)
{
    int x, y;
    int line_offset_src = 0, line_offset_dst = 0;
    unsigned int pixel_ant;
    unsigned int pixel_dst;
    unsigned short* frame_ant = (*frame_ant_ptr);

    if( !frame_ant)
    {
        (*frame_ant_ptr) = frame_ant = malloc( w*h*sizeof(unsigned short) );
        for( y = 0; y < h; y++ )
        {
            unsigned short* dst = &frame_ant[y*w];
            unsigned char*  src = frame_src + y*w;

            for( x = 0; x < w; x++ )
            {
                dst[x] = src[x] << 8;
            }
        }
    }

    /* If no spatial coefficients, do temporal denoise only */
    if( !horizontal[0] && !vertical[0] )
    {
        hqdn3d_denoise_temporal( frame_src,
                                 frame_dst,
                                 frame_ant,
                                 w, h,
                                 temporal);
        return;
    }

    /* If no temporal coefficients, do spatial denoise only */
    if( !temporal[0] )
    {
        hqdn3d_denoise_spatial( frame_src,
                                frame_dst,
                                line_ant,
                                w, h,
                                horizontal,
                                vertical);
        return;
    }

    /* First pixel has no left nor top neighbor. Only previous frame */
    line_ant[0]  = pixel_ant = frame_src[0] << 16;

    pixel_dst    = hqdn3d_lowpass_mul( frame_ant[0]<<8,
                                       pixel_ant,
                                       temporal );

    frame_ant[0] = ((pixel_dst+0x1000007F)>>8);
    frame_dst[0] = ((pixel_dst+0x10007FFF)>>16);

    /* First line has no top neighbor. Only left one for each pixel and last frame */
    for( x = 1; x < w; x++ )
    {
        line_ant[x]  = pixel_ant = hqdn3d_lowpass_mul( pixel_ant,
                                                       frame_src[x]<<16,
                                                       horizontal);

        pixel_dst    = hqdn3d_lowpass_mul( frame_ant[x]<<8,
                                           pixel_ant,
                                           temporal);

        frame_ant[x] = ((pixel_dst+0x1000007F)>>8);
        frame_dst[x] = ((pixel_dst+0x10007FFF)>>16);
    }

    /* The rest of the lines in the frame are normal */
    for( y = 1; y < h; y++ )
    {
        unsigned int pixel_ant;
        unsigned short * line_prev = &frame_ant[y*w];
        line_offset_src += w, line_offset_dst += w;

        /* First pixel on each line doesn't have previous pixel */
        pixel_ant    = frame_src[line_offset_src]<<16;
        line_ant[0]  = hqdn3d_lowpass_mul( line_ant[0],
                                           pixel_ant,
                                           vertical);
        pixel_dst    = hqdn3d_lowpass_mul( line_prev[0]<<8,
                                           line_ant[0],
                                           temporal);
        line_prev[0] = ((pixel_dst+0x1000007F)>>8);

        frame_dst[line_offset_dst] = ((pixel_dst+0x10007FFF)>>16);

        /* The rest of the pixels in the line are normal */
        for( x = 1; x < w; x++ )
        {
            unsigned int pixel_dst;
            pixel_ant    = hqdn3d_lowpass_mul( pixel_ant,
                                               frame_src[line_offset_src+x]<<16,
                                               horizontal );
            line_ant[x]  = hqdn3d_lowpass_mul( line_ant[x],
                                               pixel_ant, vertical);
            pixel_dst    = hqdn3d_lowpass_mul( line_prev[x]<<8,
                                               line_ant[x],
                                               temporal );
            line_prev[x] = ((pixel_dst+0x1000007F)>>8);

            frame_dst[line_offset_dst+x] = ((pixel_dst+0x10007FFF)>>16);
        }
    }
}

hb_filter_private_t * hb_denoise_init( int pix_fmt,
                                       int width,
                                       int height,
                                       char * settings )
{
    if( pix_fmt != PIX_FMT_YUV420P )
    {
        return 0;
    }

    hb_filter_private_t * pv = malloc( sizeof(struct hb_filter_private_s) );

    /*
     * Clear the memory to avoid freeing uninitialised memory later.
     */
    memset( pv, 0, sizeof( struct hb_filter_private_s ) );

    pv->pix_fmt  = pix_fmt;
    pv->width[0]  = width;
    pv->height[0] = height;
    pv->width[1]  = pv->width[2] = width >> 1;
    pv->height[1] = pv->height[2] = height >> 1;

    double spatial_luma, temporal_luma, spatial_chroma, temporal_chroma;

    if( settings )
    {
        switch( sscanf( settings, "%lf:%lf:%lf:%lf",
                        &spatial_luma, &spatial_chroma,
                        &temporal_luma, &temporal_chroma ) )
        {
            case 0:
                spatial_luma    = HQDN3D_SPATIAL_LUMA_DEFAULT;

                spatial_chroma  = HQDN3D_SPATIAL_CHROMA_DEFAULT;

                temporal_luma   = HQDN3D_TEMPORAL_LUMA_DEFAULT;

                temporal_chroma = temporal_luma *
                                  spatial_chroma / spatial_luma;
                break;

            case 1:
                spatial_chroma = HQDN3D_SPATIAL_CHROMA_DEFAULT *
                                 spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;

                temporal_luma   = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                                  spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;

                temporal_chroma = temporal_luma *
                                  spatial_chroma / spatial_luma;
                break;

            case 2:
                temporal_luma   = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                                  spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;

                temporal_chroma = temporal_luma *
                                  spatial_chroma / spatial_luma;
                break;

            case 3:
                temporal_chroma = temporal_luma *
                                  spatial_chroma / spatial_luma;
                break;
        }
    }

    pv->hqdn3d_line = malloc( width * sizeof(int) );

    hqdn3d_precalc_coef( pv->hqdn3d_coef[0], spatial_luma );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[1], temporal_luma );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[2], spatial_chroma );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[3], temporal_chroma );

    pv->buf_out = hb_video_buffer_init( width, height );

    return pv;
}

void hb_denoise_close( hb_filter_private_t * pv )
{
    if( !pv )
    {
        return;
    }

	if( pv->hqdn3d_line )
    {
        free( pv->hqdn3d_line );
        pv->hqdn3d_line = NULL;
    }
	if( pv->hqdn3d_frame[0] )
    {
        free( pv->hqdn3d_frame[0] );
        pv->hqdn3d_frame[0] = NULL;
    }
	if( pv->hqdn3d_frame[1] )
    {
        free( pv->hqdn3d_frame[1] );
        pv->hqdn3d_frame[1] = NULL;
    }
	if( pv->hqdn3d_frame[2] )
    {
        free( pv->hqdn3d_frame[2] );
        pv->hqdn3d_frame[2] = NULL;
    }
    if( pv->buf_out )
    {
        hb_buffer_close( &pv->buf_out );
    }

    free( pv );
}

int hb_denoise_work( const hb_buffer_t * buf_in,
                     hb_buffer_t ** buf_out,
                     int pix_fmt,
                     int width,
                     int height,
                     hb_filter_private_t * pv )
{
    if( !pv ||
        pix_fmt != pv->pix_fmt ||
        width != pv->width[0] ||
        height != pv->height[0] )
    {
        return FILTER_FAILED;
    }

    avpicture_fill( &pv->pic_in, buf_in->data,
                    pix_fmt, width, height );

    avpicture_fill( &pv->pic_out, pv->buf_out->data,
                    pix_fmt, width, height );

    hqdn3d_denoise( pv->pic_in.data[0],
                    pv->pic_out.data[0],
                    pv->hqdn3d_line,
                    &pv->hqdn3d_frame[0],
                    pv->width[0],
                    pv->height[0],
                    pv->hqdn3d_coef[0],
                    pv->hqdn3d_coef[0],
                    pv->hqdn3d_coef[1] );

    hqdn3d_denoise( pv->pic_in.data[1],
                    pv->pic_out.data[1],
                    pv->hqdn3d_line,
                    &pv->hqdn3d_frame[1],
                    pv->width[1],
                    pv->height[1],
                    pv->hqdn3d_coef[2],
                    pv->hqdn3d_coef[2],
                    pv->hqdn3d_coef[3] );

    hqdn3d_denoise( pv->pic_in.data[2],
                    pv->pic_out.data[2],
                    pv->hqdn3d_line,
                    &pv->hqdn3d_frame[2],
                    pv->width[2],
                    pv->height[2],
                    pv->hqdn3d_coef[2],
                    pv->hqdn3d_coef[2],
                    pv->hqdn3d_coef[3] );

    hb_buffer_copy_settings( pv->buf_out, buf_in );

    *buf_out = pv->buf_out;

    return FILTER_OK;
}
