/* $Id: Thread.cpp,v 1.19 2003/10/01 21:17:17 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#if defined( SYS_BEOS )
#  include <OS.h>
#  include <Locker.h>
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
#  include <pthread.h>
#endif

#include "Thread.h"
#include "Fifo.h"

HBThread::HBThread( char * name, int priority )
{
    fName      = strdup( name );
    fPriority  = priority;
    fDie       = false;
    fSuspend   = false;
}

HBThread::~HBThread()
{
    fDie     = true;
    fSuspend = false;

#if defined( SYS_BEOS )
    long exit_value;
    wait_for_thread( fThread, &exit_value );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_join( fThread, NULL );
#endif

    Log( "Thread %d stopped (\"%s\")", fThread, fName );
    free( fName );
}

void HBThread::Run()
{
#if defined( SYS_BEOS )
    fThread = spawn_thread( (int32 (*)(void *)) ThreadFunc,
                            fName, fPriority, this );
    resume_thread( fThread );
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_create( &fThread, NULL,
                    (void * (*)(void *)) ThreadFunc, this );
#if 0
#if defined( SYS_MACOSX )
    int    policy;
    struct sched_param param;
    memset( &param, 0, sizeof( struct sched_param ) );
    if( fPriority < 0 )
    {
        param.sched_priority = (-1) * fPriority;
        policy = SCHED_OTHER;
    }
    else
    {
        param.sched_priority = fPriority;
        policy = SCHED_RR;
    }
    if ( pthread_setschedparam( fThread, policy, &param ) )
    {
        Log( "HBThread::Run : couldn't set thread priority" );
    }
#endif
#endif
#endif

    Log( "Thread %d started (\"%s\")", fThread, fName );
}

void HBThread::Stop()
{
    Log( "Stopping thread %d (\"%s\")", fThread, fName );

    fDie     = true;
    fSuspend = false;
}

void HBThread::Suspend()
{
    fSuspend = true;
}

void HBThread::Resume()
{
    fSuspend = false;
}

bool HBThread::Push( HBFifo * fifo, HBBuffer * buffer )
{
    while( !fDie )
    {
        if( fifo->Push( buffer ) )
        {
            return true;
        }

        Snooze( 10000 );
    }

    delete buffer;
    return false;
}

HBBuffer * HBThread::Pop( HBFifo * fifo )
{
    HBBuffer * buffer;

    while( !fDie )
    {
        if( ( buffer = fifo->Pop() ) )
        {
            return buffer;
        }

        Snooze( 10000 );
    }

    return NULL;
}

void HBThread::ThreadFunc( HBThread * _this )
{
    _this->DoWork();
}

void HBThread::DoWork()
{
}


HBLock::HBLock()
{
#if defined( SYS_BEOS )
    fLocker = new BLocker();
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_init( &fMutex, NULL );
#endif
}

HBLock::~HBLock()
{
#if defined( SYS_BEOS )
    delete fLocker;
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_destroy( &fMutex );
#endif
}

void HBLock::Lock()
{
#if defined( SYS_BEOS )
    fLocker->Lock();
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_lock( &fMutex );
#endif
}

void HBLock::Unlock()
{
#if defined( SYS_BEOS )
    fLocker->Unlock();
#elif defined( SYS_MACOSX ) || defined( SYS_LINUX )
    pthread_mutex_unlock( &fMutex );
#endif
}
