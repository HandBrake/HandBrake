/* $Id: Fifo.h,v 1.16 2004/04/27 22:02:59 titer Exp $

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
    float   * dataf;
    float     position;
    int       pass;

    /* Only used for PStoES */
    uint32_t  streamId;
    uint64_t  pts;

    /* NTSC suxx */
    int       repeat;

    /* Only used for MPEG-4, MP3 and AAC buffers */
    int       keyFrame;

    /* Use for bitstreams */
    int       _pos;
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
static inline int        HBFifoIsHalfFull( HBFifo * );
static inline int        HBFifoPush( HBFifo *, HBBuffer ** );
static inline HBBuffer * HBFifoPop( HBFifo * );
static inline int        HBFifoWait( HBFifo * );
static inline float      HBFifoPosition( HBFifo * );
static inline int        HBFifoGetBytes( HBFifo * f, uint8_t *, int,
                                         float * position );
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

static inline int HBFifoIsHalfFull( HBFifo * f )
{
    int size;
    HBLockLock( f->lock );
    size = ( f->capacity + 1 + f->whereToPush - f->whereToPop ) %
                 ( f->capacity + 1 );
    HBLockUnlock( f->lock );
    return ( 2 * size > f->capacity );
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

static inline int HBFifoGetBytes( HBFifo * f, uint8_t * data, int nb,
                                  float * position )
{
    int whereToPop, bytes = 0;
    HBBuffer * buffer;
    HBLockLock( f->lock );

    /* Do we have enough? */
    for( whereToPop = f->whereToPop; ; whereToPop++ )
    {
        whereToPop %= ( f->capacity + 1 );
        if( f->whereToPush == whereToPop )
        {
            /* We hit the end of the fifo */
            break;
        }

        bytes += f->buffers[whereToPop]->size -
                    f->buffers[whereToPop]->_pos;

        if( bytes >= nb )
        {
            break;
        }
    }

    if( bytes < nb )
    {
        /* Not enough data */
        HBLockUnlock( f->lock );
        return 0;
    }

    for( bytes = 0; bytes < nb; )
    {
        int copy;

        buffer = f->buffers[f->whereToPop];
        copy   = MIN( nb - bytes, buffer->size - buffer->_pos );

        memcpy( data + bytes, buffer->data + buffer->_pos, copy );
        (*position) = buffer->position;

        buffer->_pos += copy;
        bytes        += copy;

        if( buffer->_pos == buffer->size )
        {
            HBBufferClose( &buffer );
            f->whereToPop++;
            f->whereToPop %= ( f->capacity + 1 );
        }
    }

    HBLockUnlock( f->lock );
    return 1;
}

#endif
