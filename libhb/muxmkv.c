/* muxmkv.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* libmkv header */

#if defined(USE_LIBMKV)

#include "libmkv.h"

#include <ogg/ogg.h>

#include "hb.h"
#include "lang.h"

/* Scale factor to apply to timecodes to convert from HandBrake's
 * 1/90000s to nanoseconds as expected by libmkv */
#define NANOSECOND_SCALE 1000000000L
#define TIMECODE_SCALE 1000000000L / 90000

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t  * job;
    mk_Writer * file;
    int         delay;
};

struct hb_mux_data_s
{
    mk_Track  * track;
    uint64_t  prev_chapter_tc;
    uint16_t  current_chapter;
    int       codec;
    int       subtitle;
    int       sub_format;
};

static uint8_t * create_flac_header( uint8_t *data, int size )
{
    uint8_t * out;
    uint8_t header[8] = {
        0x66, 0x4C, 0x61, 0x43, 0x80, 0x00, 0x00, 0x22
    };

    out = malloc( size + 8 );
    memcpy( out, header, 8 );
    memcpy( out + 8, data, size );
    return out;
}

static uint8_t* create_h264_header(hb_job_t *job, int *size)
{
    /* Taken from x264's muxers.c */
    int avcC_len  = (5 +
                     1 + 2 + job->config.h264.sps_length +
                     1 + 2 + job->config.h264.pps_length);
#define MAX_AVCC_LEN 5 + 1 + 2 + 1024 + 1 + 2 + 1024 // FIXME
    if (avcC_len > MAX_AVCC_LEN)
    {
        hb_log("create_h264_header: H.264 header too long (%d, max: %d)",
               avcC_len, MAX_AVCC_LEN);
        return NULL;
    }
    uint8_t *avcC = malloc(avcC_len);
    if (avcC == NULL)
    {
        return NULL;
    }

    avcC[0] = 1;
    avcC[1] = job->config.h264.sps[1]; /* AVCProfileIndication */
    avcC[2] = job->config.h264.sps[2]; /* profile_compat */
    avcC[3] = job->config.h264.sps[3]; /* AVCLevelIndication */
    avcC[4] = 0xff; // nalu size length is four bytes
    avcC[5] = 0xe1; // one sps

    avcC[6] = job->config.h264.sps_length >> 8;
    avcC[7] = job->config.h264.sps_length;
    memcpy(avcC + 8, job->config.h264.sps, job->config.h264.sps_length);

    avcC[8  + job->config.h264.sps_length] = 1; // one pps
    avcC[9  + job->config.h264.sps_length] = job->config.h264.pps_length >> 8;
    avcC[10 + job->config.h264.sps_length] = job->config.h264.pps_length;
    memcpy(avcC + 11 + job->config.h264.sps_length,
           job->config.h264.pps, job->config.h264.pps_length);

    if (size != NULL)
    {
        *size = avcC_len;
    }
    return avcC;
}

