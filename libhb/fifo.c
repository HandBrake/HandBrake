/* $Id: fifo.c,v 1.17 2005/10/15 18:05:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#ifndef SYS_DARWIN
#include <malloc.h>
#endif

/* Fifo */
struct hb_fifo_s
{
    hb_lock_t    * lock;
    uint32_t       capacity;
    uint32_t       size;
    uint32_t       buffer_size;
    hb_buffer_t  * first;
    hb_buffer_t  * last;
};

/* we round the requested buffer size up to the next power of 2 so there can
 * be at most 32 possible pools when the size is a 32 bit int. To avoid a lot
 * of slow & error-prone run-time checking we allow for all 32. */
#define MAX_BUFFER_POOLS  32
/* the buffer pool only exists to avoid the two malloc and two free calls that
 * it would otherwise take to allocate & free a buffer. but we don't want to
 * tie up a lot of memory in the pool because this allocator isn't as general
 * as malloc so memory tied up here puts more pressure on the malloc pool.
 * A pool of 16 elements will avoid 94% of the malloc/free calls without wasting
 * too much memory. */
#define BUFFER_POOL_MAX_ELEMENTS 32

struct hb_buffer_pools_s
{
    int64_t allocated;
    hb_lock_t *lock;
    hb_fifo_t *pool[MAX_BUFFER_POOLS];
} buffers;


void hb_buffer_pool_init( void )
{
    buffers.lock = hb_lock_init();
    buffers.allocated = 0;

    /* we allocate pools for sizes 2^10 through 2^25. requests larger than
     * 2^25 will get passed through to malloc. */
    int i;
    for ( i = 10; i < 26; ++i )
    {
        buffers.pool[i] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS);
        buffers.pool[i]->buffer_size = 1 << i;
    }
    /* requests smaller than 2^10 are satisfied from the 2^10 pool. */
    for ( i = 1; i < 10; ++i )
    {
        buffers.pool[i] = buffers.pool[10];
    }
}

void hb_buffer_pool_free( void )
{
    int i;
    int count;
    int64_t freed = 0;
    hb_buffer_t *b;

    hb_lock(buffers.lock);

    for( i = 10; i < 26; ++i)
    {
        count = 0;
        while( ( b = hb_fifo_get(buffers.pool[i]) ) )
        {
            freed += b->alloc;
            if( b->data )
            {
                free( b->data );
            }
            free( b );
            count++;
        }
        if ( count )
        {
            hb_deep_log( 2, "Freed %d buffers of size %d", count,
                    buffers.pool[i]->buffer_size);
        }
    }

    hb_deep_log( 2, "Allocated %lld bytes of buffers on this pass and Freed %lld bytes, "
           "%lld bytes leaked", buffers.allocated, freed, buffers.allocated - freed);
    buffers.allocated = 0;
    hb_unlock(buffers.lock);
}

static hb_fifo_t *size_to_pool( int size )
{
    int i;
    for ( i = 0; i < 30; ++i )
    {
        if ( size <= (1 << i) )
        {
            return buffers.pool[i];
        }
    }
    return NULL;
}

hb_buffer_t * hb_buffer_init( int size )
{
    hb_buffer_t * b;
    hb_fifo_t *buffer_pool = size_to_pool( size );

    if( buffer_pool )
    {
        b = hb_fifo_get( buffer_pool );

        if( b )
        {
            /*
             * Zero the contents of the buffer, would be nice if we
             * didn't have to do this.
             */
            uint8_t *data = b->data;
            memset( b, 0, sizeof(hb_buffer_t) );
            b->alloc = buffer_pool->buffer_size;
            b->size = size;
            b->data = data;
            return( b );
        }
    }

    /*
     * No existing buffers, create a new one
     */
    if( !( b = calloc( sizeof( hb_buffer_t ), 1 ) ) )
    {
        hb_log( "out of memory" );
        return NULL;
    }

    b->size  = size;
    b->alloc  = buffer_pool? buffer_pool->buffer_size : size;

    if (size)
    {
#if defined( SYS_DARWIN ) || defined( SYS_FREEBSD ) || defined( SYS_MINGW )
        b->data  = malloc( b->alloc );
#elif defined( SYS_CYGWIN )
        /* FIXME */
        b->data  = malloc( b->alloc + 17 );
#else
        b->data  = memalign( 16, b->alloc );
#endif
        if( !b->data )
        {
            hb_log( "out of memory" );
            free( b );
            return NULL;
        }
        hb_lock(buffers.lock);
        buffers.allocated += b->alloc;
        hb_unlock(buffers.lock);
    }
    return b;
}

