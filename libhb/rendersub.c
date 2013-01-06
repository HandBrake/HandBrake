/* rendersub.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
#include "hb.h"
#include "hbffmpeg.h"
#include <ass/ass.h>

struct hb_filter_private_s
{
    // Common
    int               crop[4];
    int               type;

    // VOBSUB
    hb_list_t       * sub_list; // List of active subs

    // SSA
    ASS_Library     * ssa;
    ASS_Renderer    * renderer;
    ASS_Track       * ssaTrack;
};

// VOBSUB
static int vobsub_init( hb_filter_object_t * filter, hb_filter_init_t * init );

static int vobsub_work( hb_filter_object_t * filter,
                        hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out );

static void vobsub_close( hb_filter_object_t * filter );


// SSA
static int ssa_init( hb_filter_object_t * filter, hb_filter_init_t * init );

static int ssa_work( hb_filter_object_t * filter,
                     hb_buffer_t ** buf_in,
                     hb_buffer_t ** buf_out );

static void ssa_close( hb_filter_object_t * filter );


// PGS
static int pgssub_init ( hb_filter_object_t * filter, hb_filter_init_t * init );

static int pgssub_work ( hb_filter_object_t * filter,
                      hb_buffer_t ** buf_in,
                      hb_buffer_t ** buf_out );

static void pgssub_close( hb_filter_object_t * filter );


// Entry points
static int hb_rendersub_init( hb_filter_object_t * filter,
                                 hb_filter_init_t * init );

static int hb_rendersub_work( hb_filter_object_t * filter,
                                 hb_buffer_t ** buf_in,
                                 hb_buffer_t ** buf_out );

static void hb_rendersub_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_render_sub =
{
    .id            = HB_FILTER_RENDER_SUB,
    .enforce_order = 1,
    .name          = "Subtitle renderer",
    .settings      = NULL,
    .init          = hb_rendersub_init,
    .work          = hb_rendersub_work,
    .close         = hb_rendersub_close,
};

static void blend( hb_buffer_t *dst, hb_buffer_t *src, int left, int top )
{
    int xx, yy;
    int ww, hh;
    int x0, y0;
    uint8_t *y_in, *y_out;
    uint8_t *u_in, *u_out;
    uint8_t *v_in, *v_out;
    uint8_t *a_in, alpha;

    x0 = y0 = 0;
    if( left < 0 )
    {
        x0 = -left;
    }
    if( top < 0 )
    {
        y0 = -top;
    }

    ww = src->f.width;
    if( src->f.width - x0 > dst->f.width - left )
    {
        ww = dst->f.width - left + x0;
    }
    hh = src->f.height;
    if( src->f.height - y0 > dst->f.height - top )
    {
        hh = dst->f.height - top + y0;
    }
    // Blend luma
    for( yy = y0; yy < hh; yy++ )
    {
        y_in   = src->plane[0].data + yy * src->plane[0].stride;
        y_out   = dst->plane[0].data + ( yy + top ) * dst->plane[0].stride;
        a_in = src->plane[3].data + yy * src->plane[3].stride;
        for( xx = x0; xx < ww; xx++ )
        {
            alpha = a_in[xx];
            /*
             * Merge the luminance and alpha with the picture
             */
            y_out[left + xx] = 
                ( (uint16_t)y_out[left + xx] * ( 255 - alpha ) +
                     (uint16_t)y_in[xx] * alpha ) >> 8;
        }
    }

    // Blend U & V
    // Assumes source and dest are the same PIX_FMT
    int hshift = 0;
    int wshift = 0;
    if( dst->plane[1].height < dst->plane[0].height )
        hshift = 1;
    if( dst->plane[1].width < dst->plane[0].width )
        wshift = 1;

    for( yy = y0 >> hshift; yy < hh >> hshift; yy++ )
    {
        u_in = src->plane[1].data + yy * src->plane[1].stride;
        u_out = dst->plane[1].data + ( yy + ( top >> hshift ) ) * dst->plane[1].stride;
        v_in = src->plane[2].data + yy * src->plane[2].stride;
        v_out = dst->plane[2].data + ( yy + ( top >> hshift ) ) * dst->plane[2].stride;
        a_in = src->plane[3].data + ( yy << hshift ) * src->plane[3].stride;

        for( xx = x0 >> wshift; xx < ww >> wshift; xx++ )
        {
            alpha = a_in[xx << wshift];

            // Blend averge U and alpha
            u_out[(left >> wshift) + xx] =
                ( (uint16_t)u_out[(left >> wshift) + xx] * ( 255 - alpha ) +
                  (uint16_t)u_in[xx] * alpha ) >> 8;

            // Blend V and alpha
            v_out[(left >> wshift) + xx] =
                ( (uint16_t)v_out[(left >> wshift) + xx] * ( 255 - alpha ) +
                  (uint16_t)v_in[xx] * alpha ) >> 8;
        }
    }
}