/**********************************************************************
 * MKVInit
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int MKVInit( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;

    uint8_t         *avcC = NULL;
    uint8_t         default_track_flag = 1;
    uint8_t         need_fonts = 0;
    int             avcC_len, i, j;
    ogg_packet      *ogg_headers[3];
    mk_TrackConfig  *track;
    iso639_lang_t   *lang;

    track = calloc(1, sizeof(mk_TrackConfig));

    m->file = mk_createWriter(job->file, 1000000, 1);

    if( !m->file )
    {
        hb_error( "Could not create output file, Disk Full?" );
        job->mux_data = NULL;
        *job->die = 1;
        free(track);
        return 0;
    }

    /* Video track */
    mux_data      = calloc(1, sizeof( hb_mux_data_t ) );
    job->mux_data = mux_data;

    track->trackType = MK_TRACK_VIDEO;
    track->flagDefault = 1;
    track->flagEnabled = 1;
    switch (job->vcodec)
    {
        case HB_VCODEC_X264:
        case HB_VCODEC_QSV_H264:
            avcC = create_h264_header(job, &avcC_len);
            if (avcC == NULL)
            {
                free(track);
                return -1;
            }
            track->codecID          = MK_VCODEC_MP4AVC;
            track->codecPrivate     = avcC;
            track->codecPrivateSize = avcC_len;
            if (job->areBframes)
                track->minCache = 1;
            break;
        case HB_VCODEC_FFMPEG_MPEG4:
            track->codecID = MK_VCODEC_MP4ASP;
            track->codecPrivate = job->config.mpeg4.bytes;
            track->codecPrivateSize = job->config.mpeg4.length;
            if (job->areBframes)
                track->minCache = 1;
            break;
        case HB_VCODEC_FFMPEG_MPEG2:
            track->codecID = MK_VCODEC_MPEG2;
            track->codecPrivate = job->config.mpeg4.bytes;
            track->codecPrivateSize = job->config.mpeg4.length;
            if (job->areBframes)
                track->minCache = 1;
            break;
        case HB_VCODEC_THEORA:
            {
                int i;
                uint64_t cp_size = 0;
                track->codecID = MK_VCODEC_THEORA;
                uint64_t  header_sizes[3];
                for (i = 0; i < 3; ++i)
                {
                    ogg_headers[i] = (ogg_packet *)job->config.theora.headers[i];
                    ogg_headers[i]->packet = (unsigned char *)&job->config.theora.headers[i] + sizeof( ogg_packet );
                    header_sizes[i] = ogg_headers[i]->bytes;
                }
                track->codecPrivate = mk_laceXiph(header_sizes, 2, &cp_size);
                track->codecPrivate = realloc(track->codecPrivate, cp_size + ogg_headers[0]->bytes + ogg_headers[1]->bytes + ogg_headers[2]->bytes);
                for(i = 0; i < 3; ++i)
                {
                    memcpy(track->codecPrivate + cp_size, ogg_headers[i]->packet, ogg_headers[i]->bytes);
                    cp_size += ogg_headers[i]->bytes;
                }
                track->codecPrivateSize = cp_size;
            }
            break;
        default:
            *job->die = 1;
            hb_error("muxmkv: Unknown video codec: %x", job->vcodec);
            free(track);
            return 0;
    }

    track->extra.video.pixelWidth = job->width;
    track->extra.video.pixelHeight = job->height;
    track->extra.video.displayHeight = job->height;
    if( job->anamorphic.mode )
    {
        track->extra.video.displayWidth = job->width * ((double)job->anamorphic.par_width / (double)job->anamorphic.par_height);
    }
    else
    {
        track->extra.video.displayWidth = job->width;
    }

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
    track->defaultDuration = (int64_t)(((float)vrate_base / (float)vrate) * 1000000000);

    mux_data->track = mk_createTrack(m->file, track);

    /* add the audio tracks */
    for( i = 0; i < hb_list_count( job->list_audio ); i++ )
    {
        audio = hb_list_item( job->list_audio, i );
        mux_data = calloc(1, sizeof( hb_mux_data_t ) );
        audio->priv.mux_data = mux_data;

        if (audio->config.out.delay > m->delay)
            m->delay = audio->config.out.delay;

        mux_data->codec = audio->config.out.codec;

        memset(track, 0, sizeof(mk_TrackConfig));
        switch (audio->config.out.codec & HB_ACODEC_MASK)
        {
            case HB_ACODEC_DCA:
            case HB_ACODEC_DCA_HD:
                track->codecPrivate = NULL;
                track->codecPrivateSize = 0;
                track->codecID = MK_ACODEC_DTS;
                break;
            case HB_ACODEC_AC3:
                track->codecPrivate = NULL;
                track->codecPrivateSize = 0;
                track->codecID = MK_ACODEC_AC3;
                break;
            case HB_ACODEC_LAME:
            case HB_ACODEC_MP3:
                track->codecPrivate = NULL;
                track->codecPrivateSize = 0;
                track->codecID = MK_ACODEC_MP3;
                break;
            case HB_ACODEC_VORBIS:
                {
                    int i;
                    uint64_t cp_size = 0;
                    track->codecID = MK_ACODEC_VORBIS;
                    uint64_t  header_sizes[3];
                    for (i = 0; i < 3; ++i)
                    {
                        ogg_headers[i] = (ogg_packet *)audio->priv.config.vorbis.headers[i];
                        ogg_headers[i]->packet = (unsigned char *)&audio->priv.config.vorbis.headers[i] + sizeof( ogg_packet );
                        header_sizes[i] = ogg_headers[i]->bytes;
                    }
                    track->codecPrivate = mk_laceXiph(header_sizes, 2, &cp_size);
                    track->codecPrivate = realloc(track->codecPrivate, cp_size + ogg_headers[0]->bytes + ogg_headers[1]->bytes + ogg_headers[2]->bytes);
                    for(i = 0; i < 3; ++i)
                    {
                        memcpy(track->codecPrivate + cp_size, ogg_headers[i]->packet, ogg_headers[i]->bytes);
                        cp_size += ogg_headers[i]->bytes;
                    }
                    track->codecPrivateSize = cp_size;
                }
                break;
            case HB_ACODEC_FFFLAC:
            case HB_ACODEC_FFFLAC24:
                if (audio->priv.config.extradata.bytes)
                {
                    track->codecPrivate = create_flac_header(audio->priv.config.extradata.bytes,
                                                             audio->priv.config.extradata.length);
                    track->codecPrivateSize = audio->priv.config.extradata.length + 8;
                }
                track->codecID = MK_ACODEC_FLAC;
                break;
            case HB_ACODEC_FAAC:
            case HB_ACODEC_FFAAC:
            case HB_ACODEC_CA_AAC:
            case HB_ACODEC_CA_HAAC:
            case HB_ACODEC_FDK_AAC:
            case HB_ACODEC_FDK_HAAC:
                track->codecPrivate = audio->priv.config.extradata.bytes;
                track->codecPrivateSize = audio->priv.config.extradata.length;
                track->codecID = MK_ACODEC_AAC;
                break;
            default:
                *job->die = 1;
                hb_error("muxmkv: Unknown audio codec: %x", audio->config.out.codec);
                return 0;
        }

        if( default_track_flag )
        {
            track->flagDefault = 1;
            default_track_flag = 0;
        }
        else
        {
            track->flagDefault = 0;
        }
        track->flagEnabled = 1;
        track->trackType = MK_TRACK_AUDIO;
        // MKV lang codes should be ISO-639-2/B
        lang =  lang_for_code2( audio->config.lang.iso639_2 );
        track->language = lang->iso639_2b ? lang->iso639_2b : lang->iso639_2;
        // sample rate
        if ((audio->config.out.codec == HB_ACODEC_CA_HAAC)  ||
            (audio->config.out.codec == HB_ACODEC_FDK_HAAC) ||
            (audio->config.out.codec == HB_ACODEC_AAC_PASS &&
             audio->config.in.samples_per_frame > 1024))
        {
            // For HE-AAC, write outputSamplingFreq too
            // samplingFreq is half of outputSamplingFreq
            track->extra.audio.outputSamplingFreq = (float)audio->config.out.samplerate;
            track->extra.audio.samplingFreq = track->extra.audio.outputSamplingFreq / 2.;
        }
        else
        {
            track->extra.audio.samplingFreq = (float)audio->config.out.samplerate;
        }
        if (audio->config.out.codec & HB_ACODEC_PASS_FLAG)
        {
            track->extra.audio.channels = av_get_channel_layout_nb_channels(audio->config.in.channel_layout);
        }
        else
        {
            track->extra.audio.channels = hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);
        }
        mux_data->track = mk_createTrack(m->file, track);
        if (audio->config.out.codec == HB_ACODEC_VORBIS ||
            audio->config.out.codec == HB_ACODEC_FFFLAC ||
            audio->config.out.codec == HB_ACODEC_FFFLAC24)
            free(track->codecPrivate);
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

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        hb_subtitle_t * subtitle;
        uint32_t        rgb[16];
        char            subidx[2048];
        int             len;

        subtitle = hb_list_item( job->list_subtitle, i );
        if (subtitle->config.dest != PASSTHRUSUB)
            continue;

        memset(track, 0, sizeof(mk_TrackConfig));
        switch (subtitle->source)
        {
            case VOBSUB:
                track->codecID = MK_SUBTITLE_VOBSUB;
                for (j = 0; j < 16; j++)
                    rgb[j] = hb_yuv2rgb(subtitle->palette[j]);
                len = snprintf(subidx, 2048, subidx_fmt, 
                        subtitle->width, subtitle->height,
                        0, 0, "OFF",
                        rgb[0], rgb[1], rgb[2], rgb[3],
                        rgb[4], rgb[5], rgb[6], rgb[7],
                        rgb[8], rgb[9], rgb[10], rgb[11],
                        rgb[12], rgb[13], rgb[14], rgb[15]);
                track->codecPrivate = subidx;
                track->codecPrivateSize = len + 1;
                break;
            case PGSSUB:
                track->codecPrivate = NULL;
                track->codecPrivateSize = 0;
                track->codecID = MK_SUBTITLE_PGS;
                break;
            case SSASUB:
                track->codecID = MK_SUBTITLE_SSA;
                need_fonts = 1;
                track->codecPrivate = subtitle->extradata;
                track->codecPrivateSize = subtitle->extradata_size;
                break;
            case CC608SUB:
            case CC708SUB:
            case UTF8SUB:
            case TX3GSUB:
            case SRTSUB:
                track->codecPrivate = NULL;
                track->codecPrivateSize = 0;
                track->codecID = MK_SUBTITLE_UTF8;
                break;
            default:
                continue;
        }
        if ( subtitle->config.default_track )
        {
            track->flagDefault = 1;
        }

        mux_data = calloc(1, sizeof( hb_mux_data_t ) );
        subtitle->mux_data = mux_data;
        mux_data->subtitle = 1;
        mux_data->sub_format = subtitle->format;

        track->flagEnabled = 1;
        track->trackType = MK_TRACK_SUBTITLE;
        // MKV lang codes should be ISO-639-2/B
        lang =  lang_for_code2( subtitle->iso639_2 );
        track->language = lang->iso639_2b ? lang->iso639_2b : lang->iso639_2;

        mux_data->track = mk_createTrack(m->file, track);
    }

    if (need_fonts)
    {
        hb_list_t * list_attachment = job->list_attachment;
        int i;
        for ( i = 0; i < hb_list_count(list_attachment); i++ )
        {
            hb_attachment_t * attachment = hb_list_item( list_attachment, i );

            if ( attachment->type == FONT_TTF_ATTACH )
            {
                mk_createAttachment(
                    m->file,
                    attachment->name,
                    NULL,
                    "application/x-truetype-font",
                    attachment->data,
                    attachment->size);
            }
        }
    }

    if( mk_writeHeader( m->file, "HandBrake " HB_PROJECT_VERSION) < 0 )
    {
        hb_error( "Failed to write to output file, disk full?");
        *job->die = 1;
    }
    if (track != NULL)
        free(track);
    if (avcC != NULL)
        free(avcC);

    return 0;
}

