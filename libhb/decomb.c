/* decomb.c

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   The yadif algorithm was created by Michael Niedermayer.
   Tritical's work inspired much of the comb detection code:
   http://web.missouri.edu/~kes25c/
*/

/*****
Parameters:
    Mode:
        1 = yadif
        2 = blend
        4 = cubic interpolation
        8 = EEDI2 interpolation
       16 = Deinterlace each field to a separate frame
       32 = Selectively deinterlace based on comb detection

Appended for EEDI2:
    Magnitude thresh : Variance thresh : Laplacian thresh : Dilation thresh :
    Erosion thresh : Noise thresh : Max search distance : Post-processing

Plus:
    Parity

Defaults:
    7:10:20:20:4:2:50:24:1:-1

*****/

/*****
These modes can be layered. For example, Yadif (1) + EEDI2 (8) = 9,
which will feed EEDI2 interpolations to yadif.

** Working combos:
 1: Just yadif
 2: Just blend
 3: Switch between yadif and blend
 4: Just cubic interpolate
 5: Cubic->yadif
 6: Switch between cubic and blend
 7: Switch between cubic->yadif and blend
 8: Just EEDI2 interpolate
 9: EEDI2->yadif
10: Switch between EEDI2 and blend
11: Switch between EEDI2->yadif and blend
...okay I'm getting bored now listing all these different modes

12-15: EEDI2 will override cubic interpolation
*****/

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/eedi2.h"
#include "handbrake/taskset.h"
#include "handbrake/decomb.h"

#define PARITY_DEFAULT   -1

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
    hb_buffer_t *dst;
    int parity;
    int tff;
    int mode;
};

typedef struct yadif_arguments_s yadif_arguments_t;

typedef struct eedi2_thread_arg_s {
    hb_filter_private_t *pv;
    int plane;
} eedi2_thread_arg_t;

typedef struct yadif_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
    int segment_start[3];
    int segment_height[3];
} yadif_thread_arg_t;

struct hb_filter_private_s
{
    // Decomb parameters
    int              mode;

    /* Make buffers to store a comb masks. */
    hb_buffer_t    * mask;
    hb_buffer_t    * mask_filtered;
    hb_buffer_t    * mask_temp;
    int              mask_box_x;
    int              mask_box_y;
    uint8_t          mask_box_color;

    // EEDI2 parameters
    int                 magnitude_threshold;
    int                 variance_threshold;
    int                 laplacian_threshold;
    int                 dilation_threshold;
    int                 erosion_threshold;
    int                 noise_threshold;
    int                 maximum_search_distance;
    int                 post_processing;

    // Deinterlace parameters
    int                 parity;
    int                 tff;

    int                 yadif_ready;

    int                 deinterlaced;
    int                 blended;
    int                 unfiltered;
    int                 frames;

    hb_buffer_t       * ref[3];

    hb_buffer_t       * eedi_half[4];
    hb_buffer_t       * eedi_full[5];
    int               * cx2;
    int               * cy2;
    int               * cxy;
    int               * tmpc;

    int                 cpu_count;
    int                 segment_height[3];

    taskset_t           yadif_taskset;     // Threads for Yadif - one per CPU
    yadif_arguments_t * yadif_arguments;   // Arguments to thread for work

    taskset_t           eedi2_taskset;     // Threads for eedi2 - one per plane

    hb_buffer_list_t    out_list;

    hb_filter_init_t    input;
    hb_filter_init_t    output;
};

typedef struct
{
    int tap[5];
    int normalize;
} filter_param_t;

static int hb_decomb_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init );

static int hb_decomb_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out );

static void hb_decomb_close( hb_filter_object_t * filter );

static const char decomb_template[] =
    "mode=^"HB_INT_REG"$:"
    "magnitude-thresh=^"HB_INT_REG"$:variance-thresh=^"HB_INT_REG"$:"
    "laplacian-thresh=^"HB_INT_REG"$:dilation-thresh=^"HB_INT_REG"$:"
    "erosion-thresh=^"HB_INT_REG"$:noise-thresh=^"HB_INT_REG"$:"
    "search-distance=^"HB_INT_REG"$:postproc=^([0-3])$:parity=^([01])$";

hb_filter_object_t hb_filter_decomb =
{
    .id                = HB_FILTER_DECOMB,
    .enforce_order     = 1,
    .name              = "Decomb",
    .settings          = NULL,
    .init              = hb_decomb_init,
    .work              = hb_decomb_work,
    .close             = hb_decomb_close,
    .settings_template = decomb_template,
};

// Borrowed from libav
#define times4(x) x, x, x, x
#define times1024(x) times4(times4(times4(times4(times4(x)))))

static const uint8_t hb_crop_table[256 + 2 * 1024] = {
times1024(0x00),
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
times1024(0xFF)
};

