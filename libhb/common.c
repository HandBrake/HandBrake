/* $Id: common.c,v 1.15 2005/03/17 19:22:47 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "lang.h"
#include "hb.h"

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
  { "256", 256 }, { "320", 320 }, { "384", 384 }, { "448", 448 },
  { "512", 512 }, { "576", 576 }, { "640", 640 }, { "768", 768 } };
int hb_audio_bitrates_count = sizeof( hb_audio_bitrates ) /
                              sizeof( hb_rate_t );
int hb_audio_bitrates_default = 8; /* 128 kbps */

static hb_error_handler_t *error_handler = NULL;

hb_mixdown_t hb_audio_mixdowns[] =
{ { "Mono",               "HB_AMIXDOWN_MONO",      "mono",   HB_AMIXDOWN_MONO      },
  { "Stereo",             "HB_AMIXDOWN_STEREO",    "stereo", HB_AMIXDOWN_STEREO    },
  { "Dolby Surround",     "HB_AMIXDOWN_DOLBY",     "dpl1",   HB_AMIXDOWN_DOLBY     },
  { "Dolby Pro Logic II", "HB_AMIXDOWN_DOLBYPLII", "dpl2",   HB_AMIXDOWN_DOLBYPLII },
  { "6-channel discrete", "HB_AMIXDOWN_6CH",       "6ch",    HB_AMIXDOWN_6CH       }
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

// Given an input bitrate, find closest match in the set of allowed bitrates
int hb_find_closest_audio_bitrate(int bitrate)
{
    int ii;
    int result;

    // result is highest rate if none found during search.
    // rate returned will always be <= rate asked for.
    result = hb_audio_bitrates[0].rate;
    for (ii = hb_audio_bitrates_count-1; ii >= 0; ii++)
    {
        if (bitrate >= hb_audio_bitrates[ii].rate)
        {
            result = hb_audio_bitrates[ii].rate;
            break;
        }
    }
    return result;
}

// Get the bitrate low and high limits for a codec/samplerate/mixdown triplet
void hb_get_audio_bitrate_limits(uint32_t codec, int samplerate, int mixdown, int *low, int *high)
{
    int channels;

    channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mixdown);
    switch (codec)
    {
        case HB_ACODEC_AC3:
            *low = 32 * channels;
            *high = 640;
            break;

        case HB_ACODEC_CA_AAC:
            *low = channels * 40;
            if (samplerate <= 44100)
                *low = channels * 32;
            if (samplerate <= 24000)
                *low = channels * 16;
            *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
            break;

        case HB_ACODEC_FAAC:
            *low = 32 * channels;
            *high = 160 * channels;
            if (*high > 768)
                *high = 768;
            break;

        case HB_ACODEC_VORBIS:
            *low = channels * 16;
            *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
            if (samplerate > 24000)
            {
                if (channels > 2)
                {
                    // Vorbis minimum is around 30kbps/ch for 6ch 
                    // at rates > 24k (32k/44.1k/48k) 
                    *low = 32 * channels;
                }
                else
                {
                    // Allow 24kbps mono and 48kbps stereo at rates > 24k 
                    // (32k/44.1k/48k)
                    *low = 24 * channels;
                }
            }
            break;

        default:
            *low = hb_audio_bitrates[0].rate;
            *high = hb_audio_bitrates[hb_audio_bitrates_count-1].rate;
            break;
    }
}

// Given an input bitrate, sanitize it.  Check low and high limits and
// make sure it is in the set of allowed bitrates.
int hb_get_best_audio_bitrate( uint32_t codec, int bitrate, int samplerate, int mixdown)
{
    int low, high;

    hb_get_audio_bitrate_limits(codec, samplerate, mixdown, &low, &high);
    if (bitrate > high)
        bitrate = high;
    if (bitrate < low)
        bitrate = low;
    bitrate = hb_find_closest_audio_bitrate(bitrate);
    return bitrate;
}

// Get the default bitrate for a given codec/samplerate/mixdown triplet.
int hb_get_default_audio_bitrate( uint32_t codec, int samplerate, int mixdown )
{
    int bitrate, channels;
    int sr_shift;

    channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mixdown);

    // Min bitrate is established such that we get good quality
    // audio as a minimum.
    sr_shift = (samplerate <= 24000) ? 1 : 0;

    switch ( codec )
    {
        case HB_ACODEC_AC3:
            if (channels == 1)
                bitrate = 96;
            else if (channels <= 2)
                bitrate = 224;
            else
                bitrate = 640;
            break;
        default:
            bitrate = channels * 80;
    }
    bitrate >>= sr_shift;
    bitrate = hb_get_best_audio_bitrate( codec, bitrate, samplerate, mixdown );
    return bitrate;
}