static int MKVMux(hb_mux_object_t *m, hb_mux_data_t *mux_data, hb_buffer_t *buf)
{
    char chapter_name[1024];
    hb_chapter_t *chapter_data;
    uint64_t timecode = 0;
    hb_job_t *job     = m->job;

    // Adjust for audio preroll and scale units
    timecode = (buf->s.start + m->delay) * TIMECODE_SCALE;
    if (mux_data == job->mux_data)
    {
        /* Video */
        if (job->chapter_markers && buf->s.new_chap)
        {
            // reached chapter N, write marker for chapter N-1
            mux_data->current_chapter = buf->s.new_chap - 1;

            // chapter numbers start at 1, but the list starts at 0
            chapter_data = hb_list_item(job->list_chapter,
                                        mux_data->current_chapter - 1);

            // make sure we're not writing a chapter that has 0 length
            if (chapter_data != NULL && mux_data->prev_chapter_tc < timecode)
            {
                if (chapter_data->title != NULL)
                {
                    snprintf(chapter_name, 1023, "%s", chapter_data->title);
                }
                else
                {
                    snprintf(chapter_name, 1023, "Chapter %d",
                             mux_data->current_chapter);
                }
                mk_createChapterSimple(m->file,
                                       mux_data->prev_chapter_tc,
                                       mux_data->prev_chapter_tc, chapter_name);
            }
            mux_data->prev_chapter_tc = timecode;
        }
    }
    else if (mux_data->subtitle)
    {
        if( mk_startFrame(m->file, mux_data->track) < 0)
        {
            hb_error("Failed to write frame to output file, Disk Full?");
            *job->die = 1;
        }
        uint64_t duration;
        if (buf->s.duration < 0)
        {
            duration = 10 * NANOSECOND_SCALE;
        }
        else
        {
            duration = buf->s.duration * TIMECODE_SCALE;
        }
        mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
        mk_setFrameFlags(m->file, mux_data->track, timecode, 1, duration);
        mk_flushFrame(m->file, mux_data->track);
        hb_buffer_close(&buf);
        return 0;
    }
    else
    {
        /* Audio */
    }

    if( mk_startFrame(m->file, mux_data->track) < 0)
    {
        hb_error( "Failed to write frame to output file, Disk Full?" );
        *job->die = 1;
    }
    mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
    mk_setFrameFlags(m->file, mux_data->track, timecode,
                     ((mux_data == job->mux_data &&
                       ((job->vcodec & HB_VCODEC_H264_MASK) ||
                        (job->vcodec & HB_VCODEC_FFMPEG_MASK))) ?
                      (buf->s.frametype == HB_FRAME_IDR)        :
                      (buf->s.frametype  & HB_FRAME_KEY) != 0), 0);
    hb_buffer_close(&buf);
    return 0;
}

