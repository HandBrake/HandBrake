/* $Id: common.c,v 1.15 2005/03/17 19:22:47 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"

/**********************************************************************
 * Global variables
 *********************************************************************/
hb_rate_t hb_video_rates[] =
{ { "5",  5400000 }, { "10",     2700000 }, { "12", 2250000 },
  { "15", 1800000 }, { "23.976", 1126125 }, { "24", 1125000 },
  { "25", 1080000 }, { "29.97",  900900  } };
int hb_video_rates_count = sizeof( hb_video_rates ) /
                           sizeof( hb_rate_t );

hb_rate_t hb_audio_rates[] =
{ { "22.05", 22050 }, { "24", 24000 }, { "32", 32000 },
  { "44.1",  44100 }, { "48", 48000 } };
int hb_audio_rates_count   = sizeof( hb_audio_rates ) /
                             sizeof( hb_rate_t );
int hb_audio_rates_default = 3; /* 44100 Hz */

hb_rate_t hb_audio_bitrates[] =
{ {  "32",  32 }, {  "40",  40 }, {  "48",  48 }, {  "56",  56 },
  {  "64",  64 }, {  "80",  80 }, {  "96",  96 }, { "112", 112 },
  { "128", 128 }, { "160", 160 }, { "192", 192 }, { "224", 224 },
  { "256", 256 }, { "320", 320 }, { "384", 384 } };
int hb_audio_bitrates_count = sizeof( hb_audio_bitrates ) /
                              sizeof( hb_rate_t );
int hb_audio_bitrates_default = 8; /* 128 kbps */

static hb_error_handler_t *error_handler = NULL;

hb_mixdown_t hb_audio_mixdowns[] =
{ { "Mono",               "HB_AMIXDOWN_MONO",      "mono",   HB_AMIXDOWN_MONO      },
  { "Stereo",             "HB_AMIXDOWN_STEREO",    "stereo", HB_AMIXDOWN_STEREO    },
  { "Dolby Surround",     "HB_AMIXDOWN_DOLBY",     "dpl1",   HB_AMIXDOWN_DOLBY     },
  { "Dolby Pro Logic II", "HB_AMIXDOWN_DOLBYPLII", "dpl2",   HB_AMIXDOWN_DOLBYPLII },
  { "6-channel discrete", "HB_AMIXDOWN_6CH",       "6ch",    HB_AMIXDOWN_6CH       },
  { "AC-3 Pass-through",  "HB_AMIXDOWN_AC3",       "ac-3",   HB_AMIXDOWN_AC3       },
  { "Dolby Pro Logic II + AC-3",  "HB_AMIXDOWN_DOLBYPLII_AC3", "dpl2+ac3", HB_AMIXDOWN_DOLBYPLII_AC3 },
};
int hb_audio_mixdowns_count = sizeof( hb_audio_mixdowns ) /
                              sizeof( hb_mixdown_t );

int hb_mixdown_get_mixdown_from_short_name( const char * short_name )
{
    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (strcmp(hb_audio_mixdowns[i].short_name, short_name) == 0)
        {
            return hb_audio_mixdowns[i].amixdown;
        }
    }
    return 0;
}

const char * hb_mixdown_get_short_name_from_mixdown( int amixdown )
{
    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (hb_audio_mixdowns[i].amixdown == amixdown)
        {
            return hb_audio_mixdowns[i].short_name;
        }
    }
    return "";
}

/**********************************************************************
 * hb_reduce
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce( int *x, int *y, int num, int den )
{
    int lower = MIN( num, den );
    int i;
    *x = num;
    *y = den;
    for( i = lower - 1; i > 1; --i )
    {
        if( ( num % i == 0 ) && ( den % i == 0 ) )
        {
            *x = num / i;
            *y = den / i;
            break;
        }
    }
}

/**********************************************************************
 * hb_fix_aspect
 **********************************************************************
 * Given the output width (if HB_KEEP_WIDTH) or height
 * (HB_KEEP_HEIGHT) and the current crop values, calculates the
 * correct height or width in order to respect the DVD aspect ratio
 *********************************************************************/