static inline int cubic_interpolate_pixel( int y0, int y1, int y2, int y3 )
{
    /* From http://www.neuron2.net/library/cubicinterp.html */
    int result = ( y0 * -3 ) + ( y1 * 23 ) + ( y2 * 23 ) + ( y3 * -3 );
    result = hb_crop_table[(result / 40) + 1024];

    return result;
}

static void cubic_interpolate_line(
        uint8_t *dst,
        uint8_t *cur,
        int width,
        int height,
        int stride,
        int y)
{
    int w = width;
    int x;

    for( x = 0; x < w; x++)
    {
        int a, b, c, d;
        a = b = c = d = 0;

        if( y >= 3 )
        {
            /* Normal top*/
            a = cur[-3*stride];
            b = cur[-stride];
        }
        else if( y == 2 || y == 1 )
        {
            /* There's only one sample above this pixel, use it twice. */
            a = cur[-stride];
            b = cur[-stride];
        }
        else if( y == 0 )
        {
            /* No samples above, triple up on the one below. */
            a = cur[+stride];
            b = cur[+stride];
        }

        if( y <= ( height - 4 ) )
        {
            /* Normal bottom*/
            c = cur[+stride];
            d = cur[3*stride];
        }
        else if( y == ( height - 3 ) || y == ( height - 2 ) )
        {
            /* There's only one sample below, use it twice. */
            c = cur[+stride];
            d = cur[+stride];
        }
        else if( y == height - 1)
        {
            /* No samples below, triple up on the one above. */
            c = cur[-stride];
            d = cur[-stride];
        }

        dst[0] = cubic_interpolate_pixel( a, b, c, d );

        dst++;
        cur++;
    }
}

static void store_ref(hb_filter_private_t * pv, hb_buffer_t * b)
{
    hb_buffer_close(&pv->ref[0]);
    memmove(&pv->ref[0], &pv->ref[1], sizeof(hb_buffer_t *) * 2 );
    pv->ref[2] = b;
}

static inline int blend_filter_pixel(filter_param_t *filter, int up2, int up1, int current, int down1, int down2)
{
    /* Low-pass 5-tap filter */
    int result = 0;

    result += up2 * filter->tap[0];
    result += up1 * filter->tap[1];
    result += current * filter->tap[2];
    result += down1 * filter->tap[3];
    result += down2 * filter->tap[4];
    result >>= filter->normalize;

    result = hb_crop_table[result + 1024];
    return result;
}

static void blend_filter_line(filter_param_t *filter,
                               uint8_t *dst,
                               uint8_t *cur,
                               int width,
                               int height,
                               int stride,
                               int y)
{
    int w = width;
    int x;
    int up1, up2, down1, down2;

    if (y > 1 && y < (height - 2))
    {
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == 0)
    {
        /* First line, so A and B don't exist.*/
        up1 = up2 = 0;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == 1)
    {
        /* Second line, no A. */
        up1 = up2 = -1 * stride;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == (height - 2))
    {
        /* Second to last line, no E. */
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = down2 = 1 * stride;
    }
    else if (y == (height -1))
    {
        /* Last line, no D or E. */
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = down2 = 0;
    }
    else
    {
        hb_error("Invalid value y %d height %d", y, height);
        return;
    }

    for( x = 0; x < w; x++)
    {
        /* Low-pass 5-tap filter */
        dst[0] = blend_filter_pixel(filter, cur[up2], cur[up1], cur[0],
                                    cur[down1], cur[down2] );
        dst++;
        cur++;
    }
}