// Assumes that the input buffer has the same dimensions
// as the original title diminsions
static void ApplySub( hb_filter_private_t * pv, hb_buffer_t * buf, hb_buffer_t * sub )
{
    int top, left, margin_top, margin_percent;

    if ( !pv->ssa )
    {
        /*
         * Percent of height of picture that form a margin that subtitles
         * should not be displayed within.
         */
        margin_percent = 2;

        /*
         * If necessary, move the subtitle so it is not in a cropped zone.
         * When it won't fit, we center it so we lose as much on both ends.
         * Otherwise we try to leave a 20px or 2% margin around it.
         */
        margin_top = ( ( buf->f.height - pv->crop[0] - pv->crop[1] ) *
                       margin_percent ) / 100;

        if( margin_top > 20 )
        {
            /*
             * A maximum margin of 20px regardless of height of the picture.
             */
            margin_top = 20;
        }

        if( sub->f.height > buf->f.height - pv->crop[0] - pv->crop[1] -
            ( margin_top * 2 ) )
        {
            /*
             * The subtitle won't fit in the cropped zone, so center
             * it vertically so we fit in as much as we can.
             */
            top = pv->crop[0] + ( buf->f.height - pv->crop[0] -
                                          pv->crop[1] - sub->f.height ) / 2;
        }
        else if( sub->f.y < pv->crop[0] + margin_top )
        {
            /*
             * The subtitle fits in the cropped zone, but is currently positioned
             * within our top margin, so move it outside of our margin.
             */
            top = pv->crop[0] + margin_top;
        }
        else if( sub->f.y > buf->f.height - pv->crop[1] - margin_top - sub->f.height )
        {
            /*
             * The subtitle fits in the cropped zone, and is not within the top
             * margin but is within the bottom margin, so move it to be above
             * the margin.
             */
            top = buf->f.height - pv->crop[1] - margin_top - sub->f.height;
        }
        else
        {
            /*
             * The subtitle is fine where it is.
             */
            top = sub->f.y;
        }

        if( sub->f.width > buf->f.width - pv->crop[2] - pv->crop[3] - 40 )
            left = pv->crop[2] + ( buf->f.width - pv->crop[2] -
                    pv->crop[3] - sub->f.width ) / 2;
        else if( sub->f.x < pv->crop[2] + 20 )
            left = pv->crop[2] + 20;
        else if( sub->f.x > buf->f.width - pv->crop[3] - 20 - sub->f.width )
            left = buf->f.width - pv->crop[3] - 20 - sub->f.width;
        else
            left = sub->f.x;
    }
    else
    {
        top = sub->f.y;
        left = sub->f.x;
    }

    blend( buf, sub, left, top );
}

// Assumes that the input buffer has the same dimensions
// as the original title diminsions
static void ApplyVOBSubs( hb_filter_private_t * pv, hb_buffer_t * buf )
{
    int ii;
    hb_buffer_t *sub, *next;

    for( ii = 0; ii < hb_list_count(pv->sub_list); )
    {
        sub = hb_list_item( pv->sub_list, ii );
        if (ii + 1 < hb_list_count(pv->sub_list))
            next = hb_list_item( pv->sub_list, ii + 1 );
        else
            next = NULL;

        if ((sub->s.stop != -1 && sub->s.stop <= buf->s.start) ||
            (next != NULL && sub->s.stop == -1 && next->s.start <= buf->s.start))
        {
            // Subtitle stop is in the past, delete it
            hb_list_rem( pv->sub_list, sub );
            hb_buffer_close( &sub );
        }
        else if( sub->s.start <= buf->s.start )
        {
            // The subtitle has started before this frame and ends
            // after it.  Render the subtitle into the frame.
            while ( sub )
            {
                ApplySub( pv, buf, sub );
                sub = sub->next;
            }
            ii++;
        }
        else
        {
            // The subtitle starts in the future.  No need to continue.
            break;
        }
    }
}

