/* $Id: Fifo.c,v 1.8 2004/01/16 19:04:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"

HBBuffer * HBBufferInit( int size )
{
    HBBuffer * b;
    if( !( b = calloc( sizeof( HBBuffer ), 1 ) ) )
    {
        HBLog( "HBBufferInit: malloc() failed, gonna crash" );
        return NULL;
    }

    b->alloc = size;
    b->size  = size;

    if( !( b->data = malloc( size ) ) )
    {
        HBLog( "HBBufferInit: malloc() failed, gonna crash" );
        free( b );
        return NULL;
    }

    b->position = 0.0;

    return b;
}

void HBBufferReAlloc( HBBuffer * b, int size )
{
    b->alloc = size;
    b->data  = realloc( b->data, size );

    if( !b->data )
    {
        HBLog( "HBBufferReAlloc: realloc() failed, gonna crash soon" );
    }
}

void HBBufferClose( HBBuffer ** _b )
{
    HBBuffer * b = *_b;

    free( b->data );
    free( b );

    *_b = NULL;
}

HBFifo * HBFifoInit( int capacity )
{
    HBFifo * f;
    if( !( f = malloc( sizeof( HBFifo ) ) ) )
    {
        HBLog( "HBFifoInit: malloc() failed, gonna crash" );
        return NULL;
    }

    f->die         = 0;
    f->capacity    = capacity;
    f->whereToPush = 0;
    f->whereToPop  = 0;

    if( !( f->buffers = malloc( ( capacity + 1 ) * sizeof( void* ) ) ) )
    {
        HBLog( "HBFifoInit: malloc() failed, gonna crash" );
        free( f );
        return NULL;
    }

    f->lock = HBLockInit();
    f->cond = HBCondInit();

    return f;
}

void HBFifoDie( HBFifo * f )
{
    f->die = 1;
    HBCondSignal( f->cond );
}

void HBFifoClose( HBFifo ** _f )
{
    HBFifo * f = (*_f);

    HBLog( "HBFifoClose: trashing %d buffer%s",
           HBFifoSize( f ), ( HBFifoSize( f ) > 1 ) ? "s" : "" );

    while( f->whereToPush != f->whereToPop )
    {
        HBBufferClose( &(f->buffers[f->whereToPop]) );
        f->whereToPop++;
        f->whereToPop %= ( f->capacity + 1 );
    }

    HBLockClose( &f->lock );
    HBCondClose( &f->cond );
    free( f->buffers );
    free( f );

    *_f = NULL;
}