static int MKVEnd(hb_mux_object_t *m)
{
    char chapter_name[1024];
    hb_chapter_t *chapter_data;
    hb_job_t *job           = m->job;
    hb_mux_data_t *mux_data = job->mux_data;

    if( !job->mux_data )
    {
        /*
         * We must have failed to create the file in the first place.
         */
        return 0;
    }

    if (job->chapter_markers)
    {
        // get the last chapter
        chapter_data = hb_list_item(job->list_chapter,
                                    mux_data->current_chapter++);

        // only write the last chapter marker if it lasts at least 1.5 second
        if (chapter_data != NULL && chapter_data->duration > 135000LL)
        {
            if (chapter_data->title != NULL)
            {
                snprintf(chapter_name, 1023, "%s", chapter_data->title);
            }
            else
            {
                snprintf(chapter_name, 1023, "Chapter %d",
                         mux_data->current_chapter);
            }
            mk_createChapterSimple(m->file,
                                   mux_data->prev_chapter_tc,
                                   mux_data->prev_chapter_tc, chapter_name);
        }
    }

    if( job->metadata )
    {
        hb_metadata_t *md = job->metadata;

        hb_deep_log( 2, "Writing Metadata to output file...");
        if ( md->name )
        {
            mk_createTagSimple( m->file, MK_TAG_TITLE, md->name );
        }
        if ( md->artist )
        {
            mk_createTagSimple( m->file, "ARTIST", md->artist );
        }
        if ( md->album_artist )
        {
            mk_createTagSimple( m->file, "DIRECTOR", md->album_artist );
        }
        if ( md->composer )
        {
            mk_createTagSimple( m->file, "COMPOSER", md->composer );
        }
        if ( md->release_date )
        {
            mk_createTagSimple( m->file, "DATE_RELEASED", md->release_date );
        }
        if ( md->comment )
        {
            mk_createTagSimple( m->file, "SUMMARY", md->comment );
        }
        if ( !md->name && md->album )
        {
            mk_createTagSimple( m->file, MK_TAG_TITLE, md->album );
        }
        if ( md->genre )
        {
            mk_createTagSimple( m->file, MK_TAG_GENRE, md->genre );
        }
        if ( md->description )
        {
            mk_createTagSimple( m->file, "DESCRIPTION", md->description );
        }
        if ( md->long_description )
        {
            mk_createTagSimple( m->file, "SYNOPSIS", md->long_description );
        }
    }

    // Update and track private data that can change during
    // encode.
    int i;
    for( i = 0; i < hb_list_count( job->list_audio ); i++ )
    {
        mk_Track  * track;
        hb_audio_t    * audio;

        audio = hb_list_item( job->list_audio, i );
        track = audio->priv.mux_data->track;

        switch (audio->config.out.codec & HB_ACODEC_MASK)
        {
            case HB_ACODEC_FFFLAC:
            case HB_ACODEC_FFFLAC24:
                if( audio->priv.config.extradata.bytes )
                {
                    uint8_t *header;
                    header = create_flac_header( 
                            audio->priv.config.extradata.bytes,
                            audio->priv.config.extradata.length );
                    mk_updateTrackPrivateData( m->file, track,
                        header,
                        audio->priv.config.extradata.length + 8 );
                    free( header );
                }
                break;
            default:
                break;
        }
    }

    if( mk_close(m->file) < 0 )
    {
        hb_error( "Failed to flush the last frame and close the output file, Disk Full?" );
        *job->die = 1;
    }

    // TODO: Free what we alloc'd

    return 0;
}

hb_mux_object_t * hb_mux_mkv_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = MKVInit;
    m->mux       = MKVMux;
    m->end       = MKVEnd;
    m->job       = job;
    return m;
}
#endif // USE_LIBMKV
