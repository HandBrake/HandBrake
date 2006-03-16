/* $Id: work.c,v 1.43 2005/03/17 16:38:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

typedef struct
{
    hb_list_t * jobs;
    int         cpu_count;
    int       * error;
    volatile int * die;

} hb_work_t;

static void work_func();
static void do_job( hb_job_t *, int cpu_count );
static void job_loop( void * );

hb_thread_t * hb_work_init( hb_list_t * jobs, int cpu_count,
                            volatile int * die, int * error )
{
    hb_work_t * work = calloc( sizeof( hb_work_t ), 1 );

    work->jobs      = jobs;
    work->cpu_count = cpu_count;
    work->die       = die;
    work->error     = error;

    return hb_thread_init( "work", work_func, work, HB_LOW_PRIORITY );
}

static void work_func( void * _work )
{
    hb_work_t  * work = _work;
    hb_job_t   * job;

    hb_log( "%d job(s) to process", hb_list_count( work->jobs ) );

    while( !*work->die && ( job = hb_list_item( work->jobs, 0 ) ) )
    {
        hb_list_rem( work->jobs, job );
        job->die = work->die;
        do_job( job, work->cpu_count );
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

static void do_job( hb_job_t * job, int cpu_count )
{
    hb_title_t    * title;
    int             i;
    hb_thread_t   * threads[8];
    hb_work_object_t * w;
    uint64_t time_total;
    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;

    title = job->title;

    job->list_work = hb_list_init();

    hb_log( "starting job" );
    hb_log( " + device %s", title->dvd );
    hb_log( " + title %d, chapter(s) %d to %d", title->index,
            job->chapter_start, job->chapter_end );
    hb_log( " + %dx%d -> %dx%d, crop %d/%d/%d/%d",
            title->width, title->height, job->width, job->height,
            job->crop[0], job->crop[1], job->crop[2], job->crop[3] );
    hb_log( " + deinterlace %s", job->deinterlace ? "on" : "off" );
    hb_log( " + grayscale %s", job->grayscale ? "on" : "off" );
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

    job->fifo_mpeg2  = hb_fifo_init( 2048 );
    job->fifo_raw    = hb_fifo_init( 8 );
    job->fifo_sync   = hb_fifo_init( 8 );
    job->fifo_render = hb_fifo_init( 8 );
    job->fifo_mpeg4  = hb_fifo_init( 8 );

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

    subtitle = hb_list_item( title->list_subtitle, 0 );
    if( subtitle )
    {
        hb_log( " + subtitle %x, %s", subtitle->id, subtitle->lang );

        subtitle->fifo_in  = hb_fifo_init( 8 );
        subtitle->fifo_raw = hb_fifo_init( 8 );

        hb_list_add( job->list_work, ( w = getWork( WORK_DECSUB ) ) );
        w->fifo_in  = subtitle->fifo_in;
        w->fifo_out = subtitle->fifo_raw;
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

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        hb_log( "   + %x, %s", audio->id, audio->lang );

        audio->fifo_in   = hb_fifo_init( 2048 );
        audio->fifo_raw  = hb_fifo_init( 8 );
        audio->fifo_sync = hb_fifo_init( 8 );
        audio->fifo_out  = hb_fifo_init( 8 );

        switch( audio->codec )
        {
            case HB_ACODEC_AC3:
                w = getWork( WORK_DECA52 );
                break;
            case HB_ACODEC_MPGA:
                w = getWork( WORK_DECAVCODEC );
                break;
            case HB_ACODEC_LPCM:
                w = getWork( WORK_DECLPCM );
                break;
        }
        w->fifo_in  = audio->fifo_in;
        w->fifo_out = audio->fifo_raw;
        hb_list_add( job->list_work, w );

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
            w->fifo_in  = audio->fifo_sync;
            w->fifo_out = audio->fifo_out;
            w->config   = &audio->config;
            hb_list_add( job->list_work, w );
        }
    }

    /* Init read & write threads */
    job->reader = hb_reader_init( job );

    hb_log( " + output: %s", job->file );
    job->muxer = hb_muxer_init( job );

    for( i = 0; i < hb_list_count( job->list_work ); i++ )
    {
        w       = hb_list_item( job->list_work, i );
        w->lock = hb_lock_init();
        w->used = 0;
        w->time = 0;
        w->init( w, job );
    }

    job->done = 0;

    /* Launch processing threads */
    for( i = 0; i < cpu_count; i++ )
    {
        char thread_name[16];
        if( cpu_count - 1 )
        {
            snprintf( thread_name, 16, "cpu killer %d", i + 1 );
        }
        else
        {
            snprintf( thread_name, 16, "cpu killer" );
        }
        threads[i] = hb_thread_init( thread_name, job_loop, job,
                                     HB_LOW_PRIORITY );
    }

    while( !*job->die && !job->done )
    {
        hb_snooze( 500 );
    }

    for( i = 0; i < cpu_count; i++ )
    {
        hb_thread_close( &threads[i] );
    }

    /* Stop read & write threads */
    hb_thread_close( &job->reader );
    hb_thread_close( &job->muxer );

    /* Stats */
    time_total = 0;
    for( i = 0; i < hb_list_count( job->list_work ); i++ )
    {
        w = hb_list_item( job->list_work, i );
        time_total += w->time;
    }
    for( i = 0; i < hb_list_count( job->list_work ); i++ )
    {
        w = hb_list_item( job->list_work, i );
        hb_log( "%s: %.2f %%", w->name,
                100.0 * (float) w->time / (float) time_total );
    }

    /* Close work objects */
    while( ( w = hb_list_item( job->list_work, 0 ) ) )
    {
        hb_list_rem( job->list_work, w );
        hb_lock_close( &w->lock );
        w->close( w );
    }

    /* Close fifos */
    hb_fifo_close( &job->fifo_mpeg2 );
    hb_fifo_close( &job->fifo_raw );
    hb_fifo_close( &job->fifo_sync );
    hb_fifo_close( &job->fifo_render );
    hb_fifo_close( &job->fifo_mpeg4 );
    if( subtitle )
    {
        hb_fifo_close( &subtitle->fifo_in );
        hb_fifo_close( &subtitle->fifo_raw );
    }
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        hb_fifo_close( &audio->fifo_in );
        hb_fifo_close( &audio->fifo_raw );
        hb_fifo_close( &audio->fifo_sync );
        hb_fifo_close( &audio->fifo_out );
    }
}

