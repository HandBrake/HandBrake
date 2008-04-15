/* $Id: reader.c,v 1.21 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
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

    hb_psdemux_t   demux;
    uint           sequence;
    int            saw_video;
} hb_reader_t;

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void        ReaderFunc( void * );
static hb_fifo_t ** GetFifoForId( hb_job_t * job, int id );

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
    r->sequence = 0;

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
    hb_fifo_t   ** fifos;
    hb_buffer_t  * buf;
    hb_buffer_t  * buf_old;
    hb_list_t    * list;
    int            n;
    int            chapter = -1;
    int            chapter_end = r->job->chapter_end;

    if( !( r->dvd = hb_dvd_init( r->title->dvd ) ) )
    {
        if ( !( r->stream = hb_stream_open( r->title->dvd, 1 ) ) )
        {
          return;
        }
    }

    if (r->dvd)
    {
      /*
       * XXX this code is a temporary hack that should go away if/when
       *     chapter merging goes away in libhb/dvd.c
       * map the start and end chapter numbers to on-media chapter
       * numbers since chapter merging could cause the handbrake numbers
       * to diverge from the media numbers and, if our chapter_end is after
       * a media chapter that got merged, we'll stop ripping too early.
       */
      int start = r->job->chapter_start;
      hb_chapter_t * chap = hb_list_item( r->title->list_chapter, chapter_end - 1 );

      chapter_end = chap->index;
      if (start > 1)
      {
         chap = hb_list_item( r->title->list_chapter, start - 1 );
         start = chap->index;
      }
      /* end chapter mapping XXX */

      if( !hb_dvd_start( r->dvd, r->title->index, start ) )
      {
          hb_dvd_close( &r->dvd );
          return;
      }
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
        if( chapter > chapter_end )
        {
            hb_log( "reader: end of chapter %d (media %d) reached at media chapter %d",
                    r->job->chapter_end, chapter_end, chapter );
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

        if( r->job->indepth_scan )
        {
            /*
             * Need to update the progress during a subtitle scan
             */
            hb_state_t state;

#define p state.param.working

            state.state = HB_STATE_WORKING;
            p.progress = (float)chapter / (float)r->job->chapter_end;
            if( p.progress > 1.0 )
            {
                p.progress = 1.0;
            }
            p.rate_avg = 0.0;
            p.hours    = -1;
            p.minutes  = -1;
            p.seconds  = -1;
            hb_set_state( r->job->h, &state );
        }

        hb_demux_ps( r->ps, list, &r->demux );

        while( ( buf = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, buf );
            fifos = GetFifoForId( r->job, buf->id );

            if ( ! r->saw_video )
            {
                /* The first video packet defines 'time zero' so discard
                   data until we get a video packet with a PTS */
                if ( buf->id == 0xE0 && buf->start != -1 )
                {
                    r->saw_video = 1;
                    r->demux.scr_offset = buf->start;
                    hb_log( "reader: first SCR %llu scr_offset %llu",
                            r->demux.last_scr, r->demux.scr_offset );
                }
                else
                {
                    fifos = NULL;
                }
            }
            if( fifos )
            {
                if ( buf->start != -1 )
                {
                    /* this packet has a PTS - correct it for the initial
                       video time offset & any timing discontinuities so that
                       everything after this sees a continuous clock with 0
                       being the time of the first video packet. */
                    buf->start -= r->demux.scr_offset;
                }
                buf->sequence = r->sequence++;
                for( n = 0; fifos[n] != NULL; n++)
                {
                    if( n != 0 )
                    {
                        /*
                         * Replace the buffer with a new copy of itself for when
                         * it is being sent down multiple fifos.
                         */
                        buf_old = buf;
                        buf = hb_buffer_init(buf_old->size);
                        memcpy( buf->data, buf_old->data, buf->size );
                        hb_buffer_copy_settings( buf, buf_old );
                    }

                    while( !*r->die && !r->job->done &&
                           hb_fifo_is_full( fifos[n] ) )
                    {
                        /*
                         * Loop until the incoming fifo is reaqdy to receive
                         * this buffer.
                         */
                        hb_snooze( 50 );
                    }

                    hb_fifo_push( fifos[n], buf );
                }
            }
            else
            {
                hb_buffer_close( &buf );
            }
        }
    }

    /* send an empty buffer upstream to signal we're done */
    hb_fifo_push( r->job->fifo_mpeg2, hb_buffer_init(0) );

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

    hb_log( "reader: done. %d scr changes", r->demux.scr_changes );

    free( r );
    _r = NULL;
}

/***********************************************************************
 * GetFifoForId
 ***********************************************************************
 *
 **********************************************************************/
static hb_fifo_t ** GetFifoForId( hb_job_t * job, int id )
{
    hb_title_t    * title = job->title;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i, n;
    static hb_fifo_t * fifos[8];

    memset(fifos, 0, sizeof(fifos));

    if( id == 0xE0 )
    {
        if( job->indepth_scan )
        {
            /*
             * Ditch the video here during the indepth scan until
             * we can improve the MPEG2 decode performance.
             */
            return NULL;
        }
        else
        {
            fifos[0] = job->fifo_mpeg2;
            return fifos;
        }
    }

    if( job->indepth_scan ) {
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

                    fifos[0] = subtitle->fifo_in;
                    return fifos;
                }
                break;
            }
        }
    } else {
        if( ( subtitle = hb_list_item( title->list_subtitle, 0 ) ) &&
            id == subtitle->id )
        {
            fifos[0] = subtitle->fifo_in;
            return fifos;
        }
    }
    if( !job->indepth_scan )
    {
        n = 0;
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );
            if( id == audio->id )
            {
                fifos[n++] = audio->priv.fifo_in;
            }
        }

        if( n != 0 )
        {
            return fifos;
        }
    }

    return NULL;
}

