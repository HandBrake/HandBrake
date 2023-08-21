/* enctheora.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "theora/codec.h"
#include "theora/theoraenc.h"

int  enctheoraInit( hb_work_object_t *, hb_job_t * );
int  enctheoraWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void enctheoraClose( hb_work_object_t * );

hb_work_object_t hb_enctheora =
{
    WORK_ENCTHEORA,
    "Theora encoder (libtheora)",
    enctheoraInit,
    enctheoraWork,
    enctheoraClose
};

struct hb_work_private_s
{
    hb_job_t    * job;

    th_enc_ctx    * ctx;

    FILE          * file;
    unsigned char   stat_buf[80];
    int             stat_read;
    int             stat_fill;
};

int enctheoraInit( hb_work_object_t * w, hb_job_t * job )
{
    int keyframe_frequency, log_keyframe, ret;
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
    {
        hb_error("theora: calloc failed");
        return 1;
    }
    w->private_data = pv;

    pv->job = job;

    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
        job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        char * filename;
        filename = hb_get_temporary_filename("theora.log");
        if ( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
        {
            pv->file = hb_fopen(filename, "wb");
        }
        else
        {
            pv->file = hb_fopen(filename, "rb");
        }
        free(filename);
    }

    th_info ti;
    th_comment tc;
    ogg_packet op;
    th_info_init( &ti );

    /* Frame width and height need to be multiples of 16 */
    ti.pic_width = job->width;
    ti.pic_height = job->height;
    ti.frame_width = (job->width + 0xf) & ~0xf;
    ti.frame_height = (job->height + 0xf) & ~0xf;
    ti.pic_x = ti.pic_y = 0;
    ti.fps_numerator = job->vrate.num;
    ti.fps_denominator = job->vrate.den;
    ti.aspect_numerator = job->par.num;
    ti.aspect_denominator = job->par.den;
    ti.colorspace = TH_CS_UNSPECIFIED;
    ti.pixel_fmt = TH_PF_420;
    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        ti.target_bitrate = job->vbitrate * 1000;
        ti.quality = 0;
    }
    else
    {
        ti.target_bitrate = 0;
        ti.quality = job->vquality;
    }

    keyframe_frequency = ((double)job->orig_vrate.num / job->orig_vrate.den +
                                  0.5) * 10;

    hb_log("theora: keyint: %i", keyframe_frequency);

    int tmp = keyframe_frequency - 1;
    for (log_keyframe = 0; tmp; log_keyframe++)
        tmp >>= 1;

    ti.keyframe_granule_shift = log_keyframe;

    pv->ctx = th_encode_alloc( &ti );
    th_info_clear( &ti );

    ret = th_encode_ctl(pv->ctx, TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
                        &keyframe_frequency, sizeof(keyframe_frequency));
    if( ret < 0 )
    {
        hb_log("theora: Could not set keyframe interval to %d", keyframe_frequency);
    }

    /* Set "soft target" rate control which improves quality at the
     * expense of solid bitrate caps */
    int arg = TH_RATECTL_CAP_UNDERFLOW;
    ret = th_encode_ctl(pv->ctx, TH_ENCCTL_SET_RATE_FLAGS, &arg, sizeof(arg));
    if( ret < 0 )
    {
        hb_log("theora: Could not set soft ratecontrol");
    }
    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
        job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        arg = keyframe_frequency * 7 >> 1;
        ret = th_encode_ctl(pv->ctx, TH_ENCCTL_SET_RATE_BUFFER, &arg, sizeof(arg));
        if( ret < 0 )
        {
            hb_log("theora: Could not set rate control buffer");
        }
    }

    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
    {
        unsigned char *buffer;
        int bytes;
        bytes = th_encode_ctl(pv->ctx, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
        if( bytes < 0 )
        {
            hb_error("Could not set up the first pass of two-pass mode.\n");
            hb_error("Did you remember to specify an estimated bitrate?\n");
            return 1;
        }
        if( fwrite( buffer, 1, bytes, pv->file ) < bytes )
        {
            hb_error("Unable to write to two-pass data file.\n");
            return 1;
        }
        fflush( pv->file );
    }
    if( job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        /* Enable the second pass here.
         * We make this call just to set the encoder into 2-pass mode, because
         * by default enabling two-pass sets the buffer delay to the whole file
         * (because there's no way to explicitly request that behavior).
         * If we waited until we were actually encoding, it would overwrite our
         * settings.*/
        hb_log("enctheora: init 2nd pass");
        if( th_encode_ctl( pv->ctx, TH_ENCCTL_2PASS_IN, NULL, 0) < 0)
        {
            hb_log("theora: Could not set up the second pass of two-pass mode.");
            return 1;
        }
    }

    th_comment_init( &tc );

    ogg_packet *header;

    int ii;
    for (ii = 0; ii < 3; ii++)
    {
        th_encode_flushheader( pv->ctx, &tc, &op );
        header = (ogg_packet*)w->config->theora.headers[ii];
        memcpy(header, &op, sizeof(op));
        header->packet = w->config->theora.headers[ii] + sizeof(ogg_packet);
        memcpy(header->packet, op.packet, op.bytes );
    }

    th_comment_clear( &tc );

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void enctheoraClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    th_encode_free( pv->ctx );

    if( pv->file )
    {
        fclose( pv->file );
    }
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int enctheoraWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    hb_buffer_t * in = *buf_in, * buf;
    th_ycbcr_buffer ycbcr;
    ogg_packet op;

    int frame_width, frame_height;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input - send it downstream & say we're done.
        // XXX may need to flush packets via a call to
        //  th_encode_packetout( pv->ctx, 1, &op );
        // but we don't have a timestamp to put on those packets so we
        // drop them for now.
        *buf_out = in;
        *buf_in = NULL;
        th_encode_packetout( pv->ctx, 1, &op );
        if( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
        {
            unsigned char *buffer;
            int bytes;

            bytes = th_encode_ctl(pv->ctx, TH_ENCCTL_2PASS_OUT,
                                  &buffer, sizeof(buffer));
            if( bytes < 0 )
            {
                fprintf(stderr,"Could not read two-pass data from encoder.\n");
                return HB_WORK_DONE;
            }
            fseek( pv->file, 0, SEEK_SET );
            if( fwrite( buffer, 1, bytes, pv->file ) < bytes)
            {
                fprintf(stderr,"Unable to write to two-pass data file.\n");
                return HB_WORK_DONE;
            }
            fflush( pv->file );
        }
        return HB_WORK_DONE;
    }

    if( job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        for(;;)
        {
            int bytes, size, ret;
            /*Ask the encoder how many bytes it would like.*/
            bytes = th_encode_ctl( pv->ctx, TH_ENCCTL_2PASS_IN, NULL, 0 );
            if( bytes < 0 )
            {
                hb_error("Error requesting stats size in second pass.");
                *job->done_error = HB_ERROR_UNKNOWN;
                *job->die = 1;
                return HB_WORK_DONE;
            }

            /*If it's got enough, stop.*/
            if( bytes == 0 ) break;

            /*Read in some more bytes, if necessary.*/
            if( bytes > pv->stat_fill - pv->stat_read )
                size = bytes - (pv->stat_fill - pv->stat_read);
            else
                size = 0;
            if( size > 80 - pv->stat_fill )
                size = 80 - pv->stat_fill;
            if( size > 0 &&
                fread( pv->stat_buf+pv->stat_fill, 1, size, pv->file ) < size )
            {
                hb_error("Could not read frame data from two-pass data file!");
                *job->done_error = HB_ERROR_UNKNOWN;
                *job->die = 1;
                return HB_WORK_DONE;
            }
            pv->stat_fill += size;

            /*And pass them off.*/
            if( bytes > pv->stat_fill - pv->stat_read )
                bytes = pv->stat_fill - pv->stat_read;
            ret = th_encode_ctl( pv->ctx, TH_ENCCTL_2PASS_IN,
                                 pv->stat_buf+pv->stat_read, bytes);
            if( ret < 0 )
            {
                hb_error("Error submitting pass data in second pass.");
                *job->done_error = HB_ERROR_UNKNOWN;
                *job->die = 1;
                return HB_WORK_DONE;
            }
            /*If the encoder consumed the whole buffer, reset it.*/
            if( ret >= pv->stat_fill - pv->stat_read )
                pv->stat_read = pv->stat_fill = 0;
            /*Otherwise remember how much it used.*/
            else
                pv->stat_read += ret;
        }
    }
    memset(&op, 0, sizeof(op));
    memset(&ycbcr, 0, sizeof(ycbcr));

    frame_width = (job->width + 0xf) & ~0xf;
    frame_height = (job->height + 0xf) & ~0xf;

    // Y
    ycbcr[0].width = frame_width;
    ycbcr[0].height = frame_height;

    // CbCr decimated by factor of 2 in both width and height
    ycbcr[1].width  = ycbcr[2].width  = (frame_width + 1) / 2;
    ycbcr[1].height = ycbcr[2].height = (frame_height + 1) / 2;

    ycbcr[0].stride = in->plane[0].stride;
    ycbcr[1].stride = in->plane[1].stride;
    ycbcr[2].stride = in->plane[2].stride;

    ycbcr[0].data = in->plane[0].data;
    ycbcr[1].data = in->plane[1].data;
    ycbcr[2].data = in->plane[2].data;

    th_encode_ycbcr_in( pv->ctx, ycbcr );

    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
    {
        unsigned char *buffer;
        int bytes;

        bytes = th_encode_ctl(pv->ctx, TH_ENCCTL_2PASS_OUT,
                              &buffer, sizeof(buffer));
        if( bytes < 0 )
        {
            fprintf(stderr,"Could not read two-pass data from encoder.\n");
            *job->done_error = HB_ERROR_UNKNOWN;
            *job->die = 1;
            return HB_WORK_DONE;
        }
        if( fwrite( buffer, 1, bytes, pv->file ) < bytes)
        {
            fprintf(stderr,"Unable to write to two-pass data file.\n");
            *job->done_error = HB_ERROR_UNKNOWN;
            *job->die = 1;
            return HB_WORK_DONE;
        }
        fflush( pv->file );
    }
    th_encode_packetout( pv->ctx, 0, &op );

    buf = hb_buffer_init(op.bytes);
    memcpy(buf->data, op.packet, op.bytes);
    buf->f.fmt = AV_PIX_FMT_YUV420P;
    buf->f.width = frame_width;
    buf->f.height = frame_height;
    buf->s.flags = HB_FLAG_FRAMETYPE_REF;
    buf->s.frametype = HB_FRAME_I;
    if (th_packet_iskeyframe(&op))
    {
        buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
    }
    buf->s.start    = in->s.start;
    buf->s.stop     = in->s.stop;
    buf->s.duration = in->s.stop - in->s.start;

    *buf_out = buf;

    return HB_WORK_OK;
}