void hb_fix_aspect( hb_job_t * job, int keep )
{
    hb_title_t * title = job->title;
    int          i;

    /* don't do anything unless the title has complete size info */
    if ( title->height == 0 || title->width == 0 || title->aspect == 0 )
    {
        hb_log( "hb_fix_aspect: incomplete info for title %d: "
                "height = %d, width = %d, aspect = %d", 
                title->height, title->width, title->aspect );
        return;
    }

    /* Sanity checks:
       Widths and heights must be multiples of 16 and greater than or
       equal to 16
       Crop values must be multiples of 2, greater than or equal to 0
       and less than half of the dimension */
    job->width   = MULTIPLE_16( job->width );
    job->height  = MULTIPLE_16( job->height );
    job->width   = MAX( 16, job->width );
    job->height  = MAX( 16, job->height );
    for( i = 0; i < 4; i++ )
    {
        job->crop[i] = EVEN( job->crop[i] );
        job->crop[i] = MAX( 0, job->crop[i] );
        if( i < 2 )
        {
            /* Top, bottom */
            job->crop[i] = MIN( job->crop[i], ( title->height / 2 ) - 2 );
        }
        else
        {
            /* Left, right */
            job->crop[i] = MIN( job->crop[i], ( title->width / 2 ) - 2 );
        }
    }

    if( keep == HB_KEEP_WIDTH )
    {
        job->height = MULTIPLE_16(
            (uint64_t) job->width * title->width * HB_ASPECT_BASE *
              ( title->height - job->crop[0] - job->crop[1] ) /
            ( (uint64_t) title->height * title->aspect *
              ( title->width - job->crop[2] - job->crop[3] ) ) );
        job->height = MAX( 16, job->height );
    }
    else
    {
        job->width = MULTIPLE_16(
            (uint64_t) job->height * title->height * title->aspect *
              ( title->width - job->crop[2] - job->crop[3] ) /
            ( (uint64_t) title->width * HB_ASPECT_BASE *
              ( title->height - job->crop[0] - job->crop[1] ) ) );
        job->width = MAX( 16, job->width );
    }
}

/**********************************************************************
 * hb_calc_bitrate
 **********************************************************************
 * size: in megabytes
 *********************************************************************/
int hb_calc_bitrate( hb_job_t * job, int size )
{
    int64_t avail = (int64_t) size * 1024 * 1024;
    int64_t length;
    int     overhead;
    int     samples_per_frame;
    int     i;

    hb_title_t   * title = job->title;
    hb_chapter_t * chapter;
    hb_audio_t   * audio;

    /* How many overhead bytes are used for each frame
       (quite guessed) */
    switch( job->mux )
    {
       case HB_MUX_MP4:
       case HB_MUX_PSP:
		case HB_MUX_IPOD:
		case HB_MUX_MKV:
            overhead = 6;
            break;
        case HB_MUX_AVI:
            overhead = 24;
            break;
        case HB_MUX_OGM:
            overhead = 6;
            break;
        default:
            return 0;
    }

    /* How many audio samples we put in each frame */
    switch( job->acodec )
    {
        case HB_ACODEC_FAAC:
        case HB_ACODEC_VORBIS:
            samples_per_frame = 1024;
            break;
        case HB_ACODEC_LAME:
            samples_per_frame = 1152;
            break;
        case HB_ACODEC_AC3:
            samples_per_frame = 1536;
            break;
        default:
            return 0;
    }

    /* Get the duration in seconds */
    length = 0;
    for( i = job->chapter_start; i <= job->chapter_end; i++ )
    {
        chapter = hb_list_item( title->list_chapter, i - 1 );
        length += chapter->duration;
    }
    length += 135000;
    length /= 90000;

    /* Video overhead */
    avail -= length * job->vrate * overhead / job->vrate_base;

    for( i = 0; job->audios[i] >= 0; i++ )
    {
        /* Audio data */
        int abitrate;
        if( job->acodec & HB_ACODEC_AC3 ||
            job->audio_mixdowns[i] == HB_AMIXDOWN_AC3)
        {
            /*
             * For AC-3 we take the bitrate from the input audio
             * bitrate as we are simply passing it through.
             */
            audio = hb_list_item( title->list_audio, job->audios[i] );
            abitrate = audio->bitrate / 8;
        }
        else
        {
            /*
             * Where we are transcoding the audio we use the destination
             * bitrate.
             */
            abitrate = job->abitrate * 1000 / 8;
        }
        avail -= length * abitrate;

        /* Audio overhead */
        avail -= length * job->arate * overhead / samples_per_frame;
    }

    if( avail < 0 )
    {
        return 0;
    }

    return ( avail / ( 125 * length ) );
}