static int vobsub_init( hb_filter_object_t * filter,
                        hb_filter_init_t * init )
{
    hb_filter_private_t * pv = filter->private_data;

    pv->sub_list = hb_list_init();

    return 0;
}

static void vobsub_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    if( pv->sub_list )
        hb_list_empty( &pv->sub_list );

    free( pv );
    filter->private_data = NULL;
}

static int vobsub_work( hb_filter_object_t * filter,
                        hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * sub;

    if ( in->size <= 0 )
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while( ( sub = hb_fifo_get( filter->subtitle->fifo_out ) ) )
    {
        hb_list_add( pv->sub_list, sub );
    }

    ApplyVOBSubs( pv, in );
    *buf_in = NULL;
    *buf_out = in;
    
    return HB_FILTER_OK;
}

static uint8_t ssaAlpha( ASS_Image *frame, int x, int y )
{
    unsigned frameA = ( frame->color ) & 0xff;
    unsigned gliphA = frame->bitmap[y*frame->stride + x];

    // Alpha for this pixel is the frame opacity (255 - frameA)
    // multiplied by the gliph alfa (gliphA) for this pixel
    unsigned alpha = (255 - frameA) * gliphA >> 8;

    return (uint8_t)alpha;
}

static hb_buffer_t * RenderSSAFrame( hb_filter_private_t * pv, ASS_Image * frame )
{
    hb_buffer_t *sub;
    int xx, yy;

    unsigned r = ( frame->color >> 24 ) & 0xff;
    unsigned g = ( frame->color >> 16 ) & 0xff;
    unsigned b = ( frame->color >>  8 ) & 0xff;

    int yuv = hb_rgb2yuv((r << 16) | (g << 8) | b );
    
    unsigned frameY = (yuv >> 16) & 0xff;
    unsigned frameV = (yuv >> 8 ) & 0xff;
    unsigned frameU = (yuv >> 0 ) & 0xff;

    sub = hb_frame_buffer_init( AV_PIX_FMT_YUVA420P, frame->w, frame->h );
    if( sub == NULL )
        return NULL;

    uint8_t *y_out, *u_out, *v_out, *a_out;
    y_out = sub->plane[0].data;
    u_out = sub->plane[1].data;
    v_out = sub->plane[2].data;
    a_out = sub->plane[3].data;

    for( yy = 0; yy < frame->h; yy++ )
    {
        for( xx = 0; xx < frame->w; xx++ )
        {
            y_out[xx] = frameY;
            if( ( yy & 1 ) == 0 )
            {
                u_out[xx>>1] = frameU;
                v_out[xx>>1] = frameV;
            }
            a_out[xx] = ssaAlpha( frame, xx, yy );;
        }
        y_out += sub->plane[0].stride;
        if( ( yy & 1 ) == 0 )
        {
            u_out += sub->plane[1].stride;
            v_out += sub->plane[2].stride;
        }
        a_out += sub->plane[3].stride;
    }
    sub->f.width = frame->w;
    sub->f.height = frame->h;
    sub->f.x = frame->dst_x + pv->crop[2];
    sub->f.y = frame->dst_y + pv->crop[0];

    return sub;
}

static void ApplySSASubs( hb_filter_private_t * pv, hb_buffer_t * buf )
{
    ASS_Image *frameList;
    hb_buffer_t *sub;
    frameList = ass_render_frame( pv->renderer, pv->ssaTrack,
                                  buf->s.start / 90, NULL );
    if ( !frameList )
        return;

    ASS_Image *frame;
    for (frame = frameList; frame; frame = frame->next) {
        sub = RenderSSAFrame( pv, frame );
        if( sub )
        {
            ApplySub( pv, buf, sub );
            hb_buffer_close( &sub );
        }
    }
}

static void ssa_log(int level, const char *fmt, va_list args, void *data)
{
    if ( level < 5 )      // same as default verbosity when no callback is set
    {
        hb_valog( 1, "[ass]", fmt, args );
    }
}