int hb_get_best_mixdown( uint32_t codec, int layout )
{
    switch (layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK)
    {
        // stereo input or something not handled below
        default:
        case HB_INPUT_CH_LAYOUT_STEREO:
            // mono gets mixed up to stereo & more than stereo gets mixed down
            return HB_AMIXDOWN_STEREO;

        // mono input
        case HB_INPUT_CH_LAYOUT_MONO:
            // everything else passes through
            return HB_AMIXDOWN_MONO;

        // dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input
        // the A52 flags don't allow for a way to distinguish between DPL1 and
        // DPL2 on a DVD so we always assume a DPL1 source for A52_DOLBY.
        case HB_INPUT_CH_LAYOUT_DOLBY:
            return HB_AMIXDOWN_DOLBY;

        // 4 channel discrete
        case HB_INPUT_CH_LAYOUT_2F2R:
        case HB_INPUT_CH_LAYOUT_3F1R:
            // a52dec and libdca can't upmix to 6ch, 
            // so we must downmix these.
            return HB_AMIXDOWN_DOLBYPLII;

        // 5 or 6 channel discrete
        case HB_INPUT_CH_LAYOUT_3F2R:
            if ( ! ( layout & HB_INPUT_CH_LAYOUT_HAS_LFE ) )
            {
                // we don't do 5 channel discrete so mixdown to DPLII
                // a52dec and libdca can't upmix to 6ch, 
                // so we must downmix this.
                return HB_AMIXDOWN_DOLBYPLII;
            }
            else
            {
                switch (codec)
                {
                    case HB_ACODEC_LAME:
                        return HB_AMIXDOWN_DOLBYPLII;

                    default:
                        return HB_AMIXDOWN_6CH;
                }
            }
    }
}

int hb_get_default_mixdown( uint32_t codec, int layout )
{
    switch (layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK)
    {
        // stereo input or something not handled below
        default:
        case HB_INPUT_CH_LAYOUT_STEREO:
            // mono gets mixed up to stereo & more than stereo gets mixed down
            return HB_AMIXDOWN_STEREO;

        // mono input
        case HB_INPUT_CH_LAYOUT_MONO:
            // everything else passes through
            return HB_AMIXDOWN_MONO;

        // dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input
        // the A52 flags don't allow for a way to distinguish between DPL1 and
        // DPL2 on a DVD so we always assume a DPL1 source for A52_DOLBY.
        case HB_INPUT_CH_LAYOUT_DOLBY:
            return HB_AMIXDOWN_DOLBY;

        // 4 channel discrete
        case HB_INPUT_CH_LAYOUT_2F2R:
        case HB_INPUT_CH_LAYOUT_3F1R:
            // a52dec and libdca can't upmix to 6ch, 
            // so we must downmix these.
            return HB_AMIXDOWN_DOLBYPLII;

        // 5 or 6 channel discrete
        case HB_INPUT_CH_LAYOUT_3F2R:
            if ( ! ( layout & HB_INPUT_CH_LAYOUT_HAS_LFE ) )
            {
                // we don't do 5 channel discrete so mixdown to DPLII
                // a52dec and libdca can't upmix to 6ch, 
                // so we must downmix this.
                return HB_AMIXDOWN_DOLBYPLII;
            }
            else
            {
                switch (codec)
                {
                    case HB_ACODEC_AC3:
                        return HB_AMIXDOWN_6CH;

                    default:
                        return HB_AMIXDOWN_DOLBYPLII;
                }
            }
    }
}