void hb_buffer_realloc( hb_buffer_t * b, int size )
{
    if ( size > b->alloc )
    {
        uint32_t orig = b->alloc;
        size = size_to_pool( size )->buffer_size;
        b->data  = realloc( b->data, size );
        b->alloc = size;

        hb_lock(buffers.lock);
        buffers.allocated += size - orig;
        hb_unlock(buffers.lock);
    }
}

void hb_buffer_close( hb_buffer_t ** _b )
{
    hb_buffer_t * b = *_b;
    hb_fifo_t *buffer_pool = size_to_pool( b->alloc );

    if( buffer_pool && b->data && !hb_fifo_is_full( buffer_pool ) )
    {
        hb_fifo_push_head( buffer_pool, b );
        *_b = NULL;
        return;
    }
    /* either the pool is full or this size doesn't use a pool - free the buf */
    if( b->data )
    {
        free( b->data );
        hb_lock(buffers.lock);
        buffers.allocated -= b->alloc;
        hb_unlock(buffers.lock);
    }
    free( b );
    *_b = NULL;
}

void hb_buffer_copy_settings( hb_buffer_t * dst, const hb_buffer_t * src )
{
    dst->start     = src->start;
    dst->stop      = src->stop;
    dst->new_chap  = src->new_chap;
    dst->frametype = src->frametype;
    dst->flags     = src->flags;
}

hb_fifo_t * hb_fifo_init( int capacity )
{
    hb_fifo_t * f;
    f           = calloc( sizeof( hb_fifo_t ), 1 );
    f->lock     = hb_lock_init();
    f->capacity = capacity;
    f->buffer_size = 0;
    return f;
}

int hb_fifo_size( hb_fifo_t * f )
{
    int ret;

    hb_lock( f->lock );
    ret = f->size;
    hb_unlock( f->lock );

    return ret;
}

int hb_fifo_is_full( hb_fifo_t * f )
{
    int ret;

    hb_lock( f->lock );
    ret = ( f->size >= f->capacity );
    hb_unlock( f->lock );

    return ret;
}

float hb_fifo_percent_full( hb_fifo_t * f )
{
    float ret;

    hb_lock( f->lock );
    ret = f->size / f->capacity;
    hb_unlock( f->lock );

    return ret;
}

hb_buffer_t * hb_fifo_get( hb_fifo_t * f )
{
    hb_buffer_t * b;

    hb_lock( f->lock );
    if( f->size < 1 )
    {
        hb_unlock( f->lock );
        return NULL;
    }
    b         = f->first;
    f->first  = b->next;
    b->next   = NULL;
    f->size  -= 1;
    hb_unlock( f->lock );

    return b;
}

hb_buffer_t * hb_fifo_see( hb_fifo_t * f )
{
    hb_buffer_t * b;

    hb_lock( f->lock );
    if( f->size < 1 )
    {
        hb_unlock( f->lock );
        return NULL;
    }
    b = f->first;
    hb_unlock( f->lock );

    return b;
}

hb_buffer_t * hb_fifo_see2( hb_fifo_t * f )
{
    hb_buffer_t * b;

    hb_lock( f->lock );
    if( f->size < 2 )
    {
        hb_unlock( f->lock );
        return NULL;
    }
    b = f->first->next;
    hb_unlock( f->lock );

    return b;
}

void hb_fifo_push( hb_fifo_t * f, hb_buffer_t * b )
{
    if( !b )
    {
        return;
    }

    hb_lock( f->lock );
    if( f->size > 0 )
    {
        f->last->next = b;
    }
    else
    {
        f->first = b;
    }
    f->last  = b;
    f->size += 1;
    while( f->last->next )
    {
        f->size += 1;
        f->last  = f->last->next;
    }
    hb_unlock( f->lock );
}

void hb_fifo_push_head( hb_fifo_t * f, hb_buffer_t * b )
{
    hb_buffer_t * tmp;
    uint32_t      size = 0;

    if( !b )
    {
        return;
    }

    hb_lock( f->lock );

    /*
     * If there are a chain of buffers prepend the lot
     */
    tmp = b;
    while( tmp->next )
    {
        tmp = tmp->next;
        size += 1;
    }

    if( f->size > 0 )
    {
        tmp->next = f->first;
    } 
    else
    {
        f->last = tmp;
    }

    f->first = b;
    f->size += ( size + 1 );

    hb_unlock( f->lock );
}

void hb_fifo_close( hb_fifo_t ** _f )
{
    hb_fifo_t   * f = *_f;
    hb_buffer_t * b;

    hb_deep_log( 2, "fifo_close: trashing %d buffer(s)", hb_fifo_size( f ) );
    while( ( b = hb_fifo_get( f ) ) )
    {
        hb_buffer_close( &b );
    }

    hb_lock_close( &f->lock );
    free( f );

    *_f = NULL;
}
