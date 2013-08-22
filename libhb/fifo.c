/* fifo.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"

#ifndef SYS_DARWIN
#include <malloc.h>
#endif

#define FIFO_TIMEOUT 200
//#define HB_FIFO_DEBUG 1

/* Fifo */
struct hb_fifo_s
{
    hb_lock_t    * lock;
    hb_cond_t    * cond_full;
    int            wait_full;
    hb_cond_t    * cond_empty;
    int            wait_empty;
    uint32_t       capacity;
    uint32_t       thresh;
    uint32_t       size;
    uint32_t       buffer_size;
    hb_buffer_t  * first;
    hb_buffer_t  * last;

#if defined(HB_FIFO_DEBUG)
    // Fifo list for debugging
    hb_fifo_t    * next;
#endif
};

#if defined(HB_FIFO_DEBUG)
static hb_fifo_t fifo_list = 
{
    .next = NULL
};
#endif

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
        buffers.pool[i] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS, 1);
        buffers.pool[i]->buffer_size = 1 << i;
    }
    /* requests smaller than 2^10 are satisfied from the 2^10 pool. */
    for ( i = 1; i < 10; ++i )
    {
        buffers.pool[i] = buffers.pool[10];
    }
}

#if defined(HB_FIFO_DEBUG)

static void dump_fifo(hb_fifo_t * f)
{
    hb_buffer_t * b = f->first;

    if (b)
    {
        while (b)
        {
            fprintf(stderr, "%p:%d:%d\n", b, b->size, b->alloc);
            b = b->next;
        }
        fprintf(stderr, "\n");
    }
}

static void fifo_list_add( hb_fifo_t * f )
{
    hb_fifo_t *next = fifo_list.next;

    fifo_list.next = f;
    f->next = next;
}

static void fifo_list_rem( hb_fifo_t * f )
{
    hb_fifo_t *next, *prev;

    prev = &fifo_list;
    next = fifo_list.next;

    while ( next && next != f )
    {
        prev = next;
        next = next->next;
    }
    if ( next == f )
    {
        prev->next = f->next;
    }
}

// These routines are useful for finding and debugging problems
// with the fifos and buffer pools
static void buffer_pool_validate( hb_fifo_t * f )
{
    hb_buffer_t *b;

    hb_lock( f->lock );
    b = f->first;
    while (b)
    {
        if (b->alloc != f->buffer_size)
        {
            fprintf(stderr, "Invalid buffer pool size! buf %p size %d pool size %d\n", b, b->alloc, f->buffer_size);
            dump_fifo( f );
            *(char*)0 = 1;
        }
        b = b->next;
    }

    hb_unlock( f->lock );
}

static void buffer_pools_validate( void )
{
    int ii;
    for ( ii = 10; ii < 26; ++ii )
    {
        buffer_pool_validate( buffers.pool[ii] );
    }
}

void fifo_list_validate( void )
{
    hb_fifo_t *next = fifo_list.next;
    hb_fifo_t *m;
    hb_buffer_t *b, *c;
    int count;

    buffer_pools_validate();
    while ( next )
    {
        count = 0;
        hb_lock( next->lock );
        b = next->first;

        // Count the number of entries in this fifo
        while (b)
        {
            c = b->next;
            // check that the current buffer is not duplicated in this fifo
            while (c)
            {
                if (c == b)
                {
                    fprintf(stderr, "Duplicate buffer in fifo!\n");
                    dump_fifo(next);
                    *(char*)0 = 1;
                }
                c = c->next;
            }

            // check that the current buffer is not duplicated in another fifo
            m = next->next;
            while (m)
            {
                hb_lock( m->lock );
                c = m->first;
                while (c)
                {
                    if (c == b)
                    {
                        fprintf(stderr, "Duplicate buffer in another fifo!\n");
                        dump_fifo(next);
                        *(char*)0 = 1;
                    }
                    c = c->next;
                }
                hb_unlock( m->lock );
                m = m->next;
            }

            count++;
            b = b->next;
        }

        if ( count != next->size )
        {
            fprintf(stderr, "Invalid fifo size! count %d size %d\n", count, next->size);
            dump_fifo(next);
            *(char*)0 = 1;
        }
        hb_unlock( next->lock );

        next = next->next;
    }
}
#endif

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
            if( b->data )
            {
                freed += b->alloc;
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

    hb_deep_log( 2, "Allocated %"PRId64" bytes of buffers on this pass and Freed %"PRId64" bytes, "
           "%"PRId64" bytes leaked", buffers.allocated, freed, buffers.allocated - freed);
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
    // Certain libraries (hrm ffmpeg) expect buffers passed to them to
    // end on certain alignments (ffmpeg is 8). So allocate some extra bytes.
    // Note that we can't simply align the end of our buffer because
    // sometimes we feed data to these libraries starting from arbitrary
    // points within the buffer.
    int alloc = size + 16;
    hb_fifo_t *buffer_pool = size_to_pool( alloc );

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
            b->s.start = -1;
            b->s.stop = -1;
            b->s.renderOffset = -1;
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
    b->alloc  = buffer_pool ? buffer_pool->buffer_size : alloc;

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
    b->s.start = -1;
    b->s.stop = -1;
    b->s.renderOffset = -1;
    return b;
}