/**********************************************************************
 * hb_reduce
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce( int *x, int *y, int num, int den )
{
    // find the greatest common divisor of num & den by Euclid's algorithm
    int n = num, d = den;
    while ( d )
    {
        int t = d;
        d = n % d;
        n = t;
    }

    // at this point n is the gcd. if it's non-zero remove it from num
    // and den. Otherwise just return the original values.
    if ( n )
    {
        *x = num / n;
        *y = den / n;
    }
    else
    {
        *x = num;
        *y = den;
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
    int  min_width;
    int min_height;
    int    modulus;

    /* don't do anything unless the title has complete size info */
    if ( title->height == 0 || title->width == 0 || title->aspect == 0 )
    {
        hb_log( "hb_fix_aspect: incomplete info for title %d: "
                "height = %d, width = %d, aspect = %.3f",
                title->index, title->height, title->width, title->aspect );
        return;
    }

    // min_width and min_height should be multiples of modulus
    min_width    = 32;
    min_height   = 32;
    modulus      = job->modulus ? job->modulus : 16;

    for( i = 0; i < 4; i++ )
    {
        // Sanity check crop values are zero or positive multiples of 2
        if( i < 2 )
        {
            // Top, bottom
            job->crop[i] = MIN( EVEN( job->crop[i] ), EVEN( ( title->height / 2 ) - ( min_height / 2 ) ) );
            job->crop[i] = MAX( 0, job->crop[i] );
        }
        else
        {
            // Left, right
            job->crop[i] = MIN( EVEN( job->crop[i] ), EVEN( ( title->width / 2 ) - ( min_width / 2 ) ) );
            job->crop[i] = MAX( 0, job->crop[i] );
        }
    }

    double par = (double)title->width / ( (double)title->height * title->aspect );
    double cropped_sar = (double)( title->height - job->crop[0] - job->crop[1] ) /
                         (double)( title->width - job->crop[2] - job->crop[3] );
    double ar = par * cropped_sar;

    // Dimensions must be greater than minimum and multiple of modulus
    if( keep == HB_KEEP_WIDTH )
    {
        job->width  = MULTIPLE_MOD( job->width, modulus );
        job->width  = MAX( min_width, job->width );
        job->height = MULTIPLE_MOD( (uint64_t)( (double)job->width * ar ), modulus );
        job->height = MAX( min_height, job->height );
    }
    else
    {
        job->height = MULTIPLE_MOD( job->height, modulus );
        job->height = MAX( min_height, job->height );
        job->width  = MULTIPLE_MOD( (uint64_t)( (double)job->height / ar ), modulus );
        job->width  = MAX( min_width, job->width );
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

    /* Get the duration in seconds */
    length = 0;
    for( i = job->chapter_start; i <= job->chapter_end; i++ )
    {
        chapter = hb_list_item( title->list_chapter, i - 1 );
        length += chapter->duration;
    }
    length += 135000;
    length /= 90000;

    if( size == -1 )
    {
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        avail = job->vbitrate * 125 * length;
        avail += length * interjob->vrate * overhead / interjob->vrate_base;
    }

    /* Video overhead */
    avail -= length * job->vrate * overhead / job->vrate_base;

    if( size == -1 )
    {
        goto ret;
    }

    for( i = 0; i < hb_list_count(job->list_audio); i++ )
    {
        /* Audio data */
        int abitrate;
        audio = hb_list_item( job->list_audio, i);

        /* How many audio samples we put in each frame */
        switch( audio->config.out.codec )
        {
            case HB_ACODEC_FAAC:
            case HB_ACODEC_CA_AAC:
            case HB_ACODEC_VORBIS:
                samples_per_frame = 1024;
                break;
            case HB_ACODEC_LAME:
                samples_per_frame = 1152;
                break;
            case HB_ACODEC_AC3_PASS:
            case HB_ACODEC_DCA_PASS:
            case HB_ACODEC_AC3:
            case HB_ACODEC_DCA:
                samples_per_frame = 1536;
                break;
            default:
                return 0;
        }

        if( audio->config.out.codec == HB_ACODEC_AC3_PASS ||
            audio->config.out.codec == HB_ACODEC_DCA_PASS)
        {
            /*
             * For pass through we take the bitrate from the input audio
             * bitrate as we are simply passing it through.
             */
            abitrate = audio->config.in.bitrate / 8;
        }
        else
        {
            /*
             * Where we are transcoding the audio we use the destination
             * bitrate.
             */
            abitrate = audio->config.out.bitrate * 1000 / 8;
        }
        avail -= length * abitrate;

        /* Audio overhead */
        avail -= length * audio->config.out.samplerate * overhead / samples_per_frame;
    }

ret:
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

int global_verbosity_level; //Necessary for hb_deep_log
/**********************************************************************
 * hb_deep_log
 **********************************************************************
 * If verbose mode is >= level, print message with timestamp. Messages
 * longer than 360 characters are stripped ;p
 *********************************************************************/
void hb_deep_log( hb_debug_level_t level, char * log, ... )
{
    char        string[362]; /* 360 chars + \n + \0 */
    time_t      _now;
    struct tm * now;
    va_list     args;

    if( global_verbosity_level < level )
    {
        /* Hiding message */
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
    char        rep_string[181];
    static char last_string[181];
    static int  last_error_count = 0;
    static uint64_t last_series_error_time = 0;
    static hb_lock_t *mutex = 0;
    va_list     args;
    uint64_t time_now;

    /* Convert the message to a string */
    va_start( args, log );
    vsnprintf( string, 180, log, args );
    va_end( args );

    if( !mutex )
    {
        mutex = hb_lock_init();
    }

    hb_lock( mutex );

    time_now = hb_get_date();

    if( strcmp( string, last_string) == 0 )
    {
        /*
         * The last error and this one are the same, don't log it
         * just count it instead, unless it was more than one second
         * ago.
         */
        last_error_count++;
        if( last_series_error_time + ( 1000 * 1 ) > time_now )
        {
            hb_unlock( mutex );
            return;
        } 
    }
    
    /*
     * A new error, or the same one more than 10sec since the last one
     * did we have any of the same counted up?
     */
    if( last_error_count > 0 )
    {
        /*
         * Print out the last error to ensure context for the last 
         * repeated message.
         */
        if( error_handler )
        {
            error_handler( last_string );
        } else {
            hb_log( "%s", last_string );
        }
        
        if( last_error_count > 1 )
        {
            /*
             * Only print out the repeat message for more than 2 of the
             * same, since we just printed out two of them already.
             */
            snprintf( rep_string, 180, "Last error repeated %d times", 
                      last_error_count - 1 );
            
            if( error_handler )
            {
                error_handler( rep_string );
            } else {
                hb_log( "%s", rep_string );
            }
        }
        
        last_error_count = 0;
    }

    last_series_error_time = time_now;

    strcpy( last_string, string );

    /*
     * Got the error in a single string, send it off to be dispatched.
     */
    if( error_handler )
    {
        error_handler( string );
    } else {
        hb_log( "%s", string );
    }

    hb_unlock( mutex );
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
hb_title_t * hb_title_init( char * path, int index )
{
    hb_title_t * t;

    t = calloc( sizeof( hb_title_t ), 1 );

    t->index         = index;
    t->list_audio    = hb_list_init();
    t->list_chapter  = hb_list_init();
    t->list_subtitle = hb_list_init();
    t->list_attachment = hb_list_init();
    strcat( t->path, path );
    // default to decoding mpeg2
    t->video_id      = 0xE0;
    t->video_codec   = WORK_DECMPEG2;

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
    hb_attachment_t * attachment;

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
        if ( subtitle->extradata )
        {
            free( subtitle->extradata );
            subtitle->extradata = NULL;
        }
        free( subtitle );
    }
    hb_list_close( &t->list_subtitle );
    
    while( ( attachment = hb_list_item( t->list_attachment, 0 ) ) )
    {
        hb_list_rem( t->list_attachment, attachment );
        if ( attachment->name )
        {
            free( attachment->name );
            attachment->name = NULL;
        }
        if ( attachment->data )
        {
            free( attachment->data );
            attachment->data = NULL;
        }
        free( attachment );
    }
    hb_list_close( &t->list_attachment );

    if( t->metadata )
    {
        if( t->metadata->coverart )
        {
            free( t->metadata->coverart );
        }
        free( t->metadata );
    }

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

/**********************************************************************
 * hb_audio_copy
 **********************************************************************
 *
 *********************************************************************/
hb_audio_t *hb_audio_copy(const hb_audio_t *src)
{
    hb_audio_t *audio = NULL;

    if( src )
    {
        audio = calloc(1, sizeof(*audio));
        memcpy(audio, src, sizeof(*audio));
    }
    return audio;
}

/**********************************************************************
 * hb_audio_new
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_config_init(hb_audio_config_t * audiocfg)
{
    /* Set read only paramaters to invalid values */
    audiocfg->in.codec = 0xDEADBEEF;
    audiocfg->in.bitrate = -1;
    audiocfg->in.samplerate = -1;
    audiocfg->in.channel_layout = 0;
    audiocfg->in.version = 0;
    audiocfg->in.mode = 0;
    audiocfg->flags.ac3 = 0;
    audiocfg->lang.description[0] = 0;
    audiocfg->lang.simple[0] = 0;
    audiocfg->lang.iso639_2[0] = 0;

    /* Initalize some sensable defaults */
    audiocfg->in.track = audiocfg->out.track = 0;
    audiocfg->out.codec = HB_ACODEC_FAAC;
    audiocfg->out.bitrate = 128;
    audiocfg->out.samplerate = 44100;
    audiocfg->out.mixdown = HB_AMIXDOWN_DOLBYPLII;
    audiocfg->out.dynamic_range_compression = 0;
    audiocfg->out.name = NULL;
}

/**********************************************************************
 * hb_audio_add
 **********************************************************************
 *
 *********************************************************************/
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg)
{
    hb_title_t *title = job->title;
    hb_audio_t *audio;

    audio = hb_audio_copy( hb_list_item( title->list_audio, audiocfg->in.track ) );
    if( audio == NULL )
    {
        /* We fail! */
        return 0;
    }

    if( (audiocfg->in.bitrate != -1) && (audiocfg->in.codec != 0xDEADBEEF) )
    {
        /* This most likely means the client didn't call hb_audio_config_init
         * so bail.
         */
        return 0;
    }

    /* Really shouldn't ignore the passed out track, but there is currently no
     * way to handle duplicates or out-of-order track numbers.
     */
    audio->config.out.track = hb_list_count(job->list_audio) + 1;
    audio->config.out.codec = audiocfg->out.codec;
    if( (audiocfg->out.codec & HB_ACODEC_MASK) == audio->config.in.codec &&
        (audiocfg->out.codec & HB_ACODEC_PASS_FLAG ) )
    {
        /* Pass-through, copy from input. */
        audio->config.out.samplerate = audio->config.in.samplerate;
        audio->config.out.bitrate = audio->config.in.bitrate;
        audio->config.out.dynamic_range_compression = 0;
        audio->config.out.mixdown = 0;
    }
    else
    {
        /* Non pass-through, use what is given. */
        audio->config.out.codec &= ~HB_ACODEC_PASS_FLAG;
        audio->config.out.samplerate = audiocfg->out.samplerate;
        audio->config.out.bitrate = audiocfg->out.bitrate;
        audio->config.out.dynamic_range_compression = audiocfg->out.dynamic_range_compression;
        audio->config.out.mixdown = audiocfg->out.mixdown;
    }

    hb_list_add(job->list_audio, audio);
    return 1;
}

hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i)
{
    hb_audio_t *audio = NULL;

    if( (audio = hb_list_item(list, i)) )
        return &(audio->config);

    return NULL;
}

