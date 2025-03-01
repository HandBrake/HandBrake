/* decssasub.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Converts SSA subtitles to either:
 * (1) TEXTSUB format: UTF-8 subtitles with limited HTML-style markup (<b>, <i>, <u>), or
 * (2) PICTURESUB format, using libass.
 *
 * SSA format references:
 *   http://www.matroska.org/technical/specs/subtitles/ssa.html
 *   http://moodub.free.fr/video/ass-specs.doc
 *   vlc-1.0.4/modules/codec/subtitles/subsass.c:ParseSSAString
 *
 * libass references:
 *   libass-0.9.9/ass.h
 *   vlc-1.0.4/modules/codec/libass.c
 *
 * @author David Foster (davidfstr)
 */

#include "handbrake/handbrake.h"
#include "handbrake/decavsub.h"
#include "handbrake/extradata.h"
#include "libavformat/avformat.h"

struct hb_work_private_s
{
    AVFormatContext       * ic;
    hb_decavsub_context_t * ctx;
    AVPacket              * pkt;
    hb_job_t              * job;
    hb_subtitle_t         * subtitle;

    // Time of first desired subtitle adjusted by reader_pts_offset
    uint64_t start_time;
    uint64_t stop_time;
};

static int extradataInit( hb_work_private_t * pv )
{
    if (pv->ic->nb_streams < 1)
    {
        hb_error("SSA demux found no streams");
        return 1;
    }

    AVStream * st = pv->ic->streams[0];
    if (st->codecpar->codec_id != AV_CODEC_ID_ASS)
    {
        hb_error("SSA demux found wrong codec_id %x", st->codecpar->codec_id);
        return 1;
    }
    if (st->codecpar->extradata != NULL)
    {
        hb_set_extradata(&pv->subtitle->extradata, st->codecpar->extradata, st->codecpar->extradata_size);
    }
    return 0;
}

static int decssaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = NULL;
    int                 ii;

    if (w->subtitle->config.src_filename == NULL)
    {
        hb_error("No SSA subtitle file specified");
        goto fail;
    }

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
    {
        goto fail;
    }
    w->private_data = pv;
    pv->job         = job;
    pv->subtitle    = w->subtitle;

    pv->pkt = av_packet_alloc();
    if (pv->pkt == NULL)
    {
        hb_error("decssaInit: av_packet_alloc failed");
        goto fail;
    }

    if (avformat_open_input(&pv->ic, pv->subtitle->config.src_filename,
        NULL, NULL ) < 0 )
    {
        hb_error("Could not open the SSA subtitle file '%s'\n",
                 pv->subtitle->config.src_filename);
        goto fail;
    }
    pv->ctx = decavsubInit(w, job);
    if (pv->ctx == NULL)
    {
        goto fail;
    }

    if (extradataInit(pv))
    {
        goto fail;
    }

    pv->ctx = decavsubInit(w, job);
    if (pv->ctx == NULL)
    {
        goto fail;
    }

    /*
     * Figure out the start and stop times from the chapters being
     * encoded - drop subtitle not in this range.
     */
    pv->start_time = 0;
    for (ii = 1; ii < job->chapter_start; ++ii)
    {
        hb_chapter_t * chapter = hb_list_item(job->list_chapter, ii - 1);
        if (chapter)
        {
            pv->start_time += chapter->duration;
        } else {
            hb_error("Could not locate chapter %d for SSA start time", ii);
        }
    }
    pv->stop_time = pv->start_time;
    for (ii = job->chapter_start; ii <= job->chapter_end; ++ii)
    {
        hb_chapter_t * chapter = hb_list_item(job->list_chapter, ii - 1);
        if (chapter)
        {
            pv->stop_time += chapter->duration;
        } else {
            hb_error("Could not locate chapter %d for SSA start time", ii);
        }
    }

    hb_deep_log(3, "SSA Start time %"PRId64", stop time %"PRId64,
                pv->start_time, pv->stop_time);

    if (job->pts_to_start != 0)
    {
        // Compute start_time after reader sets reader_pts_offset
        pv->start_time = AV_NOPTS_VALUE;
    }

    return 0;

