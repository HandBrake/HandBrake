/* $Id: work.c,v 1.43 2005/03/17 16:38:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"
#include "dca.h"
#include "libavformat/avformat.h"

typedef struct
{
    hb_list_t * jobs;
    hb_job_t  ** current_job;
    int         cpu_count;
    int       * error;
    volatile int * die;

} hb_work_t;

static void work_func();
static void do_job( hb_job_t *, int cpu_count );
static void work_loop( void * );

#define FIFO_CPU_MULT 8

/**
 * Allocates work object and launches work thread with work_func.
 * @param jobs Handle to hb_list_t.
 * @param cpu_count Humber of CPUs found in system.
 * @param die Handle to user inititated exit indicator.
 * @param error Handle to error indicator.
 */
hb_thread_t * hb_work_init( hb_list_t * jobs, int cpu_count,
                            volatile int * die, int * error, hb_job_t ** job )
{
    hb_work_t * work = calloc( sizeof( hb_work_t ), 1 );

    work->jobs      = jobs;
    work->current_job = job;
    work->cpu_count = cpu_count;
    work->die       = die;
    work->error     = error;

    return hb_thread_init( "work", work_func, work, HB_LOW_PRIORITY );
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

    while( !*work->die && ( job = hb_list_item( work->jobs, 0 ) ) )
    {
        hb_list_rem( work->jobs, job );
        job->die = work->die;
        *(work->current_job) = job;
        do_job( job, work->cpu_count );
        *(work->current_job) = NULL;
    }

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
        case HB_ACODEC_DCA:  return hb_get_work( WORK_DECDCA );
        case HB_ACODEC_MPGA: return hb_get_work( WORK_DECAVCODEC );
        case HB_ACODEC_LPCM: return hb_get_work( WORK_DECLPCM );
        case HB_ACODEC_FFMPEG: return hb_get_work( WORK_DECAVCODECAI );
    }
    return NULL;
}

