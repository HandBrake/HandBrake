/* $Id: decsub.c,v 1.12 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

struct hb_work_private_s
{
    hb_job_t * job;

    uint8_t    buf[0xFFFF];
    int        size_sub;
    int        size_got;
    int        size_rle;
    int64_t    pts;
    int64_t    pts_start;
    int64_t    pts_stop;
    int        pts_forced;
    int        x;
    int        y;
    int        width;
    int        height;
    int        stream_id;

    int        offsets[2];
    uint8_t    lum[4];
    uint8_t    chromaU[4];
    uint8_t    chromaV[4];
    uint8_t    alpha[4];
};

static hb_buffer_t * Decode( hb_work_object_t * );

int decsubInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;
    pv->pts = -1;

    return 0;
}

int decsubWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    int size_sub, size_rle;

    pv->stream_id = in->id;

    size_sub = ( in->data[0] << 8 ) | in->data[1];
    size_rle = ( in->data[2] << 8 ) | in->data[3];

    if( !pv->size_sub )
    {
        /* We are looking for the start of a new subtitle */
        if( size_sub && size_rle && size_sub > size_rle &&
            in->size <= size_sub )
        {
            /* Looks all right so far */
            pv->size_sub = size_sub;
            pv->size_rle = size_rle;

            memcpy( pv->buf, in->data, in->size );
            pv->size_got = in->size;
            pv->pts      = in->start;
        }
    }
    else
    {
        /* We are waiting for the end of the current subtitle */
        if( in->size <= pv->size_sub - pv->size_got )
        {
            memcpy( pv->buf + pv->size_got, in->data, in->size );
            pv->size_got += in->size;
            if( in->start >= 0 )
            {
                pv->pts = in->start;
            }
        }
    }

    *buf_out = NULL;

    if( pv->size_sub && pv->size_sub == pv->size_got )
    {
        /* We got a complete subtitle, decode it */
        *buf_out = Decode( w );

        if( buf_out && *buf_out )
        {
            (*buf_out)->sequence = in->sequence;
        }

        /* Wait for the next one */
        pv->size_sub = 0;
        pv->size_got = 0;
        pv->size_rle = 0;
        pv->pts      = -1;
    }

    return HB_WORK_OK;
}

void decsubClose( hb_work_object_t * w )
{
    free( w->private_data );
}

hb_work_object_t hb_decsub =
{
    WORK_DECSUB,
    "Subtitle decoder",
    decsubInit,
    decsubWork,
    decsubClose
};


/***********************************************************************
 * ParseControls
 ***********************************************************************
 * Get the start and end dates (relative to the PTS from the PES
 * header), the width and height of the subpicture and the colors and
 * alphas used in it
 **********************************************************************/
