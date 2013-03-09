/* ports.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifdef USE_PTHREAD
#ifdef SYS_LINUX
#define _GNU_SOURCE
#include <sched.h>
#endif
#include <pthread.h>
#endif

#ifdef SYS_BEOS
#include <kernel/OS.h>
#endif

#if defined(SYS_DARWIN) || defined(SYS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef SYS_OPENBSD
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#ifdef SYS_MINGW
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#ifdef SYS_CYGWIN
#include <windows.h>
#endif

#ifdef SYS_MINGW
#include <pthread.h>
#include <windows.h>
#endif

#ifdef SYS_SunOS
#include <sys/processor.h>
#endif

#include <time.h>
#include <sys/time.h>

#if defined( SYS_LINUX )
#include <linux/cdrom.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#elif defined( SYS_OPENBSD )
#include <sys/dvdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

#ifdef __APPLE__
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

#include <stddef.h>
#include <unistd.h>

#include "hb.h"

/************************************************************************
 * hb_get_date()
 ************************************************************************
 * Returns the current date in milliseconds.
 * On Win32, we implement a gettimeofday emulation here because
 * libdvdread and libmp4v2 use it without checking.
 ************************************************************************/
/*
#ifdef SYS_CYGWIN
struct timezone
{
};

int gettimeofday( struct timeval * tv, struct timezone * tz )
{
    int tick;
    tick        = GetTickCount();
    tv->tv_sec  = tick / 1000;
    tv->tv_usec = ( tick % 1000 ) * 1000;
    return 0;
}
#endif
*/

int hb_dvd_region(char *device, int *region_mask)
{
#if defined( DVD_LU_SEND_RPC_STATE ) && defined( DVD_AUTH )
    struct stat  st;
    dvd_authinfo ai;
    int          fd, ret;

    fd = open( device, O_RDONLY );
    if ( fd < 0 )
        return -1;
    if ( fstat( fd, &st ) < 0 )
	{
        close( fd );
        return -1;
	}
    if ( !( S_ISBLK( st.st_mode ) || S_ISCHR( st.st_mode ) ) )
	{
        close( fd );
        return -1;
	}

    ai.type = DVD_LU_SEND_RPC_STATE;
    ret = ioctl(fd, DVD_AUTH, &ai);
    close( fd );
    if ( ret < 0 )
        return ret;

    *region_mask = ai.lrpcs.region_mask;
    return 0;
#else
    return -1;
#endif
}

uint64_t hb_get_date()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return( (uint64_t) tv.tv_sec * 1000 + (uint64_t) tv.tv_usec / 1000 );
}

/************************************************************************
 * hb_snooze()
 ************************************************************************
 * Waits <delay> milliseconds.
 ************************************************************************/
void hb_snooze( int delay )
{
    if( delay < 1 )
    {
        return;
    }
#if defined( SYS_BEOS )
    snooze( 1000 * delay );
#elif defined( SYS_DARWIN ) || defined( SYS_LINUX ) || defined( SYS_FREEBSD) || defined( SYS_SunOS )
    usleep( 1000 * delay );
#elif defined( SYS_CYGWIN ) || defined( SYS_MINGW )
    Sleep( delay );
#endif
}

/************************************************************************
 * hb_get_cpu_count()
 ************************************************************************
 * Whenever possible, returns the number of CPUs on the current
 * computer. Returns 1 otherwise.
 * The detection is actually only performed on the first call.
 ************************************************************************/
int hb_get_cpu_count()
{
    static int cpu_count = 0;

    if( cpu_count )
    {
        return cpu_count;
    }
    cpu_count = 1;

#if defined(SYS_CYGWIN) || defined(SYS_MINGW)
    SYSTEM_INFO cpuinfo;
    GetSystemInfo( &cpuinfo );
    cpu_count = cpuinfo.dwNumberOfProcessors;

#elif defined(SYS_LINUX)
    unsigned int bit;
    cpu_set_t p_aff;
    memset( &p_aff, 0, sizeof(p_aff) );
    sched_getaffinity( 0, sizeof(p_aff), &p_aff );
    for( cpu_count = 0, bit = 0; bit < sizeof(p_aff); bit++ )
         cpu_count += (((uint8_t *)&p_aff)[bit / 8] >> (bit % 8)) & 1;

#elif defined(SYS_BEOS)
    system_info info;
    get_system_info( &info );
    cpu_count = info.cpu_count;

#elif defined(SYS_DARWIN) || defined(SYS_FREEBSD) || defined(SYS_OPENBSD)
    size_t length = sizeof( cpu_count );
#ifdef SYS_OPENBSD
    int mib[2] = { CTL_HW, HW_NCPU };
    if( sysctl(mib, 2, &cpu_count, &length, NULL, 0) )
#else
    if( sysctlbyname("hw.ncpu", &cpu_count, &length, NULL, 0) )
#endif
    {
        cpu_count = 1;
    }

#elif defined( SYS_SunOS )
    {
        processorid_t cpumax;
        int i,j=0;

        cpumax = sysconf(_SC_CPUID_MAX);

        for(i = 0; i <= cpumax; i++ )
        {
            if(p_online(i, P_STATUS) != -1)
            {
                j++;
            }
        }
        cpu_count=j;
    }
#endif

    cpu_count = MAX( 1, cpu_count );
    cpu_count = MIN( cpu_count, 64 );

    return cpu_count;
}

