/* $Id: fifo.c,v 1.17 2005/10/15 18:05:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#ifndef SYS_DARWIN
#include <malloc.h>
#endif

/* Fifo */
struct hb_fifo_s
{
    hb_lock_t    * lock;
    int            capacity;
    int            size;
    int            buffer_size;
    hb_buffer_t  * first;
    hb_buffer_t  * last;
};

#define MAX_BUFFER_POOLS  15
#define BUFFER_POOL_MAX_ELEMENTS 2048

struct hb_buffer_pools_s
{
    int entries;
    int allocated;
    hb_fifo_t *pool[MAX_BUFFER_POOLS];
    hb_lock_t *lock;
};

struct hb_buffer_pools_s buffers;

void hb_buffer_pool_init( void )
{
    hb_fifo_t *buffer_pool;
    int size = 512;
    int max_size = 32768;;

    buffers.entries = 0;
    buffers.lock = hb_lock_init();
    buffers.allocated = 0;
    
    while(size <= max_size) {
        buffer_pool = buffers.pool[buffers.entries++] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS);
        buffer_pool->buffer_size = size;
        size *= 2;
    }
}

void hb_buffer_pool_free( void )
{
    int i;
    int count;
    int freed = 0;
    hb_buffer_t *b;

    hb_lock(buffers.lock);

    for( i = 0; i < buffers.entries; i++) 
    {
        count = 0;
        while( ( b = hb_fifo_get(buffers.pool[i]) ) )
        {
            freed += b->alloc;
            if( b->data )
            {
                free( b->data );
                b->data = NULL;
            }
            free( b );
            count++;
        }
        hb_log("Freed %d buffers of size %d", count, buffers.pool[i]->buffer_size);
    }

    hb_log("Allocated %d bytes of buffers on this pass and Freed %d bytes, %d bytes leaked",
           buffers.allocated, freed, buffers.allocated - freed);
    buffers.allocated = 0;
    hb_unlock(buffers.lock);
}


hb_buffer_t * hb_buffer_init( int size )
{ 
    hb_buffer_t * b;
    int i;
    hb_fifo_t *buffer_pool = NULL;
    uint8_t *data;
    int b_alloc;
    int resize = 0;

    /*
     * The buffer pools are allocated in increasing size
     */
    for( i = 0; i < buffers.entries; i++ )
    {
        if( buffers.pool[i]->buffer_size >= size )
        {
            /*
             * This pool is big enough, but are there any buffers in it?
             */
            if( hb_fifo_size( buffers.pool[i] ) ) 
            {
                /*
                 * We've found a matching buffer pool, with buffers.
                 */
                buffer_pool = buffers.pool[i];
                resize =  buffers.pool[i]->buffer_size;
            } else {
                /*
                 * Buffer pool is empty, 
                 */
                if( resize ) {
                    /*
                     * This is the second time through, so break out of here to avoid
                     * using too large a buffer for a small job.
                     */
                    break;
                }
                resize =  buffers.pool[i]->buffer_size;
            }
        }
    }

    /*
     * Don't reuse the 0 size buffers, not much gain.
     */
    if( size != 0 && buffer_pool )
    {
        b = hb_fifo_get( buffer_pool );    

        if( b )
        {
            /*
             * Zero the contents of the buffer, would be nice if we
             * didn't have to do this.
             *
            hb_log("Reused buffer size %d for size %d from pool %d depth %d", 
                   b->alloc, size, smallest_pool->buffer_size, 
                   hb_fifo_size(smallest_pool));
            */
            data = b->data;
            b_alloc = b->alloc;
            memset( b, 0, sizeof(hb_buffer_t) );
            b->alloc = b_alloc;
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

    if( resize )
    {
        size = resize;
    } 
    b->alloc  = size;  

    /*
    hb_log("Allocating new buffer of size %d for size %d", 
           b->alloc, 
           b->size);
    */

    if (!size)
        return b;
#if defined( SYS_DARWIN ) || defined( SYS_FREEBSD )
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

    buffers.allocated += b->alloc;

    return b;
}

void hb_buffer_realloc( hb_buffer_t * b, int size )
{
    /* No more alignment, but we don't care */
    if( size < 2048 ) {
        size = 2048;
    }
    b->data  = realloc( b->data, size );
    buffers.allocated -= b->alloc;
    b->alloc = size;
    buffers.allocated += b->alloc;
}

void hb_buffer_close( hb_buffer_t ** _b )
{
    hb_buffer_t * b = *_b;
    hb_fifo_t *buffer_pool = NULL;
    int i;

    /*
     * Put the buffer into our free list in the matching buffer pool, if there is one.
     */
    if( b->alloc != 0 )
    {
        for( i = 0; i < buffers.entries; i++ )
        {
            if( b->alloc == buffers.pool[i]->buffer_size )
            { 
                buffer_pool = buffers.pool[i];
                break;
            }
        }
    }

    if( buffer_pool ) 
    {
        if( !hb_fifo_is_full( buffer_pool ) ) 
        {
            if(b->data)
            {
                /*
                hb_log("Putting a buffer of size %d on pool %d, depth %d",
                       b->alloc, 
                       buffer_pool->buffer_size, 
                       hb_fifo_size(buffer_pool));
                */
                hb_fifo_push( buffer_pool, b );
            } else {
                free(b);
            }
        } else {
            /*
             * Got a load of these size ones already, free this buffer.
             *
            hb_log("Buffer pool for size %d full, freeing buffer", b->alloc);
            */
            if( b->data )
            {
                free( b->data );
            }
            buffers.allocated -= b->alloc;
            free( b );
        }
    } else {
        /*
         * Need a new buffer pool for this size.
         */
        hb_lock(buffers.lock);
        if ( b->alloc != 0 && buffers.entries < MAX_BUFFER_POOLS)
        {
            buffer_pool = buffers.pool[buffers.entries++] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS);
            buffer_pool->buffer_size = b->alloc;
            hb_fifo_push( buffer_pool, b );
            /*
            hb_log("*** Allocated a new buffer pool for size %d [%d]", b->alloc,
                   buffers.entries );
            */
        } else {
            if( b->alloc != 0 )
            {
                for( i = buffers.entries-1; i >= 0; i-- )
                {
                    if( hb_fifo_size(buffers.pool[i]) == 0 )
                    {
                        /*
                         * Reuse this pool as it is empty.
                         */
                        buffers.pool[i]->buffer_size = b->alloc;
                        hb_fifo_push( buffers.pool[i], b );
                        b = NULL;
                        break;
                    }
                }
            }

            if( b )
            {
                if( b->data )
                {
                    free( b->data );
                    b->data = NULL;
                    buffers.allocated -= b->alloc;
                }
                free( b );
            }
        }
        hb_unlock(buffers.lock);
    }

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

void hb_fifo_close( hb_fifo_t ** _f )
{
    hb_fifo_t   * f = *_f;
    hb_buffer_t * b;
    
    hb_log( "fifo_close: trashing %d buffer(s)", hb_fifo_size( f ) );
    while( ( b = hb_fifo_get( f ) ) )
    {
        hb_buffer_close( &b );
    }

    hb_lock_close( &f->lock );
    free( f );

    *_f = NULL;
}
