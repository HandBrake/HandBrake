/* bd.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/lang.h"
#include "handbrake/hbffmpeg.h"

#include "libbluray/bluray.h"

struct hb_bd_s
{
    char                    * path;
    BLURAY                  * bd;
    int                       title_count;
    BLURAY_TITLE_INFO      ** title_info;
    const BLURAY_DISC_INFO  * disc_info;
    int64_t                   duration;
    hb_stream_t             * stream;
    int                       chapter;
    int                       next_chap;
    hb_handle_t             * h;
    int                       keep_duplicate_titles;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static int           next_packet( BLURAY *bd, uint8_t *pkt );
static int title_info_compare_mpls(const void *, const void *);

/***********************************************************************
 * hb_bd_init
 ***********************************************************************
 *
 **********************************************************************/
hb_bd_t * hb_bd_init( hb_handle_t *h, const char * path, int keep_duplicate_titles )
{
    hb_bd_t * d;
    int ii;

    d = calloc( sizeof( hb_bd_t ), 1 );
    d->h = h;
    d->keep_duplicate_titles = keep_duplicate_titles;

    /* Open device */
    d->bd = bd_open( path, NULL );
    if( d->bd == NULL )
    {
        /*
         * Not an error, may be a stream - which we'll try in a moment.
         */
        hb_log( "bd: not a bd - trying as a stream/file instead" );
        goto fail;
    }

    uint8_t flags = TITLES_FILTER_DUP_CLIP;
    if (!keep_duplicate_titles)
    {
        flags |= TITLES_FILTER_DUP_TITLE;
    }
    d->title_count = bd_get_titles( d->bd, flags, 0 );
    if ( d->title_count == 0 )
    {
        hb_log( "bd: not a bd - trying as a stream/file instead" );
        goto fail;
    }
    d->title_info = calloc( sizeof( BLURAY_TITLE_INFO* ) , d->title_count );
    for ( ii = 0; ii < d->title_count; ii++ )
    {
        d->title_info[ii] = bd_get_title_info( d->bd, ii, 0 );
    }
    qsort(d->title_info, d->title_count, sizeof( BLURAY_TITLE_INFO* ), title_info_compare_mpls );
    d->disc_info = bd_get_disc_info(d->bd);
    d->path = strdup( path );

    return d;

fail:
    if( d->bd ) bd_close( d->bd );
    free( d );
    return NULL;
}

/***********************************************************************
 * hb_bd_title_count
 **********************************************************************/
int hb_bd_title_count( hb_bd_t * d )
{
    return d->title_count;
}

static void add_subtitle(int track, hb_list_t *list_subtitle, BLURAY_STREAM_INFO *bdsub, uint32_t codec, uint32_t codec_param)
{
    hb_subtitle_t * subtitle;
    iso639_lang_t * lang;

    subtitle = calloc( sizeof( hb_subtitle_t ), 1 );

    subtitle->track = track;
    subtitle->id = bdsub->pid;

    switch ( bdsub->coding_type )
    {
        case BLURAY_STREAM_TYPE_SUB_PG:
            subtitle->source = PGSSUB;
            subtitle->format = PICTURESUB;
            subtitle->config.dest = RENDERSUB;
            break;
        default:
            // Unrecognized, don't add to list
            free( subtitle );
            return;
    }
    lang = lang_for_code2( (char*)bdsub->lang );
    snprintf(subtitle->lang, sizeof( subtitle->lang ), "%s (%s)",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name,
             hb_subsource_name(subtitle->source));
    snprintf(subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "%s",
             lang->iso639_2);

    subtitle->reg_desc     = STR4_TO_UINT32("HDMV");
    subtitle->stream_type  = bdsub->coding_type;
    subtitle->codec        = codec;
    subtitle->codec_param  = codec_param;
    subtitle->timebase.num = 1;
    subtitle->timebase.den = 90000;

    hb_log( "bd: subtitle id=0x%x, lang=%s, 3cc=%s", subtitle->id,
            subtitle->lang, subtitle->iso639_2 );

    hb_list_add( list_subtitle, subtitle );
    return;
}