static int ssa_init( hb_filter_object_t * filter,
                     hb_filter_init_t * init )
{
    hb_filter_private_t * pv = filter->private_data;

    pv->ssa = ass_library_init();
    if ( !pv->ssa ) {
        hb_error( "decssasub: libass initialization failed\n" );
        return 1;
    }
    
    // Redirect libass output to hb_log
    ass_set_message_cb( pv->ssa, ssa_log, NULL );
    
    // Load embedded fonts
    hb_list_t * list_attachment = init->job->list_attachment;
    int i;
    for ( i = 0; i < hb_list_count(list_attachment); i++ )
    {
        hb_attachment_t * attachment = hb_list_item( list_attachment, i );
        
        if ( attachment->type == FONT_TTF_ATTACH )
        {
            ass_add_font(
                pv->ssa,
                attachment->name,
                attachment->data,
                attachment->size );
        }
    }
    
    ass_set_extract_fonts( pv->ssa, 1 );
    ass_set_style_overrides( pv->ssa, NULL );
    
    pv->renderer = ass_renderer_init( pv->ssa );
    if ( !pv->renderer ) {
        hb_log( "decssasub: renderer initialization failed\n" );
        return 1;
    }
    
    ass_set_use_margins( pv->renderer, 0 );
    ass_set_hinting( pv->renderer, ASS_HINTING_LIGHT ); // VLC 1.0.4 uses this
    ass_set_font_scale( pv->renderer, 1.0 );
    ass_set_line_spacing( pv->renderer, 1.0 );
    
    // Setup default font family
    // 
    // SSA v4.00 requires that "Arial" be the default font
    const char *font = NULL;
    const char *family = "Arial";
    // NOTE: This can sometimes block for several *seconds*.
    //       It seems that process_fontdata() for some embedded fonts is slow.
    ass_set_fonts( pv->renderer, font, family, /*haveFontConfig=*/1, NULL, 1 );
    
    // Setup track state
    pv->ssaTrack = ass_new_track( pv->ssa );
    if ( !pv->ssaTrack ) {
        hb_log( "decssasub: ssa track initialization failed\n" );
        return 1;
    }
    
    // NOTE: The codec extradata is expected to be in MKV format
    ass_process_codec_private( pv->ssaTrack,
        (char *)filter->subtitle->extradata, filter->subtitle->extradata_size );
    
    int width = init->width - ( pv->crop[2] + pv->crop[3] );
    int height = init->height - ( pv->crop[0] + pv->crop[1] );
    ass_set_frame_size( pv->renderer, width, height);

    double par = (double)init->par_width / init->par_height;
    ass_set_aspect_ratio( pv->renderer, 1, par );

    return 0;
}

static void ssa_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    if ( pv->ssaTrack )
        ass_free_track( pv->ssaTrack );
    if ( pv->renderer )
        ass_renderer_done( pv->renderer );
    if ( pv->ssa )
        ass_library_done( pv->ssa );

    free( pv );
    filter->private_data = NULL;
}

static int ssa_work( hb_filter_object_t * filter,
                     hb_buffer_t ** buf_in,
                     hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * sub;

    if ( in->size <= 0 )
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while( ( sub = hb_fifo_get( filter->subtitle->fifo_out ) ) )
    {
        // Parse MKV-SSA packet
        // SSA subtitles always have an explicit stop time, so we
        // do not need to do special processing for stop == -1
        ass_process_chunk( pv->ssaTrack, (char*)sub->data, sub->size,
                           sub->s.start / 90,
                           (sub->s.stop - sub->s.start) / 90 );
    }

    ApplySSASubs( pv, in );
    *buf_in = NULL;
    *buf_out = in;
    
    return HB_FILTER_OK;
}

