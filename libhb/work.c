/* work.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "a52dec/a52.h"
#include "libavformat/avformat.h"
#include "openclwrapper.h"

typedef struct
{
    hb_handle_t  * handle;
    hb_list_t    * jobs;
    hb_job_t    ** current_job;
    int          * error;
    volatile int * die;

} hb_work_t;

static void work_func();
static void do_job( hb_job_t *);
static void work_loop( void * );
static void filter_loop( void * );

#define FIFO_UNBOUNDED 65536
#define FIFO_UNBOUNDED_WAKE 65535
#define FIFO_LARGE 32
#define FIFO_LARGE_WAKE 16
#define FIFO_SMALL 16
#define FIFO_SMALL_WAKE 15
#define FIFO_MINI 4
#define FIFO_MINI_WAKE 3

/**
 * Allocates work object and launches work thread with work_func.
 * @param jobs Handle to hb_list_t.
 * @param die Handle to user inititated exit indicator.
 * @param error Handle to error indicator.
 */
hb_thread_t * hb_work_init( hb_handle_t * handle, hb_list_t * jobs, volatile int * die, int * error, hb_job_t ** job )
{
    hb_work_t * work = calloc( sizeof( hb_work_t ), 1 );

    work->handle      = handle;
    work->jobs        = jobs;
    work->current_job = job;
    work->die         = die;
    work->error       = error;

    return hb_thread_init( "work", work_func, work, HB_LOW_PRIORITY );
}

static void InitWorkState( hb_handle_t * h )
{
    hb_state_t state;

    state.state = HB_STATE_WORKING;
#define p state.param.working
    p.progress  = 0.0;
    p.rate_cur  = 0.0;
    p.rate_avg  = 0.0;
    p.hours     = -1;
    p.minutes   = -1;
    p.seconds   = -1; 
#undef p

    hb_set_state( h, &state );

}

/**
 * Iterates through job list and calls do_job for each job.
 * @param _work Handle work object.
 */
static void work_func( void * _work )
{
    hb_work_t  * work = _work;
    hb_job_t   * job;

    hb_log( "%d job(s) to process", hb_list_count( work->jobs ) );

    hb_prevent_sleep( work->handle );

    while( !*work->die && ( job = hb_list_item( work->jobs, 0 ) ) )
    {
        hb_list_rem( work->jobs, job );
        job->die = work->die;
        *(work->current_job) = job;

        InitWorkState( job->h );
        do_job( job );

        *(work->current_job) = NULL;
    }

    hb_allow_sleep( work->handle );

    *(work->error) = HB_ERROR_NONE;

    free( work );
}

hb_work_object_t * hb_get_work( int id )
{
    hb_work_object_t * w;
    for( w = hb_objects; w; w = w->next )
    {
        if( w->id == id )
        {
            hb_work_object_t *wc = malloc( sizeof(*w) );
            *wc = *w;
            return wc;
        }
    }
    return NULL;
}

hb_work_object_t * hb_codec_decoder( int codec )
{
    switch( codec )
    {
        case HB_ACODEC_AC3:  return hb_get_work( WORK_DECA52 );
        case HB_ACODEC_LPCM: return hb_get_work( WORK_DECLPCM );
        default:
            if ( codec & HB_ACODEC_FF_MASK )
            {
                return hb_get_work( WORK_DECAVCODEC );
            }
            break;
    }
    return NULL;
}

hb_work_object_t * hb_codec_encoder( int codec )
{
    hb_work_object_t * w;
    switch( codec )
    {
        case HB_ACODEC_FAAC:   return hb_get_work( WORK_ENCFAAC );
        case HB_ACODEC_LAME:   return hb_get_work( WORK_ENCLAME );
        case HB_ACODEC_VORBIS: return hb_get_work( WORK_ENCVORBIS );
        case HB_ACODEC_CA_AAC: return hb_get_work( WORK_ENC_CA_AAC );
        case HB_ACODEC_CA_HAAC:return hb_get_work( WORK_ENC_CA_HAAC );
        case HB_ACODEC_FFAAC:
        {
            w = hb_get_work( WORK_ENCAVCODEC_AUDIO );
            w->codec_param = AV_CODEC_ID_AAC;
            return w;
        }
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
        {
            w = hb_get_work( WORK_ENCAVCODEC_AUDIO );
            w->codec_param = AV_CODEC_ID_FLAC;
            return w;
        }
        case HB_ACODEC_AC3:
        {
            w = hb_get_work( WORK_ENCAVCODEC_AUDIO );
            w->codec_param = AV_CODEC_ID_AC3;
            return w;
        }
    }
    return NULL;
}

/**
 * Displays job parameters in the debug log.
 * @param job Handle work hb_job_t.
 */
