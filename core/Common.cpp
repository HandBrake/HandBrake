/* $Id: Common.cpp,v 1.31 2003/10/07 22:48:31 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#if defined( SYS_BEOS )
#  include <OS.h>
#endif

#include "Common.h"
#include "Fifo.h"
#include "MpegDemux.h"
#include "Languages.h"

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

extern "C" {
#include <mpeg2dec/mpeg2.h>
}

void Snooze( uint64_t time )
{
#if defined( SYS_BEOS )
    snooze( time );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    usleep( time );
#endif
}

void Log( char * log, ... )
{
    if( !getenv( "HB_DEBUG" ) )
    {
        return;
    }

    char string[1024];

    /* Show the time */
    time_t _now = time( NULL );
    struct tm * now = localtime( &_now );
    sprintf( string, "[%02d:%02d:%02d] ",
             now->tm_hour, now->tm_min, now->tm_sec );

    /* Convert the message to a string */
    va_list args;
    va_start( args, log );
    int ret = vsnprintf( string + 11, 1011, log, args );
    va_end( args );

    /* Add the end of line */
    string[ret+11] = '\n';
    string[ret+12] = '\0';

    /* Print it */
    fprintf( stderr, "%s", string );
}

char * LanguageForCode( int code )
{
    char codeString[2];
    codeString[0] = ( code >> 8 ) & 0xFF;
    codeString[1] = code & 0xFF;

    iso639_lang_t * lang;
    for( lang = languages; lang->engName; lang++ )
    {
        if( !strncmp( lang->iso639_1, codeString, 2 ) )
        {
            if( *lang->nativeName )
                return lang->nativeName;

            return lang->engName;
        }
    }

    return "Unknown";
}

uint64_t GetDate()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec );
}

int GetCPUCount()
{
    int CPUCount = 1;

#if defined( SYS_BEOS )
    system_info info;
    get_system_info( &info );
    CPUCount = info.cpu_count;

#elif defined( SYS_MACOSX )
    FILE * info;
    char   buffer[256];

    if( ( info = popen( "/usr/sbin/sysctl hw.ncpu", "r" ) ) )
    {
        if( fgets( buffer, 256, info ) )
        {
            int count;
            if( sscanf( buffer, "hw.ncpu: %d", &count ) == 1 )
            {
                CPUCount = count;
            }
            else
            {
                Log( "GetCPUCount: sscanf() failed" );
            }
        }
        else
        {
            Log( "GetCPUCount: fgets() failed" );
        }
        fclose( info );
    }
    else
    {
        Log( "GetCPUCount: popen() failed" );
    }
   
#elif defined( SYS_LINUX )
    FILE * info;
    char   buffer[256];

    if( ( info = fopen( "/proc/cpuinfo", "r" ) ) )
    {
        int count = 0;
        while( fgets( buffer, 256, info ) )
        {
            if( !memcmp( buffer, "processor",
                         sizeof( "processor" ) - 1 ) )
            {
                count++;
            }
        }
        CPUCount = count;
        fclose( info );
    }
    else
    {
        Log( "GetCPUCount: fopen() failed" );
    }
    
#endif
    CPUCount = MAX( 1, CPUCount );
    CPUCount = MIN( CPUCount, 8 );

    return CPUCount;
}

#define HBLIST_DEFAULT_SIZE 20

HBList::HBList()
{
    fItems      = (void**) malloc( HBLIST_DEFAULT_SIZE * sizeof( void* ) );
    fAllocItems = HBLIST_DEFAULT_SIZE;
    fNbItems    = 0;
}

HBList::~HBList()
{
    free( fItems );
}

uint32_t HBList::CountItems()
{
    return fNbItems;
}

void HBList::AddItem( void * item )
{
    if( !item )
    {
        return;
    }

    if( fNbItems == fAllocItems )
    {
        fAllocItems += HBLIST_DEFAULT_SIZE;
        fItems = (void**) realloc( fItems, fAllocItems * sizeof( void* ) );
    }

    fItems[fNbItems] = item;

    fNbItems++;
}

void HBList::RemoveItem( void * item )
{
    if( !item || !fNbItems  )
    {
        return;
    }

    uint32_t i;
    for( i = 0; i < fNbItems; i++ )
    {
        if( fItems[i] == item )
        {
            break;
        }
    }

    if( fItems[i] != item )
    {
        Log( "HBList::RemoveItem() : item not in the list" );
        return;
    }

    for( ; i < fNbItems - 1; i++ )
    {
        fItems[i] = fItems[i+1];
    }

    fNbItems--;
}

void * HBList::ItemAt( uint32_t index )
{
    if( index < fNbItems )
    {
        return fItems[index];
    }

    return NULL;
}

HBTitle::HBTitle( char * device, int index )
{
    fDevice      = strdup( device );
    fIndex       = index;

    fAudioList   = new HBList();
    fPSFifo      = NULL;
    fMpeg2Fifo   = NULL;
    fRawFifo     = NULL;
    fMpeg4Fifo   = NULL;

    fTopCrop     = 0;
    fBottomCrop  = 0;
    fLeftCrop    = 0;
    fRightCrop   = 0;
    fBitrate     = 1024;
    fDeinterlace = false;
    fTwoPass     = false;
}

HBTitle::~HBTitle()
{
    HBAudio * audio;

    while( ( audio = (HBAudio*) fAudioList->ItemAt( 0 ) ) )
    {
        fAudioList->RemoveItem( audio );
        delete audio;
    }
    delete fAudioList;
}

/* Audio track */
HBAudio::HBAudio( int id, char * description )
{
    fId            = id;
    fDescription   = strdup( description );
    fOutSampleRate = 44100;
    fOutBitrate    = 128;

    fAc3Fifo = NULL;
    fRawFifo = NULL;
    fMp3Fifo = NULL;
}

HBAudio::~HBAudio()
{
    free( fDescription );
}
