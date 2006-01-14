/* $Id: Thread.h,v 1.19 2003/10/09 16:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_THREAD_H
#define HB_THREAD_H

#include "Common.h"

#if defined( SYS_BEOS )
#  define HB_LOW_PRIORITY    5
#  define HB_NORMAL_PRIORITY 10
#elif defined( SYS_MACOSX )
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 31
#elif defined( SYS_LINUX )
/* Actually unused */
#  define HB_LOW_PRIORITY    0
#  define HB_NORMAL_PRIORITY 0
#endif

class HBThread
{
    public:
                      HBThread( char * name,
                                int priority = HB_LOW_PRIORITY );
        virtual       ~HBThread();
        void          Suspend();
        void          Resume();
        int           GetPid();

    protected:
        void          Run();
        bool          Push( HBFifo * fifo, HBBuffer * buffer );
        HBBuffer *    Pop( HBFifo * fifo );

        volatile bool fDie;
        volatile bool fSuspend;

    private:
        static void   ThreadFunc( HBThread * _this );
        virtual void  DoWork();

        char        * fName;
        int           fPriority;

#if defined( SYS_BEOS )
        int           fThread;
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
        pthread_t     fThread;
#endif
        int           fPid;
};

#if defined( SYS_BEOS )
class BLocker;
#endif

class HBLock
{
    public:
                          HBLock();
                          ~HBLock();
        void              Lock();
        void              Unlock();

    private:
#if defined( SYS_BEOS )
        BLocker         * fLocker;
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
        pthread_mutex_t   fMutex;
#endif
};

#endif
