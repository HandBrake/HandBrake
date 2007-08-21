/* $Id: reader.c,v 1.21 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

typedef struct
{
    hb_job_t     * job;
    hb_title_t   * title;
    volatile int * die;

    hb_dvd_t     * dvd;
    hb_buffer_t  * ps;
    hb_stream_t  * stream;

} hb_reader_t;

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void        ReaderFunc( void * );
static hb_fifo_t * GetFifoForId( hb_job_t * job, int id );

/***********************************************************************
 * hb_reader_init
 ***********************************************************************
 *
 **********************************************************************/
hb_thread_t * hb_reader_init( hb_job_t * job )
{
    hb_reader_t * r;

    r = calloc( sizeof( hb_reader_t ), 1 );

    r->job   = job;
    r->title = job->title;
    r->die   = job->die;
    
    return hb_thread_init( "reader", ReaderFunc, r,
                           HB_NORMAL_PRIORITY );
}

/***********************************************************************
 * ReaderFunc
 ***********************************************************************
 *
 **********************************************************************/
static void ReaderFunc( void * _r )
{
    hb_reader_t  * r = _r;
    hb_fifo_t    * fifo;
    hb_buffer_t  * buf;
    hb_list_t    * list;
    int            chapter = -1;

    if( !( r->dvd = hb_dvd_init( r->title->dvd ) ) )
    {
        if ( !(r->stream = hb_stream_open(r->title->dvd) ) )
        {
          return;
        }
    }

    if (r->dvd)
    {
      if( !hb_dvd_start( r->dvd, r->title->index, r->job->chapter_start ) )
      {
          hb_dvd_close( &r->dvd );
          return;
      }
    }
    
	if (r->stream)
	{
		// At this point r->audios[0] gives us the index of the selected audio track for output track 0
		// we cannot effectively demux multiple PID's into the seperate output tracks unfortunately
		// so we'll just specifiy things here for a single track.
		hb_stream_set_selected_audio_pid_index(r->stream, r->job->audios[0]);
	}
	
    list  = hb_list_init();
    r->ps = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );

    while( !*r->die && !r->job->done )
    {
        if (r->dvd)
          chapter = hb_dvd_chapter( r->dvd );
        else if (r->stream)
          chapter = 1;
          
        if( chapter < 0 )
        {
            hb_log( "reader: end of the title reached" );
            break;
        }
        if( chapter > r->job->chapter_end )
        {
            hb_log( "reader: end of chapter %d reached (%d)",
                    r->job->chapter_end, chapter );
            break;
        }

        if (r->dvd)
        {
          if( !hb_dvd_read( r->dvd, r->ps ) )
          {
              break;
          }
        }
        else if (r->stream)
        {
          if ( !hb_stream_read( r->stream, r->ps ) )
          {
            break;
          }
        }
        
        hb_demux_ps( r->ps, list );

        while( ( buf = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, buf );
            fifo = GetFifoForId( r->job, buf->id );
            if( fifo )
            {
                while( !*r->die && !r->job->done &&
                       hb_fifo_is_full( fifo ) )
                {
                    hb_snooze( 50 );
                }
                hb_fifo_push( fifo, buf );
            }
            else
            {
                hb_buffer_close( &buf );
            }
        }
    }

    hb_list_empty( &list );
    hb_buffer_close( &r->ps );
    if (r->dvd)
    {
      hb_dvd_stop( r->dvd );
      hb_dvd_close( &r->dvd );
    }
    else if (r->stream)
    {
      hb_stream_close(&r->stream);
    }
    
    free( r );
    _r = NULL;

    hb_log( "reader: done" );
}

/***********************************************************************
 * GetFifoForId
 ***********************************************************************
 *
 **********************************************************************/
static hb_fifo_t * GetFifoForId( hb_job_t * job, int id )
{
    hb_title_t    * title = job->title;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i;

    if( id == 0xE0 )
    {
        if( !job->subtitle_scan )
        {
            return job->fifo_mpeg2;
        } else {
            /*
             * Ditch the mpeg2 video when doing a subtitle scan.
             */
            return NULL;
        }
    }

    if (job->subtitle_scan) {
        /*
         * Count the occurances of the subtitles, don't actually
         * return any to encode unless we are looking fro forced
         * subtitles in which case we need to look in the sub picture
         * to see if it has the forced flag enabled.
         */
        for (i=0; i < hb_list_count(title->list_subtitle); i++) {
            subtitle =  hb_list_item( title->list_subtitle, i);
            if (id == subtitle->id) {
                /*
                 * A hit, count it.
                 */
                subtitle->hits++;
                if( job->subtitle_force )
                {
                    return subtitle->fifo_in;
                }
                break;
            }
        }
    } else {
        if( ( subtitle = hb_list_item( title->list_subtitle, 0 ) ) &&
            id == subtitle->id )
        {
            return subtitle->fifo_in;
        }
    }
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( id == audio->id )
        {
            return audio->fifo_in;
        }
    }

    return NULL;
}