// This function calls all the eedi2 filters in sequence for a given plane.
// It outputs the final interpolated image to pv->eedi_full[DST2PF].
static void eedi2_interpolate_plane( hb_filter_private_t * pv, int plane )
{
    /* We need all these pointers. No, seriously.
       I swear. It's not a joke. They're used.
       All nine of them.                         */
    uint8_t * mskp = pv->eedi_half[MSKPF]->plane[plane].data;
    uint8_t * srcp = pv->eedi_half[SRCPF]->plane[plane].data;
    uint8_t * tmpp = pv->eedi_half[TMPPF]->plane[plane].data;
    uint8_t * dstp = pv->eedi_half[DSTPF]->plane[plane].data;
    uint8_t * dst2p = pv->eedi_full[DST2PF]->plane[plane].data;
    uint8_t * tmp2p2 = pv->eedi_full[TMP2PF2]->plane[plane].data;
    uint8_t * msk2p = pv->eedi_full[MSK2PF]->plane[plane].data;
    uint8_t * tmp2p = pv->eedi_full[TMP2PF]->plane[plane].data;
    uint8_t * dst2mp = pv->eedi_full[DST2MPF]->plane[plane].data;
    int * cx2 = pv->cx2;
    int * cy2 = pv->cy2;
    int * cxy = pv->cxy;
    int * tmpc = pv->tmpc;

    int pitch = pv->eedi_full[0]->plane[plane].stride;
    int height = pv->eedi_full[0]->plane[plane].height;
    int width = pv->eedi_full[0]->plane[plane].width;
    int half_height = pv->eedi_half[0]->plane[plane].height;

    // edge mask
    eedi2_build_edge_mask( mskp, pitch, srcp, pitch,
                     pv->magnitude_threshold, pv->variance_threshold, pv->laplacian_threshold,
                     half_height, width );
    eedi2_erode_edge_mask( mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width );
    eedi2_dilate_edge_mask( tmpp, pitch, mskp, pitch, pv->dilation_threshold, half_height, width );
    eedi2_erode_edge_mask( mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width );
    eedi2_remove_small_gaps( tmpp, pitch, mskp, pitch, half_height, width );

    // direction mask
    eedi2_calc_directions( plane, mskp, pitch, srcp, pitch, tmpp, pitch,
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
    eedi2_interpolate_lattice( plane, tmp2p, pitch, dst2p, pitch, tmp2p2, pitch, pv->tff,
                         pv->noise_threshold, height, width );

    if( pv->post_processing == 1 || pv->post_processing == 3 )
    {
        // make sure the edge directions are consistent
        eedi2_bit_blit( tmp2p2, pitch, tmp2p, pitch, width, height );
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
static void eedi2_filter_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int plane;
    eedi2_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    plane = thread_args->plane;

    hb_deep_log(3, "eedi2 thread started for plane %d", plane);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->eedi2_taskset, plane );

        if( taskset_thread_stop( &pv->eedi2_taskset, plane ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        /*
         * Process plane
         */
        eedi2_interpolate_plane( pv, plane );

        /*
         * Finished this segment, let everyone know.
         */
        taskset_thread_complete( &pv->eedi2_taskset, plane );
    }

    taskset_thread_complete( &pv->eedi2_taskset, plane );
}

// Sets up the input field planes for EEDI2 in pv->eedi_half[SRCPF]
// and then runs eedi2_filter_thread for each plane.
static void eedi2_planer( hb_filter_private_t * pv )
{
    /* Copy the first field from the source to a half-height frame. */
    int pp;
    for( pp = 0;  pp < 3; pp++ )
    {
        int pitch = pv->ref[1]->plane[pp].stride;
        int height = pv->ref[1]->plane[pp].height;
        int start_line = !pv->tff;

        eedi2_fill_half_height_buffer_plane(
                &pv->ref[1]->plane[pp].data[pitch * start_line],
                pv->eedi_half[SRCPF]->plane[pp].data, pitch, height );
    }

    /*
     * Now that all data is ready for our threads, fire them off
     * and wait for their completion.
     */
    taskset_cycle( &pv->eedi2_taskset );
}

/* EDDI: Edge Directed Deinterlacing Interpolation
   Checks 4 different slopes to see if there is more similarity along a diagonal
   than there was vertically. If a diagonal is more similar, then it indicates
   an edge, so interpolate along that instead of a vertical line, using either
   linear or cubic interpolation depending on mode. */
#define YADIF_CHECK(j) {\
        int score = ABS(cur[-stride-1+j] - cur[+stride-1-j])\
                      + ABS(cur[-stride  +j] - cur[+stride  -j])\
                      + ABS(cur[-stride+1+j] - cur[+stride+1-j]);\
        if( score < spatial_score ){\
            spatial_score = score;\
            if( ( pv->mode & MODE_DECOMB_CUBIC ) && !vertical_edge )\
            {\
                switch(j)\
                {\
                    case -1:\
                        spatial_pred = cubic_interpolate_pixel(cur[-3 * stride - 3], cur[-stride -1], cur[+stride + 1], cur[3* stride + 3] );\
                    break;\
                    case -2:\
                        spatial_pred = cubic_interpolate_pixel( ( ( cur[-3*stride - 4] + cur[-stride - 4] ) / 2 ) , cur[-stride -2], cur[+stride + 2], ( ( cur[3*stride + 4] + cur[stride + 4] ) / 2 ) );\
                    break;\
                    case 1:\
                        spatial_pred = cubic_interpolate_pixel(cur[-3 * stride +3], cur[-stride +1], cur[+stride - 1], cur[3* stride -3] );\
                    break;\
                    case 2:\
                        spatial_pred = cubic_interpolate_pixel(( ( cur[-3*stride + 4] + cur[-stride + 4] ) / 2 ), cur[-stride +2], cur[+stride - 2], ( ( cur[3*stride - 4] + cur[stride - 4] ) / 2 ) );\
                    break;\
                }\
            }\
            else\
            {\
                spatial_pred = ( cur[-stride +j] + cur[+stride -j] ) >>1;\
            }\

static void yadif_filter_line(
       hb_filter_private_t * pv,
       uint8_t             * dst,
       uint8_t             * prev,
       uint8_t             * cur,
       uint8_t             * next,
       int                   plane,
       int                   width,
       int                   height,
       int                   stride,
       int                   parity,
       int                   y)
{
    /* While prev and next point to the previous and next frames,
       prev2 and next2 will shift depending on the parity, usually 1.
       They are the previous and next fields, the fields temporally adjacent
       to the other field in the current frame--the one not being filtered.  */
    uint8_t *prev2 = parity ? prev : cur ;
    uint8_t *next2 = parity ? cur  : next;

    int x;
    int eedi2_mode = ( pv->mode & MODE_DECOMB_EEDI2 );

    /* We can replace spatial_pred with this interpolation*/
    uint8_t * eedi2_guess = NULL;
    if (eedi2_mode)
    {
        eedi2_guess = &pv->eedi_full[DST2PF]->plane[plane].data[y*stride];
    }

    /* Decomb's cubic interpolation can only function when there are
       three samples above and below, so regress to yadif's traditional
       two-tap interpolation when filtering at the top and bottom edges. */
    int vertical_edge = 0;
    if( ( y < 3 ) || ( y > ( height - 4 ) )  )
        vertical_edge = 1;

    for( x = 0; x < width; x++)
    {
        /* Pixel above*/
        int c              = cur[-stride];
        /* Temporal average: the current location in the adjacent fields */
        int d              = (prev2[0] + next2[0])>>1;
        /* Pixel below */
        int e              = cur[+stride];

        /* How the current pixel changes between the adjacent fields */
        int temporal_diff0 = ABS(prev2[0] - next2[0]);
        /* The average of how much the pixels above and below change from the frame before to now. */
        int temporal_diff1 = ( ABS(prev[-stride] - cur[-stride]) + ABS(prev[+stride] - cur[+stride]) ) >> 1;
        /* The average of how much the pixels above and below change from now to the next frame. */
        int temporal_diff2 = ( ABS(next[-stride] - cur[-stride]) + ABS(next[+stride] - cur[+stride]) ) >> 1;
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
            int spatial_score  = ABS(cur[-stride-1] - cur[+stride-1]) + ABS(cur[-stride]-cur[+stride]) +
                                         ABS(cur[-stride+1] - cur[+stride+1]) - 1;

            /* Spatial pred is either a bilinear or cubic vertical interpolation. */
            if( ( pv->mode & MODE_DECOMB_CUBIC ) && !vertical_edge)
            {
                spatial_pred = cubic_interpolate_pixel( cur[-3*stride], cur[-stride], cur[+stride], cur[3*stride] );
            }
            else
            {
                spatial_pred = (c+e)>>1;
            }

            // YADIF_CHECK requires a margin to avoid invalid memory access.
            // In MODE_DECOMB_CUBIC, margin needed is 2 + ABS(param).
            // Else, the margin needed is 1 + ABS(param).
            int margin = 2;
            if (pv->mode & MODE_DECOMB_CUBIC)
                margin = 3;

            if (x >= margin && x <= width - (margin + 1))
            {
                YADIF_CHECK(-1)
                if (x >= margin + 1 && x <= width - (margin + 2))
                    YADIF_CHECK(-2) }} }}
            }
            if (x >= margin && x <= width - (margin + 1))
            {
                YADIF_CHECK(1)
                if (x >= margin + 1 && x <= width - (margin + 2))
                    YADIF_CHECK(2) }} }}
            }
        }

        /* Temporally adjust the spatial prediction by
           comparing against lines in the adjacent fields. */
        int b = (prev2[-2*stride] + next2[-2*stride])>>1;
        int f = (prev2[+2*stride] + next2[+2*stride])>>1;

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
static void yadif_decomb_filter_thread( void *thread_args_v )
{
    yadif_arguments_t *yadif_work = NULL;
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    yadif_thread_arg_t *thread_args = thread_args_v;
    filter_param_t filter;

    filter.tap[0] = -1;
    filter.tap[1] = 2;
    filter.tap[2] = 6;
    filter.tap[3] = 2;
    filter.tap[4] = -1;
    filter.normalize = 3;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "yadif thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->yadif_taskset, segment );

        if( taskset_thread_stop( &pv->yadif_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        yadif_work = &pv->yadif_arguments[segment];

        /*
         * Process all three planes, but only this segment of it.
         */
        hb_buffer_t *dst;
        int parity, tff, mode;

        mode = pv->yadif_arguments[segment].mode;
        dst = yadif_work->dst;
        tff = yadif_work->tff;
        parity = yadif_work->parity;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            int yy;
            int width = dst->plane[pp].width;
            int stride = dst->plane[pp].stride;
            int height = dst->plane[pp].height_stride;
            int penultimate = height - 2;

            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            // Filter parity lines
            int start = parity ? (segment_start + 1) & ~1 : segment_start | 1;
            uint8_t *dst2 = &dst->plane[pp].data[start * stride];
            uint8_t *prev = &pv->ref[0]->plane[pp].data[start * stride];
            uint8_t *cur  = &pv->ref[1]->plane[pp].data[start * stride];
            uint8_t *next = &pv->ref[2]->plane[pp].data[start * stride];

            if (mode == MODE_DECOMB_BLEND)
            {
                /* These will be useful if we ever do temporal blending. */
                for( yy = start; yy < segment_stop; yy += 2 )
                {
                    /* This line gets blend filtered, not yadif filtered. */
                    blend_filter_line(&filter, dst2, cur, width, height, stride, yy);
                    dst2 += stride * 2;
                    cur += stride * 2;
                }
            }
            else if (mode == MODE_DECOMB_CUBIC)
            {
                for( yy = start; yy < segment_stop; yy += 2 )
                {
                    /* Just apply vertical cubic interpolation */
                    cubic_interpolate_line(dst2, cur, width, height, stride, yy);
                    dst2 += stride * 2;
                    cur += stride * 2;
                }
            }
            else if (mode & MODE_DECOMB_YADIF)
            {
                for( yy = start; yy < segment_stop; yy += 2 )
                {
                    if( yy > 1 && yy < penultimate )
                    {
                        // This isn't the top or bottom,
                        // proceed as normal to yadif
                        yadif_filter_line(pv, dst2, prev, cur, next, pp,
                                          width, height, stride,
                                          parity ^ tff, yy);
                    }
                    else
                    {
                        // parity == 0 (TFF), y1 = y0
                        // parity == 1 (BFF), y0 = y1
                        // parity == 0 (TFF), yu = yp
                        // parity == 1 (BFF), yp = yu
                        int yp = (yy ^ parity) * stride;
                        memcpy(dst2, &pv->ref[1]->plane[pp].data[yp], width);
                    }
                    dst2 += stride * 2;
                    prev += stride * 2;
                    cur += stride * 2;
                    next += stride * 2;
                }
            }
            else
            {
                // No combing, copy frame
                for( yy = start; yy < segment_stop; yy += 2 )
                {
                    memcpy(dst2, cur, width);
                    dst2 += stride * 2;
                    cur += stride * 2;
                }
            }

            // Copy unfiltered lines
            start = !parity ? (segment_start + 1) & ~1 : segment_start | 1;
            dst2 = &dst->plane[pp].data[start * stride];
            prev = &pv->ref[0]->plane[pp].data[start * stride];
            cur  = &pv->ref[1]->plane[pp].data[start * stride];
            next = &pv->ref[2]->plane[pp].data[start * stride];
            for( yy = start; yy < segment_stop; yy += 2 )
            {
                memcpy(dst2, cur, width);
                dst2 += stride * 2;
                cur += stride * 2;
            }
        }
        taskset_thread_complete( &pv->yadif_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->yadif_taskset, segment );
}

static void yadif_filter( hb_filter_private_t * pv,
                          hb_buffer_t * dst,
                          int parity,
                          int tff)
{
    /* If we're running comb detection, do it now, otherwise default to true. */
    int is_combed = HB_COMB_HEAVY;
    int mode = 0;

    if (pv->mode & MODE_DECOMB_SELECTIVE)
    {
        is_combed = pv->ref[1]->s.combed;
    }

    // Pick a mode based on the comb detect state and selected decomb modes
    if ((pv->mode & MODE_DECOMB_BLEND) && is_combed == HB_COMB_LIGHT )
    {
        mode = MODE_DECOMB_BLEND;
    }
    else if (is_combed != HB_COMB_NONE)
    {
        mode = pv->mode & ~MODE_DECOMB_SELECTIVE;
    }

    if (mode == MODE_DECOMB_BLEND)
    {
        pv->blended++;
    }
    else if (mode != 0)
    {
        pv->deinterlaced++;
    }
    else
    {
        pv->unfiltered++;
    }
    pv->frames++;

    if (mode & MODE_DECOMB_EEDI2)
    {
        /* Generate an EEDI2 interpolation */
        eedi2_planer( pv );
    }

    if (mode != 0)
    {
        if ((mode & MODE_DECOMB_EEDI2 ) && !(mode & MODE_DECOMB_YADIF))
        {
            // Just pass through the EEDI2 interpolation
            int pp;
            for( pp = 0; pp < 3; pp++ )
            {
                uint8_t * ref = pv->eedi_full[DST2PF]->plane[pp].data;
                int ref_stride = pv->eedi_full[DST2PF]->plane[pp].stride;

                uint8_t * dest = dst->plane[pp].data;
                int width = dst->plane[pp].width;
                int height = dst->plane[pp].height;
                int stride = dst->plane[pp].stride;

                int yy;
                for( yy = 0; yy < height; yy++ )
                {
                    memcpy(dest, ref, width);
                    dest += stride;
                    ref += ref_stride;
                }
            }
        }
        else
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
                pv->yadif_arguments[segment].mode = mode;
            }

            /*
             * Allow the taskset threads to make one pass over the data.
             */
            taskset_cycle( &pv->yadif_taskset );

            /*
             * Entire frame is now deinterlaced.
             */
        }
    }
    else
    {
        /*  Just passing through... */
        pv->yadif_arguments[0].mode = mode; // 0
        hb_buffer_copy(dst, pv->ref[1]);
    }
}

