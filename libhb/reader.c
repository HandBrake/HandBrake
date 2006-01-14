/* $Id: reader.c,v 1.20 2005/04/29 19:55:54 titer Exp $

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
    int            chapter;

    if( !( r->dvd = hb_dvd_init( r->title->dvd ) ) )
    {
        return;
    }

    if( !hb_dvd_start( r->dvd, r->title->index, r->job->chapter_start ) )
    {
        hb_dvd_close( &r->dvd );
        return;
    }

    list  = hb_list_init();
    r->ps = hb_buffer_init( 2048 );

    while( !*r->die && !r->job->done )
    {
        chapter = hb_dvd_chapter( r->dvd );
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

        if( !hb_dvd_read( r->dvd, r->ps ) )
        {
            break;
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
    hb_dvd_close( &r->dvd );

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
        return job->fifo_mpeg2;
    }

    if( ( subtitle = hb_list_item( title->list_subtitle, 0 ) ) &&
        id == subtitle->id )
    {
        return subtitle->fifo_in;
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

