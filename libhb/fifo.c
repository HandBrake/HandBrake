/* fifo.c

   Copyright (c) 2003-2024 HandBrake Team
   Copyright 2022 NVIDIA Corporation
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "libavcodec/avcodec.h"

#include "handbrake/handbrake.h"
#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_libav.h"
#include "handbrake/qsv_common.h"
#endif

#ifdef __APPLE__
#include <CoreMedia/CoreMedia.h>
#include "platform/macosx/vt_common.h"
#endif

#ifndef SYS_DARWIN
#if defined( SYS_FREEBSD ) || defined( SYS_NETBSD ) || defined( SYS_OPENBSD )
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#endif

#define FIFO_TIMEOUT 200
//#define HB_FIFO_DEBUG 1
// defining HB_BUFFER_DEBUG and HB_NO_BUFFER_POOL allows tracking
// buffer memory leaks using valgrind.  The source of the leak
// can be determined with "valgrind --leak-check=full"
//#define HB_BUFFER_DEBUG 1
//#define HB_NO_BUFFER_POOL 1

#if defined(HB_BUFFER_DEBUG)
#include <assert.h>
#endif

/* Fifo */
struct hb_fifo_s
{
    hb_lock_t    * lock;
    hb_cond_t    * cond_full;
    int            wait_full;
    hb_cond_t    * cond_empty;
    int            wait_empty;
    hb_cond_t    * cond_alert_full;
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
#define BUFFER_POOL_FIRST 10
#define BUFFER_POOL_LAST  25
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
#if !defined(HB_NO_BUFFER_POOL)
    hb_fifo_t *pool[MAX_BUFFER_POOLS];
#endif
#if defined(HB_BUFFER_DEBUG)
    hb_list_t *alloc_list;
#endif
} buffers;


#if defined(HB_BUFFER_DEBUG)
static int hb_fifo_contains( hb_fifo_t *f, hb_buffer_t *b );
#endif

void hb_buffer_pool_init( void )
{
    buffers.lock = hb_lock_init();
    buffers.allocated = 0;

#if defined(HB_BUFFER_DEBUG)
    buffers.alloc_list = hb_list_init();
#endif

#if !defined(HB_NO_BUFFER_POOL)
    /* we allocate pools for sizes 2^10 through 2^25. requests larger than
     * 2^25 will get passed through to malloc. */
    int i;

    // Create a queue with empty buffers for non native storage types
    buffers.pool[0] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS*10, 1);
    buffers.pool[0]->buffer_size = 0;

    // Create larger queue for 2^10 bucket since all allocations smaller than
    // 2^10 come from here.
    buffers.pool[BUFFER_POOL_FIRST] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS*10, 1);
    buffers.pool[BUFFER_POOL_FIRST]->buffer_size = 1 << 10;

    /* requests smaller than 2^10 are satisfied from the 2^10 pool. */
    for ( i = 1; i < BUFFER_POOL_FIRST; ++i )
    {
        buffers.pool[i] = buffers.pool[BUFFER_POOL_FIRST];
    }
    for ( i = BUFFER_POOL_FIRST + 1; i <= BUFFER_POOL_LAST; ++i )
    {
        buffers.pool[i] = hb_fifo_init(BUFFER_POOL_MAX_ELEMENTS, 1);
        buffers.pool[i]->buffer_size = 1 << i;
    }
#endif
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

#if !defined(HB_NO_BUFFER_POOL)
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
    for ( ii = BUFFER_POOL_FIRST; ii <= BUFFER_POOL_LAST; ++ii )
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
#endif