static int hb_decomb_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;
    pv->input                = *init;
    hb_buffer_list_clear(&pv->out_list);

    pv->deinterlaced = 0;
    pv->blended      = 0;
    pv->unfiltered   = 0;
    pv->frames       = 0;
    pv->yadif_ready  = 0;

    pv->mode                    = MODE_DECOMB_YADIF | MODE_DECOMB_BLEND |
                                  MODE_DECOMB_CUBIC;
    pv->magnitude_threshold     = 10;
    pv->variance_threshold      = 20;
    pv->laplacian_threshold     = 20;
    pv->dilation_threshold      = 4;
    pv->erosion_threshold       = 2;
    pv->noise_threshold         = 50;
    pv->maximum_search_distance = 24;
    pv->post_processing         = 1;
    pv->parity                  = PARITY_DEFAULT;

    if (filter->settings)
    {
        hb_value_t * dict = filter->settings;

        // Get comb detection settings
        hb_dict_extract_int(&pv->mode, dict, "mode");

        // Get deinterlace settings
        hb_dict_extract_int(&pv->parity, dict, "parity");
        if (pv->mode & MODE_DECOMB_EEDI2)
        {
            hb_dict_extract_int(&pv->magnitude_threshold, dict,
                                "magnitude-thresh");
            hb_dict_extract_int(&pv->variance_threshold, dict,
                                "variance-thresh");
            hb_dict_extract_int(&pv->laplacian_threshold, dict,
                                "laplacian-thresh");
            hb_dict_extract_int(&pv->dilation_threshold, dict,
                                "dilation-thresh");
            hb_dict_extract_int(&pv->erosion_threshold, dict,
                                "erosion-thresh");
            hb_dict_extract_int(&pv->noise_threshold, dict,
                                "noise-thresh");
            hb_dict_extract_int(&pv->maximum_search_distance, dict,
                                "search-distance");
            hb_dict_extract_int(&pv->post_processing, dict,
                                "postproc");
        }
    }

    pv->cpu_count = hb_get_cpu_count();

    // Make segment sizes an even number of lines
    int height = hb_image_height(init->pix_fmt, init->geometry.height, 0);
    // Each segment must begin on the even "parity" row.
    // I.e. each segment of each plane must begin on an even row.
    pv->segment_height[0] = (height / pv->cpu_count) & ~3;
    pv->segment_height[1] = hb_image_height(init->pix_fmt, pv->segment_height[0], 1);
    pv->segment_height[2] = hb_image_height(init->pix_fmt, pv->segment_height[0], 2);

    int ii;
    if( pv->mode & MODE_DECOMB_EEDI2 )
    {
        /* Allocate half-height eedi2 buffers */
        for( ii = 0; ii < 4; ii++ )
        {
            pv->eedi_half[ii] = hb_frame_buffer_init(
                init->pix_fmt, init->geometry.width, init->geometry.height / 2);
        }

        /* Allocate full-height eedi2 buffers */
        for( ii = 0; ii < 5; ii++ )
        {
            pv->eedi_full[ii] = hb_frame_buffer_init(
                init->pix_fmt, init->geometry.width, init->geometry.height);
        }
    }

    /*
     * Setup yadif taskset.
     */
    pv->yadif_arguments = malloc( sizeof( yadif_arguments_t ) * pv->cpu_count );
    if( pv->yadif_arguments == NULL ||
        taskset_init( &pv->yadif_taskset, pv->cpu_count,
                      sizeof( yadif_thread_arg_t ) ) == 0 )
    {
        hb_error( "yadif could not initialize taskset" );
    }

    yadif_thread_arg_t *yadif_prev_thread_args = NULL;
    for( ii = 0; ii < pv->cpu_count; ii++ )
    {
        yadif_thread_arg_t *thread_args;

        thread_args = taskset_thread_args( &pv->yadif_taskset, ii );
        thread_args->pv = pv;
        thread_args->segment = ii;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            if (yadif_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    yadif_prev_thread_args->segment_start[pp] +
                    yadif_prev_thread_args->segment_height[pp];
            }
            if( ii == pv->cpu_count - 1 )
            {
                /*
                 * Final segment
                 */
                thread_args->segment_height[pp] =
                    ((hb_image_height(init->pix_fmt, init->geometry.height, pp)
                     + 3) & ~3) - thread_args->segment_start[pp];
            } else {
                thread_args->segment_height[pp] = pv->segment_height[pp];
            }
        }
        pv->yadif_arguments[ii].dst = NULL;
        if( taskset_thread_spawn( &pv->yadif_taskset, ii,
                                 "yadif_filter_segment",
                                 yadif_decomb_filter_thread,
                                 HB_NORMAL_PRIORITY ) == 0 )
        {
            hb_error( "yadif could not spawn thread" );
        }
        yadif_prev_thread_args = thread_args;
    }

    if( pv->mode & MODE_DECOMB_EEDI2 )
    {
        /*
         * Create eedi2 taskset.
         */
        if( taskset_init( &pv->eedi2_taskset, /*thread_count*/3,
                          sizeof( eedi2_thread_arg_t ) ) == 0 )
        {
            hb_error( "eedi2 could not initialize taskset" );
        }

        if( pv->post_processing > 1 )
        {
            int stride;
            stride = hb_image_stride(init->pix_fmt, init->geometry.width, 0);

            pv->cx2 = (int*)eedi2_aligned_malloc(
                    init->geometry.height * stride * sizeof(int), 16);

            pv->cy2 = (int*)eedi2_aligned_malloc(
                    init->geometry.height * stride * sizeof(int), 16);

            pv->cxy = (int*)eedi2_aligned_malloc(
                    init->geometry.height * stride * sizeof(int), 16);

            pv->tmpc = (int*)eedi2_aligned_malloc(
                    init->geometry.height * stride * sizeof(int), 16);

            if( !pv->cx2 || !pv->cy2 || !pv->cxy || !pv->tmpc )
                hb_error("EEDI2: failed to malloc derivative arrays");
            else
                hb_log("EEDI2: successfully malloced derivative arrays");
        }

        for( ii = 0; ii < 3; ii++ )
        {
            eedi2_thread_arg_t *eedi2_thread_args;

            eedi2_thread_args = taskset_thread_args( &pv->eedi2_taskset, ii );

            eedi2_thread_args->pv = pv;
            eedi2_thread_args->plane = ii;

            if( taskset_thread_spawn( &pv->eedi2_taskset, ii,
                                      "eedi2_filter_segment",
                                      eedi2_filter_thread,
                                      HB_NORMAL_PRIORITY ) == 0 )
            {
                hb_error( "eedi2 could not spawn thread" );
            }
        }
    }

    pv->output = *init;
    init->job->use_decomb = 1;

    return 0;
}

