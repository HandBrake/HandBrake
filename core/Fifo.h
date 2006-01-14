/* $Id: Fifo.h,v 1.13 2004/03/17 10:35:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_FIFO_H
#define HB_FIFO_H

#include "Utils.h"
#include "Thread.h"

struct HBBuffer
{
    /* Members used everywhere */
    int       alloc;
    int       size;
    uint8_t * data;
#if defined( HB_CYGWIN )
    uint8_t * dataOrig;
#endif
    float     position;
    int       pass;

    /* Only used for PStoES */
    int       streamId;
    uint64_t  pts;

    /* NTSC suxx */
    int       repeat;

    /* Only used for raw audio buffers */
    int       samples; /* Number of samples for each track */
    float   * left;
    float   * right;

    /* Only used for MPEG-4, MP3 and AAC buffers */
    int       keyFrame;
};

HBBuffer * HBBufferInit( int size );
void       HBBufferReAlloc( HBBuffer *, int size );
void       HBBufferClose( HBBuffer ** );

struct HBFifo
{
    int         die;
    int         capacity;
    int         whereToPush;
    int         whereToPop;
    HBBuffer ** buffers;
    HBLock    * lock;
    HBCond    * cond;
};

HBFifo                 * HBFifoInit( int capacity );
static inline int        HBFifoSize( HBFifo * );
static inline int        HBFifoPush( HBFifo *, HBBuffer ** );
static inline HBBuffer * HBFifoPop( HBFifo * );
static inline int        HBFifoWait( HBFifo * );
static inline float      HBFifoPosition( HBFifo * );
void                     HBFifoDie( HBFifo * );
void                     HBFifoClose( HBFifo ** );

static inline int HBFifoSize( HBFifo * f )
{
    int size;
    HBLockLock( f->lock );
    size = ( f->capacity + 1 + f->whereToPush - f->whereToPop ) %
                 ( f->capacity + 1 );
    HBLockUnlock( f->lock );
    return size;
}

static inline int HBFifoPush( HBFifo * f, HBBuffer ** b )
{
    HBLockLock( f->lock );
    HBCondSignal( f->cond );
    if( ( f->capacity + 1 + f->whereToPush - f->whereToPop ) %
            ( f->capacity + 1 ) != f->capacity )
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

static inline int HBFifoWait( HBFifo * f )
{
    HBLockLock( f->lock );
    if( f->whereToPush != f->whereToPop )
    {
        HBLockUnlock( f->lock );
        return 1;
    }
    if( f->die )
    {
        HBLockUnlock( f->lock );
        return 0;
    }
    HBCondWait( f->cond, f->lock );
    if( f->whereToPush != f->whereToPop )
    {
        HBLockUnlock( f->lock );
        return 1;
    }
    HBLockUnlock( f->lock );
    return 0;
}

static inline float HBFifoPosition( HBFifo * f )
{
    float pos;
    HBLockLock( f->lock );
    if( f->whereToPush != f->whereToPop )
    {
        pos = f->buffers[f->whereToPop]->position;
    }
    else
    {
        pos = 0.0;
    }
    HBLockUnlock( f->lock );
    return pos;
}

#endif