void hb_display_job_info( hb_job_t * job )
{
    hb_title_t * title = job->title;
    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    int             i, j;
    
    hb_log("job configuration:");
    hb_log( " * source");
    
    hb_log( "   + %s", title->path );

    if( job->pts_to_start || job->pts_to_stop )
    {
        int64_t stop;
        int hr_start, min_start, hr_stop, min_stop;
        float sec_start, sec_stop;

        stop = job->pts_to_start + job->pts_to_stop;

        hr_start = job->pts_to_start / (90000 * 60 * 60);
        min_start = job->pts_to_start / (90000 * 60);
        sec_start = (float)job->pts_to_start / 90000.0 - min_start * 60;
        min_start %= 60;

        hr_stop = stop / (90000 * 60 * 60);
        min_stop = stop / (90000 * 60);
        sec_stop = (float)stop / 90000.0 - min_stop * 60;
        min_stop %= 60;

        hb_log( "   + title %d, start %d:%d:%.2f stop %d:%d:%.2f", title->index,
                hr_start, min_start, sec_start,
                hr_stop, min_stop, sec_stop);
    }
    else if( job->frame_to_start || job->frame_to_stop )
    {
        hb_log( "   + title %d, frames %d to %d", title->index,
                job->frame_to_start, job->frame_to_start + job->frame_to_stop );
    }
    else
    {
        hb_log( "   + title %d, chapter(s) %d to %d", title->index,
                job->chapter_start, job->chapter_end );
    }

    if( title->container_name != NULL )
        hb_log( "   + container: %s", title->container_name);

    if( title->data_rate )
    {
        hb_log( "   + data rate: %d kbps", title->data_rate / 1000 );
    }
    
    hb_log( " * destination");

    hb_log( "   + %s", job->file );

    switch( job->mux )
    {
        case HB_MUX_MP4:
            hb_log("   + container: MPEG-4 (.mp4 and .m4v)");
            
            if( job->ipod_atom )
                hb_log( "     + compatibility atom for iPod 5G");

            if( job->largeFileSize )
                hb_log( "     + 64-bit formatting");

            if( job->mp4_optimize )
                hb_log( "     + optimized for progressive web downloads");
            
            if( job->color_matrix_code )
                hb_log( "     + custom color matrix: %s", job->color_matrix_code == 1 ? "ITU Bt.601 (SD)" : job->color_matrix_code == 2 ? "ITU Bt.709 (HD)" : "Custom" );
            break;

        case HB_MUX_MKV:
            hb_log("   + container: Matroska (.mkv)");
            break;
    }

    if( job->chapter_markers )
    {
        hb_log( "     + chapter markers" );
    }
    
    hb_log(" * video track");
    
    hb_log("   + decoder: %s", title->video_codec_name );
    
    if( title->video_bitrate )
    {
        hb_log( "     + bitrate %d kbps", title->video_bitrate / 1000 );
    }
    
    if( job->cfr == 0 )
    {
        hb_log( "   + frame rate: same as source (around %.3f fps)",
            (float) title->rate / (float) title->rate_base );
    }
    else if( job->cfr == 1 )
    {
        hb_log( "   + frame rate: %.3f fps -> constant %.3f fps",
            (float) title->rate / (float) title->rate_base,
            (float) job->vrate / (float) job->vrate_base );
    }
    else if( job->cfr == 2 )
    {
        hb_log( "   + frame rate: %.3f fps -> peak rate limited to %.3f fps",
            (float) title->rate / (float) title->rate_base,
            (float) job->pfr_vrate / (float) job->pfr_vrate_base );
    }

    // Filters can modify dimensions.  So show them first.
    if( hb_list_count( job->list_filter ) )
    {
        hb_log("   + %s", hb_list_count( job->list_filter) > 1 ? "filters" : "filter" );
        for( i = 0; i < hb_list_count( job->list_filter ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            if( filter->settings )
                hb_log("     + %s (%s)", filter->name, filter->settings);
            else
                hb_log("     + %s (default settings)", filter->name);
            if( filter->info )
            {
                hb_filter_info_t info;
                filter->info( filter, &info );
                if( info.human_readable_desc[0] )
                {
                    hb_log("       + %s", info.human_readable_desc);
                }
            }
        }
    }
    
    if( job->anamorphic.mode )
    {
        hb_log( "   + %s anamorphic", job->anamorphic.mode == 1 ? "strict" : job->anamorphic.mode == 2? "loose" : "custom" );
        if( job->anamorphic.mode == 3 && job->anamorphic.keep_display_aspect )
        {
            hb_log( "     + keeping source display aspect ratio"); 
        }
        hb_log( "     + storage dimensions: %d * %d, mod %i",
                    job->width, job->height, job->modulus );
        if( job->anamorphic.itu_par )
        {
            hb_log( "     + using ITU pixel aspect ratio values"); 
        }
        hb_log( "     + pixel aspect ratio: %i / %i", job->anamorphic.par_width, job->anamorphic.par_height );
        hb_log( "     + display dimensions: %.0f * %i",
            (float)( job->width * job->anamorphic.par_width / job->anamorphic.par_height ), job->height );
    }
    else
    {
        hb_log( "   + dimensions: %d * %d, mod %i",
                job->width, job->height, job->modulus );
    }

    if ( job->grayscale )
        hb_log( "   + grayscale mode" );

    if( !job->indepth_scan )
    {
        /* Video encoder */
        for( i = 0; i < hb_video_encoders_count; i++ )
        {
            if( hb_video_encoders[i].encoder == job->vcodec )
            {
                hb_log( "   + encoder: %s", hb_video_encoders[i].human_readable_name );
                break;
            }
        }

        if( job->x264_preset && *job->x264_preset &&
            job->vcodec == HB_VCODEC_X264 )
        {
            hb_log( "     + x264 preset: %s", job->x264_preset );
        }
        if( job->x264_tune && *job->x264_tune &&
            job->vcodec == HB_VCODEC_X264 )
        {
            hb_log( "     + x264 tune: %s", job->x264_tune );
        }
        if( job->advanced_opts && *job->advanced_opts &&
            ( ( job->vcodec & HB_VCODEC_FFMPEG_MASK ) ||
              ( job->vcodec == HB_VCODEC_X264 ) ) )
        {
            hb_log( "     + options: %s", job->advanced_opts );
        }
        if( job->h264_profile && *job->h264_profile &&
            job->vcodec == HB_VCODEC_X264 )
        {
            hb_log( "     + h264 profile: %s", job->h264_profile );
        }
        if( job->h264_level && *job->h264_level &&
            job->vcodec == HB_VCODEC_X264 )
        {
            hb_log( "     + h264 level: %s", job->h264_level );
        }

        if( job->vquality >= 0 )
        {
            hb_log( "     + quality: %.2f %s", job->vquality, job->vcodec == HB_VCODEC_X264 ? "(RF)" : "(QP)" ); 
        }
        else
        {
            hb_log( "     + bitrate: %d kbps, pass: %d", job->vbitrate, job->pass );
            if( job->pass == 1 && job->fastfirstpass == 1 &&
                job->vcodec == HB_VCODEC_X264 )
            {
                hb_log( "     + fast first pass" );
                hb_log( "     + options: ref=1:8x8dct=0:me=dia:trellis=0" );
                hb_log( "                analyse=i4x4 (if originally enabled, else analyse=none)" );
                hb_log( "                subq=2 (if originally greater than 2, else subq unchanged)" );
            }
        }
    }

    if( job->indepth_scan )
    {
        hb_log( " * Foreign Audio Search: %s%s%s",
                job->select_subtitle_config.dest == RENDERSUB ? "Render/Burn-in" : "Passthrough",
                job->select_subtitle_config.force ? ", Forced Only" : "",
                job->select_subtitle_config.default_track ? ", Default" : "" );
    }

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( job->list_subtitle, i );

        if( subtitle )
        {
            if( job->indepth_scan )
            {
                hb_log( "   + subtitle, %s (track %d, id 0x%x) %s [%s]",
                        subtitle->lang, subtitle->track, subtitle->id,
                        subtitle->format == PICTURESUB ? "Picture" : "Text",
                        hb_subsource_name( subtitle->source ) );
            }
            else if( subtitle->source == SRTSUB )
            {
                /* For SRT, print offset and charset too */
                hb_log( " * subtitle track %d, %s (track %d, id 0x%x) Text [SRT] -> Passthrough%s, offset: %"PRId64", charset: %s",
                        subtitle->out_track, subtitle->lang, subtitle->track, subtitle->id,
                        subtitle->config.default_track ? ", Default" : "",
                        subtitle->config.offset, subtitle->config.src_codeset );
            }
            else
            {
                hb_log( " * subtitle track %d, %s (track %d, id 0x%x) %s [%s] -> %s%s%s",
                        subtitle->out_track, subtitle->lang, subtitle->track, subtitle->id,
                        subtitle->format == PICTURESUB ? "Picture" : "Text",
                        hb_subsource_name( subtitle->source ),
                        subtitle->config.dest == RENDERSUB ? "Render/Burn-in" : "Passthrough",
                        subtitle->config.force ? ", Forced Only" : "",
                        subtitle->config.default_track ? ", Default" : "" );
            }
        }
    }

    if( !job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            audio = hb_list_item( job->list_audio, i );

            hb_log( " * audio track %d", audio->config.out.track );
            
            if( audio->config.out.name )
                hb_log( "   + name: %s", audio->config.out.name );

            hb_log( "   + decoder: %s (track %d, id 0x%x)", audio->config.lang.description, audio->config.in.track + 1, audio->id );

            if (audio->config.in.bitrate >= 1000)
                hb_log("     + bitrate: %d kbps, samplerate: %d Hz",
                       audio->config.in.bitrate / 1000,
                       audio->config.in.samplerate);
            else
                hb_log("     + samplerate: %d Hz",
                       audio->config.in.samplerate);

            if( audio->config.out.codec & HB_ACODEC_PASS_FLAG )
            {
                for( j = 0; j < hb_audio_encoders_count; j++ )
                {
                    if( hb_audio_encoders[j].encoder == audio->config.out.codec )
                    {
                        hb_log( "   + %s", hb_audio_encoders[j].human_readable_name );
                        break;
                    }
                }
            }
            else
            {
                for( j = 0; j < hb_audio_mixdowns_count; j++ )
                {
                    if( hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown )
                    {
                        hb_log( "   + mixdown: %s", hb_audio_mixdowns[j].human_readable_name );
                        break;
                    }
                }
                if( audio->config.out.normalize_mix_level != 0 )
                {
                    hb_log( "   + normalized mixing levels" );
                }
                if( audio->config.out.gain != 0.0 )
                {
                    hb_log( "   + gain: %.fdB", audio->config.out.gain );
                }
                if( ( audio->config.out.dynamic_range_compression != 0.0 ) && ( audio->config.in.codec == HB_ACODEC_AC3 ) )
                {
                    hb_log( "   + dynamic range compression: %f", audio->config.out.dynamic_range_compression );
                }
                if (hb_audio_dither_is_supported(audio->config.out.codec))
                {
                    hb_log("   + dither: %s",
                           hb_audio_dither_get_description(audio->config.out.dither_method));
                }
                for( j = 0; j < hb_audio_encoders_count; j++ )
                {
                    if( hb_audio_encoders[j].encoder == audio->config.out.codec )
                    {
                        hb_log( "   + encoder: %s", hb_audio_encoders[j].human_readable_name );
                        if( audio->config.out.bitrate > 0 )
                            hb_log( "     + bitrate: %d kbps, samplerate: %d Hz", audio->config.out.bitrate, audio->config.out.samplerate );
                        else if( audio->config.out.quality != HB_INVALID_AUDIO_QUALITY )
                            hb_log( "     + quality: %.2f, samplerate: %d Hz", audio->config.out.quality, audio->config.out.samplerate );
                        else if( audio->config.out.samplerate > 0 )
                            hb_log( "     + samplerate: %d Hz", audio->config.out.samplerate );
                        if( audio->config.out.compression_level >= 0 )
                            hb_log( "     + compression level: %.2f", 
                                    audio->config.out.compression_level );
                        break;
                    }
                }
            }
        }
    }
}