static void hb_decomb_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    hb_log("decomb: deinterlaced %i | blended %i | unfiltered %i | total %i",
           pv->deinterlaced, pv->blended, pv->unfiltered, pv->frames);

    taskset_fini( &pv->yadif_taskset );

    if( pv->mode & MODE_DECOMB_EEDI2 )
    {
        taskset_fini( &pv->eedi2_taskset );
    }

    /* Cleanup reference buffers. */
    int ii;
    for (ii = 0; ii < 3; ii++)
    {
        hb_buffer_close(&pv->ref[ii]);
    }

    if( pv->mode & MODE_DECOMB_EEDI2 )
    {
        /* Cleanup eedi-half  buffers */
        int ii;
        for( ii = 0; ii < 4; ii++ )
        {
            hb_buffer_close(&pv->eedi_half[ii]);
        }

        /* Cleanup eedi-full  buffers */
        for( ii = 0; ii < 5; ii++ )
        {
            hb_buffer_close(&pv->eedi_full[ii]);
        }
    }

    if( pv->post_processing > 1  && ( pv->mode & MODE_DECOMB_EEDI2 ) )
    {
        if (pv->cx2) eedi2_aligned_free(pv->cx2);
        if (pv->cy2) eedi2_aligned_free(pv->cy2);
        if (pv->cxy) eedi2_aligned_free(pv->cxy);
        if (pv->tmpc) eedi2_aligned_free(pv->tmpc);
    }

    /*
     * free memory for yadif structs
     */
    free( pv->yadif_arguments );

    free( pv );
    filter->private_data = NULL;
}

