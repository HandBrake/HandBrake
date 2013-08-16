/* muxavformat.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#if defined(USE_AVFORMAT)

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
};

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t          * job;

    AVFormatContext   * oc;
    AVRational          time_base;

    int                 ntracks;
    hb_mux_data_t    ** tracks;

    int                 delay;
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
    int ii, ret;

    const char *muxer_name = NULL;

    uint8_t         default_track_flag = 1;
    uint8_t         need_fonts = 0;
    char *lang;


    m->delay = -1;
    max_tracks = 1 + hb_list_count( job->list_audio ) +
                     hb_list_count( job->list_subtitle );

    m->tracks = calloc(max_tracks, sizeof(hb_mux_data_t*));

    m->oc = avformat_alloc_context();
    if (m->oc == NULL)
    {
        hb_error( "Could not initialize avformat context." );
        goto error;
    }

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
        case HB_VCODEC_X264:
            track->st->codec->codec_id = AV_CODEC_ID_H264;

            /* Taken from x264 muxers.c */
            priv_size = 5 + 1 + 2 + job->config.h264.sps_length + 1 + 2 +
                        job->config.h264.pps_length;
            priv_data = av_malloc(priv_size);
            if (priv_data == NULL)
            {
                hb_error("malloc failure");
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
                    hb_error("malloc failure");
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
                    hb_error("malloc failure");
                    goto error;
                }
                memcpy(priv_data, job->config.mpeg4.bytes, priv_size);
            }
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
                hb_error("malloc failure");
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

        default:
            hb_error("muxavformat: Unknown video codec: %x", job->vcodec);
            goto error;
    }
    track->st->codec->extradata = priv_data;
    track->st->codec->extradata_size = priv_size;

    track->st->codec->width = job->width;
    track->st->codec->height = job->height;
    track->st->sample_aspect_ratio.num = job->anamorphic.par_width;
    track->st->sample_aspect_ratio.den = job->anamorphic.par_height;
    track->st->codec->sample_aspect_ratio.num = job->anamorphic.par_width;
    track->st->codec->sample_aspect_ratio.den = job->anamorphic.par_height;
    track->st->disposition |= AV_DISPOSITION_DEFAULT;

    int vrate_base, vrate;
    if( job->pass == 2 )
    {
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        vrate_base = interjob->vrate_base;
        vrate = interjob->vrate;
    }
    else
    {
        vrate_base = job->vrate_base;
        vrate = job->vrate;
    }

    // If the vrate is 27000000, there's a good chance this is
    // a standard rate that we have in our hb_video_rates table.
    // Because of rounding errors and approximations made while
    // measuring framerate, the actual value may not be exact.  So
    // we look for rates that are "close" and make an adjustment
    // to fps.den.
    if (vrate == 27000000)
    {
        const hb_rate_t *video_framerate = NULL;
        while ((video_framerate = hb_video_framerate_get_next(video_framerate)) != NULL)
        {
            if (abs(vrate_base - video_framerate->rate) < 10)
            {
                vrate_base = video_framerate->rate;
                break;
            }
        }
    }
    hb_reduce(&vrate_base, &vrate, vrate_base, vrate);
    if (job->mux == HB_MUX_AV_MP4)
    {
        // libavformat mp4 muxer requires that the codec time_base have the
        // same denominator as the stream time_base, it uses it for the
        // mdhd timescale.
        double scale = (double)track->st->time_base.den / vrate;
        track->st->codec->time_base.den = track->st->time_base.den;
        track->st->codec->time_base.num = vrate_base * scale;
    }
    else
    {
        track->st->codec->time_base.num = vrate_base;
        track->st->codec->time_base.den = vrate;
    }

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
                    hb_error("malloc failure");
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

                if (audio->priv.config.extradata.bytes)
                {
                    priv_size = audio->priv.config.extradata.length;
                    priv_data = av_malloc(priv_size);
                    if (priv_data == NULL)
                    {
                        hb_error("malloc failure");
                        goto error;
                    }
                    memcpy(priv_data,
                           audio->priv.config.extradata.bytes,
                           audio->priv.config.extradata.length);
                }
                break;
            case HB_ACODEC_FAAC:
            case HB_ACODEC_FFAAC:
            case HB_ACODEC_CA_AAC:
            case HB_ACODEC_CA_HAAC:
            case HB_ACODEC_FDK_AAC:
            case HB_ACODEC_FDK_HAAC:
                track->st->codec->codec_id = AV_CODEC_ID_AAC;

                if (audio->priv.config.extradata.bytes)
                {
                    priv_size = audio->priv.config.extradata.length;
                    priv_data = av_malloc(priv_size);
                    if (priv_data == NULL)
                    {
                        hb_error("malloc failure");
                        goto error;
                    }
                    memcpy(priv_data,
                           audio->priv.config.extradata.bytes,
                           audio->priv.config.extradata.length);
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
        av_dict_set(&track->st->metadata, "title", name, 0);
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
                    hb_error("malloc failure");
                    goto error;
                }
                memcpy(priv_data, subidx, priv_size);
            } break;

            case PGSSUB:
            {
                track->st->codec->codec_id = AV_CODEC_ID_HDMV_PGS_SUBTITLE;
            } break;

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
                            hb_error("malloc failure");
                            goto error;
                        }
                        memcpy(priv_data, subtitle->extradata, priv_size);
                    }
                }
            } break;

            case CC608SUB:
            case CC708SUB:
            case UTF8SUB:
            case TX3GSUB:
            case SRTSUB:
            {
                if (job->mux == HB_MUX_AV_MP4)
                    track->st->codec->codec_id = AV_CODEC_ID_MOV_TEXT;
                else
                    track->st->codec->codec_id = AV_CODEC_ID_TEXT;
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
            if (job->anamorphic.mode)
                width = job->width * ((float)job->anamorphic.par_width / job->anamorphic.par_height);
            else
                width = job->width;
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
                hb_error("malloc failure");
                goto error;
            }
            memcpy(priv_data, properties, priv_size);
        }
        track->st->codec->extradata = priv_data;
        track->st->codec->extradata_size = priv_size;

        if ( ii == subtitle_default )
        {
            track->st->disposition |= AV_DISPOSITION_DEFAULT;
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

            if (attachment->type == FONT_TTF_ATTACH &&
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
                track->st->codec->codec_id = AV_CODEC_ID_TTF;

                priv_size = attachment->size;
                priv_data = av_malloc(priv_size);
                if (priv_data == NULL)
                {
                    hb_error("malloc failure");
                    goto error;
                }
                memcpy(priv_data, attachment->data, priv_size);

                track->st->codec->extradata = priv_data;
                track->st->codec->extradata_size = priv_size;

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

    AVDictionary * av_opts = NULL;
    if (job->mp4_optimize && (job->mux & HB_MUX_MASK_MP4))
        av_dict_set( &av_opts, "movflags", "faststart", 0 );

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
        hb_error("malloc failure");
        return -1;
    }

    chap = av_mallocz(sizeof(AVChapter));
    if (chap == NULL)
    {
        hb_error("malloc failure");
        return -1;
    }

    m->oc->chapters = chapters;
    m->oc->chapters[nchap-1] = chap;
    m->oc->nb_chapters = nchap;

    chap->id = nchap;
    chap->time_base = m->time_base;
    chap->start = start;
    chap->end = end;
    av_dict_set(&chap->metadata, "title", title, 0);

    return 0;
}