hb_work_object_t * hb_codec_encoder( int codec )
{
    switch( codec )
    {
        case HB_ACODEC_FAAC:   return hb_get_work( WORK_ENCFAAC );
        case HB_ACODEC_LAME:   return hb_get_work( WORK_ENCLAME );
        case HB_ACODEC_VORBIS: return hb_get_work( WORK_ENCVORBIS );
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
    
    hb_log( "   + %s", title->dvd );

    hb_log( "   + title %d, chapter(s) %d to %d", title->index,
            job->chapter_start, job->chapter_end );

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
            
            if( job->color_matrix )
                hb_log( "     + custom color matrix: %s", job->color_matrix == 1 ? "ITU Bt.601 (SD)" : "ITU Bt.709 (HD)");
            break;

        case HB_MUX_AVI:
            hb_log("   + container: AVI");
            break;

        case HB_MUX_MKV:
            hb_log("   + container: Matroska (.mkv)");
            break;

        case HB_MUX_OGM:
            hb_log("   + conttainer: Ogg Media (.ogm)");
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
    
    if( !job->cfr )
    {
        hb_log( "   + frame rate: same as source (around %.3f fps)",
            (float) title->rate / (float) title->rate_base );
    }
    else
    {
        static const char *frtypes[] = {
            "", "constant", "peak rate limited to"
        };
        hb_log( "   + frame rate: %.3f fps -> %s %.3f fps",
            (float) title->rate / (float) title->rate_base, frtypes[job->cfr],
            (float) job->vrate / (float) job->vrate_base );
    }

    if( job->anamorphic.mode )
    {
        hb_log( "   + %s anamorphic", job->anamorphic.mode == 1 ? "strict" : "loose" );
        hb_log( "     + storage dimensions: %d * %d -> %d * %d, crop %d/%d/%d/%d",
                    title->width, title->height, job->width, job->height,
                    job->crop[0], job->crop[1], job->crop[2], job->crop[3] );
        hb_log( "     + pixel aspect ratio: %i / %i", job->anamorphic.par_width, job->anamorphic.par_height );
        hb_log( "     + display dimensions: %.0f * %i",
            (float)( job->width * job->anamorphic.par_width / job->anamorphic.par_height ), job->height );
    }
    else
    {
        hb_log( "   + dimensions: %d * %d -> %d * %d, crop %d/%d/%d/%d",
                title->width, title->height, job->width, job->height,
                job->crop[0], job->crop[1], job->crop[2], job->crop[3] );
    }

    if ( job->grayscale )
        hb_log( "   + grayscale mode" );

    if( hb_list_count( job->filters ) )
    {
        hb_log("   + %s", hb_list_count( job->filters) > 1 ? "filters" : "filter" );
        for( i = 0; i < hb_list_count( job->filters ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            if (filter->settings)
                hb_log("     + %s (%s)", filter->name, filter->settings);
            else
                hb_log("     + %s (default settings)", filter->name);
        }
    }
    
    if( !job->indepth_scan )
    {
        /* Video encoder */
        switch( job->vcodec )
        {
            case HB_VCODEC_FFMPEG:
                hb_log( "   + encoder: FFmpeg" );
                break;

            case HB_VCODEC_XVID:
                hb_log( "   + encoder: XviD" );
                break;

            case HB_VCODEC_X264:
                hb_log( "   + encoder: x264" );
                if( job->x264opts != NULL && *job->x264opts != '\0' )
                    hb_log( "     + options: %s", job->x264opts);
                break;

            case HB_VCODEC_THEORA:
                hb_log( "   + encoder: Theora" );
                break;
        }

        if( job->vquality >= 0.0 && job->vquality <= 1.0 )
        {
            hb_log( "     + quality: %.2f", job->vquality );
        }
        else if( job->vquality > 1 )
        {
            hb_log( "     + quality: %.2f %s", job->vquality, job->crf && job->vcodec == HB_VCODEC_X264 ? "(RF)" : "(QP)" ); 
        }
        else
        {
            hb_log( "     + bitrate: %d kbps, pass: %d", job->vbitrate, job->pass );
        }
    }

    for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle =  hb_list_item( title->list_subtitle, i );

        if( subtitle )
        {
            hb_log( " * subtitle track %i, %s (id %x) %s [%s] -> %s ", subtitle->track, subtitle->lang, subtitle->id,
                    subtitle->format == PICTURESUB ? "Picture" : "Text",
                    subtitle->source == VOBSUB ? "VOBSUB" : (subtitle->source == CCSUB ? "CC" : "SRT"),
                    subtitle->dest == RENDERSUB ? "Render/Burn in" : "Pass-Through");
        }
    }

    if( !job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );

            hb_log( " * audio track %d", audio->config.out.track );
            
            if( audio->config.out.name )
                hb_log( "   + name: %s", audio->config.out.name );

            hb_log( "   + decoder: %s (track %d, id %x)", audio->config.lang.description, audio->config.in.track + 1, audio->id );

            if( ( audio->config.in.codec == HB_ACODEC_AC3 ) || ( audio->config.in.codec == HB_ACODEC_DCA) )
            {
                hb_log( "     + bitrate: %d kbps, samplerate: %d Hz", audio->config.in.bitrate / 1000, audio->config.in.samplerate );
            }

            if( (audio->config.out.codec != HB_ACODEC_AC3) && (audio->config.out.codec != HB_ACODEC_DCA) )
            {
                for (j = 0; j < hb_audio_mixdowns_count; j++)
                {
                    if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                        hb_log( "   + mixdown: %s", hb_audio_mixdowns[j].human_readable_name );
                        break;
                    }
                }
            }

            if ( audio->config.out.dynamic_range_compression && (audio->config.out.codec != HB_ACODEC_AC3) && (audio->config.out.codec != HB_ACODEC_DCA))
            {
                hb_log("   + dynamic range compression: %f", audio->config.out.dynamic_range_compression);
            }
            
            if( (audio->config.out.codec == HB_ACODEC_AC3) || (audio->config.out.codec == HB_ACODEC_DCA) )
            {
                hb_log( "   + %s passthrough", (audio->config.out.codec == HB_ACODEC_AC3) ?
                    "AC3" : "DCA" );
            }
            else
            {
                hb_log( "   + encoder: %s", ( audio->config.out.codec == HB_ACODEC_FAAC ) ?
                    "faac" : ( ( audio->config.out.codec == HB_ACODEC_LAME ) ? "lame" :
                    "vorbis" ) );
                hb_log( "     + bitrate: %d kbps, samplerate: %d Hz", audio->config.out.bitrate, audio->config.out.samplerate );            
            }
        }
    }
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
 * @param cpu_count number of CPUs found in system.
 */
