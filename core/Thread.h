/* $Id: Thread.h,v 1.9 2004/02/19 17:59:13 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_THREAD_H
#define HB_THREAD_H

/* System headers */
#if defined( HB_BEOS )
#  include <OS.h>
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
#  include <pthread.h>
#elif defined( HB_CYGWIN )
#  include <windows.h>
#endif

#include "Utils.h"

/* Thread priorities */
#if defined( HB_BEOS )
#  define HB_LOW_PRIORITY    5
#  define HB_NORMAL_PRIORITY 10
#elif defined( HB_MACOSX )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 31
#elif defined( HB_LINUX ) || defined( HB_CYGWIN )
/* Actually unused */
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#endif

/**********************************************************************
 * HBThread/HBLock/HBCond declarations
 **********************************************************************/
HBThread *           HBThreadInit( char * name,
                                   void (* function)(void *),
                                   void * arg, int priority );
void                 HBThreadClose( HBThread ** );

HBLock             * HBLockInit();
static inline void   HBLockLock( HBLock * );
static inline void   HBLockUnlock( HBLock * );
void                 HBLockClose( HBLock ** );

HBCond             * HBCondInit();
static inline void   HBCondWait( HBCond *, HBLock * );
static inline void   HBCondSignal( HBCond * );
void                 HBCondClose( HBCond ** );


/**********************************************************************
 * HBLock implementation (inline functions)
 **********************************************************************/
struct HBLock
{
#if defined( HB_BEOS )
    sem_id          sem;
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_mutex_t mutex;
#elif defined( HB_CYGWIN )
    HANDLE          mutex;
#endif
};

static inline void HBLockLock( HBLock * l )
{
#if defined( HB_BEOS )
    acquire_sem( l->sem );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_mutex_lock( &l->mutex );
#elif defined( HB_CYGWIN )
    WaitForSingleObject( l->mutex, INFINITE );
#endif
}

static inline void HBLockUnlock( HBLock * l )
{
#if defined( HB_BEOS )
    release_sem( l->sem );
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_mutex_unlock( &l->mutex );
#elif defined( HB_CYGWIN )
    ReleaseMutex( l->mutex );
#endif
}


/**********************************************************************
 * HBCond implementation (inline functions)
 **********************************************************************/
struct HBCond
{
#if defined( HB_BEOS )
    int             thread;
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_cond_t  cond;
#elif defined( HB_CYGWIN )
    /* TODO */
#endif

};

static inline void HBCondWait( HBCond * c, HBLock * lock )
{
#if defined( HB_BEOS )
    c->thread = find_thread( NULL );
    release_sem( lock->sem );
    suspend_thread( c->thread );
    acquire_sem( lock->sem );
    c->thread = -1;
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_cond_wait( &c->cond, &lock->mutex );
#elif defined( HB_CYGWIN )
    /* TODO */
#endif
}

static inline void HBCondSignal( HBCond * c )
{
#if defined( HB_BEOS )
    while( c->thread != -1 )
    {
        thread_info info;
        get_thread_info( c->thread, &info );
        if( info.state == B_THREAD_SUSPENDED )
        {
            resume_thread( c->thread );
            break;
        }
        /* In case HBCondSignal is called between HBCondWait's
           release_sem() and suspend_thread() lines, wait a bit */
        snooze( 5000 );
    }
#elif defined( HB_MACOSX ) || defined( HB_LINUX )
    pthread_cond_signal( &c->cond );
#elif defined( HB_CYGWIN )
    /* TODO */
#endif
}

#endif
