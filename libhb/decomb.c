/* $Id: decomb.c,v 1.14 2008/04/25 5:00:00 jbrjake Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. 
   
   The yadif algorithm was created by Michael Niedermayer.
   Tritical's work inspired much of the comb detection code:
   http://web.missouri.edu/~kes25c/
*/

#include "hb.h"
#include "hbffmpeg.h"
#include "mpeg2dec/mpeg2.h"
#include "eedi2.h"

#define SUPPRESS_AV_LOG

#define MODE_DEFAULT     1
#define PARITY_DEFAULT   -1

#define MCDEINT_MODE_DEFAULT   -1
#define MCDEINT_QP_DEFAULT      1

#define ABS(a) ((a) > 0 ? (a) : (-(a)))
#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

// Some names to correspond to the pv->eedi_half array's contents
#define SRCPF 0
#define MSKPF 1
#define TMPPF 2
#define DSTPF 3
// Some names to correspond to the pv->eedi_full array's contents
#define DST2PF 0
#define TMP2PF2 1
#define MSK2PF 2
#define TMP2PF 3
#define DST2MPF 4

struct yadif_arguments_s {
    uint8_t **dst;
    int parity;
    int tff;
    int stop;
    int is_combed;
};

struct decomb_arguments_s {
    int stop;
};

struct eedi2_arguments_s {
    int stop;
};

typedef struct yadif_arguments_s yadif_arguments_t;
typedef struct decomb_arguments_s decomb_arguments_t;
typedef struct eedi2_arguments_s eedi2_arguments_t;

typedef struct eedi2_thread_arg_s {
    hb_filter_private_t *pv;
    int plane;
} eedi2_thread_arg_t;

typedef struct decomb_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
} decomb_thread_arg_t;

typedef struct yadif_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
} yadif_thread_arg_t;

struct hb_filter_private_s
{
    int              pix_fmt;
    int              width[3];
    int              height[3];

    // Decomb parameters
    int              mode;
    int              spatial_metric;
    int              motion_threshold;
    int              spatial_threshold;
    int              block_threshold;
    int              block_width;
    int              block_height;
    
    // EEDI2 parameters
    int              magnitude_threshold;
    int              variance_threshold;
    int              laplacian_threshold;
    int              dilation_threshold;
    int              erosion_threshold;
    int              noise_threshold;
    int              maximum_search_distance;
    int              post_processing;

    int              parity;
    int              tff;
    
    int              yadif_ready;

    int              mcdeint_mode;
    int              mcdeint_qp;

    int              mcdeint_outbuf_size;
    uint8_t        * mcdeint_outbuf;
    AVCodecContext * mcdeint_avctx_enc;
    AVFrame        * mcdeint_frame;
    AVFrame        * mcdeint_frame_dec;

    int              yadif_deinterlaced_frames;
    int              blend_deinterlaced_frames;
    int              unfiltered_frames;

    uint8_t        * ref[4][3];
    int              ref_stride[3];

    /* Make a buffer to store a comb mask. */
    uint8_t        * mask[3];

    uint8_t        * eedi_half[4][3];
    uint8_t        * eedi_full[5][3];
    int            * cx2;
    int            * cy2;
    int            * cxy;
    int            * tmpc;
    
    AVPicture        pic_in;
    AVPicture        pic_out;
    hb_buffer_t *    buf_out[2];
    hb_buffer_t *    buf_settings;
    
    int              cpu_count;

    hb_thread_t    ** yadif_threads;         // Threads for Yadif - one per CPU
    hb_lock_t      ** yadif_begin_lock;      // Thread has work
    hb_lock_t      ** yadif_complete_lock;   // Thread has completed work
    yadif_arguments_t *yadif_arguments;      // Arguments to thread for work
    
    hb_thread_t    ** decomb_threads;        // Threads for comb detection - one per CPU
    hb_lock_t      ** decomb_begin_lock;     // Thread has work
    hb_lock_t      ** decomb_complete_lock;  // Thread has completed work
    decomb_arguments_t *decomb_arguments;    // Arguments to thread for work

    hb_thread_t    ** eedi2_threads;        // Threads for eedi2 - one per plane
    hb_lock_t      ** eedi2_begin_lock;     // Thread has work
    hb_lock_t      ** eedi2_complete_lock;  // Thread has completed work
    eedi2_arguments_t *eedi2_arguments;    // Arguments to thread for work
    
};

hb_filter_private_t * hb_decomb_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings );

int hb_decomb_work(      const hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv );

void hb_decomb_close( hb_filter_private_t * pv );

