/* $Id:  $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/* libmkv header */
#include "libmkv.h"

#include <ogg/ogg.h>

#include "hb.h"

/* Scale factor to apply to timecodes to convert from HandBrake's
 * 1/90000s to nanoseconds as expected by libmkv */
#define TIMECODE_SCALE 1000000000 / 90000

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    mk_Writer * file;
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

static int yuv2rgb(int yuv)
{
    double y, Cr, Cb;
    int r, g, b;

    y =  (yuv >> 16) & 0xff;
    Cb = (yuv >>  8) & 0xff;
    Cr = (yuv      ) & 0xff;

    r = 1.164 * (y - 16)                      + 2.018 * (Cb - 128);
    g = 1.164 * (y - 16) - 0.813 * (Cr - 128) - 0.391 * (Cb - 128);
    b = 1.164 * (y - 16) + 1.596 * (Cr - 128);
    r = (r < 0) ? 0 : r;
    g = (g < 0) ? 0 : g;
    b = (b < 0) ? 0 : b;
    r = (r > 255) ? 255 : r;
    g = (g > 255) ? 255 : g;
    b = (b > 255) ? 255 : b;
    return (r << 16) | (g << 8) | b;
}

/**********************************************************************
 * MKVInit
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int MKVInit( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;
    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;

    uint8_t         *avcC = NULL;
    uint8_t         default_track_flag = 1;
    int             avcC_len, i, j;
    ogg_packet      *ogg_headers[3];
    mk_TrackConfig *track;

    track = calloc(1, sizeof(mk_TrackConfig));

    m->file = mk_createWriter(job->file, 1000000, 1);

    if( !m->file )
    {
        hb_error( "Could not create output file, Disk Full?" );
        job->mux_data = NULL;
        *job->die = 1;
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
            track->codecID = MK_VCODEC_MP4AVC;
            /* Taken from x264 muxers.c */
            avcC_len = 5 + 1 + 2 + job->config.h264.sps_length + 1 + 2 + job->config.h264.pps_length;
            avcC = malloc(avcC_len);
            if (avcC == NULL)
                return -1;

            avcC[0] = 1;
            avcC[1] = job->config.h264.sps[1];      /* AVCProfileIndication */
            avcC[2] = job->config.h264.sps[2];      /* profile_compat */
            avcC[3] = job->config.h264.sps[3];      /* AVCLevelIndication */
            avcC[4] = 0xff; // nalu size length is four bytes
            avcC[5] = 0xe1; // one sps

            avcC[6] = job->config.h264.sps_length >> 8;
            avcC[7] = job->config.h264.sps_length;

            memcpy(avcC+8, job->config.h264.sps, job->config.h264.sps_length);

            avcC[8+job->config.h264.sps_length] = 1; // one pps
            avcC[9+job->config.h264.sps_length] = job->config.h264.pps_length >> 8;
            avcC[10+job->config.h264.sps_length] = job->config.h264.pps_length;

            memcpy( avcC+11+job->config.h264.sps_length, job->config.h264.pps, job->config.h264.pps_length );
            track->codecPrivate = avcC;
            track->codecPrivateSize = avcC_len;
            if (job->areBframes)
                track->minCache = 1;
            break;
        case HB_VCODEC_FFMPEG:
            track->codecID = MK_VCODEC_MP4ASP;
            track->codecPrivate = job->config.mpeg4.bytes;
            track->codecPrivateSize = job->config.mpeg4.length;
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


    track->defaultDuration = (int64_t)(((float)job->vrate_base / (float)job->vrate) * 1000000000);

    mux_data->track = mk_createTrack(m->file, track);

    memset(track, 0, sizeof(mk_TrackConfig));

    /* add the audio tracks */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        mux_data = calloc(1, sizeof( hb_mux_data_t ) );
        audio->priv.mux_data = mux_data;

        mux_data->codec = audio->config.out.codec;

        switch (audio->config.out.codec)
        {
            case HB_ACODEC_DCA:
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
            case HB_ACODEC_FAAC:
            case HB_ACODEC_CA_AAC:
                track->codecPrivate = audio->priv.config.aac.bytes;
                track->codecPrivateSize = audio->priv.config.aac.length;
                track->codecID = MK_ACODEC_AAC;
                break;
            default:
                *job->die = 1;
                hb_error("muxmkv: Unknown audio codec: %x", audio->config.out.codec);
                return 0;
        }

        if (default_track_flag)
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
        track->language = audio->config.lang.iso639_2;
        track->extra.audio.samplingFreq = (float)audio->config.out.samplerate;
        if (audio->config.out.codec == HB_ACODEC_AC3 ||
            audio->config.out.codec == HB_ACODEC_DCA)
        {
            track->extra.audio.channels = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(audio->config.in.channel_layout);
        }
        else
        {
            track->extra.audio.channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);
        }