static void do_job( hb_job_t * job, int cpu_count )
{
    hb_title_t    * title;
    int             i, j;
    hb_work_object_t * w;

    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    unsigned int subtitle_highest = 0;
    unsigned int subtitle_highest_id = 0;
    unsigned int subtitle_lowest = -1;
    unsigned int subtitle_lowest_id = 0;
    unsigned int subtitle_forced_id = 0;
    unsigned int subtitle_hit = 0;

    title = job->title;

    job->list_work = hb_list_init();

    hb_log( "starting job" );

    if( job->anamorphic.mode )
    {
        hb_set_anamorphic_size(job, &job->width, &job->height, &job->anamorphic.par_width, &job->anamorphic.par_height);

        if( job->vcodec == HB_VCODEC_FFMPEG )
        {
            /* Just to make working with ffmpeg even more fun,
               lavc's MPEG-4 encoder can't handle PAR values >= 255,
               even though AVRational does. Adjusting downwards
               distorts the display aspect slightly, but such is life. */
            while ((job->anamorphic.par_width & ~0xFF) ||
                   (job->anamorphic.par_height & ~0xFF))
            {
                job->anamorphic.par_width >>= 1;
                job->anamorphic.par_height >>= 1;
            }
        }
    }
    
    /* Keep width and height within these boundaries,
       but ignore for anamorphic. For "loose" anamorphic encodes,
       this stuff is covered in the pixel_ratio section above.    */
    if ( job->maxHeight && ( job->height > job->maxHeight ) && ( !job->anamorphic.mode ) )
    {
        job->height = job->maxHeight;
        hb_fix_aspect( job, HB_KEEP_HEIGHT );
        hb_log( "Height out of bounds, scaling down to %i", job->maxHeight );
        hb_log( "New dimensions %i * %i", job->width, job->height );
    }
    if ( job->maxWidth && ( job->width > job->maxWidth ) && ( !job->anamorphic.mode ) )
    {
        job->width = job->maxWidth;
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        hb_log( "Width out of bounds, scaling down to %i", job->maxWidth );
        hb_log( "New dimensions %i * %i", job->width, job->height );
    }

    if( job->mux & HB_MUX_AVI )
    {
        // The concept of variable frame rate video was a bit too advanced
        // for Microsoft so AVI doesn't support it. Since almost all dvd
        // video is VFR we have to convert it to constant frame rate to
        // put it in an AVI container. So duplicate, drop and
        // otherwise trash video frames to appease the gods of Redmond.
        job->cfr = 1;
    }

    if ( job->cfr == 0 )
    {
        /* Ensure we're using "Same as source" FPS */
        job->vrate_base = title->rate_base;
    }

    job->fifo_mpeg2  = hb_fifo_init( 256 );
    job->fifo_raw    = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_sync   = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_render = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_mpeg4  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );

    /* Synchronization */
    hb_list_add( job->list_work, ( w = hb_get_work( WORK_SYNC ) ) );
    w->fifo_in  = NULL;
    w->fifo_out = NULL;

    /* Video decoder */
    int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
    hb_list_add( job->list_work, ( w = hb_get_work( vcodec ) ) );
    w->codec_param = title->video_codec_param;
    w->fifo_in  = job->fifo_mpeg2;
    w->fifo_out = job->fifo_raw;

    /* Video renderer */
    hb_list_add( job->list_work, ( w = hb_get_work( WORK_RENDER ) ) );
    w->fifo_in  = job->fifo_sync;
    w->fifo_out = job->fifo_render;

    if( !job->indepth_scan )
    {

        /* Video encoder */
        switch( job->vcodec )
        {
        case HB_VCODEC_FFMPEG:
            w = hb_get_work( WORK_ENCAVCODEC );
            break;
        case HB_VCODEC_XVID:
            w = hb_get_work( WORK_ENCXVID );
            break;
        case HB_VCODEC_X264:
            w = hb_get_work( WORK_ENCX264 );
            break;
        case HB_VCODEC_THEORA:
            w = hb_get_work( WORK_ENCTHEORA );
            break;
        }
        w->fifo_in  = job->fifo_render;
        w->fifo_out = job->fifo_mpeg4;
        w->config   = &job->config;

        hb_list_add( job->list_work, w );
    }

    if( job->select_subtitle && !job->indepth_scan )
    {
        /*
         * Must be second pass of a two pass with subtitle scan enabled, so
         * add the subtitle that we found on the first pass for use in this
         * pass.
         */
        if (*(job->select_subtitle))
        {
            hb_list_add( title->list_subtitle, *( job->select_subtitle ) );
        }
    }

    for( i=0; i < hb_list_count(title->list_subtitle); i++ )
    {
        subtitle =  hb_list_item( title->list_subtitle, i );

        if( subtitle )
        {
            subtitle->fifo_in  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
            subtitle->fifo_raw = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
            subtitle->fifo_out = hb_fifo_init( FIFO_CPU_MULT * cpu_count );

            /*
             * Disable forced subtitles if we didn't find any in the scan
             * so that we display normal subtitles instead.
             *
             * select_subtitle implies that we did a scan.
             */
            if( !job->indepth_scan && job->subtitle_force &&
                job->select_subtitle )
            {
                if( subtitle->forced_hits == 0 )
                {
                    job->subtitle_force = 0;
                }
            }

            if( !job->indepth_scan || job->subtitle_force ) {
                /*
                 * Don't add threads for subtitles when we are scanning, unless
                 * looking for forced subtitles.
                 */
                w = hb_get_work( WORK_DECSUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                hb_list_add( job->list_work, w );
            }
        }
    }

    if( !job->indepth_scan )
    {
    // if we are doing passthru, and the input codec is not the same as the output
    // codec, then remove this audio from the job. If we're not doing passthru and
    // the input codec is the 'internal' ffmpeg codec, make sure that only one
    // audio references that audio stream since the codec context is specific to
    // the audio id & multiple copies of the same stream will garble the audio
    // or cause aborts.
    uint8_t aud_id_uses[MAX_STREAMS];
    memset( aud_id_uses, 0, sizeof(aud_id_uses) );
    for( i = 0; i < hb_list_count( title->list_audio ); )
    {
        audio = hb_list_item( title->list_audio, i );
        if( ( ( audio->config.out.codec == HB_ACODEC_AC3 ) && ( audio->config.in.codec != HB_ACODEC_AC3 ) ) ||
            ( ( audio->config.out.codec == HB_ACODEC_DCA ) && ( audio->config.in.codec != HB_ACODEC_DCA ) ) )
        {
            hb_log( "Passthru requested and input codec is not the same as output codec for track %d",
                    audio->config.out.track );
            hb_list_rem( title->list_audio, audio );
            free( audio );
            continue;
        }
        if ( audio->config.in.codec == HB_ACODEC_FFMPEG )
        {
            if ( aud_id_uses[audio->id] )
            {
                hb_log( "Multiple decodes of audio id %d, removing track %d",
                        audio->id, audio->config.out.track );
                hb_list_rem( title->list_audio, audio );
                free( audio );
                continue;
            }
            ++aud_id_uses[audio->id];
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         */
        audio->config.out.track = i++;
    }

    int requested_mixdown = 0;
    int requested_mixdown_index = 0;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );

        if( audio->config.out.codec != audio->config.in.codec )
        {
            /* sense-check the current mixdown options */

            /* log the requested mixdown */
            for (j = 0; j < hb_audio_mixdowns_count; j++) {
                if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                    requested_mixdown = audio->config.out.mixdown;
                    requested_mixdown_index = j;
                }
            }

            /* sense-check the requested mixdown */

            if( audio->config.out.mixdown == 0 &&
                audio->config.out.codec != HB_ACODEC_AC3 && 
                audio->config.out.codec != HB_ACODEC_DCA )
            {
                /*
                 * Mixdown wasn't specified and this is not pass-through,
                 * set a default mixdown of stereo.
                 */
                audio->config.out.mixdown = HB_AMIXDOWN_STEREO;
            }

            // Here we try to sanitize the audio input to output mapping.
            // Constraints are:
            //   1. only the AC3 & DCA decoder libraries currently support mixdown
            //   2. the lame encoder library only supports stereo.
            // So if the encoder is lame we need the output to be stereo (or multichannel
            // matrixed into stereo like dpl). If the decoder is not AC3 or DCA the
            // encoder has to handle the input format since we can't do a mixdown.
#define CAN_MIXDOWN(a) ( a->config.in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA) )
#define STEREO_ONLY(a) ( a->config.out.codec & HB_ACODEC_LAME )

            switch (audio->config.in.channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK)
            {
                // stereo input or something not handled below
                default:
                case HB_INPUT_CH_LAYOUT_STEREO:
                    // mono gets mixed up to stereo & more than stereo gets mixed down
                    if ( STEREO_ONLY( audio ) ||
                         audio->config.out.mixdown > HB_AMIXDOWN_STEREO)
                    {
                        audio->config.out.mixdown = HB_AMIXDOWN_STEREO;
                    }
                    break;

                // mono input
                case HB_INPUT_CH_LAYOUT_MONO:
                    if ( STEREO_ONLY( audio ) )
                    {
                        if ( !CAN_MIXDOWN( audio ) )
                        {
                            // XXX we're hosed - we can't mix up & lame can't handle
                            // the input format. The user shouldn't be able to make
                            // this choice. It's too late to do anything about it now
                            // so complain in the log & let things abort in lame.
                            hb_log( "ERROR - can't use lame mp3 audio output with "
                                    "mono audio stream %x - output will be messed up",
                                    audio->id );
                        }
                        audio->config.out.mixdown = HB_AMIXDOWN_STEREO;
                    }
                    else
                    {
                        // everything else passes through
                        audio->config.out.mixdown = HB_AMIXDOWN_MONO;
                    }
                    break;

                // dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input
                // the A52 flags don't allow for a way to distinguish between DPL1 and
                // DPL2 on a DVD so we always assume a DPL1 source for A52_DOLBY.
                case HB_INPUT_CH_LAYOUT_DOLBY:
                    if ( STEREO_ONLY( audio ) || !CAN_MIXDOWN( audio ) ||
                         audio->config.out.mixdown > HB_AMIXDOWN_DOLBY )
                    {
                        audio->config.out.mixdown = HB_AMIXDOWN_DOLBY;
                    }
                    break;

                // 4 channel discrete
                case HB_INPUT_CH_LAYOUT_2F2R:
                case HB_INPUT_CH_LAYOUT_3F1R:
                    if ( CAN_MIXDOWN( audio ) )
                    {
                        if ( STEREO_ONLY( audio ) ||
                             audio->config.out.mixdown > HB_AMIXDOWN_DOLBY )
                        {
                            audio->config.out.mixdown = HB_AMIXDOWN_DOLBY;
                        }
                    }
                    else
                    {
                        // XXX we can't mixdown & don't have any way to specify
                        // 4 channel discrete output so we're hosed.
                        hb_log( "ERROR - can't handle 4 channel discrete audio stream "
                                "%x - output will be messed up", audio->id );
                    }
                    break;

                // 5 or 6 channel discrete
                case HB_INPUT_CH_LAYOUT_3F2R:
                    if ( CAN_MIXDOWN( audio ) )
                    {
                        if ( STEREO_ONLY( audio ) )
                        {
                            if ( audio->config.out.mixdown < HB_AMIXDOWN_STEREO )
                            {
                                audio->config.out.mixdown = HB_AMIXDOWN_STEREO;
                            }
                            else if ( audio->config.out.mixdown > HB_AMIXDOWN_DOLBYPLII )
                            {
                                audio->config.out.mixdown = HB_AMIXDOWN_DOLBYPLII;
                            }
                        }
                        else if ( ! ( audio->config.in.channel_layout &
                                        HB_INPUT_CH_LAYOUT_HAS_LFE ) )
                        {
                            // we don't do 5 channel discrete so mixdown to DPLII
                            audio->config.out.mixdown = HB_AMIXDOWN_DOLBYPLII;
                        }
                    }
                    else if ( ! ( audio->config.in.channel_layout &
                                        HB_INPUT_CH_LAYOUT_HAS_LFE ) )
                    {
                        // XXX we can't mixdown & don't have any way to specify
                        // 5 channel discrete output so we're hosed.
                        hb_log( "ERROR - can't handle 5 channel discrete audio stream "
                                "%x - output will be messed up", audio->id );
                    }
                    else
                    {
                        // we can't mixdown so force 6 channel discrete
                        audio->config.out.mixdown = HB_AMIXDOWN_6CH;
                    }
                    break;
            }

            /* log the output mixdown */
            for (j = 0; j < hb_audio_mixdowns_count; j++) {
                if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                    if ( audio->config.out.mixdown != requested_mixdown )
                    {
                        hb_log("work: sanitizing track %i mixdown %s to %s", i, hb_audio_mixdowns[requested_mixdown_index].human_readable_name, hb_audio_mixdowns[j].human_readable_name);
                    }
                    break;
                }
            }
        }

        if (audio->config.out.codec == HB_ACODEC_VORBIS)
            audio->priv.config.vorbis.language = audio->config.lang.simple;

        /* set up the audio work structures */
        audio->priv.fifo_in   = hb_fifo_init( 32 );
        audio->priv.fifo_raw  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
        audio->priv.fifo_sync = hb_fifo_init( 32 );
        audio->priv.fifo_out  = hb_fifo_init( 8 * FIFO_CPU_MULT * cpu_count );


        /*
         * Audio Decoder Thread
         */
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

        /*
         * Audio Encoder Thread
         */
        if( audio->config.out.codec != HB_ACODEC_AC3 &&
            audio->config.out.codec != HB_ACODEC_DCA )
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

    /* Display settings */
    hb_display_job_info( job );

    /* Init read & write threads */
    job->reader = hb_reader_init( job );


    job->done = 0;

    /* Launch processing threads */
    for( i = 1; i < hb_list_count( job->list_work ); i++ )
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

    // The muxer requires track information that's set up by the encoder
    // init routines so we have to init the muxer last.
    job->muxer = job->indepth_scan? NULL : hb_muxer_init( job );

    w = hb_list_item( job->list_work, 0 );
    w->thread_sleep_interval = 10;
    w->init( w, job );
    while( !*job->die )
    {
        if ( ( w->status = w->work( w, NULL, NULL ) ) == HB_WORK_DONE )
        {
            break;
        }
        hb_snooze( w->thread_sleep_interval );
    }
    hb_list_rem( job->list_work, w );
    w->close( w );
    free( w );

    hb_handle_t * h = job->h;
    hb_state_t state;
    hb_get_state( h, &state );
    
    hb_log("work: average encoding speed for job is %f fps", state.param.working.rate_avg);