hb_filter_object_t hb_filter_decomb =
{
    FILTER_DECOMB,
    "Decomb",
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

int check_combing_mask( hb_filter_private_t * pv )
{
    /* Go through the mask in X*Y blocks. If any of these windows
       have threshold or more combed pixels, consider the whole
       frame to be combed and send it on to be deinterlaced.     */

    /* Block mask threshold -- The number of pixels
       in a block_width * block_height window of
       he mask that need to show combing for the
       whole frame to be seen as such.            */
    int threshold       = pv->block_threshold;
    int block_width     = pv->block_width;
    int block_height    = pv->block_height;
    int block_x, block_y;
    int block_score = 0; int send_to_blend = 0;
    
    int x, y, k;

    for( k = 0; k < 1; k++ )
    {
        int ref_stride = pv->ref_stride[k];
        for( y = 0; y < ( pv->height[k] - block_height ); y = y + block_height )
        {
            for( x = 0; x < ( pv->width[k] - block_width ); x = x + block_width )
            {
                block_score = 0;
                for( block_y = 0; block_y < block_height; block_y++ )
                {
                    for( block_x = 0; block_x < block_width; block_x++ )
                    {
                        int mask_y = y + block_y;
                        int mask_x = x + block_x;
                        
                        /* We only want to mark a pixel in a block as combed
                           if the pixels above and below are as well. Got to
                           handle the top and bottom lines separately.       */
                        if( y + block_y == 0 )
                        {
                            if( pv->mask[k][mask_y*ref_stride+mask_x    ] == 255 &&
                                pv->mask[k][mask_y*ref_stride+mask_x + 1] == 255 )
                                    block_score++;
                        }
                        else if( y + block_y == pv->height[k] - 1 )
                        {
                            if( pv->mask[k][mask_y*ref_stride+mask_x - 1] == 255 &&
                                pv->mask[k][mask_y*ref_stride+mask_x    ] == 255 )
                                    block_score++;
                        }
                        else
                        {
                            if( pv->mask[k][mask_y*ref_stride+mask_x - 1] == 255 &&
                                pv->mask[k][mask_y*ref_stride+mask_x    ] == 255 &&
                                pv->mask[k][mask_y*ref_stride+mask_x + 1] == 255 )
                                    block_score++;
                        } 
                    }
                }

                if( block_score >= ( threshold / 2 ) )
                {
#if 0
                    hb_log("decomb: frame %i | score %i | type %s", pv->yadif_deinterlaced_frames + pv->blend_deinterlaced_frames +  pv->unfiltered_frames + 1, block_score, pv->buf_settings->flags & 16 ? "Film" : "Video");
#endif
                    if ( block_score <= threshold && !( pv->buf_settings->flags & 16) )
                    {
                        /* Blend video content that scores between
                           ( threshold / 2 ) and threshold.        */
                        send_to_blend = 1;
                    }
                    else if( block_score > threshold )
                    {
                        if( pv->buf_settings->flags & 16 )
                        {
                            /* Blend progressive content above the threshold.*/
                            return 2;
                        }
                        else
                        {
                            /* Yadif deinterlace video content above the threshold. */
                            return 1;
                        }
                    }
                }
            }
        } 
    }
    
    if( send_to_blend )
    {
        return 2;
    }
    else
    {
        /* Consider this frame to be uncombed. */
        return 0;
    }
}

int detect_combed_segment( hb_filter_private_t * pv, int segment_start, int segment_stop )
{
    /* A mish-mash of various comb detection tricks
       picked up from neuron2's Decomb plugin for
       AviSynth and tritical's IsCombedT and
       IsCombedTIVTC plugins.                       */
       
    int x, y, k, width, height;
    
    /* Comb scoring algorithm */
    int spatial_metric  = pv->spatial_metric;
    /* Motion threshold */
    int mthresh         = pv->motion_threshold;
    /* Spatial threshold */
    int athresh         = pv->spatial_threshold;
    int athresh_squared = athresh * athresh;
    int athresh6        = 6 *athresh;

    /* One pas for Y, one pass for U, one pass for V */    
    for( k = 0; k < 1; k++ )
    {
        int ref_stride  = pv->ref_stride[k];
        width           = pv->width[k];
        height          = pv->height[k];
        
        /* Comb detection has to start at y = 2 and end at
           y = height - 2, because it needs to examine
           2 pixels above and 2 below the current pixel.      */
        if( segment_start < 2 )
            segment_start = 2;
        if( segment_stop > height - 2 )
            segment_stop = height - 2;
            
        for( y =  segment_start; y < segment_stop; y++ )
        {
            /* These are just to make the buffer locations easier to read. */
            int back_2    = ( y - 2 )*ref_stride ;
            int back_1    = ( y - 1 )*ref_stride;
            int current   =         y*ref_stride;
            int forward_1 = ( y + 1 )*ref_stride;
            int forward_2 = ( y + 2 )*ref_stride;
            
            /* We need to examine a column of 5 pixels
               in the prev, cur, and next frames.      */
            uint8_t previous_frame[5];
            uint8_t current_frame[5];
            uint8_t next_frame[5];
            
            for( x = 0; x < width; x++ )
            {
                /* Fill up the current frame array with the current pixel values.*/
                current_frame[0] = pv->ref[1][k][back_2    + x];
                current_frame[1] = pv->ref[1][k][back_1    + x];
                current_frame[2] = pv->ref[1][k][current   + x];
                current_frame[3] = pv->ref[1][k][forward_1 + x];
                current_frame[4] = pv->ref[1][k][forward_2 + x];

                int up_diff   = current_frame[2] - current_frame[1];
                int down_diff = current_frame[2] - current_frame[3];

                if( ( up_diff >  athresh && down_diff >  athresh ) ||
                    ( up_diff < -athresh && down_diff < -athresh ) )
                {
                    /* The pixel above and below are different,
                       and they change in the same "direction" too.*/
                    int motion = 0;
                    if( mthresh > 0 )
                    {
                        /* Make sure there's sufficient motion between frame t-1 to frame t+1. */
                        previous_frame[0] = pv->ref[0][k][back_2    + x];
                        previous_frame[1] = pv->ref[0][k][back_1    + x];
                        previous_frame[2] = pv->ref[0][k][current   + x];
                        previous_frame[3] = pv->ref[0][k][forward_1 + x];
                        previous_frame[4] = pv->ref[0][k][forward_2 + x];
                        next_frame[0]     = pv->ref[2][k][back_2    + x];
                        next_frame[1]     = pv->ref[2][k][back_1    + x];
                        next_frame[2]     = pv->ref[2][k][current   + x];
                        next_frame[3]     = pv->ref[2][k][forward_1 + x];
                        next_frame[4]     = pv->ref[2][k][forward_2 + x];
                        
                        if( abs( previous_frame[2] - current_frame[2] ) > mthresh &&
                            abs(  current_frame[1] - next_frame[1]    ) > mthresh &&
                            abs(  current_frame[3] - next_frame[3]    ) > mthresh )
                                motion++;
                        if( abs(     next_frame[2] - current_frame[2] ) > mthresh &&
                            abs( previous_frame[1] - current_frame[1] ) > mthresh &&
                            abs( previous_frame[3] - current_frame[3] ) > mthresh )
                                motion++;
                    }
                    else
                    {
                        /* User doesn't want to check for motion,
                           so move on to the spatial check.       */
                        motion = 1;
                    }
                           
                    if( motion || ( pv->yadif_deinterlaced_frames==0 && pv->blend_deinterlaced_frames==0 && pv->unfiltered_frames==0) )
                    {
                           /* That means it's time for the spatial check.
                              We've got several options here.             */
                        if( spatial_metric == 0 )
                        {
                            /* Simple 32detect style comb detection */
                            if( ( abs( current_frame[2] - current_frame[4] ) < 10  ) &&
                                ( abs( current_frame[2] - current_frame[3] ) > 15 ) )
                            {
                                pv->mask[k][y*ref_stride + x] = 255;
                            }
                            else
                            {
                                pv->mask[k][y*ref_stride + x] = 0;
                            }
                        }
                        else if( spatial_metric == 1 )
                        {
                            /* This, for comparison, is what IsCombed uses.
                               It's better, but still noise senstive.      */
                               int combing = ( current_frame[1] - current_frame[2] ) *
                                             ( current_frame[3] - current_frame[2] );
                               
                               if( combing > athresh_squared )
                                   pv->mask[k][y*ref_stride + x] = 255; 
                               else
                                   pv->mask[k][y*ref_stride + x] = 0;
                        }
                        else if( spatial_metric == 2 )
                        {
                            /* Tritical's noise-resistant combing scorer.
                               The check is done on a bob+blur convolution. */
                            int combing = abs( current_frame[0]
                                             + ( 4 * current_frame[2] )
                                             + current_frame[4]
                                             - ( 3 * ( current_frame[1]
                                                     + current_frame[3] ) ) );

                            /* If the frame is sufficiently combed,
                               then mark it down on the mask as 255. */
                            if( combing > athresh6 )
                                pv->mask[k][y*ref_stride + x] = 255; 
                            else
                                pv->mask[k][y*ref_stride + x] = 0;
                        }
                    }
                    else
                    {
                        pv->mask[k][y*ref_stride + x] = 0;
                    }
                }
                else
                {
                    pv->mask[k][y*ref_stride + x] = 0;
                }
            }
        }
    }
}

// This function calls all the eedi2 filters in sequence for a given plane.
// It outputs the final interpolated image to pv->eedi_full[DST2PF].
void eedi2_interpolate_plane( hb_filter_private_t * pv, int k )
{
    /* We need all these pointers. No, seriously.
       I swear. It's not a joke. They're used.
       All nine of them.                         */
    uint8_t * mskp = pv->eedi_half[MSKPF][k];
    uint8_t * srcp = pv->eedi_half[SRCPF][k];
    uint8_t * tmpp = pv->eedi_half[TMPPF][k];
    uint8_t * dstp = pv->eedi_half[DSTPF][k];
    uint8_t * dst2p = pv->eedi_full[DST2PF][k];
    uint8_t * tmp2p2 = pv->eedi_full[TMP2PF2][k];
    uint8_t * msk2p = pv->eedi_full[MSK2PF][k];
    uint8_t * tmp2p = pv->eedi_full[TMP2PF][k];
    uint8_t * dst2mp = pv->eedi_full[DST2MPF][k];
    int * cx2 = pv->cx2;
    int * cy2 = pv->cy2;
    int * cxy = pv->cxy;
    int * tmpc = pv->tmpc;

    int pitch = pv->ref_stride[k];
    int height = pv->height[k]; int width = pv->width[k];
    int half_height = height / 2;

    // edge mask
    eedi2_build_edge_mask( mskp, pitch, srcp, pitch,
                     pv->magnitude_threshold, pv->variance_threshold, pv->laplacian_threshold, 
                     half_height, width );
    eedi2_erode_edge_mask( mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width );
    eedi2_dilate_edge_mask( tmpp, pitch, mskp, pitch, pv->dilation_threshold, half_height, width );
    eedi2_erode_edge_mask( mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width );
    eedi2_remove_small_gaps( tmpp, pitch, mskp, pitch, half_height, width );

    // direction mask
    eedi2_calc_directions( k, mskp, pitch, srcp, pitch, tmpp, pitch,
                     pv->maximum_search_distance, pv->noise_threshold,
                     half_height, width );
    eedi2_filter_dir_map( mskp, pitch, tmpp, pitch, dstp, pitch, half_height, width );
    eedi2_expand_dir_map( mskp, pitch, dstp, pitch, tmpp, pitch, half_height, width );
    eedi2_filter_map( mskp, pitch, tmpp, pitch, dstp, pitch, half_height, width );

    // upscale 2x vertically
    eedi2_upscale_by_2( srcp, dst2p, half_height, pitch );
    eedi2_upscale_by_2( dstp, tmp2p2, half_height, pitch );
    eedi2_upscale_by_2( mskp, msk2p, half_height, pitch );

    // upscale the direction mask
    eedi2_mark_directions_2x( msk2p, pitch, tmp2p2, pitch, tmp2p, pitch, pv->tff, height, width );
    eedi2_filter_dir_map_2x( msk2p, pitch, tmp2p, pitch,  dst2mp, pitch, pv->tff, height, width );
    eedi2_expand_dir_map_2x( msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width );
    eedi2_fill_gaps_2x( msk2p, pitch, tmp2p, pitch, dst2mp, pitch, pv->tff, height, width );
    eedi2_fill_gaps_2x( msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width );

    // interpolate a full-size plane
    eedi2_interpolate_lattice( k, tmp2p, pitch, dst2p, pitch, tmp2p2, pitch, pv->tff,
                         pv->noise_threshold, height, width );

    if( pv->post_processing == 1 || pv->post_processing == 3 )
    {
        // make sure the edge directions are consistent
        eedi2_bit_blit( tmp2p2, pitch, tmp2p, pitch, pv->width[k], pv->height[k] );
        eedi2_filter_dir_map_2x( msk2p, pitch, tmp2p, pitch, dst2mp, pitch, pv->tff, height, width );
        eedi2_expand_dir_map_2x( msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width );
        eedi2_post_process( tmp2p, pitch, tmp2p2, pitch, dst2p, pitch, pv->tff, height, width );
    }
    if( pv->post_processing == 2 || pv->post_processing == 3 )
    {
        // filter junctions and corners
        eedi2_gaussian_blur1( srcp, pitch, tmpp, pitch, srcp, pitch, half_height, width );
        eedi2_calc_derivatives( srcp, pitch, half_height, width, cx2, cy2, cxy );
        eedi2_gaussian_blur_sqrt2( cx2, tmpc, cx2, pitch, half_height, width);
        eedi2_gaussian_blur_sqrt2( cy2, tmpc, cy2, pitch, half_height, width);
        eedi2_gaussian_blur_sqrt2( cxy, tmpc, cxy, pitch, half_height, width);
        eedi2_post_process_corner( cx2, cy2, cxy, pitch, tmp2p2, pitch, dst2p, pitch, height, width, pv->tff );
    }
}

/*
 *  eedi2 interpolate this plane in a single thread.
 */
void eedi2_filter_thread( void *thread_args_v )
{
    eedi2_arguments_t *eedi2_work = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int plane;
    eedi2_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    plane = thread_args->plane;

    hb_log("eedi2 thread started for plane %d", plane);

    while( run )
    {
        /*
         * Wait here until there is work to do. hb_lock() blocks until
         * render releases it to say that there is more work to do.
         */
        hb_lock( pv->eedi2_begin_lock[plane] );

        eedi2_work = &pv->eedi2_arguments[plane];

        if( eedi2_work->stop )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            continue;
        } 

        /*
         * Process plane
         */
            eedi2_interpolate_plane( pv, plane );
        
        /*
         * Finished this segment, let everyone know.
         */
        hb_unlock( pv->eedi2_complete_lock[plane] );
    }
    free( thread_args_v );
}

