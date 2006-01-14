/* $Id: Fifo.c,v 1.9 2004/02/24 21:55:53 titer Exp $

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

#if defined( HB_BEOS ) || defined( HB_LINUX )
    if( !( b->data = memalign( 16, size ) ) )
    {
        HBLog( "HBBufferInit: malloc() failed, gonna crash" );
        free( b );
        return NULL;
    }
#elif defined( HB_MACOSX )
    if( !( b->dataOrig = malloc( size + 15 ) ) )
    {
        HBLog( "HBBufferInit: malloc() failed, gonna crash" );
        free( b );
        return NULL;
    }
    b->data  = b->dataOrig + 15;
    b->data -= (long) b->data & 15;
#elif defined( HB_CYGWIN )
    /* TODO */
#endif

    b->position = 0.0;

    return b;
}

void HBBufferReAlloc( HBBuffer * b, int size )
{
    b->alloc = size;
#if defined( HB_BEOS ) || defined( HB_LINUX )
    b->data  = realloc( b->data, size );
#elif defined( HB_MACOSX )
    b->dataOrig = realloc( b->dataOrig, size );
    b->data     = b->dataOrig;
#elif defined( HB_CYGWIN )
    /* TODO */
#endif

    if( !b->data )
    {
        HBLog( "HBBufferReAlloc: realloc() failed, gonna crash soon" );
    }
}

void HBBufferClose( HBBuffer ** _b )
{
    HBBuffer * b = *_b;

#if defined( HB_BEOS ) || defined( HB_LINUX )
    free( b->data );
#elif defined( HB_MACOSX )
    free( b->dataOrig );
#elif defined( HB_CYGWIN )
    /* TODO */
#endif
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

