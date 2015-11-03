/*
 Copyright (c) 2003 Daniel Moreno <comac AT comac DOT darktech DOT org>
 Copyright (c) 2012 Loren Merritt

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

#define HQDN3D_SPATIAL_LUMA_DEFAULT    4.0f
#define HQDN3D_SPATIAL_CHROMA_DEFAULT  3.0f
#define HQDN3D_TEMPORAL_LUMA_DEFAULT   6.0f

struct hb_filter_private_s
{
    short            hqdn3d_coef[6][512*16];
    unsigned short * hqdn3d_line;
    unsigned short * hqdn3d_frame[3];
};

static int hb_denoise_init( hb_filter_object_t * filter,
                            hb_filter_init_t * init );

static int hb_denoise_work( hb_filter_object_t * filter,
                            hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out );

static void hb_denoise_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_denoise =
{
    .id            = HB_FILTER_DENOISE,
    .enforce_order = 1,
    .name          = "Denoise (hqdn3d)",
    .settings      = NULL,
    .init          = hb_denoise_init,
    .work          = hb_denoise_work,
    .close         = hb_denoise_close,
};

static void hqdn3d_precalc_coef( short * ct,
                                 double dist25 )
{
    int i;
    double gamma, simil, c;

    gamma = log( 0.25 ) / log( 1.0 - MIN(dist25,252.0)/255.0 - 0.00001 );

    for( i = -255*16; i <= 255*16; i++ )
    {
        /* hqdn3d_lowpass_mul() truncates (not rounds) the diff, use +15/32 as midpoint */
        double f = (i + 15.0/32.0) / 16.0;
        simil = 1.0 - ABS(f) / 255.0;
        c = pow(simil, gamma) * 256.0 * f;
        ct[16*256+i] = (c<0) ? (c-0.5) : (c+0.5);
    }

    ct[0] = (dist25 != 0);
}

static inline unsigned int hqdn3d_lowpass_mul( int prev_mul,
                                               int curr_mul,
                                               short * coef )
{
    int d = (prev_mul - curr_mul)>>4;
    return curr_mul + coef[d];
}

static void hqdn3d_denoise_temporal( unsigned char * frame_src,
                                     unsigned char * frame_dst,
                                     unsigned short * frame_ant,
                                     int w, int h,
                                     short * temporal)
{
    int x, y;
    unsigned int tmp;

    temporal += 0x1000;

    for( y = 0; y < h; y++ )
    {
        for( x = 0; x < w; x++ )
        {
            frame_ant[x] = tmp = hqdn3d_lowpass_mul( frame_ant[x],
                                                     frame_src[x]<<8,
                                                     temporal );
            frame_dst[x] = (tmp+0x7F)>>8;
        }

        frame_src += w;
        frame_dst += w;
        frame_ant += w;
    }
}

static void hqdn3d_denoise_spatial( unsigned char * frame_src,
                                    unsigned char * frame_dst,
                                    unsigned short * line_ant,
                                    unsigned short * frame_ant,
                                    int w, int h,
                                    short * spatial,
                                    short * temporal )
{
    int x, y;
    unsigned int pixel_ant;
    unsigned int tmp;

    spatial  += 0x1000;
    temporal += 0x1000;

    /* First line has no top neighbor. Only left one for each tmp and last frame */
    pixel_ant = frame_src[0]<<8;
    for ( x = 0; x < w; x++)
    {
        line_ant[x] = tmp = pixel_ant = hqdn3d_lowpass_mul( pixel_ant,
                                                            frame_src[x]<<8,
                                                            spatial );
        frame_ant[x] = tmp = hqdn3d_lowpass_mul( frame_ant[x],
                                                 tmp,
                                                 temporal );
        frame_dst[x] = (tmp+0x7F)>>8;
    }

    for( y = 1; y < h; y++ )
    {
        frame_src += w;
        frame_dst += w;
        frame_ant += w;
        pixel_ant = frame_src[0]<<8;
        for ( x = 0; x < w-1; x++ )
        {
            line_ant[x] = tmp =  hqdn3d_lowpass_mul( line_ant[x],
                                                     pixel_ant,
                                                     spatial );
            pixel_ant =          hqdn3d_lowpass_mul( pixel_ant,
                                                     frame_src[x+1]<<8,
                                                     spatial );
            frame_ant[x] = tmp = hqdn3d_lowpass_mul( frame_ant[x],
                                                     tmp,
                                                     temporal );
            frame_dst[x] = (tmp+0x7F)>>8;
        }
        line_ant[x] = tmp =  hqdn3d_lowpass_mul( line_ant[x],
                                                 pixel_ant,
                                                 spatial );
        frame_ant[x] = tmp = hqdn3d_lowpass_mul( frame_ant[x],
                                                 tmp,
                                                 temporal );
        frame_dst[x] = (tmp+0x7F)>>8;
    }
}