/************************************************************************
 * Get a temporary directory for HB
 ***********************************************************************/
void hb_get_temporary_directory( char path[512] )
{
    char base[512];
    char *p;

    /* Create the base */
#if defined( SYS_CYGWIN ) || defined( SYS_MINGW )
    int i_size = GetTempPath( 512, base );
    if( i_size <= 0 || i_size >= 512 )
    {
        if( getcwd( base, 512 ) == NULL )
            strcpy( base, "c:" ); /* Bad fallback but ... */
    }

    /* c:/path/ works like a charm under cygwin(win32?) so use it */
    while( ( p = strchr( base, '\\' ) ) )
        *p = '/';
#else
    if( (p = getenv( "TMPDIR" ) ) != NULL ||
        (p = getenv( "TEMP" ) ) != NULL )
        strcpy( base, p );
    else
        strcpy( base, "/tmp" );
#endif
    /* I prefer to remove evntual last '/' (for cygwin) */
    if( base[strlen(base)-1] == '/' )
        base[strlen(base)-1] = '\0';

    snprintf( path, 512, "%s/hb.%d", base, getpid() );
}

/************************************************************************
 * Get a tempory filename for HB
 ***********************************************************************/
void hb_get_tempory_filename( hb_handle_t * h, char name[1024],
                              char *fmt, ... )
{
    va_list args;

    hb_get_temporary_directory( name );
    strcat( name, "/" );

    va_start( args, fmt );
    vsnprintf( &name[strlen(name)], 1024 - strlen(name), fmt, args );
    va_end( args );
}

/************************************************************************
 * hb_mkdir
 ************************************************************************
 * Wrapper to the real mkdir, needed only because it doesn't take a
 * second argument on Win32. Grrr.
 ***********************************************************************/
void hb_mkdir( char * name )
{
#ifdef SYS_MINGW
    mkdir( name );
#else
    mkdir( name, 0755 );
#endif
}

/************************************************************************
 * Portable thread implementation
 ***********************************************************************/
struct hb_thread_s
{
    char          * name;
    int             priority;
    thread_func_t * function;
    void          * arg;

    hb_lock_t     * lock;
    int             exited;

#if defined( SYS_BEOS )
    thread_id       thread;
#elif USE_PTHREAD
    pthread_t       thread;
//#elif defined( SYS_CYGWIN )
//    HANDLE          thread;
#endif
};

/* Get a unique identifier to thread and represent as 64-bit unsigned.
 * If unsupported, the value 0 is be returned.
 * Caller should use result only for display/log purposes.
 */
static uint64_t hb_thread_to_integer( const hb_thread_t* t )
{
#if defined( USE_PTHREAD )
    #if defined( SYS_CYGWIN )
        return (uint64_t)t->thread;
    #elif defined( _WIN32 ) || defined( __MINGW32__ )
        return (uint64_t)(ptrdiff_t)t->thread.p;
    #elif defined( SYS_DARWIN )
        return (unsigned long)t->thread;
    #else
        return (uint64_t)t->thread;
    #endif
#else
    return 0;
#endif
}

/************************************************************************
 * hb_thread_func()
 ************************************************************************
 * We use it as the root routine for any thread, for two reasons:
 *  + To set the thread priority on OS X (pthread_setschedparam() could
 *    be called from hb_thread_init(), but it's nicer to do it as we
 *    are sure it is done before the real routine starts)
 *  + Get informed when the thread exits, so we know whether
 *    hb_thread_close() will block or not.
 ***********************************************************************/