static void add_audio(int index, int linked_index, int track, hb_list_t *list_audio, BLURAY_STREAM_INFO *bdaudio, int substream_type, uint32_t codec, uint32_t codec_param, int attributes)
{
    const char * codec_name;
    hb_audio_t * audio;
    iso639_lang_t * lang;

    audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = (substream_type << 16) | bdaudio->pid;
    audio->config.index = index;
    if (linked_index >= 0)
    {
        audio->config.list_linked_index = hb_list_init();
        hb_list_add_dup(audio->config.list_linked_index,
                        &linked_index, sizeof(linked_index));
    }
    audio->config.in.reg_desc = STR4_TO_UINT32("HDMV");
    audio->config.in.stream_type = bdaudio->coding_type;
    audio->config.in.substream_type = substream_type;
    audio->config.in.codec = codec;
    audio->config.in.codec_param = codec_param;

    switch( audio->config.in.codec )
    {
        case HB_ACODEC_AC3:
            codec_name = "AC3";
            break;
        case HB_ACODEC_DCA:
            codec_name = "DTS";
            break;
        default:
        {
            if( audio->config.in.codec & HB_ACODEC_FF_MASK )
            {
                switch( bdaudio->coding_type )
                {
                    case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
                        codec_name = "E-AC3";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
                        codec_name = "DTS-HD HRA";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
                        codec_name = "DTS-HD MA";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_LPCM:
                        codec_name = "BD LPCM";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
                        codec_name = "MPEG1";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
                        codec_name = "MPEG2";
                        break;
                    case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
                        codec_name = "TrueHD";
                        break;
                    default:
                        codec_name = "Unknown FFmpeg";
                        break;
                }
            }
            else
            {
                codec_name = "Unknown";
            }
        }
        break;
    }

    lang = lang_for_code2( (char*)bdaudio->lang );

    audio->config.lang.attributes = attributes;

    snprintf( audio->config.lang.simple,
              sizeof( audio->config.lang.simple ), "%s",
              strlen( lang->native_name ) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2,
              sizeof( audio->config.lang.iso639_2 ), "%s", lang->iso639_2 );

    hb_log("bd: audio id=0x%x, lang=%s (%s), 3cc=%s", audio->id,
           audio->config.lang.simple, codec_name, audio->config.lang.iso639_2);

    audio->config.in.track        = track;
    audio->config.in.timebase.num = 1;
    audio->config.in.timebase.den = 90000;

    hb_list_add( list_audio, audio );
    return;
}

static int bd_audio_equal( BLURAY_CLIP_INFO *a, BLURAY_CLIP_INFO *b )
{
    int ii, jj, equal;

    if ( a->audio_stream_count != b->audio_stream_count )
        return 0;

    if ( a->audio_stream_count == 0 )
        return 0;

    for ( ii = 0; ii < a->audio_stream_count; ii++ )
    {
        BLURAY_STREAM_INFO * s = &a->audio_streams[ii];
        equal = 0;
        for ( jj = 0; jj < b->audio_stream_count; jj++ )
        {
            if ( s->pid == b->audio_streams[jj].pid &&
                 s->coding_type == b->audio_streams[jj].coding_type)
            {
                equal = 1;
                break;
            }
        }
        if ( !equal )
            return 0;
    }
    return 1;
}

static void show_clip_list( BLURAY_TITLE_INFO * ti )
{
    int ii;

    for (ii = 0; ii < ti->clip_count; ii++)
    {
        BLURAY_CLIP_INFO * ci = &ti->clips[ii];
        int64_t            duration = ci->out_time - ci->in_time;
        int                hh, mm, ss;

        hh = duration / (90000 * 60 * 60);
        mm = (duration / (90000 * 60)) % 60;
        ss = (duration / 90000) % 60;
        hb_log("bd:\t\t%s.M2TS -- Duration: %02d:%02d:%02d",
               ti->clips[ii].clip_id, hh, mm, ss);
    }
}

