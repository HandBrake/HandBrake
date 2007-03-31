#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_handle_s
{
    /* The "Check for update" thread */
    int            build;
    char           version[16];
    hb_thread_t  * update_thread;

    /* This thread's only purpose is to check other threads'
       states */
    volatile int   die;
    hb_thread_t  * main_thread;
    int            pid;

    /* DVD/file scan thread */
    hb_list_t    * list_title;
    hb_thread_t  * scan_thread;

    /* The thread which processes the jobs. Others threads are launched
       from this one (see work.c) */
    hb_list_t    * jobs;
    int            job_count;
    volatile int   work_die;
    int            work_error;
    hb_thread_t  * work_thread;

    int            cpu_count;

    hb_lock_t    * state_lock;
    hb_state_t     state;

    int            paused;
    hb_lock_t    * pause_lock;
};

hb_work_object_t * hb_objects = NULL;

static void thread_func( void * );

/**
 * Registers work objects, by adding the work object to a liked list.
 * @param w Handle to hb_work_object_t to register.
 */
void hb_register( hb_work_object_t * w )
{
    w->next    = hb_objects;
    hb_objects = w;
}

/**
 * libhb initialization routine.
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @param update_check signals libhb to check for updated version from HandBrake website.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init_real( int verbose, int update_check )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );
    uint64_t      date;

    /* See hb_log() in common.c */
    if( verbose > HB_DEBUG_NONE )
    {
        putenv( "HB_DEBUG=1" );
		av_log_set_level(AV_LOG_DEBUG);
    }

    /* Check for an update on the website if asked to */
    h->build = -1;

    if( update_check )
    {
        hb_log( "hb_init: checking for updates" );
        date             = hb_get_date();
        h->update_thread = hb_update_init( &h->build, h->version );

        for( ;; )
        {
            if( hb_thread_has_exited( h->update_thread ) )
            {
                /* Immediate success or failure */
                hb_thread_close( &h->update_thread );
                break;
            }
            if( hb_get_date() > date + 1000 )
            {
                /* Still nothing after one second. Connection problem,
                   let the thread die */
                hb_log( "hb_init: connection problem, not waiting for "
                        "update_thread" );
                break;
            }
            hb_snooze( 500 );
        }
    }

    /* CPU count detection */
    hb_log( "hb_init: checking cpu count" );
    h->cpu_count = hb_get_cpu_count();

    h->list_title = hb_list_init();
    h->jobs       = hb_list_init();

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    /* libavcodec */
    avcodec_init();
    register_avcodec( &mpeg4_encoder );
    register_avcodec( &mp2_decoder );
    register_avcodec( &ac3_encoder );

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    return h;
}

/**
 * libhb initialization routine.
 * This version is to use when calling the dylib, the macro hb_init isn't available from a dylib call!
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @param update_check signals libhb to check for updated version from HandBrake website.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init_dl( int verbose, int update_check )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );
    uint64_t      date;

    /* See hb_log() in common.c */
    if( verbose > HB_DEBUG_NONE )
    {
        putenv( "HB_DEBUG=1" );
		av_log_set_level(AV_LOG_DEBUG);
    }

    /* Check for an update on the website if asked to */
    h->build = -1;

    if( update_check )
    {
        hb_log( "hb_init: checking for updates" );
        date             = hb_get_date();
        h->update_thread = hb_update_init( &h->build, h->version );

        for( ;; )
        {
            if( hb_thread_has_exited( h->update_thread ) )
            {
                /* Immediate success or failure */
                hb_thread_close( &h->update_thread );
                break;
            }
            if( hb_get_date() > date + 1000 )
            {
                /* Still nothing after one second. Connection problem,
                   let the thread die */
                hb_log( "hb_init: connection problem, not waiting for "
                        "update_thread" );
                break;
            }
            hb_snooze( 500 );
        }
    }

    /* CPU count detection */
    hb_log( "hb_init: checking cpu count" );
    h->cpu_count = hb_get_cpu_count();

    h->list_title = hb_list_init();
    h->jobs       = hb_list_init();

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    /* libavcodec */
    avcodec_init();
    register_avcodec( &mpeg4_encoder );
    register_avcodec( &mp2_decoder );
    register_avcodec( &ac3_encoder );

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    hb_register( &hb_sync ); 
	hb_register( &hb_decmpeg2 ); 
	hb_register( &hb_decsub ); 
	hb_register( &hb_render ); 
	hb_register( &hb_encavcodec ); 
	hb_register( &hb_encxvid ); 
	hb_register( &hb_encx264 ); 
	hb_register( &hb_deca52 ); 
	hb_register( &hb_decavcodec ); 
	hb_register( &hb_declpcm ); 
	hb_register( &hb_encfaac ); 
	hb_register( &hb_enclame ); 
	hb_register( &hb_encvorbis ); 
	
	return h;
}


