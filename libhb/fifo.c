/* $Id: fifo.c,v 1.17 2005/10/15 18:05:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#ifndef SYS_DARWIN
#include <malloc.h>
#endif

hb_buffer_t * hb_buffer_init( int size )
{
    hb_buffer_t * b;
    
    if( !( b = calloc( sizeof( hb_buffer_t ), 1 ) ) )
    {
        hb_log( "out of memory" );
        return NULL;
    }

    b->alloc = size;
    b->size  = size;
#if defined( SYS_DARWIN ) || defined( SYS_FREEBSD )
    b->data  = malloc( size );
#elif defined( SYS_CYGWIN )
    /* FIXME */
    b->data  = malloc( size + 17 );
#else
    b->data  = memalign( 16, size );
#endif

    if( !b->data )
    {
        hb_log( "out of memory" );
        free( b );
        return NULL;
    }
    return b;
}

void hb_buffer_realloc( hb_buffer_t * b, int size )
{
    /* No more alignment, but we don't care */
    b->data  = realloc( b->data, size );
    b->alloc = size;
}

void hb_buffer_close( hb_buffer_t ** _b )
{
    hb_buffer_t * b = *_b;

    if( b->data )
    {
        free( b->data );
    }
    free( b );

    *_b = NULL;
}

/* Fifo */
struct hb_fifo_s
{
    hb_lock_t    * lock;
    int            capacity;
    int            size;
    hb_buffer_t  * first;
    hb_buffer_t  * last;
};

hb_fifo_t * hb_fifo_init( int capacity )
{
    hb_fifo_t * f;
    f           = calloc( sizeof( hb_fifo_t ), 1 );
    f->lock     = hb_lock_init();
    f->capacity = capacity;
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