void hb_buffer_pool_free( void )
{
    int i;
    int64_t freed = 0;

    hb_lock(buffers.lock);

#if defined(HB_BUFFER_DEBUG)
    hb_deep_log(2, "leaked %d buffers", hb_list_count(buffers.alloc_list));
    for (i = 0; i < hb_list_count(buffers.alloc_list); i++)
    {
        hb_buffer_t *b = hb_list_item(buffers.alloc_list, i);
        hb_deep_log(2, "leaked buffer %p type %d size %d alloc %d",
               b, b->s.type, b->size, b->alloc);
    }
#endif

#if !defined(HB_NO_BUFFER_POOL)
    hb_buffer_t * b;
    int           count;
    for( i = BUFFER_POOL_FIRST; i <= BUFFER_POOL_LAST; ++i)
    {
        count = 0;
        while( ( b = hb_fifo_get(buffers.pool[i]) ) )
        {
            if( b->data )
            {
                freed += b->alloc;
                av_free(b->data);
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
#endif

#if defined(HB_BUFFER_DEBUG) && defined(HB_NO_BUFFER_POOL)
    // defining HB_BUFFER_DEBUG and HB_NO_BUFFER_POOL allows tracking
    // buffer memory leaks using valgrind.  The source of the leak
    // can be determined with "valgrind --leak-check=full"
    for (i = 0; i < hb_list_count(buffers.alloc_list); i++)
    {
        hb_buffer_t *b = hb_list_item(buffers.alloc_list, i);
        hb_list_rem(buffers.alloc_list, b);
    }
#endif

    hb_deep_log( 2, "Allocated %"PRId64" bytes of buffers on this pass and Freed %"PRId64" bytes, "
           "%"PRId64" bytes leaked", buffers.allocated, freed, buffers.allocated - freed);
    buffers.allocated = 0;
    hb_unlock(buffers.lock);
}

static hb_fifo_t *size_to_pool( int size )
{
#if !defined(HB_NO_BUFFER_POOL)
    if (size == 0)
    {
        return buffers.pool[0];
    }

    int i;
    for ( i = BUFFER_POOL_FIRST; i <= BUFFER_POOL_LAST; ++i )
    {
        if ( size <= (1 << i) )
        {
            return buffers.pool[i];
        }
    }
#endif
    return NULL;
}

hb_buffer_t * hb_buffer_init_internal( int size )
{
    hb_buffer_t * b;
    // Certain libraries (hrm ffmpeg) expect buffers passed to them to
    // end on certain alignments. So allocate some extra bytes.
    // Note that we can't simply align the end of our buffer because
    // sometimes we feed data to these libraries starting from arbitrary
    // points within the buffer.
    int alloc = size ? size + AV_INPUT_BUFFER_PADDING_SIZE : 0;
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
            b->alloc          = buffer_pool->buffer_size;
            b->size           = size;
            if (size)
            {
                b->data           = data;
            }
            b->storage        = NULL;
            b->s.start        = AV_NOPTS_VALUE;
            b->s.stop         = AV_NOPTS_VALUE;
            b->s.renderOffset = AV_NOPTS_VALUE;
            b->s.scr_sequence = -1;

#if defined(HB_BUFFER_DEBUG)
            hb_lock(buffers.lock);
            hb_list_add(buffers.alloc_list, b);
            hb_unlock(buffers.lock);
#endif
            return( b );
        }
    }

    /*
     * No existing buffers, create a new one
     */
    if( !( b = calloc( sizeof( hb_buffer_t ), 1 ) ) )
    {
        hb_error( "out of memory" );
        return NULL;
    }

    b->size  = size;
    b->alloc  = buffer_pool ? buffer_pool->buffer_size : alloc;

    if (size)
    {
        b->data = av_malloc(b->alloc);
        if( !b->data )
        {
            hb_error( "out of memory" );
            free( b );
            return NULL;
        }
#if defined(HB_BUFFER_DEBUG)
        memset(b->data, 0, b->size);
#endif
        hb_lock(buffers.lock);
        buffers.allocated += b->alloc;
        hb_unlock(buffers.lock);
    }
    b->s.start        = AV_NOPTS_VALUE;
    b->s.stop         = AV_NOPTS_VALUE;
    b->s.renderOffset = AV_NOPTS_VALUE;
    b->s.scr_sequence = -1;
#if defined(HB_BUFFER_DEBUG)
    hb_lock(buffers.lock);
    hb_list_add(buffers.alloc_list, b);
    hb_unlock(buffers.lock);
#endif
    return b;
}

hb_buffer_t * hb_buffer_wrapper_init()
{
    return hb_buffer_init_internal(0);
}

hb_buffer_t * hb_buffer_init( int size )
{
    return hb_buffer_init_internal(size);
}

hb_buffer_t * hb_buffer_eof_init(void)
{
    hb_buffer_t * buf = hb_buffer_init(0);
    buf->s.flags = HB_BUF_FLAG_EOF;
    return buf;
}

void hb_buffer_realloc( hb_buffer_t * b, int size )
{
    if ( size > b->alloc || b->data == NULL )
    {
        uint8_t   * tmp;
        uint32_t    orig = b->data != NULL ? b->alloc : 0;
        hb_fifo_t * buffer_pool = size_to_pool(size);

        if (buffer_pool != NULL)
        {
            size = buffer_pool->buffer_size;
        }
        tmp = av_malloc(size);
        if (tmp == NULL)
        {
            return;
        }
        if (b->data != NULL)
        {
            memcpy(tmp, b->data, b->alloc);
            av_free(b->data);
        }
        b->data  = tmp;
        b->alloc = size;

        hb_lock(buffers.lock);
        buffers.allocated += size - orig;
        hb_unlock(buffers.lock);
    }
}

void hb_buffer_reduce( hb_buffer_t * b, int size )
{
    if (b->storage_type == STANDARD && (size < b->alloc / 8 || b->data == NULL))
    {
        hb_buffer_t *tmp = hb_buffer_init(size);
        if (tmp)
        {
            hb_buffer_swap_copy(b, tmp);
            if (tmp->data)
            {
                memcpy(b->data, tmp->data, size);
            }
            tmp->next = NULL;
        }
        hb_buffer_close(&tmp);
    }
}

AVFrameSideData *hb_buffer_new_side_data_from_buf(hb_buffer_t *buf,
                                                  enum AVFrameSideDataType type,
                                                  AVBufferRef *side_data_buf)
{
    AVFrameSideData *ret;
    if (buf->storage_type == AVFRAME)
    {
        AVFrame *frame = (AVFrame *)buf->storage;
        ret = av_frame_new_side_data_from_buf(frame, type, side_data_buf);
        buf->side_data = (void **)frame->side_data;
        buf->nb_side_data = frame->nb_side_data;
        return ret;
    }
    else
    {
        AVFrameSideData **tmp;

        if (!buf)
        {
            return NULL;
        }

        if (buf->nb_side_data > INT_MAX / sizeof(*buf->side_data) - 1)
        {
            return NULL;
        }

        tmp = av_realloc(buf->side_data, (buf->nb_side_data + 1) * sizeof(*buf->side_data));
        if (!tmp)
        {
            return NULL;
        }
        buf->side_data = (void **)tmp;

        ret = av_mallocz(sizeof(*ret));
        if (!ret)
        {
            return NULL;
        }

        ret->buf = side_data_buf;
        ret->data = ret->buf->data;
        ret->size = side_data_buf->size;
        ret->type = type;

        buf->side_data[buf->nb_side_data++] = ret;

        return ret;
    }
}

static void free_side_data(AVFrameSideData **ptr_sd)
{
    AVFrameSideData *sd = *ptr_sd;

    av_buffer_unref(&sd->buf);
    av_dict_free(&sd->metadata);
    av_freep(ptr_sd);
}

void hb_buffer_remove_side_data(hb_buffer_t *buf, enum AVFrameSideDataType type)
{
    if (buf->storage_type == AVFRAME)
    {
        AVFrame *frame = (AVFrame *)buf->storage;
        av_frame_remove_side_data(frame, type);
        buf->nb_side_data = frame->nb_side_data;
    }
    else
    {
        for (int i = buf->nb_side_data - 1; i >= 0; i--)
        {
            AVFrameSideData *sd = buf->side_data[i];
            if (sd->type == type)
            {
                free_side_data((AVFrameSideData **)&buf->side_data[i]);
                buf->side_data[i] = buf->side_data[buf->nb_side_data - 1];
                buf->nb_side_data--;
            }
        }
    }
}

void hb_buffer_wipe_side_data(hb_buffer_t *buf)
{
    for (int i = 0; i < buf->nb_side_data; i++)
    {
        free_side_data((AVFrameSideData **)&buf->side_data[i]);
    }
    buf->nb_side_data = 0;

    av_freep(&buf->side_data);
}

void hb_buffer_copy_side_data(hb_buffer_t *dst, const hb_buffer_t *src)
{
    for (int i = 0; i < src->nb_side_data; i++)
    {
        const AVFrameSideData *sd_src = src->side_data[i];
        AVBufferRef *ref = av_buffer_ref(sd_src->buf);
        AVFrameSideData *sd_dst = hb_buffer_new_side_data_from_buf(dst, sd_src->type, ref);
        if (!sd_dst)
        {
            av_buffer_unref(&ref);
            hb_buffer_wipe_side_data(dst);
        }
    }
}

void hb_buffer_copy_props(hb_buffer_t *dst, const hb_buffer_t *src)
{
    dst->s = src->s;
    hb_buffer_copy_side_data(dst, src);
}

int hb_buffer_is_writable(const hb_buffer_t *buf)
{
    switch (buf->storage_type)
    {
        case AVFRAME:
            return av_frame_is_writable((AVFrame *)buf->storage);
        case STANDARD:
            return 1;
#ifdef __APPLE__
        case COREMEDIA:
            return CFGetRetainCount(buf->storage);
#endif
        default:
            return 0;
    }
}

static int copy_hwframe_to_video_buffer(const AVFrame *frame, hb_buffer_t *buf)
{
    int ret;
    AVFrame *hw_frame = av_frame_alloc();

    ret = av_frame_copy_props(hw_frame, frame);
    if (ret < 0)
    {
        hb_log("fifo: av_frame_copy_props");
    }
    ret = av_hwframe_get_buffer(frame->hw_frames_ctx, hw_frame, 0);
    if (ret < 0)
    {
        hb_log("fifo: av_hwframe_get_buffer failed");
    }
    ret = av_hwframe_transfer_data(hw_frame, frame, 0);
    if (ret < 0)
    {
        hb_log("fifo: av_hwframe_transfer_data failed");
    }

    buf->storage = hw_frame;
    buf->storage_type = AVFRAME;

    return ret;
}

static void copy_avframe_to_video_buffer(const AVFrame *frame, hb_buffer_t *buf)
{
    for (int pp = 0; pp <= buf->f.max_plane; pp++)
    {
        if (buf->plane[pp].stride == frame->linesize[pp])
        {
            memcpy(buf->plane[pp].data, frame->data[pp], frame->linesize[pp] * buf->plane[pp].height);
        }
        else
        {
            const int stride    = buf->plane[pp].stride;
            const int height    = buf->plane[pp].height;
            const int linesize  = frame->linesize[pp];
            const int size = linesize < stride ? ABS(linesize) : stride;
            uint8_t *dst = buf->plane[pp].data;
            uint8_t *src = frame->data[pp];
            for (int yy = 0; yy < height; yy++)
            {
                memcpy(dst, src, size);
                dst += stride;
                src += linesize;
            }
        }
    }
}

hb_buffer_t * hb_buffer_dup(const hb_buffer_t *src)
{
    hb_buffer_t *buf = NULL;

    if (src == NULL)
    {
        return NULL;
    }

    if (src->storage_type == STANDARD)
    {
        buf = hb_buffer_init(src->size);
        if (buf)
        {
            buf->f = src->f;
            hb_buffer_copy_props(buf, src);

            if (buf->s.type == FRAME_BUF)
            {
                hb_buffer_init_planes(buf);
            }

            memcpy(buf->data, src->data, src->size);
        }
    }
    else if (src->storage_type == AVFRAME)
    {
        const AVFrame *frame = (AVFrame *)src->storage;

        // If it's an hardware frame, make a copy
        // into another hardware AVFrame.
        if (frame->hw_frames_ctx)
        {
#ifdef __APPLE__
            if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX)
            {
                buf = hb_vt_buffer_dup(src);
            }
            else
#endif
            {
                buf = hb_buffer_wrapper_init();
                if (buf)
                {
                    buf->f = src->f;
                    hb_buffer_copy_props(buf, src);
                    copy_hwframe_to_video_buffer(frame, buf);
                }
            }
        }
        // If not, copy the content to a standard hb_buffer
        else
        {
            buf = hb_frame_buffer_init(src->f.fmt, src->f.width, src->f.height);
            if (buf)
            {
                buf->f = src->f;
                hb_buffer_copy_props(buf, src);
                copy_avframe_to_video_buffer(frame, buf);
            }
        }
    }
#ifdef __APPLE__
    else if (src->storage_type == COREMEDIA)
    {
        buf = hb_vt_buffer_dup(src);
    }
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
    dst->f = src->f;
    hb_buffer_copy_props(dst, src);
    if (dst->s.type == FRAME_BUF)
        hb_buffer_init_planes(dst);

    return 0;
}