/* Corrects framerates when actual duration and frame count numbers are known. */
void correct_framerate( hb_job_t * job )
{
    hb_interjob_t * interjob = hb_interjob_get( job->h );

    if( ( job->sequence_id & 0xFFFFFF ) != ( interjob->last_job & 0xFFFFFF) )
        return; // Interjob information is for a different encode.

    // compute actual output vrate from first pass
    interjob->vrate = job->vrate_base * ( (double)interjob->out_frame_count * 90000 / interjob->total_time );
    interjob->vrate_base = job->vrate_base;
}

/**
 * Job initialization rountine.
 * Initializes fifos.
 * Creates work objects for synchronizer, video decoder, video renderer, video decoder, audio decoder, audio encoder, reader, muxer.
 * Launches thread for each work object with work_loop.
 * Loops while monitoring status of work threads and fifos.
 * Exits loop when conversion is done and fifos are empty.
 * Closes threads and frees fifos.
 * @param job Handle work hb_job_t.
 */
static void do_job( hb_job_t * job )
{
    hb_title_t    * title;
    int             i, j;
    hb_work_object_t * w;
    hb_work_object_t * sync;
    hb_work_object_t * muxer;
    hb_work_object_t *reader = hb_get_work(WORK_READER);
    hb_interjob_t * interjob;

    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    unsigned int subtitle_highest = 0;
    unsigned int subtitle_lowest = 0;
    unsigned int subtitle_lowest_id = 0;
    unsigned int subtitle_forced_id = 0;
    unsigned int subtitle_forced_hits = 0;
    unsigned int subtitle_hit = 0;

    title = job->title;
    interjob = hb_interjob_get( job->h );

    if( job->pass == 2 )
    {
        correct_framerate( job );
    }

    job->list_work = hb_list_init();

    hb_log( "starting job" );
    if ( job->use_opencl || job->use_hwd)
    {
        hb_log( "Using GPU : Yes.\n" );
        /* init opencl environment */ 
#ifdef USE_OPENCL
        if ( job->use_opencl )
            job->use_opencl =! hb_init_opencl_run_env(0, NULL, "-I.");
#endif    
    }
    else
        hb_log( "Using GPU : NO.\n" );
    /* Look for the scanned subtitle in the existing subtitle list
     * select_subtitle implies that we did a scan. */
    if( !job->indepth_scan && interjob->select_subtitle )
    {
        /* Disable forced subtitles if we didn't find any in the scan, so that
         * we display normal subtitles instead. */
        if( interjob->select_subtitle->config.force &&
            interjob->select_subtitle->forced_hits == 0 )
        {
            interjob->select_subtitle->config.force = 0;
        }
        for( i = 0; i < hb_list_count( job->list_subtitle ); )
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if( subtitle )
            {
                /* Remove the scanned subtitle from the list if
                 * it would result in:
                 * - an emty track (forced and no forced hits)
                 * - an identical, duplicate subtitle track:
                 *   -> both (or neither) are forced 
                 *   -> subtitle is not forced but all its hits are forced */
                if( ( interjob->select_subtitle->id == subtitle->id ) &&
                    ( ( subtitle->config.force &&
                        interjob->select_subtitle->forced_hits == 0 ) ||
                      ( subtitle->config.force == interjob->select_subtitle->config.force ) ||
                      ( !subtitle->config.force &&
                        interjob->select_subtitle->hits == interjob->select_subtitle->forced_hits ) ) )
                {
                    hb_list_rem( job->list_subtitle, subtitle );
                    free( subtitle );
                    continue;
                }
                /* Adjust output track number, in case we removed one.
                 * Output tracks sadly still need to be in sequential order.
                 * Note: out.track starts at 1, i starts at 0, and track 1 is interjob->select_subtitle */
                subtitle->out_track = ++i + 1;
            }
            else
            {
                // avoid infinite loop is subtitle == NULL
                i++;
            }
        }

        /* Add the subtitle that we found on the subtitle scan pass.
         *
         * Make sure it's the first subtitle in the list so that it becomes the
         * first burned subtitle (explicitly or after sanitizing) - which should
         * ensure that it doesn't get dropped. */
        interjob->select_subtitle->out_track = 1;
        if (job->pass == 0 || job->pass == 2)
        {
            // final pass, interjob->select_subtitle is no longer needed
            hb_list_insert(job->list_subtitle, 0, interjob->select_subtitle);
            interjob->select_subtitle = NULL;
        }
        else
        {
            // this is not the final pass, so we need to copy it instead
            hb_list_insert(job->list_subtitle, 0, hb_subtitle_copy(interjob->select_subtitle));
        }
    }

    if ( !job->indepth_scan )
    {
        // Sanitize subtitles
        uint8_t one_burned = 0;
        for( i = 0; i < hb_list_count( job->list_subtitle ); )
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if ( subtitle->config.dest == RENDERSUB )
            {
                if ( one_burned )
                {
                    if ( !hb_subtitle_can_pass(subtitle->source, job->mux) )
                    {
                        hb_log( "More than one subtitle burn-in requested, dropping track %d.", i );
                        hb_list_rem( job->list_subtitle, subtitle );
                        free( subtitle );
                        continue;
                    }
                    else
                    {
                        hb_log( "More than one subtitle burn-in requested.  Changing track %d to soft subtitle.", i );
                        subtitle->config.dest = PASSTHRUSUB;
                    }
                }
                else if ( !hb_subtitle_can_burn(subtitle->source) )
                {
                    hb_log( "Subtitle burn-in requested and input track can not be rendered.  Changing track %d to soft subtitle.", i );
                    subtitle->config.dest = PASSTHRUSUB;
                }
                else
                {
                    one_burned = 1;
                }
            }

            if ( subtitle->config.dest == PASSTHRUSUB &&
                 !hb_subtitle_can_pass(subtitle->source, job->mux) )
            {
                if ( !one_burned )
                {
                    hb_log( "Subtitle pass-thru requested and input track is not compatible with container.  Changing track %d to burned-in subtitle.", i );
                    subtitle->config.dest = RENDERSUB;
                    subtitle->config.default_track = 0;
                    one_burned = 1;
                }
                else
                {
                    hb_log( "Subtitle pass-thru requested and input track is not compatible with container.  One track already burned, dropping track %d.", i );
                    hb_list_rem( job->list_subtitle, subtitle );
                    free( subtitle );
                    continue;
                }
            }
            /* Adjust output track number, in case we removed one.
             * Output tracks sadly still need to be in sequential order.
             * Note: out.track starts at 1, i starts at 0 */
            subtitle->out_track = ++i;
        }
        if ( one_burned )
        {
            hb_filter_object_t * filter;

            // Add subtitle rendering filter
            // Note that if the filter is already in the filter chain, this
            // has no effect. Note also that this means the front-end is
            // not required to add the subtitle rendering filter since
            // we will always try to do it here.
            filter = hb_filter_init(HB_FILTER_RENDER_SUB);
            hb_add_filter( job, filter, NULL );
        }
    }

    // Filters have an effect on settings.
    // So initialize the filters and update the job.
    if( job->list_filter && hb_list_count( job->list_filter ) )
    {
        hb_filter_init_t init;

        init.job = job;
        init.pix_fmt = AV_PIX_FMT_YUV420P;
        init.width = title->width;
        init.height = title->height;
#ifdef USE_OPENCL
        init.title_width = title->width;
        init.title_height = title->height;
        init.use_dxva = hb_use_dxva( title ); 
        if ( init.use_dxva && ( title->width > job->width || title->height > job->height ) )
        {
            init.width = job->width;
            init.height = job->height;
        }
#endif
        init.par_width = job->anamorphic.par_width;
        init.par_height = job->anamorphic.par_height;
        memcpy(init.crop, job->crop, sizeof(int[4]));
        init.vrate_base = job->vrate_base;
        init.vrate = job->vrate;
        init.pfr_vrate_base = job->pfr_vrate_base;
        init.pfr_vrate = job->pfr_vrate;
        init.cfr = 0;
        for( i = 0; i < hb_list_count( job->list_filter ); )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            if( filter->init( filter, &init ) )
            {
                hb_log( "Failure to initialise filter '%s', disabling",
                        filter->name );
                hb_list_rem( job->list_filter, filter );
                hb_filter_close( &filter );
                continue;
            }
            i++;
        }
        job->width = init.width;
        job->height = init.height;
        job->anamorphic.par_width = init.par_width;
        job->anamorphic.par_height = init.par_height;
        memcpy(job->crop, init.crop, sizeof(int[4]));
        job->vrate_base = init.vrate_base;
        job->vrate = init.vrate;
        job->pfr_vrate_base = init.pfr_vrate_base;
        job->pfr_vrate = init.pfr_vrate;
        job->cfr = init.cfr;
    }

    if( job->anamorphic.mode )
    {
        /* While x264 is smart enough to reduce fractions on its own, libavcodec and
         * the MacGUI need some help with the math, so lose superfluous factors. */
        hb_reduce( &job->anamorphic.par_width, &job->anamorphic.par_height,
                    job->anamorphic.par_width,  job->anamorphic.par_height );
        if( job->vcodec & HB_VCODEC_FFMPEG_MASK )
        {
            /* Just to make working with ffmpeg even more fun,
             * lavc's MPEG-4 encoder can't handle PAR values >= 255,
             * even though AVRational does. Adjusting downwards
             * distorts the display aspect slightly, but such is life. */
            while( ( job->anamorphic.par_width  & ~0xFF ) ||
                   ( job->anamorphic.par_height & ~0xFF ) )
            {
                job->anamorphic.par_width  >>= 1;
                job->anamorphic.par_height >>= 1;
                hb_reduce( &job->anamorphic.par_width, &job->anamorphic.par_height,
                            job->anamorphic.par_width,  job->anamorphic.par_height );
            }
        }
    }
    
    job->fifo_mpeg2  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
    job->fifo_raw    = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_sync   = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_mpeg4  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
    job->fifo_render = NULL; // Attached to filter chain

    /* Audio fifos must be initialized before sync */
    if (!job->indepth_scan)
    {
        // apply Auto Passthru settings
        hb_autopassthru_apply_settings(job);
        // sanitize audio settings
        for (i = 0; i < hb_list_count(job->list_audio);)
        {
            audio = hb_list_item(job->list_audio, i);
            if (audio->config.out.codec == HB_ACODEC_AUTO_PASS)
            {
                // Auto Passthru should have been handled above
                // remove track to avoid a crash
                hb_log("Auto Passthru error, dropping track %d",
                       audio->config.out.track);
                hb_list_rem(job->list_audio, audio);
                free(audio);
                continue;
            }
            if ((audio->config.out.codec & HB_ACODEC_PASS_FLAG) &&
                !(audio->config.in.codec &
                  audio->config.out.codec & HB_ACODEC_PASS_MASK))
            {
                hb_log("Passthru requested and input codec is not the same as output codec for track %d, dropping track",
                       audio->config.out.track);
                hb_list_rem(job->list_audio, audio);
                free(audio);
                continue;
            }
            /* Adjust output track number, in case we removed one.
             * Output tracks sadly still need to be in sequential order.
             * Note: out.track starts at 1, i starts at 0 */
            audio->config.out.track = ++i;
        }

        int best_mixdown    = 0;
        int best_bitrate    = 0;
        int best_samplerate = 0;

        for (i = 0; i < hb_list_count(job->list_audio); i++)
        {
            audio = hb_list_item(job->list_audio, i);

            /* set up the audio work structures */
            audio->priv.fifo_raw  = hb_fifo_init(FIFO_SMALL, FIFO_SMALL_WAKE);
            audio->priv.fifo_sync = hb_fifo_init(FIFO_SMALL, FIFO_SMALL_WAKE);
            audio->priv.fifo_out  = hb_fifo_init(FIFO_LARGE, FIFO_LARGE_WAKE);
            audio->priv.fifo_in   = hb_fifo_init(FIFO_LARGE, FIFO_LARGE_WAKE);

            /* Passthru audio, nothing to sanitize here */
            if (audio->config.out.codec & HB_ACODEC_PASS_FLAG)
                continue;

            /* Vorbis language information */
            if (audio->config.out.codec == HB_ACODEC_VORBIS)
                audio->priv.config.vorbis.language = audio->config.lang.simple;

            /* sense-check the requested samplerate */
            if (audio->config.out.samplerate < 0)
            {
                // if not specified, set to same as input
                audio->config.out.samplerate = audio->config.in.samplerate;
            }
            best_samplerate =
                hb_get_best_samplerate(audio->config.out.codec,
                                       audio->config.out.samplerate, NULL);
            if (best_samplerate != audio->config.out.samplerate)
            {
                int ii;
                for (ii = 0; ii < hb_audio_rates_count; ii++)
                {
                    if (best_samplerate == hb_audio_rates[ii].rate)
                    {
                        hb_log("work: sanitizing track %d unsupported samplerate %d Hz to %s kHz",
                               audio->config.out.track, audio->config.out.samplerate,
                               hb_audio_rates[ii].string);
                        break;
                    }
                }
                audio->config.out.samplerate = best_samplerate;
            }

            /* sense-check the requested mixdown */
            if (audio->config.out.mixdown <= HB_AMIXDOWN_NONE)
            {
                /* Mixdown not specified, set the default mixdown */
                audio->config.out.mixdown =
                    hb_get_default_mixdown(audio->config.out.codec,
                                           audio->config.in.channel_layout);
                for (j = 0; j < hb_audio_mixdowns_count; j++)
                {
                    if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown)
                    {
                        hb_log("work: mixdown not specified, track %d setting mixdown %s",
                               audio->config.out.track,
                               hb_audio_mixdowns[j].human_readable_name);
                        break;
                    }
                }
            }
            else
            {
                best_mixdown =
                    hb_get_best_mixdown(audio->config.out.codec,
                                        audio->config.in.channel_layout,
                                        audio->config.out.mixdown);
                if (audio->config.out.mixdown != best_mixdown)
                {
                    int prev_mix_idx = 0, best_mix_idx = 0;
                    for (j = 0; j < hb_audio_mixdowns_count; j++)
                    {
                        if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown)
                        {
                            prev_mix_idx = j;
                        }
                        else if (hb_audio_mixdowns[j].amixdown == best_mixdown)
                        {
                            best_mix_idx = j;
                        }
                    }
                    /* log the output mixdown */
                    hb_log("work: sanitizing track %d mixdown %s to %s",
                           audio->config.out.track,
                           hb_audio_mixdowns[prev_mix_idx].human_readable_name,
                           hb_audio_mixdowns[best_mix_idx].human_readable_name);
                    audio->config.out.mixdown = best_mixdown;
                }
            }

            /* sense-check the requested compression level */
            if (audio->config.out.compression_level < 0)
            {
                audio->config.out.compression_level =
                    hb_get_default_audio_compression(audio->config.out.codec);
                if (audio->config.out.compression_level >= 0)
                {
                    hb_log("work: compression level not specified, track %d setting compression level %.2f",
                           audio->config.out.track,
                           audio->config.out.compression_level);
                }
            }
            else
            {
                float best_compression =
                    hb_get_best_audio_compression(audio->config.out.codec,
                                                  audio->config.out.compression_level);
                if (best_compression != audio->config.out.compression_level)
                {
                    if (best_compression == -1)
                    {
                        hb_log("work: track %d, compression level not supported by codec",
                               audio->config.out.track);
                    }
                    else
                    {
                        hb_log("work: sanitizing track %d compression level %.2f to %.2f",
                               audio->config.out.track,
                               audio->config.out.compression_level,
                               best_compression);
                    }
                    audio->config.out.compression_level = best_compression;
                }
            }

            /* sense-check the requested quality */
            if (audio->config.out.quality != HB_INVALID_AUDIO_QUALITY)
            {
                float best_quality =
                    hb_get_best_audio_quality(audio->config.out.codec,
                                              audio->config.out.quality);
                if (best_quality != audio->config.out.quality)
                {
                    if (best_quality == HB_INVALID_AUDIO_QUALITY)
                    {
                        hb_log("work: track %d, quality mode not supported by codec",
                               audio->config.out.track);
                    }
                    else
                    {
                        hb_log("work: sanitizing track %d quality %.2f to %.2f",
                               audio->config.out.track,
                               audio->config.out.quality, best_quality);
                    }
                    audio->config.out.quality = best_quality;
                }
            }

            /* sense-check the requested bitrate */
            if (audio->config.out.quality == HB_INVALID_AUDIO_QUALITY)
            {
                if (audio->config.out.bitrate <= 0)
                {
                    /* Bitrate not specified, set the default bitrate */
                    audio->config.out.bitrate =
                        hb_get_default_audio_bitrate(audio->config.out.codec,
                                                     audio->config.out.samplerate,
                                                     audio->config.out.mixdown);
                    if (audio->config.out.bitrate > 0)
                    {
                        hb_log("work: bitrate not specified, track %d setting bitrate %d Kbps",
                               audio->config.out.track,
                               audio->config.out.bitrate);
                    }
                }
                else
                {
                    best_bitrate =
                        hb_get_best_audio_bitrate(audio->config.out.codec,
                                                  audio->config.out.bitrate,
                                                  audio->config.out.samplerate,
                                                  audio->config.out.mixdown);
                    if (best_bitrate > 0 &&
                        best_bitrate != audio->config.out.bitrate)
                    {
                        /* log the output bitrate */
                        hb_log("work: sanitizing track %d bitrate %d to %d Kbps",
                               audio->config.out.track,
                               audio->config.out.bitrate, best_bitrate);
                    }
                    audio->config.out.bitrate = best_bitrate;
                }
            }

            /* sense-check the requested dither */
            if (hb_audio_dither_is_supported(audio->config.out.codec))
            {
                if (audio->config.out.dither_method ==
                    hb_audio_dither_get_default())
                {
                    /* "auto", enable with default settings */
                    audio->config.out.dither_method =
                        hb_audio_dither_get_default_method();
                }
            }
            else if (audio->config.out.dither_method !=
                     hb_audio_dither_get_default())
            {
                /* specific dither requested but dithering not supported */
                hb_log("work: track %d, dithering not supported by codec",
                       audio->config.out.track);
            }
        }
    }

    /* Synchronization */
    sync = hb_sync_init( job );

    /* Video decoder */
    int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
