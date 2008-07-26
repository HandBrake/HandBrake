/* $Id: work.c,v 1.43 2005/03/17 16:38:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"
#include "dca.h"

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
    hb_work_object_t * final_w = NULL;

    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    int done;
    unsigned int subtitle_highest = 0;
    unsigned int subtitle_highest_id = 0;
    unsigned int subtitle_lowest = -1;
    unsigned int subtitle_lowest_id = 0;
    unsigned int subtitle_forced_id = 0;
    unsigned int subtitle_hit = 0;

    title = job->title;

    job->list_work = hb_list_init();

    hb_log( "starting job" );
    hb_log( " + device %s", title->dvd );
    hb_log( " + title %d, chapter(s) %d to %d", title->index,
            job->chapter_start, job->chapter_end );
    if ( job->pixel_ratio == 1 )
    {
    	/* Correct the geometry of the output movie when using PixelRatio */
    	job->height=title->height-job->crop[0]-job->crop[1];
    	job->width=title->width-job->crop[2]-job->crop[3];
    }
    else if ( job->pixel_ratio == 2 )
    {

        /* While keeping the DVD storage aspect, resize the job width and height
           so they fit into the user's specified dimensions. */
        hb_set_anamorphic_size(job, &job->width, &job->height, &job->pixel_aspect_width, &job->pixel_aspect_height);
    }

    if( job->pixel_ratio && job->vcodec == HB_VCODEC_FFMPEG)
    {
        /* Just to make working with ffmpeg even more fun,
           lavc's MPEG-4 encoder can't handle PAR values >= 255,
           even though AVRational does. Adjusting downwards
           distorts the display aspect slightly, but such is life. */
        while ((job->pixel_aspect_width & ~0xFF) ||
               (job->pixel_aspect_height & ~0xFF))
        {
            job->pixel_aspect_width >>= 1;
            job->pixel_aspect_height >>= 1;
        }
    }

	/* Keep width and height within these boundaries,
	   but ignore for "loose" anamorphic encodes, for
	   which this stuff is covered in the pixel_ratio
	   section right above.*/
	if (job->maxHeight && (job->height > job->maxHeight) && (job->pixel_ratio != 2))
	{
		job->height = job->maxHeight;
		hb_fix_aspect( job, HB_KEEP_HEIGHT );
		hb_log("Height out of bounds, scaling down to %i", job->maxHeight);
		hb_log("New dimensions %i * %i", job->width, job->height);
	}
	if (job->maxWidth && (job->width > job->maxWidth) && (job->pixel_ratio != 2))
	{
		job->width = job->maxWidth;
		hb_fix_aspect( job, HB_KEEP_WIDTH );
		hb_log("Width out of bounds, scaling down to %i", job->maxWidth);
		hb_log("New dimensions %i * %i", job->width, job->height);
	}

    hb_log( " + %dx%d -> %dx%d, crop %d/%d/%d/%d",
            title->width, title->height, job->width, job->height,
            job->crop[0], job->crop[1], job->crop[2], job->crop[3] );

    if ( job->grayscale )
        hb_log( " + grayscale mode" );

    if ( job->vfr )
    {
        job->vrate_base = title->rate_base;

        int detelecine_present = 0;
        if ( job->filters )
        {
            for( i = 0; i < hb_list_count( job->filters ); i++ )
            {
                hb_filter_object_t * filter = hb_list_item( job->filters, i );
                if (filter->id == FILTER_DETELECINE)
                    detelecine_present = 1;
            }
        }

        if (!detelecine_present)
        {
            /* Allocate the filter. */
            hb_filter_object_t * filter =  malloc( sizeof( hb_filter_object_t ) );

            /* Copy in the contents of the detelecine struct. */
            memcpy( filter, &hb_filter_detelecine, sizeof( hb_filter_object_t ) );

            /* Set the name to a copy of the template name so render.c has something to free. */
            filter->name = strdup(hb_filter_detelecine.name);

            /* Add it to the list. */
            hb_list_add( job->filters, filter );

            hb_log("work: VFR mode -- adding detelecine filter");
        }
    }

    if( hb_list_count( job->filters ) )
    {
        hb_log(" + filters");
        for( i = 0; i < hb_list_count( job->filters ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            if (filter->settings)
                hb_log("   + %s (%s)", filter->name, filter->settings);
            else
                hb_log("   + %s (default settings)", filter->name);
        }
    }

    if( job->vfr)
    {
        hb_log( " + video frame rate: %.3f fps -> variable fps", (float) job->vrate /
            (float) job->vrate_base );
    }
    else
    {
        hb_log( " + video frame rate: %.3f fps", (float) job->vrate / (float) job->vrate_base);
    }

    if( job->vquality >= 0.0 && job->vquality <= 1.0 )
    {
        hb_log( " + video quality %.2f", job->vquality );
    }
    else
    {
        hb_log( " + video bitrate %d kbps, pass %d", job->vbitrate, job->pass );
    }

	hb_log (" + PixelRatio: %d, width:%d, height: %d",job->pixel_ratio,job->width, job->height);
    job->fifo_mpeg2  = hb_fifo_init( 128 );
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
    if ( job->indepth_scan )
    {
        /*
         * if we're doing a subtitle scan the last thread in the
         * processing pipeline is render - remember it so we can
         * wait for its completion below.
         */
        final_w = w;
    }

    if( !job->indepth_scan )
    {

        /* Video encoder */
        switch( job->vcodec )
        {
        case HB_VCODEC_FFMPEG:
            hb_log( " + encoder FFmpeg" );
            w = hb_get_work( WORK_ENCAVCODEC );
            break;
        case HB_VCODEC_XVID:
            hb_log( " + encoder XviD" );
            w = hb_get_work( WORK_ENCXVID );
            break;
        case HB_VCODEC_X264:
            hb_log( " + encoder x264" );
            if( job->x264opts != NULL && *job->x264opts != '\0' )
                hb_log( "   + x264 options: %s", job->x264opts);
            w = hb_get_work( WORK_ENCX264 );
            break;
        case HB_VCODEC_THEORA:
            hb_log( " + encoder Theora" );
            w = hb_get_work( WORK_ENCTHEORA );
            break;
        }
        w->fifo_in  = job->fifo_render;
        w->fifo_out = job->fifo_mpeg4;
        w->config   = &job->config;

        hb_list_add( job->list_work, w );

        /*
         * if we're not doing a subtitle scan the last thread in the
         * processing pipeline is the encoder - remember it so we can
         * wait for its completion below.
         */
        final_w = w;
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
            hb_log( " + subtitle %x, %s", subtitle->id, subtitle->lang );

            subtitle->fifo_in  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
            subtitle->fifo_raw = hb_fifo_init( FIFO_CPU_MULT * cpu_count );

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

            if (!job->indepth_scan || job->subtitle_force) {
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
    /* if we are doing passthru, and the input codec is not the same as the output
     * codec, then remove this audio from the job */
    /* otherwise, Bad Things will happen */
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
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         */
        audio->config.out.track = i++;
    }

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        hb_log( " + audio track %d", audio->config.out.track );
        hb_log( "   + input track %d", audio->config.in.track );
        if( (audio->config.out.codec == HB_ACODEC_AC3) || (audio->config.out.codec == HB_ACODEC_DCA) )
        {
            hb_log( "   + %s passthrough", (audio->config.out.codec == HB_ACODEC_AC3) ?
                "AC3" : "DCA" );
        }
        else
        {
            hb_log( "   + audio %d kbps, %d Hz", audio->config.out.bitrate, audio->config.out.samplerate );
            hb_log( "   + encoder %s", ( audio->config.out.codec == HB_ACODEC_FAAC ) ?
            "faac" : ( ( audio->config.out.codec == HB_ACODEC_LAME ) ? "lame" :
            "vorbis" ) );
            if ( audio->config.out.dynamic_range_compression > 1 )
                hb_log("   + dynamic range compression: %f", audio->config.out.dynamic_range_compression);
        }

        hb_log( "     + %x, %s", audio->id, audio->config.lang.description );

        if( audio->config.out.codec != audio->config.in.codec )
        {
            /* sense-check the current mixdown options */

            /* log the requested mixdown */
            for (j = 0; j < hb_audio_mixdowns_count; j++) {
                if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                    hb_log( "       + Requested mixdown: %s (%s)", hb_audio_mixdowns[j].human_readable_name, hb_audio_mixdowns[j].internal_name );
                    break;
                }
            }

            /* sense-check the requested mixdown */

            if( audio->config.out.mixdown == 0 &&
                audio->config.out.codec != HB_ACODEC_AC3 )
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
                    hb_log( "       + Actual mixdown: %s (%s)", hb_audio_mixdowns[j].human_readable_name, hb_audio_mixdowns[j].internal_name );
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
        audio->priv.fifo_out  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );


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
        if( audio->config.out.codec != HB_ACODEC_AC3 )
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

    /* Init read & write threads */
    job->reader = hb_reader_init( job );

    hb_log( " + output: %s", job->file );

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

    done = 0;
    w = hb_list_item( job->list_work, 0 );
    w->thread_sleep_interval = 50;
    w->init( w, job );
    while( !*job->die )
    {
        if ( !done && ( w->status = w->work( w, NULL, NULL ) ) == HB_WORK_DONE )
        {
            done = 1;
        }
        if( done && final_w->status == HB_WORK_DONE )
        {
            break;
        }
        hb_snooze( w->thread_sleep_interval );
    }
    hb_list_rem( job->list_work, w );
    w->close( w );
    free( w );
    job->done = 1;

cleanup:
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

    /* Stop read & write threads */
    if( job->reader != NULL )
        hb_thread_close( &job->reader );
    if( job->muxer != NULL )
        hb_thread_close( &job->muxer );

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
