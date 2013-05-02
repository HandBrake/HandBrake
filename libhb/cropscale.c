/* cropscale.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
   
#include "hb.h"
#include "hbffmpeg.h"
#include "common.h"


struct hb_filter_private_s
{
    hb_job_t            *job;
    int                 width_in;
    int                 height_in;
    int                 pix_fmt;
    int                 pix_fmt_out;
    int                 width_out;
    int                 height_out;
    int                 crop[4];
    
#ifdef USE_OPENCL
    int                 use_dxva;
    int                 use_decomb;
    int                 use_detelecine;
    hb_oclscale_t       *os; //ocl scaler handler
#endif    
    struct SwsContext * context;
};

static int hb_crop_scale_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init );

static int hb_crop_scale_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out );

static int hb_crop_scale_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info );

static void hb_crop_scale_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_crop_scale =
{
    .id            = HB_FILTER_CROP_SCALE,
    .enforce_order = 1,
    .name          = "Crop and Scale",
    .settings      = NULL,
    .init          = hb_crop_scale_init,
    .work          = hb_crop_scale_work,
    .close         = hb_crop_scale_close,
    .info          = hb_crop_scale_info,
};

static int hb_crop_scale_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    // TODO: add pix format option to settings
    pv->job = init->job;
    pv->pix_fmt_out = init->pix_fmt;
    pv->width_in = init->width;
    pv->height_in = init->height;
    pv->width_out = init->width;
    pv->height_out = init->height;
#ifdef USE_OPENCL
    pv->use_dxva = init->use_dxva;
    pv->use_decomb = init->job->use_decomb;
    pv->use_detelecine = init->job->use_detelecine;

    if( pv->job->use_opencl )
    {
        pv->os = ( hb_oclscale_t * )malloc( sizeof( hb_oclscale_t ) );
        memset( pv->os, 0, sizeof( hb_oclscale_t ) );
    }
#endif
    memcpy( pv->crop, init->crop, sizeof( int[4] ) );
    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d:%d:%d:%d:%d",
                &pv->width_out, &pv->height_out,
                &pv->crop[0], &pv->crop[1], &pv->crop[2], &pv->crop[3] );
    }
    // Set init values so the next stage in the pipline
    // knows what it will be getting
    init->pix_fmt = pv->pix_fmt;
    init->width = pv->width_out;
    init->height = pv->height_out;
    memcpy( init->crop, pv->crop, sizeof( int[4] ) );
#ifdef USE_OPENCL
    pv->use_dxva = init->use_dxva;
#endif

    return 0;
}

static int hb_crop_scale_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
        return 0;

    // Set init values so the next stage in the pipline
    // knows what it will be getting
    memset( info, 0, sizeof( hb_filter_info_t ) );
    info->out.pix_fmt = pv->pix_fmt;
    info->out.width = pv->width_out;
    info->out.height = pv->height_out;
    memcpy( info->out.crop, pv->crop, sizeof( int[4] ) );

    int cropped_width = pv->width_in - ( pv->crop[2] + pv->crop[3] );
    int cropped_height = pv->height_in - ( pv->crop[0] + pv->crop[1] );

    sprintf( info->human_readable_desc, 
        "source: %d * %d, crop (%d/%d/%d/%d): %d * %d, scale: %d * %d",
        pv->width_in, pv->height_in,
        pv->crop[0], pv->crop[1], pv->crop[2], pv->crop[3],
        cropped_width, cropped_height, pv->width_out, pv->height_out );

    return 0;
}

static void hb_crop_scale_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if ( !pv )
    {
        return;
    }
#ifdef USE_OPENCL

    if( pv->job->use_opencl && pv->os )
    {
        CL_FREE( pv->os->h_in_buf );
        CL_FREE( pv->os->h_out_buf );
        CL_FREE( pv->os->v_out_buf );
        CL_FREE( pv->os->h_coeff_y );
        CL_FREE( pv->os->h_coeff_uv );
        CL_FREE( pv->os->h_index_y );
        CL_FREE( pv->os->h_index_uv );
        CL_FREE( pv->os->v_coeff_y );
        CL_FREE( pv->os->v_coeff_uv );
        CL_FREE( pv->os->v_index_y );
        CL_FREE( pv->os->v_index_uv );
        free( pv->os );
    }
#endif
    if( pv->context )
    {
        sws_freeContext( pv->context );
    }

    free( pv );
    filter->private_data = NULL;
}

#ifdef USE_OPENCL
static uint8_t *copy_plane( uint8_t *dst, uint8_t* src, int dstride, int sstride, int h )
{
    if( dstride == sstride )
    {
        memcpy( dst, src, dstride * h );
        return dst + dstride * h;
    }
    int lbytes = dstride <= sstride ? dstride : sstride;
    while( --h >= 0 )
    {
        memcpy( dst, src, lbytes );
        src += sstride;
        dst += dstride;
    }
    return dst;
}
#endif

static hb_buffer_t* crop_scale( hb_filter_private_t * pv, hb_buffer_t * in )
{
    AVPicture           pic_in;
    AVPicture           pic_out;
    AVPicture           pic_crop;
    hb_buffer_t * out;
    out = hb_video_buffer_init( pv->width_out, pv->height_out );

    hb_avpicture_fill( &pic_in, in );
    hb_avpicture_fill( &pic_out, out );

    // Crop; this alters the pointer to the data to point to the
    // correct place for cropped frame
    av_picture_crop( &pic_crop, &pic_in, in->f.fmt,
                     pv->crop[0], pv->crop[2] );

#ifdef USE_OPENCL

    if( pv->job->use_opencl )
    {
        int w = in->f.width - ( pv->crop[2] + pv->crop[3] );
        int h = in->f.height - ( pv->crop[0] + pv->crop[1] );
        uint8_t *tmp_in = malloc( w * h * 3 / 2 );
        uint8_t *tmp_out = malloc( pv->width_out * pv->height_out * 3 / 2 );
        if( pic_crop.data[0] || pic_crop.data[1] || pic_crop.data[2] || pic_crop.data[3] )
        {
            int i;
            for( i = 0; i < h >> 1; i++ )
            {
                memcpy( tmp_in + ( ( i << 1 ) + 0 ) * w, pic_crop.data[0] + ( ( i << 1 ) + 0 ) * pic_crop.linesize[0], w );
                memcpy( tmp_in + ( ( i << 1 ) + 1 ) * w, pic_crop.data[0] + ( ( i << 1 ) + 1 ) * pic_crop.linesize[0], w );
                memcpy( tmp_in + ( w * h ) + i * ( w >> 1 ), pic_crop.data[1] + i * pic_crop.linesize[1], w >> 1 );
                memcpy( tmp_in + ( w * h ) + ( ( w * h ) >> 2 ) + i * ( w >> 1 ), pic_crop.data[2] + i * pic_crop.linesize[2], w >> 1 );
            }
        }
        else
        {
            memcpy( tmp_in, pic_crop.data[0], w * h );
            memcpy( tmp_in + w * h, pic_crop.data[1], (w * h) >> 2 );
            memcpy( tmp_in + w * h + ((w * h) >> 2), pic_crop.data[2], (w * h) >> 2 );
        }
        hb_ocl_scale( NULL, tmp_in, tmp_out, w, h, out->f.width, out->f.height, pv->os );
        w = out->plane[0].stride;
        h = out->plane[0].height;
        uint8_t *dst = out->plane[0].data;
        copy_plane( dst, tmp_out, w, pv->width_out, h );
        w = out->plane[1].stride;
        h = out->plane[1].height;
        dst = out->plane[1].data;
        copy_plane( dst, tmp_out + pv->width_out * pv->height_out, w, pv->width_out >> 1, h );
        w = out->plane[2].stride;
        h = out->plane[2].height;
        dst = out->plane[2].data;
        copy_plane( dst, tmp_out + pv->width_out * pv->height_out +( ( pv->width_out * pv->height_out ) >> 2 ), w, pv->width_out >> 1, h );
        free( tmp_out );
        free( tmp_in );
    }
    else
    {
        if( !pv->context ||
            pv->width_in   != in->f.width  ||
            pv->height_in  != in->f.height ||
            pv->pix_fmt != in->f.fmt )
        {
            // Something changed, need a new scaling context.
            if( pv->context )
                sws_freeContext( pv->context );
                pv->context = hb_sws_get_context(
                                        in->f.width  - (pv->crop[2] + pv->crop[3]),
                                        in->f.height - (pv->crop[0] + pv->crop[1]),
                                        in->f.fmt,
                                        out->f.width, out->f.height, out->f.fmt,
                                        SWS_LANCZOS | SWS_ACCURATE_RND );
                pv->width_in = in->f.width;
                pv->height_in = in->f.height;
                pv->pix_fmt = in->f.fmt;
        }

        // Scale pic_crop into pic_render according to the
        // context set up above
        sws_scale(pv->context,
                  (const uint8_t* const*)pic_crop.data,
                  pic_crop.linesize,
                  0, in->f.height - (pv->crop[0] + pv->crop[1]),
                  pic_out.data,  pic_out.linesize);
    }
#else
    if ( !pv->context ||
         pv->width_in   != in->f.width  ||
         pv->height_in  != in->f.height ||
         pv->pix_fmt != in->f.fmt )
    {
        // Something changed, need a new scaling context.
        if( pv->context )
            sws_freeContext( pv->context );

        pv->context = hb_sws_get_context(
                                in->f.width  - (pv->crop[2] + pv->crop[3]),
                                in->f.height - (pv->crop[0] + pv->crop[1]),
                                in->f.fmt,
                                out->f.width, out->f.height, out->f.fmt,
                                SWS_LANCZOS | SWS_ACCURATE_RND );
        pv->width_in = in->f.width;
        pv->height_in = in->f.height;
        pv->pix_fmt = in->f.fmt;
    }

    // Scale pic_crop into pic_render according to the
    // context set up above
    sws_scale(pv->context,
              (const uint8_t* const*)pic_crop.data,
              pic_crop.linesize,
              0, in->f.height - (pv->crop[0] + pv->crop[1]),
              pic_out.data,  pic_out.linesize);
#endif
    out->s = in->s;
    hb_buffer_move_subs( out, in );
    return out;
}

static int hb_crop_scale_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    if ( in->size <= 0 )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    if ( !pv )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }

    // If width or height were not set, set them now based on the
    // input width & height
    if ( pv->width_out <= 0 || pv->height_out <= 0 )
    {
        pv->width_out = in->f.width - (pv->crop[2] + pv->crop[3]);
        pv->height_out = in->f.height - (pv->crop[0] + pv->crop[1]);
    }
#ifdef USE_OPENCL
    if ( (in->f.fmt == pv->pix_fmt_out &&
         !pv->crop[0] && !pv->crop[1] && !pv->crop[2] && !pv->crop[3] &&
         in->f.width == pv->width_out && in->f.height == pv->height_out) && 
         (pv->use_decomb == 0) && (pv->use_detelecine == 0) ||
         (pv->use_dxva && in->f.width == pv->width_out && in->f.height == pv->height_out) )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }
#else
    if ( in->f.fmt == pv->pix_fmt_out &&
         !pv->crop[0] && !pv->crop[1] && !pv->crop[2] && !pv->crop[3] &&
         in->f.width == pv->width_out && in->f.height == pv->height_out )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }
#endif

    *buf_out = crop_scale( pv, in );

    return HB_FILTER_OK;
}