void hb_buffer_init_planes(hb_buffer_t * b)
{
    uint8_t * data = b->data;
    int       pp;

    for( pp = 0; pp <= b->f.max_plane; pp++ )
    {
        b->plane[pp].data = data;
        b->plane[pp].stride        = hb_image_stride(b->f.fmt, b->f.width, pp);
        b->plane[pp].width         = hb_image_width(b->f.fmt, b->f.width, pp);
        b->plane[pp].height        = hb_image_height(b->f.fmt, b->f.height, pp);
        b->plane[pp].size          = b->plane[pp].stride *
                                     b->plane[pp].height;
        data                      += b->plane[pp].size;
    }
}

// this routine gets a buffer for an uncompressed picture
// with pixel format pix_fmt and dimensions width x height.
hb_buffer_t * hb_frame_buffer_init( int pix_fmt, int width, int height )
{
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    hb_buffer_t              * buf;
    uint8_t                    has_plane[4] = {0,};
    int                        ii, pp, max_plane = 0;

    if (desc == NULL)
    {
        return NULL;
    }

    int size = 0;
    for (ii = 0; ii < desc->nb_components; ii++)
    {
        pp    = desc->comp[ii].plane;
        if (pp > max_plane)
        {
            max_plane = pp;
        }
        if (!has_plane[pp])
        {
            has_plane[pp] = 1;
            size += hb_image_stride( pix_fmt, width, pp ) *
                    hb_image_height( pix_fmt, height, pp );
        }
    }

    buf = hb_buffer_init_internal(size);

    if( buf == NULL )
        return NULL;

    buf->f.max_plane = max_plane;
    buf->s.type = FRAME_BUF;
    buf->f.width = width;
    buf->f.height = height;
    buf->f.fmt = pix_fmt;

    hb_buffer_init_planes(buf);

    return buf;
}

