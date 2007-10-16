/* $Id: work.c,v 1.43 2005/03/17 16:38:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
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

static hb_work_object_t * getWork( int id )
{
    hb_work_object_t * w;
    for( w = hb_objects; w; w = w->next )
    {
        if( w->id == id )
        {
            return w;
        }
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
    
    /* FIXME: This feels really hackish, anything better? */
    hb_work_object_t * audio_w = NULL;
    hb_work_object_t * sub_w = NULL;

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
        hb_set_anamorphic_size(job);
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
    hb_log( " + grayscale %s", job->grayscale ? "on" : "off" );
    
    if( job->filters )
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
    
    if( job->vquality >= 0.0 && job->vquality <= 1.0 )
    {
        hb_log( " + %.3f fps, video quality %.2f", (float) job->vrate /
                (float) job->vrate_base, job->vquality );
    }
    else
    {
        hb_log( " + %.3f fps, video bitrate %d kbps, pass %d",
                (float) job->vrate / (float) job->vrate_base,
                job->vbitrate, job->pass );
    }
	hb_log (" + PixelRatio: %d, width:%d, height: %d",job->pixel_ratio,job->width, job->height);
    job->fifo_mpeg2  = hb_fifo_init( 2048 );
    job->fifo_raw    = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_sync   = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_render = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
    job->fifo_mpeg4  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );

    /* Synchronization */
    hb_list_add( job->list_work, ( w = getWork( WORK_SYNC ) ) );
    w->fifo_in  = NULL;
    w->fifo_out = NULL;

    /* Video decoder */
    hb_list_add( job->list_work, ( w = getWork( WORK_DECMPEG2 ) ) );
    w->fifo_in  = job->fifo_mpeg2;
    w->fifo_out = job->fifo_raw;

    /* Video renderer */
    hb_list_add( job->list_work, ( w = getWork( WORK_RENDER ) ) );
    w->fifo_in  = job->fifo_sync;
    w->fifo_out = job->fifo_render;

    /* Video encoder */
    switch( job->vcodec )
    {
        case HB_VCODEC_FFMPEG:
            hb_log( " + encoder FFmpeg" );
            w = getWork( WORK_ENCAVCODEC );
            break;
        case HB_VCODEC_XVID:
            hb_log( " + encoder XviD" );
            w = getWork( WORK_ENCXVID );
            break;
        case HB_VCODEC_X264:
            hb_log( " + encoder x264" );
            w = getWork( WORK_ENCX264 );
            break;
    }
    w->fifo_in  = job->fifo_render;
    w->fifo_out = job->fifo_mpeg4;
    w->config   = &job->config;
    
    hb_list_add( job->list_work, w );

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
                if( sub_w != NULL )
                { 
                    /*
                     * Need to copy the prior subtitle structure so that we
                     * don't overwrite the fifos.
                     */
                    sub_w = calloc( sizeof( hb_work_object_t ), 1 );
                    sub_w = memcpy( sub_w, w, sizeof( hb_work_object_t ));
                } else {
                    w = sub_w = getWork( WORK_DECSUB );
                }
                hb_list_add( job->list_work, sub_w );
                sub_w->fifo_in  = subtitle->fifo_in;
                sub_w->fifo_out = subtitle->fifo_raw;
            }
        }
    }

    if( job->acodec & HB_ACODEC_AC3 )
    {
        hb_log( " + audio AC3 passthrough" );
    }
    else
    {
        hb_log( " + audio %d kbps, %d Hz", job->abitrate, job->arate );
        hb_log( " + encoder %s", ( job->acodec & HB_ACODEC_FAAC ) ?
                "faac" : ( ( job->acodec & HB_ACODEC_LAME ) ? "lame" :
                "vorbis" ) );
    }

    /* if we are doing AC3 passthru, then remove any non-AC3 audios from the job */
    /* otherwise, Bad Things will happen */
    for( i = 0; i < hb_list_count( title->list_audio ); )
    {
        audio = hb_list_item( title->list_audio, i );
        if( ( job->acodec & HB_ACODEC_AC3 ) && ( audio->codec != HB_ACODEC_AC3 ) )
        {
            hb_list_rem( title->list_audio, audio );
            free( audio );
            continue;
        }
        i++;
    }

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        hb_log( "   + %x, %s", audio->id, audio->lang );
		        
		/* sense-check the current mixdown options */

		/* log the requested mixdown */
		for (j = 0; j < hb_audio_mixdowns_count; j++) {
			if (hb_audio_mixdowns[j].amixdown == job->audio_mixdowns[i]) {
				hb_log( "     + Requested mixdown: %s (%s)", hb_audio_mixdowns[j].human_readable_name, hb_audio_mixdowns[j].internal_name );
				break;
			}
		}

        /* sense-check the requested mixdown */

        /* audioCodecsSupportMono and audioCodecsSupport6Ch are the same for now,
           but this may change in the future, so they are separated for flexibility */
        int audioCodecsSupportMono = ((audio->codec == HB_ACODEC_AC3 ||
            audio->codec == HB_ACODEC_DCA) && (job->acodec == HB_ACODEC_FAAC || job->acodec == HB_ACODEC_VORBIS));
        int audioCodecsSupport6Ch =  ((audio->codec == HB_ACODEC_AC3 ||
            audio->codec == HB_ACODEC_DCA) && (job->acodec == HB_ACODEC_FAAC || job->acodec == HB_ACODEC_VORBIS));

        /* find out what the format of our source audio is */
        switch (audio->input_channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK) {
        
            /* mono sources */
            case HB_INPUT_CH_LAYOUT_MONO:
                /* regardless of what stereo mixdown we've requested, a mono source always get mixed down
                to mono if we can, and mixed up to stereo if we can't */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 1) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_MONO;
                } else {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_STEREO;
                }
                break;

            /* stereo input */
            case HB_INPUT_CH_LAYOUT_STEREO:
                /* if we've requested a mono mixdown, and it is supported, then do the mix */
                /* use stereo if not supported */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 0) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_STEREO;
                /* otherwise, preserve stereo regardless of if we requested something higher */
                } else if (job->audio_mixdowns[i] > HB_AMIXDOWN_STEREO) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_STEREO;
                }
                break;

            /* dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input */
            /* the A52 flags don't allow for a way to distinguish between DPL1 and DPL2 on a DVD,
               so we always assume a DPL1 source for A52_DOLBY */
            case HB_INPUT_CH_LAYOUT_DOLBY:
                /* if we've requested a mono mixdown, and it is supported, then do the mix */
                /* preserve dolby if not supported */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 0) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBY;
                /* otherwise, preserve dolby even if we requested something higher */
                /* a stereo mixdown will still be honoured here */
                } else if (job->audio_mixdowns[i] > HB_AMIXDOWN_DOLBY) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBY;
                }
                break;

            /* 3F/2R input */
            case HB_INPUT_CH_LAYOUT_3F2R:
                /* if we've requested a mono mixdown, and it is supported, then do the mix */
                /* use dpl2 if not supported */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 0) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBYPLII;
                } else {
                    /* check if we have 3F2R input and also have an LFE - i.e. we have a 5.1 source) */
                    if (audio->input_channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE) {
                        /* we have a 5.1 source */
                        /* if we requested 6ch, but our audio format doesn't support it, then mix to DPLII instead */
                        if (job->audio_mixdowns[i] == HB_AMIXDOWN_6CH && audioCodecsSupport6Ch == 0) {
                            job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBYPLII;
                        }
                    } else {
                        /* we have a 5.0 source, so we can't do 6ch conversion
                        default to DPL II instead */
                        if (job->audio_mixdowns[i] > HB_AMIXDOWN_DOLBYPLII) {
                            job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBYPLII;
                        }
                    }
                }
                /* all other mixdowns will have been preserved here */
                break;

            /* 3F/1R input */
            case HB_INPUT_CH_LAYOUT_3F1R:
                /* if we've requested a mono mixdown, and it is supported, then do the mix */
                /* use dpl1 if not supported */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 0) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBY;
                } else {
                    /* we have a 4.0 or 4.1 source, so we can't do DPLII or 6ch conversion
                    default to DPL I instead */
                    if (job->audio_mixdowns[i] > HB_AMIXDOWN_DOLBY) {
                        job->audio_mixdowns[i] = HB_AMIXDOWN_DOLBY;
                    }
                }
                /* all other mixdowns will have been preserved here */
                break;

            default:
                /* if we've requested a mono mixdown, and it is supported, then do the mix */
                if (job->audio_mixdowns[i] == HB_AMIXDOWN_MONO && audioCodecsSupportMono == 1) {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_MONO;
                /* mix everything else down to stereo */
                } else {
                    job->audio_mixdowns[i] = HB_AMIXDOWN_STEREO;
                }

        }

		/* log the output mixdown */
		for (j = 0; j < hb_audio_mixdowns_count; j++) {
			if (hb_audio_mixdowns[j].amixdown == job->audio_mixdowns[i]) {
				hb_log( "     + Actual mixdown: %s (%s)", hb_audio_mixdowns[j].human_readable_name, hb_audio_mixdowns[j].internal_name );
				break;
			}
		}

		/* we now know we have a valid mixdown for the input source and the audio output format */
		/* remember the mixdown for this track */
		audio->amixdown = job->audio_mixdowns[i];

        audio->config.vorbis.language = audio->lang_simple;

		/* set up the audio work structures */
        audio->fifo_in   = hb_fifo_init( 2048 );
        audio->fifo_raw  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
        audio->fifo_sync = hb_fifo_init( FIFO_CPU_MULT * cpu_count );
        audio->fifo_out  = hb_fifo_init( FIFO_CPU_MULT * cpu_count );

        switch( audio->codec )
        {
            case HB_ACODEC_AC3:
                w = getWork( WORK_DECA52 );
                break;
            case HB_ACODEC_DCA:
                w = getWork( WORK_DECDCA );
                break;
            case HB_ACODEC_MPGA:
                w = getWork( WORK_DECAVCODEC );
                break;
            case HB_ACODEC_LPCM:
                w = getWork( WORK_DECLPCM );
                break;
        }
        w->fifo_in       = audio->fifo_in;
        w->fifo_out      = audio->fifo_raw;
        w->config        = &audio->config;
        w->amixdown      = audio->amixdown;
        w->source_acodec = audio->codec;
        
        /* FIXME: This feels really hackish, anything better? */
        audio_w = calloc( sizeof( hb_work_object_t ), 1 );
        audio_w = memcpy( audio_w, w, sizeof( hb_work_object_t ));
        
        hb_list_add( job->list_work, audio_w );

        switch( job->acodec )
        {
            case HB_ACODEC_FAAC:
                w = getWork( WORK_ENCFAAC );
                break;
            case HB_ACODEC_LAME:
                w = getWork( WORK_ENCLAME );
                break;
            case HB_ACODEC_VORBIS:
                w = getWork( WORK_ENCVORBIS );
                break;
        }

        if( job->acodec != HB_ACODEC_AC3 )
        {
            w->fifo_in       = audio->fifo_sync;
            w->fifo_out      = audio->fifo_out;
            w->config        = &audio->config;
            w->amixdown      = audio->amixdown;
            w->source_acodec = audio->codec;
            
            /* FIXME: This feels really hackish, anything better? */
            audio_w = calloc( sizeof( hb_work_object_t ), 1 );
            audio_w = memcpy( audio_w, w, sizeof( hb_work_object_t ));
        
            hb_list_add( job->list_work, audio_w );
        }


    }

    /* Init read & write threads */
    job->reader = hb_reader_init( job );

    hb_log( " + output: %s", job->file );
    job->muxer = hb_muxer_init( job );

    job->done = 0;

    /* Launch processing threads */
    for( i = 1; i < hb_list_count( job->list_work ); i++ )
    {
        w = hb_list_item( job->list_work, i );
        w->done = &job->done;
        w->thread_sleep_interval = 10;
        w->init( w, job );
        w->thread = hb_thread_init( w->name, work_loop, w,
                                    HB_LOW_PRIORITY );
    }

    done = 0;
    w = hb_list_item( job->list_work, 0 );
    w->thread_sleep_interval = 50;
    w->init( w, job );
    while( !*job->die )
    {
        if( w->work( w, NULL, NULL ) == HB_WORK_DONE )
        {
            done = 1;
        }
        if( done &&
            !hb_fifo_size( job->fifo_sync ) &&
            !hb_fifo_size( job->fifo_render ) &&
            !hb_fifo_size( job->fifo_mpeg4 ) )
        {
            break;
        }
        hb_snooze( w->thread_sleep_interval );
    }
    hb_list_rem( job->list_work, w );
    w->close( w );
    job->done = 1;

    /* Close work objects */
    while( ( w = hb_list_item( job->list_work, 0 ) ) )
    {
        hb_list_rem( job->list_work, w );
        hb_thread_close( &w->thread );
        w->close( w );
        
        /* FIXME: This feels really hackish, anything better? */
        if ( w->id == WORK_DECA52 ||
             w->id == WORK_DECDCA ||
             w->id == WORK_DECLPCM ||
             w->id == WORK_ENCFAAC ||
             w->id == WORK_ENCLAME ||
             w->id == WORK_ENCVORBIS )
        {
            free( w );
            w = NULL;
        }
    }
    
    hb_list_close( &job->list_work );

    /* Stop read & write threads */
    hb_thread_close( &job->reader );
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
        hb_fifo_close( &audio->fifo_in );
        hb_fifo_close( &audio->fifo_raw );
        hb_fifo_close( &audio->fifo_sync );
        hb_fifo_close( &audio->fifo_out );
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

    hb_buffer_pool_free();

    hb_title_close( &job->title );
    free( job );
}

/**
 * Performs the work objects specific work function.
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

        w->work( w, &buf_in, &buf_out );

        // Propogate any chapter breaks for the worker
        if( buf_in && buf_out && buf_in->new_chap )
        {
            hb_log("WORK: Copying Chapter Break");
            buf_out->new_chap = 1;
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