static void ParseControls( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    hb_title_t * title = job->title;
    hb_subtitle_t * subtitle;

    int i, n;
    int command;
    int date, next;

    pv->pts_start = 0;
    pv->pts_stop  = 0;
    pv->pts_forced  = 0;

    pv->alpha[3] = 0;
    pv->alpha[2] = 0;
    pv->alpha[1] = 0;
    pv->alpha[0] = 0;

    for( i = pv->size_rle; ; )
    {
        date = ( pv->buf[i] << 8 ) | pv->buf[i+1]; i += 2;
        next = ( pv->buf[i] << 8 ) | pv->buf[i+1]; i += 2;

        for( ;; )
        {
            command = pv->buf[i++];

            /*
             * There are eight commands available for
             * Sub-Pictures. The first SP_DCSQ should contain, as a
             * minimum, SET_COLOR, SET_CONTR, SET_DAREA, and
             * SET_DSPXA
             */

            if( command == 0xFF ) // 0xFF - CMD_END - ends one SP_DCSQ
            {
                break;
            }

            switch( command )
            {
                case 0x00: // 0x00 - FSTA_DSP - Forced Start Display, no arguments
                    pv->pts_start = pv->pts + date * 900;
                    pv->pts_forced = 1;

                    /*
                     * If we are doing a subtitle scan then note down
                     */
                    if( job->indepth_scan )
                    {
                        for( n=0; n < hb_list_count(title->list_subtitle); n++ )
                        {
                            subtitle = hb_list_item( title->list_subtitle, n);
                            if( pv->stream_id == subtitle->id ) {
                                /*
                                 * A hit, count it.
                                 */
                                subtitle->forced_hits++;
                            }
                        }
                    }
                    break;

                case 0x01: // 0x01 - STA_DSP - Start Display, no arguments
                    pv->pts_start = pv->pts + date * 900;
                    pv->pts_forced  = 0;
                    break;

                case 0x02: // 0x02 - STP_DSP - Stop Display, no arguments
                    if(!pv->pts_stop)
                        pv->pts_stop = pv->pts + date * 900;
                    break;

                case 0x03: // 0x03 - SET_COLOR - Set Colour indices
                {
                    /*
                     * SET_COLOR - provides four indices into the CLUT
                     * for the current PGC to associate with the four
                     * pixel values
                     */
                    int colors[4];
                    int j;

                    colors[0] = (pv->buf[i+0]>>4)&0x0f;
                    colors[1] = (pv->buf[i+0])&0x0f;
                    colors[2] = (pv->buf[i+1]>>4)&0x0f;
                    colors[3] = (pv->buf[i+1])&0x0f;

                    for( j = 0; j < 4; j++ )
                    {
                        /*
                         * Not sure what is happening here, in theory
                         * the palette is in YCbCr. And we want YUV.
                         *
                         * However it looks more like YCrCb (according
                         * to pgcedit). And the scalers for YCrCb don't
                         * work, but I get the right colours by doing
                         * no conversion.
                         */
                        uint32_t color = title->palette[colors[j]];
                        uint8_t Cr, Cb, y;
                        y = (color>>16) & 0xff;
                        Cr = (color>>8) & 0xff;
                        Cb = (color) & 0xff;
                        pv->lum[3-j] = y;
                        pv->chromaU[3-j] = Cb;
                        pv->chromaV[3-j] = Cr;
                        /* hb_log("color[%d] y = %d, u = %d, v = %d",
                               3-j,
                               pv->lum[3-j],
                               pv->chromaU[3-j],
                               pv->chromaV[3-j]);
                        */
                    }
                    i += 2;
                    break;
                }
                case 0x04: // 0x04 - SET_CONTR - Set Contrast
                {
                    /*
                     * SET_CONTR - directly provides the four contrast
                     * (alpha blend) values to associate with the four
                     * pixel values
                     */
                    uint8_t    alpha[4];

                    alpha[3] = (pv->buf[i+0]>>4)&0x0f;
                    alpha[2] = (pv->buf[i+0])&0x0f;
                    alpha[1] = (pv->buf[i+1]>>4)&0x0f;
                    alpha[0] = (pv->buf[i+1])&0x0f;


                    int lastAlpha = pv->alpha[3] + pv->alpha[2] + pv->alpha[1] + pv->alpha[0];
                    int currAlpha = alpha[3] + alpha[2] + alpha[1] + alpha[0];

                    // fading-in, save the highest alpha value
                    if( currAlpha > lastAlpha )
                    {
                        pv->alpha[3] = alpha[3];
                        pv->alpha[2] = alpha[2];
                        pv->alpha[1] = alpha[1];
                        pv->alpha[0] = alpha[0];
                    }

                    // fading-out
                    if( currAlpha < lastAlpha && !pv->pts_stop )
                    {
                        pv->pts_stop = pv->pts + date * 900;
                    }

                    i += 2;
                    break;
                }
                case 0x05: // 0x05 - SET_DAREA - defines the display area
                {
                    pv->x     = (pv->buf[i+0]<<4) | ((pv->buf[i+1]>>4)&0x0f);
                    pv->width = (((pv->buf[i+1]&0x0f)<<8)| pv->buf[i+2]) - pv->x + 1;
                    pv->y     = (pv->buf[i+3]<<4)| ((pv->buf[i+4]>>4)&0x0f);
                    pv->height = (((pv->buf[i+4]&0x0f)<<8)| pv->buf[i+5]) - pv->y + 1;
                    i += 6;
                    break;
                }
                case 0x06: // 0x06 - SET_DSPXA - defines the pixel data addresses
                {
                    pv->offsets[0] = ( pv->buf[i] << 8 ) | pv->buf[i+1]; i += 2;
                    pv->offsets[1] = ( pv->buf[i] << 8 ) | pv->buf[i+1]; i += 2;
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

    if( !pv->pts_stop )
    {
        /* Show it for 3 seconds */
        pv->pts_stop = pv->pts_start + 3 * 90000;
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
    hb_work_private_t * pv = w->private_data;
    int i;
    for( i = 0; i < pv->width; i++ )
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
    hb_work_private_t * pv = w->private_data;
    int i;
    for( i = 0; i < pv->height; i++ )
    {
        if( p[i*pv->width] )
        {
            return 0;
        }
    }
    return 1;
}
static hb_buffer_t * CropSubtitle( hb_work_object_t * w, uint8_t * raw )
{
    hb_work_private_t * pv = w->private_data;
    int i;
    int crop[4] = { -1,-1,-1,-1 };
    uint8_t * alpha;
    int realwidth, realheight;
    hb_buffer_t * buf;
    uint8_t * lum_in, * lum_out, * alpha_in, * alpha_out;
    uint8_t * u_in, * u_out, * v_in, * v_out;

    alpha = raw + pv->width * pv->height;

    /* Top */
    for( i = 0; i < pv->height; i++ )
    {
        if( !LineIsTransparent( w, &alpha[i*pv->width] ) )
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
    for( i = pv->height - 1; i >= 0; i-- )
    {
        if( !LineIsTransparent( w, &alpha[i*pv->width] ) )
        {
            crop[1] = i;
            break;
        }
    }

    /* Left */
    for( i = 0; i < pv->width; i++ )
    {
        if( !ColumnIsTransparent( w, &alpha[i] ) )
        {
            crop[2] = i;
            break;
        }
    }

    /* Right */
    for( i = pv->width - 1; i >= 0; i-- )
    {
        if( !ColumnIsTransparent( w, &alpha[i] ) )
        {
            crop[3] = i;
            break;
        }
    }

    realwidth  = crop[3] - crop[2] + 1;
    realheight = crop[1] - crop[0] + 1;

    buf         = hb_buffer_init( realwidth * realheight * 4 );
    buf->start  = pv->pts_start;
    buf->stop   = pv->pts_stop;
    buf->x      = pv->x + crop[2];
    buf->y      = pv->y + crop[0];
    buf->width  = realwidth;
    buf->height = realheight;

    lum_in    = raw + crop[0] * pv->width + crop[2];
    alpha_in  = lum_in + pv->width * pv->height;
    u_in      = alpha_in + pv->width * pv->height;
    v_in      = u_in + pv->width * pv->height;

    lum_out   = buf->data;
    alpha_out = lum_out + realwidth * realheight;
    u_out     = alpha_out + realwidth * realheight;
    v_out     = u_out + realwidth * realheight;

    for( i = 0; i < realheight; i++ )
    {
        memcpy( lum_out, lum_in, realwidth );
        memcpy( alpha_out, alpha_in, realwidth );
        memcpy( u_out, u_in, realwidth );
        memcpy( v_out, v_in, realwidth );

        lum_in    += pv->width;
        alpha_in  += pv->width;
        u_in      += pv->width;
        v_in      += pv->width;

        lum_out   += realwidth;
        alpha_out += realwidth;
        u_out     += realwidth;
        v_out     += realwidth;
    }

    return buf;
}

static hb_buffer_t * Decode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    int code, line, col;
    int offsets[2];
    int * offset;
    hb_buffer_t * buf;
    uint8_t * buf_raw = NULL;
    hb_job_t * job = pv->job;

    /* Get infos about the subtitle */
    ParseControls( w );

    if( job->indepth_scan || ( job->subtitle_force && pv->pts_forced == 0 ) )
    {
        /*
         * Don't encode subtitles when doing a scan.
         *
         * When forcing subtitles, ignore all those that don't
         * have the forced flag set.
         */
        return NULL;
    }

    /* Do the actual decoding now */
    buf_raw = malloc( ( pv->width * pv->height ) * 4 );

#define GET_NEXT_NIBBLE code = ( code << 4 ) | ( ( ( *offset & 1 ) ? \
( pv->buf[((*offset)>>1)] & 0xF ) : ( pv->buf[((*offset)>>1)] >> 4 ) ) ); \
(*offset)++

    offsets[0] = pv->offsets[0] * 2;
    offsets[1] = pv->offsets[1] * 2;

    for( line = 0; line < pv->height; line++ )
    {
        /* Select even or odd field */
        offset = ( line & 1 ) ? &offsets[1] : &offsets[0];

        for( col = 0; col < pv->width; col += code >> 2 )
        {
            uint8_t * lum, * alpha,  * chromaU, * chromaV;

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
                            code |= ( pv->width - col ) << 2;
                        }
                    }
                }
            }

            lum   = buf_raw;
            alpha = lum + pv->width * pv->height;
            chromaU = alpha + pv->width * pv->height;
            chromaV = chromaU + pv->width * pv->height;

            memset( lum + line * pv->width + col,
                    pv->lum[code & 3], code >> 2 );
            memset( alpha + line * pv->width + col,
                    pv->alpha[code & 3], code >> 2 );
            memset( chromaU + line * pv->width + col,
                    pv->chromaU[code & 3], code >> 2 );
            memset( chromaV + line * pv->width + col,
                    pv->chromaV[code & 3], code >> 2 );
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