void hb_frame_buffer_blank_stride(hb_buffer_t * buf)
{
    uint8_t * data;
    int       pp, yy, width, height, stride;

    for (pp = 0; pp <= buf->f.max_plane; pp++)
    {
        data          = buf->plane[pp].data;
        width         = buf->plane[pp].width;
        height        = buf->plane[pp].height;
        stride        = buf->plane[pp].stride;

        if (data != NULL)
        {
            // Blank right margin
            for (yy = 0; yy < height; yy++)
            {
                memset(data + yy * stride + width, 0x80, stride - width);
            }
        }
    }
}

#define DEF_MIRROR_STRIDE_FUNC(name, nbits)                                  \
static void name##_##nbits(uint8_t *data, int width, int height, int stride) \
{                                                                            \
    int pos, margin, margin_front, margin_back, bps;                         \
    uint##nbits##_t *data_in = (uint##nbits##_t *)data;                      \
                                                                             \
    bps = nbits > 8 ? 2 : 1;                                                 \
    stride      /= bps;                                                      \
    margin       = stride - width;                                           \
    margin_front = margin / 2;                                               \
    margin_back  = margin - margin_front;                                    \
    for (int yy = 0; yy < height; yy++)                                      \
    {                                                                        \
        /* Mirror final row pixels into front of stride region */            \
        pos = yy * stride + width;                                           \
        for (int ii = 0; ii < margin_back; ii++)                             \
        {                                                                    \
            *(data_in + pos + ii) = *(data_in + pos - ii - 1);               \
        }                                                                    \
        /* Mirror start of next row into end of stride region */             \
        pos = (yy + 1) * stride - 1;                                         \
        for (int ii = 0; ii < margin_front; ii++)                            \
        {                                                                    \
            *(data_in + pos - ii) = *(data_in + pos + ii + 1);               \
        }                                                                    \
    }                                                                        \
}                                                                            \