cleanup:
    /* Stop the write thread (thread_close will block until the muxer finishes) */
    if( job->muxer != NULL )
        hb_thread_close( &job->muxer );

    job->done = 1;

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
    if( job->reader != NULL )
        hb_thread_close( &job->reader );

    /* Close fifos */
    hb_fifo_close( &job->fifo_mpeg2 );
    hb_fifo_close( &job->fifo_raw );
    hb_fifo_close( &job->fifo_sync );
    hb_fifo_close( &job->fifo_render );
    hb_fifo_close( &job->fifo_mpeg4 );

    for (i=0; i < hb_list_count(title->list_subtitle); i++) {
        subtitle =  hb_list_item( title->list_subtitle, i);
        if( subtitle )
        {
            hb_fifo_close( &subtitle->fifo_in );
            hb_fifo_close( &subtitle->fifo_raw );
        }
    }
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->priv.fifo_in != NULL )
            hb_fifo_close( &audio->priv.fifo_in );
        if( audio->priv.fifo_raw != NULL )
            hb_fifo_close( &audio->priv.fifo_raw );
        if( audio->priv.fifo_sync != NULL )
            hb_fifo_close( &audio->priv.fifo_sync );
        if( audio->priv.fifo_out != NULL )
            hb_fifo_close( &audio->priv.fifo_out );
    }

    if( job->indepth_scan )
    {
        /*
         * Before closing the title print out our subtitle stats if we need to
         * Find the highest and lowest.
         */
        for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
        {
            subtitle =  hb_list_item( title->list_subtitle, i );
            hb_log( "Subtitle stream 0x%x '%s': %d hits (%d forced)",
                    subtitle->id, subtitle->lang, subtitle->hits,
                    subtitle->forced_hits );
            if( subtitle->hits > subtitle_highest )
            {
                subtitle_highest = subtitle->hits;
                subtitle_highest_id = subtitle->id;
            }

            if( subtitle->hits < subtitle_lowest )
            {
                subtitle_lowest = subtitle->hits;
                subtitle_lowest_id = subtitle->id;
            }

            if( subtitle->forced_hits > 0 )
            {
                if( subtitle_forced_id == 0 )
                {
                    subtitle_forced_id = subtitle->id;
                }
            }
        }

        if( job->native_language ) {
            /*
             * We still have a native_language, so the audio and subtitles are
             * different, so in this case it is a foreign film and we want to
             * select the subtitle with the highest hits in our language.
             */
            subtitle_hit = subtitle_highest_id;
            hb_log( "Found a native-language subtitle id 0x%x", subtitle_hit);
        } else {
            if( subtitle_forced_id )
            {
                /*
                 * If there are any subtitle streams with forced subtitles
                 * then select it in preference to the lowest.
                 */
                subtitle_hit = subtitle_forced_id;
                hb_log("Found a subtitle candidate id 0x%x (contains forced subs)",
                       subtitle_hit);
            } else if( subtitle_lowest < subtitle_highest )
            {
                /*
                 * OK we have more than one, and the lowest is lower,
                 * but how much lower to qualify for turning it on by
                 * default?
                 *
                 * Let's say 10% as a default.
                 */
                if( subtitle_lowest < ( subtitle_highest * 0.1 ) )
                {
                    subtitle_hit = subtitle_lowest_id;
                    hb_log( "Found a subtitle candidate id 0x%x",
                            subtitle_hit );
                } else {
                    hb_log( "No candidate subtitle detected during subtitle-scan");
                }
            }
        }
    }

    if( job->select_subtitle )
    {
        if( job->indepth_scan )
        {
            for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
            {
                subtitle =  hb_list_item( title->list_subtitle, i );
                if( subtitle->id == subtitle_hit )
                {
                    hb_list_rem( title->list_subtitle, subtitle );
                    *( job->select_subtitle ) = subtitle;
                }
            }
        } else {
            /*
             * Must be the end of pass 0 or 2 - we don't need this anymore.
             *
             * Have to put the subtitle list back together in the title though
             * or the GUI will have a hissy fit.
             */
            free( job->select_subtitle );
            job->select_subtitle = NULL;
        }
    }

    if( job->filters )
    {
        for( i = 0; i < hb_list_count( job->filters ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            hb_filter_close( &filter );
        }
        hb_list_close( &job->filters );
    }

    hb_buffer_pool_free();

    hb_title_close( &job->title );
    free( job );
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
    hb_buffer_t      * buf_in, * buf_out;

    while( !*w->done )
    {
#if 0
        hb_lock( job->pause );
        hb_unlock( job->pause );
#endif
        if( hb_fifo_is_full( w->fifo_out ) ||
//        if( (hb_fifo_percent_full( w->fifo_out ) > 0.8) ||
            !( buf_in = hb_fifo_get( w->fifo_in ) ) )
        {
            hb_snooze( w->thread_sleep_interval );
//			w->thread_sleep_interval += 1;
            continue;
        }
//		w->thread_sleep_interval = MAX(1, (w->thread_sleep_interval - 1));

        w->status = w->work( w, &buf_in, &buf_out );

        // Propagate any chapter breaks for the worker if and only if the
        // output frame has the same time stamp as the input frame (any
        // worker that delays frames has to propagate the chapter marks itself
        // and workers that move chapter marks to a different time should set
        // 'buf_in' to NULL so that this code won't generate spurious duplicates.)
        if( buf_in && buf_out && buf_in->new_chap && buf_in->start == buf_out->start)
        {
            // restore log below to debug chapter mark propagation problems
            //hb_log("work %s: Copying Chapter Break @ %lld", w->name, buf_in->start);
            buf_out->new_chap = buf_in->new_chap;
        }

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if( buf_out )
        {
            hb_fifo_push( w->fifo_out, buf_out );
        }
    }
}