/**********************************************************************
 * hb_subtitle_copy
 **********************************************************************
 *
 *********************************************************************/
hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src)
{
    hb_subtitle_t *subtitle = NULL;

    if( src )
    {
        subtitle = calloc(1, sizeof(*subtitle));
        memcpy(subtitle, src, sizeof(*subtitle));
        if ( src->extradata )
        {
            subtitle->extradata = malloc( src->extradata_size );
            memcpy( subtitle->extradata, src->extradata, src->extradata_size );
        }
    }
    return subtitle;
}

/**********************************************************************
 * hb_subtitle_add
 **********************************************************************
 *
 *********************************************************************/
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track)
{
    hb_title_t *title = job->title;
    hb_subtitle_t *subtitle;

    subtitle = hb_subtitle_copy( hb_list_item( title->list_subtitle, track ) );
    if( subtitle == NULL )
    {
        /* We fail! */
        return 0;
    }
    subtitle->config = *subtitlecfg;
    hb_list_add(job->list_subtitle, subtitle);
    return 1;
}

int hb_srt_add( const hb_job_t * job, 
                const hb_subtitle_config_t * subtitlecfg, 
                const char *lang )
{
    hb_subtitle_t *subtitle;
    iso639_lang_t *language = NULL;
    int retval = 0;

    subtitle = calloc( 1, sizeof( *subtitle ) );
    
    subtitle->id = (hb_list_count(job->list_subtitle) << 8) | 0xFF;
    subtitle->format = TEXTSUB;
    subtitle->source = SRTSUB;

    language = lang_for_code2( lang );

    if( language )
    {

        strcpy( subtitle->lang, language->eng_name );
        strncpy( subtitle->iso639_2, lang, 4 );
        
        subtitle->config = *subtitlecfg;
        subtitle->config.dest = PASSTHRUSUB;

        hb_list_add(job->list_subtitle, subtitle);
        retval = 1;
    }
    return retval;
}