DEF_MIRROR_STRIDE_FUNC(mirror_stride, 16)
DEF_MIRROR_STRIDE_FUNC(mirror_stride, 8)

void hb_frame_buffer_mirror_stride(hb_buffer_t * buf)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(buf->f.fmt);
    int   depth = desc->comp[0].depth > 8 ? 2 : 1;

    for (int pp = 0; pp <= buf->f.max_plane; pp++)
    {
        if (buf->plane[pp].data != NULL)
        {
            switch (depth)
            {
                case 8:
                    mirror_stride_8(buf->plane[pp].data, buf->plane[pp].width,
                                    buf->plane[pp].height, buf->plane[pp].stride);
                    break;
                default:
                    mirror_stride_16(buf->plane[pp].data, buf->plane[pp].width,
                                     buf->plane[pp].height, buf->plane[pp].stride);
                    break;
            }
        }
    }
}

// this routine reallocs a buffer for an uncompressed video frame
// with dimensions width x height.
void hb_video_buffer_realloc( hb_buffer_t * buf, int width, int height )
{
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(buf->f.fmt);
    uint8_t                    has_plane[4] = {0,};
    int                        ii, pp;

    if (desc == NULL)
    {
        return;
    }

    buf->f.max_plane = 0;
    int size = 0;
    for (ii = 0; ii < desc->nb_components; ii++)
    {
        pp = desc->comp[ii].plane;
        if (pp > buf->f.max_plane)
        {
            buf->f.max_plane = pp;
        }
        if (!has_plane[pp])
        {
            has_plane[pp] = 1;
            size += hb_image_stride(buf->f.fmt, width, pp) *
                    hb_image_height(buf->f.fmt, height, pp );
        }
    }

    hb_buffer_realloc(buf, size );

    buf->f.width = width;
    buf->f.height = height;
    buf->size = size;

    hb_buffer_init_planes(buf);
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

#if HB_PROJECT_FEATURE_QSV
static void free_qsv_resources(hb_buffer_t *b)
{
    // Reclaim QSV resources before dropping the buffer.
    // when decoding without QSV, the QSV atom will be NULL.
    if (b->storage != NULL && b->qsv_details.ctx != NULL)
    {
        AVFrame *frame = (AVFrame *)b->storage;
        mfxFrameSurface1 *surface = (mfxFrameSurface1 *)frame->data[3];
        if (surface)
        {
            hb_qsv_release_surface_from_pool_by_surface_pointer(b->qsv_details.qsv_frames_ctx, surface);
            frame->data[3] = 0;
        }
    }
    if (b->qsv_details.qsv_atom != NULL && b->qsv_details.ctx != NULL)
    {
        hb_qsv_stage *stage = hb_qsv_get_last_stage(b->qsv_details.qsv_atom);
        if (stage != NULL)
        {
            hb_qsv_wait_on_sync(b->qsv_details.ctx, stage);
            if (stage->out.sync->in_use > 0)
            {
                ff_qsv_atomic_dec(&stage->out.sync->in_use);
            }
            if (stage->out.p_surface->Data.Locked > 0)
            {
                ff_qsv_atomic_dec(&stage->out.p_surface->Data.Locked);
            }
        }
        hb_qsv_flush_stages(b->qsv_details.ctx->pipes,
                            (hb_qsv_list**)&b->qsv_details.qsv_atom, 1);
    }
}
#endif


static void free_buffer_resources(hb_buffer_t *b)
{
    if (b->storage_type == AVFRAME)
    {
        av_frame_unref((AVFrame *)b->storage);
        av_frame_free((AVFrame **)&b->storage);
    }
#ifdef __APPLE__
    else if (b->storage_type == COREMEDIA)
    {
        CFRelease((CMSampleBufferRef)b->storage);
    }
#endif
    if (b->storage_type != AVFRAME)
    {
        hb_buffer_wipe_side_data(b);
        av_freep(&b->side_data);
    }
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

#if defined(HB_BUFFER_DEBUG)
        hb_lock(buffers.lock);
        hb_list_rem(buffers.alloc_list, b);
        hb_unlock(buffers.lock);
#endif

#if HB_PROJECT_FEATURE_QSV
        free_qsv_resources(b);
#endif
        free_buffer_resources(b);

        if (buffer_pool && !hb_fifo_is_full(buffer_pool))
        {
#if defined(HB_BUFFER_DEBUG)
            if (hb_fifo_contains(buffer_pool, b))
            {
                hb_error("hb_buffer_close: buffer %p already freed", b);
                assert(0);
            }
#endif
            hb_fifo_push_head( buffer_pool, b );
            b = next;
            continue;
        }
        // either the pool is full or this size doesn't use a pool
        // free the buf
        if (b->data && b->storage_type == STANDARD)
        {
            av_free(b->data);
            hb_lock(buffers.lock);
            buffers.allocated -= b->alloc;
            hb_unlock(buffers.lock);
        }
        free( b );
        b = next;
    }

    *_b = NULL;
}

