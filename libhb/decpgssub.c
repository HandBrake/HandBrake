#include "hb.h"
#include "hbffmpeg.h"

struct hb_work_private_s
{
    AVCodecContext * context;
    hb_job_t * job;
    // For PGS subs, when doing passthru, we don't know if we need a
    // packet until we have processed several packets.  So we cache
    // all the packets we see until libav returns a subtitle with
    // the information we need.
    hb_buffer_t * list_pass_buffer;
    hb_buffer_t * last_pass_buffer;
    // It is possible for multiple subtitles to be enncapsulated in
    // one packet.  This won't happen for PGS subs, but may for other
    // types of subtitles.  Since I plan to generalize this code to handle
    // other than PGS, we will need to keep a list of all subtitles seen
    // while parsing an input packet.
    hb_buffer_t * list_buffer;
    hb_buffer_t * last_buffer;
};

static int decsubInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec *codec = avcodec_find_decoder( CODEC_ID_HDMV_PGS_SUBTITLE );
    AVCodecContext *context = avcodec_alloc_context3( codec );
    context->codec = codec;

    hb_work_private_t * pv;
    pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->context = context;
    pv->job = job;

    return 0;
}

static int decsubWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if ( in->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        if ( pv->list_buffer == NULL )
        {
            pv->list_buffer = pv->last_buffer = in;
        }
        else
        {
            pv->last_buffer->next = in;
        }
        *buf_in = NULL;
        *buf_out = pv->list_buffer;
        pv->list_buffer = NULL;

        return HB_WORK_DONE;
    }

    if ( !pv->job->indepth_scan &&
         w->subtitle->config.dest == PASSTHRUSUB &&
         hb_subtitle_can_pass( PGSSUB, pv->job->mux ) )
    {
        // Append to buffer list.  It will be sent to fifo after we determine
        // if this is a packet we need.
        if ( pv->list_pass_buffer == NULL )
        {
            pv->list_pass_buffer = pv->last_pass_buffer = in;
        }
        else
        {
            pv->last_pass_buffer->next = in;
            pv->last_pass_buffer = in;
        }
        // We are keeping the buffer, so prevent the filter loop from
        // deleting it.
        *buf_in = NULL;
    }

    AVSubtitle subtitle;
    memset( &subtitle, 0, sizeof(subtitle) );

    AVPacket avp;
    av_init_packet( &avp );
    avp.data = in->data;
    avp.size = in->size;
    // libav wants pkt pts in AV_TIME_BASE units
    avp.pts = av_rescale(in->s.start, AV_TIME_BASE, 90000);

    int has_subtitle = 0;

    do
    {
        int usedBytes = avcodec_decode_subtitle2( pv->context, &subtitle, &has_subtitle, &avp );
        if (usedBytes < 0)
        {
            hb_log("unable to decode subtitle with %d bytes.", avp.size);
            return HB_WORK_OK;
        }

        if (usedBytes <= avp.size)
        {
            avp.data += usedBytes;
            avp.size -= usedBytes;
        }
        else
        {
            avp.size = 0;
        }

        // The sub is "usable" if:
        // 1. libav returned a sub AND
        // 2. we are scanning for foreign audio subs OR
        // 3. we want all subs (e.g. not forced) OR
        // 4. the sub is forced and we only want forced OR
        // 5. subtitle clears the previous sub (these are never forced)
        //    subtitle.num_rects == 0
        uint8_t useable_sub =
                    pv->job->indepth_scan ||
                    subtitle.num_rects == 0 ||
                    !w->subtitle->config.force ||
                    ( w->subtitle->config.force && subtitle.forced );

        if (has_subtitle && useable_sub)
        {
            int64_t pts = av_rescale(subtitle.pts, 90000, AV_TIME_BASE);
            hb_buffer_t * out = NULL;

            if( pv->job->indepth_scan )
            {
                if (subtitle.forced)
                {
                    w->subtitle->forced_hits++;
                }
            }
            else if ( w->subtitle->config.dest == PASSTHRUSUB &&
                 hb_subtitle_can_pass( PGSSUB, pv->job->mux ) )
            {
                out = pv->list_pass_buffer;
                pv->list_pass_buffer = NULL;
            }
            else
            {
                if (subtitle.num_rects != 0)
                {
                    unsigned ii;
                    for (ii = 0; ii < subtitle.num_rects; ii++)
                    {
                        AVSubtitleRect *rect = subtitle.rects[ii];

                        out = hb_frame_buffer_init(
                                PIX_FMT_YUVA420P, rect->w, rect->h );

                        out->s.id     = in->s.id;
                        out->sequence = in->sequence;
                        out->s.start  = pts;
                        out->s.stop   = 0;
                        out->f.x      = rect->x;
                        out->f.y      = rect->y;

                        uint8_t *lum     = out->plane[0].data;
                        uint8_t *chromaU = out->plane[1].data;
                        uint8_t *chromaV = out->plane[2].data;
                        uint8_t *alpha   = out->plane[3].data;

                        int xx, yy;
                        for (yy = 0; yy < rect->h; yy++)
                        {
                            for (xx = 0; xx < rect->w; xx++)
                            {
                                uint32_t argb, yuv;
                                int pixel;
                                uint8_t color;

                                pixel = yy * rect->w + xx;
                                color = rect->pict.data[0][pixel];
                                argb = ((uint32_t*)rect->pict.data[1])[color];
                                yuv = hb_rgb2yuv(argb);

                                lum[xx] = (yuv >> 16) & 0xff;
                                alpha[xx] = (argb >> 24) & 0xff;
                                if ((xx & 1) == 0 && (yy & 1) == 0)
                                {
                                    chromaV[xx>>1] = (yuv >> 8) & 0xff;
                                    chromaU[xx>>1] = yuv & 0xff;
                                }
                            }
                            lum += out->plane[0].stride;
                            if ((yy & 1) == 0)
                            {
                                chromaU += out->plane[1].stride;
                                chromaV += out->plane[2].stride;
                            }
                            alpha += out->plane[3].stride;
                        }

                        if ( pv->list_buffer == NULL )
                        {
                            pv->list_buffer = pv->last_buffer = out;
                        }
                        else
                        {
                            pv->last_buffer->next = out;
                            pv->last_buffer = out;
                        }
                        out = NULL;
                    }
                }
                else
                {
                    out = hb_buffer_init( 1 );

                    out->s.id     = in->s.id;
                    out->s.start  = pts;
                    out->s.stop   = 0;
                    out->f.x      = 0;
                    out->f.y      = 0;
                    out->f.width  = 0;
                    out->f.height = 0;
                }
            }
            if ( pv->list_buffer == NULL )
            {
                pv->list_buffer = pv->last_buffer = out;
            }
            else
            {
                pv->last_buffer->next = out;
            }
            while (pv->last_buffer && pv->last_buffer->next)
            {
                pv->last_buffer = pv->last_buffer->next;
            }
        }
        else if ( has_subtitle )
        {
            hb_buffer_close( &pv->list_pass_buffer );
            pv->list_pass_buffer = NULL;
        }
        if ( has_subtitle )
        {
            avsubtitle_free(&subtitle);
        }
    } while (avp.size > 0);

    *buf_out = pv->list_buffer;
    pv->list_buffer = NULL;
    return HB_WORK_OK;
}

static void decsubClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    avcodec_flush_buffers( pv->context );
    avcodec_close( pv->context );
}

hb_work_object_t hb_decpgssub =
{
    WORK_DECPGSSUB,
    "PGS decoder",
    decsubInit,
    decsubWork,
    decsubClose
};
