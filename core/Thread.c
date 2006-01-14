/* $Id: Thread.c,v 1.4 2003/11/06 15:51:36 titer Exp $

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
    
#if defined( SYS_BEOS )
    int       thread;
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_t thread;
#elif defined( SYS_CYGWIN )
    /* TODO */
    int       thread;
#endif
};

#ifndef SYS_CYGWIN
static void ThreadFunc( void * t );
#endif

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

#if defined( SYS_BEOS )
    t->thread = spawn_thread( (int32 (*)( void * )) ThreadFunc,
                              name, priority, t );
    resume_thread( t->thread );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_create( &t->thread, NULL,
                    (void * (*)( void * )) ThreadFunc, t );
#elif defined( SYS_CYGWIN )
    /* TODO */
    t->thread = 0;
#endif

    HBLog( "HBThreadInit: thread %d started (\"%s\")",
           t->thread, t->name );

    return t;
}

#ifndef SYS_CYGWIN
static void ThreadFunc( void * _t )
{
    HBThread * t = (HBThread*) _t;

#if defined( SYS_MACOSX )
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
#endif

void HBThreadClose( HBThread ** _t )
{
    HBThread * t = *_t;
    
#if defined( SYS_BEOS )
    long exitValue;
    wait_for_thread( t->thread, &exitValue );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_join( t->thread, NULL );
#elif defined( SYS_CYGWIN )
    /* TODO */
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

#if defined( SYS_BEOS )
    l->sem = create_sem( 1, "sem" );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_init( &l->mutex, NULL );
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif

    return l;
}

void HBLockClose( HBLock ** _l )
{
    HBLock * l = *_l;
    
#if defined( SYS_BEOS )
    delete_sem( l->sem );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_destroy( &l->mutex );
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif
    free( l );

    *_l = NULL;
}