static void ApplyPGSSubs( hb_filter_private_t * pv, hb_buffer_t * buf )
{
    int index;
    hb_buffer_t * old_sub;
    hb_buffer_t * sub;

    // Each PGS subtitle supersedes anything that preceded it.
    // Find the active subtitle (if there is one), and delete
    // everything before it.
    for( index = hb_list_count( pv->sub_list ) - 1; index > 0; index-- )
    {
        sub = hb_list_item( pv->sub_list, index);
        if ( sub->s.start <= buf->s.start )
        {
            while ( index > 0 )
            {
                old_sub = hb_list_item( pv->sub_list, index - 1);
                hb_list_rem( pv->sub_list, old_sub );
                hb_buffer_close( &old_sub );
                index--;
            }
        }
    }

    // Some PGS subtitles have no content and only serve to clear
    // the screen. If any of these are at the front of our list,
    // we can now get rid of them.
    while ( hb_list_count( pv->sub_list ) > 0 )
    {
        sub = hb_list_item( pv->sub_list, 0 );
        if (sub->f.width != 0 && sub->f.height != 0)
            break;

        hb_list_rem( pv->sub_list, sub );
        hb_buffer_close( &sub );
    }

    // Check to see if there's an active subtitle, and apply it.
    if ( hb_list_count( pv->sub_list ) > 0)
    {
        sub = hb_list_item( pv->sub_list, 0 );
        if ( sub->s.start <= buf->s.start )
        {
            while ( sub )
            {
                ApplySub( pv, buf, sub );
                sub = sub->sub;
            }
        }
    }
}

static int pgssub_init( hb_filter_object_t * filter,
                        hb_filter_init_t * init )
{
    hb_filter_private_t * pv = filter->private_data;

    pv->sub_list = hb_list_init();

    return 0;
}

static void pgssub_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if ( !pv )
    {
        return;
    }

    if ( pv->sub_list )
        hb_list_empty( &pv->sub_list );

    free( pv );
    filter->private_data = NULL;
}

static int pgssub_work( hb_filter_object_t * filter,
                        hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out)
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * sub;

    if ( in->size <= 0 )
    {
        *buf_in = NULL;
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Get any pending subtitles and add them to the active
    // subtitle list
    while ( ( sub = hb_fifo_get( filter->subtitle->fifo_out ) ) )
    {
        hb_list_add( pv->sub_list, sub );
    }

    ApplyPGSSubs( pv, in );
    *buf_in = NULL;
    *buf_out = in;

    return HB_FILTER_OK;
}

static int hb_rendersub_init( hb_filter_object_t * filter,
                                 hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;
    hb_subtitle_t *subtitle;
    int ii;

    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d:%d:%d",
                &pv->crop[0],
                &pv->crop[1],
                &pv->crop[2],
                &pv->crop[3]);
    }

    // Find the subtitle we need
    for( ii = 0; ii < hb_list_count(init->job->list_subtitle); ii++ )
    {
        subtitle = hb_list_item( init->job->list_subtitle, ii );
        if( subtitle && subtitle->config.dest == RENDERSUB )
        {
            // Found it
            filter->subtitle = subtitle;
            pv->type = subtitle->source;
            break;
        }
    }
    if( filter->subtitle == NULL )
    {
        hb_log("rendersub: no subtitle marked for burn");
        return 1;
    }

    switch( pv->type )
    {
        case VOBSUB:
        {
            return vobsub_init( filter, init );
        } break;

        case SSASUB:
        {
            return ssa_init( filter, init );
        } break;

        case PGSSUB:
        {
            return pgssub_init( filter, init );
        } break;

        default:
        {
            hb_log("rendersub: unsupported subtitle format %d", pv->type );
            return 1;
        } break;
    }
}

static int hb_rendersub_work( hb_filter_object_t * filter,
                                 hb_buffer_t ** buf_in,
                                 hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    switch( pv->type )
    {
        case VOBSUB:
        {
            return vobsub_work( filter, buf_in, buf_out );
        } break;

        case SSASUB:
        {
            return ssa_work( filter, buf_in, buf_out );
        } break;

        case PGSSUB:
        {
            return pgssub_work( filter, buf_in, buf_out );
        } break;

        default:
        {
            hb_error("rendersub: unsupported subtitle format %d", pv->type );
            return 1;
        } break;
    }
}

static void hb_rendersub_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    switch( pv->type )
    {
        case VOBSUB:
        {
            vobsub_close( filter );
        } break;

        case SSASUB:
        {
            ssa_close( filter );
        } break;

        case PGSSUB:
        {
            pgssub_close( filter );
        } break;

        default:
        {
            hb_error("rendersub: unsupported subtitle format %d", pv->type );
        } break;
    }
}