void hb_buffer_realloc( hb_buffer_t * b, int size )
{
    if ( size > b->alloc || b->data == NULL )
    {
        uint32_t orig = b->data != NULL ? b->alloc : 0;
        size = size_to_pool( size )->buffer_size;
        b->data  = realloc( b->data, size );
        b->alloc = size;

        hb_lock(buffers.lock);
        buffers.allocated += size - orig;
        hb_unlock(buffers.lock);
    }
}

void hb_buffer_reduce( hb_buffer_t * b, int size )
{
    if ( size < b->alloc / 8 || b->data == NULL )
    {
        hb_buffer_t * tmp = hb_buffer_init( size );

        hb_buffer_swap_copy( b, tmp );
        memcpy( b->data, tmp->data, size );
        tmp->next = NULL;
        hb_buffer_close( &tmp );
    }
}

hb_buffer_t * hb_buffer_dup( const hb_buffer_t * src )
{
    hb_buffer_t * buf;

    if ( src == NULL )
        return NULL;

    buf = hb_buffer_init( src->size );
    if ( buf )
    {
        memcpy( buf->data, src->data, src->size );
        buf->s = src->s;
        buf->f = src->f;
        if ( buf->s.type == FRAME_BUF )
            hb_buffer_init_planes( buf );
    }

#ifdef USE_QSV
    memcpy(&buf->qsv_details, &src->qsv_details, sizeof(src->qsv_details));
#endif

    return buf;
}

int hb_buffer_copy(hb_buffer_t * dst, const hb_buffer_t * src)
{
    if (src == NULL || dst == NULL)
        return -1;

    if ( dst->size < src->size )
        return -1;

    memcpy( dst->data, src->data, src->size );
    dst->s = src->s;
    dst->f = src->f;
    if (dst->s.type == FRAME_BUF)
        hb_buffer_init_planes(dst);

    return 0;
}

static void hb_buffer_init_planes_internal( hb_buffer_t * b, uint8_t * has_plane )
{
    uint8_t * plane = b->data;
    int p;

    for( p = 0; p < 4; p++ )
    {
        if ( has_plane[p] )
        {
            b->plane[p].data = plane;
            b->plane[p].stride = hb_image_stride( b->f.fmt, b->f.width, p );
            b->plane[p].height_stride = hb_image_height_stride( b->f.fmt, b->f.height, p );
            b->plane[p].width  = hb_image_width( b->f.fmt, b->f.width, p );
            b->plane[p].height = hb_image_height( b->f.fmt, b->f.height, p );
            b->plane[p].size   = b->plane[p].stride * b->plane[p].height_stride;
            plane += b->plane[p].size;
        }
    }
}

void hb_buffer_init_planes( hb_buffer_t * b )
{
    const AVPixFmtDescriptor *desc = &av_pix_fmt_descriptors[b->f.fmt];
    int p;

    uint8_t has_plane[4] = {0,};

    for( p = 0; p < 4; p++ )
    {
        has_plane[desc->comp[p].plane] = 1;
    }
    hb_buffer_init_planes_internal( b, has_plane );
}

// this routine gets a buffer for an uncompressed picture
// with pixel format pix_fmt and dimensions width x height.
hb_buffer_t * hb_frame_buffer_init( int pix_fmt, int width, int height )
{
    const AVPixFmtDescriptor *desc = &av_pix_fmt_descriptors[pix_fmt];
    hb_buffer_t * buf;
    int p;
    uint8_t has_plane[4] = {0,};

    for( p = 0; p < 4; p++ )
    {
        has_plane[desc->comp[p].plane] = 1;
    }

    int size = 0;
    for( p = 0; p < 4; p++ )
    {
        if ( has_plane[p] )
        {
            size += hb_image_stride( pix_fmt, width, p ) * 
                    hb_image_height_stride( pix_fmt, height, p );
        }
    }

    buf = hb_buffer_init( size );
    if( buf == NULL )
        return NULL;

    buf->s.type = FRAME_BUF;
    buf->f.width = width;
    buf->f.height = height;
    buf->f.fmt = pix_fmt;

    hb_buffer_init_planes_internal( buf, has_plane );
    return buf;
}