static void hqdn3d_denoise( unsigned char * frame_src,
                            unsigned char * frame_dst,
                            unsigned short * line_ant,
                            unsigned short ** frame_ant_ptr,
                            int w,
                            int h,
                            short * spatial,
                            short * temporal )
{
    int x, y;
    unsigned short* frame_ant = (*frame_ant_ptr);

    if( !frame_ant)
    {
        unsigned char * src = frame_src;
        (*frame_ant_ptr) = frame_ant = malloc( w*h*sizeof(unsigned short) );
        for ( y = 0; y < h; y++, frame_src += w, frame_ant += w )
        {
            for( x = 0; x < w; x++ )
            {
                frame_ant[x] = frame_src[x]<<8;
            }
        }
        frame_src = src;
        frame_ant = *frame_ant_ptr;
    }

    /* If no spatial coefficients, do temporal denoise only */
    if( spatial[0] )
    {
        hqdn3d_denoise_spatial( frame_src,
                                frame_dst,
                                line_ant,
                                frame_ant,
                                w, h,
                                spatial,
                                temporal );
    }
    else
    {
        hqdn3d_denoise_temporal( frame_src,
                                 frame_dst,
                                 frame_ant,
                                 w, h,
                                 temporal);
    }
}

static int hb_denoise_init( hb_filter_object_t * filter,
                            hb_filter_init_t * init )
{
    filter->private_data = calloc( sizeof(struct hb_filter_private_s), 1 );
    hb_filter_private_t * pv = filter->private_data;

    double spatial_luma      = 0.0f,
           spatial_chroma_b  = 0.0f,
           spatial_chroma_r  = 0.0f,
           temporal_luma     = 0.0f,
           temporal_chroma_b = 0.0f,
           temporal_chroma_r = 0.0f;

    if (filter->settings != NULL)
    {
        switch( sscanf( filter->settings, "%lf:%lf:%lf:%lf:%lf:%lf",
                        &spatial_luma, &spatial_chroma_b, &spatial_chroma_r,
                        &temporal_luma, &temporal_chroma_b, &temporal_chroma_r ) )
        {
            case 0:
                spatial_luma      = HQDN3D_SPATIAL_LUMA_DEFAULT;
                spatial_chroma_b  = HQDN3D_SPATIAL_CHROMA_DEFAULT;
                spatial_chroma_r  = spatial_chroma_b;
                temporal_luma     = HQDN3D_TEMPORAL_LUMA_DEFAULT;
                temporal_chroma_b = temporal_luma *
                                    spatial_chroma_b / spatial_luma;
                temporal_chroma_r = temporal_chroma_b;
                break;

            case 1:
                spatial_chroma_b  = HQDN3D_SPATIAL_CHROMA_DEFAULT *
                                    spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
                spatial_chroma_r  = spatial_chroma_b;
                temporal_luma     = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                                    spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
                temporal_chroma_b = temporal_luma *
                                    spatial_chroma_b / spatial_luma;
                temporal_chroma_r = temporal_chroma_b;
                break;

            case 2:
                spatial_chroma_r  = spatial_chroma_b;
                temporal_luma     = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                                    spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
                temporal_chroma_b = temporal_luma *
                                    spatial_chroma_b / spatial_luma;
                temporal_chroma_r = temporal_chroma_b;
                break;

            case 3:
                temporal_luma     = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                                    spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
                temporal_chroma_b = temporal_luma *
                                    spatial_chroma_b / spatial_luma;
                temporal_chroma_r = temporal_chroma_b;
                break;

            case 4:
                temporal_chroma_b = temporal_luma *
                                    spatial_chroma_b / spatial_luma;
                temporal_chroma_r = temporal_chroma_b;
                break;

            case 5:
                temporal_chroma_r = temporal_chroma_b;
                break;
        }
    }

    hqdn3d_precalc_coef( pv->hqdn3d_coef[0], spatial_luma );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[1], temporal_luma );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[2], spatial_chroma_b );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[3], temporal_chroma_b );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[4], spatial_chroma_r );
    hqdn3d_precalc_coef( pv->hqdn3d_coef[5], temporal_chroma_r );

    return 0;
}

static void hb_denoise_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

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

    free( pv );
    filter->private_data = NULL;
}

static int hb_denoise_work( hb_filter_object_t * filter,
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

    out = hb_video_buffer_init( in->f.width, in->f.height );

    if( !pv->hqdn3d_line )
    {
        pv->hqdn3d_line = malloc( in->plane[0].stride * sizeof(unsigned short) );
    }

    int c, coef_index;

    for ( c = 0; c < 3; c++ )
    {
        coef_index = c * 2;
        hqdn3d_denoise( in->plane[c].data,
                        out->plane[c].data,
                        pv->hqdn3d_line,
                        &pv->hqdn3d_frame[c],
                        in->plane[c].stride,
                        in->plane[c].height,
                        pv->hqdn3d_coef[coef_index],
                        pv->hqdn3d_coef[coef_index+1] );
    }

    out->s = in->s;
    *buf_out = out;

    return HB_FILTER_OK;
}