static void attribute_align_thread hb_thread_func( void * _t )
{
    hb_thread_t * t = (hb_thread_t *) _t;

#if defined( SYS_DARWIN ) || defined( SYS_FREEBSD )
    /* Set the thread priority */
    struct sched_param param;
    memset( &param, 0, sizeof( struct sched_param ) );
    param.sched_priority = t->priority;
    pthread_setschedparam( pthread_self(), SCHED_OTHER, &param );
#endif

#if defined( SYS_BEOS )
    signal( SIGINT, SIG_IGN );
#endif

    /* Start the actual routine */
    t->function( t->arg );

    /* Inform that the thread can be joined now */
    hb_deep_log( 2, "thread %"PRIx64" exited (\"%s\")", hb_thread_to_integer( t ), t->name );
    hb_lock( t->lock );
    t->exited = 1;
    hb_unlock( t->lock );
}

/************************************************************************
 * hb_thread_init()
 ************************************************************************
 * name:     user-friendly name
 * function: the thread routine
 * arg:      argument of the routine
 * priority: HB_LOW_PRIORITY or HB_NORMAL_PRIORITY
 ***********************************************************************/
hb_thread_t * hb_thread_init( const char * name, void (* function)(void *),
                              void * arg, int priority )
{
    hb_thread_t * t = calloc( sizeof( hb_thread_t ), 1 );

    t->name     = strdup( name );
    t->function = function;
    t->arg      = arg;
    t->priority = priority;

    t->lock     = hb_lock_init();

    /* Create and start the thread */
#if defined( SYS_BEOS )
    t->thread = spawn_thread( (thread_func) hb_thread_func,
                              name, priority, t );
    resume_thread( t->thread );

#elif USE_PTHREAD
    pthread_create( &t->thread, NULL,
                    (void * (*)( void * )) hb_thread_func, t );

//#elif defined( SYS_CYGWIN )
//    t->thread = CreateThread( NULL, 0,
//        (LPTHREAD_START_ROUTINE) hb_thread_func, t, 0, NULL );
//
//    /* Maybe use THREAD_PRIORITY_LOWEST instead */
//    if( priority == HB_LOW_PRIORITY )
//        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
#endif

    hb_deep_log( 2, "thread %"PRIx64" started (\"%s\")", hb_thread_to_integer( t ), t->name );
    return t;
}

/************************************************************************
 * hb_thread_close()
 ************************************************************************
 * Joins the thread and frees memory.
 ***********************************************************************/
void hb_thread_close( hb_thread_t ** _t )
{
    hb_thread_t * t = *_t;

    /* Join the thread */
#if defined( SYS_BEOS )
    long exit_value;
    wait_for_thread( t->thread, &exit_value );

#elif USE_PTHREAD
    pthread_join( t->thread, NULL );

//#elif defined( SYS_CYGWIN )
//    WaitForSingleObject( t->thread, INFINITE );
#endif

    hb_deep_log( 2, "thread %"PRIx64" joined (\"%s\")", hb_thread_to_integer( t ), t->name );

    hb_lock_close( &t->lock );
    free( t->name );
    free( t );
    *_t = NULL;
}

/************************************************************************
 * hb_thread_has_exited()
 ************************************************************************
 * Returns 1 if the thread can be joined right away, 0 otherwise.
 ***********************************************************************/
int hb_thread_has_exited( hb_thread_t * t )
{
    int exited;

    hb_lock( t->lock );
    exited = t->exited;
    hb_unlock( t->lock );

    return exited;
}

/************************************************************************
 * Portable mutex implementation
 ***********************************************************************/
struct hb_lock_s
{
#if defined( SYS_BEOS )
    sem_id          sem;
#elif USE_PTHREAD
    pthread_mutex_t mutex;
//#elif defined( SYS_CYGWIN )
//    HANDLE          mutex;
#endif
};

/************************************************************************
 * hb_lock_init()
 * hb_lock_close()
 * hb_lock()
 * hb_unlock()
 ************************************************************************
 * Basic wrappers to OS-specific semaphore or mutex functions.
 ***********************************************************************/
hb_lock_t * hb_lock_init()
{
    hb_lock_t * l = calloc( sizeof( hb_lock_t ), 1 );

#if defined( SYS_BEOS )
    l->sem = create_sem( 1, "sem" );
#elif USE_PTHREAD
    pthread_mutexattr_t mta;

    pthread_mutexattr_init(&mta);

#if defined( SYS_CYGWIN ) || defined( SYS_FREEBSD )
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);
#endif

    pthread_mutex_init( &l->mutex, &mta );
//#elif defined( SYS_CYGWIN )
//    l->mutex = CreateMutex( 0, FALSE, 0 );
#endif

    return l;
}