// Fill rows above height with copy of last row to prevent color distortion
// during blending
static void fill_stride(hb_buffer_t * buf)
{
    int pp, ii;

    for (pp = 0; pp < 3; pp++)
    {
        uint8_t * src, * dst;

        src = buf->plane[pp].data + (buf->plane[pp].height - 1) *
              buf->plane[pp].stride;
        dst = buf->plane[pp].data + buf->plane[pp].height *
              buf->plane[pp].stride;
        for (ii = 0; ii < 3; ii++)
        {
            memcpy(dst, src, buf->plane[pp].stride);
            dst += buf->plane[pp].stride;
        }
    }
}

static void process_frame( hb_filter_private_t * pv )
{
    if ((pv->mode & MODE_DECOMB_SELECTIVE) &&
        pv->ref[1]->s.combed == HB_COMB_NONE)
    {
        // Input buffer is not combed.  Just make a dup of it.
        hb_buffer_t * buf = hb_buffer_dup(pv->ref[1]);
        hb_buffer_list_append(&pv->out_list, buf);
        pv->frames++;
        pv->unfiltered++;
    }
    else
    {
        /* Determine if top-field first layout */
        int tff;
        if (pv->parity < 0)
        {
            tff = !!(pv->ref[1]->s.flags & PIC_FLAG_TOP_FIELD_FIRST);
        }
        else
        {
            tff = (pv->parity & 1) ^ 1;
        }

        /* deinterlace both fields if bob */
        int frame, num_frames = 1;
        if (pv->mode & MODE_DECOMB_BOB)
        {
            num_frames = 2;
        }

        // Will need up to 2 buffers simultaneously

        /* Perform yadif filtering */
        for (frame = 0; frame < num_frames; frame++)
        {
            hb_buffer_t * buf;
            int parity = frame ^ tff ^ 1;

            // tff for eedi2
            pv->tff = !parity;

            buf = hb_frame_buffer_init(pv->ref[1]->f.fmt,
                                       pv->ref[1]->f.width,
                                       pv->ref[1]->f.height);
            buf->f.color_prim     = pv->output.color_prim;
            buf->f.color_transfer = pv->output.color_transfer;
            buf->f.color_matrix   = pv->output.color_matrix;
            buf->f.color_range    = pv->output.color_range ;
            yadif_filter(pv, buf, parity, tff);

            /* Copy buffered settings to output buffer settings */
            buf->s = pv->ref[1]->s;

            hb_buffer_list_append(&pv->out_list, buf);
        }

        /* if this frame was deinterlaced and bob mode is engaged, halve
           the duration of the saved timestamps. */
        if (pv->mode & MODE_DECOMB_BOB)
        {
            hb_buffer_t *first  = hb_buffer_list_head(&pv->out_list);
            hb_buffer_t *second = hb_buffer_list_tail(&pv->out_list);
            first->s.stop -= (first->s.stop - first->s.start) / 2LL;
            second->s.start = first->s.stop;
            second->s.new_chap = 0;
        }
    }
}

