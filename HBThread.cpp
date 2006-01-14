/* $Id: HBThread.cpp,v 1.3 2003/08/24 13:27:41 titer Exp $ */

#include "HBCommon.h"
#include "HBThread.h"

#include <OS.h>

HBThread::HBThread( char * name, int priority = B_LOW_PRIORITY )
{
    fName = strdup( name );
    fThread = spawn_thread( ThreadFunc, fName, priority, this );
}

HBThread::~HBThread()
{
    fDie = true;
    Log( "Stopping thread %d (\"%s\")", fThread, fName );
    int32 exit_value;
    wait_for_thread( fThread, &exit_value );
    Log( "Thread %d stopped (\"%s\")", fThread, fName );
    free( fName );
}

void HBThread::Run()
{
    fDie = false;
    resume_thread( fThread );
    Log( "Thread %d started (\"%s\")", fThread, fName );
}

void HBThread::Suspend()
{
    suspend_thread( fThread );
}

void HBThread::Resume()
{
    resume_thread( fThread );
}

long HBThread::ThreadFunc( HBThread * _this )
{
    _this->DoWork();
    return 0;
}

void HBThread::DoWork()
{
}
