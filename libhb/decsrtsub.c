/* 
   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include "hb.h"

struct start_and_end {
    unsigned long start, end;
};

enum
{
    k_state_inEntry,
    k_state_potential_new_entry,
    k_state_timecode,
};

typedef struct srt_entry_s {
    long offset, duration;
    long start, stop;
    char text[1024];
} srt_entry_t;

/*
 * Store all context in the work private struct,
 */
struct hb_work_private_s
{
    hb_job_t *job;
    FILE *file;
    unsigned long current_time;
    unsigned long number_of_entries;
    unsigned long current_state;
    srt_entry_t current_entry;
    iconv_t *iconv_context;
    hb_subtitle_t *subtitle;
    uint64_t start_time;              // In HB time
    uint64_t stop_time;               // In HB time
};

static struct start_and_end read_time_from_string( const char* timeString ) 
{
    // for ex. 00:00:15,248 --> 00:00:16,545
    
    long houres1, minutes1, seconds1, milliseconds1,
	houres2, minutes2, seconds2, milliseconds2;
    
    sscanf(timeString, "%ld:%ld:%ld,%ld --> %ld:%ld:%ld,%ld\n",	&houres1, &minutes1, &seconds1, &milliseconds1,
           &houres2, &minutes2, &seconds2, &milliseconds2);
    
    struct start_and_end result = {
        milliseconds1 + seconds1*1000 + minutes1*60*1000 + houres1*60*60*1000,
        milliseconds2 + seconds2*1000 + minutes2*60*1000 + houres2*60*60*1000};
    return result;
}

/*
 * Read the SRT file and put the entries into the subtitle fifo for all to read
 */
static hb_buffer_t *srt_read( hb_work_private_t *pv )
{

    char line_buffer[1024];

    if( !pv->file )
    {
        return NULL;
    }
    
    while( fgets( line_buffer, sizeof( line_buffer ), pv->file ) ) 
    {
        switch (pv->current_state)
        {
        case k_state_timecode:
        {
            struct start_and_end timing = read_time_from_string( line_buffer );
            pv->current_entry.duration = timing.end - timing.start;
            pv->current_entry.offset = timing.start - pv->current_time;
            
            pv->current_time = timing.end;

            pv->current_entry.start = timing.start;
            pv->current_entry.stop = timing.end;

            pv->current_state = k_state_inEntry;
            continue;				
        }
	
        case k_state_inEntry:
        {
            char *p, *q;
            size_t in_size;
            size_t out_size;
            size_t retval;

            // If the current line is empty, we assume this is the
            //	seperation betwene two entries. In case we are wrong,
            //	the mistake is corrected in the next state.
            if (strcmp(line_buffer, "\n") == 0 || strcmp(line_buffer, "\r\n") == 0) {
                pv->current_state = k_state_potential_new_entry;
                continue;
            }
            

            for( q = pv->current_entry.text; (q < pv->current_entry.text+1024) && *q; q++);
            
            p = line_buffer;

            in_size = strlen(line_buffer);
            out_size = (pv->current_entry.text+1024) - q;

            retval = iconv( pv->iconv_context, &p, &in_size, &q, &out_size);
            *q = '\0';

            if( ( retval == -1 ) && ( errno == EINVAL ) )
            {
                hb_error( "Invalid shift sequence" );
            } else if ( ( retval == -1 ) && ( errno == EILSEQ ) )
            {
                hb_error( "Invalid byte for codeset in input, %"PRId64" bytes discarded", (int64_t)in_size);
            } else if ( ( retval == -1 ) && ( errno == E2BIG ) )
            {
                hb_error( "Not enough space in output buffer");
            }

            break;				
        }
	
        case k_state_potential_new_entry:
        {
            const char endpoint[] = "\0";
            const unsigned long potential_entry_number = strtol(line_buffer, (char**)&endpoint, 10);
            hb_buffer_t *buffer = NULL;
            /*
             * Is this really new next entry begin?
             */
            if (potential_entry_number == pv->number_of_entries + 1) {
                /*
                 * We found the next entry - or a really rare error condition
                 */
                if( *pv->current_entry.text )
                {
                    long length;
                    char *p;
                    uint64_t start_time = ( pv->current_entry.start + 
                                            pv->subtitle->config.offset ) * 90;
                    uint64_t stop_time = ( pv->current_entry.stop + 
                                           pv->subtitle->config.offset ) * 90;

                    if( !( start_time > pv->start_time && stop_time < pv->stop_time ) )
                    {
                        hb_deep_log( 3, "Discarding SRT at time start %"PRId64", stop %"PRId64, start_time, stop_time);
                        memset( &pv->current_entry, 0, sizeof( srt_entry_t ) );
                        ++(pv->number_of_entries);
                        pv->current_state = k_state_timecode;
                        continue;
                    }

                    length = strlen( pv->current_entry.text );

                    for( p = pv->current_entry.text; *p; p++)
                    {
                        if( *p == '\n' || *p == '\r' )
                        {
                            *p = ' ';
                        }
                    }

                    buffer = hb_buffer_init( length + 1 );

                    if( buffer )
                    {
                        buffer->start = start_time - pv->start_time;
                        buffer->stop = stop_time - pv->start_time;

                        memcpy( buffer->data, pv->current_entry.text, length + 1 );
                    }
                }
                memset( &pv->current_entry, 0, sizeof( srt_entry_t ) );
                ++(pv->number_of_entries);
                pv->current_state = k_state_timecode;
                if( buffer )
                {
                    return buffer;
                }
                continue;
            } else {
                /*
                 * Well.. looks like we are in the wrong mode.. lets add the
                 * newline we misinterpreted...
                 */
                strncat(pv->current_entry.text, " ", 1024);
                pv->current_state = k_state_inEntry;
            }
            
            break;
        }
        }
    }
    
    return NULL;
}