/**
 * Returns current version of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of version number.
 */
char * hb_get_version( hb_handle_t * h )
{
    return HB_VERSION;
}

/**
 * Returns current build of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of build number.
 */
int hb_get_build( hb_handle_t * h )
{
    return HB_BUILD;
}

/**
 * Checks for needed update.
 * @param h Handle to hb_handle_t.
 * @param version Pointer to handle where version will be copied.
 * @return update indicator.
 */
int hb_check_update( hb_handle_t * h, char ** version )
{
    *version = ( h->build < 0 ) ? NULL : h->version;
    return h->build;
}

/**
 * Sets the cpu count to the desired value.
 * @param h Handle to hb_handle_t
 * @param cpu_count Number of CPUs to use.
 */
void hb_set_cpu_count( hb_handle_t * h, int cpu_count )
{
    cpu_count    = MAX( 1, cpu_count );
    cpu_count    = MIN( cpu_count, 8 );
    h->cpu_count = cpu_count;
}

/**
 * Initializes a scan of the by calling hb_scan_init
 * @param h Handle to hb_handle_t
 * @param path location of VIDEO_TS folder.
 * @param title_index Desired title to scan.  0 for all titles.
 */
void hb_scan( hb_handle_t * h, const char * path, int title_index )
{
    hb_title_t * title;

    /* Clean up from previous scan */
    while( ( title = hb_list_item( h->list_title, 0 ) ) )
    {
        hb_list_rem( h->list_title, title );
        hb_title_close( &title );
    }
    
    hb_log( "hb_scan: path=%s, title_index=%d", path, title_index );
    h->scan_thread = hb_scan_init( h, path, title_index, h->list_title );
}

/**
 * Returns the list of titles found.
 * @param h Handle to hb_handle_t
 * @return Handle to hb_list_t of the title list.
 */
hb_list_t * hb_get_titles( hb_handle_t * h )
{
    return h->list_title;
}

/**
 * Create preview image of desired title a index of picture.
 * @param h Handle to hb_handle_t.
 * @param title Handle to hb_title_t of desired title.
 * @param picture Index in title.
 * @param buffer Handle to buufer were inage will be drawn.
 */
void hb_get_preview( hb_handle_t * h, hb_title_t * title, int picture,
                     uint8_t * buffer )
{
    hb_job_t           * job = title->job;
    char                 filename[1024];
    FILE               * file;
    uint8_t            * buf1, * buf2, * buf3, * buf4, * pen;
    uint32_t           * p32;
    AVPicture            pic1, pic2, pic3, pic4;
    ImgReSampleContext * context;
    int                  i;

    buf1 = malloc( title->width * title->height * 3 / 2 );
    buf2 = malloc( title->width * title->height * 3 / 2 );
    buf3 = malloc( title->width * title->height * 3 / 2 );
    buf4 = malloc( title->width * title->height * 4 );
    avpicture_fill( &pic1, buf1, PIX_FMT_YUV420P,
                    title->width, title->height );
    avpicture_fill( &pic2, buf2, PIX_FMT_YUV420P,
                    title->width, title->height );
    avpicture_fill( &pic3, buf3, PIX_FMT_YUV420P,
                    job->width, job->height );
    avpicture_fill( &pic4, buf4, PIX_FMT_RGBA32,
                    job->width, job->height );
    
    memset( filename, 0, 1024 );

    hb_get_tempory_filename( h, filename, "%x%d",
                             (int) title, picture );

    file = fopen( filename, "r" );
    if( !file )
    {
        hb_log( "hb_get_preview: fopen failed" );
        return;
    }

    fread( buf1, title->width * title->height * 3 / 2, 1, file );
    fclose( file );

    context = img_resample_full_init(
            job->width, job->height, title->width, title->height,
            job->crop[0], job->crop[1], job->crop[2], job->crop[3],
            0, 0, 0, 0 );

    if( job->deinterlace )
    {
        avpicture_deinterlace( &pic2, &pic1, PIX_FMT_YUV420P,
                               title->width, title->height );
        img_resample( context, &pic3, &pic2 );
    }
    else
    {
        img_resample( context, &pic3, &pic1 );
    }
    img_convert( &pic4, PIX_FMT_RGBA32, &pic3, PIX_FMT_YUV420P,
                 job->width, job->height );

    /* Gray background */
    p32 = (uint32_t *) buffer;
    for( i = 0; i < ( title->width + 2 ) * ( title->height + 2 ); i++ )
    {
        p32[i] = 0xFF808080;
    }

    /* Draw the picture, centered, and draw the cropping zone */
    pen = buffer + ( title->height - job->height ) *
        ( title->width + 2 ) * 2 + ( title->width - job->width ) * 2;
    memset( pen, 0xFF, 4 * ( job->width + 2 ) );
    pen += 4 * ( title->width + 2 );
    for( i = 0; i < job->height; i++ )
    {
        uint8_t * nextLine;
        nextLine = pen + 4 * ( title->width + 2 );
        memset( pen, 0xFF, 4 );
        pen += 4;
        memcpy( pen, buf4 + 4 * job->width * i, 4 * job->width );
        pen += 4 * job->width;
        memset( pen, 0xFF, 4 );
        pen = nextLine;
    }
    memset( pen, 0xFF, 4 * ( job->width + 2 ) );

    free( buf1 );
    free( buf2 );
    free( buf3 );
    free( buf4 );
}