void hb_lock_close( hb_lock_t ** _l )
{
    hb_lock_t * l = *_l;

#if defined( SYS_BEOS )
    delete_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_destroy( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    CloseHandle( l->mutex );
#endif
    free( l );

    *_l = NULL;
}

void hb_lock( hb_lock_t * l )
{
#if defined( SYS_BEOS )
    acquire_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_lock( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    WaitForSingleObject( l->mutex, INFINITE );
#endif
}

void hb_unlock( hb_lock_t * l )
{
#if defined( SYS_BEOS )
    release_sem( l->sem );
#elif USE_PTHREAD
    pthread_mutex_unlock( &l->mutex );
//#elif defined( SYS_CYGWIN )
//    ReleaseMutex( l->mutex );
#endif
}

/************************************************************************
 * Portable condition variable implementation
 ***********************************************************************/
struct hb_cond_s
{
#if defined( SYS_BEOS )
    int                 thread;
#elif USE_PTHREAD
    pthread_cond_t      cond;
//#elif defined( SYS_CYGWIN )
//    HANDLE              event;
#endif
};

/************************************************************************
 * hb_cond_init()
 * hb_cond_close()
 * hb_cond_wait()
 * hb_cond_signal()
 ************************************************************************
 * Win9x is not supported by this implementation (SignalObjectAndWait()
 * only available on Windows 2000/XP).
 ***********************************************************************/
hb_cond_t * hb_cond_init()
{
    hb_cond_t * c = calloc( sizeof( hb_cond_t ), 1 );

    if( c == NULL )
        return NULL;

#if defined( SYS_BEOS )
    c->thread = -1;
#elif USE_PTHREAD
    pthread_cond_init( &c->cond, NULL );
//#elif defined( SYS_CYGWIN )
//    c->event = CreateEvent( NULL, FALSE, FALSE, NULL );
#endif

    return c;
}

void hb_cond_close( hb_cond_t ** _c )
{
    hb_cond_t * c = *_c;

#if defined( SYS_BEOS )
#elif USE_PTHREAD
    pthread_cond_destroy( &c->cond );
//#elif defined( SYS_CYGWIN )
//    CloseHandle( c->event );
#endif
    free( c );

    *_c = NULL;
}

void hb_cond_wait( hb_cond_t * c, hb_lock_t * lock )
{
#if defined( SYS_BEOS )
    c->thread = find_thread( NULL );
    release_sem( lock->sem );
    suspend_thread( c->thread );
    acquire_sem( lock->sem );
    c->thread = -1;
#elif USE_PTHREAD
    pthread_cond_wait( &c->cond, &lock->mutex );
//#elif defined( SYS_CYGWIN )
//    SignalObjectAndWait( lock->mutex, c->event, INFINITE, FALSE );
//    WaitForSingleObject( lock->mutex, INFINITE );
#endif
}

void hb_clock_gettime( struct timespec *tp )
{
    struct timeval tv;

    gettimeofday( &tv, NULL );
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
}

void hb_cond_timedwait( hb_cond_t * c, hb_lock_t * lock, int msec )
{
#if defined( SYS_BEOS )
    c->thread = find_thread( NULL );
    release_sem( lock->sem );
    suspend_thread( c->thread );
    acquire_sem( lock->sem );
    c->thread = -1;
#elif USE_PTHREAD
    struct timespec ts;
    hb_clock_gettime(&ts);
    ts.tv_nsec += (msec % 1000) * 1000000;
    ts.tv_sec += msec / 1000 + (ts.tv_nsec / 1000000000);
    ts.tv_nsec %= 1000000000;
    pthread_cond_timedwait( &c->cond, &lock->mutex, &ts );
#endif
}

void hb_cond_signal( hb_cond_t * c )
{
#if defined( SYS_BEOS )
    while( c->thread != -1 )
    {
        thread_info info;
        get_thread_info( c->thread, &info );
        if( info.state == B_THREAD_SUSPENDED )
        {
            resume_thread( c->thread );
            break;
        }
        /* Looks like we have been called between hb_cond_wait's
           release_sem() and suspend_thread() lines. Wait until the
           thread is actually suspended before we resume it */
        snooze( 5000 );
    }
#elif USE_PTHREAD
    pthread_cond_signal( &c->cond );
//#elif defined( SYS_CYGWIN )
//    PulseEvent( c->event );
#endif
}

void hb_cond_broadcast( hb_cond_t * c )
{
#if USE_PTHREAD
    pthread_cond_broadcast( &c->cond );
#endif
}

/************************************************************************
 * Network
 ***********************************************************************/

struct hb_net_s
{
    int socket;
};

hb_net_t * hb_net_open( char * address, int port )
{
    hb_net_t * n = calloc( sizeof( hb_net_t ), 1 );

    struct sockaddr_in   sock;
    struct hostent     * host;

#ifdef SYS_MINGW
    WSADATA wsaData;
    int iResult, winsock_init = 0;

    // Initialize Winsock
    if (!winsock_init)
    {
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
        {
            hb_log("WSAStartup failed: %d", iResult);
            free(n);
            return NULL;
        }
        winsock_init = 1;
    }
#endif

    /* TODO: find out why this doesn't work on Win32 */
    if( !( host = gethostbyname( address ) ) )
    {
        hb_log( "gethostbyname failed (%s)", address );
        free( n );
        return NULL;
    }

    memset( &sock, 0, sizeof( struct sockaddr_in ) );
    sock.sin_family = host->h_addrtype;
    sock.sin_port   = htons( port );
    memcpy( &sock.sin_addr, host->h_addr, host->h_length );

    if( ( n->socket = socket( host->h_addrtype, SOCK_STREAM, 0 ) ) < 0 )
    {
        hb_log( "socket failed" );
        free( n );
        return NULL;
    }

    if( connect( n->socket, (struct sockaddr *) &sock,
                 sizeof( struct sockaddr_in ) ) < 0 )
    {
        hb_log( "connect failed" );
        free( n );
        return NULL;
    }

    return n;
}

int hb_net_send( hb_net_t * n, char * buffer )
{
    return send( n->socket, buffer, strlen( buffer ), 0 );
}

int hb_net_recv( hb_net_t * n, char * buffer, int size )
{
    return recv( n->socket, buffer, size - 1, 0 );
}

void hb_net_close( hb_net_t ** _n )
{
    hb_net_t * n = (hb_net_t *) *_n;
    close( n->socket );
    free( n );
    *_n = NULL;
}

#ifdef SYS_MINGW
char *strtok_r(char *s, const char *delim, char **save_ptr) 
{
    char *token;

    if (s == NULL) s = *save_ptr;

    /* Scan leading delimiters.  */
    s += strspn(s, delim);
    if (*s == '\0') return NULL;

    /* Find the end of the token.  */
    token = s;
    s = strpbrk(token, delim);
    if (s == NULL)
        /* This token finishes the string.  */
        *save_ptr = strchr(token, '\0');
    else {
        /* Terminate the token and make *SAVE_PTR point past it.  */
        *s = '\0';
        *save_ptr = s + 1;
    }

    return token;
}
#endif

/************************************************************************
* OS Sleep Allow / Prevent
***********************************************************************/

#ifdef __APPLE__
// 128 chars limit for IOPMAssertionCreateWithName
static CFStringRef reasonForActivity= CFSTR("HandBrake is currently scanning and/or encoding");
#endif

void* hb_system_sleep_opaque_init()
{
    void* opaque;
#ifdef __APPLE__
    opaque = calloc( sizeof( IOPMAssertionID ), 1);
    IOPMAssertionID * assertionID = (IOPMAssertionID *)opaque;
    *assertionID = -1;
#endif

    return opaque;
}

void hb_system_sleep_opaque_close(void **_opaque)
{
#ifdef __APPLE__
    IOPMAssertionID * assertionID = (IOPMAssertionID *) *_opaque;
    free( assertionID );
#endif
    *_opaque = NULL;
}

void hb_system_sleep_allow(void *opaque)
{
#ifdef __APPLE__
    IOPMAssertionID * assertionID = (IOPMAssertionID *)opaque;

    if (*assertionID == -1)
        return;

    IOReturn success = IOPMAssertionRelease(*assertionID);

    if (success == kIOReturnSuccess) {
        hb_deep_log( 3, "osxsleep: IOPM assertion %d successfully released, sleep allowed", *assertionID );
        *assertionID = -1;
    } else {
        hb_log( "osxsleep: error while trying to unset power management assertion" );
    }
#endif
}

void hb_system_sleep_prevent(void *opaque)
{
#ifdef __APPLE__
    IOPMAssertionID * assertionID = (IOPMAssertionID *)opaque;

    if (*assertionID != -1)
        return;

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep,
                                                   kIOPMAssertionLevelOn, reasonForActivity, assertionID);
    if (success == kIOReturnSuccess) {
        hb_deep_log( 3, "IOPM assertion %d successfully created, prevent sleep", *assertionID);
    } else {
        hb_log( "osxsleep: error while trying to set power management assertion" );
    }
#endif
}
