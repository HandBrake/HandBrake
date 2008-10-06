/* $Id: ports.c,v 1.15 2005/10/15 18:05:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <time.h>
#include <sys/time.h>

#if defined( SYS_BEOS )
#include <OS.h>
#include <signal.h>
#elif defined( SYS_CYGWIN )
#include <windows.h>
#elif defined( SYS_SunOS )
#include <sys/processor.h>
#endif

#if USE_PTHREAD
#include <pthread.h>
#endif

//#ifdef SYS_CYGWIN
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
//#endif

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
#elif defined( SYS_CYGWIN )
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

#if defined( SYS_BEOS )
    {
        system_info info;
        get_system_info( &info );
        cpu_count = info.cpu_count;
    }

#elif defined( SYS_DARWIN ) || defined( SYS_FREEBSD )
    FILE * info;
    char   buffer[16];

    if( ( info = popen( "/usr/sbin/sysctl hw.ncpu", "r" ) ) )
    {
        memset( buffer, 0, 16 );
        if( fgets( buffer, 15, info ) )
        {
            if( sscanf( buffer, "hw.ncpu: %d", &cpu_count ) != 1 )
            {
                cpu_count = 1;
            }
        }
        fclose( info );
    }

#elif defined( SYS_LINUX )
    {
        FILE * info;
        char   buffer[8];

        if( ( info = popen( "grep -c '^processor' /proc/cpuinfo",
                            "r" ) ) )
        {
            memset( buffer, 0, 8 );
            if( fgets( buffer, 7, info ) )
            {
                if( sscanf( buffer, "%d", &cpu_count ) != 1 )
                {
                    cpu_count = 1;
                }
            }
            fclose( info );
        }
    }

#elif defined( SYS_CYGWIN )
    SYSTEM_INFO cpuinfo;
    GetSystemInfo( &cpuinfo );
    cpu_count = cpuinfo.dwNumberOfProcessors;
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
    cpu_count = MIN( cpu_count, 8 );

    return cpu_count;
}

/************************************************************************
 * Get a tempory directory for HB
 ***********************************************************************/
void hb_get_tempory_directory( hb_handle_t * h, char path[512] )
{
    char base[512];

    /* Create the base */
#ifdef SYS_CYGWIN
    char *p;
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
    strcpy( base, "/tmp" );
#endif
    /* I prefer to remove evntual last '/' (for cygwin) */
    if( base[strlen(base)-1] == '/' )
        base[strlen(base)-1] = '\0';

    snprintf( path, 512, "%s/hb.%d", base, hb_get_pid( h ) );
}

/************************************************************************
 * Get a tempory filename for HB
 ***********************************************************************/
void hb_get_tempory_filename( hb_handle_t * h, char name[1024],
                              char *fmt, ... )
{
    va_list args;

    hb_get_tempory_directory( h, name );
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
//#ifdef SYS_CYGWIN
//    mkdir( name );
//#else
    mkdir( name, 0755 );
//#endif
}

/************************************************************************
 * Portable thread implementation
 ***********************************************************************/
struct hb_thread_s
{
    char       * name;
    int          priority;
    void      (* function) ( void * );
    void       * arg;

    hb_lock_t  * lock;
    int          exited;

#if defined( SYS_BEOS )
    thread_id    thread;
#elif USE_PTHREAD
    pthread_t    thread;
//#elif defined( SYS_CYGWIN )
//    HANDLE       thread;
#endif
};

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
static void hb_thread_func( void * _t )
{
    hb_thread_t * t = (hb_thread_t *) _t;

#if defined( SYS_DARWIN )
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
    hb_log( "thread %x exited (\"%s\")", t->thread, t->name );
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
hb_thread_t * hb_thread_init( char * name, void (* function)(void *),
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

    hb_log( "thread %x started (\"%s\")", t->thread, t->name );
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

    hb_log( "thread %x joined (\"%s\")",
            t->thread, t->name );

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
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_NORMAL);

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