// Video with b-frames and certain audio types require a lead-in delay.
// Compute the max delay and offset all timestamps by this amount.
//
// For mp4, avformat will automatically put entries in the edts atom
// to account for the offset of the first dts in each track.
static void computeDelay(hb_mux_object_t *m)
{
    int ii;
    hb_audio_t    * audio;

    m->delay = m->job->config.h264.init_delay;
    for(ii = 0; ii < hb_list_count( m->job->list_audio ); ii++ )
    {
        audio = hb_list_item( m->job->list_audio, ii );
        if (audio->config.out.delay > m->delay)
            m->delay = audio->config.out.delay;
    }
}

static int avformatMux(hb_mux_object_t *m, hb_mux_data_t *track, hb_buffer_t *buf)
{
    AVPacket pkt;
    int64_t dts, pts, duration = -1;
    hb_job_t *job     = m->job;
    uint8_t tx3g_out[2048];

    if (m->delay == -1)
    {
        computeDelay(m);
    }

    if (buf != NULL)
    {
        if (buf->s.start != -1)
            buf->s.start += m->delay;
        if (buf->s.renderOffset != -1)
            buf->s.renderOffset += m->delay;
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

    if (buf->s.renderOffset == -1)
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
        // 10 seconds in this case.
        if (track->type == MUX_TYPE_SUBTITLE)
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

    if (track->type == MUX_TYPE_VIDEO &&
        (job->vcodec == HB_VCODEC_X264 || job->vcodec & HB_VCODEC_FFMPEG_MASK))
    {
        if (buf->s.frametype == HB_FRAME_IDR)
            pkt.flags |= AV_PKT_FLAG_KEY;
    }
    else if (buf->s.frametype & HB_FRAME_KEY)
    {
        pkt.flags |= AV_PKT_FLAG_KEY;
    }

    track->duration += pkt.duration;

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
                        hb_error("av_interleaved_write_frame failed!");
                        *job->die = 1;
                        return -1;
                    }
                    track->duration = pts;
                }
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
                memcpy( tx3g_out + 2, buffer, buffersize );
                memcpy( tx3g_out + 2 + buffersize, styleatom, stylesize);
                tx3g_out[0] = ( buffersize >> 8 ) & 0xff;
                tx3g_out[1] = buffersize & 0xff;
                pkt.data = tx3g_out;
                pkt.size = buffersize + stylesize + 2;
            }
            pkt.convergence_duration = pkt.duration;

        } break;
        case MUX_TYPE_AUDIO:
        default:
            break;
    }

    pkt.stream_index = track->st->index;
    int ret = av_interleaved_write_frame(m->oc, &pkt);
    if (ret < 0)
    {
        hb_error("av_interleaved_write_frame failed!");
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
                if( audio->priv.config.extradata.bytes )
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

#endif // USE_AVFORMAT