// Sets up the input field planes for EEDI2 in pv->eedi_half[SRCPF]
// and then runs eedi2_filter_thread for each plane.
void eedi2_planer( hb_filter_private_t * pv )
{
    /* Copy the first field from the source to a half-height frame. */
    int i;
    for( i = 0;  i < 3; i++ )
    {
        int pitch = pv->ref_stride[i];
        int start_line = !pv->tff;
        eedi2_fill_half_height_buffer_plane( &pv->ref[1][i][pitch*start_line], pv->eedi_half[SRCPF][i], pitch, pv->height[i] );
    }
    
    int plane;
    for( plane = 0; plane < 3; plane++ )
    {  
        /*
         * Let the thread for this plane know that we've setup work 
         * for it by releasing the begin lock (ensuring that the
         * complete lock is already locked so that we block when
         * we try to lock it again below).
         */
        hb_lock( pv->eedi2_complete_lock[plane] );
        hb_unlock( pv->eedi2_begin_lock[plane] );
    }

    /*
     * Wait until all three threads have completed by trying to get
     * the complete lock that we locked earlier for each thread, which
     * will block until that thread has completed the work on that
     * plane.
     */
    for( plane = 0; plane < 3; plane++ )
    {
        hb_lock( pv->eedi2_complete_lock[plane] );
        hb_unlock( pv->eedi2_complete_lock[plane] );
    }
}