fail:
    if (pv != NULL)
    {
        av_packet_free(&pv->pkt);
        decavsubClose(pv->ctx);
        if (pv->ic)
        {
            avformat_close_input(&pv->ic);
        }
        free(pv);
        w->private_data = NULL;
    }
    return 1;
}

static hb_buffer_t * ssa_read( hb_work_private_t * pv )
{
    int           err;
    hb_buffer_t * out;

    if (pv->job->reader_pts_offset == AV_NOPTS_VALUE)
    {
        // We need to wait for reader to initialize it's pts offset so that
        // we know where to start reading SSA.
        return NULL;
    }
    if (pv->start_time == AV_NOPTS_VALUE)
    {
        pv->start_time = pv->job->reader_pts_offset;
        if (pv->job->pts_to_stop > 0)
        {
            pv->stop_time = pv->job->pts_to_start + pv->job->pts_to_stop;
        }
    }

    if ((err = av_read_frame(pv->ic, pv->pkt)) < 0)
    {
        if (err != AVERROR_EOF)
        {
            hb_error("SSA demux read error %d", err);
        }
        return hb_buffer_eof_init();
    }
    AVStream * st = pv->ic->streams[pv->pkt->stream_index];

    out = hb_buffer_init(pv->pkt->size + 1);
    memcpy(out->data, pv->pkt->data, pv->pkt->size);
    out->data[pv->pkt->size] = 0;
    out->size = pv->pkt->size;

    double tsconv = (double)90000. * st->time_base.num / st->time_base.den;
    if (pv->pkt->pts != AV_NOPTS_VALUE)
    {
        out->s.start = pv->pkt->pts * tsconv +
                       pv->subtitle->config.offset * 90;
    }
    if (pv->pkt->dts != AV_NOPTS_VALUE)
    {
        out->s.renderOffset = pv->pkt->dts * tsconv +
                              pv->subtitle->config.offset * 90;
    }
    if (out->s.renderOffset >= 0 && out->s.start == AV_NOPTS_VALUE)
    {
        out->s.start = out->s.renderOffset;
    }
    else if (out->s.renderOffset == AV_NOPTS_VALUE && out->s.start >= 0)
    {
        out->s.renderOffset = out->s.start;
    }
    int64_t pkt_duration = pv->pkt->duration;
    if (pkt_duration != AV_NOPTS_VALUE)
    {
        out->s.duration = pkt_duration * tsconv;
        out->s.stop = out->s.start + out->s.duration;
    }
    else
    {
        out->s.duration = (int64_t)AV_NOPTS_VALUE;
    }
    out->s.type = SUBTITLE_BUF;
    av_packet_unref(pv->pkt);

    if (out->s.stop  <= pv->start_time || out->s.start >= pv->stop_time)
    {
        // Drop subtitles that end before the PtoP start time
        // or start after the PtoP stop time
        hb_deep_log(3, "Discarding SSA at time start %"PRId64", stop %"PRId64,
                    out->s.start, out->s.stop);
        hb_buffer_close(&out);
    }
    else
    {
        // Adjust start/stop for subtitles that span PtoP start/stop
        if (out->s.start < pv->start_time)
        {
            out->s.start = pv->start_time;
        }
        if (out->s.stop > pv->stop_time)
        {
            out->s.stop = pv->stop_time;
        }
        out->s.start -= pv->start_time;
        out->s.stop  -= pv->start_time;
    }
    return out;
}

static int decssaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv =  w->private_data;
    hb_buffer_t       * in;
    int                 result;

    in = ssa_read(pv);
    if (in == NULL)
    {
        return HB_WORK_OK;
    }

    result = decavsubWork(pv->ctx, &in, buf_out);
    if (in != NULL)
    {
        hb_buffer_close(&in);
    }
    return result;
}

static void decssaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    if (pv != NULL)
    {
        decavsubClose(pv->ctx);
        av_packet_free(&pv->pkt);
        avformat_close_input(&pv->ic);
        free(pv);
    }
    w->private_data = NULL;
}

hb_work_object_t hb_decssasub =
{
    WORK_DECSSASUB,
    "SSA Subtitle Decoder",
    decssaInit,
    decssaWork,
    decssaClose
};
