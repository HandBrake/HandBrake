/* $Id: Fifo.c,v 1.17 2004/04/27 19:30:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#if defined( HB_BEOS ) || defined( HB_LINUX )
#include <malloc.h>
#endif

HBBuffer * HBBufferInit( int size )
{
    HBBuffer * b;
    if( !( b = calloc( sizeof( HBBuffer ), 1 ) ) )
    {
        HBLog( "HBBuffer: malloc() failed, gonna crash" );
        return NULL;
    }

    b->alloc = size;
    b->size  = size;

#if defined( HB_BEOS ) || defined( HB_LINUX )
    b->data = memalign( 16, size );
#elif defined( HB_MACOSX )
    /* OS X's malloc returns 16-bytes aligned memory */
    b->data = malloc( size );
#elif defined( HB_CYGWIN )
    b->dataOrig  = malloc( size + 15 );
    b->data      = b->dataOrig + 15;
    b->data     -= (long) b->data & 15;
#endif
    b->dataf     = (float*) b->data;

    if( !b->data )
    {
        HBLog( "HBBuffer: malloc() failed, gonna crash" );
        free( b );
        return NULL;
    }

    return b;
}

void HBBufferReAlloc( HBBuffer * b, int size )
{
    /* We don't care about alignment here, realloc is only used in the
       AVI muxer anyway */
#if defined( HB_BEOS ) || defined( HB_LINUX ) || defined( HB_MACOSX )
    b->data = realloc( b->data, size );
#elif defined( HB_CYGWIN )
    int alignment = b->data - b->dataOrig;
    b->dataOrig   = realloc( b->dataOrig, size + alignment );
    b->data       = b->dataOrig + alignment;
#endif
    b->alloc = size;

    if( !b->data )
    {
        HBLog( "HBBuffer: realloc() failed, gonna crash soon" );
    }
}

void HBBufferClose( HBBuffer ** _b )
{
    HBBuffer * b = *_b;

#if defined( HB_BEOS ) || defined( HB_LINUX ) || defined( HB_MACOSX )
    free( b->data );
#elif defined( HB_CYGWIN )
    free( b->dataOrig );
#endif
    free( b );

    *_b = NULL;
}

HBFifo * HBFifoInit( int capacity )
{
    HBFifo * f;
    if( !( f = malloc( sizeof( HBFifo ) ) ) )
    {
        HBLog( "HBFifo: malloc() failed, gonna crash" );
        return NULL;
    }

    f->die         = 0;
    f->capacity    = capacity;
    f->whereToPush = 0;
    f->whereToPop  = 0;

    if( !( f->buffers = malloc( ( capacity + 1 ) * sizeof( void* ) ) ) )
    {
        HBLog( "HBFifo: malloc() failed, gonna crash" );
        free( f );
        return NULL;
    }

    f->lock = HBLockInit();
    f->cond = HBCondInit();

    return f;
}

void HBFifoDie( HBFifo * f )
{
    HBLockLock( f->lock );
    f->die = 1;
    HBCondSignal( f->cond );
    HBLockUnlock( f->lock );
}

void HBFifoClose( HBFifo ** _f )
{
    HBFifo * f = (*_f);

    HBLog( "HBFifo: trashing %d buffer%s",
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

