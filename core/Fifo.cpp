/* $Id: Fifo.cpp,v 1.15 2003/10/13 14:12:18 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#include "Thread.h"

HBBuffer::HBBuffer( uint32_t size )
{
    fAllocSize = size;
    fSize = size;
    fKeyFrame = false;
    fData = (uint8_t*) malloc( size );
    fPosition = 0;

    if( !fData )
    {
        Log( "HBBuffer::HBBuffer() : malloc() failed, gonna crash soon" );
    }
}

HBBuffer::~HBBuffer()
{
    free( fData );
}

void HBBuffer::ReAlloc( uint32_t size )
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
    fLock        = new HBLock();
}

HBFifo::~HBFifo()
{
    Log( "HBFifo::~HBFifo: trashing %d buffer%s",
         Size(), Size() ? "s" : "" );
    
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

    delete fLock;
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

/* Push() - returns immediatly (true if successful, false otherwise ) */
bool HBFifo::Push( HBBuffer * buffer )
{
    fLock->Lock();

    if( Size() < fCapacity )
    {
        fBuffers[fWhereToPush] = buffer;
        fWhereToPush++;
        fWhereToPush %= ( fCapacity + 1 );
        fLock->Unlock();
        return true;
    }

    fLock->Unlock();
    return false;
}

/* Pop() - returns immediatly (a pointer to a buffer if successful,
   NULL otherwise ) */
HBBuffer * HBFifo::Pop()
{
    fLock->Lock();

    if( fWhereToPush != fWhereToPop )
    {
        HBBuffer * buffer = fBuffers[fWhereToPop];
        fWhereToPop++;
        fWhereToPop %= ( fCapacity + 1 );
        fLock->Unlock();
        return buffer;
    }

    fLock->Unlock();
    return NULL;
}