/**
 * Calculates job width, height, and cropping parameters.
 * @param job Handle to hb_job_t.
 * @param aspect Desired aspect ratio. Value of -1 uses title aspect.
 * @param pixels Maximum desired pixel count.
 */
void hb_set_size( hb_job_t * job, int aspect, int pixels )
{
    hb_title_t * title = job->title;

    int croppedWidth  = title->width - title->crop[2] - title->crop[3];
    int croppedHeight = title->height - title->crop[0] - title->crop[1];
    int croppedAspect = title->aspect * title->height * croppedWidth /
                            croppedHeight / title->width;
    int addCrop;
    int i, w, h;

    if( aspect <= 0 )
    {
        /* Keep the best possible aspect ratio */
        aspect = croppedAspect;
    }

    /* Crop if necessary to obtain the desired ratio */
    memcpy( job->crop, title->crop, 4 * sizeof( int ) );
    if( aspect < croppedAspect )
    {
        /* Need to crop on the left and right */
        addCrop = croppedWidth - aspect * croppedHeight * title->width /
                    title->aspect / title->height;
        if( addCrop & 3 )
        {
            addCrop = ( addCrop + 1 ) / 2;
            job->crop[2] += addCrop;
            job->crop[3] += addCrop;
        }
        else if( addCrop & 2 )
        {
            addCrop /= 2;
            job->crop[2] += addCrop - 1;
            job->crop[3] += addCrop + 1;
        }
        else
        {
            addCrop /= 2;
            job->crop[2] += addCrop;
            job->crop[3] += addCrop;
        }
    }
    else if( aspect > croppedAspect )
    {
        /* Need to crop on the top and bottom */
        addCrop = croppedHeight - croppedWidth * title->aspect *
            title->height / aspect / title->width;
        if( addCrop & 3 )
        {
            addCrop = ( addCrop + 1 ) / 2;
            job->crop[0] += addCrop;
            job->crop[1] += addCrop;
        }
        else if( addCrop & 2 )
        {
            addCrop /= 2;
            job->crop[0] += addCrop - 1;
            job->crop[1] += addCrop + 1;
        }
        else
        {
            addCrop /= 2;
            job->crop[0] += addCrop;
            job->crop[1] += addCrop;
        }
    }

    /* Compute a resolution from the number of pixels and aspect */
    for( i = 0;; i++ )
    {
        w = 16 * i;
        h = MULTIPLE_16( w * HB_ASPECT_BASE / aspect );
        if( w * h > pixels )
        {
            break;
        }
    }
    i--;
    job->width  = 16 * i;
    job->height = MULTIPLE_16( 16 * i * HB_ASPECT_BASE / aspect );
}

/**
 * Returns the number of jobs in the queue.
 * @param h Handle to hb_handle_t.
 * @return Number of jobs.
 */
int hb_count( hb_handle_t * h )
{
    return hb_list_count( h->jobs );
}