hb_image_t * hb_image_init(int pix_fmt, int width, int height)
{
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    uint8_t                    has_plane[4] = {0,};
    int                        ii, pp;

    if (desc == NULL)
    {
        return NULL;
    }

    hb_image_t *image = calloc(1, sizeof(hb_image_t));
    if (image == NULL)
    {
        return NULL;
    }

    int size = 0;
    for (ii = 0; ii < desc->nb_components; ii++)
    {
        // For non-planar formats, comp[ii].plane can contain the
        // same value for multiple comp.
        pp = desc->comp[ii].plane;
        if (pp > image->max_plane)
        {
            image->max_plane = pp;
        }
        if (!has_plane[pp])
        {
            has_plane[pp] = 1;
            size += hb_image_stride( pix_fmt, width, pp ) *
                    hb_image_height( pix_fmt, height, pp );
        }
    }

    image->data  = av_malloc(size);
    if (image->data == NULL)
    {
        free(image);
        return NULL;
    }
    image->format = pix_fmt;
    image->width = width;
    image->height = height;
    memset(image->data, 0, size);

    uint8_t * data = image->data;
    for (pp = 0; pp <= image->max_plane; pp++)
    {
        image->plane[pp].data   = data;
        image->plane[pp].stride = hb_image_stride(pix_fmt, width, pp);
        image->plane[pp].width  = hb_image_width(pix_fmt, width, pp);
        image->plane[pp].height = hb_image_height(pix_fmt, height, pp);
        image->plane[pp].size   = image->plane[pp].stride *
                                  image->plane[pp].height;
        data                   += image->plane[pp].size;
    }
    return image;
}