static int hb_decomb_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    // Input buffer is always consumed.
    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if (pv->ref[2] != NULL)
        {
            // Duplicate last frame and process refs
            store_ref(pv, hb_buffer_dup(pv->ref[2]));
            process_frame(pv);
        }
        hb_buffer_list_append(&pv->out_list, in);
        *buf_out = hb_buffer_list_clear(&pv->out_list);
        return HB_FILTER_DONE;
    }

    fill_stride(in);

    // yadif requires 3 buffers, prev, cur, and next.  For the first
    // frame, there can be no prev, so we duplicate the first frame.
    if (!pv->yadif_ready)
    {
        // If yadif is not ready, store another ref and return HB_FILTER_DELAY
        store_ref(pv, hb_buffer_dup(in));
        store_ref(pv, in);
        pv->yadif_ready = 1;
        // Wait for next
        return HB_FILTER_DELAY;
    }

    store_ref(pv, in);
    process_frame(pv);

    *buf_out = hb_buffer_list_clear(&pv->out_list);
    return HB_FILTER_OK;
}

void hb_deinterlace(hb_buffer_t *dst, hb_buffer_t *src)
{
    int pp;
    filter_param_t filter;

    filter.tap[0] = -1;
    filter.tap[1] = 4;
    filter.tap[2] = 2;
    filter.tap[3] = 4;
    filter.tap[4] = -1;
    filter.normalize = 3;

    fill_stride(src);
    for (pp = 0; pp < 3; pp++)
    {
        int yy;
        int width  = src->plane[pp].width;
        int stride = src->plane[pp].stride;
        int height = src->plane[pp].height_stride;

        // Filter parity lines
        uint8_t *pdst = &dst->plane[pp].data[0];
        uint8_t *psrc = &src->plane[pp].data[0];

        /* These will be useful if we ever do temporal blending. */
        for( yy = 0; yy < height - 1; yy += 2 )
        {
            /* This line gets blend filtered, not yadif filtered. */
            memcpy(pdst, psrc, width);
            pdst += stride;
            psrc += stride;
            blend_filter_line(&filter, pdst, psrc, width, height, stride, yy + 1);
            pdst += stride;
            psrc += stride;
        }
    }
}