static int decsrtInit( hb_work_object_t * w, hb_job_t * job )
{
    int retval = 1;
    hb_work_private_t * pv;
    hb_buffer_t *buffer;
    int i;
    hb_chapter_t * chapter;
    hb_title_t *title = job->title;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    if( pv )
    {
        w->private_data = pv;

        pv->job = job;

        buffer = hb_buffer_init( 0 );
        hb_fifo_push( w->fifo_in, buffer);
        
        pv->file = fopen( w->subtitle->config.src_filename, "r" );
        
        pv->current_state = k_state_potential_new_entry;
        pv->number_of_entries = 0;
        pv->current_time = 0;
        pv->subtitle = w->subtitle;

        /*
         * Figure out the start and stop times from teh chapters being
         * encoded - drop subtitle not in this range.
         */
        pv->start_time = 0;
        for( i = 1; i < job->chapter_start; ++i )
        {
            chapter = hb_list_item( title->list_chapter, i - 1 );
            if( chapter )
            {
                pv->start_time += chapter->duration;
            } else {
                hb_error( "Could not locate chapter %d for SRT start time", i );
                retval = 0;
            }
        }
        chapter = hb_list_item( title->list_chapter, i - 1 );

        if( chapter )
        {
            pv->stop_time = pv->start_time + chapter->duration;
        } else {
            hb_error( "Could not locate chapter %d for SRT stop time", i );
            retval = 0;
        }

        hb_deep_log( 3, "SRT Start time %"PRId64", stop time %"PRId64, pv->start_time, pv->stop_time);

        pv->iconv_context = iconv_open( "utf8", pv->subtitle->config.src_codeset );


        if( pv->iconv_context == (iconv_t) -1 )
        {
            hb_error("Could not open the iconv library with those file formats\n");

        } else {
            memset( &pv->current_entry, 0, sizeof( srt_entry_t ) );
            
            pv->file = fopen( w->subtitle->config.src_filename, "r" );
            
            if( !pv->file )
            {
                hb_error("Could not open the SRT subtitle file '%s'\n", 
                         w->subtitle->config.src_filename);
            } else {
                retval = 0;
            }
        }
    } 

    return retval;
}

static int decsrtWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                       hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = NULL;

    out = srt_read( pv );

    if( out )
    {
        /*
         * Keep a buffer in our input fifo so that we get run.
         */
        hb_fifo_push( w->fifo_in, in);
        *buf_in = NULL;
        *buf_out = out;
    } else {
        *buf_out = NULL;
        return HB_WORK_OK;
    }

    return HB_WORK_OK;  
}

static void decsrtClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    fclose( pv->file );
    iconv_close(pv->iconv_context);
    free( w->private_data );
}

hb_work_object_t hb_decsrtsub =
{
    WORK_DECSRTSUB,
    "SRT Subtitle Decoder",
    decsrtInit,
    decsrtWork,
    decsrtClose
};