/*
 * comb detect this segment of all three planes in a single thread.
 */
void decomb_filter_thread( void *thread_args_v )
{
    decomb_arguments_t *decomb_work = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int segment, segment_start, segment_stop, plane;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("decomb thread started for segment %d", segment);

    while( run )
    {
        /*
         * Wait here until there is work to do. hb_lock() blocks until
         * render releases it to say that there is more work to do.
         */
        hb_lock( pv->decomb_begin_lock[segment] );

        decomb_work = &pv->decomb_arguments[segment];

        if( decomb_work->stop )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            continue;
        } 

        /*
         * Process segment (for now just from luma)
         */
        for( plane = 0; plane < 1; plane++)
        {

            int w = pv->width[plane];
            int h = pv->height[plane];
            int ref_stride = pv->ref_stride[plane];
            segment_start = ( h / pv->cpu_count ) * segment;
            if( segment == pv->cpu_count - 1 )
            {
                /*
                 * Final segment
                 */
                segment_stop = h;
            } else {
                segment_stop = ( h / pv->cpu_count ) * ( segment + 1 );
            }
            
            detect_combed_segment( pv, segment_start, segment_stop );
        }
        /*
         * Finished this segment, let everyone know.
         */
        hb_unlock( pv->decomb_complete_lock[segment] );
    }
    free( thread_args_v );
}

