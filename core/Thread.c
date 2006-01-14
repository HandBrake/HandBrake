/* $Id: Thread.c,v 1.10 2004/01/14 21:37:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Thread.h"

struct HBThread
{
    char    * name;
    int       priority;
    void      (*function) ( void * );
    void    * arg;

#if defined( HB_BEOS )
    int       thread;
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_t thread;
#elif defined( HB_CYGWIN )
    HANDLE    thread;
#endif
};

static void ThreadFunc( void * t );

HBThread * HBThreadInit( char * name, void (* function)(void *),
                         void * arg, int priority )
{
    HBThread * t;
    if( !( t = malloc( sizeof( HBThread ) ) ) )
    {
        HBLog( "HBThreadInit: malloc() failed, gonna crash" );
        return NULL;
    }

    t->name     = strdup( name );
    t->priority = priority;
    t->function = function;
    t->arg      = arg;

#if defined( HB_BEOS )
    t->thread = spawn_thread( (int32 (*)( void * )) ThreadFunc,
                              name, priority, t );
    resume_thread( t->thread );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_create( &t->thread, NULL,
                    (void * (*)( void * )) ThreadFunc, t );
#elif defined( HB_CYGWIN )
    t->thread = CreateThread( NULL, 0,
        (LPTHREAD_START_ROUTINE) ThreadFunc, t, 0, NULL );
#endif

    HBLog( "HBThreadInit: thread %d started (\"%s\")",
           t->thread, t->name );

    return t;
}

static void ThreadFunc( void * _t )
{
    HBThread * t = (HBThread*) _t;

#if defined( HB_MACOSX )
    struct sched_param param;
    memset( &param, 0, sizeof( struct sched_param ) );
    param.sched_priority = t->priority;
    if( pthread_setschedparam( pthread_self(), SCHED_OTHER, &param ) )
    {
        HBLog( "HBThreadInit: couldn't set thread priority" );
    }
#endif

    t->function( t->arg );
}

void HBThreadClose( HBThread ** _t )
{
    HBThread * t = *_t;

#if defined( HB_BEOS )
    long exitValue;
    wait_for_thread( t->thread, &exitValue );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_join( t->thread, NULL );
#elif defined( HB_CYGWIN )
    WaitForSingleObject( t->thread, INFINITE );
#endif

    HBLog( "HBThreadClose: thread %d stopped (\"%s\")",
           t->thread, t->name );

    free( t->name );
    free( t );
    *_t = NULL;
}

HBLock * HBLockInit()
{
    HBLock * l;
    if( !( l = malloc( sizeof( HBLock ) ) ) )
    {
        HBLog( "HBLockInit: malloc() failed, gonna crash" );
        return NULL;
    }

#if defined( HB_BEOS )
    l->sem = create_sem( 1, "sem" );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_mutex_init( &l->mutex, NULL );
#elif defined( HB_CYGWIN )
    l->mutex = CreateMutex( 0, FALSE, 0 );
#endif

    return l;
}

void HBLockClose( HBLock ** _l )
{
    HBLock * l = *_l;

#if defined( HB_BEOS )
    delete_sem( l->sem );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_mutex_destroy( &l->mutex );
#elif defined( HB_CYGWIN )
    CloseHandle( l->mutex );
#endif
    free( l );

    *_l = NULL;
}

HBCond * HBCondInit()
{
    HBCond * c = malloc( sizeof( HBCond ) );

#if defined( HB_BEOS )
    c->thread = -1;
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_cond_init( &c->cond, NULL );
#elif defined( HB_CYGWIN )
    /* TODO */
#endif

    return c;
}

void HBCondClose( HBCond ** _c )
{
    HBCond * c = *_c;

#if defined( HB_BEOS )
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_cond_destroy( &c->cond );
#elif defined( HB_CYGWIN )
    /* TODO */
#endif
    free( c );

    *_c = NULL;
}