#if defined(USE_FF_MPEG2)
    if (vcodec == WORK_DECMPEG2)
    {
        vcodec = WORK_DECAVCODECV;
        title->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
    }
#endif

    hb_list_add( job->list_work, ( w = hb_get_work( vcodec ) ) );
    w->codec_param = title->video_codec_param;
    w->fifo_in  = job->fifo_mpeg2;
    w->fifo_out = job->fifo_raw;

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( job->list_subtitle, i );

        if( subtitle )
        {
            subtitle->fifo_in   = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
            // Must set capacity of the raw-FIFO to be set >= the maximum number of subtitle
            // lines that could be decoded prior to a video frame in order to prevent the following
            // deadlock condition:
            //   1. Subtitle decoder blocks trying to generate more subtitle lines than will fit in the FIFO.
            //   2. Blocks the processing of further subtitle packets read from the input stream.
            //   3. And that blocks the processing of any further video packets read from the input stream.
            //   4. And that blocks the sync work-object from running, which is needed to consume the subtitle lines in the raw-FIFO.
            // Since that number is unbounded, the FIFO must be made (effectively) unbounded in capacity.
            subtitle->fifo_raw  = hb_fifo_init( FIFO_UNBOUNDED, FIFO_UNBOUNDED_WAKE );
            subtitle->fifo_sync = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
            subtitle->fifo_out  = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );

            w = hb_get_work( subtitle->codec );
            w->fifo_in = subtitle->fifo_in;
            w->fifo_out = subtitle->fifo_raw;
            w->subtitle = subtitle;
            hb_list_add( job->list_work, w );
        }
    }

    /* Set up the video filter fifo pipeline */
    if( !job->indepth_scan )
    {
        if( job->list_filter )
        {
            int filter_count = hb_list_count( job->list_filter );
            int i;
            hb_fifo_t * fifo_in = job->fifo_sync;

            for( i = 0; i < filter_count; i++ )
            {
                hb_filter_object_t * filter = hb_list_item( job->list_filter, i );

                filter->fifo_in = fifo_in;
                filter->fifo_out = hb_fifo_init( FIFO_MINI, FIFO_MINI_WAKE );
                fifo_in = filter->fifo_out;
            }
            job->fifo_render = fifo_in;
        }
        else if ( !job->list_filter )
        {
            hb_log("work: Internal Error: no filters");
            job->fifo_render = NULL;
        }

        /* Video encoder */
        switch( job->vcodec )
        {
        case HB_VCODEC_FFMPEG_MPEG4:
            w = hb_get_work( WORK_ENCAVCODEC );
            w->codec_param = AV_CODEC_ID_MPEG4;
            break;
        case HB_VCODEC_FFMPEG_MPEG2:
            w = hb_get_work( WORK_ENCAVCODEC );
            w->codec_param = AV_CODEC_ID_MPEG2VIDEO;
            break;
        case HB_VCODEC_X264:
            w = hb_get_work( WORK_ENCX264 );
            break;
        case HB_VCODEC_THEORA:
            w = hb_get_work( WORK_ENCTHEORA );
            break;
        }
        // Handle case where there are no filters.  
        // This really should never happen.
        if ( job->fifo_render )
            w->fifo_in  = job->fifo_render;
        else
            w->fifo_in  = job->fifo_sync;

        w->fifo_out = job->fifo_mpeg4;
        w->config   = &job->config;

        hb_list_add( job->list_work, w );

        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            audio = hb_list_item( job->list_audio, i );

            /*
            * Audio Decoder Thread
            */
            if ( audio->priv.fifo_in )
            {
                if ( ( w = hb_codec_decoder( audio->config.in.codec ) ) == NULL )
                {
                    hb_error("Invalid input codec: %d", audio->config.in.codec);
                    *job->die = 1;
                    goto cleanup;
                }
                w->fifo_in  = audio->priv.fifo_in;
                w->fifo_out = audio->priv.fifo_raw;
                w->config   = &audio->priv.config;
                w->audio    = audio;
                w->codec_param = audio->config.in.codec_param;

                hb_list_add( job->list_work, w );
            }

            /*
            * Audio Encoder Thread
            */
            if ( !(audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
            {
                /*
                * Add the encoder thread if not doing AC-3 pass through
                */
                if ( ( w = hb_codec_encoder( audio->config.out.codec ) ) == NULL )
                {
                    hb_error("Invalid audio codec: %#x", audio->config.out.codec);
                    w = NULL;
                    *job->die = 1;
                    goto cleanup;
                }
                w->fifo_in  = audio->priv.fifo_sync;
                w->fifo_out = audio->priv.fifo_out;
                w->config   = &audio->priv.config;
                w->audio    = audio;

                hb_list_add( job->list_work, w );
            }
        }
    }
    
    if( job->chapter_markers && job->chapter_start == job->chapter_end )
    {
        job->chapter_markers = 0;
        hb_log("work: only 1 chapter, disabling chapter markers");
    }

    /* Display settings */
    hb_display_job_info( job );

    /* Init read & write threads */
    if ( reader->init( reader, job ) )
    {
        hb_error( "Failure to initialise thread '%s'", reader->name );
        *job->die = 1;
        goto cleanup;
    }
    reader->done = &job->done;
    reader->thread = hb_thread_init( reader->name, ReadLoop, reader, HB_NORMAL_PRIORITY );

    job->done = 0;

    if( job->list_filter && !job->indepth_scan )
    {
        int filter_count = hb_list_count( job->list_filter );
        int i;

        for( i = 0; i < filter_count; i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );

            if( !filter ) continue;

            // Filters were initialized earlier, so we just need
            // to start the filter's thread
            filter->done = &job->done;
            filter->thread = hb_thread_init( filter->name, filter_loop, filter,
                                             HB_LOW_PRIORITY );
        }
    }

    /* Launch processing threads */
    for( i = 0; i < hb_list_count( job->list_work ); i++ )
    {
        w = hb_list_item( job->list_work, i );
        w->done = &job->done;
        w->thread_sleep_interval = 10;
        if( w->init( w, job ) )
        {
            hb_error( "Failure to initialise thread '%s'", w->name );
            *job->die = 1;
            goto cleanup;
        }
        w->thread = hb_thread_init( w->name, work_loop, w,
                                    HB_LOW_PRIORITY );
    }

    if ( job->indepth_scan )
    {
        muxer = NULL;
        w = sync;
        sync->done = &job->done;
    }
    else
    {
        sync->done = &job->done;
        sync->thread_sleep_interval = 10;
        if( sync->init( w, job ) )
        {
            hb_error( "Failure to initialise thread '%s'", w->name );
            *job->die = 1;
            goto cleanup;
        }
        sync->thread = hb_thread_init( sync->name, work_loop, sync,
                                    HB_LOW_PRIORITY );

        // The muxer requires track information that's set up by the encoder
        // init routines so we have to init the muxer last.
        muxer = hb_muxer_init( job );
        w = muxer;
    }

    hb_buffer_t      * buf_in, * buf_out = NULL;

    while ( !*job->die && !*w->done && w->status != HB_WORK_DONE )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in == NULL )
            continue;
        if ( *job->die )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }

        buf_out = NULL;
        w->status = w->work( w, &buf_in, &buf_out );

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && w->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*job->die )
            {
                if ( hb_fifo_full_wait( w->fifo_out ) )
                {
                    hb_fifo_push( w->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
    }

    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    hb_handle_t * h = job->h;
    hb_state_t state;
    hb_get_state( h, &state );
    
    hb_log("work: average encoding speed for job is %f fps", state.param.working.rate_avg);

    job->done = 1;
    if( muxer != NULL )
    {
        muxer->close( muxer );
        free( muxer );

        if( sync->thread != NULL )
        {
            hb_thread_close( &sync->thread );
            sync->close( sync );
        }
        free( sync );
    }

cleanup:
    /* Stop the write thread (thread_close will block until the muxer finishes) */
    job->done = 1;

    // Close render filter pipeline
    if( job->list_filter )
    {
        int filter_count = hb_list_count( job->list_filter );
        int i;

        for( i = 0; i < filter_count; i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );

            if( !filter ) continue;

            if( filter->thread != NULL )
            {
                hb_thread_close( &filter->thread );
            }
            filter->close( filter );
        }
    }

    /* Close work objects */
    while( ( w = hb_list_item( job->list_work, 0 ) ) )
    {
        hb_list_rem( job->list_work, w );
        if( w->thread != NULL )
        {
            hb_thread_close( &w->thread );
            w->close( w );
        }
        free( w );
    }

    hb_list_close( &job->list_work );

    /* Stop the read thread */
    if( reader->thread != NULL )
    {
        hb_thread_close( &reader->thread );
        reader->close( reader );
    }
    free( reader );

    /* Close fifos */
    hb_fifo_close( &job->fifo_mpeg2 );
    hb_fifo_close( &job->fifo_raw );
    hb_fifo_close( &job->fifo_sync );
    hb_fifo_close( &job->fifo_mpeg4 );

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( job->list_subtitle, i );
        if( subtitle )
        {
            hb_fifo_close( &subtitle->fifo_in );
            hb_fifo_close( &subtitle->fifo_raw );
            hb_fifo_close( &subtitle->fifo_sync );
            hb_fifo_close( &subtitle->fifo_out );
        }
    }
    for( i = 0; i < hb_list_count( job->list_audio ); i++ )
    {
        audio = hb_list_item( job->list_audio, i );
        if( audio->priv.fifo_in != NULL )
            hb_fifo_close( &audio->priv.fifo_in );
        if( audio->priv.fifo_raw != NULL )
            hb_fifo_close( &audio->priv.fifo_raw );
        if( audio->priv.fifo_sync != NULL )
            hb_fifo_close( &audio->priv.fifo_sync );
        if( audio->priv.fifo_out != NULL )
            hb_fifo_close( &audio->priv.fifo_out );
    }

    if( job->list_filter )
    {
        for( i = 0; i < hb_list_count( job->list_filter ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            hb_fifo_close( &filter->fifo_out );
        }
    }

    if( job->indepth_scan )
    {
        /* Before closing the title print out our subtitle stats if we need to
         * find the highest and lowest. */
        for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
        {
            subtitle = hb_list_item( job->list_subtitle, i );

            hb_log( "Subtitle track %d (id 0x%x) '%s': %d hits (%d forced)",
                    subtitle->track, subtitle->id, subtitle->lang,
                    subtitle->hits, subtitle->forced_hits );

            if( subtitle->hits == 0 )
                continue;

            if( subtitle_highest < subtitle->hits )
            {
                subtitle_highest = subtitle->hits;
            }

            if( subtitle_lowest == 0 ||
                subtitle_lowest > subtitle->hits )
            {
                subtitle_lowest = subtitle->hits;
                subtitle_lowest_id = subtitle->id;
            }

            // pick the track with fewest forced hits
            if( subtitle->forced_hits > 0 &&
                ( subtitle_forced_hits == 0 ||
                  subtitle_forced_hits > subtitle->forced_hits ) )
            {
                subtitle_forced_id = subtitle->id;
                subtitle_forced_hits = subtitle->forced_hits;
            }
        }

        if( subtitle_forced_id && job->select_subtitle_config.force )
        {
            /* If there is a subtitle stream with forced subtitles and forced-only
             * is set, then select it in preference to the lowest. */
            subtitle_hit = subtitle_forced_id;
            hb_log( "Found a subtitle candidate with id 0x%x (contains forced subs)",
                    subtitle_hit );
        }
        else if( subtitle_lowest > 0 &&
                 subtitle_lowest < ( subtitle_highest * 0.1 ) )
        {
            /* OK we have more than one, and the lowest is lower,
             * but how much lower to qualify for turning it on by
             * default?
             *
             * Let's say 10% as a default. */
            subtitle_hit = subtitle_lowest_id;
            hb_log( "Found a subtitle candidate with id 0x%x", subtitle_hit );
        }
        else
        {
            hb_log( "No candidate detected during subtitle scan" );
        }

        for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if( subtitle->id == subtitle_hit )
            {
                subtitle->config = job->select_subtitle_config;
                // Remove from list since we are taking ownership
                // of the subtitle.
                hb_list_rem( job->list_subtitle, subtitle );
                interjob->select_subtitle = subtitle;
                break;
            }
        }
    }

    hb_buffer_pool_free();
    hb_job_close( &job );
}