int comb_segmenter( hb_filter_private_t * pv )
{
    int segment;

    for( segment = 0; segment < pv->cpu_count; segment++ )
    {  
        /*
         * Let the thread for this plane know that we've setup work 
         * for it by releasing the begin lock (ensuring that the
         * complete lock is already locked so that we block when
         * we try to lock it again below).
         */
        hb_lock( pv->decomb_complete_lock[segment] );
        hb_unlock( pv->decomb_begin_lock[segment] );
    }

    /*
     * Wait until all three threads have completed by trying to get
     * the complete lock that we locked earlier for each thread, which
     * will block until that thread has completed the work on that
     * plane.
     */
    for( segment = 0; segment < pv->cpu_count; segment++ )
    {
        hb_lock( pv->decomb_complete_lock[segment] );
        hb_unlock( pv->decomb_complete_lock[segment] );
    }
    
    return check_combing_mask( pv );
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
    /* While prev and next point to the previous and next frames,
       prev2 and next2 will shift depending on the parity, usually 1.
       They are the previous and next fields, the fields temporally adjacent
       to the other field in the current frame--the one not being filtered.  */
    uint8_t *prev2 = parity ? prev : cur ;
    uint8_t *next2 = parity ? cur  : next;
    
    int w = pv->width[plane];
    int refs = pv->ref_stride[plane];
    int x;
    int eedi2_mode = (pv->mode == 5);
    
    /* We can replace spatial_pred with this interpolation*/
    uint8_t * eedi2_guess = &pv->eedi_full[DST2PF][plane][y*refs];

    /* Decomb's cubic interpolation can only function when there are
       three samples above and below, so regress to yadif's traditional
       two-tap interpolation when filtering at the top and bottom edges. */
    int edge = 0;
    if( ( y < 3 ) || ( y > ( pv->height[plane] - 4 ) )  )
        edge = 1;

    for( x = 0; x < w; x++)
    {
        /* Pixel above*/
        int c              = cur[-refs];
        /* Temporal average: the current location in the adjacent fields */
        int d              = (prev2[0] + next2[0])>>1;
        /* Pixel below */
        int e              = cur[+refs];
        
        /* How the current pixel changes between the adjacent fields */
        int temporal_diff0 = ABS(prev2[0] - next2[0]);
        /* The average of how much the pixels above and below change from the frame before to now. */
        int temporal_diff1 = ( ABS(prev[-refs] - cur[-refs]) + ABS(prev[+refs] - cur[+refs]) ) >> 1;
        /* The average of how much the pixels above and below change from now to the next frame. */
        int temporal_diff2 = ( ABS(next[-refs] - cur[-refs]) + ABS(next[+refs] - cur[+refs]) ) >> 1;
        /* For the actual difference, use the largest of the previous average diffs. */
        int diff           = MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);

        int spatial_pred;
        
        if( eedi2_mode )
        {
            /* Who needs yadif's spatial predictions when we can have EEDI2's? */
            spatial_pred = eedi2_guess[0];
            eedi2_guess++;
        }
        else // Yadif spatial interpolation
        {
            /* SAD of how the pixel-1, the pixel, and the pixel+1 change from the line above to below. */ 
            int spatial_score  = ABS(cur[-refs-1] - cur[+refs-1]) + ABS(cur[-refs]-cur[+refs]) +
                                         ABS(cur[-refs+1] - cur[+refs+1]) - 1;         
            
            /* Spatial pred is either a bilinear or cubic vertical interpolation. */
            if( pv->mode > 0 && !edge)
            {
                spatial_pred = cubic_interpolate( cur[-3*refs], cur[-refs], cur[+refs], cur[3*refs] );
            }
            else
            {
                spatial_pred = (c+e)>>1;
            }

        /* EDDI: Edge Directed Deinterlacing Interpolation
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
                        if( pv->mode > 0 && !edge )\
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
        }

        /* Temporally adjust the spatial prediction by
           comparing against lines in the adjacent fields. */
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
                        
        dst++;
        cur++;
        prev++;
        next++;
        prev2++;
        next2++;
    }
}

/*
 * deinterlace this segment of all three planes in a single thread.
 */
