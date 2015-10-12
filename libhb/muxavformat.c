/* muxavformat.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <ogg/ogg.h>
#include "libavformat/avformat.h"
#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"

#include "hb.h"
#include "lang.h"

struct hb_mux_data_s
{
    enum
    {
        MUX_TYPE_VIDEO,
        MUX_TYPE_AUDIO,
        MUX_TYPE_SUBTITLE
    } type;

    AVStream    *st;

    int64_t  duration;

    hb_buffer_t * delay_buf;

    int64_t  prev_chapter_tc;
    int16_t  current_chapter;

    AVBitStreamFilterContext* bitstream_filter;
};

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t          * job;

    AVFormatContext   * oc;
    AVRational          time_base;

    int                 ntracks;
    hb_mux_data_t    ** tracks;

    int64_t             chapter_delay;
};

enum
{
    META_TITLE,
    META_ARTIST,
    META_DIRECTOR,
    META_COMPOSER,
    META_RELEASE_DATE,
    META_COMMENT,
    META_ALBUM,
    META_GENRE,
    META_DESCRIPTION,
    META_SYNOPSIS,
    META_LAST
};

enum
{
    META_MUX_MP4,
    META_MUX_MKV,
    META_MUX_LAST
};

const char *metadata_keys[META_LAST][META_MUX_LAST] =
{
    {"title",        "TITLE"},
    {"artist",       "ARTIST"},
    {"album_artist", "DIRECTOR"},
    {"composer",     "COMPOSER"},
    {"date",         "DATE_RELEASED"},
    {"comment",      "SUMMARY"},
    {"album",        NULL},
    {"genre",        "GENRE"},
    {"description",  "DESCRIPTION"},
    {"synopsis",     "SYNOPSIS"}
};

static char* lookup_lang_code(int mux, char *iso639_2)
{
    iso639_lang_t   *lang;
    char *out = NULL;

    switch (mux)
    {
        case HB_MUX_AV_MP4:
            out = iso639_2;
            break;
        case HB_MUX_AV_MKV:
            // MKV lang codes should be ISO-639-2B if it exists,
            // else ISO-639-2
            lang =  lang_for_code2( iso639_2 );
            out = lang->iso639_2b ? lang->iso639_2b : lang->iso639_2;
            break;
        default:
            break;
    }
    return out;
}

/**********************************************************************
 * avformatInit
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int avformatInit( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_audio_t    * audio;
    hb_mux_data_t * track;
    int meta_mux;
    int max_tracks;
    int ii, jj, ret;

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    const char *muxer_name = NULL;

    uint8_t         default_track_flag = 1;
    uint8_t         need_fonts = 0;
    char *lang;


    max_tracks = 1 + hb_list_count( job->list_audio ) +
                     hb_list_count( job->list_subtitle );

    m->tracks = calloc(max_tracks, sizeof(hb_mux_data_t*));

    m->oc = avformat_alloc_context();
    if (m->oc == NULL)
    {
        hb_error( "Could not initialize avformat context." );
        goto error;
    }

    AVDictionary * av_opts = NULL;
    switch (job->mux)
    {
        case HB_MUX_AV_MP4:
            m->time_base.num = 1;
            m->time_base.den = 90000;
            if( job->ipod_atom )
                muxer_name = "ipod";
            else
                muxer_name = "mp4";
            meta_mux = META_MUX_MP4;

            av_dict_set(&av_opts, "brand", "mp42", 0);
            if (job->mp4_optimize)
                av_dict_set(&av_opts, "movflags", "faststart+disable_chpl", 0);
            else
                av_dict_set(&av_opts, "movflags", "+disable_chpl", 0);
            break;

        case HB_MUX_AV_MKV:
            // libavformat is essentially hard coded such that it only
            // works with a timebase of 1/1000
            m->time_base.num = 1;
            m->time_base.den = 1000;
            muxer_name = "matroska";
            meta_mux = META_MUX_MKV;
            break;

        default:
        {
            hb_error("Invalid Mux %x", job->mux);
            goto error;
        }
    }
    m->oc->oformat = av_guess_format(muxer_name, NULL, NULL);
    if(m->oc->oformat == NULL)
    {
        hb_error("Could not guess output format %s", muxer_name);
        goto error;
    }
    av_strlcpy(m->oc->filename, job->file, sizeof(m->oc->filename));
    ret = avio_open2(&m->oc->pb, job->file, AVIO_FLAG_WRITE,
                     &m->oc->interrupt_callback, NULL);
    if( ret < 0 )
    {
        hb_error( "avio_open2 failed, errno %d", ret);
        goto error;
    }

    /* Video track */
    track = m->tracks[m->ntracks++] = calloc(1, sizeof( hb_mux_data_t ) );
    job->mux_data = track;

    track->type = MUX_TYPE_VIDEO;
    track->prev_chapter_tc = AV_NOPTS_VALUE;
    track->st = avformat_new_stream(m->oc, NULL);
    if (track->st == NULL)
    {
        hb_error("Could not initialize video stream");
        goto error;
    }
    track->st->time_base = m->time_base;
    avcodec_get_context_defaults3(track->st->codec, NULL);

    track->st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    track->st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    uint8_t *priv_data = NULL;
    int priv_size = 0;
    switch (job->vcodec)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
        case HB_VCODEC_QSV_H264:
            track->st->codec->codec_id = AV_CODEC_ID_H264;

            /* Taken from x264 muxers.c */
            priv_size = 5 + 1 + 2 + job->config.h264.sps_length + 1 + 2 +
                        job->config.h264.pps_length;
            priv_data = av_malloc(priv_size);
            if (priv_data == NULL)
            {
                hb_error("H.264 extradata: malloc failure");
                goto error;
            }

            priv_data[0] = 1;
            priv_data[1] = job->config.h264.sps[1]; /* AVCProfileIndication */
            priv_data[2] = job->config.h264.sps[2]; /* profile_compat */
            priv_data[3] = job->config.h264.sps[3]; /* AVCLevelIndication */
            priv_data[4] = 0xff; // nalu size length is four bytes
            priv_data[5] = 0xe1; // one sps

            priv_data[6] = job->config.h264.sps_length >> 8;
            priv_data[7] = job->config.h264.sps_length;

            memcpy(priv_data+8, job->config.h264.sps,
                   job->config.h264.sps_length);

            priv_data[8+job->config.h264.sps_length] = 1; // one pps
            priv_data[9+job->config.h264.sps_length] =
                                        job->config.h264.pps_length >> 8;
            priv_data[10+job->config.h264.sps_length] =
                                        job->config.h264.pps_length;

            memcpy(priv_data+11+job->config.h264.sps_length,
                   job->config.h264.pps, job->config.h264.pps_length );
            break;

        case HB_VCODEC_FFMPEG_MPEG4:
            track->st->codec->codec_id = AV_CODEC_ID_MPEG4;

            if (job->config.mpeg4.length != 0)
            {
                priv_size = job->config.mpeg4.length;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("MPEG4 extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data, job->config.mpeg4.bytes, priv_size);
            }
            break;

        case HB_VCODEC_FFMPEG_MPEG2:
            track->st->codec->codec_id = AV_CODEC_ID_MPEG2VIDEO;

            if (job->config.mpeg4.length != 0)
            {
                priv_size = job->config.mpeg4.length;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("MPEG2 extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data, job->config.mpeg4.bytes, priv_size);
            }
            break;

        case HB_VCODEC_FFMPEG_VP8:
            track->st->codec->codec_id = AV_CODEC_ID_VP8;
            priv_data                  = NULL;
            priv_size                  = 0;
            break;

        case HB_VCODEC_THEORA:
        {
            track->st->codec->codec_id = AV_CODEC_ID_THEORA;

            int size = 0;
            ogg_packet *ogg_headers[3];

            for (ii = 0; ii < 3; ii++)
            {
                ogg_headers[ii] = (ogg_packet *)job->config.theora.headers[ii];
                size += ogg_headers[ii]->bytes + 2;
            }

            priv_size = size;
            priv_data = av_malloc(priv_size);
            if (priv_data == NULL)
            {
                hb_error("Theora extradata: malloc failure");
                goto error;
            }

            size = 0;
            for(ii = 0; ii < 3; ii++)
            {
                AV_WB16(priv_data + size, ogg_headers[ii]->bytes);
                size += 2;
                memcpy(priv_data+size, ogg_headers[ii]->packet,
                                       ogg_headers[ii]->bytes);
                size += ogg_headers[ii]->bytes;
            }
        } break;

        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
        case HB_VCODEC_QSV_H265:
            track->st->codec->codec_id = AV_CODEC_ID_HEVC;

            if (job->config.h265.headers_length > 0)
            {
                priv_size = job->config.h265.headers_length;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("H.265 extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data, job->config.h265.headers, priv_size);
            }
            break;

        default:
            hb_error("muxavformat: Unknown video codec: %x", job->vcodec);
            goto error;
    }
    track->st->codec->extradata = priv_data;
    track->st->codec->extradata_size = priv_size;

    track->st->sample_aspect_ratio.num        = job->par.num;
    track->st->sample_aspect_ratio.den        = job->par.den;
    track->st->codec->sample_aspect_ratio.num = job->par.num;
    track->st->codec->sample_aspect_ratio.den = job->par.den;
    track->st->codec->width                   = job->width;
    track->st->codec->height                  = job->height;
    track->st->disposition |= AV_DISPOSITION_DEFAULT;

    hb_rational_t vrate = job->vrate;

    // If the vrate is the internal clock rate, there's a good chance
    // this is a standard rate that we have in our hb_video_rates table.
    // Because of rounding errors and approximations made while
    // measuring framerate, the actual value may not be exact.  So
    // we look for rates that are "close" and make an adjustment
    // to fps.den.
    if (vrate.num == clock)
    {
        const hb_rate_t *video_framerate = NULL;
        while ((video_framerate = hb_video_framerate_get_next(video_framerate)) != NULL)
        {
            if (abs(vrate.den - video_framerate->rate) < 10)
            {
                vrate.den = video_framerate->rate;
                break;
            }
        }
    }
    hb_reduce(&vrate.num, &vrate.den, vrate.num, vrate.den);
    if (job->mux == HB_MUX_AV_MP4)
    {
        // libavformat mp4 muxer requires that the codec time_base have the
        // same denominator as the stream time_base, it uses it for the
        // mdhd timescale.
        double scale = (double)track->st->time_base.den / vrate.num;
        track->st->codec->time_base.den = track->st->time_base.den;
        track->st->codec->time_base.num = vrate.den * scale;
    }
    else
    {
        track->st->codec->time_base.num = vrate.den;
        track->st->codec->time_base.den = vrate.num;
    }
    track->st->avg_frame_rate.num = vrate.num;
    track->st->avg_frame_rate.den = vrate.den;

    /* add the audio tracks */
    for(ii = 0; ii < hb_list_count( job->list_audio ); ii++ )
    {
        audio = hb_list_item( job->list_audio, ii );
        track = m->tracks[m->ntracks++] = calloc(1, sizeof( hb_mux_data_t ) );
        audio->priv.mux_data = track;

        track->type = MUX_TYPE_AUDIO;

        track->st = avformat_new_stream(m->oc, NULL);
        if (track->st == NULL)
        {
            hb_error("Could not initialize audio stream");
            goto error;
        }
        avcodec_get_context_defaults3(track->st->codec, NULL);

        track->st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        track->st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        if (job->mux == HB_MUX_AV_MP4)
        {
            track->st->codec->time_base.num = audio->config.out.samples_per_frame;
            track->st->codec->time_base.den = audio->config.out.samplerate;
            track->st->time_base.num = 1;
            track->st->time_base.den = audio->config.out.samplerate;
        }
        else
        {
            track->st->codec->time_base = m->time_base;
            track->st->time_base = m->time_base;
        }

        priv_data = NULL;
        priv_size = 0;
        switch (audio->config.out.codec & HB_ACODEC_MASK)
        {
            case HB_ACODEC_DCA:
            case HB_ACODEC_DCA_HD:
                track->st->codec->codec_id = AV_CODEC_ID_DTS;
                break;
            case HB_ACODEC_AC3:
                track->st->codec->codec_id = AV_CODEC_ID_AC3;
                break;
            case HB_ACODEC_FFEAC3:
                track->st->codec->codec_id = AV_CODEC_ID_EAC3;
                break;
            case HB_ACODEC_FFTRUEHD:
                track->st->codec->codec_id = AV_CODEC_ID_TRUEHD;
                break;
            case HB_ACODEC_LAME:
            case HB_ACODEC_MP3:
                track->st->codec->codec_id = AV_CODEC_ID_MP3;
                break;
            case HB_ACODEC_VORBIS:
            {
                track->st->codec->codec_id = AV_CODEC_ID_VORBIS;

                int jj, size = 0;
                ogg_packet *ogg_headers[3];

                for (jj = 0; jj < 3; jj++)
                {
                    ogg_headers[jj] = (ogg_packet *)audio->priv.config.vorbis.headers[jj];
                    size += ogg_headers[jj]->bytes + 2;
                }

                priv_size = size;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("Vorbis extradata: malloc failure");
                    goto error;
                }

                size = 0;
                for(jj = 0; jj < 3; jj++)
                {
                    AV_WB16(priv_data + size, ogg_headers[jj]->bytes);
                    size += 2;
                    memcpy(priv_data+size, ogg_headers[jj]->packet,
                                           ogg_headers[jj]->bytes);
                    size += ogg_headers[jj]->bytes;
                }
            } break;
            case HB_ACODEC_FFFLAC:
            case HB_ACODEC_FFFLAC24:
                track->st->codec->codec_id = AV_CODEC_ID_FLAC;

                if (audio->priv.config.extradata.length)
                {
                    priv_size = audio->priv.config.extradata.length;
                    priv_data = av_malloc(priv_size);
                    if (priv_data == NULL)
                    {
                        hb_error("FLAC extradata: malloc failure");
                        goto error;
                    }
                    memcpy(priv_data,
                           audio->priv.config.extradata.bytes,
                           audio->priv.config.extradata.length);
                }
                break;
            case HB_ACODEC_FFAAC:
            case HB_ACODEC_CA_AAC:
            case HB_ACODEC_CA_HAAC:
            case HB_ACODEC_FDK_AAC:
            case HB_ACODEC_FDK_HAAC:
                track->st->codec->codec_id = AV_CODEC_ID_AAC;

                // libav mkv muxer expects there to be extradata for
                // AAC and will crash if it is NULL.  So allocate extra
                // byte so that av_malloc does not return NULL when length
                // is 0.
                priv_size = audio->priv.config.extradata.length;
                priv_data = av_malloc(priv_size + 1);
                if (priv_data == NULL)
                {
                    hb_error("AAC extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data,
                       audio->priv.config.extradata.bytes,
                       audio->priv.config.extradata.length);

                // AAC from pass-through source may be ADTS.
                // Therefore inserting "aac_adtstoasc" bitstream filter is
                // preferred.
                // The filter does nothing for non-ADTS bitstream.
                if (audio->config.out.codec == HB_ACODEC_AAC_PASS)
                {
                    track->bitstream_filter = av_bitstream_filter_init("aac_adtstoasc");
                }
                break;
            default:
                hb_error("muxavformat: Unknown audio codec: %x",
                         audio->config.out.codec);
                goto error;
        }
        track->st->codec->extradata = priv_data;
        track->st->codec->extradata_size = priv_size;

        if( default_track_flag )
        {
            track->st->disposition |= AV_DISPOSITION_DEFAULT;
            default_track_flag = 0;
        }

        lang = lookup_lang_code(job->mux, audio->config.lang.iso639_2 );
        if (lang != NULL)
        {
            av_dict_set(&track->st->metadata, "language", lang, 0);
        }
        track->st->codec->sample_rate = audio->config.out.samplerate;
        if (audio->config.out.codec & HB_ACODEC_PASS_FLAG)
        {
            track->st->codec->channels = av_get_channel_layout_nb_channels(audio->config.in.channel_layout);
            track->st->codec->channel_layout = audio->config.in.channel_layout;
        }
        else
        {
            track->st->codec->channels = hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);
            track->st->codec->channel_layout = hb_ff_mixdown_xlat(audio->config.out.mixdown, NULL);
        }

        char *name;
        if (audio->config.out.name == NULL)
        {
            switch (track->st->codec->channels)
            {
                case 1:
                    name = "Mono";
                    break;

                case 2:
                    name = "Stereo";
                    break;

                default:
                    name = "Surround";
                    break;
            }
        }
        else
        {
            name = audio->config.out.name;
        }
        // Set audio track title
        av_dict_set(&track->st->metadata, "title", name, 0);
        if (job->mux == HB_MUX_AV_MP4)
        {
            // Some software (MPC, mediainfo) use hdlr description
            // for track title
            av_dict_set(&track->st->metadata, "handler", name, 0);
        }
    }

    // Check for audio track associations
    for (ii = 0; ii < hb_list_count(job->list_audio); ii++)
    {
        audio = hb_list_item(job->list_audio, ii);
        switch (audio->config.out.codec & HB_ACODEC_MASK)
        {
            case HB_ACODEC_FFAAC:
            case HB_ACODEC_CA_AAC:
            case HB_ACODEC_CA_HAAC:
            case HB_ACODEC_FDK_AAC:
            case HB_ACODEC_FDK_HAAC:
                break;

            default:
            {
                // Mark associated fallback audio tracks for any non-aac track
                for(jj = 0; jj < hb_list_count( job->list_audio ); jj++ )
                {
                    hb_audio_t    * fallback;
                    int             codec;

                    if (ii == jj) continue;

                    fallback = hb_list_item( job->list_audio, jj );
                    codec = fallback->config.out.codec & HB_ACODEC_MASK;
                    if (fallback->config.in.track == audio->config.in.track &&
                        (codec == HB_ACODEC_FFAAC ||
                         codec == HB_ACODEC_CA_AAC ||
                         codec == HB_ACODEC_CA_HAAC ||
                         codec == HB_ACODEC_FDK_AAC ||
                         codec == HB_ACODEC_FDK_HAAC))
                    {
                        hb_mux_data_t * fallback_track;
                        int           * sd;

                        track = audio->priv.mux_data;
                        fallback_track = fallback->priv.mux_data;
                        sd = (int*)av_stream_new_side_data(track->st,
                                                     AV_PKT_DATA_FALLBACK_TRACK,
                                                     sizeof(int));
                        if (sd != NULL)
                        {
                            *sd = fallback_track->st->index;
                        }
                    }
                }
            } break;
        }
    }

    char * subidx_fmt =
        "size: %dx%d\n"
        "org: %d, %d\n"
        "scale: 100%%, 100%%\n"
        "alpha: 100%%\n"
        "smooth: OFF\n"
        "fadein/out: 50, 50\n"
        "align: OFF at LEFT TOP\n"
        "time offset: 0\n"
        "forced subs: %s\n"
        "palette: %06x, %06x, %06x, %06x, %06x, %06x, "
        "%06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x\n"
        "custom colors: OFF, tridx: 0000, "
        "colors: 000000, 000000, 000000, 000000\n";

    int subtitle_default = -1;
    for( ii = 0; ii < hb_list_count( job->list_subtitle ); ii++ )
    {
        hb_subtitle_t *subtitle = hb_list_item( job->list_subtitle, ii );

        if( subtitle->config.dest == PASSTHRUSUB )
        {
            if ( subtitle->config.default_track )
                subtitle_default = ii;
        }
    }
    // Quicktime requires that at least one subtitle is enabled,
    // else it doesn't show any of the subtitles.
    // So check to see if any of the subtitles are flagged to be
    // the defualt.  The default will the the enabled track, else
    // enable the first track.
    if (job->mux == HB_MUX_AV_MP4 && subtitle_default == -1)
    {
        subtitle_default = 0;
    }

    for( ii = 0; ii < hb_list_count( job->list_subtitle ); ii++ )
    {
        hb_subtitle_t * subtitle;
        uint32_t        rgb[16];
        char            subidx[2048];
        int             len;

        subtitle = hb_list_item( job->list_subtitle, ii );
        if (subtitle->config.dest != PASSTHRUSUB)
            continue;

        track = m->tracks[m->ntracks++] = calloc(1, sizeof( hb_mux_data_t ) );
        subtitle->mux_data = track;

        track->type = MUX_TYPE_SUBTITLE;
        track->st = avformat_new_stream(m->oc, NULL);
        if (track->st == NULL)
        {
            hb_error("Could not initialize subtitle stream");
            goto error;
        }
        avcodec_get_context_defaults3(track->st->codec, NULL);

        track->st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;
        track->st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        track->st->time_base = m->time_base;
        track->st->codec->time_base = m->time_base;
        track->st->codec->width = subtitle->width;
        track->st->codec->height = subtitle->height;

        priv_data = NULL;
        priv_size = 0;
        switch (subtitle->source)
        {
            case VOBSUB:
            {
                int jj;
                track->st->codec->codec_id = AV_CODEC_ID_DVD_SUBTITLE;

                for (jj = 0; jj < 16; jj++)
                    rgb[jj] = hb_yuv2rgb(subtitle->palette[jj]);
                len = snprintf(subidx, 2048, subidx_fmt,
                        subtitle->width, subtitle->height,
                        0, 0, "OFF",
                        rgb[0], rgb[1], rgb[2], rgb[3],
                        rgb[4], rgb[5], rgb[6], rgb[7],
                        rgb[8], rgb[9], rgb[10], rgb[11],
                        rgb[12], rgb[13], rgb[14], rgb[15]);

                priv_size = len + 1;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("VOBSUB extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data, subidx, priv_size);
            } break;

            case PGSSUB:
            {
                track->st->codec->codec_id = AV_CODEC_ID_HDMV_PGS_SUBTITLE;
            } break;

            case CC608SUB:
            case CC708SUB:
            case TX3GSUB:
            case SRTSUB:
            case UTF8SUB:
            case SSASUB:
            {
                if (job->mux == HB_MUX_AV_MP4)
                {
                    track->st->codec->codec_id = AV_CODEC_ID_MOV_TEXT;
                }
                else
                {
                    track->st->codec->codec_id = AV_CODEC_ID_SSA;
                    need_fonts = 1;

                    if (subtitle->extradata_size)
                    {
                        priv_size = subtitle->extradata_size;
                        priv_data = av_malloc(priv_size);
                        if (priv_data == NULL)
                        {
                            hb_error("SSA extradata: malloc failure");
                            goto error;
                        }
                        memcpy(priv_data, subtitle->extradata, priv_size);
                    }
                }
            } break;

            default:
                continue;
        }
        if (track->st->codec->codec_id == AV_CODEC_ID_MOV_TEXT)
        {
            // Build codec extradata for tx3g.
            // If we were using a libav codec to generate this data
            // this would (or should) be done for us.
            uint8_t properties[] = {
                0x00, 0x00, 0x00, 0x00,     // Display Flags
                0x01,                       // Horiz. Justification
                0xff,                       // Vert. Justification
                0x00, 0x00, 0x00, 0xff,     // Bg color
                0x00, 0x00, 0x00, 0x00,     // Default text box
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,     // Reserved
                0x00, 0x01,                 // Font ID
                0x00,                       // Font face
                0x18,                       // Font size
                0xff, 0xff, 0xff, 0xff,     // Fg color
                // Font table:
                0x00, 0x00, 0x00, 0x12,     // Font table size
                'f','t','a','b',            // Tag
                0x00, 0x01,                 // Count
                0x00, 0x01,                 // Font ID
                0x05,                       // Font name length
                'A','r','i','a','l'         // Font name
            };

            int width, height = 60;
            width = job->width * job->par.num / job->par.den;
            track->st->codec->width = width;
            track->st->codec->height = height;
            properties[14] = height >> 8;
            properties[15] = height & 0xff;
            properties[16] = width >> 8;
            properties[17] = width & 0xff;

            priv_size = sizeof(properties);
            priv_data = av_malloc(priv_size);
            if (priv_data == NULL)
            {
                hb_error("TX3G extradata: malloc failure");
                goto error;
            }
            memcpy(priv_data, properties, priv_size);
        }
        track->st->codec->extradata = priv_data;
        track->st->codec->extradata_size = priv_size;

        if (ii == subtitle_default)
        {
            track->st->disposition |= AV_DISPOSITION_DEFAULT;
        }
        if (subtitle->config.default_track)
        {
            track->st->disposition |= AV_DISPOSITION_FORCED;
        }

        lang = lookup_lang_code(job->mux, subtitle->iso639_2 );
        if (lang != NULL)
        {
            av_dict_set(&track->st->metadata, "language", lang, 0);
        }
    }

    if (need_fonts)
    {
        hb_list_t * list_attachment = job->list_attachment;
        int i;
        for ( i = 0; i < hb_list_count(list_attachment); i++ )
        {
            hb_attachment_t * attachment = hb_list_item( list_attachment, i );

            if ((attachment->type == FONT_TTF_ATTACH || attachment->type == FONT_OTF_ATTACH) &&
                attachment->size > 0)
            {
                AVStream *st = avformat_new_stream(m->oc, NULL);
                if (st == NULL)
                {
                    hb_error("Could not initialize attachment stream");
                    goto error;
                }
                avcodec_get_context_defaults3(st->codec, NULL);

                st->codec->codec_type = AVMEDIA_TYPE_ATTACHMENT;
                if (attachment->type == FONT_TTF_ATTACH)
                {
                    st->codec->codec_id = AV_CODEC_ID_TTF;
                }
                else if (attachment->type == FONT_OTF_ATTACH)
                {
                    st->codec->codec_id = MKBETAG( 0 ,'O','T','F');
                    av_dict_set(&st->metadata, "mimetype", "application/vnd.ms-opentype", 0);
                }

                priv_size = attachment->size;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("Font extradata: malloc failure");
                    goto error;
                }
                memcpy(priv_data, attachment->data, priv_size);

                st->codec->extradata = priv_data;
                st->codec->extradata_size = priv_size;

                av_dict_set(&st->metadata, "filename", attachment->name, 0);
            }
        }
    }

    if( job->metadata )
    {
        hb_metadata_t *md = job->metadata;

        hb_deep_log(2, "Writing Metadata to output file...");
        if (md->name &&
            metadata_keys[META_TITLE][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_TITLE][meta_mux], md->name, 0);
        }
        if (md->artist &&
            metadata_keys[META_ARTIST][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_ARTIST][meta_mux], md->artist, 0);
        }
        if (md->album_artist &&
            metadata_keys[META_DIRECTOR][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_DIRECTOR][meta_mux],
                        md->album_artist, 0);
        }
        if (md->composer &&
            metadata_keys[META_COMPOSER][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_COMPOSER][meta_mux],
                        md->composer, 0);
        }
        if (md->release_date &&
            metadata_keys[META_RELEASE_DATE][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_RELEASE_DATE][meta_mux],
                        md->release_date, 0);
        }
        if (md->comment &&
            metadata_keys[META_COMMENT][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_COMMENT][meta_mux], md->comment, 0);
        }
        if (!md->name && md->album &&
            metadata_keys[META_ALBUM][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_ALBUM][meta_mux], md->album, 0);
        }
        if (md->genre &&
            metadata_keys[META_GENRE][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_GENRE][meta_mux], md->genre, 0);
        }
        if (md->description &&
            metadata_keys[META_DESCRIPTION][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_DESCRIPTION][meta_mux],
                        md->description, 0);
        }
        if (md->long_description &&
            metadata_keys[META_SYNOPSIS][meta_mux] != NULL)
        {
            av_dict_set(&m->oc->metadata,
                        metadata_keys[META_SYNOPSIS][meta_mux],
                        md->long_description, 0);
        }
    }

    char tool_string[80];
    snprintf(tool_string, sizeof(tool_string), "HandBrake %s %i",
             HB_PROJECT_VERSION, HB_PROJECT_BUILD);
    av_dict_set(&m->oc->metadata, "encoding_tool", tool_string, 0);
    time_t now = time(NULL);
    struct tm * now_utc = gmtime(&now);
    char now_8601[24];
    strftime(now_8601, sizeof(now_8601), "%Y-%m-%dT%H:%M:%SZ", now_utc);
    av_dict_set(&m->oc->metadata, "creation_time", now_8601, 0);

    ret = avformat_write_header(m->oc, &av_opts);
    if( ret < 0 )
    {
        av_dict_free( &av_opts );
        hb_error( "muxavformat: avformat_write_header failed!");
        goto error;
    }

    AVDictionaryEntry *t = NULL;
    while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
    {
        hb_log( "muxavformat: Unknown option %s", t->key );
    }
    av_dict_free( &av_opts );

    return 0;

