/* decavsub.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/decavsub.h"

struct hb_avsub_context_s
{
    AVCodecContext * context;
    AVPacket       * pkt;
    hb_job_t       * job;
    hb_subtitle_t  * subtitle;
    // For subs, when doing passthru, we don't know if we need a
    // packet until we have processed several packets.  So we cache
    // all the packets we see until libav returns a subtitle with
    // the information we need.
    hb_buffer_list_t list_pass;
    // List of subtitle packets to be output by this decoder.
    hb_buffer_list_t list;
    // XXX: we may occasionally see subtitles with broken timestamps
    //      while this should really get fixed elsewhere,
    //      dropping subtitles should be avoided as much as possible
    int64_t last_pts;
    // For PGS subs, we need to pass 'empty' subtitles through (they clear the
    // display) - when doing forced-only extraction, only pass empty subtitles
    // through if we've seen a forced sub since the last empty sub
    uint8_t seen_forced_sub;
};

struct hb_work_private_s
{
    hb_avsub_context_t * ctx;
};

hb_avsub_context_t * decavsubInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_avsub_context_t * ctx = calloc( 1, sizeof( hb_avsub_context_t ) );

    if (ctx == NULL)
    {
        return NULL;
    }
    ctx->seen_forced_sub       = 0;
    ctx->last_pts              = AV_NOPTS_VALUE;
    ctx->job                   = job;
    ctx->subtitle              = w->subtitle;

    const AVCodec  * codec   = avcodec_find_decoder(ctx->subtitle->codec_param);
    AVCodecContext * context = avcodec_alloc_context3(codec);
    context->codec = codec;

    hb_buffer_list_clear(&ctx->list);
    hb_buffer_list_clear(&ctx->list_pass);
    ctx->context               = context;
    context->pkt_timebase.num = ctx->subtitle->timebase.num;
    context->pkt_timebase.den = ctx->subtitle->timebase.den;

    // Set decoder opts...
    AVDictionary * av_opts = NULL;
    if (ctx->subtitle->source == CC608SUB)
    {
        av_dict_set( &av_opts, "data_field", "first", 0 );
        av_dict_set( &av_opts, "real_time", "1", 0 );
    }
    if (ctx->subtitle->source == VOBSUB && ctx->subtitle->palette_set)
    {
        char * palette = hb_strdup_printf(
            "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
            hb_yuv2rgb(ctx->subtitle->palette[0]),
            hb_yuv2rgb(ctx->subtitle->palette[1]),
            hb_yuv2rgb(ctx->subtitle->palette[2]),
            hb_yuv2rgb(ctx->subtitle->palette[3]),
            hb_yuv2rgb(ctx->subtitle->palette[4]),
            hb_yuv2rgb(ctx->subtitle->palette[5]),
            hb_yuv2rgb(ctx->subtitle->palette[6]),
            hb_yuv2rgb(ctx->subtitle->palette[7]),
            hb_yuv2rgb(ctx->subtitle->palette[8]),
            hb_yuv2rgb(ctx->subtitle->palette[9]),
            hb_yuv2rgb(ctx->subtitle->palette[10]),
            hb_yuv2rgb(ctx->subtitle->palette[11]),
            hb_yuv2rgb(ctx->subtitle->palette[12]),
            hb_yuv2rgb(ctx->subtitle->palette[13]),
            hb_yuv2rgb(ctx->subtitle->palette[14]),
            hb_yuv2rgb(ctx->subtitle->palette[15]));
        av_dict_set( &av_opts, "palette", palette, 0 );
        free(palette);
    }

    if (hb_avcodec_open(ctx->context, codec, &av_opts, 0))
    {
        av_dict_free( &av_opts );
        free(ctx);
        hb_log("decsubInit: avcodec_open failed");
        return NULL;
    }
    av_dict_free( &av_opts );

    ctx->pkt = av_packet_alloc();
    if (ctx->pkt == NULL)
    {
        hb_log("decsubInit: av_packet_alloc failed");
        return NULL;
    }

    if (ctx->subtitle->format == TEXTSUB)
    {
        int height = job->title->geometry.height - job->crop[0] - job->crop[1];
        int width  = job->title->geometry.width -  job->crop[2] - job->crop[3];
        switch (ctx->subtitle->codec_param)
        {
            case AV_CODEC_ID_ASS:
            {
                // Extradata should already be filled in by demux
            } break;

            case AV_CODEC_ID_EIA_608:
            {
                // Mono font for CC
                hb_subtitle_add_ssa_header(ctx->subtitle, HB_FONT_MONO,
                    20, 384, 288);
            } break;

            default:
            {
                hb_subtitle_add_ssa_header(ctx->subtitle, HB_FONT_SANS,
                    .066 * job->title->geometry.height, width, height);
            } break;
        }
    }
    return ctx;
}

static int decsubInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
    {
        return 1;
    }

    pv->ctx = decavsubInit(w, job);
    if (pv->ctx == NULL)
    {
        free(pv);
        return 1;
    }
    w->private_data = pv;
    return 0;
}

static void make_empty_pgs( hb_buffer_t * buf )
{
    hb_buffer_t * b = buf;
    uint8_t done = 0;

    // Each buffer is composed of 1 or more segments.
    // Segment header is:
    //      type   - 1 byte
    //      length - 2 bytes
    // We want to modify the presentation segment which is type 0x16
    //
    // Note that every pgs display set is required to have a presentation
    // segment, so we will only have to look at one display set.
    while ( b && !done )
    {
        int ii = 0;

        while (ii + 3 <= b->size)
        {
            uint8_t type;
            int len;
            int segment_len_pos;

            type = b->data[ii++];
            segment_len_pos = ii;
            len = ((int)b->data[ii] << 8) + b->data[ii+1];
            ii += 2;

            if (type == 0x16 && ii + len <= b->size)
            {
                int obj_count;
                int kk, jj = ii;
                int obj_start;

                // Skip
                // video descriptor 5 bytes
                // composition descriptor 3 bytes
                // palette update flg 1 byte
                // palette id ref 1 byte
                jj += 10;

                // Set number of composition objects to 0
                obj_count = b->data[jj];
                b->data[jj] = 0;
                jj++;
                obj_start = jj;

                // And remove all the composition objects
                for (kk = 0; kk < obj_count; kk++)
                {
                    uint8_t crop;

                    crop = b->data[jj + 3];
                    // skip
                    // object id - 2 bytes
                    // window id - 1 byte
                    // object/forced flag - 1 byte
                    // x pos - 2 bytes
                    // y pos - 2 bytes
                    jj += 8;
                    if (crop & 0x80)
                    {
                        // skip
                        // crop x - 2 bytes
                        // crop y - 2 bytes
                        // crop w - 2 bytes
                        // crop h - 2 bytes
                        jj += 8;
                    }
                }
                if (jj < b->size)
                {
                    memmove(b->data + obj_start, b->data + jj, b->size - jj);
                }
                b->size = obj_start + ( b->size - jj );
                done = 1;
                len = obj_start - (segment_len_pos + 2);
                b->data[segment_len_pos] = len >> 8;
                b->data[segment_len_pos+1] = len & 0xff;
                break;
            }
            ii += len;
        }
        b = b->next;
    }
}

static void make_empty_sub( int source, hb_buffer_list_t * list_pass )
{
    switch (source)
    {
        case PGSSUB:
            make_empty_pgs(hb_buffer_list_head(list_pass));
            break;
        case DVBSUB:
            break;
        default:
            hb_buffer_list_close(list_pass);
            break;
    }
}

// Returns a pointer to the first character after the ASS preamble
static const char * ssa_text(const char * ssa)
{
    int ii;
    const char * text = ssa;

    if (ssa == NULL)
        return NULL;
    for (ii = 0; ii < 8; ii++)
    {
        text = strchr(text, ',');
        if (text == NULL)
            break;
        text++;
    }
    return text;
}

int decavsubWork( hb_avsub_context_t * ctx,
                  hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in;
    hb_buffer_settings_t in_s = in->s;

    if (in_s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_in = NULL;
        hb_buffer_list_append(&ctx->list, in);

        *buf_out = hb_buffer_list_clear(&ctx->list);
        return HB_WORK_DONE;
    }

    if (!ctx->job->indepth_scan &&
        ctx->subtitle->config.dest == PASSTHRUSUB &&
        hb_subtitle_can_pass(ctx->subtitle->source, ctx->job->mux))
    {
        // Append to buffer list.  It will be sent to fifo after we determine
        // if this is a packet we need.
        hb_buffer_list_append(&ctx->list_pass, in);

        // We are keeping the buffer, so prevent the filter loop from
        // deleting it.
        *buf_in = NULL;
    }

    AVSubtitle subtitle;
    memset( &subtitle, 0, sizeof(subtitle) );

    int64_t duration = AV_NOPTS_VALUE;

    ctx->pkt->data = in->data;
    ctx->pkt->size = in->size;
    ctx->pkt->pts  = in_s.start;
    if (in_s.duration > 0 || ctx->subtitle->source == SSASUB || ctx->subtitle->source == IMPORTSSA)
    {
        duration = in_s.duration;
    }

    if (duration <= 0 &&
        in_s.start != AV_NOPTS_VALUE &&
        in_s.stop  != AV_NOPTS_VALUE &&
        in_s.stop > in_s.start)
    {
        duration = in_s.stop - in_s.start;
    }

    int has_subtitle = 0;

    while (ctx->pkt->size > 0)
    {
        int usedBytes = avcodec_decode_subtitle2(ctx->context, &subtitle,
                                                 &has_subtitle, ctx->pkt );
        if (usedBytes < 0)
        {
            hb_error("unable to decode subtitle with %d bytes.", ctx->pkt->size);
            return HB_WORK_OK;
        }

        if (usedBytes == 0)
        {
            // We expect avcodec_decode_subtitle2 to return the number
            // of bytes consumed, or an error.  If for some unforeseen reason
            // it returns 0, lets not get stuck in an infinite loop!
            usedBytes = ctx->pkt->size;
        }

        if (usedBytes <= ctx->pkt->size)
        {
            ctx->pkt->data += usedBytes;
            ctx->pkt->size -= usedBytes;
        }
        else
        {
            ctx->pkt->size = 0;
        }

        if (!has_subtitle)
        {
            continue;
        }

        uint8_t forced_sub = 0;
        uint8_t usable_sub = 0;
        uint8_t clear_sub  = 0;

        // collect subtitle statistics for foreign audio search
        if (subtitle.num_rects)
        {
            ctx->subtitle->hits++;
            if (subtitle.rects[0]->flags & AV_SUBTITLE_FLAG_FORCED)
            {
                forced_sub = 1;
                ctx->subtitle->forced_hits++;
            }
        }
        else
        {
            clear_sub = 1;
        }

        // do we need this subtitle?
        usable_sub =
            // Need all subs
            !ctx->subtitle->config.force ||
            // Need only forced subs
            forced_sub                 ||
            // Need to terminate last forced sub
            (ctx->seen_forced_sub && clear_sub);

        // do we need to create an empty subtitle?
        if (ctx->subtitle->config.force && ctx->seen_forced_sub && !usable_sub)
        {
            // We are forced-only and need to output this subtitle, but
            // it's neither forced nor empty.
            //
            // If passthru, create an empty subtitle.
            // Also, flag an empty subtitle for subtitle RENDER.
            make_empty_sub(ctx->subtitle->source, &ctx->list_pass);
            usable_sub = clear_sub = 1;
        }

        if (!usable_sub)
        {
            // Discard accumulated passthrough subtitle data
            hb_buffer_list_close(&ctx->list_pass);
            avsubtitle_free(&subtitle);
            continue;
        }

        // Keep track of forced subs that we may need to manually
        // terminate with an empty subtitle packet.
        ctx->seen_forced_sub = forced_sub && !clear_sub;

        int64_t pts = AV_NOPTS_VALUE;
        hb_buffer_t * out = NULL;

        if (clear_sub)
        {
            duration = 0;
        }
        else if (ctx->subtitle->source != DVBSUB &&
                 ctx->subtitle->source != PGSSUB &&
                 subtitle.end_display_time > 0 &&
                 subtitle.end_display_time < UINT32_MAX)
        {
            duration = av_rescale(subtitle.end_display_time, 90000, 1000);
        }
        if (subtitle.pts != AV_NOPTS_VALUE)
        {
            pts = av_rescale(subtitle.pts, 90000, AV_TIME_BASE) +
                  av_rescale(subtitle.start_display_time, 90000, 1000);
        }
        else
        {
            if (in_s.start >= 0)
            {
                pts = in_s.start;
            }
            else
            {
                // XXX: a broken pts will cause us to drop this subtitle,
                //      which is bad; use a default duration of 3 seconds
                //
                //      A broken pts is only generated when a subtitle packet
                //      occurs after a discontinuity and before the
                //      next audio or video packet which re-establishes
                //      timing (afaik).
                if (ctx->last_pts == AV_NOPTS_VALUE)
                {
                    pts = 0LL;
                }
                else
                {
                    pts = ctx->last_pts + 3 * 90000LL;
                }
                hb_log("[warning] decavsub: track %d, invalid PTS",
                       ctx->subtitle->out_track);
            }
        }
        // work around broken timestamps
        if (pts < ctx->last_pts)
        {
            // XXX: this should only happen if the previous pts
            // was unknown and our 3 second default duration
            // overshot the next subtitle pts.
            //
            // assign a 1 second duration
            hb_log("decavsub: track %d, non-monotonically increasing PTS, last %"PRId64" current %"PRId64"",
                   ctx->subtitle->out_track,
                   ctx->last_pts, pts);
            pts = ctx->last_pts + 1 * 90000LL;
        }
        ctx->last_pts = pts;

        if (ctx->subtitle->format == TEXTSUB)
        {
            // TEXTSUB && (PASSTHROUGHSUB || RENDERSUB)

            // Text subtitles are treated the same regardless of
            // whether we are burning or passing through.  They
            // get translated to SSA
            //
            // When using the "real_time" option with CC608 subtitles,
            // ffmpeg prepends an ASS rect that has only the preamble
            // to every list of returned rects.  libass doesn't like this
            // and logs a warning for every one of these. So strip these
            // out by using only the last rect in the list.
            //
            // Also, when a CC needs to be removed from the screen, ffmpeg
            // emits a single rect with only the preamble.  Detect this
            // and flag an "End Of Subtitle" EOS.
            int ii = subtitle.num_rects - 1;
            const char * text = ssa_text(subtitle.rects[ii]->ass);
            if (!clear_sub && text != NULL && *text != 0)
            {
                int size = strlen(subtitle.rects[ii]->ass) + 1;
                out = hb_buffer_init(size);
                strcpy((char*)out->data, subtitle.rects[ii]->ass);
            }
            else
            {
                out = hb_buffer_init(0);
                out->s.flags = HB_BUF_FLAG_EOS;
            }
            hb_buffer_list_close(&ctx->list_pass);
        }
        else if (ctx->subtitle->config.dest == PASSTHRUSUB &&
                 hb_subtitle_can_pass(ctx->subtitle->source, ctx->job->mux))
        {
            // PICTURESUB && PASSTHROUGHSUB

            // subtitles may be spread across multiple packets
            //
            // In the MKV container, all segments are found in the same
            // packet (this is expected by some devices, such as the
            // WD TV Live).  So if there are multiple packets,
            // merge them.
            if (hb_buffer_list_count(&ctx->list_pass) == 1)
            {
                // packets already merged (e.g. MKV sources)
                out = hb_buffer_list_clear(&ctx->list_pass);
                out->s.start    = AV_NOPTS_VALUE;
                out->s.stop     = AV_NOPTS_VALUE;
                out->s.duration = (int64_t)AV_NOPTS_VALUE;
            }
            else
            {
                int size = 0;
                uint8_t * data;
                hb_buffer_t * b;

                b = hb_buffer_list_head(&ctx->list_pass);
                while (b != NULL)
                {
                    size += b->size;
                    b = b->next;
                }

                out = hb_buffer_init( size );
                out->s.duration = (int64_t)AV_NOPTS_VALUE;
                data = out->data;
                b = hb_buffer_list_head(&ctx->list_pass);
                while (b != NULL)
                {
                    memcpy(data, b->data, b->size);
                    data += b->size;
                    b = b->next;
                }
                hb_buffer_list_close(&ctx->list_pass);
            }
            if (clear_sub)
            {
                out->s.flags = HB_BUF_FLAG_EOS;
            }
        }
        else
        {
            // PICTURESUB && RENDERSUB
            if (!clear_sub)
            {
                unsigned ii, x0, y0, x1, y1, w, h;

                x0 = subtitle.rects[0]->x;
                y0 = subtitle.rects[0]->y;
                x1 = subtitle.rects[0]->x + subtitle.rects[0]->w;
                y1 = subtitle.rects[0]->y + subtitle.rects[0]->h;

                // First, find total bounding rectangle
                for (ii = 1; ii < subtitle.num_rects; ii++)
                {
                    if (subtitle.rects[ii]->x < x0)
                        x0 = subtitle.rects[ii]->x;
                    if (subtitle.rects[ii]->y < y0)
                        y0 = subtitle.rects[ii]->y;
                    if (subtitle.rects[ii]->x + subtitle.rects[ii]->w > x1)
                        x1 = subtitle.rects[ii]->x + subtitle.rects[ii]->w;
                    if (subtitle.rects[ii]->y + subtitle.rects[ii]->h > y1)
                        y1 = subtitle.rects[ii]->y + subtitle.rects[ii]->h;
                }
                w = x1 - x0;
                h = y1 - y0;

                out = hb_frame_buffer_init(AV_PIX_FMT_YUVA420P, w, h);
                memset(out->data, 0, out->size);

                out->f.x             = x0;
                out->f.y             = y0;
                out->f.window_width  = ctx->context->width;
                out->f.window_height = ctx->context->height;
                for (ii = 0; ii < subtitle.num_rects; ii++)
                {
                    AVSubtitleRect *rect = subtitle.rects[ii];

                    int off_x = rect->x - x0;
                    int off_y = rect->y - y0;
                    uint8_t *lum     = out->plane[0].data;
                    uint8_t *chromaU = out->plane[1].data;
                    uint8_t *chromaV = out->plane[2].data;
                    uint8_t *alpha   = out->plane[3].data;

                    lum     += off_y * out->plane[0].stride + off_x;
                    alpha   += off_y * out->plane[3].stride + off_x;
                    chromaU += (off_y >> 1) * out->plane[1].stride + (off_x >> 1);
                    chromaV += (off_y >> 1) * out->plane[2].stride + (off_x >> 1);

                    int xx, yy;
                    for (yy = 0; yy < rect->h; yy++)
                    {
                        for (xx = 0; xx < rect->w; xx++)
                        {
                            uint32_t argb, yuv;
                            int pixel;
                            uint8_t color;

                            pixel = yy * rect->w + xx;
                            color = rect->data[0][pixel];
                            argb = ((uint32_t*)rect->data[1])[color];
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
                }
            }
            else
            {
                out = hb_buffer_init( 0 );

                out->s.flags  = HB_BUF_FLAG_EOS;
                out->f.x      = 0;
                out->f.y      = 0;
                out->f.width  = 0;
                out->f.height = 0;
                duration      = 0;
            }
        }
        out->s.id           = in_s.id;
        out->s.scr_sequence = in_s.scr_sequence;
        out->s.frametype    = HB_FRAME_SUBTITLE;
        out->s.start        = pts;
        if (duration != AV_NOPTS_VALUE)
        {
            out->s.stop      = pts + duration;
            out->s.duration  = duration;
        }

        hb_buffer_list_append(&ctx->list, out);
        avsubtitle_free(&subtitle);
    }

    *buf_out = hb_buffer_list_clear(&ctx->list);
    return HB_WORK_OK;
}

static int decsubWork( hb_work_object_t * w,
                       hb_buffer_t ** buf_in,
                       hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;

    return decavsubWork(pv->ctx, buf_in, buf_out );
}

void decavsubClose( hb_avsub_context_t * ctx )
{
    if (ctx == NULL)
    {
        return;
    }
    av_packet_free(&ctx->pkt);
    hb_buffer_list_close(&ctx->list_pass);
    avcodec_flush_buffers(ctx->context);
    avcodec_free_context(&ctx->context);
    free(ctx);
}

static void decsubClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    if (pv == NULL)
    {
        return;
    }
    decavsubClose(pv->ctx);
    free(pv);
    w->private_data = NULL;
}

hb_work_object_t hb_decavsub =
{
    .id    = WORK_DECAVSUB,
    .name  = "Subtitle decoder (libavcodec)",
    .init  = decsubInit,
    .work  = decsubWork,
    .close = decsubClose,
};