static inline void copy_chapter( hb_buffer_t * dst, hb_buffer_t * src )
{
    // Propagate any chapter breaks for the worker if and only if the
    // output frame has the same time stamp as the input frame (any
    // worker that delays frames has to propagate the chapter marks itself
    // and workers that move chapter marks to a different time should set
    // 'src' to NULL so that this code won't generate spurious duplicates.)
    if( src && dst && src->s.start == dst->s.start)
    {
        // restore log below to debug chapter mark propagation problems
        //hb_log("work %s: Copying Chapter Break @ %"PRId64, w->name, src->s.start);
        dst->s.new_chap = src->s.new_chap;
    }
}

/**
 * Performs the work object's specific work function.
 * Loops calling work function for associated work object. Sleeps when fifo is full.
 * Monitors work done indicator.
 * Exits loop when work indiactor is set.
 * @param _w Handle to work object.
 */
static void work_loop( void * _w )
{
    hb_work_object_t * w = _w;
    hb_buffer_t      * buf_in = NULL, * buf_out = NULL;

    while( !*w->done && w->status != HB_WORK_DONE )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in == NULL )
            continue;
        if ( *w->done )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }
        // Invalidate buf_out so that if there is no output
        // we don't try to pass along junk.
        buf_out = NULL;
        w->status = w->work( w, &buf_in, &buf_out );

        copy_chapter( buf_out, buf_in );

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && w->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*w->done )
            {
                if ( hb_fifo_full_wait( w->fifo_out ) )
                {
                    hb_fifo_push( w->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
    }
    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    // Consume data in incoming fifo till job complete so that
    // residual data does not stall the pipeline
    while( !*w->done )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in != NULL )
            hb_buffer_close( &buf_in );
    }
}