// this routine reallocs a buffer for an uncompressed YUV420 video frame
// with dimensions width x height.
void hb_video_buffer_realloc( hb_buffer_t * buf, int width, int height )
{
    const AVPixFmtDescriptor *desc = &av_pix_fmt_descriptors[buf->f.fmt];
    int p;
    uint8_t has_plane[4] = {0,};

    for( p = 0; p < 4; p++ )
    {
        has_plane[desc->comp[p].plane] = 1;
    }

    int size = 0;
    for( p = 0; p < 4; p++ )
    {
        if ( has_plane[p] )
        {
            size += hb_image_stride( buf->f.fmt, width, p ) * 
                    hb_image_height_stride( buf->f.fmt, height, p );
        }
    }

    hb_buffer_realloc(buf, size );

    buf->f.width = width;
    buf->f.height = height;
    buf->size = size;

    hb_buffer_init_planes_internal( buf, has_plane );
}

// this routine 'moves' data from src to dst by interchanging 'data',
// 'size' & 'alloc' between them and copying the rest of the fields
// from src to dst.
void hb_buffer_swap_copy( hb_buffer_t *src, hb_buffer_t *dst )
{
    uint8_t *data  = dst->data;
    int      size  = dst->size;
    int      alloc = dst->alloc;

    *dst = *src;

    src->data  = data;
    src->size  = size;
    src->alloc = alloc;
}

// Frees the specified buffer list.
void hb_buffer_close( hb_buffer_t ** _b )
{
    hb_buffer_t * b = *_b;

    while( b )
    {
        hb_buffer_t * next = b->next;
        hb_fifo_t *buffer_pool = size_to_pool( b->alloc );

        b->next = NULL;

        // Close any attached subtitle buffers
        hb_buffer_close( &b->sub );

        if( buffer_pool && b->data && !hb_fifo_is_full( buffer_pool ) )
        {
            hb_fifo_push_head( buffer_pool, b );
            b = next;
            continue;
        }
        // either the pool is full or this size doesn't use a pool
        // free the buf 
        if( b->data )
        {
            free( b->data );
            hb_lock(buffers.lock);
            buffers.allocated -= b->alloc;
            hb_unlock(buffers.lock);
        }
        free( b );
        b = next;
    }

    *_b = NULL;
}

void hb_buffer_move_subs( hb_buffer_t * dst, hb_buffer_t * src )
{
    // Note that dst takes ownership of the subtitles
    dst->sub       = src->sub;
    src->sub       = NULL;

#ifdef USE_QSV
	memcpy(&dst->qsv_details, &src->qsv_details, sizeof(src->qsv_details));
#endif

}

hb_fifo_t * hb_fifo_init( int capacity, int thresh )
{
    hb_fifo_t * f;
    f             = calloc( sizeof( hb_fifo_t ), 1 );
    f->lock       = hb_lock_init();
    f->cond_full  = hb_cond_init();
    f->cond_empty = hb_cond_init();
    f->capacity   = capacity;
    f->thresh     = thresh;
    f->buffer_size = 0;

#if defined(HB_FIFO_DEBUG)
    // Add the fifo to the global fifo list
    fifo_list_add( f );
#endif
    return f;
}

int hb_fifo_size_bytes( hb_fifo_t * f )
{
    int ret = 0;
    hb_buffer_t * link;

    hb_lock( f->lock );
    link = f->first;
    while ( link )
    {
        ret += link->size;
        link = link->next;
    }
    hb_unlock( f->lock );

    return ret;
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

// Pulls the first packet out of this FIFO, blocking until such a packet is available.
// Returns NULL if this FIFO has been closed or flushed.
hb_buffer_t * hb_fifo_get_wait( hb_fifo_t * f )
{
    hb_buffer_t * b;

    hb_lock( f->lock );
    if( f->size < 1 )
    {
        f->wait_empty = 1;
        hb_cond_timedwait( f->cond_empty, f->lock, FIFO_TIMEOUT );
        if( f->size < 1 )
        {
            hb_unlock( f->lock );
            return NULL;
        }
    }
    b         = f->first;
    f->first  = b->next;
    b->next   = NULL;
    f->size  -= 1;
    if( f->wait_full && f->size == f->capacity - f->thresh )
    {
        f->wait_full = 0;
        hb_cond_signal( f->cond_full );
    }
    hb_unlock( f->lock );

    return b;
}

// Pulls a packet out of this FIFO, or returns NULL if no packet is available.
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
    if( f->wait_full && f->size == f->capacity - f->thresh )
    {
        f->wait_full = 0;
        hb_cond_signal( f->cond_full );
    }
    hb_unlock( f->lock );

    return b;
}