/**
 * Returns handle to job at index i within the job list.
 * @param h Handle to hb_handle_t.
 * @param i Index of job.
 * @returns Handle to hb_job_t of desired job.
 */
hb_job_t * hb_job( hb_handle_t * h, int i )
{
    return hb_list_item( h->jobs, i );
}

/**
 * Adds a job to the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to hb_job_t.
 */
void hb_add( hb_handle_t * h, hb_job_t * job )
{
    hb_job_t      * job_copy;
    hb_title_t    * title,    * title_copy;
    hb_chapter_t  * chapter,  * chapter_copy;
    hb_audio_t    * audio,    * audio_copy;
    hb_subtitle_t * subtitle, * subtitle_copy;
    int             i;

    /* Copy the title */
    title      = job->title;
    title_copy = malloc( sizeof( hb_title_t ) );
    memcpy( title_copy, title, sizeof( hb_title_t ) );

    title_copy->list_chapter = hb_list_init();
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter      = hb_list_item( title->list_chapter, i );
        chapter_copy = malloc( sizeof( hb_chapter_t ) );
        memcpy( chapter_copy, chapter, sizeof( hb_chapter_t ) );
        hb_list_add( title_copy->list_chapter, chapter_copy );
    }

    /* Copy the audio track(s) we want */
    title_copy->list_audio = hb_list_init();

    /* Do nothing about audio during first pass */
    if( job->pass != 1 )
    {
        for( i = 0; i < 8; i++ )
        {
            if( job->audios[i] < 0 )
            {
                break;
            }
            if( ( audio = hb_list_item( title->list_audio, job->audios[i] ) ) )
            {
                audio_copy = malloc( sizeof( hb_audio_t ) );
                memcpy( audio_copy, audio, sizeof( hb_audio_t ) );
                hb_list_add( title_copy->list_audio, audio_copy );
            }
        }
    }

    /* Copy the subtitle we want (or not) */
    title_copy->list_subtitle = hb_list_init();
    if( ( subtitle = hb_list_item( title->list_subtitle, job->subtitle ) ) )
    {
        subtitle_copy = malloc( sizeof( hb_subtitle_t ) );
        memcpy( subtitle_copy, subtitle, sizeof( hb_subtitle_t ) );
        hb_list_add( title_copy->list_subtitle, subtitle_copy );
    }

    /* Copy the job */
    job_copy        = calloc( sizeof( hb_job_t ), 1 );
    memcpy( job_copy, job, sizeof( hb_job_t ) );
    title_copy->job = job_copy;
    job_copy->title = title_copy;
    job_copy->file  = strdup( job->file );
    job_copy->h     = h;
    job_copy->pause = h->pause_lock;

    /* Add the job to the list */
    hb_list_add( h->jobs, job_copy );
}

/**
 * Removes a job from the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to hb_job_t.
 */
void hb_rem( hb_handle_t * h, hb_job_t * job )
{
    hb_list_rem( h->jobs, job );

    /* XXX free everything XXX */
}

/**
 * Starts the conversion process.
 * Sets state to HB_STATE_WORKING.
 * calls hb_work_init, to launch work thread. Stores handle to work thread.
 * @param h Handle to hb_handle_t.
 */
void hb_start( hb_handle_t * h )
{
    /* XXX Hack */
    h->job_count = hb_list_count( h->jobs );

    hb_lock( h->state_lock );
    h->state.state = HB_STATE_WORKING;
#define p h->state.param.working
    p.progress  = 0.0;
    p.job_cur   = 1;
    p.job_count = h->job_count;
    p.rate_cur  = 0.0;
    p.rate_avg  = 0.0;
    p.hours     = -1;
    p.minutes   = -1;
    p.seconds   = -1;
#undef p
    hb_unlock( h->state_lock );

    h->paused = 0;

    h->work_die    = 0;
    h->work_thread = hb_work_init( h->jobs, h->cpu_count,
                                   &h->work_die, &h->work_error );
}

/**
 * Pauses the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_pause( hb_handle_t * h )
{
    if( !h->paused )
    {
        hb_lock( h->pause_lock );
        h->paused = 1;

        hb_lock( h->state_lock );
        h->state.state = HB_STATE_PAUSED;
        hb_unlock( h->state_lock );
    }
}

/**
 * Resumes the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_resume( hb_handle_t * h )
{
    if( h->paused )
    {
        hb_unlock( h->pause_lock );
        h->paused = 0;
    }
}

/**
 * Stops the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_stop( hb_handle_t * h )
{
    h->work_die = 1;

    hb_resume( h );
}

/**
 * Returns the state of the conversion process.
 * @param h Handle to hb_handle_t.
 * @param s Handle to hb_state_t which to copy the state data.
 */
