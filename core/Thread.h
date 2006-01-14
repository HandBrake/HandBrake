/* $Id: Thread.h,v 1.3 2003/11/06 15:51:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_THREAD_H
#define HB_THREAD_H

#if defined( SYS_BEOS )
#  include <OS.h>
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
#  include <pthread.h>
#endif

#include "Utils.h"

#if defined( SYS_BEOS )
#  define HB_LOW_PRIORITY    5
#  define HB_NORMAL_PRIORITY 10
#elif defined( SYS_MACOSX )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 31
#elif defined( SYS_LINUX ) || defined( SYS_CYGWIN )
/* Actually unused */
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#endif

HBThread * HBThreadInit( char * name, void (* function)(void *),
                         void * arg, int priority );
void       HBThreadClose( HBThread ** );

struct HBLock
{
#if defined( SYS_BEOS )
    sem_id          sem;
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_t mutex;
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif
};

HBLock             * HBLockInit();
static inline void   HBLockLock( HBLock * );
static inline void   HBLockUnlock( HBLock * );
void                 HBLockClose( HBLock ** );

static inline void HBLockLock( HBLock * l )
{
#if defined( SYS_BEOS )
    acquire_sem( l->sem );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_lock( &l->mutex );
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif
}

static inline void HBLockUnlock( HBLock * l )
{
#if defined( SYS_BEOS )
    release_sem( l->sem );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_unlock( &l->mutex );
#elif defined( SYS_CYGWIN )
    /* TODO */
#endif
}

#endif