hb_buffer_t * hb_fifo_see_wait( hb_fifo_t * f )
{
    hb_buffer_t * b;

    hb_lock( f->lock );
    if( f->size < 1 )
    {
        f->wait_empty = 1;
        hb_cond_timedwait( f->cond_empty, f->lock, FIFO_TIMEOUT );
        if( f->size < 1 )
        {
            hb_unlock( f->lock );
            return NULL;
        }
    }
    b = f->first;
    hb_unlock( f->lock );

    return b;
}

// Returns the first packet in the specified FIFO.
// If the FIFO is empty, returns NULL.
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

// Waits until the specified FIFO is no longer full or until FIFO_TIMEOUT milliseconds have elapsed.
// Returns whether the FIFO is non-full upon return.
int hb_fifo_full_wait( hb_fifo_t * f )
{
    int result;

    hb_lock( f->lock );
    if( f->size >= f->capacity )
    {
        f->wait_full = 1;
        hb_cond_timedwait( f->cond_full, f->lock, FIFO_TIMEOUT );
    }
    result = ( f->size < f->capacity );
    hb_unlock( f->lock );
    return result;
}

// Pushes the specified buffer onto the specified FIFO,
// blocking until the FIFO has space available.
void hb_fifo_push_wait( hb_fifo_t * f, hb_buffer_t * b )
{
    if( !b )
    {
        return;
    }

    hb_lock( f->lock );
    if( f->size >= f->capacity )
    {
        f->wait_full = 1;
        hb_cond_timedwait( f->cond_full, f->lock, FIFO_TIMEOUT );
    }
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
    if( f->wait_empty && f->size >= 1 )
    {
        f->wait_empty = 0;
        hb_cond_signal( f->cond_empty );
    }
    hb_unlock( f->lock );
}

// Appends the specified packet list to the end of the specified FIFO.
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
    if( f->wait_empty && f->size >= 1 )
    {
        f->wait_empty = 0;
        hb_cond_signal( f->cond_empty );
    }
    hb_unlock( f->lock );
}

// Prepends the specified packet list to the start of the specified FIFO.
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

// Pushes a list of packets onto the specified FIFO as a single element.
void hb_fifo_push_list_element( hb_fifo_t *fifo, hb_buffer_t *buffer_list )
{
    hb_buffer_t *container = hb_buffer_init( 0 );
    // XXX: Using an arbitrary hb_buffer_t pointer (other than 'next')
    //      to carry the list inside a single "container" buffer
    container->sub = buffer_list;
    
    hb_fifo_push( fifo, container );
}

// Removes a list of packets from the specified FIFO that were stored as a single element.
hb_buffer_t *hb_fifo_get_list_element( hb_fifo_t *fifo )
{
    hb_buffer_t *container = hb_fifo_get( fifo );
    // XXX: Using an arbitrary hb_buffer_t pointer (other than 'next')
    //      to carry the list inside a single "container" buffer
    hb_buffer_t *buffer_list = container->sub;
    hb_buffer_close( &container );
    
    return buffer_list;
}

void hb_fifo_close( hb_fifo_t ** _f )
{
    hb_fifo_t   * f = *_f;
    hb_buffer_t * b;

    if ( f == NULL )
        return;

    hb_deep_log( 2, "fifo_close: trashing %d buffer(s)", hb_fifo_size( f ) );
    while( ( b = hb_fifo_get( f ) ) )
    {
        hb_buffer_close( &b );
    }

    hb_lock_close( &f->lock );
    hb_cond_close( &f->cond_empty );
    hb_cond_close( &f->cond_full );

#if defined(HB_FIFO_DEBUG)
    // Remove the fifo from the global fifo list
    fifo_list_rem( f );
#endif

    free( f );

    *_f = NULL;
}

void hb_fifo_flush( hb_fifo_t * f )
{
    hb_buffer_t * b;

    while( ( b = hb_fifo_get( f ) ) )
    {
        hb_buffer_close( &b );
    }
    hb_lock( f->lock );
    hb_cond_signal( f->cond_empty );
    hb_cond_signal( f->cond_full );
    hb_unlock( f->lock );

}