void hb_get_state( hb_handle_t * h, hb_state_t * s )
{
    hb_lock( h->state_lock );

    memcpy( s, &h->state, sizeof( hb_state_t ) );
    h->state.state = HB_STATE_IDLE;

    hb_unlock( h->state_lock );
}

/**
 * Closes access to libhb by freeing the hb_handle_t handle ontained in hb_init_real.
 * @param _h Pointer to handle to hb_handle_t.
 */
void hb_close( hb_handle_t ** _h )
{
    hb_handle_t * h = *_h;
    hb_title_t * title;

    h->die = 1;
    hb_thread_close( &h->main_thread );

    while( ( title = hb_list_item( h->list_title, 0 ) ) )
    {
        hb_list_rem( h->list_title, title );
        free( title->job );
        hb_title_close( &title );
    }
    hb_list_close( &h->list_title );

    hb_list_close( &h->jobs );
    hb_lock_close( &h->state_lock );
    hb_lock_close( &h->pause_lock );
    free( h );
    *_h = NULL;
}

/**
 * Monitors the state of the update, scan, and work threads.
 * Sets scan done state when scan thread exits.
 * Sets work done state when work thread exits.
 * @param _h Handle to hb_handle_t
 */
static void thread_func( void * _h )
{
    hb_handle_t * h = (hb_handle_t *) _h;
    char dirname[1024];
    DIR * dir;
    struct dirent * entry;

    h->pid = getpid();

    /* Create folder for temporary files */
    memset( dirname, 0, 1024 );
    hb_get_tempory_directory( h, dirname );

    hb_mkdir( dirname );

    while( !h->die )
    {
        /* In case the check_update thread hangs, it'll die sooner or
           later. Then, we join it here */
        if( h->update_thread &&
            hb_thread_has_exited( h->update_thread ) )
        {
            hb_thread_close( &h->update_thread );
        }

        /* Check if the scan thread is done */
        if( h->scan_thread &&
            hb_thread_has_exited( h->scan_thread ) )
        {
            hb_thread_close( &h->scan_thread );

            hb_log( "libhb: scan thread found %d valid title(s)",
                    hb_list_count( h->list_title ) );
            hb_lock( h->state_lock );
            h->state.state = HB_STATE_SCANDONE;
            hb_unlock( h->state_lock );
        }

        /* Check if the work thread is done */
        if( h->work_thread &&
            hb_thread_has_exited( h->work_thread ) )
        {
            hb_thread_close( &h->work_thread );

            hb_log( "libhb: work result = %d",
                    h->work_error );
            hb_lock( h->state_lock );
            h->state.state                = HB_STATE_WORKDONE;
            h->state.param.workdone.error = h->work_error;
            hb_unlock( h->state_lock );
        }

        hb_snooze( 50 );
    }

    if( h->work_thread )
    {
        hb_stop( h );
        hb_thread_close( &h->work_thread );
    }

    /* Remove temp folder */
    dir = opendir( dirname );
    while( ( entry = readdir( dir ) ) )
    {
        char filename[1024];
        if( entry->d_name[0] == '.' )
        {
            continue;
        }
        memset( filename, 0, 1024 );
        snprintf( filename, 1023, "%s/%s", dirname, entry->d_name );
        unlink( filename );
    }
    closedir( dir );
    rmdir( dirname );
}

/**
 * Returns the PID.
 * @param h Handle to hb_handle_t
 */
int hb_get_pid( hb_handle_t * h )
{
    return h->pid;
}

/**
 * Sets the current state.
 * @param h Handle to hb_handle_t
 * @param s Handle to new hb_state_t
 */
void hb_set_state( hb_handle_t * h, hb_state_t * s )
{
    hb_lock( h->pause_lock );
    hb_lock( h->state_lock );
    memcpy( &h->state, s, sizeof( hb_state_t ) );
    if( h->state.state == HB_STATE_WORKING )
    {
        /* XXX Hack */
        h->state.param.working.job_cur =
            h->job_count - hb_list_count( h->jobs );
        h->state.param.working.job_count = h->job_count;
    }
    hb_unlock( h->state_lock );
    hb_unlock( h->pause_lock );
}
