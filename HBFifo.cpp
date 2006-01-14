/* $Id: HBFifo.cpp,v 1.11 2003/08/26 07:48:36 titer Exp $ */

#include "HBCommon.h"
#include "HBFifo.h"

#include <Locker.h>

HBBuffer::HBBuffer( int size )
{
    fAllocSize = size;
    fSize = size;
    fKeyFrame = false;
    fData = (uint8_t*) malloc( size );
    
    if( !fData )
    {
        Log( "HBBuffer::HBBuffer() : malloc() failed, gonna crash soon" );
    }
}

HBBuffer::~HBBuffer()
{
    free( fData );
}

void HBBuffer::ReAlloc( int size )
{
    fData = (uint8_t*) realloc( fData, size );

    if( !fData )
    {
        Log( "HBBuffer::ReAlloc() : realloc() failed, gonna crash soon" );
    }

    fAllocSize = size;
}

/* Constructor */
HBFifo::HBFifo( int capacity )
{
    fCapacity    = capacity;
    
    fWhereToPush = 0;
    fWhereToPop  = 0;
    fBuffers     = (HBBuffer**) malloc( ( fCapacity + 1 ) * sizeof( void* ) );
    fLocker      = new BLocker();
    fDie         = false;
}

void HBFifo::Die()
{
    Lock();

    /* Empty the fifo */
    while( fWhereToPush != fWhereToPop )
    {
        HBBuffer * buffer = fBuffers[fWhereToPop];
        fWhereToPop++;
        fWhereToPop %= ( fCapacity + 1 );
        delete buffer;
    }
    
    fDie = true;
    
    Unlock();
}

HBFifo::~HBFifo()
{
    /* Empty the fifo */
    while( fWhereToPush != fWhereToPop )
    {
        HBBuffer * buffer = fBuffers[fWhereToPop];
        fWhereToPop++;
        fWhereToPop %= ( fCapacity + 1 );
        delete buffer;
    }
    
    /* Cleaning */
    free( fBuffers );
    delete fLocker;
}

/* Size() : returns how much the fifo is currently filled */
int HBFifo::Size()
{
    return ( fCapacity + 1 + fWhereToPush - fWhereToPop ) %
             ( fCapacity + 1 );
}

/* Capacity() : simply returns the fifo capacity... */
int HBFifo::Capacity()
{
    return fCapacity;
}

/* Push() : add a packet to the fifo. If the fifo is full, it blocks
   until the packet can be added. Returns true when it is successful,
   or false if the fifo has been destroyed before we could add it */
bool HBFifo::Push( HBBuffer * buffer )
{
    bool success = false;

    while( !fDie )
    {
        Lock();
        if( Size() < fCapacity )
        {
            fBuffers[fWhereToPush] = buffer;
            fWhereToPush++;
            fWhereToPush %= ( fCapacity + 1 );
            Unlock();
            success = true;
            break;
        }
        Unlock();
        snooze( 10000 );
    }
    
    if( !success )
    {
        delete buffer;
    }
    
    return success;
}

/* Pop() : get the first packet if the fifo. If the fifo is empty, it
   blocks until a packet comes. Returns true when it is successful,
   or false if the fifo has been destroyed before we could get a packet */
HBBuffer * HBFifo::Pop()
{
    while( !fDie )
    {
        Lock();    
        if( fWhereToPush != fWhereToPop )
        {
            HBBuffer * buffer = fBuffers[fWhereToPop];
            fWhereToPop++;
            fWhereToPop %= ( fCapacity + 1 );
            Unlock();
            return buffer;
        }
        Unlock();
        snooze( 10000 );
    }
    
    return NULL;
}

/* Lock() : private function */
void HBFifo::Lock()
{
    fLocker->Lock();
}

/* Unlock() : private function */
void HBFifo::Unlock()
{
    fLocker->Unlock();
}