static int lock( hb_work_object_t * w )
{
    hb_lock( w->lock );
    if( w->used )
    {
        hb_unlock( w->lock );
        return 0;
    }
    w->used = 1;
    hb_unlock( w->lock );
    return 1;
}

static void unlock( hb_work_object_t * w )
{
    hb_lock( w->lock );
    w->used = 0;
    hb_unlock( w->lock );
}

static void job_loop( void * _job )
{
    hb_job_t         * job = _job;
    hb_buffer_t      * buf_in, * buf_out;
    hb_work_object_t * w;
    int                work_count;
    int                act;
    int                i;
    uint64_t           date;
    int                done;

    work_count = hb_list_count( job->list_work );
    act        = 0;
    done       = 0;

    while( !*job->die && !job->done )
    {
        /* Handle synchronization, resampling, framerate change,
           etc */
        w = hb_list_item( job->list_work, 0 );
        if( lock( w ) )
        {
            date = hb_get_date();
            if( w->work( w, NULL, NULL ) == HB_WORK_DONE )
            {
                done = 1;
            }
            w->time += hb_get_date() - date;
            unlock( w );
        }

        for( i = 1; !*job->die && !job->done && i < work_count; i++ )
        {
            w = hb_list_item( job->list_work, i );
            if( !lock( w ) )
                continue;

            for( ;; )
            {
                hb_lock( job->pause );
                hb_unlock( job->pause );

                if( hb_fifo_is_full( w->fifo_out ) ||
                    !( buf_in = hb_fifo_get( w->fifo_in ) ) )
                {
                    break;
                }

                date = hb_get_date();
                w->work( w, &buf_in, &buf_out );
                w->time += hb_get_date() - date;
                if( buf_in )
                {
                    hb_buffer_close( &buf_in );
                }
                if( buf_out )
                {
                    act = 1;
                    hb_fifo_push( w->fifo_out, buf_out );
                }
            }

            unlock( w );
        }

        if( done &&
            !hb_fifo_size( job->fifo_sync ) &&
            !hb_fifo_size( job->fifo_render ) &&
            hb_fifo_size( job->fifo_mpeg4 ) < 2 )
        {
            job->done = 1;
            break;
        }

        /* If we did nothing, wait a bit before trying again */
        if( !act )
        {
            hb_snooze( 50 );
        }
        act = 0;
    }
}