//        track->defaultDuration = job->arate * 1000;
        mux_data->track = mk_createTrack(m->file, track);
        if (audio->config.out.codec == HB_ACODEC_VORBIS && track->codecPrivate != NULL)
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

    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        hb_subtitle_t * subtitle;
        uint32_t        rgb[16];
        char            subidx[2048];
        int             len;

        subtitle = hb_list_item( title->list_subtitle, i );
        if (subtitle->config.dest != PASSTHRUSUB)
            continue;

        memset(track, 0, sizeof(mk_TrackConfig));
        switch (subtitle->format)
        {
            case PICTURESUB:
                track->codecID = MK_SUBTITLE_VOBSUB;
                for (j = 0; j < 16; j++)
                    rgb[j] = yuv2rgb(title->palette[j]);
                len = snprintf(subidx, 2048, subidx_fmt, 
                        title->width, title->height,
                        0, 0, "OFF",
                        rgb[0], rgb[1], rgb[2], rgb[3],
                        rgb[4], rgb[5], rgb[6], rgb[7],
                        rgb[8], rgb[9], rgb[10], rgb[11],
                        rgb[12], rgb[13], rgb[14], rgb[15]);
                track->codecPrivate = subidx;
                track->codecPrivateSize = len + 1;
                break;
            case TEXTSUB:
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
        track->language = subtitle->iso639_2;

        mux_data->track = mk_createTrack(m->file, track);
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

static int MKVMux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    ogg_packet  *op = NULL;
    hb_job_t * job = m->job;
    hb_title_t * title = job->title;
    uint64_t   timecode = 0;
    hb_chapter_t *chapter_data;
    char tmp_buffer[1024];
    char *string = tmp_buffer;

    if (mux_data == job->mux_data)
    {
        /* Video */
        timecode = buf->start * TIMECODE_SCALE;

        if (job->chapter_markers && (buf->new_chap || timecode == 0))
        {
            /* Make sure we're not writing a chapter that has 0 length */
            if (mux_data->prev_chapter_tc != timecode)
            {
                if ( buf->new_chap )
                {
                    mux_data->current_chapter = buf->new_chap - 2;
                }
                chapter_data = hb_list_item( title->list_chapter,
                                             mux_data->current_chapter++ );
                tmp_buffer[0] = '\0';

                if( chapter_data != NULL )
                {
                    string = chapter_data->title;
                }

                if( strlen(string) == 0 || strlen(string) >= 1024 )
                {
                    snprintf( tmp_buffer, 1023, "Chapter %02i", mux_data->current_chapter );
                    string = tmp_buffer;
                }
                mk_createChapterSimple(m->file, mux_data->prev_chapter_tc, mux_data->prev_chapter_tc, string);
            }
            mux_data->prev_chapter_tc = timecode;
        }

        if (job->vcodec == HB_VCODEC_THEORA)
        {
            /* ughhh, theora is a pain :( */
            op = (ogg_packet *)buf->data;
            op->packet = buf->data + sizeof( ogg_packet );
            if (mk_startFrame(m->file, mux_data->track) < 0)
            {
                hb_error( "Failed to write frame to output file, Disk Full?" );
                *job->die = 1;
            }
            mk_addFrameData(m->file, mux_data->track, op->packet, op->bytes);
            mk_setFrameFlags(m->file, mux_data->track, timecode, 1, 0);
            return 0;
        }
    }
    else if ( mux_data->subtitle )
    {
        timecode = buf->start * TIMECODE_SCALE;
        if( mk_startFrame(m->file, mux_data->track) < 0)
        {
            hb_error( "Failed to write frame to output file, Disk Full?" );
            *job->die = 1;
        }
        if( mux_data->sub_format == TEXTSUB )
        {
            uint64_t   duration;

            duration = buf->stop * TIMECODE_SCALE - timecode;
            mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
            mk_setFrameFlags(m->file, mux_data->track, timecode, 1, duration);
        }
        else
        {
            mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
            mk_setFrameFlags(m->file, mux_data->track, timecode, 1, 0);
        }
        mk_flushFrame(m->file, mux_data->track);
        return 0;
    }
    else
    {
        /* Audio */
        timecode = buf->start * TIMECODE_SCALE;
        if (mux_data->codec == HB_ACODEC_VORBIS)
        {
            /* ughhh, vorbis is a pain :( */
            op = (ogg_packet *)buf->data;
            op->packet = buf->data + sizeof( ogg_packet );
            if (mk_startFrame(m->file, mux_data->track))
            {
                hb_error( "Failed to write frame to output file, Disk Full?" );
                *job->die = 1;
            }
            mk_addFrameData(m->file, mux_data->track, op->packet, op->bytes);
            mk_setFrameFlags(m->file, mux_data->track, timecode, 1, 0);
            return 0;
        }
    }

    if( mk_startFrame(m->file, mux_data->track) < 0)
    {
        hb_error( "Failed to write frame to output file, Disk Full?" );
        *job->die = 1;
    }
    mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
    mk_setFrameFlags(m->file, mux_data->track, timecode,
                     ((job->vcodec == HB_VCODEC_X264 && 
                       mux_data == job->mux_data) ? 
                            (buf->frametype == HB_FRAME_IDR) : 
                            ((buf->frametype & HB_FRAME_KEY) != 0)), 0 );
    return 0;
}

