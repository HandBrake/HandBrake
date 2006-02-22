/* $Id: decsub.c,v 1.12 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t * job;

    uint8_t    buf[0xFFFF];
    int        size_sub;
    int        size_got;
    int        size_rle;
    int64_t    pts;
    int64_t    pts_start;
    int64_t    pts_stop;
    int        x;
    int        y;
    int        width;
    int        height;

    int        offsets[2];
    uint8_t    lum[4];
    uint8_t    alpha[4];
};


/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void          Close( hb_work_object_t ** _w );
static int           Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out );
static hb_buffer_t * Decode( hb_work_object_t * w );
static void          ParseControls( hb_work_object_t * w );
static hb_buffer_t * CropSubtitle( hb_work_object_t * w,
                                   uint8_t * raw );

/***********************************************************************
 * hb_work_decsub_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_decsub_init( hb_job_t * job )
{
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "Subtitle decoder" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->pts   = -1;

    return w;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;
    free( w->name );
    free( w );
    *_w = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;

    int size_sub, size_rle;

    size_sub = ( in->data[0] << 8 ) | in->data[1];
    size_rle = ( in->data[2] << 8 ) | in->data[3];

    if( !w->size_sub )
    {
        /* We are looking for the start of a new subtitle */
        if( size_sub && size_rle && size_sub > size_rle &&
            in->size <= size_sub )
        {
            /* Looks all right so far */
            w->size_sub = size_sub;
            w->size_rle = size_rle;

            memcpy( w->buf, in->data, in->size );
            w->size_got = in->size;
            w->pts      = in->start;
        }
    }
    else
    {
        /* We are waiting for the end of the current subtitle */
        if( in->size <= w->size_sub - w->size_got )
        {
            memcpy( w->buf + w->size_got, in->data, in->size );
            w->size_got += in->size;
            if( in->start >= 0 )
            {
                w->pts = in->start;
            }
        }
    }

    *buf_out = NULL;

    if( w->size_sub && w->size_sub == w->size_got )
    {
        /* We got a complete subtitle, decode it */
        *buf_out = Decode( w );

        /* Wait for the next one */
        w->size_sub = 0;
        w->size_got = 0;
        w->size_rle = 0;
        w->pts      = -1;
    }

    return HB_WORK_OK;
}

static hb_buffer_t * Decode( hb_work_object_t * w )
{
    int code, line, col;
    int offsets[2];
    int * offset;
    hb_buffer_t * buf;
    uint8_t * buf_raw = NULL;

    /* Get infos about the subtitle */
    ParseControls( w );

    /* Do the actual decoding now */
    buf_raw = malloc( w->width * w->height * 2 );

#define GET_NEXT_NIBBLE code = ( code << 4 ) | ( ( ( *offset & 1 ) ? \
( w->buf[((*offset)>>1)] & 0xF ) : ( w->buf[((*offset)>>1)] >> 4 ) ) ); \
(*offset)++
    
    offsets[0] = w->offsets[0] * 2;
    offsets[1] = w->offsets[1] * 2;

    for( line = 0; line < w->height; line++ )
    {
        /* Select even or odd field */
        offset = ( line & 1 ) ? &offsets[1] : &offsets[0];

        for( col = 0; col < w->width; col += code >> 2 )
        {
            uint8_t * lum, * alpha;

            code = 0;
            GET_NEXT_NIBBLE;
            if( code < 0x4 )
            {
                GET_NEXT_NIBBLE;
                if( code < 0x10 )
                {
                    GET_NEXT_NIBBLE;
                    if( code < 0x40 )
                    {
                        GET_NEXT_NIBBLE;
                        if( code < 0x100 )
                        {
                            /* End of line */
                            code |= ( w->width - col ) << 2;
                        }
                    }
                }
            }

            lum   = buf_raw;
            alpha = lum + w->width * w->height;
            memset( lum + line * w->width + col,
                    w->lum[code & 3], code >> 2 );
            memset( alpha + line * w->width + col,
                    w->alpha[code & 3], code >> 2 );
        }

        /* Byte-align */
        if( *offset & 1 )
        {
            (*offset)++;
        }
    }

    /* Crop subtitle (remove transparent borders) */
    buf = CropSubtitle( w, buf_raw );

    free( buf_raw );

    return buf;
}

/***********************************************************************
 * ParseControls
 ***********************************************************************
 * Get the start and end dates (relative to the PTS from the PES
 * header), the width and height of the subpicture and the colors and
 * alphas used in it
 **********************************************************************/