void yadif_decomb_filter_thread( void *thread_args_v )
{
    yadif_arguments_t *yadif_work = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int plane;
    int segment, segment_start, segment_stop;
    yadif_thread_arg_t *thread_args = thread_args_v;
    uint8_t **dst;
    int parity, tff, y, w, h, penultimate, ultimate, ref_stride, is_combed;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("yadif thread started for segment %d", segment);

    while( run )
    {
        /*
         * Wait here until there is work to do. hb_lock() blocks until
         * render releases it to say that there is more work to do.
         */
        hb_lock( pv->yadif_begin_lock[segment] );

        yadif_work = &pv->yadif_arguments[segment];

        if( yadif_work->stop )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            continue;
        } 

        if( yadif_work->dst == NULL )
        {
            hb_error( "thread started when no work available" );
            hb_snooze(500);
            continue;
        }
        
        is_combed = pv->yadif_arguments[segment].is_combed;

        /*
         * Process all three planes, but only this segment of it.
         */
        for( plane = 0; plane < 3; plane++)
        {

            dst = yadif_work->dst;
            parity = yadif_work->parity;
            tff = yadif_work->tff;
            w = pv->width[plane];
            h = pv->height[plane];
            penultimate = h - 2;
            ultimate = h - 1;
            ref_stride = pv->ref_stride[plane];
            segment_start = ( h / pv->cpu_count ) * segment;
            if( segment == pv->cpu_count - 1 )
            {
                /*
                 * Final segment
                 */
                segment_stop = h;
            } else {
                segment_stop = ( h / pv->cpu_count ) * ( segment + 1 );
            }

            for( y = segment_start; y < segment_stop; y++ )
            {
                if( ( pv->mode == 4 && is_combed ) || is_combed == 2 )
                {
                    /* This line gets blend filtered, not yadif filtered. */
                    uint8_t *prev = &pv->ref[0][plane][y*ref_stride];
                    uint8_t *cur  = &pv->ref[1][plane][y*ref_stride];
                    uint8_t *next = &pv->ref[2][plane][y*ref_stride];
                    uint8_t *dst2 = &dst[plane][y*w];

                    blend_filter_line( dst2, cur, plane, y, pv );
                }
                else if( ( ( y ^ parity ) &  1 )  && ( is_combed == 1 ) )
                {
                    /* This line gets yadif filtered. It is the bottom field
                       when TFF and vice-versa. It's the field that gets
                       filtered. Because yadif needs 2 lines above and below
                       the one being filtered, we need to mirror the edges.
                       When TFF, this means replacing the 2nd line with a
                       copy of the 1st, and the last with the second-to-last. */
                    if( y > 1 && y < ( h -2 ) )
                    {
                        /* This isn't the top or bottom, proceed as normal to yadif. */
                        uint8_t *prev = &pv->ref[0][plane][y*ref_stride];
                        uint8_t *cur  = &pv->ref[1][plane][y*ref_stride];
                        uint8_t *next = &pv->ref[2][plane][y*ref_stride];
                        uint8_t *dst2 = &dst[plane][y*w];

                        yadif_filter_line( dst2, 
                                           prev, 
                                           cur, 
                                           next, 
                                           plane, 
                                           parity ^ tff,
                                           y, 
                                           pv );
                    }
                    else if( y == 0 )
                    {
                        /* BFF, so y0 = y1 */
                        memcpy( &dst[plane][y*w],
                                &pv->ref[1][plane][1*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == 1 )
                    {
                        /* TFF, so y1 = y0 */
                        memcpy( &dst[plane][y*w],
                                &pv->ref[1][plane][0],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == penultimate )
                    {
                        /* BFF, so penultimate y = ultimate y */
                        memcpy( &dst[plane][y*w],
                                &pv->ref[1][plane][ultimate*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == ultimate )
                    {
                        /* TFF, so ultimate y = penultimate y */
                        memcpy( &dst[plane][y*w],
                                &pv->ref[1][plane][penultimate*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                }
                else
                {
                    memcpy( &dst[plane][y*w],
                            &pv->ref[1][plane][y*ref_stride],
                            w * sizeof(uint8_t) );              
                }
            }
        }
        /*
         * Finished this segment, let everyone know.
         */
        hb_unlock( pv->yadif_complete_lock[segment] );
    }
    free( thread_args_v );
}

static void yadif_filter( uint8_t ** dst,
                          int parity,
                          int tff,
                          hb_filter_private_t * pv )
{
    /* If we're running comb detection, do it now, otherwise blend if mode 4 and interpolate if not. */
    int is_combed = pv->spatial_metric >= 0 ? comb_segmenter( pv ) : pv->mode == 4 ? 2 : 1;

    if( is_combed == 1 )
    {
        pv->yadif_deinterlaced_frames++;
    }
    else if( is_combed == 2 )
    {
        pv->blend_deinterlaced_frames++;
    }
    else
    {
        pv->unfiltered_frames++;
    }
    
    if( is_combed == 1 && pv->mode == 5 )
    {
        /* Generate an EEDI2 interpolation */
        eedi2_planer( pv );
    }
    
    if( is_combed )
    {
        int segment;

        for( segment = 0; segment < pv->cpu_count; segment++ )
        {  
            /*
             * Setup the work for this plane.
             */
            pv->yadif_arguments[segment].parity = parity;
            pv->yadif_arguments[segment].tff = tff;
            pv->yadif_arguments[segment].dst = dst;
            pv->yadif_arguments[segment].is_combed = is_combed;

            /*
             * Let the thread for this plane know that we've setup work 
             * for it by releasing the begin lock (ensuring that the
             * complete lock is already locked so that we block when
             * we try to lock it again below).
             */
            hb_lock( pv->yadif_complete_lock[segment] );
            hb_unlock( pv->yadif_begin_lock[segment] );
        }

        /*
         * Wait until all three threads have completed by trying to get
         * the complete lock that we locked earlier for each thread, which
         * will block until that thread has completed the work on that
         * plane.
         */
        for( segment = 0; segment < pv->cpu_count; segment++ )
        {
            hb_lock( pv->yadif_complete_lock[segment] );
            hb_unlock( pv->yadif_complete_lock[segment] );
        }

        /*
         * Entire frame is now deinterlaced.
         */
    }
    else
    {
        /*  Just passing through... */
        int i;
        for( i = 0; i < 3; i++ )
        {
            uint8_t * ref = pv->ref[1][i];
            uint8_t * dest = dst[i];
            
            int w = pv->width[i];
            int ref_stride = pv->ref_stride[i];
            
            int y;
            for( y = 0; y < pv->height[i]; y++ )
            {
                memcpy(dest, ref, w);
                dest += w;
                ref += ref_stride;
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

    pv->buf_out[0] = hb_video_buffer_init( width, height );
    pv->buf_out[1] = hb_video_buffer_init( width, height );
    pv->buf_settings = hb_buffer_init( 0 );

    pv->yadif_deinterlaced_frames = 0;
    pv->blend_deinterlaced_frames = 0;
    pv->unfiltered_frames = 0;

    pv->yadif_ready    = 0;

    pv->mode     = MODE_DEFAULT;
    pv->spatial_metric = 2;
    pv->motion_threshold = 6;
    pv->spatial_threshold = 9;
    pv->block_threshold = 80;
    pv->block_width = 16;
    pv->block_height = 16;
    
    pv->magnitude_threshold = 10;
    pv->variance_threshold = 20;
    pv->laplacian_threshold = 20;
    pv->dilation_threshold = 4;
    pv->erosion_threshold = 2;
    pv->noise_threshold = 50;
    pv->maximum_search_distance = 24;
    pv->post_processing = 1;

    pv->parity   = PARITY_DEFAULT;

    pv->mcdeint_mode   = MCDEINT_MODE_DEFAULT;
    pv->mcdeint_qp     = MCDEINT_QP_DEFAULT;

    if( settings )
    {
        sscanf( settings, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
                &pv->mode,
                &pv->spatial_metric,
                &pv->motion_threshold,
                &pv->spatial_threshold,
                &pv->block_threshold,
                &pv->block_width,
                &pv->block_height,
                &pv->magnitude_threshold,
                &pv->variance_threshold,
                &pv->laplacian_threshold,
                &pv->dilation_threshold,
                &pv->erosion_threshold,
                &pv->noise_threshold,
                &pv->maximum_search_distance,
                &pv->post_processing );
    }
    
    pv->cpu_count = hb_get_cpu_count();
    

    if( pv->mode == 2 || pv->mode == 3 )
    {
        pv->mcdeint_mode = 0;
    }
    
    /* Allocate yadif specific buffers */
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

    /* Allocate a buffer to store a comb mask. */
    for( i = 0; i < 3; i++ )
    {
        int is_chroma = !!i;
        int w = ((pv->width[0]   + 31) & (~31))>>is_chroma;
        int h = ((pv->height[0]+6+ 31) & (~31))>>is_chroma;

        pv->mask[i] = calloc( 1, w*h*sizeof(uint8_t) ) + 3*w;
    }
    
    if( pv->mode == 5 )
    {
        /* Allocate half-height eedi2 buffers */
        height = pv->height[0] / 2;
        for( i = 0; i < 3; i++ )
        {
            int is_chroma = !!i;
            int w = ((width   + 31) & (~31))>>is_chroma;
            int h = ((height+6+ 31) & (~31))>>is_chroma;

            for( j = 0; j < 4; j++ )
            {
                pv->eedi_half[j][i] = malloc( w*h*sizeof(uint8_t) ) + 3*w;
            }
        }

        /* Allocate full-height eedi2 buffers */
        height = pv->height[0];
        for( i = 0; i < 3; i++ )
        {
            int is_chroma = !!i;
            int w = ((width   + 31) & (~31))>>is_chroma;
            int h = ((height+6+ 31) & (~31))>>is_chroma;

            for( j = 0; j < 5; j++ )
            {
                pv->eedi_full[j][i] = malloc( w*h*sizeof(uint8_t) ) + 3*w;
            }
        }
    }
    
     /*
      * Create yadif threads and locks.
      */
     pv->yadif_threads = malloc( sizeof( hb_thread_t* ) * pv->cpu_count );
     pv->yadif_begin_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
     pv->yadif_complete_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
     pv->yadif_arguments = malloc( sizeof( yadif_arguments_t ) * pv->cpu_count );

     for( i = 0; i < pv->cpu_count; i++ )
     {
         yadif_thread_arg_t *thread_args;

         thread_args = malloc( sizeof( yadif_thread_arg_t ) );

         if( thread_args )
         {
             thread_args->pv = pv;
             thread_args->segment = i;

             pv->yadif_begin_lock[i] = hb_lock_init();
             pv->yadif_complete_lock[i] = hb_lock_init();

             /*
              * Important to start off with the threads locked waiting
              * on input.
              */
             hb_lock( pv->yadif_begin_lock[i] );

             pv->yadif_arguments[i].stop = 0;
             pv->yadif_arguments[i].dst = NULL;
             
             pv->yadif_threads[i] = hb_thread_init( "yadif_filter_segment",
                                                    yadif_decomb_filter_thread,
                                                    thread_args,
                                                    HB_NORMAL_PRIORITY );
         }
         else
         {
             hb_error( "yadif could not create threads" );
         }
    }
    
    /*
     * Create decomb threads and locks.
     */
    pv->decomb_threads = malloc( sizeof( hb_thread_t* ) * pv->cpu_count );
    pv->decomb_begin_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
    pv->decomb_complete_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
    pv->decomb_arguments = malloc( sizeof( decomb_arguments_t ) * pv->cpu_count );
    
    for( i = 0; i < pv->cpu_count; i++ )
    {
        decomb_thread_arg_t *decomb_thread_args;
    
        decomb_thread_args = malloc( sizeof( decomb_thread_arg_t ) );
    
        if( decomb_thread_args )
        {
            decomb_thread_args->pv = pv;
            decomb_thread_args->segment = i;
    
            pv->decomb_begin_lock[i] = hb_lock_init();
            pv->decomb_complete_lock[i] = hb_lock_init();
    
            /*
             * Important to start off with the threads locked waiting
             * on input.
             */
            hb_lock( pv->decomb_begin_lock[i] );
    
            pv->decomb_arguments[i].stop = 0;
    
            pv->decomb_threads[i] = hb_thread_init( "decomb_filter_segment",
                                                   decomb_filter_thread,
                                                   decomb_thread_args,
                                                   HB_NORMAL_PRIORITY );
        }
        else
        {
            hb_error( "decomb could not create threads" );
        }
    }
    
    if( pv->mode == 5 )
    {
        /*
         * Create eedi2 threads and locks.
         */
        pv->eedi2_threads = malloc( sizeof( hb_thread_t* ) * 3 );
        pv->eedi2_begin_lock = malloc( sizeof( hb_lock_t * ) * 3 );
        pv->eedi2_complete_lock = malloc( sizeof( hb_lock_t * ) * 3 );
        pv->eedi2_arguments = malloc( sizeof( eedi2_arguments_t ) * 3 );

        if( pv->post_processing > 1 )
        {
            pv->cx2 = (int*)eedi2_aligned_malloc(pv->height[0]*pv->ref_stride[0]*sizeof(int), 16);
            pv->cy2 = (int*)eedi2_aligned_malloc(pv->height[0]*pv->ref_stride[0]*sizeof(int), 16);
            pv->cxy = (int*)eedi2_aligned_malloc(pv->height[0]*pv->ref_stride[0]*sizeof(int), 16);
            pv->tmpc = (int*)eedi2_aligned_malloc(pv->height[0]*pv->ref_stride[0]*sizeof(int), 16);
            if( !pv->cx2 || !pv->cy2 || !pv->cxy || !pv->tmpc )
                hb_log("EEDI2: failed to malloc derivative arrays");
            else
                hb_log("EEDI2: successfully mallloced derivative arrays");
        }

        for( i = 0; i < 3; i++ )
        {
            eedi2_thread_arg_t *eedi2_thread_args;

            eedi2_thread_args = malloc( sizeof( eedi2_thread_arg_t ) );

            if( eedi2_thread_args )
            {
                eedi2_thread_args->pv = pv;
                eedi2_thread_args->plane = i;

                pv->eedi2_begin_lock[i] = hb_lock_init();
                pv->eedi2_complete_lock[i] = hb_lock_init();

                /*
                 * Important to start off with the threads locked waiting
                 * on input.
                 */
                hb_lock( pv->eedi2_begin_lock[i] );

                pv->eedi2_arguments[i].stop = 0;

                pv->eedi2_threads[i] = hb_thread_init( "eedi2_filter_segment",
                                                       eedi2_filter_thread,
                                                       eedi2_thread_args,
                                                       HB_NORMAL_PRIORITY );
            }
            else
            {
                hb_error( "eedi2 could not create threads" );
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

            hb_avcodec_open(avctx_enc, enc);
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
    
    hb_log("decomb: %s deinterlaced %i | blend deinterlaced %i | unfiltered %i | total %i", pv->mode == 5 ? "yadif+eedi2" : "yadif", pv->yadif_deinterlaced_frames, pv->blend_deinterlaced_frames, pv->unfiltered_frames, pv->yadif_deinterlaced_frames + pv->blend_deinterlaced_frames + pv->unfiltered_frames);

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
    
    /* Cleanup combing mask. */
    for( i = 0; i<3*3; i++ )
    {
        uint8_t **p = &pv->mask[i/3];
        if (*p)
        {
            free( *p - 3*pv->ref_stride[i/3] );
            *p = NULL;
        }
    }
    
    if( pv->mode == 5 )
    {
        /* Cleanup eedi-half  buffers */
        int j;
        for( i = 0; i<3; i++ )
        {
            for( j = 0; j < 4; j++ )
            {
                uint8_t **p = &pv->eedi_half[j][i];
                if (*p)
                {
                    free( *p - 3*pv->ref_stride[i] );
                    *p = NULL;
                }            
            }
        }

        /* Cleanup eedi-full  buffers */
        for( i = 0; i<3; i++ )
        {
            for( j = 0; j < 5; j++ )
            {
                uint8_t **p = &pv->eedi_full[j][i];
                if (*p)
                {
                    free( *p - 3*pv->ref_stride[i] );
                    *p = NULL;
                }            
            }
        }
    }
    
    if( pv->post_processing > 1  && pv->mode == 5 )
    {
        if (pv->cx2) eedi2_aligned_free(pv->cx2);
        if (pv->cy2) eedi2_aligned_free(pv->cy2);
        if (pv->cxy) eedi2_aligned_free(pv->cxy);
        if (pv->tmpc) eedi2_aligned_free(pv->tmpc);
    }
    
    for( i = 0; i < pv->cpu_count; i++)
    {
        /*
         * Tell each yadif thread to stop, and then cleanup.
         */
        pv->yadif_arguments[i].stop = 1;
        hb_unlock(  pv->yadif_begin_lock[i] );

        hb_thread_close( &pv->yadif_threads[i] );
        hb_lock_close( &pv->yadif_begin_lock[i] );
        hb_lock_close( &pv->yadif_complete_lock[i] );
    }
    
    /*
     * free memory for yadif structs
     */
    free( pv->yadif_threads );
    free( pv->yadif_begin_lock );
    free( pv->yadif_complete_lock );
    free( pv->yadif_arguments );
    
    for( i = 0; i < pv->cpu_count; i++)
    {
        /*
         * Tell each decomb thread to stop, and then cleanup.
         */
        pv->decomb_arguments[i].stop = 1;
        hb_unlock(  pv->decomb_begin_lock[i] );

        hb_thread_close( &pv->decomb_threads[i] );
        hb_lock_close( &pv->decomb_begin_lock[i] );
        hb_lock_close( &pv->decomb_complete_lock[i] );
    }
    
    /*
     * free memory for decomb structs
     */
    free( pv->decomb_threads );
    free( pv->decomb_begin_lock );
    free( pv->decomb_complete_lock );
    free( pv->decomb_arguments );
    
    if( pv->mode == 5 )
    {
        for( i = 0; i < 3; i++)
        {
            /*
             * Tell each eedi2 thread to stop, and then cleanup.
             */
            pv->eedi2_arguments[i].stop = 1;
            hb_unlock(  pv->eedi2_begin_lock[i] );

            hb_thread_close( &pv->eedi2_threads[i] );
            hb_lock_close( &pv->eedi2_begin_lock[i] );
            hb_lock_close( &pv->eedi2_complete_lock[i] );
        }

        /*
         * free memory for eedi2 structs
         */
        free( pv->eedi2_threads );
        free( pv->eedi2_begin_lock );
        free( pv->eedi2_complete_lock );
        free( pv->eedi2_arguments );
    }
    
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

    free( pv );
}

int hb_decomb_work( const hb_buffer_t * cbuf_in,
                    hb_buffer_t ** buf_out,
                    int pix_fmt,
                    int width,
                    int height,
                    hb_filter_private_t * pv )
{
    hb_buffer_t * buf_in = (hb_buffer_t *)cbuf_in;

    if( !pv ||
        pix_fmt != pv->pix_fmt ||
        width   != pv->width[0] ||
        height  != pv->height[0] )
    {
        return FILTER_FAILED;
    }

    avpicture_fill( &pv->pic_in, buf_in->data,
                    pix_fmt, width, height );

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

    pv->tff = tff;
    
    /* Store current frame in yadif cache */
    store_ref( (const uint8_t**)pv->pic_in.data, pv );

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

    /* Perform yadif filtering */        
    int frame;
    for( frame = 0; frame <= ( ( pv->mode == 2 || pv->mode == 3 )? 1 : 0 ) ; frame++ )
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

    /* Copy buffered settings to output buffer settings */
    hb_buffer_copy_settings( *buf_out, pv->buf_settings );

    /* Replace buffered settings with input buffer settings */
    hb_buffer_copy_settings( pv->buf_settings, buf_in );

    /* don't let 'work_loop' send a chapter mark upstream */
    buf_in->new_chap  = 0;

    return FILTER_OK;
}