char * hb_strdup_printf( char * fmt, ... )
{
    int       len;
    va_list   ap;
    int       size = 256;
    char    * str;
    char    * tmp;

    str = malloc( size );
    if ( str == NULL )
        return NULL;

    while (1) 
    {
        /* Try to print in the allocated space. */
        va_start( ap, fmt );
        len = vsnprintf( str, size, fmt, ap );
        va_end( ap );

        /* If that worked, return the string. */
        if ( len > -1 && len < size )
        {
            return str;
        }

        /* Else try again with more space. */
        if ( len > -1 )     /* glibc 2.1 */
            size = len + 1; /* precisely what is needed */
        else                /* glibc 2.0 */
            size *= 2;      /* twice the old size */
        tmp = realloc( str, size );
        if ( tmp == NULL )
        {
            free( str );
            return NULL;
        }
        else
            str = tmp;
    }
}

/**********************************************************************
 * hb_attachment_copy
 **********************************************************************
 *
 *********************************************************************/
hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src)
{
    hb_attachment_t *attachment = NULL;

    if( src )
    {
        attachment = calloc(1, sizeof(*attachment));
        memcpy(attachment, src, sizeof(*attachment));
        if ( src->name )
        {
            attachment->name = strdup( src->name );
        }
        if ( src->data )
        {
            attachment->data = malloc( src->size );
            memcpy( attachment->data, src->data, src->size );
        }
    }
    return attachment;
}