static void ParseControls( hb_work_object_t * w )
{
    hb_job_t * job = w->job;
    hb_title_t * title = job->title;

    int i;
    int command;
    int date, next;

    w->pts_start = 0;
    w->pts_stop  = 0;

    for( i = w->size_rle; ; )
    {
        date = ( w->buf[i] << 8 ) | w->buf[i+1]; i += 2;
        next = ( w->buf[i] << 8 ) | w->buf[i+1]; i += 2;

        for( ;; )
        {
            command = w->buf[i++];

            if( command == 0xFF )
            {
                break;
            }

            switch( command )
            {
                case 0x00:
                    break;

                case 0x01:
                    w->pts_start = w->pts + date * 900;
                    break;

                case 0x02:
                    w->pts_stop = w->pts + date * 900;
                    break;

                case 0x03:
                {
                    int colors[4];
                    int j;

                    colors[0] = (w->buf[i+0]>>4)&0x0f;
                    colors[1] = (w->buf[i+0])&0x0f;
                    colors[2] = (w->buf[i+1]>>4)&0x0f;
                    colors[3] = (w->buf[i+1])&0x0f;

                    for( j = 0; j < 4; j++ )
                    {
                        uint32_t color = title->palette[colors[j]];
                        w->lum[3-j] = (color>>16) & 0xff;
                    }
                    i += 2;
                    break;
                }
                case 0x04:
                {
                    w->alpha[3] = (w->buf[i+0]>>4)&0x0f;
                    w->alpha[2] = (w->buf[i+0])&0x0f;
                    w->alpha[1] = (w->buf[i+1]>>4)&0x0f;
                    w->alpha[0] = (w->buf[i+1])&0x0f;
                    i += 2;
                    break;
                }
                case 0x05:
                {
                    w->x     = (w->buf[i+0]<<4) | ((w->buf[i+1]>>4)&0x0f);
                    w->width = (((w->buf[i+1]&0x0f)<<8)| w->buf[i+2]) - w->x + 1;
                    w->y     = (w->buf[i+3]<<4)| ((w->buf[i+4]>>4)&0x0f);
                    w->height = (((w->buf[i+4]&0x0f)<<8)| w->buf[i+5]) - w->y + 1;
                    i += 6;
                    break;
                }
                case 0x06:
                {
                    w->offsets[0] = ( w->buf[i] << 8 ) | w->buf[i+1]; i += 2;
                    w->offsets[1] = ( w->buf[i] << 8 ) | w->buf[i+1]; i += 2;
                    break;
                }
            }
        }

        if( i > next )
        {
            break;
        }
        i = next;
    }

    if( !w->pts_stop )
    {
        /* Show it for 3 seconds */
        w->pts_stop = w->pts_start + 3 * 90000;
    }
}

/***********************************************************************
 * CropSubtitle
 ***********************************************************************
 * Given a raw decoded subtitle, detects transparent borders and
 * returns a cropped subtitle in a hb_buffer_t ready to be used by
 * the renderer, or NULL if the subtitle was completely transparent
 **********************************************************************/
static int LineIsTransparent( hb_work_object_t * w, uint8_t * p )
{
    int i;
    for( i = 0; i < w->width; i++ )
    {
        if( p[i] )
        {
            return 0;
        }
    }
    return 1;
}
static int ColumnIsTransparent( hb_work_object_t * w, uint8_t * p )
{
    int i;
    for( i = 0; i < w->height; i++ )
    {
        if( p[i*w->width] )
        {
            return 0;
        }
    }
    return 1;
}
static hb_buffer_t * CropSubtitle( hb_work_object_t * w, uint8_t * raw )
{
    int i;
    int crop[4] = { -1,-1,-1,-1 };
    uint8_t * alpha;
    int realwidth, realheight;
    hb_buffer_t * buf;
    uint8_t * lum_in, * lum_out, * alpha_in, * alpha_out;

    alpha = raw + w->width * w->height;

    /* Top */
    for( i = 0; i < w->height; i++ )
    {
        if( !LineIsTransparent( w, &alpha[i*w->width] ) )
        {
            crop[0] = i;
            break;
        }
    }

    if( crop[0] < 0 )
    {
        /* Empty subtitle */
        return NULL;
    }

    /* Bottom */
    for( i = w->height - 1; i >= 0; i-- )
    {
        if( !LineIsTransparent( w, &alpha[i*w->width] ) )
        {
            crop[1] = i;
            break;
        }
    }

    /* Left */
    for( i = 0; i < w->width; i++ )
    {
        if( !ColumnIsTransparent( w, &alpha[i] ) )
        {
            crop[2] = i;
            break;
        }
    }

    /* Right */
    for( i = w->width - 1; i >= 0; i-- )
    {
        if( !ColumnIsTransparent( w, &alpha[i] ) )
        {
            crop[3] = i;
            break;
        }
    }

    realwidth  = crop[3] - crop[2] + 1;
    realheight = crop[1] - crop[0] + 1;

    buf         = hb_buffer_init( realwidth * realheight * 2 );
    buf->start  = w->pts_start;
    buf->stop   = w->pts_stop;
    buf->x      = w->x + crop[2];
    buf->y      = w->y + crop[0];
    buf->width  = realwidth;
    buf->height = realheight;

    lum_in    = raw + crop[0] * w->width + crop[2];
    alpha_in  = lum_in + w->width * w->height;
    lum_out   = buf->data;
    alpha_out = lum_out + realwidth * realheight;

    for( i = 0; i < realheight; i++ )
    {
        memcpy( lum_out, lum_in, realwidth );
        memcpy( alpha_out, alpha_in, realwidth );
        lum_in    += w->width;
        alpha_in  += w->width;
        lum_out   += realwidth;
        alpha_out += realwidth;
    }

    return buf;
}