hb_image_t * hb_buffer_to_image(hb_buffer_t *buf)
{
    hb_image_t *image = calloc(1, sizeof(hb_image_t));

    image->data  = av_malloc( buf->size );
    if (image->data == NULL)
    {
        free(image);
        return NULL;
    }

    image->format = buf->f.fmt;
    image->width = buf->f.width;
    image->height = buf->f.height;
    image->color_prim     = buf->f.color_prim;
    image->color_transfer = buf->f.color_transfer;
    image->color_matrix   = buf->f.color_matrix;

    int p;
    uint8_t *data = image->data;
    for (p = 0; p <= buf->f.max_plane; p++)
    {
        image->plane[p].data = data;
        image->plane[p].width = buf->plane[p].width;
        image->plane[p].height = buf->plane[p].height;
        image->plane[p].stride = buf->plane[p].stride;
        image->plane[p].size = buf->plane[p].size;

        memcpy(image->plane[p].data, buf->plane[p].data, buf->plane[p].size);

        data += image->plane[p].size;
    }
    return image;
}

void hb_image_close(hb_image_t **_image)
{
    if (_image == NULL)
        return;

    hb_image_t * image = *_image;
    if (image != NULL)
    {
        av_free(image->data);
        free(image);
        *_image = NULL;
    }
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

void hb_fifo_register_full_cond( hb_fifo_t * f, hb_cond_t * c )
{
    f->cond_alert_full = c;
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
        if (f->cond_alert_full != NULL)
            hb_cond_broadcast( f->cond_alert_full );
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
    if (f->size >= f->capacity &&
        f->cond_alert_full != NULL)
    {
        hb_cond_broadcast( f->cond_alert_full );
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
    if (f->size >= f->capacity &&
        f->cond_alert_full != NULL)
    {
        hb_cond_broadcast( f->cond_alert_full );
    }

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

#if defined(HB_BUFFER_DEBUG)
static int hb_fifo_contains( hb_fifo_t *f, hb_buffer_t *b )
{
    hb_buffer_t * tmp = f->first;

    while (tmp != NULL)
    {
        if (b == tmp)
        {
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}
#endif