error:
    free(job->mux_data);
    job->mux_data = NULL;
    avformat_free_context(m->oc);
    *job->done_error = HB_ERROR_INIT;
    *job->die = 1;
    return -1;
}

static int add_chapter(hb_mux_object_t *m, int64_t start, int64_t end, char * title)
{
    AVChapter *chap;
    AVChapter **chapters;
    int nchap = m->oc->nb_chapters;

    nchap++;
    chapters = av_realloc(m->oc->chapters, nchap * sizeof(AVChapter*));
    if (chapters == NULL)
    {
        hb_error("chapter array: malloc failure");
        return -1;
    }

    chap = av_mallocz(sizeof(AVChapter));
    if (chap == NULL)
    {
        hb_error("chapter: malloc failure");
        return -1;
    }

    m->oc->chapters = chapters;
    m->oc->chapters[nchap-1] = chap;
    m->oc->nb_chapters = nchap;

    chap->id = nchap;
    chap->time_base = m->time_base;
    // libav does not currently have a good way to deal with chapters and
    // delayed stream timestamps.  It makes no corrections to the chapter 
    // track.  A patch to libav would touch a lot of things, so for now,
    // work around the issue here.
    chap->start = start + m->chapter_delay;
    chap->end = end;
    av_dict_set(&chap->metadata, "title", title, 0);

    return 0;
}