/**********************************************************************
 * hb_list implementation
 **********************************************************************
 * Basic and slow, but enough for what we need
 *********************************************************************/

#define HB_LIST_DEFAULT_SIZE 20

struct hb_list_s
{
    /* Pointers to items in the list */
    void ** items;

    /* How many (void *) allocated in 'items' */
    int     items_alloc;

    /* How many valid pointers in 'items' */
    int     items_count;
};

/**********************************************************************
 * hb_list_init
 **********************************************************************
 * Allocates an empty list ready for HB_LIST_DEFAULT_SIZE items
 *********************************************************************/
hb_list_t * hb_list_init()
{
    hb_list_t * l;

    l              = calloc( sizeof( hb_list_t ), 1 );
    l->items       = calloc( HB_LIST_DEFAULT_SIZE * sizeof( void * ), 1 );
    l->items_alloc = HB_LIST_DEFAULT_SIZE;

    return l;
}

/**********************************************************************
 * hb_list_count
 **********************************************************************
 * Returns the number of items currently in the list
 *********************************************************************/
int hb_list_count( hb_list_t * l )
{
    return l->items_count;
}

/**********************************************************************
 * hb_list_add
 **********************************************************************
 * Adds an item at the end of the list, making it bigger if necessary.
 * Can safely be called with a NULL pointer to add, it will be ignored.
 *********************************************************************/
void hb_list_add( hb_list_t * l, void * p )
{
    if( !p )
    {
        return;
    }

    if( l->items_count == l->items_alloc )
    {
        /* We need a bigger boat */
        l->items_alloc += HB_LIST_DEFAULT_SIZE;
        l->items        = realloc( l->items,
                                   l->items_alloc * sizeof( void * ) );
    }

    l->items[l->items_count] = p;
    (l->items_count)++;
}

/**********************************************************************
 * hb_list_rem
 **********************************************************************
 * Remove an item from the list. Bad things will happen if called
 * with a NULL pointer or if the item is not in the list.
 *********************************************************************/
void hb_list_rem( hb_list_t * l, void * p )
{
    int i;

    /* Find the item in the list */
    for( i = 0; i < l->items_count; i++ )
    {
        if( l->items[i] == p )
        {
            break;
        }
    }

    /* Shift all items after it sizeof( void * ) bytes earlier */
    memmove( &l->items[i], &l->items[i+1],
             ( l->items_count - i - 1 ) * sizeof( void * ) );

    (l->items_count)--;
}

/**********************************************************************
 * hb_list_item
 **********************************************************************
 * Returns item at position i, or NULL if there are not that many
 * items in the list
 *********************************************************************/
void * hb_list_item( hb_list_t * l, int i )
{
    if( i < 0 || i >= l->items_count )
    {
        return NULL;
    }

    return l->items[i];
}

/**********************************************************************
 * hb_list_bytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, returns the total
 * number of bytes in the list
 *********************************************************************/
int hb_list_bytes( hb_list_t * l )
{
    hb_buffer_t * buf;
    int           ret;
    int           i;

    ret = 0;
    for( i = 0; i < hb_list_count( l ); i++ )
    {
        buf  = hb_list_item( l, i );
        ret += buf->size - buf->cur;
    }

    return ret;
}

/**********************************************************************
 * hb_list_seebytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>, keeping the list unmodified.
 *********************************************************************/
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    int           i;

    for( i = 0, copied = 0; copied < size; i++ )
    {
        buf     = hb_list_item( l, i );
        copying = MIN( buf->size - buf->cur, size - copied );
        memcpy( &dst[copied], &buf->data[buf->cur], copying );
        copied += copying;
    }
}

/**********************************************************************
 * hb_list_getbytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>. What's copied is removed from the list.
 * The variable pointed by <pts> is set to the PTS of the buffer the
 * first byte has been got from.
 * The variable pointed by <pos> is set to the position of that byte
 * in that buffer.
 *********************************************************************/
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, uint64_t * pos )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    uint8_t       has_pts;

    /* So we won't have to deal with NULL pointers */
     uint64_t dummy1, dummy2;

    if( !pts ) pts = &dummy1;
    if( !pos ) pos = &dummy2;

    for( copied = 0, has_pts = 0; copied < size;  )
    {
        buf     = hb_list_item( l, 0 );
        copying = MIN( buf->size - buf->cur, size - copied );
        memcpy( &dst[copied], &buf->data[buf->cur], copying );

        if( !has_pts )
        {
            *pts    = buf->start;
            *pos    = buf->cur;
            has_pts = 1;
        }

        buf->cur += copying;
        if( buf->cur >= buf->size )
        {
            hb_list_rem( l, buf );
            hb_buffer_close( &buf );
        }

        copied += copying;
    }
}

