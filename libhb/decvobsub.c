/* decvobsub.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Decoder for DVD bitmap subtitles, also known as "VOB subtitles" within the HandBrake source code.
 * 
 * Input format of the subtitle packets is described here:
 *   http://sam.zoy.org/writings/dvd/subtitles/
 *
 * An auxiliary input is the color palette lookup table, in 'subtitle->palette'.
 * The demuxer implementation must fill out this table appropriately.
 * - In the case of a true DVD input, the palette is read from the IFO file.
 * - In the case of an MKV file input, the palette is read from the codec private data of the subtitle track.
 *
 * Output format of this decoder is PICTURESUB, which is:
 *   struct PictureSubPacket {
 *       uint8_t lum[pixelCount];       // Y
 *       uint8_t alpha[pixelCount];     // alpha (max = 16)
 *       uint8_t chromaU[pixelCount];   // Cb
 *       uint8_t chromaV[pixelCount];   // Cr
 *   }
 */

#include "hb.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    hb_buffer_t * buf;
    int           size_sub;
    int           size_got;
    int           size_rle;
    int64_t       pts;
    int64_t       pts_start;
    int64_t       pts_stop;
    int           pts_forced;
    int           x;
    int           y;
    int           width;
    int           height;
    int           stream_id;

    int           offsets[2];
    uint8_t       lum[4];
    uint8_t       chromaU[4];
    uint8_t       chromaV[4];
    uint8_t       alpha[4];
    uint8_t       palette_set;
};

static hb_buffer_t * Decode( hb_work_object_t * );

int decsubInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;
    pv->pts = 0;
    
    // Warn if the input color palette is empty
    pv->palette_set = w->subtitle->palette_set;
    if ( pv->palette_set )
    {
        // Make sure the entries in the palette are not all 0
        pv->palette_set = 0;
        int i;
        for (i=0; i<16; i++)
        {
            if (w->subtitle->palette[i])
            {
                pv->palette_set = 1;
                break;
            }
        }
    }
    if (!pv->palette_set) {
        hb_log( "decvobsub: input color palette is empty!" );
    }

    return 0;
}

int decsubWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;
    int size_sub, size_rle;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    pv->stream_id = in->s.id;

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

            pv->buf      = hb_buffer_init( 0xFFFF );
            memcpy( pv->buf->data, in->data, in->size );
            pv->buf->s.id = in->s.id;
            pv->buf->s.frametype = HB_FRAME_SUBTITLE;
            pv->buf->sequence = in->sequence;
            pv->size_got = in->size;
            if( in->s.start >= 0 )
            {
                pv->pts      = in->s.start;
            }
        }
    }
    else
    {
        /* We are waiting for the end of the current subtitle */
        if( in->size <= pv->size_sub - pv->size_got )
        {
            memcpy( pv->buf->data + pv->size_got, in->data, in->size );
            pv->buf->s.id = in->s.id;
            pv->buf->sequence = in->sequence;
            pv->size_got += in->size;
            if( in->s.start >= 0 )
            {
                pv->pts = in->s.start;
            }
        }
        else
        {
            // bad size, must have lost sync
            // force re-sync
            if ( pv->buf != NULL )
                hb_buffer_close( &pv->buf );
            pv->size_sub = 0;
        }

    }

    *buf_out = NULL;

    if( pv->size_sub && pv->size_sub == pv->size_got )
    {
        pv->buf->size = pv->size_sub;

        /* We got a complete subtitle, decode it */
        *buf_out = Decode( w );

        if( buf_out && *buf_out )
        {
            (*buf_out)->s.id = in->s.id;
            (*buf_out)->sequence = in->sequence;
        }

        /* Wait for the next one */
        pv->size_sub = 0;
        pv->size_got = 0;
        pv->size_rle = 0;

        if ( pv->pts_stop != AV_NOPTS_VALUE )
        {
            // If we don't get a valid next timestamp, use the stop time
            // of the current sub as the start of the next.
            // This can happen if reader invalidates timestamps while 
            // waiting for an audio to update the SCR.
            pv->pts      = pv->pts_stop;
        }
    }

    return HB_WORK_OK;
}

void decsubClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv->buf )
        hb_buffer_close( &pv->buf );
    free( w->private_data );
}

hb_work_object_t hb_decvobsub =
{
    WORK_DECVOBSUB,
    "VOBSUB decoder",
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
    uint8_t * buf = pv->buf->data;

    int i;
    int command;
    int date, next;

    pv->pts_start = AV_NOPTS_VALUE;
    pv->pts_stop  = AV_NOPTS_VALUE;
    pv->pts_forced  = 0;

    pv->alpha[3] = 0;
    pv->alpha[2] = 0;
    pv->alpha[1] = 0;
    pv->alpha[0] = 0;

    for( i = pv->size_rle; ; )
    {
        date = ( buf[i] << 8 ) | buf[i+1]; i += 2;
        next = ( buf[i] << 8 ) | buf[i+1]; i += 2;

        for( ;; )
        {
            command = buf[i++];

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
                    pv->pts_start = pv->pts + date * 1024;
                    pv->pts_forced = 1;
                    w->subtitle->hits++;
                    w->subtitle->forced_hits++;
                    break;

                case 0x01: // 0x01 - STA_DSP - Start Display, no arguments
                    pv->pts_start = pv->pts + date * 1024;
                    pv->pts_forced  = 0;
                    w->subtitle->hits++;
                    break;

                case 0x02: // 0x02 - STP_DSP - Stop Display, no arguments
                    if(pv->pts_stop == AV_NOPTS_VALUE)
                        pv->pts_stop = pv->pts + date * 1024;
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

                    colors[0] = (buf[i+0]>>4)&0x0f;
                    colors[1] = (buf[i+0])&0x0f;
                    colors[2] = (buf[i+1]>>4)&0x0f;
                    colors[3] = (buf[i+1])&0x0f;

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
                        uint32_t color = w->subtitle->palette[colors[j]];
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

                    alpha[3] = ((buf[i+0] >> 4) & 0x0f) << 4;
                    alpha[2] = ((buf[i+0]     ) & 0x0f) << 4;
                    alpha[1] = ((buf[i+1] >> 4) & 0x0f) << 4;
                    alpha[0] = ((buf[i+1]     ) & 0x0f) << 4;


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
                    if (currAlpha < lastAlpha && pv->pts_stop == AV_NOPTS_VALUE)
                    {
                        pv->pts_stop = pv->pts + date * 1024;
                    }

                    i += 2;
                    break;
                }
                case 0x05: // 0x05 - SET_DAREA - defines the display area
                {
                    pv->x     = (buf[i+0]<<4) | ((buf[i+1]>>4)&0x0f);
                    pv->width = (((buf[i+1]&0x0f)<<8)| buf[i+2]) - pv->x + 1;
                    pv->y     = (buf[i+3]<<4)| ((buf[i+4]>>4)&0x0f);
                    pv->height = (((buf[i+4]&0x0f)<<8)| buf[i+5]) - pv->y + 1;
                    i += 6;
                    break;
                }
                case 0x06: // 0x06 - SET_DSPXA - defines the pixel data addresses
                {
                    pv->offsets[0] = ( buf[i] << 8 ) | buf[i+1]; i += 2;
                    pv->offsets[1] = ( buf[i] << 8 ) | buf[i+1]; i += 2;
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
    // Generate timestamps if they are not set
    if( pv->pts_start == AV_NOPTS_VALUE )
    {
        // Set pts to end of last sub if the start time is unknown.
        pv->pts_start = pv->pts;
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

// Brain dead resampler.  This should really use swscale...
// Uses Bresenham algo to pick source samples for averaging
static void resample( uint8_t * dst, uint8_t * src, int dst_w, int src_w )
{
    int dst_x, src_x, err, cnt, sum, val;

    if( dst_w < src_w )
    {
        // sample down
        err = 0;
        sum = 0;
        val = 0;
        cnt = 0;
        err = src_w / 2;
        dst_x = 0;
        for( src_x = 0; src_x < src_w; src_x++ )
        {
            sum += src[src_x];
            cnt++;
            err -= dst_w;
            if( err < 0 )
            {
                val = sum / cnt;
                dst[dst_x++] = val;
                sum = cnt = 0;
                err += src_w;
            }
        }
        for( ; dst_x < dst_w; dst_x++ )
        {
            dst[dst_x] = val;
        }
    }
    else
    {
        // sample up
        err = 0;
        err = dst_w / 2;
        src_x = 0;
        for( dst_x = 0; dst_x < dst_w; dst_x++ )
        {
            dst[dst_x] = src[src_x];
            err -= src_w;
            if( err < 0 )
            {
                src_x++;
                err += dst_w;
            }
        }
    }
}

static hb_buffer_t * CropSubtitle( hb_work_object_t * w, uint8_t * raw )
{
    hb_work_private_t * pv = w->private_data;
    int i;
    int crop[4] = { -1,-1,-1,-1 };
    uint8_t * alpha;
    int realwidth, realheight;
    hb_buffer_t * buf;
    uint8_t * lum_in, * alpha_in, * u_in, * v_in;

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

    buf = hb_frame_buffer_init( AV_PIX_FMT_YUVA420P, realwidth, realheight );
    buf->s.frametype = HB_FRAME_SUBTITLE;
    buf->s.start  = pv->pts_start;
    buf->s.stop   = pv->pts_stop;

    buf->f.x = pv->x + crop[2];
    buf->f.y = pv->y + crop[0];
    buf->f.window_width  = w->subtitle->width;
    buf->f.window_height = w->subtitle->height;

    lum_in    = raw + crop[0] * pv->width + crop[2];
    alpha_in  = lum_in + pv->width * pv->height;
    u_in      = alpha_in + pv->width * pv->height;
    v_in      = u_in + pv->width * pv->height;

    uint8_t *dst;
    for( i = 0; i < realheight; i++ )
    {
        // Luma
        dst = buf->plane[0].data + buf->plane[0].stride * i;
        memcpy( dst, lum_in, realwidth );

        if( ( i & 1 ) == 0 )
        {
            // chroma U (resample to YUV420)
            dst = buf->plane[1].data + buf->plane[1].stride * ( i >> 1 );
            resample( dst, u_in, buf->plane[1].width, realwidth );

            // chroma V (resample to YUV420)
            dst = buf->plane[2].data + buf->plane[2].stride * ( i >> 1 );
            resample( dst, v_in, buf->plane[2].width, realwidth );
        }
        // Alpha
        dst = buf->plane[3].data + buf->plane[3].stride * i;
        memcpy( dst, alpha_in, realwidth );

        lum_in    += pv->width;
        alpha_in  += pv->width;
        u_in      += pv->width;
        v_in      += pv->width;
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

    if( job->indepth_scan || ( w->subtitle->config.force && pv->pts_forced == 0 ) )
    {
        /*
         * Don't encode subtitles when doing a scan.
         *
         * When forcing subtitles, ignore all those that don't
         * have the forced flag set.
         */
        hb_buffer_close( &pv->buf );
        return NULL;
    }

    if (w->subtitle->config.dest == PASSTHRUSUB)
    {
        pv->buf->s.start  = pv->pts_start;
        pv->buf->s.stop   = pv->pts_stop;
        buf = pv->buf;
        pv->buf = NULL;
        return buf;
    }

    /* Do the actual decoding now */
    buf_raw = malloc( ( pv->width * pv->height ) * 4 );

#define GET_NEXT_NIBBLE code = ( code << 4 ) | ( ( ( *offset & 1 ) ? \
( pv->buf->data[((*offset)>>1)] & 0xF ) : ( pv->buf->data[((*offset)>>1)] >> 4 ) ) ); \
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

    hb_buffer_close( &pv->buf );

    /* Crop subtitle (remove transparent borders) */
    buf = CropSubtitle( w, buf_raw );

    free( buf_raw );

    return buf;
}
