/* decomb.c

   Copyright (c) 2003-2015 HandBrake Team
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
    Mode : Spatial metric : Motion thresh : Spatial thresh : Mask Filter Mode :
    Block thresh : Block width : Block height

Appended for EEDI2:
    Magnitude thresh : Variance thresh : Laplacian thresh : Dilation thresh :
    Erosion thresh : Noise thresh : Max search distance : Post-processing

Plus:
    Parity

Defaults:
    391:2:3:3:2:40:16:16:10:20:20:4:2:50:24:1:-1

Original "Faster" settings:
    7:2:6:9:1:80:16:16:10:20:20:4:2:50:24:1:-1
*****/
#define MODE_YADIF       1 // Use yadif
#define MODE_BLEND       2 // Use blending interpolation
#define MODE_CUBIC       4 // Use cubic interpolation
#define MODE_EEDI2       8 // Use EEDI2 interpolation
#define MODE_MASK       32 // Output combing masks instead of pictures
#define MODE_BOB        64 // Deinterlace each field to a separate frame

#define MODE_GAMMA      128 // Scale gamma when decombing
#define MODE_FILTER     256 // Filter combing mask
#define MODE_COMPOSITE  512 // Overlay combing mask onto picture

#define FILTER_CLASSIC 1
#define FILTER_ERODE_DILATE 2

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
32: Passes through the combing mask for every combed frame (white for combed pixels, otherwise black)
33+: Overlay the combing mask for every combed frame on top of the filtered output (white for combed pixels)

12-15: EEDI2 will override cubic interpolation
*****/

#include "hb.h"
#include "hbffmpeg.h"
#include "eedi2.h"
#include "taskset.h"

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
    int is_combed;
};

typedef struct yadif_arguments_s yadif_arguments_t;

typedef struct eedi2_thread_arg_s {
    hb_filter_private_t *pv;
    int plane;
} eedi2_thread_arg_t;

typedef struct decomb_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
    int segment_start[3];
    int segment_height[3];
} decomb_thread_arg_t;

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
    int              filter_mode;
    int              spatial_metric;
    int              motion_threshold;
    int              spatial_threshold;
    int              block_threshold;
    int              block_width;
    int              block_height;
    int            * block_score;
    int              comb_check_complete;
    int              comb_check_nthreads;
    int              skip_comb_check;
    int              is_combed;

    float            gamma_lut[256];

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

    int              deinterlaced_frames;
    int              blended_frames;
    int              unfiltered_frames;

    hb_buffer_t    * ref[3];

    /* Make buffers to store a comb masks. */
    hb_buffer_t    * mask;
    hb_buffer_t    * mask_filtered;
    hb_buffer_t    * mask_temp;
    int              mask_box_x;
    int              mask_box_y;
    uint8_t          mask_box_color;


    hb_buffer_t    * eedi_half[4];
    hb_buffer_t    * eedi_full[5];
    int            * cx2;
    int            * cy2;
    int            * cxy;
    int            * tmpc;

    int              cpu_count;
    int              segment_height[3];

    taskset_t        yadif_taskset;       // Threads for Yadif - one per CPU
    yadif_arguments_t *yadif_arguments;   // Arguments to thread for work

    taskset_t        decomb_filter_taskset; // Threads for comb detection
    taskset_t        decomb_check_taskset;  // Threads for comb check
    taskset_t        mask_filter_taskset; // Threads for decomb mask filter
    taskset_t        mask_erode_taskset;  // Threads for decomb mask erode
    taskset_t        mask_dilate_taskset; // Threads for decomb mask dilate

    taskset_t        eedi2_taskset;       // Threads for eedi2 - one per plane
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