static int MKVEnd( hb_mux_object_t * m )
{
    hb_job_t  *job = m->job;
    hb_mux_data_t *mux_data = job->mux_data;
    hb_title_t  *title = job->title;
    hb_chapter_t *chapter_data;
    char tmp_buffer[1024];
    char *string = tmp_buffer;

    if( !job->mux_data )
    {
        /*
         * We must have failed to create the file in the first place.
         */
        return 0;
    }

    chapter_data = hb_list_item( title->list_chapter, mux_data->current_chapter++ );

    if(job->chapter_markers)
    {
        tmp_buffer[0] = '\0';

        if( chapter_data != NULL )
        {
            string = chapter_data->title;
        }

        if( strlen(string) == 0 || strlen(string) >= 1024 )
        {
            snprintf( tmp_buffer, 1023, "Chapter %02i", mux_data->current_chapter );
            string = tmp_buffer;
        }
        mk_createChapterSimple(m->file, mux_data->prev_chapter_tc, mux_data->prev_chapter_tc, string);
    }

    if( title->metadata )
    {
        hb_metadata_t *md = title->metadata;

        hb_deep_log( 2, "Writing Metadata to output file...");
        mk_createTagSimple( m->file, MK_TAG_TITLE, md->name );
        mk_createTagSimple( m->file, "ARTIST", md->artist );
        mk_createTagSimple( m->file, "COMPOSER", md->composer );
        mk_createTagSimple( m->file, MK_TAG_SYNOPSIS, md->comment );
        mk_createTagSimple( m->file, "DATE_RELEASED", md->release_date );
        // mk_createTagSimple( m->file, "", md->album );
        mk_createTagSimple( m->file, MK_TAG_GENRE, md->genre );
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