/**
 * Performs the filter object's specific work function.
 * Loops calling work function for associated filter object. 
 * Sleeps when fifo is full.
 * Monitors work done indicator.
 * Exits loop when work indiactor is set.
 * @param _w Handle to work object.
 */
static void filter_loop( void * _f )
{
    hb_filter_object_t * f = _f;
    hb_buffer_t      * buf_in, * buf_out;

    while( !*f->done && f->status != HB_FILTER_DONE )
    {
        buf_in = hb_fifo_get_wait( f->fifo_in );
        if ( buf_in == NULL )
            continue;

        // Filters can drop buffers.  Remember chapter information
        // so that it can be propagated to the next buffer
        if ( buf_in->s.new_chap )
        {
            f->chapter_time = buf_in->s.start;
            f->chapter_val = buf_in->s.new_chap;
        }
        if ( *f->done )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }

        buf_out = NULL;
        f->status = f->work( f, &buf_in, &buf_out );

        if ( buf_out && f->chapter_val && f->chapter_time <= buf_out->s.start )
        {
            buf_out->s.new_chap = f->chapter_val;
            f->chapter_val = 0;
        }

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && f->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*f->done )
            {
                if ( hb_fifo_full_wait( f->fifo_out ) )
                {
                    hb_fifo_push( f->fifo_out, buf_out );
                    break;
                }
            }
        }
    }
    // Consume data in incoming fifo till job complete so that
    // residual data does not stall the pipeline
    while( !*f->done )
    {
        buf_in = hb_fifo_get_wait( f->fifo_in );
        if ( buf_in != NULL )
            hb_buffer_close( &buf_in );
    }
}

