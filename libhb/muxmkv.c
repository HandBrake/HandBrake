/* $Id:  $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

/* libmkv header */
#include "libmkv.h"

#include <ogg/ogg.h>

#include "hb.h"

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
    uint64_t  max_tc;
    uint16_t  current_chapter;
};

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
    int             avcC_len, i;
    ogg_packet      *ogg_headers[3];
    mk_TrackConfig *track;

    track = calloc(1, sizeof(mk_TrackConfig));

    m->file = mk_createWriter(job->file, 1000000, 1);

    /* Video track */
    mux_data      = calloc(1, sizeof( hb_mux_data_t ) );
    job->mux_data = mux_data;

    track->trackType = MK_TRACK_VIDEO;
    track->flagDefault = 1;
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
            break;
        case HB_VCODEC_XVID:
        case HB_VCODEC_FFMPEG:
            track->codecID = MK_VCODEC_MP4ASP;
            track->codecPrivate = job->config.mpeg4.bytes;
            track->codecPrivateSize = job->config.mpeg4.length;
            break;
        default:
            *job->die = 1;
            hb_log("muxmkv: Unknown video codec: %x", job->vcodec);
            return 0;
    }

    track->video.pixelWidth = job->width;
    track->video.pixelHeight = job->height;
    track->video.displayHeight = job->height;
    if(job->pixel_ratio)
    {
        track->video.displayWidth = job->width * ((double)job->pixel_aspect_width / (double)job->pixel_aspect_height);
    }
    else
    {
        track->video.displayWidth = job->width;
    }


    track->defaultDuration = (int64_t)(((float)job->vrate_base / (float)job->vrate) * 1000000000);

    mux_data->track = mk_createTrack(m->file, track);

    memset(track, 0, sizeof(mk_TrackConfig));

    /* add the audio tracks */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        mux_data = malloc( sizeof( hb_mux_data_t ) );
        audio->mux_data = mux_data;

        switch (job->acodec)
        {
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
                    int i, j;
                    int64_t offset = 0;
                    int64_t cp_size = 0;
                    char    *cp;
                    track->codecID = MK_ACODEC_VORBIS;
                    cp_size = sizeof( char );
                    for (i = 0; i < 3; ++i)
                    {
                        ogg_headers[i] = (ogg_packet *)audio->config.vorbis.headers[i];
                        ogg_headers[i]->packet = (unsigned char *)&audio->config.vorbis.headers[i] + sizeof( ogg_packet );
                        cp_size += (sizeof( char ) * ((ogg_headers[i]->bytes / 255) + 1)) + ogg_headers[i]->bytes;
                            /* This will be too big, but it doesn't matter, as we only need it to be big enough. */
                    }
                    cp = track->codecPrivate = calloc(1, cp_size);
                    cp[offset++] = 0x02;
                    for (i = 0; i < 2; ++i)
                    {
                        for (j = ogg_headers[i]->bytes; j >= 255; j -= 255)
                        {
                            cp[offset++] = 255;
                        }
                        cp[offset++] = j;
                    }
                    for(i = 0; i < 3; ++i)
                    {
                        memcpy(cp + offset, ogg_headers[i]->packet, ogg_headers[i]->bytes);
                        offset += ogg_headers[i]->bytes;
                    }
                    track->codecPrivateSize = offset;
                }
                break;
            case HB_ACODEC_FAAC:
                track->codecPrivate = audio->config.aac.bytes;
                track->codecPrivateSize = audio->config.aac.length;
                track->codecID = MK_ACODEC_AAC;
                break;
            default:
                *job->die = 1;
                hb_log("muxmkv: Unknown audio codec: %x", job->acodec);
                return 0;
        }
        
        if (default_track_flag)
        {
            track->flagDefault = 1;
            default_track_flag = 0;
        }
        
        track->trackType = MK_TRACK_AUDIO;
        track->language = audio->iso639_2;
        track->audio.samplingFreq = (float)job->arate;
        track->audio.channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown);
//        track->defaultDuration = job->arate * 1000;
        mux_data->track = mk_createTrack(m->file, track);
        if (job->acodec == HB_ACODEC_VORBIS && track->codecPrivate != NULL)
          free(track->codecPrivate);
    }

    mk_writeHeader( m->file, "HandBrake " HB_VERSION);
    if (track != NULL)
        free(track);
    if (avcC != NULL)
        free(avcC);

    return 0;
}

static int MKVMux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    hb_job_t * job = m->job;
    hb_title_t * title = job->title;
    uint64_t   timecode = 0;
    hb_chapter_t *chapter_data;
    char tmp_buffer[1024];
    char *string = tmp_buffer;
    if (mux_data == job->mux_data)
    {
        /* Video */
        /* Where does the 11130 come from? I had to calculate it from the actual
          * and the observed duration of the file. Otherwise the timecodes come
          * out way too small, and you get a 2hr movie that plays in .64 sec.  */
        if ((job->vcodec == HB_VCODEC_X264) && (buf->frametype & HB_FRAME_REF))
        {
            timecode = (buf->start + (buf->renderOffset - 1000000)) * 11130;
        }
        else
        {
            timecode = buf->start * 11130;
        }

        if (job->chapter_markers && (buf->new_chap || timecode == 0))
        {
            /* Make sure we're not writing a chapter that has 0 length */
            if (mux_data->prev_chapter_tc != timecode)
            {
                chapter_data = hb_list_item( title->list_chapter, mux_data->current_chapter );
                tmp_buffer[0] = '\0';

                if( chapter_data != NULL )
                {
                    string = chapter_data->title;
                }

                if( strlen(string) == 0 || strlen(string) >= 1024 )
                {
                    snprintf( tmp_buffer, 1023, "Chapter %02i", mux_data->current_chapter++ );
                    string = tmp_buffer;
                }
                mk_createChapterSimple(m->file, mux_data->prev_chapter_tc, timecode, string);
            }
            mux_data->prev_chapter_tc = timecode;
        }

        if (buf->stop * 11130 > mux_data->max_tc)
            mux_data->max_tc = buf->stop * 11130;
    }
    else
    {
        /* Audio */
        timecode = buf->start * 11130;
        if (job->acodec == HB_ACODEC_VORBIS)
        {
            /* ughhh, vorbis is a pain :( */
            ogg_packet  *op;

            op = (ogg_packet *)buf->data;
            op->packet = buf->data + sizeof( ogg_packet );
            mk_startFrame(m->file, mux_data->track);
            mk_addFrameData(m->file, mux_data->track, op->packet, op->bytes);
            mk_setFrameFlags(m->file, mux_data->track, timecode, 1);
            return 0;
        }
    }

    mk_startFrame(m->file, mux_data->track);
    mk_addFrameData(m->file, mux_data->track, buf->data, buf->size);
    mk_setFrameFlags(m->file, mux_data->track, timecode,
        ((job->vcodec == HB_VCODEC_X264 && mux_data == job->mux_data) ? (buf->frametype == HB_FRAME_IDR) : ((buf->frametype & HB_FRAME_KEY) != 0)) );
    return 0;
}

static int MKVEnd( hb_mux_object_t * m )
{
    hb_job_t  *job = m->job;
    hb_mux_data_t *mux_data = job->mux_data;
    hb_title_t  *title = job->title;
    hb_chapter_t *chapter_data = hb_list_item( title->list_chapter, mux_data->current_chapter );
    char tmp_buffer[1024];
    char *string = tmp_buffer;

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
        mk_createChapterSimple(m->file, mux_data->prev_chapter_tc, mux_data->max_tc, string);
    }

    mk_close(m->file);

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
