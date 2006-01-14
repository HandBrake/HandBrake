/* $Id: Fifo.h,v 1.3 2003/11/06 13:07:52 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_FIFO_H
#define HB_FIFO_H

#include "Utils.h"
#include "Thread.h"

struct HBBuffer
{
    int       alloc;
    int       size;
    uint8_t * data;

    float     position;
    int       streamId;
    int       keyFrame;
    uint64_t  pts;
    int       pass;
    int       last;
};

HBBuffer * HBBufferInit( int size );
void       HBBufferReAlloc( HBBuffer *, int size );
void       HBBufferClose( HBBuffer ** );

struct HBFifo
{
    int         capacity;
    int         whereToPush;
    int         whereToPop;
    HBBuffer ** buffers;
    HBLock    * lock;
};

HBFifo                 * HBFifoInit( int capacity );
int                      HBFifoSize( HBFifo * );
static inline int        HBFifoPush( HBFifo *, HBBuffer ** );
static inline HBBuffer * HBFifoPop( HBFifo * );
void                     HBFifoClose( HBFifo ** );

static inline int HBFifoPush( HBFifo * f, HBBuffer ** b )
{
    HBLockLock( f->lock );

    if( HBFifoSize( f ) < f->capacity )
    {
        f->buffers[f->whereToPush] = *b;
        f->whereToPush++;
        f->whereToPush %= ( f->capacity + 1 );
        HBLockUnlock( f->lock );
        *b = NULL;
        return 1;
    }

    HBLockUnlock( f->lock );
    return 0;
}

static inline HBBuffer * HBFifoPop( HBFifo * f )
{
    HBLockLock( f->lock );

    if( f->whereToPush != f->whereToPop )
    {
        HBBuffer * b = f->buffers[f->whereToPop];
        f->whereToPop++;
        f->whereToPop %= ( f->capacity + 1 );
        HBLockUnlock( f->lock );
        return b;
    }

    HBLockUnlock( f->lock );
    return NULL;
}

#endif