/***********************************************************************
 * hb_bd_title_scan
 **********************************************************************/
hb_title_t * hb_bd_title_scan( hb_bd_t * d, int tt, uint64_t min_duration, uint64_t max_duration )
{

    hb_title_t   * title;
    hb_chapter_t * chapter;
    int            ii, jj;
    BLURAY_TITLE_INFO * ti = NULL;

    hb_log( "bd: scanning title %d", tt );

    title = hb_title_init( d->path, tt );
    title->demuxer = HB_TS_DEMUXER;
    title->type = HB_BD_TYPE;
    title->reg_desc = STR4_TO_UINT32("HDMV");
    title->keep_duplicate_titles = d->keep_duplicate_titles;

    if (d->disc_info->disc_name != NULL && d->disc_info->disc_name[0] != 0)
    {
        title->name = strdup(d->disc_info->disc_name);
    }
    else if (d->disc_info->udf_volume_id != NULL &&
             d->disc_info->udf_volume_id[0] != 0)
    {
        title->name = strdup(d->disc_info->udf_volume_id);
    }
    else
    {
        char * p_cur, * p_last = d->path;
        for( p_cur = d->path; *p_cur; p_cur++ )
        {
            if( IS_DIR_SEP(p_cur[0]) && p_cur[1] )
            {
                p_last = &p_cur[1];
            }
        }
        title->name = strdup(p_last);
        char *dot_term = strrchr(title->name, '.');
        if (dot_term)
            *dot_term = '\0';
    }

    if (tt <= d->title_count)
    {
        ti = d->title_info[tt - 1];
    }
    if ( ti == NULL )
    {
        hb_log( "bd: invalid title" );
        goto fail;
    }
    if ( ti->clip_count == 0 )
    {
        hb_log( "bd: stream has no clips" );
        goto fail;
    }
    if ( ti->clips[0].video_stream_count == 0 )
    {
        hb_log( "bd: stream has no video" );
        goto fail;
    }

    hb_log( "bd: playlist %05d.MPLS", ti->playlist );
    title->playlist = ti->playlist;
    title->angle_count = ti->angle_count;

    /* Get duration */
    title->duration = ti->duration;
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "bd: duration is %02d:%02d:%02d (%"PRIu64" ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* ignore short titles because they're often stills */
    if( ti->duration < min_duration )
    {
        hb_log( "bd: ignoring title (too short)" );
        goto fail;
    }
    
    if ( max_duration > 0 && ti->duration > max_duration ) 
    {
        hb_log( "bd: ignoring title (too long)" );
        goto fail;
    }
    
    if (global_verbosity_level >= 2)
    {
        show_clip_list(ti);
    }

    BLURAY_STREAM_INFO * bdvideo = &ti->clips[0].video_streams[0];

    title->video_id = bdvideo->pid;
    title->video_stream_type = bdvideo->coding_type;

    hb_log( "bd: video id=0x%x, stream type=%s, format %s", title->video_id,
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_MPEG1 ? "MPEG1" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_MPEG2 ? "MPEG2" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_VC1 ? "VC-1" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_H264 ? "H.264" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_HEVC ? "HEVC" :
            "Unknown",
            bdvideo->format == BLURAY_VIDEO_FORMAT_480I ? "480i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_576I ? "576i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_480P ? "480p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_1080I ? "1080i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_720P ? "720p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_1080P ? "1080p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_576P ? "576p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_2160P ? "2160p" :
            "Unknown"
          );

    switch( bdvideo->coding_type )
    {
        case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
        case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
            break;

        case BLURAY_STREAM_TYPE_VIDEO_VC1:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_VC1;
            break;

        case BLURAY_STREAM_TYPE_VIDEO_H264:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_H264;
            break;

        case BLURAY_STREAM_TYPE_VIDEO_HEVC:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = AV_CODEC_ID_HEVC;
            break;

        default:
            hb_log( "scan: unknown video codec (0x%x)",
                    bdvideo->coding_type );
            goto fail;
    }

    switch ( bdvideo->aspect )
    {
        case BLURAY_ASPECT_RATIO_4_3:
            title->container_dar.num = 4;
            title->container_dar.den = 3;
            break;
        case BLURAY_ASPECT_RATIO_16_9:
            title->container_dar.num = 16;
            title->container_dar.den = 9;
            break;
        default:
            hb_log( "bd: unknown aspect %d, assuming 16:9", bdvideo->aspect );
            title->container_dar.num = 16;
            title->container_dar.den = 9;
            break;
    }
    hb_log("bd: aspect = %d:%d",
           title->container_dar.num, title->container_dar.den);

    /* Detect audio */
    // Max primary BD audios is 32
    int matches;
    int most_audio = 0;
    int audio_clip_index = 0;
    if (ti->clip_count > 2)
    {
        // All BD clips are not all required to have the same audio.
        // But clips that have seamless transition are required
        // to have the same audio as the previous clip.
        // So find the clip that has the most other clips with the
        // matching audio.
        for ( ii = 0; ii < ti->clip_count; ii++ )
        {
            matches = 0;
            for ( jj = 0; jj < ti->clip_count; jj++ )
            {
                if ( bd_audio_equal( &ti->clips[ii], &ti->clips[jj] ) )
                {
                    matches++;
                }
            }
            if ( matches > most_audio )
            {
                most_audio = matches;
                audio_clip_index = ii;
            }
        }
    }
    else if (ti->clip_count == 2)
    {
        // If there are only 2 clips, pick audios from the longer clip
        if (ti->clips[0].pkt_count < ti->clips[1].pkt_count)
            audio_clip_index = 1;
    }

    // Add all the audios found in the above clip.
    for (ii = 0; ii < ti->clips[audio_clip_index].audio_stream_count; ii++)
    {
        BLURAY_STREAM_INFO * bdaudio;
        int                  index;

        bdaudio = &ti->clips[audio_clip_index].audio_streams[ii];
        index   = hb_list_count(title->list_audio);

        switch (bdaudio->coding_type)
        {
            case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
                // Add 2 audio tracks.  One for TrueHD and one for AC-3
                add_audio(index, index + 1, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_AC3,
                          HB_ACODEC_AC3, AV_CODEC_ID_AC3, HB_AUDIO_ATTR_NONE);
                add_audio(index + 1, index, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_TRUEHD,
                          HB_ACODEC_FFTRUEHD, AV_CODEC_ID_TRUEHD,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTS:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA, AV_CODEC_ID_DTS, HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
            case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFMPEG, AV_CODEC_ID_MP2,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFEAC3, AV_CODEC_ID_EAC3,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFEAC3, AV_CODEC_ID_EAC3,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_LPCM:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFMPEG, AV_CODEC_ID_PCM_BLURAY,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_AC3, AV_CODEC_ID_AC3, HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
            case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
                // Add 2 audio tracks.  One for DTS-HD and one for DTS
                add_audio(index, index + 1, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_DTS,
                          HB_ACODEC_DCA, AV_CODEC_ID_DTS, HB_AUDIO_ATTR_NONE);
                // DTS-HD is special.  The substreams must be concatenated
                // DTS-core followed by DTS-hd-extensions.  Setting
                // a substream id of 0 says use all substreams.
                add_audio(index + 1, index, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_NONE);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
                // BD "DTSHD_SECONDARY" is DTS Express which has no
                // DTS core
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_NONE);
                break;

            default:
                hb_log("scan: unknown audio pid 0x%x codec 0x%x", bdaudio->pid,
                       bdaudio->coding_type);
                break;
        }
    }

    // Add all the secondary audios found in the above clip.
    for (jj = 0; jj < ti->clips[audio_clip_index].sec_audio_stream_count; jj++, ii++)
    {
        BLURAY_STREAM_INFO * bdaudio;
        int                  index;

        bdaudio = &ti->clips[audio_clip_index].sec_audio_streams[jj];
        index   = hb_list_count(title->list_audio);

        switch (bdaudio->coding_type)
        {
            case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
                // Add 2 audio tracks.  One for TrueHD and one for AC-3
                add_audio(index, index + 1, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_AC3,
                          HB_ACODEC_AC3, AV_CODEC_ID_AC3,
                          HB_AUDIO_ATTR_SECONDARY);
                add_audio(index + 1, index, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_TRUEHD,
                          HB_ACODEC_FFTRUEHD, AV_CODEC_ID_TRUEHD,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTS:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
            case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFMPEG, AV_CODEC_ID_MP2,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
            case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS_SECONDARY:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFEAC3, AV_CODEC_ID_EAC3,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_LPCM:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_FFMPEG, AV_CODEC_ID_PCM_BLURAY,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3:
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_AC3, AV_CODEC_ID_AC3,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
            case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
                // Add 2 audio tracks.  One for DTS-HD and one for DTS
                add_audio(index, index + 1, ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_DTS,
                          HB_ACODEC_DCA, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_SECONDARY);
                // DTS-HD is special.  The substreams must be concatenated
                // DTS-core followed by DTS-hd-extensions.  Setting
                // a substream id of 0 says use all substreams.
                add_audio(index + 1, index, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTSHD_SECONDARY:
                // BD "DTSHD_SECONDARY" is DTS Express which has no
                // DTS core
                add_audio(index, -1, ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_DCA_HD, AV_CODEC_ID_DTS,
                          HB_AUDIO_ATTR_SECONDARY);
                break;

            default:
                hb_log("scan: unknown audio pid 0x%x codec 0x%x", bdaudio->pid,
                       bdaudio->coding_type);
                break;
        }
    }

    // Add all the subtitles found in the above clip.
    for ( ii = 0; ii < ti->clips[audio_clip_index].pg_stream_count; ii++ )
    {
        BLURAY_STREAM_INFO * bdpgs;

        bdpgs = &ti->clips[audio_clip_index].pg_streams[ii];

        switch( bdpgs->coding_type )
        {
            case BLURAY_STREAM_TYPE_SUB_PG:
                add_subtitle(ii, title->list_subtitle, bdpgs, WORK_DECAVSUB,
                             AV_CODEC_ID_HDMV_PGS_SUBTITLE);
                break;
            default:
                hb_log( "scan: unknown subtitle pid 0x%x codec 0x%x",
                        bdpgs->pid, bdpgs->coding_type );
                break;
        }
    }

    /* Chapters */
    for ( ii = 0, jj = 0; ii < ti->chapter_count; ii++ )
    {
        char chapter_title[80];

        // Sanity check start time of this chapter.
        // If the chapter starts within 1.5 seconds of the end of
        // the title, drop it.
        if (ti->duration - ti->chapters[ii].start < 90000 * 1.5)
        {
            hb_log("bd: chapter %d too short %"PRIu64", dropping", ii+1,
                   ti->chapters[ii].start);
            continue;
        }

        chapter = calloc( sizeof( hb_chapter_t ), 1 );

        chapter->index = ++jj;
        snprintf( chapter_title, sizeof(chapter_title), "Chapter %d", chapter->index );
        hb_chapter_set_title( chapter, chapter_title );

        chapter->duration = ti->chapters[ii].duration;

        // Sanity check chapter duration and start times
        // Have seen some invalid durations in the wild
        if (ii < ti->chapter_count - 1)
        {
            // Validate start time
            if (ti->chapters[ii+1].start < ti->chapters[ii].start)
            {
                hb_log("bd: chapter %d invalid start %"PRIu64"", ii+1,
                       ti->chapters[ii+1].start);
                ti->chapters[ii+1].start = ti->chapters[ii].start +
                                           chapter->duration;
            }
            if (ti->chapters[ii+1].start - ti->chapters[ii].start !=
                chapter->duration)
            {
                hb_log("bd: chapter %d invalid duration %"PRIu64"", ii+1,
                       chapter->duration);
                chapter->duration = ti->chapters[ii+1].start -
                                    ti->chapters[ii].start;
            }
        }
        else
        {
            if (ti->duration - ti->chapters[ii].start != chapter->duration)
            {
                hb_log("bd: chapter %d invalid duration %"PRIu64"", ii+1,
                       chapter->duration);
                chapter->duration = ti->duration - ti->chapters[ii].start;
            }
        }

        int seconds      = ( chapter->duration + 45000 ) / 90000;
        chapter->hours   = ( seconds / 3600 );
        chapter->minutes = ( seconds % 3600 ) / 60;
        chapter->seconds = ( seconds % 60 );

        hb_log( "bd: chap %d, %"PRIu64" ms",
                chapter->index,
                chapter->duration / 90 );

        hb_list_add( title->list_chapter, chapter );
    }
    hb_log( "bd: title %d has %d chapters", tt, hb_list_count(title->list_chapter));

    /* This title is ok so far */
    goto cleanup;

fail:
    hb_title_close( &title );

cleanup:

    return title;
}

/***********************************************************************
 * hb_bd_main_feature
 **********************************************************************/
int hb_bd_main_feature( hb_bd_t * d, hb_list_t * list_title )
{
    int longest = 0;
    int ii;
    uint64_t longest_duration = 0;
    int highest_rank = 0, rank;
    int most_chapters = 0;
    int ranks[9] = {0, 1, 3, 2, 6, 5, 7, 4, 8};
    BLURAY_TITLE_INFO * ti;

    for ( ii = 0; ii < hb_list_count( list_title ); ii++ )
    {
        hb_title_t * title = hb_list_item( list_title, ii );
        ti = d->title_info[title->index - 1];
        if ( ti )
        {
            BLURAY_STREAM_INFO * bdvideo = &ti->clips[0].video_streams[0];
            if ( title->duration > longest_duration * 0.7 && bdvideo->format < 8 )
            {
                rank = 0;
                if (bdvideo->format <= 8)
                {
                    rank = ranks[bdvideo->format];
                }
                if (highest_rank < rank ||
                    ( title->duration > longest_duration &&
                      highest_rank == rank))
                {
                    longest = title->index;
                    longest_duration = title->duration;
                    highest_rank = rank;
                    most_chapters = ti->chapter_count;
                }
                else if (highest_rank == rank &&
                         title->duration == longest_duration &&
                         ti->chapter_count > most_chapters)
                {
                    longest = title->index;
                    most_chapters = ti->chapter_count;
                }
            }
        }
        else if ( title->duration > longest_duration )
        {
            longest_duration = title->duration;
            longest = title->index;
        }
    }
    return longest;
}

/***********************************************************************
 * hb_bd_start
 ***********************************************************************
 * Title and chapter start at 1
 **********************************************************************/
int hb_bd_start( hb_bd_t * d, hb_title_t *title )
{
    BD_EVENT event;

    d->duration  = title->duration;

    // Calling bd_get_event initializes libbluray event queue.
    bd_select_title( d->bd, d->title_info[title->index - 1]->idx );
    bd_get_event( d->bd, &event );
    d->chapter = 0;
    d->next_chap = 1;
    d->stream = hb_bd_stream_open( d->h, title );
    if ( d->stream == NULL )
    {
        return 0;
    }
    return 1;
}

/***********************************************************************
 * hb_bd_stop
 ***********************************************************************
 *
 **********************************************************************/
void hb_bd_stop( hb_bd_t * d )
{
    if( d->stream ) hb_stream_close( &d->stream );
}

/***********************************************************************
 * hb_bd_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_bd_seek( hb_bd_t * d, float f )
{
    uint64_t pos = f * d->duration;

    bd_seek_time(d->bd, pos);
    d->next_chap = bd_get_current_chapter( d->bd ) + 1;
    hb_ts_stream_reset(d->stream);
    return 1;
}

int hb_bd_seek_pts( hb_bd_t * d, uint64_t pts )
{
    bd_seek_time(d->bd, pts);
    d->next_chap = bd_get_current_chapter( d->bd ) + 1;
    hb_ts_stream_reset(d->stream);
    return 1;
}

int hb_bd_seek_chapter( hb_bd_t * d, int c )
{
    d->next_chap = c;
    bd_seek_chapter( d->bd, c - 1 );
    hb_ts_stream_reset(d->stream);
    return 1;
}

/***********************************************************************
 * hb_bd_read
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_bd_read( hb_bd_t * d )
{
    int result;
    int error_count = 0;
    int retry_count = 0;
    uint8_t buf[192];
    BD_EVENT event;
    uint64_t pos;
    hb_buffer_t * out = NULL;
    uint8_t discontinuity;

    while ( 1 )
    {
        discontinuity = 0;
        result = next_packet( d->bd, buf );
        while ( bd_get_event( d->bd, &event ) )
        {
            switch ( event.event )
            {
                case BD_EVENT_CHAPTER:
                    // The muxers expect to only get chapter 2 and above
                    // They write chapter 1 when chapter 2 is detected.
                    if (event.param > d->chapter)
                    {
                        d->next_chap = event.param;
                    }
                    break;

                case BD_EVENT_PLAYITEM:
                    discontinuity = 1;
                    hb_deep_log(2, "bd: Play item %u", event.param);
                    break;

                case BD_EVENT_STILL:
                    bd_read_skip_still( d->bd );
                    break;

                case BD_EVENT_END_OF_TITLE:
                    hb_log("bd: End of title");
                    if (result <= 0)
                    {
                        return NULL;
                    }
                    break;

                default:
                    break;
            }
        }

        if ( result < 0 )
        {
            hb_error("bd: Read Error");
            pos = bd_tell( d->bd );
            bd_seek( d->bd, pos + 192 );
            error_count++;
            if (error_count > 10)
            {
                hb_error("bd: Error, too many consecutive read errors");
                hb_set_work_error(d->h, HB_ERROR_READ);
                return NULL;
            }
            continue;
        }
        else if ( result == 0 )
        {
            // libbluray returns 0 when it encounters and skips a bad unit.
            // So retry a few times to be certain there is no more data
            // to be read.
            retry_count++;
            if (retry_count > 1000)
            {
                // A unit is 6144 bytes (32 TS packets).  Give up after we've
                // seen > 6MB of invalid data.
                hb_error("bd: Error, too many consecutive bad units.");
                hb_set_work_error(d->h, HB_ERROR_READ);
                return NULL;
            }
            continue;
        }

        if (retry_count > 0)
        {
            hb_error("bd: Read Error, skipping bad data.");
            retry_count = 0;
        }

        error_count = 0;
        // buf+4 to skip the BD timestamp at start of packet
        if (d->chapter != d->next_chap)
        {
            d->chapter = d->next_chap;
            out = hb_ts_decode_pkt(d->stream, buf+4, d->chapter, discontinuity);
        }
        else
        {
            out = hb_ts_decode_pkt(d->stream, buf+4, 0, discontinuity);
        }
        if (out != NULL)
        {
            return out;
        }
    }
}

/***********************************************************************
 * hb_bd_chapter
 ***********************************************************************
 * Returns in which chapter the next block to be read is.
 * Chapter numbers start at 1.
 **********************************************************************/
int hb_bd_chapter( hb_bd_t * d )
{
    return d->chapter;
}

/***********************************************************************
 * hb_bd_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_bd_close( hb_bd_t ** _d )
{
    hb_bd_t * d = *_d;
    int ii;

    if ( d->title_info )
    {
        for ( ii = 0; ii < d->title_count; ii++ )
            bd_free_title_info( d->title_info[ii] );
        free( d->title_info );
    }
    if( d->stream ) hb_stream_close( &d->stream );
    if( d->bd ) bd_close( d->bd );
    if( d->path ) free( d->path );

    free( d );
    *_d = NULL;
}

/***********************************************************************
 * hb_bd_set_angle
 ***********************************************************************
 * Sets the angle to read
 **********************************************************************/
void hb_bd_set_angle( hb_bd_t * d, int angle )
{

    if ( !bd_select_angle( d->bd, angle) )
    {
        hb_log("bd_select_angle failed");
    }
}

static int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte, no scrambling & a legal adaptation ctrl
    return (buf[0] == 0x47) && ((buf[3] >> 6) == 0) && ((buf[3] >> 4) > 0);
}

static int have_ts_sync(const uint8_t *buf, int psize)
{
    return check_ts_sync(&buf[0*psize]) && check_ts_sync(&buf[1*psize]) &&
           check_ts_sync(&buf[2*psize]) && check_ts_sync(&buf[3*psize]) &&
           check_ts_sync(&buf[4*psize]) && check_ts_sync(&buf[5*psize]) &&
           check_ts_sync(&buf[6*psize]) && check_ts_sync(&buf[7*psize]);
}

#define MAX_HOLE 192*80

static uint64_t align_to_next_packet(BLURAY *bd, uint8_t *pkt)
{
    int      result;
    uint8_t  buf[MAX_HOLE];
    uint64_t pos = 0;
    uint64_t start = bd_tell(bd);
    uint64_t orig;
    uint64_t off = 192;

    memcpy(buf, pkt, 192);
    if ( start >= 192 ) {
        start -= 192;
    }
    orig = start;

    while (1)
    {
        result = bd_read(bd, buf + off, sizeof(buf) - off);
        if (result == sizeof(buf) - off)
        {
            const uint8_t *bp = buf;
            int i;

            for ( i = sizeof(buf) - 8 * 192; --i >= 0; ++bp )
            {
                if ( have_ts_sync( bp, 192 ) )
                {
                    break;
                }
            }
            if ( i >= 0 )
            {
                pos = ( bp - buf );
                break;
            }
            off = 8 * 192;
            memcpy(buf, buf + sizeof(buf) - off, off);
            start += sizeof(buf) - off;
        }
        else if (result < 0)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
    off = start + pos - 4;
    // bd_seek seeks to the nearest access unit *before* the requested position
    // we don't want to seek backwards, so we need to read until we get
    // past that position.
    bd_seek(bd, off);
    while (off > bd_tell(bd))
    {
        result = bd_read(bd, buf, 192);
        if (result < 0)
        {
            return -1;
        }
        else if (result != 192)
        {
            return 0;
        }
    }
    return start - orig + pos;
}

static int next_packet( BLURAY *bd, uint8_t *pkt )
{
    int result;

    while ( 1 )
    {
        result = bd_read( bd, pkt, 192 );
        if ( result < 0 )
        {
            return -1;
        }
        if ( result < 192 )
        {
            return 0;
        }
        // Sync byte is byte 4.  0-3 are timestamp.
        if (pkt[4] == 0x47)
        {
            return 1;
        }
        // lost sync - back up to where we started then try to re-establish.
        uint64_t pos = bd_tell(bd);
        uint64_t pos2 = align_to_next_packet(bd, pkt);
        if (pos2 < 0)
        {
            return -1;
        }
        else if (pos2 == 0)
        {
            hb_log("next_packet: eof while re-establishing sync @ %"PRIu64"", pos );
            return 0;
        }
        hb_log("next_packet: sync lost @ %"PRIu64", regained after %"PRIu64" bytes",
                 pos, pos2 );
    }
}

static int title_info_compare_mpls(const void *va, const void *vb)
{
    BLURAY_TITLE_INFO *a, *b;

    a = *(BLURAY_TITLE_INFO**)va;
    b = *(BLURAY_TITLE_INFO**)vb;

    return a->playlist - b->playlist;
}