hb_filter_object_t hb_filter_decomb =
{
    .id            = HB_FILTER_DECOMB,
    .enforce_order = 1,
    .name          = "Decomb",
    .settings      = NULL,
    .init          = hb_decomb_init,
    .work          = hb_decomb_work,
    .close         = hb_decomb_close,
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

static void draw_mask_box( hb_filter_private_t * pv )
{
    int x = pv->mask_box_x;
    int y = pv->mask_box_y;
    int box_width = pv->block_width;
    int box_height = pv->block_height;
    int stride;
    uint8_t * mskp;

    if (pv->mode & MODE_FILTER)
    {
        mskp = pv->mask_filtered->plane[0].data;
        stride = pv->mask_filtered->plane[0].stride;
    }
    else
    {
        mskp = pv->mask->plane[0].data;
        stride = pv->mask->plane[0].stride;
    }


    int block_x, block_y;
    for( block_x = 0; block_x < box_width; block_x++)
    {
        mskp[y*stride+x+block_x] = 128;
        mskp[(y+box_height)*stride+x+block_x] = 128;
    }

    for( block_y = 0; block_y < box_height; block_y++)
    {
        mskp[stride*(y+block_y)+x] = 128;
        mskp[stride*(y+block_y) + x + box_width] = 128;
    }
}

static void apply_mask_line( uint8_t * srcp,
                             uint8_t * mskp,
                             int width )
{
    int x;

    for( x = 0; x < width; x++ )
    {
        if( mskp[x] == 1 )
        {
            srcp[x] = 255;
        }
        if( mskp[x] == 128 )
        {
            srcp[x] = 128;
        }
    }
}

static void apply_mask(hb_filter_private_t * pv, hb_buffer_t * b)
{
    /* draw_boxes */
    draw_mask_box( pv );

    int pp, yy;
    hb_buffer_t * m;

    if (pv->mode & MODE_FILTER)
    {
        m = pv->mask_filtered;
    }
    else
    {
        m = pv->mask;
    }
    for (pp = 0; pp < 3; pp++)
    {
        uint8_t * dstp = b->plane[pp].data;
        uint8_t * mskp = m->plane[pp].data;

        for( yy = 0; yy < m->plane[pp].height; yy++ )
        {
            if (!(pv->mode & MODE_COMPOSITE) && pp == 0)
            {
                memcpy(dstp, mskp, m->plane[pp].width);
            }
            else if (!(pv->mode & MODE_COMPOSITE))
            {
                memset(dstp, 128, m->plane[pp].width);
            }
            if (pp == 0)
            {
                apply_mask_line(dstp, mskp, m->plane[pp].width);
            }

            dstp += b->plane[pp].stride;
            mskp += m->plane[pp].stride;
        }
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
        hb_error("Invalid value y %d heigh %d", y, height);
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

static void reset_combing_results( hb_filter_private_t * pv )
{
    pv->comb_check_complete = 0;
    int ii;
    for (ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
       pv->block_score[ii] = 0;
    }
}

static int check_combing_results( hb_filter_private_t * pv )
{
    int threshold       = pv->block_threshold;
    int send_to_blend = 0;

    int ii;
    for (ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
        if( pv->block_score[ii] >= ( threshold / 2 ) )
        {
            if (pv->block_score[ii] <= threshold)
            {
                /* Blend video content that scores between
                   ( threshold / 2 ) and threshold.        */
                send_to_blend = 1;
                pv->mask_box_color = 2;
            }
            else if( pv->block_score[ii] > threshold )
            {
                /* Yadif deinterlace video content above the threshold. */
                pv->mask_box_color = 1;
                return 1;
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

static void check_filtered_combing_mask( hb_filter_private_t * pv, int segment, int start, int stop )
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
    int block_score = 0;
    uint8_t * mask_p;
    int x, y, pp;

    for( pp = 0; pp < 1; pp++ )
    {
        int stride = pv->mask_filtered->plane[pp].stride;
        int width = pv->mask_filtered->plane[pp].width;

        pv->mask_box_x = -1;
        pv->mask_box_y = -1;
        pv->mask_box_color = 0;

        for( y = start; y < ( stop - block_height + 1 ); y = y + block_height )
        {
            for( x = 0; x < ( width - block_width ); x = x + block_width )
            {
                block_score = 0;

                for( block_y = 0; block_y < block_height; block_y++ )
                {
                    int my = y + block_y;
                    mask_p = &pv->mask_filtered->plane[pp].data[my*stride + x];

                    for( block_x = 0; block_x < block_width; block_x++ )
                    {
                        block_score += mask_p[0];
                        mask_p++;
                    }
                }

                if (pv->comb_check_complete)
                {
                    // Some other thread found coming before this one
                    return;
                }

                if( block_score >= ( threshold / 2 ) )
                {
                    pv->mask_box_x = x;
                    pv->mask_box_y = y;

                    pv->block_score[segment] = block_score;
                    if( block_score > threshold )
                    {
                        pv->comb_check_complete = 1;
                        return;
                    }
                }
            }
        }
    }
}

static void check_combing_mask( hb_filter_private_t * pv, int segment, int start, int stop )
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
    int block_score = 0;
    uint8_t * mask_p;
    int x, y, pp;

    for( pp = 0; pp < 1; pp++ )
    {
        int stride = pv->mask->plane[pp].stride;
        int width = pv->mask->plane[pp].width;

        for (y = start; y < (stop - block_height + 1); y = y + block_height)
        {
            for (x = 0; x < (width - block_width); x = x + block_width)
            {
                block_score = 0;

                for( block_y = 0; block_y < block_height; block_y++ )
                {
                    int mask_y = y + block_y;
                    mask_p = &pv->mask->plane[pp].data[mask_y * stride + x];

                    for( block_x = 0; block_x < block_width; block_x++ )
                    {
                        /* We only want to mark a pixel in a block as combed
                           if the adjacent pixels are as well. Got to
                           handle the sides separately.       */
                        if( (x + block_x) == 0 )
                        {
                            block_score += mask_p[0] & mask_p[1];
                        }
                        else if( (x + block_x) == (width -1) )
                        {
                            block_score += mask_p[-1] & mask_p[0];
                        }
                        else
                        {
                            block_score += mask_p[-1] & mask_p[0] & mask_p[1];
                        }

                        mask_p++;
                    }
                }

                if (pv->comb_check_complete)
                {
                    // Some other thread found coming before this one
                    return;
                }

                if( block_score >= ( threshold / 2 ) )
                {
                    pv->mask_box_x = x;
                    pv->mask_box_y = y;

                    pv->block_score[segment] = block_score;
                    if( block_score > threshold )
                    {
                        pv->comb_check_complete = 1;
                        return;
                    }
                }
            }
        }
    }
}

static void build_gamma_lut( hb_filter_private_t * pv )
{
    int i;
    for( i = 0; i < 256; i++ )
    {
        pv->gamma_lut[i] = pow( ( (float)i / (float)255 ), 2.2f );
    }
}

static void detect_gamma_combed_segment( hb_filter_private_t * pv, int segment_start, int segment_stop )
{
    /* A mish-mash of various comb detection tricks
       picked up from neuron2's Decomb plugin for
       AviSynth and tritical's IsCombedT and
       IsCombedTIVTC plugins.                       */

    /* Comb scoring algorithm */
    /* Motion threshold */
    float mthresh         = (float)pv->motion_threshold / (float)255;
    /* Spatial threshold */
    float athresh         = (float)pv->spatial_threshold / (float)255;
    float athresh6        = 6 *athresh;

    /* One pas for Y, one pass for U, one pass for V */
    int pp;
    for( pp = 0; pp < 1; pp++ )
    {
        int x, y;
        int stride  = pv->ref[0]->plane[pp].stride;
        int width   = pv->ref[0]->plane[pp].width;
        int height  = pv->ref[0]->plane[pp].height;

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
            int up_2    = -2 * stride ;
            int up_1    = -1 * stride;
            int down_1  =      stride;
            int down_2  =  2 * stride;

            /* We need to examine a column of 5 pixels
               in the prev, cur, and next frames.      */
            uint8_t * prev = &pv->ref[0]->plane[pp].data[y * stride];
            uint8_t * cur  = &pv->ref[1]->plane[pp].data[y * stride];
            uint8_t * next = &pv->ref[2]->plane[pp].data[y * stride];
            uint8_t * mask = &pv->mask->plane[pp].data[y * stride];

            memset(mask, 0, stride);

            for( x = 0; x < width; x++ )
            {
                float up_diff, down_diff;
                up_diff   = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[up_1]];
                down_diff = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[down_1]];

                if( ( up_diff >  athresh && down_diff >  athresh ) ||
                    ( up_diff < -athresh && down_diff < -athresh ) )
                {
                    /* The pixel above and below are different,
                       and they change in the same "direction" too.*/
                    int motion = 0;
                    if( mthresh > 0 )
                    {
                        /* Make sure there's sufficient motion between frame t-1 to frame t+1. */
                        if( fabs( pv->gamma_lut[prev[0]]      - pv->gamma_lut[cur[0]] ) > mthresh &&
                            fabs( pv->gamma_lut[cur[up_1]]    - pv->gamma_lut[next[up_1]]    ) > mthresh &&
                            fabs( pv->gamma_lut[cur[down_1]]  - pv->gamma_lut[next[down_1]]    ) > mthresh )
                                motion++;
                        if( fabs( pv->gamma_lut[next[0]]      - pv->gamma_lut[cur[0]] ) > mthresh &&
                            fabs( pv->gamma_lut[prev[up_1]]   - pv->gamma_lut[cur[up_1]] ) > mthresh &&
                            fabs( pv->gamma_lut[prev[down_1]] - pv->gamma_lut[cur[down_1]] ) > mthresh )
                                motion++;

                    }
                    else
                    {
                        /* User doesn't want to check for motion,
                           so move on to the spatial check.       */
                        motion = 1;
                    }

                    if( motion || ( pv->deinterlaced_frames==0 && pv->blended_frames==0 && pv->unfiltered_frames==0) )
                    {

                        /* Tritical's noise-resistant combing scorer.
                           The check is done on a bob+blur convolution. */
                        float combing = fabs( pv->gamma_lut[cur[up_2]]
                                         + ( 4 * pv->gamma_lut[cur[0]] )
                                         + pv->gamma_lut[cur[down_2]]
                                         - ( 3 * ( pv->gamma_lut[cur[up_1]]
                                                 + pv->gamma_lut[cur[down_1]] ) ) );
                        /* If the frame is sufficiently combed,
                           then mark it down on the mask as 1. */
                        if( combing > athresh6 )
                        {
                            mask[0] = 1;
                        }
                    }
                }

                cur++;
                prev++;
                next++;
                mask++;
            }
        }
    }
}


static void detect_combed_segment( hb_filter_private_t * pv, int segment_start, int segment_stop )
{
    /* A mish-mash of various comb detection tricks
       picked up from neuron2's Decomb plugin for
       AviSynth and tritical's IsCombedT and
       IsCombedTIVTC plugins.                       */


    /* Comb scoring algorithm */
    int spatial_metric  = pv->spatial_metric;
    /* Motion threshold */
    int mthresh         = pv->motion_threshold;
    /* Spatial threshold */
    int athresh         = pv->spatial_threshold;
    int athresh_squared = athresh * athresh;
    int athresh6        = 6 * athresh;

    /* One pas for Y, one pass for U, one pass for V */
    int pp;
    for( pp = 0; pp < 1; pp++ )
    {
        int x, y;
        int stride  = pv->ref[0]->plane[pp].stride;
        int width   = pv->ref[0]->plane[pp].width;
        int height  = pv->ref[0]->plane[pp].height;

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
            int up_2    = -2 * stride ;
            int up_1    = -1 * stride;
            int down_1  =      stride;
            int down_2  =  2 * stride;

            /* We need to examine a column of 5 pixels
               in the prev, cur, and next frames.      */
            uint8_t * prev = &pv->ref[0]->plane[pp].data[y * stride];
            uint8_t * cur  = &pv->ref[1]->plane[pp].data[y * stride];
            uint8_t * next = &pv->ref[2]->plane[pp].data[y * stride];
            uint8_t * mask = &pv->mask->plane[pp].data[y * stride];

            memset(mask, 0, stride);

            for( x = 0; x < width; x++ )
            {
                int up_diff = cur[0] - cur[up_1];
                int down_diff = cur[0] - cur[down_1];

                if( ( up_diff >  athresh && down_diff >  athresh ) ||
                    ( up_diff < -athresh && down_diff < -athresh ) )
                {
                    /* The pixel above and below are different,
                       and they change in the same "direction" too.*/
                    int motion = 0;
                    if( mthresh > 0 )
                    {
                        /* Make sure there's sufficient motion between frame t-1 to frame t+1. */
                        if( abs( prev[0] - cur[0] ) > mthresh &&
                            abs(  cur[up_1] - next[up_1]    ) > mthresh &&
                            abs(  cur[down_1] - next[down_1]    ) > mthresh )
                                motion++;
                        if( abs(     next[0] - cur[0] ) > mthresh &&
                            abs( prev[up_1] - cur[up_1] ) > mthresh &&
                            abs( prev[down_1] - cur[down_1] ) > mthresh )
                                motion++;
                    }
                    else
                    {
                        /* User doesn't want to check for motion,
                           so move on to the spatial check.       */
                        motion = 1;
                    }

                    if( motion || ( pv->deinterlaced_frames==0 && pv->blended_frames==0 && pv->unfiltered_frames==0) )
                    {
                           /* That means it's time for the spatial check.
                              We've got several options here.             */
                        if( spatial_metric == 0 )
                        {
                            /* Simple 32detect style comb detection */
                            if( ( abs( cur[0] - cur[down_2] ) < 10  ) &&
                                ( abs( cur[0] - cur[down_1] ) > 15 ) )
                            {
                                mask[0] = 1;
                            }
                        }
                        else if( spatial_metric == 1 )
                        {
                            /* This, for comparison, is what IsCombed uses.
                               It's better, but still noise senstive.      */
                               int combing = ( cur[up_1] - cur[0] ) *
                                             ( cur[down_1] - cur[0] );

                               if( combing > athresh_squared )
                               {
                                   mask[0] = 1;
                               }
                        }
                        else if( spatial_metric == 2 )
                        {
                            /* Tritical's noise-resistant combing scorer.
                               The check is done on a bob+blur convolution. */
                            int combing = abs( cur[up_2]
                                             + ( 4 * cur[0] )
                                             + cur[down_2]
                                             - ( 3 * ( cur[up_1]
                                                     + cur[down_1] ) ) );

                            /* If the frame is sufficiently combed,
                               then mark it down on the mask as 1. */
                            if( combing > athresh6 )
                            {
                                mask[0] = 1;
                            }
                        }
                    }
                }

                cur++;
                prev++;
                next++;
                mask++;
            }
        }
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

    hb_log("eedi2 thread started for plane %d", plane);

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


static void mask_dilate_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("mask dilate thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_dilate_taskset, segment );

        if (taskset_thread_stop(&pv->mask_dilate_taskset, segment))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        int count;
        int dilation_threshold = 4;

        for( pp = 0; pp < 1; pp++ )
        {
            int width = pv->mask_filtered->plane[pp].width;
            int height = pv->mask_filtered->plane[pp].height;
            int stride = pv->mask_filtered->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            if (segment_start == 0)
            {
                start = 1;
                p = 0;
                c = 1;
                n = 2;
            }
            else
            {
                start = segment_start;
                p = segment_start - 1;
                c = segment_start;
                n = segment_start + 1;
            }

            if (segment_stop == height)
            {
                stop = height -1;
            }
            else
            {
                stop = segment_stop;
            }

            uint8_t *curp = &pv->mask_filtered->plane[pp].data[p * stride + 1];
            uint8_t *cur  = &pv->mask_filtered->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask_filtered->plane[pp].data[n * stride + 1];
            uint8_t *dst = &pv->mask_temp->plane[pp].data[c * stride + 1];

            for( yy = start; yy < stop; yy++ )
            {
                for( xx = 1; xx < width - 1; xx++ )
                {
                    if (cur[xx])
                    {
                        dst[xx] = 1;
                        continue;
                    }

                    count = curp[xx-1] + curp[xx] + curp[xx+1] +
                            cur [xx-1] +            cur [xx+1] +
                            curn[xx-1] + curn[xx] + curn[xx+1];

                    dst[xx] = count >= dilation_threshold;
                }
                curp += stride;
                cur += stride;
                curn += stride;
                dst += stride;
            }
        }

        taskset_thread_complete( &pv->mask_dilate_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_dilate_taskset, segment );
}

static void mask_erode_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("mask erode thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_erode_taskset, segment );

        if( taskset_thread_stop( &pv->mask_erode_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        int count;
        int erosion_threshold = 2;

        for( pp = 0; pp < 1; pp++ )
        {
            int width = pv->mask_filtered->plane[pp].width;
            int height = pv->mask_filtered->plane[pp].height;
            int stride = pv->mask_filtered->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            if (segment_start == 0)
            {
                start = 1;
                p = 0;
                c = 1;
                n = 2;
            }
            else
            {
                start = segment_start;
                p = segment_start - 1;
                c = segment_start;
                n = segment_start + 1;
            }

            if (segment_stop == height)
            {
                stop = height -1;
            }
            else
            {
                stop = segment_stop;
            }

            uint8_t *curp = &pv->mask_temp->plane[pp].data[p * stride + 1];
            uint8_t *cur  = &pv->mask_temp->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask_temp->plane[pp].data[n * stride + 1];
            uint8_t *dst = &pv->mask_filtered->plane[pp].data[c * stride + 1];

            for( yy = start; yy < stop; yy++ )
            {
                for( xx = 1; xx < width - 1; xx++ )
                {
                    if( cur[xx] == 0 )
                    {
                        dst[xx] = 0;
                        continue;
                    }

                    count = curp[xx-1] + curp[xx] + curp[xx+1] +
                            cur [xx-1] +            cur [xx+1] +
                            curn[xx-1] + curn[xx] + curn[xx+1];

                    dst[xx] = count >= erosion_threshold;
                }
                curp += stride;
                cur += stride;
                curn += stride;
                dst += stride;
            }
        }

        taskset_thread_complete( &pv->mask_erode_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_erode_taskset, segment );
}

static void mask_filter_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("mask filter thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_filter_taskset, segment );

        if( taskset_thread_stop( &pv->mask_filter_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        for( pp = 0; pp < 1; pp++ )
        {
            int width = pv->mask->plane[pp].width;
            int height = pv->mask->plane[pp].height;
            int stride = pv->mask->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            if (segment_start == 0)
            {
                start = 1;
                p = 0;
                c = 1;
                n = 2;
            }
            else
            {
                start = segment_start;
                p = segment_start - 1;
                c = segment_start;
                n = segment_start + 1;
            }

            if (segment_stop == height)
            {
                stop = height - 1;
            }
            else
            {
                stop = segment_stop;
            }

            uint8_t *curp = &pv->mask->plane[pp].data[p * stride + 1];
            uint8_t *cur = &pv->mask->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask->plane[pp].data[n * stride + 1];
            uint8_t *dst = (pv->filter_mode == FILTER_CLASSIC ) ?
                &pv->mask_filtered->plane[pp].data[c * stride + 1] :
                &pv->mask_temp->plane[pp].data[c * stride + 1] ;

            for( yy = start; yy < stop; yy++ )
            {
                for( xx = 1; xx < width - 1; xx++ )
                {
                    int h_count, v_count;

                    h_count = cur[xx-1] & cur[xx] & cur[xx+1];
                    v_count = curp[xx] & cur[xx] & curn[xx];

                    if (pv->filter_mode == FILTER_CLASSIC)
                    {
                        dst[xx] = h_count;
                    }
                    else
                    {
                        dst[xx] = h_count & v_count;
                    }
                }
                curp += stride;
                cur += stride;
                curn += stride;
                dst += stride;
            }
        }

        taskset_thread_complete( &pv->mask_filter_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_filter_taskset, segment );
}

static void decomb_check_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("decomb check thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->decomb_check_taskset, segment );

        if( taskset_thread_stop( &pv->decomb_check_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        segment_start = thread_args->segment_start[0];
        segment_stop = segment_start + thread_args->segment_height[0];

        if( pv->mode & MODE_FILTER )
        {
            check_filtered_combing_mask(pv, segment, segment_start, segment_stop);
        }
        else
        {
            check_combing_mask(pv, segment, segment_start, segment_stop);
        }

        taskset_thread_complete( &pv->decomb_check_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->decomb_check_taskset, segment );
}

/*
 * comb detect this segment of all three planes in a single thread.
 */
static void decomb_filter_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("decomb filter thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->decomb_filter_taskset, segment );

        if( taskset_thread_stop( &pv->decomb_filter_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        /*
         * Process segment (for now just from luma)
         */
        int pp;
        for( pp = 0; pp < 1; pp++)
        {
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            if( pv->mode & MODE_GAMMA )
            {
                detect_gamma_combed_segment( pv, segment_start, segment_stop );
            }
            else
            {
                detect_combed_segment( pv, segment_start, segment_stop );
            }
        }

        taskset_thread_complete( &pv->decomb_filter_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->decomb_filter_taskset, segment );
}

static int comb_segmenter( hb_filter_private_t * pv )
{
    /*
     * Now that all data for decomb detection is ready for
     * our threads, fire them off and wait for their completion.
     */
    taskset_cycle( &pv->decomb_filter_taskset );

    if( pv->mode & MODE_FILTER )
    {
         taskset_cycle( &pv->mask_filter_taskset );
        if( pv->filter_mode == FILTER_ERODE_DILATE )
        {
            taskset_cycle( &pv->mask_erode_taskset );
            taskset_cycle( &pv->mask_dilate_taskset );
            taskset_cycle( &pv->mask_erode_taskset );
        }
        //return check_filtered_combing_mask( pv );
    }
    else
    {
        //return check_combing_mask( pv );
    }
    reset_combing_results(pv);
    taskset_cycle( &pv->decomb_check_taskset );
    return check_combing_results(pv);
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
            if( ( pv->mode & MODE_CUBIC ) && !vertical_edge )\
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
    int eedi2_mode = ( pv->mode & MODE_EEDI2 );

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
            if( ( pv->mode & MODE_CUBIC ) && !vertical_edge)
            {
                spatial_pred = cubic_interpolate_pixel( cur[-3*stride], cur[-stride], cur[+stride], cur[3*stride] );
            }
            else
            {
                spatial_pred = (c+e)>>1;
            }

            // YADIF_CHECK requires a margin to avoid invalid memory access.
            // In MODE_CUBIC, margin needed is 2 + ABS(param).
            // Else, the margin needed is 1 + ABS(param).
            int margin = 2;
            if (pv->mode & MODE_CUBIC)
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

    hb_log("yadif thread started for segment %d", segment);

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
        int parity, tff, is_combed;

        is_combed = pv->yadif_arguments[segment].is_combed;
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

            if( is_combed == 2 )
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
            else if (pv->mode == MODE_CUBIC && is_combed)
            {
                for( yy = start; yy < segment_stop; yy += 2 )
                {
                    /* Just apply vertical cubic interpolation */
                    cubic_interpolate_line(dst2, cur, width, height, stride, yy);
                    dst2 += stride * 2;
                    cur += stride * 2;
                }
            }
            else if ((pv->mode & MODE_YADIF) && is_combed == 1)
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
    int is_combed;

    if (!pv->skip_comb_check)
    {
        is_combed = pv->spatial_metric >= 0 ? comb_segmenter( pv ) : 1;
    }
    else
    {
        is_combed = pv->is_combed;
    }

    /* The comb detector suggests three different values:
       0: Don't comb this frame.
       1: Deinterlace this frame.
       2: Blend this frame.
       Since that might conflict with the filter's mode,
       it may be necesary to adjust this value.          */
    if( is_combed == 1 && (pv->mode == MODE_BLEND) )
    {
        /* All combed frames are getting blended */
        is_combed = 2;
    }
    else if( is_combed == 2 && !( pv->mode & MODE_BLEND ) )
    {
        /* Blending is disabled, so force interpolation of these frames. */
        is_combed = 1;
    }
    if( is_combed == 1 &&
        ( pv->mode & MODE_BLEND ) &&
        !( pv->mode & ( MODE_YADIF | MODE_EEDI2 | MODE_CUBIC ) ) )
    {
        /* Deinterlacers are disabled, blending isn't, so blend these frames. */
        is_combed = 2;
    }
    else if( is_combed &&
             !( pv->mode & ( MODE_BLEND | MODE_YADIF | MODE_EEDI2 | MODE_CUBIC | MODE_MASK ) ) )
    {
        /* No deinterlacer or mask chosen, pass the frame through. */
        is_combed = 0;
    }

    if( is_combed == 1 )
    {
        pv->deinterlaced_frames++;
    }
    else if( is_combed == 2 )
    {
        pv->blended_frames++;
    }
    else
    {
        pv->unfiltered_frames++;
    }

    if( is_combed == 1 && ( pv->mode & MODE_EEDI2 ) )
    {
        /* Generate an EEDI2 interpolation */
        eedi2_planer( pv );
    }

    pv->is_combed = is_combed;
    if( is_combed )
    {
        if( ( pv->mode & MODE_EEDI2 ) && !( pv->mode & MODE_YADIF ) && is_combed == 1 )
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
                pv->yadif_arguments[segment].is_combed = is_combed;
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
        pv->yadif_arguments[0].is_combed = is_combed; // 0
        hb_buffer_copy(dst, pv->ref[1]);
    }
}

static int hb_decomb_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    build_gamma_lut( pv );

    pv->deinterlaced_frames = 0;
    pv->blended_frames = 0;
    pv->unfiltered_frames = 0;

    pv->yadif_ready    = 0;

    pv->mode     = MODE_YADIF | MODE_BLEND | MODE_CUBIC |
                   MODE_GAMMA | MODE_FILTER;
    pv->filter_mode = FILTER_ERODE_DILATE;
    pv->spatial_metric = 2;
    pv->motion_threshold = 3;
    pv->spatial_threshold = 3;
    pv->block_threshold = 40;
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

    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
                &pv->mode,
                &pv->spatial_metric,
                &pv->motion_threshold,
                &pv->spatial_threshold,
                &pv->filter_mode,
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
                &pv->post_processing,
                &pv->parity );
    }

    pv->cpu_count = hb_get_cpu_count();

    // Make segment sizes an even number of lines
    int height = hb_image_height(init->pix_fmt, init->geometry.height, 0);
    // Each segment must begin on the even "parity" row.
    // I.e. each segment of each plane must begin on an even row.
    pv->segment_height[0] = (height / pv->cpu_count) & ~3;
    pv->segment_height[1] = hb_image_height(init->pix_fmt, pv->segment_height[0], 1);
    pv->segment_height[2] = hb_image_height(init->pix_fmt, pv->segment_height[0], 2);

    /* Allocate buffers to store comb masks. */
    pv->mask = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    pv->mask_filtered = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    pv->mask_temp = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    memset(pv->mask->data, 0, pv->mask->size);
    memset(pv->mask_filtered->data, 0, pv->mask_filtered->size);
    memset(pv->mask_temp->data, 0, pv->mask_temp->size);

    int ii;
    if( pv->mode & MODE_EEDI2 )
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

    /*
     * Create comb detection taskset.
     */
    if( taskset_init( &pv->decomb_filter_taskset, pv->cpu_count,
                      sizeof( decomb_thread_arg_t ) ) == 0 )
    {
        hb_error( "decomb could not initialize taskset" );
    }

    decomb_thread_arg_t *decomb_prev_thread_args = NULL;
    for( ii = 0; ii < pv->cpu_count; ii++ )
    {
        decomb_thread_arg_t *thread_args;

        thread_args = taskset_thread_args( &pv->decomb_filter_taskset, ii );
        thread_args->pv = pv;
        thread_args->segment = ii;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            if (decomb_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    decomb_prev_thread_args->segment_start[pp] +
                    decomb_prev_thread_args->segment_height[pp];
            }
            if( ii == pv->cpu_count - 1 )
            {
                /*
                 * Final segment
                 */
                thread_args->segment_height[pp] =
                    hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                    thread_args->segment_start[pp];
            } else {
                thread_args->segment_height[pp] = pv->segment_height[pp];
            }
        }

        if( taskset_thread_spawn( &pv->decomb_filter_taskset, ii,
                                 "decomb_filter_segment",
                                 decomb_filter_thread,
                                 HB_NORMAL_PRIORITY ) == 0 )
        {
            hb_error( "decomb could not spawn thread" );
        }

        decomb_prev_thread_args = thread_args;
    }

    pv->comb_check_nthreads = init->geometry.height / pv->block_height;

    if (pv->comb_check_nthreads > pv->cpu_count)
        pv->comb_check_nthreads = pv->cpu_count;

    pv->block_score = calloc(pv->comb_check_nthreads, sizeof(int));

    /*
     * Create comb check taskset.
     */
    if( taskset_init( &pv->decomb_check_taskset, pv->comb_check_nthreads,
                      sizeof( decomb_thread_arg_t ) ) == 0 )
    {
        hb_error( "decomb check could not initialize taskset" );
    }

    decomb_prev_thread_args = NULL;
    for( ii = 0; ii < pv->comb_check_nthreads; ii++ )
    {
        decomb_thread_arg_t *thread_args, *decomb_prev_thread_args = NULL;

        thread_args = taskset_thread_args( &pv->decomb_check_taskset, ii );
        thread_args->pv = pv;
        thread_args->segment = ii;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            if (decomb_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    decomb_prev_thread_args->segment_start[pp] +
                    decomb_prev_thread_args->segment_height[pp];
            }

            // Make segment hight a multiple of block_height
            int h = hb_image_height(init->pix_fmt, init->geometry.height, pp) / pv->comb_check_nthreads;
            h = h / pv->block_height * pv->block_height;
            if (h == 0)
                h = pv->block_height;

            if (ii == pv->comb_check_nthreads - 1)
            {
                /*
                 * Final segment
                 */
                thread_args->segment_height[pp] =
                    hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                    thread_args->segment_start[pp];
            } else {
                thread_args->segment_height[pp] = h;
            }
        }

        if( taskset_thread_spawn( &pv->decomb_check_taskset, ii,
                                 "decomb_check_segment",
                                 decomb_check_thread,
                                 HB_NORMAL_PRIORITY ) == 0 )
        {
            hb_error( "decomb check could not spawn thread" );
        }

        decomb_prev_thread_args = thread_args;
    }

    if( pv->mode & MODE_FILTER )
    {
        if( taskset_init( &pv->mask_filter_taskset, pv->cpu_count,
                          sizeof( decomb_thread_arg_t ) ) == 0 )
        {
            hb_error( "maske filter could not initialize taskset" );
        }

        decomb_prev_thread_args = NULL;
        for( ii = 0; ii < pv->cpu_count; ii++ )
        {
            decomb_thread_arg_t *thread_args;

            thread_args = taskset_thread_args( &pv->mask_filter_taskset, ii );
            thread_args->pv = pv;
            thread_args->segment = ii;

            int pp;
            for (pp = 0; pp < 3; pp++)
            {
                if (decomb_prev_thread_args != NULL)
                {
                    thread_args->segment_start[pp] =
                        decomb_prev_thread_args->segment_start[pp] +
                        decomb_prev_thread_args->segment_height[pp];
                }

                if( ii == pv->cpu_count - 1 )
                {
                    /*
                     * Final segment
                     */
                    thread_args->segment_height[pp] =
                        hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                        thread_args->segment_start[pp];
                } else {
                    thread_args->segment_height[pp] = pv->segment_height[pp];
                }
            }

            if( taskset_thread_spawn( &pv->mask_filter_taskset, ii,
                                     "mask_filter_segment",
                                     mask_filter_thread,
                                     HB_NORMAL_PRIORITY ) == 0 )
            {
                hb_error( "mask filter could not spawn thread" );
            }

            decomb_prev_thread_args = thread_args;
        }

        if( pv->filter_mode == FILTER_ERODE_DILATE )
        {
            if( taskset_init( &pv->mask_erode_taskset, pv->cpu_count,
                              sizeof( decomb_thread_arg_t ) ) == 0 )
            {
                hb_error( "mask erode could not initialize taskset" );
            }

            decomb_prev_thread_args = NULL;
            for( ii = 0; ii < pv->cpu_count; ii++ )
            {
                decomb_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_erode_taskset, ii );
                thread_args->pv = pv;
                thread_args->segment = ii;

                int pp;
                for (pp = 0; pp < 3; pp++)
                {
                    if (decomb_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            decomb_prev_thread_args->segment_start[pp] +
                            decomb_prev_thread_args->segment_height[pp];
                    }

                    if( ii == pv->cpu_count - 1 )
                    {
                        /*
                         * Final segment
                         */
                        thread_args->segment_height[pp] =
                            hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                            thread_args->segment_start[pp];
                    } else {
                        thread_args->segment_height[pp] = pv->segment_height[pp];
                    }
                }

                if( taskset_thread_spawn( &pv->mask_erode_taskset, ii,
                                         "mask_erode_segment",
                                         mask_erode_thread,
                                         HB_NORMAL_PRIORITY ) == 0 )
                {
                    hb_error( "mask erode could not spawn thread" );
                }

                decomb_prev_thread_args = thread_args;
            }

            if( taskset_init( &pv->mask_dilate_taskset, pv->cpu_count,
                              sizeof( decomb_thread_arg_t ) ) == 0 )
            {
                hb_error( "mask dilate could not initialize taskset" );
            }

            decomb_prev_thread_args = NULL;
            for( ii = 0; ii < pv->cpu_count; ii++ )
            {
                decomb_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_dilate_taskset, ii );
                thread_args->pv = pv;
                thread_args->segment = ii;

                int pp;
                for (pp = 0; pp < 3; pp++)
                {
                    if (decomb_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            decomb_prev_thread_args->segment_start[pp] +
                            decomb_prev_thread_args->segment_height[pp];
                    }

                    if( ii == pv->cpu_count - 1 )
                    {
                        /*
                         * Final segment
                         */
                        thread_args->segment_height[pp] =
                            hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                            thread_args->segment_start[pp];
                    } else {
                        thread_args->segment_height[pp] = pv->segment_height[pp];
                    }
                }

                if( taskset_thread_spawn( &pv->mask_dilate_taskset, ii,
                                         "mask_dilate_segment",
                                         mask_dilate_thread,
                                         HB_NORMAL_PRIORITY ) == 0 )
                {
                    hb_error( "mask dilate could not spawn thread" );
                }

                decomb_prev_thread_args = thread_args;
            }
        }
    }

    if( pv->mode & MODE_EEDI2 )
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
                hb_log("EEDI2: failed to malloc derivative arrays");
            else
                hb_log("EEDI2: successfully mallloced derivative arrays");
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

    hb_log("decomb: deinterlaced %i | blended %i | unfiltered %i | total %i", pv->deinterlaced_frames, pv->blended_frames, pv->unfiltered_frames, pv->deinterlaced_frames + pv->blended_frames + pv->unfiltered_frames);

    taskset_fini( &pv->yadif_taskset );
    taskset_fini( &pv->decomb_filter_taskset );
    taskset_fini( &pv->decomb_check_taskset );

    if( pv->mode & MODE_FILTER )
    {
        taskset_fini( &pv->mask_filter_taskset );
        if( pv->filter_mode == FILTER_ERODE_DILATE )
        {
            taskset_fini( &pv->mask_erode_taskset );
            taskset_fini( &pv->mask_dilate_taskset );
        }
    }

    if( pv->mode & MODE_EEDI2 )
    {
        taskset_fini( &pv->eedi2_taskset );
    }


    /* Cleanup reference buffers. */
    int ii;
    for (ii = 0; ii < 3; ii++)
    {
        hb_buffer_close(&pv->ref[ii]);
    }

    /* Cleanup combing masks. */
    hb_buffer_close(&pv->mask);
    hb_buffer_close(&pv->mask_filtered);
    hb_buffer_close(&pv->mask_temp);

    if( pv->mode & MODE_EEDI2 )
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

    if( pv->post_processing > 1  && ( pv->mode & MODE_EEDI2 ) )
    {
        if (pv->cx2) eedi2_aligned_free(pv->cx2);
        if (pv->cy2) eedi2_aligned_free(pv->cy2);
        if (pv->cxy) eedi2_aligned_free(pv->cxy);
        if (pv->tmpc) eedi2_aligned_free(pv->tmpc);
    }

    free(pv->block_score);

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

static int hb_decomb_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    /* Store current frame in yadif cache */
    *buf_in = NULL;
    fill_stride(in);
    store_ref(pv, in);

    // yadif requires 3 buffers, prev, cur, and next.  For the first
    // frame, there can be no prev, so we duplicate the first frame.
    if (!pv->yadif_ready)
    {
        // If yadif is not ready, store another ref and return HB_FILTER_DELAY
        store_ref(pv, hb_buffer_dup(in));
        pv->yadif_ready = 1;
        // Wait for next
        return HB_FILTER_DELAY;
    }

    /* Determine if top-field first layout */
    int tff;
    if( pv->parity < 0 )
    {
        tff = !!(in->s.flags & PIC_FLAG_TOP_FIELD_FIRST);
    }
    else
    {
        tff = (pv->parity & 1) ^ 1;
    }

    /* deinterlace both fields if bob */
    int frame, num_frames = 1;
    if (pv->mode & MODE_BOB)
    {
        num_frames = 2;
    }

    // Will need up to 2 buffers simultaneously
    int idx = 0;
    hb_buffer_t * o_buf[2] = {NULL,};

    /* Perform yadif filtering */
    for( frame = 0; frame < num_frames; frame++ )
    {
        int parity = frame ^ tff ^ 1;

        /* Skip the second run if the frame is uncombed */
        if (frame && pv->is_combed == 0)
        {
            break;
        }

        // tff for eedi2
        pv->tff = !parity;

        if (o_buf[idx] == NULL)
        {
            o_buf[idx] = hb_video_buffer_init(in->f.width, in->f.height);
        }

        if (frame)
            pv->skip_comb_check = 1;
        else
            pv->skip_comb_check = 0;

        yadif_filter(pv, o_buf[idx], parity, tff);

        // If bob, add all frames to output
        // else, if not combed, add frame to output
        // else if final iteration, add frame to output
        if ((pv->mode & MODE_BOB) ||
            pv->is_combed == 0 ||
            frame == num_frames - 1)
        {
            /* Copy buffered settings to output buffer settings */
            o_buf[idx]->s = pv->ref[1]->s;

            o_buf[idx]->next = NULL;
            hb_buffer_list_append(&list, o_buf[idx]);

            // Indicate that buffer was consumed
            o_buf[idx] = NULL;

            idx ^= 1;

            if ((pv->mode & MODE_MASK) && pv->spatial_metric >= 0 )
            {
                if (pv->mode == MODE_MASK ||
                    ((pv->mode & MODE_MASK) && (pv->mode & MODE_FILTER)) ||
                    ((pv->mode & MODE_MASK) && (pv->mode & MODE_GAMMA)) ||
                    pv->is_combed)
                {
                    apply_mask(pv, hb_buffer_list_tail(&list));
                }
            }
        }
    }
    hb_buffer_close(&o_buf[0]);
    hb_buffer_close(&o_buf[1]);

    /* if this frame was deinterlaced and bob mode is engaged, halve
       the duration of the saved timestamps. */
    if ((pv->mode & MODE_BOB) && pv->is_combed)
    {
        hb_buffer_t *first  = hb_buffer_list_head(&list);
        hb_buffer_t *second = hb_buffer_list_tail(&list);
        first->s.stop -= (first->s.stop - first->s.start) / 2LL;
        second->s.start = first->s.stop;
        second->s.new_chap = 0;
    }

    *buf_out = hb_buffer_list_clear(&list);
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