/**********************************************************************
 * hb_list_empty
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, close them all and
 * close the list.
 *********************************************************************/
void hb_list_empty( hb_list_t ** _l )
{
    hb_list_t * l = *_l;
    hb_buffer_t * b;

    while( ( b = hb_list_item( l, 0 ) ) )
    {
        hb_list_rem( l, b );
        hb_buffer_close( &b );
    }

    hb_list_close( _l );
}

/**********************************************************************
 * hb_list_close
 **********************************************************************
 * Free memory allocated by hb_list_init. Does NOT free contents of
 * items still in the list.
 *********************************************************************/
void hb_list_close( hb_list_t ** _l )
{
    hb_list_t * l = *_l;

    free( l->items );
    free( l );

    *_l = NULL;
}

/**********************************************************************
 * hb_log
 **********************************************************************
 * If verbose mode is one, print message with timestamp. Messages
 * longer than 180 characters are stripped ;p
 *********************************************************************/
void hb_log( char * log, ... )
{
    char        string[362]; /* 360 chars + \n + \0 */
    time_t      _now;
    struct tm * now;
    va_list     args;

    if( !getenv( "HB_DEBUG" ) )
    {
        /* We don't want to print it */
        return;
    }

    /* Get the time */
    _now = time( NULL );
    now  = localtime( &_now );
    sprintf( string, "[%02d:%02d:%02d] ",
             now->tm_hour, now->tm_min, now->tm_sec );

    /* Convert the message to a string */
    va_start( args, log );
    vsnprintf( string + 11, 349, log, args );
    va_end( args );

    /* Add the end of line */
    strcat( string, "\n" );

    /* Print it */
    fprintf( stderr, "%s", string );
}

/**********************************************************************
 * hb_error
 **********************************************************************
 * Using whatever output is available display this error.
 *********************************************************************/
void hb_error( char * log, ... )
{
    char        string[181]; /* 180 chars + \0 */
    va_list     args;

    /* Convert the message to a string */
    va_start( args, log );
    vsnprintf( string, 180, log, args );
    va_end( args );

    /*
     * Got the error in a single string, send it off to be dispatched.
     */
    if( error_handler )
    {
        error_handler( string );
    } else {
        hb_log( string );
    }
}

void hb_register_error_handler( hb_error_handler_t * handler )
{
    error_handler = handler;
}

/**********************************************************************
 * hb_title_init
 **********************************************************************
 *
 *********************************************************************/
hb_title_t * hb_title_init( char * dvd, int index )
{
    hb_title_t * t;

    t = calloc( sizeof( hb_title_t ), 1 );

    t->index         = index;
    t->list_audio    = hb_list_init();
    t->list_chapter  = hb_list_init();
    t->list_subtitle = hb_list_init();
    strcat( t->dvd, dvd );

    return t;
}

/**********************************************************************
 * hb_title_close
 **********************************************************************
 *
 *********************************************************************/
void hb_title_close( hb_title_t ** _t )
{
    hb_title_t * t = *_t;
    hb_audio_t * audio;
    hb_chapter_t * chapter;
    hb_subtitle_t * subtitle;

    while( ( audio = hb_list_item( t->list_audio, 0 ) ) )
    {
        hb_list_rem( t->list_audio, audio );
        free( audio );
    }
    hb_list_close( &t->list_audio );

    while( ( chapter = hb_list_item( t->list_chapter, 0 ) ) )
    {
        hb_list_rem( t->list_chapter, chapter );
        free( chapter );
    }
    hb_list_close( &t->list_chapter );

    while( ( subtitle = hb_list_item( t->list_subtitle, 0 ) ) )
    {
        hb_list_rem( t->list_subtitle, subtitle );
        free( subtitle );
    }
    hb_list_close( &t->list_subtitle );

    free( t );
    *_t = NULL;
}

/**********************************************************************
 * hb_filter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_filter_close( hb_filter_object_t ** _f )
{
    hb_filter_object_t * f = *_f;

    f->close( f->private_data );

    if( f->name )
        free( f->name );
    if( f->settings )
        free( f->settings );

    free( f );
    *_f = NULL;
}