static int avformatMux(hb_mux_object_t *m, hb_mux_data_t *track, hb_buffer_t *buf)
{
    AVPacket pkt;
    int64_t dts, pts, duration = AV_NOPTS_VALUE;
    hb_job_t *job     = m->job;
    uint8_t sub_out[2048];

    if (track->type == MUX_TYPE_VIDEO &&
        track->prev_chapter_tc == AV_NOPTS_VALUE)
    {
        // Chapter timestamps are biased the same as video timestamps.
        // This needs to be reflected in the initial chapter timestamp.
        //
        // TODO: Don't assume the first chapter is at 0.  Pass the first
        // chapter through the pipeline instead of dropping it as we
        // currently do.
        m->chapter_delay = av_rescale_q(m->job->config.h264.init_delay,
                                        (AVRational){1,90000},
                                        track->st->time_base);
        track->prev_chapter_tc = -m->chapter_delay;
    }
    // We only compute dts duration for MP4 files
    if (track->type == MUX_TYPE_VIDEO && (job->mux & HB_MUX_MASK_MP4))
    {
        hb_buffer_t * tmp;

        // delay by one frame so that we can compute duration properly.
        tmp = track->delay_buf;
        track->delay_buf = buf;
        buf = tmp;
    }
    if (buf == NULL)
        return 0;

    if (buf->s.renderOffset == AV_NOPTS_VALUE)
    {
        dts = av_rescale_q(buf->s.start, (AVRational){1,90000},
                           track->st->time_base);
    }
    else
    {
        dts = av_rescale_q(buf->s.renderOffset, (AVRational){1,90000},
                           track->st->time_base);
    }

    pts = av_rescale_q(buf->s.start, (AVRational){1,90000},
                       track->st->time_base);

    if (track->type == MUX_TYPE_VIDEO && track->delay_buf != NULL)
    {
        int64_t delayed_dts;
        delayed_dts = av_rescale_q(track->delay_buf->s.renderOffset,
                                   (AVRational){1,90000},
                                   track->st->time_base);
        duration = delayed_dts - dts;
    }
    if (duration < 0 && buf->s.duration > 0)
    {
        duration = av_rescale_q(buf->s.duration, (AVRational){1,90000},
                                track->st->time_base);
    }
    if (duration < 0)
    {
        // There is a possiblility that some subtitles get through the pipeline
        // without ever discovering their true duration.  Make the duration
        // 10 seconds in this case. Unless they are PGS subs which should
        // have zero duration.
        if (track->type == MUX_TYPE_SUBTITLE &&
            track->st->codec->codec_id != AV_CODEC_ID_HDMV_PGS_SUBTITLE)
            duration = av_rescale_q(10, (AVRational){1,1},
                                    track->st->time_base);
        else
            duration = 0;
    }

    av_init_packet(&pkt);
    pkt.data = buf->data;
    pkt.size = buf->size;
    pkt.dts = dts;
    pkt.pts = pts;
    pkt.duration = duration;

    if (track->type == MUX_TYPE_VIDEO && ((job->vcodec & HB_VCODEC_H264_MASK) ||
                                          (job->vcodec & HB_VCODEC_H265_MASK) ||
                                          (job->vcodec & HB_VCODEC_FFMPEG_MASK)))
    {
        if (buf->s.frametype == HB_FRAME_IDR)
            pkt.flags |= AV_PKT_FLAG_KEY;
    }
    else if (buf->s.frametype & HB_FRAME_KEY)
    {
        pkt.flags |= AV_PKT_FLAG_KEY;
    }

    switch (track->type)
    {
        case MUX_TYPE_VIDEO:
        {
            if (job->chapter_markers && buf->s.new_chap)
            {
                hb_chapter_t *chapter;

                // reached chapter N, write marker for chapter N-1
                // we don't know the end time of chapter N-1 till we receive
                // chapter N.  So we are always writing the previous chapter
                // mark.
                track->current_chapter = buf->s.new_chap - 1;

                // chapter numbers start at 1, but the list starts at 0
                chapter = hb_list_item(job->list_chapter,
                                            track->current_chapter - 1);

                // make sure we're not writing a chapter that has 0 length
                if (chapter != NULL && track->prev_chapter_tc < pkt.pts)
                {
                    char title[1024];
                    if (chapter->title != NULL)
                    {
                        snprintf(title, 1023, "%s", chapter->title);
                    }
                    else
                    {
                        snprintf(title, 1023, "Chapter %d",
                                 track->current_chapter);
                    }
                    add_chapter(m, track->prev_chapter_tc, pkt.pts, title);
                }
                track->prev_chapter_tc = pkt.pts;
            }
        } break;

        case MUX_TYPE_SUBTITLE:
        {
            if (job->mux == HB_MUX_AV_MP4)
            {
                /* Write an empty sample */
                if ( track->duration < pts )
                {
                    AVPacket empty_pkt;
                    uint8_t empty[2] = {0,0};

                    av_init_packet(&empty_pkt);
                    empty_pkt.data = empty;
                    empty_pkt.size = 2;
                    empty_pkt.dts = track->duration;
                    empty_pkt.pts = track->duration;
                    empty_pkt.duration = pts - duration;
                    empty_pkt.convergence_duration = empty_pkt.duration;
                    empty_pkt.stream_index = track->st->index;
                    int ret = av_interleaved_write_frame(m->oc, &empty_pkt);
                    if (ret < 0)
                    {
                        char errstr[64];
                        av_strerror(ret, errstr, sizeof(errstr));
                        hb_error("avformatMux: track %d, av_interleaved_write_frame failed with error '%s' (empty_pkt)",
                                 track->st->index, errstr);
                        *job->done_error = HB_ERROR_UNKNOWN;
                        *job->die = 1;
                        return -1;
                    }
                }
                if (track->st->codec->codec_id == AV_CODEC_ID_MOV_TEXT)
                {
                    uint8_t styleatom[2048];;
                    uint16_t stylesize = 0;
                    uint8_t buffer[2048];
                    uint16_t buffersize = 0;

                    *buffer = '\0';

                    /*
                     * Copy the subtitle into buffer stripping markup and creating
                     * style atoms for them.
                     */
                    hb_muxmp4_process_subtitle_style( buf->data,
                                                      buffer,
                                                      styleatom, &stylesize );

                    buffersize = strlen((char*)buffer);

                    /* Write the subtitle sample */
                    memcpy( sub_out + 2, buffer, buffersize );
                    memcpy( sub_out + 2 + buffersize, styleatom, stylesize);
                    sub_out[0] = ( buffersize >> 8 ) & 0xff;
                    sub_out[1] = buffersize & 0xff;
                    pkt.data = sub_out;
                    pkt.size = buffersize + stylesize + 2;
                }
            }
            if (track->st->codec->codec_id == AV_CODEC_ID_SSA &&
                job->mux == HB_MUX_AV_MKV)
            {
                // avformat requires the this additional information
                // which it parses and then strips away
                int start_hh, start_mm, start_ss, start_ms;
                int stop_hh, stop_mm, stop_ss, stop_ms, layer;
                char *ssa;

                start_hh = buf->s.start / (90000 * 60 * 60);
                start_mm = (buf->s.start / (90000 * 60)) % 60;
                start_ss = (buf->s.start / 90000) % 60;
                start_ms = (buf->s.start / 900) % 100;
                stop_hh = buf->s.stop / (90000 * 60 * 60);
                stop_mm = (buf->s.stop / (90000 * 60)) % 60;
                stop_ss = (buf->s.stop / 90000) % 60;
                stop_ms = (buf->s.stop / 900) % 100;

                // Skip the read-order field
                ssa = strchr((char*)buf->data, ',');
                if (ssa != NULL)
                    ssa++;
                // Skip the layer field
                layer = strtol(ssa, NULL, 10);
                ssa = strchr(ssa, ',');
                if (ssa != NULL)
                    ssa++;
                sprintf((char*)sub_out,
                    "Dialogue: %d,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s",
                    layer,
                    start_hh, start_mm, start_ss, start_ms,
                    stop_hh, stop_mm, stop_ss, stop_ms, ssa);
                pkt.data = sub_out;
                pkt.size = strlen((char*)sub_out) + 1;
            }
            pkt.convergence_duration = pkt.duration;

        } break;
        case MUX_TYPE_AUDIO:
        default:
            break;
    }
    track->duration = pts + pkt.duration;

    if (track->bitstream_filter)
    {
        av_bitstream_filter_filter(track->bitstream_filter, track->st->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
    }

    pkt.stream_index = track->st->index;
    int ret = av_interleaved_write_frame(m->oc, &pkt);
    // Many avformat muxer functions do not check the error status
    // of the AVIOContext.  So we need to check it ourselves to detect
    // write errors (like disk full condition).
    if (ret < 0 || m->oc->pb->error != 0)
    {
        char errstr[64];
        av_strerror(ret < 0 ? ret : m->oc->pb->error, errstr, sizeof(errstr));
        hb_error("avformatMux: track %d, av_interleaved_write_frame failed with error '%s'",
                 track->st->index, errstr);
        *job->done_error = HB_ERROR_UNKNOWN;
        *job->die = 1;
        return -1;
    }

    hb_buffer_close( &buf );
    return 0;
}

static int avformatEnd(hb_mux_object_t *m)
{
    hb_job_t *job           = m->job;
    hb_mux_data_t *track = job->mux_data;

    if( !job->mux_data )
    {
        /*
         * We must have failed to create the file in the first place.
         */
        return 0;
    }

    // Flush any delayed frames
    int ii;
    for (ii = 0; ii < m->ntracks; ii++)
    {
        avformatMux(m, m->tracks[ii], NULL);

        if (m->tracks[ii]->bitstream_filter)
        {
            av_bitstream_filter_close(m->tracks[ii]->bitstream_filter);
        }
    }

    if (job->chapter_markers)
    {
        hb_chapter_t *chapter;

        // get the last chapter
        chapter = hb_list_item(job->list_chapter, track->current_chapter++);

        // only write the last chapter marker if it lasts at least 1.5 second
        if (chapter != NULL && chapter->duration > 135000LL)
        {
            char title[1024];
            if (chapter->title != NULL)
            {
                snprintf(title, 1023, "%s", chapter->title);
            }
            else
            {
                snprintf(title, 1023, "Chapter %d", track->current_chapter);
            }
            add_chapter(m, track->prev_chapter_tc, track->duration, title);
        }
    }

    // Update and track private data that can change during
    // encode.
    for(ii = 0; ii < hb_list_count( job->list_audio ); ii++)
    {
        AVStream *st;
        hb_audio_t    * audio;

        audio = hb_list_item(job->list_audio, ii);
        st = audio->priv.mux_data->st;

        switch (audio->config.out.codec & HB_ACODEC_MASK)
        {
            case HB_ACODEC_FFFLAC:
            case HB_ACODEC_FFFLAC24:
                if( audio->priv.config.extradata.length )
                {
                    uint8_t *priv_data;
                    int priv_size;

                    priv_size = audio->priv.config.extradata.length;
                    priv_data = av_realloc(st->codec->extradata, priv_size);
                    if (priv_data == NULL)
                    {
                        break;
                    }
                    memcpy(priv_data,
                           audio->priv.config.extradata.bytes,
                           audio->priv.config.extradata.length);
                    st->codec->extradata = priv_data;
                    st->codec->extradata_size = priv_size;
                }
                break;
            default:
                break;
        }
    }

    av_write_trailer(m->oc);
    avio_close(m->oc->pb);
    avformat_free_context(m->oc);
    free(m->tracks);
    m->oc = NULL;

    return 0;
}

hb_mux_object_t * hb_mux_avformat_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = avformatInit;
    m->mux       = avformatMux;
    m->end       = avformatEnd;
    m->job       = job;
    return m;
}