/**********************************************************************
 * hb_yuv2rgb
 **********************************************************************
 * Converts a YCbCr pixel to an RGB pixel.
 * 
 * This conversion is lossy (due to rounding and clamping).
 * 
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_yuv2rgb(int yuv)
{
    double y, Cr, Cb;
    int r, g, b;

    y  = (yuv >> 16) & 0xff;
    Cb = (yuv >>  8) & 0xff;
    Cr = (yuv      ) & 0xff;

    r = 1.164 * (y - 16)                      + 2.018 * (Cb - 128);
    g = 1.164 * (y - 16) - 0.813 * (Cr - 128) - 0.391 * (Cb - 128);
    b = 1.164 * (y - 16) + 1.596 * (Cr - 128);
    
    r = (r < 0) ? 0 : r;
    g = (g < 0) ? 0 : g;
    b = (b < 0) ? 0 : b;
    
    r = (r > 255) ? 255 : r;
    g = (g > 255) ? 255 : g;
    b = (b > 255) ? 255 : b;
    
    return (r << 16) | (g << 8) | b;
}

/**********************************************************************
 * hb_rgb2yuv
 **********************************************************************
 * Converts an RGB pixel to a YCbCr pixel.
 * 
 * This conversion is lossy (due to rounding and clamping).
 * 
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_rgb2yuv(int rgb)
{
    double r, g, b;
    int y, Cr, Cb;
    
    r = (rgb >> 16) & 0xff;
    g = (rgb >>  8) & 0xff;
    b = (rgb      ) & 0xff;

    y  =  16. + ( 0.257 * r) + (0.504 * g) + (0.098 * b);
    Cb = 128. + (-0.148 * r) - (0.291 * g) + (0.439 * b);
    Cr = 128. + ( 0.439 * r) - (0.368 * g) - (0.071 * b);
    
    y = (y < 0) ? 0 : y;
    Cb = (Cb < 0) ? 0 : Cb;
    Cr = (Cr < 0) ? 0 : Cr;
    
    y = (y > 255) ? 255 : y;
    Cb = (Cb > 255) ? 255 : Cb;
    Cr = (Cr > 255) ? 255 : Cr;
    
    return (y << 16) | (Cb << 8) | Cr;
}

const char * hb_subsource_name( int source )
{
    switch (source)
    {
        case VOBSUB:
            return "VOBSUB";
        case SRTSUB:
            return "SRT";
        case CC608SUB:
            return "CC";
        case CC708SUB:
            return "CC";
        case UTF8SUB:
            return "UTF-8";
        case TX3GSUB:
            return "TX3G";
        case SSASUB:
            return "SSA";
        default:
            return "Unknown";
    }
}

