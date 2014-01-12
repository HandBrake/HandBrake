/* decpgssub.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
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
    // XXX: we may occasionally see subtitles with broken timestamps
    //      while this should really get fixed elsewhere,
    //      dropping subtitles should be avoided as much as possible
    int64_t last_pts;
    // for PGS subs, we need to pass 'empty' subtitles through (they clear the
    // display) - when doing forced-only extraction, only pass empty subtitles
    // through if we've seen a forced sub and haven't seen any empty sub since
    uint8_t seen_forced_sub;
    // if we start encoding partway through the source, we may encounter empty
    // subtitles before we see any actual subtitle content - discard them
    uint8_t discard_subtitle;
};

static int decsubInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec *codec = avcodec_find_decoder( AV_CODEC_ID_HDMV_PGS_SUBTITLE );
    AVCodecContext *context = avcodec_alloc_context3( codec );
    context->codec = codec;

    hb_work_private_t * pv;
    pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->discard_subtitle = 1;
    pv->seen_forced_sub  = 0;
    pv->last_pts         = 0;
    pv->context          = context;
    pv->job              = job;

    // Set decoder opts...
    AVDictionary * av_opts = NULL;
    // e.g. av_dict_set( &av_opts, "refcounted_frames", "1", 0 );

    if (hb_avcodec_open(pv->context, codec, &av_opts, 0))
    {
        av_dict_free( &av_opts );
        hb_log("decsubInit: avcodec_open failed");
        return 1;
    }
    av_dict_free( &av_opts );


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
    if (in->s.start != -1)
    {
        avp.pts = av_rescale(in->s.start, AV_TIME_BASE, 90000);
    }
    else
    {
        avp.pts = AV_NOPTS_VALUE;
    }

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

        /* Subtitles are "usable" if:
         *   1. Libav returned a subtitle (has_subtitle) AND
         *   2. we're not doing Foreign Audio Search (!pv->job->indepth_scan) AND
         *   3. the sub is non-empty or we've seen one such sub before (!pv->discard_subtitle)
         * For forced-only extraction, usable subtitles also need to:
         *   a. be forced (subtitle.rects[0]->flags & AV_SUBTITLE_FLAG_FORCED) OR
         *   b. follow a forced sub (pv->seen_forced_sub) */
        uint8_t forced_sub     = 0;
        uint8_t useable_sub    = 0;
        uint8_t clear_subtitle = 0;

        if (has_subtitle)
        {
            // subtitle statistics
            if (subtitle.num_rects)
            {
                w->subtitle->hits++;
                if (subtitle.rects[0]->flags & AV_SUBTITLE_FLAG_FORCED)
                {
                    forced_sub = 1;
                    w->subtitle->forced_hits++;
                }
            }
            else
            {
                clear_subtitle = 1;
            }
            // are we doing Foreign Audio Search?
            if (!pv->job->indepth_scan)
            {
                // do we want to discard this subtitle?
                pv->discard_subtitle = pv->discard_subtitle && clear_subtitle;
                // do we need this subtitle?
                useable_sub = (!pv->discard_subtitle &&
                               (!w->subtitle->config.force ||
                                forced_sub || pv->seen_forced_sub));
                // do we need to create an empty subtitle?
                if (w->subtitle->config.force &&
                    useable_sub && !forced_sub && !clear_subtitle)
                {
                    // We are forced-only and need to output this subtitle, but
                    // it's neither forced nor empty.
                    //
                    // If passthru, create an empty subtitle.
                    // Also, flag an empty subtitle for subtitle RENDER.
                    make_empty_pgs(pv->list_pass_buffer);
                    clear_subtitle = 1;
                }
                // is the subtitle forced?
                pv->seen_forced_sub = forced_sub;
            }
        }

        if (useable_sub)
        {
            int64_t pts = AV_NOPTS_VALUE;
            hb_buffer_t * out = NULL;

            if (subtitle.pts != AV_NOPTS_VALUE)
            {
                pts = av_rescale(subtitle.pts, 90000, AV_TIME_BASE);
            }
            else
            {
                if (in->s.start >= 0)
                {
                    pts = in->s.start;
                }
                else
                {
                    // XXX: a broken pts will cause us to drop this subtitle,
                    //      which is bad; use a default duration of 3 seconds
                    //
                    //      A broken pts is only generated when a pgs packet
                    //      occurs after a discontinuity and before the
                    //      next audio or video packet which re-establishes
                    //      timing (afaik).
                    pts = pv->last_pts + 3 * 90000LL;
                    hb_log("[warning] decpgssub: track %d, invalid PTS",
                           w->subtitle->out_track);
                }
            }
            // work around broken timestamps
            if (pts < pv->last_pts)
            {
                // XXX: this should only happen if the prevous pts
                // was unknown and our 3 second default duration
                // overshot the next pgs pts.
                //
                // assign a 1 second duration
                pts = pv->last_pts + 1 * 90000LL;
                hb_log("[warning] decpgssub: track %d, non-monotically increasing PTS",
                       w->subtitle->out_track);
            }
            pv->last_pts = pts;

            if ( w->subtitle->config.dest == PASSTHRUSUB &&
                 hb_subtitle_can_pass( PGSSUB, pv->job->mux ) )
            {
                /* PGS subtitles are spread across multiple packets (1 per segment).
                 * In the MKV container, all segments are found in the same packet
                 * (this is expected by some devices, such as the WD TV Live).
                 * So if there are multiple packets, merge them. */
                if (pv->list_pass_buffer->next == NULL)
                {
                    // packets already merged (e.g. MKV sources)
                    out = pv->list_pass_buffer;
                    pv->list_pass_buffer = NULL;
                }
                else
                {
                    int size = 0;
                    uint8_t * data;
                    hb_buffer_t * b;

                    b = pv->list_pass_buffer;
                    while (b != NULL)
                    {
                        size += b->size;
                        b = b->next;
                    }

                    out = hb_buffer_init( size );
                    data = out->data;
                    b = pv->list_pass_buffer;
                    while (b != NULL)
                    {
                        memcpy( data, b->data, b->size );
                        data += b->size;
                        b = b->next;
                    }
                    hb_buffer_close( &pv->list_pass_buffer );

                    out->s        = in->s;
                    out->s.frametype = HB_FRAME_SUBTITLE;
                    out->sequence = in->sequence;
                }
                out->s.renderOffset = -1;
                out->s.start = out->s.stop = pts;
            }
            else
            {
                if (!clear_subtitle)
                {
                    unsigned ii;
                    for (ii = 0; ii < subtitle.num_rects; ii++)
                    {
                        AVSubtitleRect *rect = subtitle.rects[ii];

                        out = hb_frame_buffer_init(AV_PIX_FMT_YUVA420P,
                                                   rect->w, rect->h);

                        out->s.frametype = HB_FRAME_SUBTITLE;
                        out->s.id     = in->s.id;
                        out->sequence = in->sequence;
                        out->s.start  = pts;
                        out->s.stop   = pts;
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

                    out->s.frametype = HB_FRAME_SUBTITLE;
                    out->s.id     = in->s.id;
                    out->s.start  = pts;
                    out->s.stop   = pts;
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
