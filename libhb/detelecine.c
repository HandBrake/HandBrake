/* detelecine.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

/*
 *
 * PULLUP DEFINITIONS
 *
 */

#define PULLUP_FMT_Y         1
#define PULLUP_HAVE_BREAKS   1
#define PULLUP_HAVE_AFFINITY 2
#define PULLUP_BREAK_LEFT    1
#define PULLUP_BREAK_RIGHT   2

#define PULLUP_ABS( a ) (((a)^((a)>>31))-((a)>>31))

#ifndef PIC_FLAG_REPEAT_FIRST_FIELD
#define PIC_FLAG_REPEAT_FIRST_FIELD 256
#endif

struct pullup_buffer
{
    int lock[2];
    uint8_t **planes;
    int *size;
};

struct pullup_field
{
    int parity;
    struct pullup_buffer *buffer;
    unsigned int flags;
    int breaks;
    int affinity;
    int *diffs;
    int *comb;
    int *var;
    struct pullup_field *prev, *next;
};

struct pullup_frame
{
    int lock;
    int length;
    int parity;
    struct pullup_buffer **ifields, *ofields[2];
    struct pullup_buffer *buffer;
};

struct pullup_context
{
    /* Public interface */
    int format;
    int nplanes;
    int depth;
    int field_stride_shift;
    int half_value;
    int quarter_value;
    int *bpp, *w, *h, *stride, *background;
    void **background_lines;
    int junk_left, junk_right, junk_top, junk_bottom;
    int verbose;
    int metric_plane;
    int strict_breaks;
    int strict_pairs;
    int parity;
    /* Internal data */
    struct pullup_field *first, *last, *head;
    struct pullup_buffer *buffers;
    int nbuffers;
    int (*diff)(void *, void *, int);
    int (*comb)(void *, void *, int);
    int (*var)(void *, void *, int);
    int metric_w, metric_h, metric_len, metric_offset;
    struct pullup_frame *frame;
};

/*
 *
 * DETELECINE FILTER DEFINITIONS
 *
 */

struct hb_filter_private_s
{
    struct pullup_context * pullup_ctx;
    int                     pullup_fakecount;
    int                     pullup_skipflag;

    hb_filter_init_t        input;
    hb_filter_init_t        output;
};

static int hb_detelecine_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init );

static int hb_detelecine_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out );

static void hb_detelecine_close( hb_filter_object_t * filter );

static const char detelecine_template[] =
    "skip-left=^"HB_INT_REG"$:skip-right=^"HB_INT_REG"$:"
    "skip-top=^"HB_INT_REG"$:skip-bottom=^"HB_INT_REG"$:"
    "strict-breaks=^"HB_BOOL_REG"$:plane=^([012])$:parity=^([01])$:"
    "disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_detelecine =
{
    .id                = HB_FILTER_DETELECINE,
    .enforce_order     = 1,
    .name              = "Detelecine (pullup)",
    .settings          = NULL,
    .init              = hb_detelecine_init,
    .work              = hb_detelecine_work,
    .close             = hb_detelecine_close,
    .settings_template = detelecine_template,
};

/*
 *
 * PULLUP STATIC FUNCTIONS
 *
 */

#define DEF_INIT_BACKGROUND_LINE_FUNC(nbits)                                               \
static int init_background_line##_##nbits(struct pullup_context *c)                        \
{                                                                                          \
    for (int p = 0; p < c->nplanes; p++)                                                   \
    {                                                                                      \
        uint##nbits##_t *line = (uint##nbits##_t *)malloc(sizeof(uint##nbits##_t) *        \
                                                          c->stride[p] / (nbits / 8));     \
        if (line == NULL)                                                                  \
        {                                                                                  \
            return -1;                                                                     \
        }                                                                                  \
        const uint##nbits##_t background = c->background[p];                               \
        for (int i = 0; i < c->w[p]; i++)                                                  \
        {                                                                                  \
            line[i] = background;                                                          \
        }                                                                                  \
        c->background_lines[p] = (void *)line;                                             \
    }                                                                                      \
    return 0;                                                                              \
}

#define DEF_DIFF_Y_FUNC(nbits)                                              \
static int pullup_diff_y##_##nbits(void *a_in, void *b_in, int s)           \
{                                                                           \
    int diff = 0;                                                           \
    const uint##nbits##_t *a = (const uint##nbits##_t *)a_in;               \
    const uint##nbits##_t *b = (const uint##nbits##_t *)b_in;               \
    for (int i = 4; i; i--)                                                 \
    {                                                                       \
        for (int j = 0; j < 8; j++)                                         \
        {                                                                   \
            diff += PULLUP_ABS(a[j]-b[j]);                                  \
        }                                                                   \
        a+=s; b+=s;                                                         \
    }                                                                       \
    return diff;                                                            \
}                                                                           \

#define DEF_LICOMB_Y_FUNC(nbits)                                            \
static int pullup_licomb_y##_##nbits(void *a_in, void *b_in, int s)         \
{                                                                           \
    int diff = 0;                                                           \
    const uint##nbits##_t *a = (const uint##nbits##_t *)a_in;               \
    const uint##nbits##_t *b = (const uint##nbits##_t *)b_in;               \
    for (int i = 4; i; i--)                                                 \
    {                                                                       \
        for (int j = 0; j < 8; j++)                                         \
        {                                                                   \
            diff += PULLUP_ABS((a[j]<<1) - b[j-s] - b[j])                   \
                  + PULLUP_ABS((b[j]<<1) - a[j] - a[j+s]);                  \
        }                                                                   \
        a+=s; b+=s;                                                         \
    }                                                                       \
    return diff;                                                            \
}                                                                           \

#define DEF_VAR_Y_FUNC(nbits)                                               \
static int pullup_var_y##_##nbits(void *a_in, void *b_in, int s)            \
{                                                                           \
    int var = 0;                                                            \
    const uint##nbits##_t *a = (const uint##nbits##_t *)a_in;               \
    for (int i = 3; i; i--)                                                 \
    {                                                                       \
        for (int j = 0; j < 8; j++)                                         \
        {                                                                   \
            var += PULLUP_ABS(a[j]-a[j+s]);                                 \
        }                                                                   \
        a+= s;                                                              \
    }                                                                       \
    return 4 * var;                                                         \
}                                                                           \

DEF_DIFF_Y_FUNC(8)
DEF_DIFF_Y_FUNC(16)

DEF_LICOMB_Y_FUNC(8)
DEF_LICOMB_Y_FUNC(16)

DEF_VAR_Y_FUNC(8)
DEF_VAR_Y_FUNC(16)

DEF_INIT_BACKGROUND_LINE_FUNC(8)
DEF_INIT_BACKGROUND_LINE_FUNC(16)

static void pullup_alloc_metrics( struct pullup_context * c,
                                  struct pullup_field * f )
{
    f->diffs = calloc( c->metric_len, sizeof(int) );
    f->comb  = calloc( c->metric_len, sizeof(int) );
    f->var   = calloc( c->metric_len, sizeof(int) );
}

static void pullup_compute_metric( struct pullup_context * c,
                                   struct pullup_field * fa, int pa,
                                   struct pullup_field * fb, int pb,
                                   int (* func)( void *,
                                                 void *, int),
                                   int * dest )
{
    uint8_t *a, *b;
    int x, y;
    int mp    = c->metric_plane;
    int xstep = c->bpp[mp];
    int ystep = c->stride[mp] << 3;
    int s     = c->stride[mp] << c->field_stride_shift; /* field stride */
    int w     = c->metric_w*xstep;

    if( !fa->buffer || !fb->buffer ) return;

    /* Shortcut for duplicate fields (e.g. from RFF flag) */
    if( fa->buffer == fb->buffer && pa == pb )
    {
        memset( dest, 0, c->metric_len * sizeof(int) );
        return;
    }

    a = fa->buffer->planes[mp] + pa * c->stride[mp] + c->metric_offset;
    b = fb->buffer->planes[mp] + pb * c->stride[mp] + c->metric_offset;

    for( y = c->metric_h; y; y-- )
    {
        for( x = 0; x < w; x += xstep )
        {
            *dest++ = func( a + x, b + x, s );
        }
        a += ystep; b += ystep;
    }
}

static struct pullup_field * pullup_make_field_queue( struct pullup_context * c,
                                                      int len )
{
    struct pullup_field * head, * f;
    f = head = calloc( 1, sizeof(struct pullup_field) );
    pullup_alloc_metrics( c, f );
    for ( ; len > 0; len-- )
    {
        f->next = calloc( 1, sizeof(struct pullup_field) );
        f->next->prev = f;
        f = f->next;
        pullup_alloc_metrics( c, f );
    }
    f->next = head;
    head->prev = f;
    return head;
}

static void pullup_check_field_queue( struct pullup_context * c )
{
    if( c->head->next == c->first )
    {
        struct pullup_field *f = calloc( 1, sizeof(struct pullup_field) );
        pullup_alloc_metrics( c, f );
        f->prev = c->head;
        f->next = c->first;
        c->head->next = f;
        c->first->prev = f;
    }
}

static void pullup_copy_field( struct pullup_context * c,
                               struct pullup_buffer * dest,
                               struct pullup_buffer * src,
                               int parity )
{
    int i, j;
    uint8_t *d, *s;
    for( i = 0; i < c->nplanes; i++ )
    {
        s = src->planes[i] + parity*c->stride[i];
        d = dest->planes[i] + parity*c->stride[i];
        for( j = c->h[i]>>1; j; j-- )
        {
            memcpy( d, s, c->stride[i] );
            s += c->stride[i]<<1;
            d += c->stride[i]<<1;
        }
    }
}


static int pullup_queue_length( struct pullup_field * begin,
                                struct pullup_field * end )
{
    int count = 1;
    struct pullup_field * f;

    if( !begin || !end ) return 0;
    for( f = begin; f != end; f = f->next ) count++;
    return count;
}

static int pullup_find_first_break( struct pullup_field * f, int max )
{
    int i;
    for( i = 0; i < max; i++ )
    {
        if( f->breaks & PULLUP_BREAK_RIGHT ||
            f->next->breaks & PULLUP_BREAK_LEFT )
        {
            return i+1;
        }
        f = f->next;
    }
    return 0;
}

static void pullup_compute_breaks( struct pullup_context * c,
                                   struct pullup_field * f0 )
{
    int i;
    struct pullup_field *f1 = f0->next;
    struct pullup_field *f2 = f1->next;
    struct pullup_field *f3 = f2->next;
    int l, max_l=0, max_r=0;

    if( f0->flags & PULLUP_HAVE_BREAKS ) return;
    f0->flags |= PULLUP_HAVE_BREAKS;

    /* Special case when fields are 100% identical */
    if( f0->buffer == f2->buffer && f1->buffer != f3->buffer )
    {
        f2->breaks |= PULLUP_BREAK_RIGHT;
        return;
    }
    if( f0->buffer != f2->buffer && f1->buffer == f3->buffer )
    {
        f1->breaks |= PULLUP_BREAK_LEFT;
        return;
    }

    for( i = 0; i < c->metric_len; i++ )
    {
        l = f2->diffs[i] - f3->diffs[i];
        if(  l > max_l) max_l = l;
        if( -l > max_r) max_r = -l;
    }

    /* Don't get tripped up when differences are mostly quant error */
    if( max_l + max_r < c->half_value ) return;
    if( max_l > 4*max_r ) f1->breaks |= PULLUP_BREAK_LEFT;
    if( max_r > 4*max_l ) f2->breaks |= PULLUP_BREAK_RIGHT;
}

static void pullup_compute_affinity( struct pullup_context * c,
                                     struct pullup_field * f )
{
    int i;
    int max_l = 0, max_r = 0, l;

    if( f->flags & PULLUP_HAVE_AFFINITY )
    {
        return;
    }
    f->flags |= PULLUP_HAVE_AFFINITY;

    if( f->buffer == f->next->next->buffer )
    {
        f->affinity             =  1;
        f->next->affinity       =  0;
        f->next->next->affinity = -1;

        f->next->flags       |= PULLUP_HAVE_AFFINITY;
        f->next->next->flags |= PULLUP_HAVE_AFFINITY;

        return;
    }

    for( i = 0; i < c->metric_len; i++ )
    {
        int lv = f->prev->var[i];
        int rv = f->next->var[i];
        int v  = f->var[i];
        int lc = f->comb[i] - (v+lv) + PULLUP_ABS( v-lv );
        int rc = f->next->comb[i] - (v+rv) + PULLUP_ABS( v-rv );

        lc = (lc > 0) ? lc : 0;
        rc = (rc > 0) ? rc : 0;
        l = lc - rc;
        if(  l > max_l ) max_l = l;
        if( -l > max_r ) max_r = -l;
    }

    if( max_l + max_r < c->quarter_value )
    {
        return;
    }

    if( max_r > 6*max_l )
    {
        f->affinity = -1;
    }
    else if( max_l > 6*max_r )
    {
        f->affinity = 1;
    }
}

static void pullup_foo( struct pullup_context * c )
{
    struct pullup_field * f = c->first;
    int i, n = pullup_queue_length (f, c->last );
    for( i = 0; i < n-1; i++ )
    {
        if( i < n-3 ) pullup_compute_breaks( c, f );
        pullup_compute_affinity( c, f );
        f = f->next;
    }
}

static int pullup_decide_frame_length( struct pullup_context * c )
{
    struct pullup_field *f0 = c->first;
    struct pullup_field *f1 = f0->next;
    struct pullup_field *f2 = f1->next;
    int l;

    if( pullup_queue_length( c->first, c->last ) < 4 )
    {
        return 0;
    }
    pullup_foo( c );

    if( f0->affinity == -1 ) return 1;

    l = pullup_find_first_break( f0, 3 );
    if( l == 1 && c->strict_breaks < 0 ) l = 0;

    switch (l)
    {
        case 1:
            if ( c->strict_breaks < 1 &&
                 f0->affinity == 1 &&
                 f1->affinity == -1 )
            {
                return 2;
            }
            else
            {
                return 1;
            }

        case 2:
            /* FIXME: strictly speaking, f0->prev is no longer valid... :) */
            if( c->strict_pairs &&
                (f0->prev->breaks & PULLUP_BREAK_RIGHT) &&
                (f2->breaks & PULLUP_BREAK_LEFT) &&
                (f0->affinity != 1 || f1->affinity != -1) )
            {
                return 1;
            }
            if( f1->affinity == 1 )
            {
                return 1;
            }
            else
            {
                return 2;
            }

        case 3:
            if( f2->affinity == 1 )
            {
                return 2;
            }
            else
            {
                return 3;
            }

        default:
            /* 9 possibilities covered before switch */
            if( f1->affinity == 1 )
            {
                return 1; /* covers 6 */
            }
            else if( f1->affinity == -1 )
            {
                return 2; /* covers 6 */
            }
            else if( f2->affinity == -1 )
            {
                /* covers 2 */
                if( f0->affinity == 1 )
                {
                    return 3;
                }
                else
                {
                    return 1;
                }
            }
            else
            {
                return 2; /* the remaining 6 */
            }
    }
}

static void pullup_print_aff_and_breaks(struct pullup_context * c,
                                        struct pullup_field * f )
{
    int i;
    struct pullup_field * f0 = f;
    const char aff_l[] = "+..", aff_r[] = "..+";
    hb_log( "affinity: " );
    for( i = 0; i < 4; i++ )
    {
        printf( "%c%d%c",
                aff_l[1+f->affinity],
                i,
                aff_r[1+f->affinity] );

        f = f->next;
    }
    f = f0;
    printf("\nbreaks:   ");
    for( i = 0; i < 4; i++ )
    {
        printf( "%c%d%c",
                f->breaks & PULLUP_BREAK_LEFT  ? '|' : '.',
                i,
                f->breaks & PULLUP_BREAK_RIGHT ? '|' : '.' );

        f = f->next;
    }
    printf("\n");
}

/*
 *
 * PULLUP CONTEXT FUNCTIONS
 *
 */

struct pullup_context * pullup_alloc_context( void )
{
    struct pullup_context * c;

    c = calloc( 1, sizeof(struct pullup_context)) ;

    return c;
}

int pullup_preinit_context(struct pullup_context *c)
{
    c->bpp        = calloc( c->nplanes, sizeof(int) );
    c->w          = calloc( c->nplanes, sizeof(int) );
    c->h          = calloc( c->nplanes, sizeof(int) );
    c->stride     = calloc( c->nplanes, sizeof(int) );
    c->background = calloc( c->nplanes, sizeof(int) );
    c->background_lines = calloc( c->nplanes, sizeof(void *) );

    if (c->bpp == NULL || c->w == NULL || c->h == NULL ||
        c->stride == NULL || c->background == NULL ||
        c->background_lines == NULL)
    {
        return -1;
    }

    return 0;
}

int pullup_init_context(struct pullup_context *c)
{
    int mp = c->metric_plane;
    if ( c->nbuffers < 10 )
    {
        c->nbuffers = 10;
    }
    c->buffers = calloc( c->nbuffers, sizeof (struct pullup_buffer) );

    if (c->buffers == NULL)
    {
        return -1;
    }

    c->metric_w      = (c->w[mp] - ((c->junk_left + c->junk_right) << 3)) >> 3;
    c->metric_h      = (c->h[mp] - ((c->junk_top + c->junk_bottom) << 1)) >> 3;
    c->metric_offset = c->junk_left*c->bpp[mp] + (c->junk_top<<1)*c->stride[mp];
    c->metric_len    = c->metric_w * c->metric_h;

    c->head = pullup_make_field_queue( c, 8 );

    if (c->head == NULL)
    {
        return -1;
    }

    c->frame = calloc( 1, sizeof (struct pullup_frame) );
    c->frame->ifields = calloc( 3, sizeof (struct pullup_buffer *) );

    if (c->frame == NULL || c->frame->ifields == NULL)
    {
        return -1;
    }

    int err = 0;
    switch (c->depth)
    {
        case 8:
            err = init_background_line_8(c);
            break;
        default:
            err = init_background_line_16(c);
            break;
    }

    if (err)
    {
        return -1;
    }

    if (c->format == PULLUP_FMT_Y)
    {
        switch (c->depth)
        {
            case 8:
                c->diff = pullup_diff_y_8;
                c->comb = pullup_licomb_y_8;
                c->var  = pullup_var_y_8;
                break;

            default:
                c->diff = pullup_diff_y_16;
                c->comb = pullup_licomb_y_16;
                c->var  = pullup_var_y_16;
                break;
        }
    }

    return 0;
}

void pullup_free_context( struct pullup_context * c )
{
    for (int i = 0; i < c->nbuffers; i++)
    {
        struct pullup_buffer *b = &c->buffers[i];
        if (b->planes)
        {
            for (int p = 0; p < c->nplanes; p++)
            {
                free(b->planes[p]);
            }
            free(b->planes);
            free(b->size);
        }
    }
    free(c->buffers);

    free(c->bpp);
    free(c->w);
    free(c->h);
    free(c->stride);
    free(c->background);

    for (int p = 0; p < c->nplanes; p++)
    {
        free(c->background_lines[p]);
    }
    free(c->background_lines);

    if (c->head)
    {
        struct pullup_field *f = c->head->next;
        while (f != c->head)
        {
            free(f->diffs);
            free(f->comb);
            free(f->var);
            f = f->next;
            free(f->prev);
        }
        free(f->diffs);
        free(f->comb);
        free(f->var);
        free(f);
    }

    if (c->frame)
    {
        free(c->frame->ifields);
        free(c->frame);
    }
    free(c);
}

/*
 *
 * PULLUP BUFFER FUNCTIONS
 *
 */

static int pullup_alloc_buffer(struct pullup_context *c,
                               struct pullup_buffer *b)
{
    if (b->planes) return 0;

    b->planes = calloc(c->nplanes, sizeof(uint8_t *));
    b->size = calloc(c->nplanes, sizeof(int));

    if (b->planes == NULL || b->size == NULL)
    {
        return -1;
    }

    for (int i = 0; i < c->nplanes; i++)
    {
        b->size[i] = c->h[i] * c->stride[i];
        b->planes[i] = malloc(b->size[i]);

        if (b->planes[i] == NULL)
        {
            return -1;
        }

        /* Deal with idiotic 128=0 for chroma: */
        if (c->depth == 8 || i == 0)
        {
            memset( b->planes[i], c->background[i], b->size[i] );
        }
        else
        {
            // Copy a precomputed line if depth is > 8
            for (int h = 0; h < c->h[i]; h++)
            {
                memcpy(b->planes[i], c->background_lines[i], c->stride[i]);
            }
        }
    }

    return 0;
}

struct pullup_buffer * pullup_lock_buffer( struct pullup_buffer * b,
                                           int parity )
{
    if( !b ) return 0;
    if( (parity+1) & 1 ) b->lock[0]++;
    if( (parity+1) & 2 ) b->lock[1]++;

    return b;
}

void pullup_release_buffer( struct pullup_buffer * b,
                            int parity )
{
    if( !b ) return;
    if( (parity+1) & 1 ) b->lock[0]--;
    if( (parity+1) & 2 ) b->lock[1]--;
}

struct pullup_buffer * pullup_get_buffer( struct pullup_context * c,
                                          int parity )
{
    /* Try first to get the sister buffer for the previous field */
    if (parity < 2 &&
        c->last &&
        parity != c->last->parity &&
        !c->last->buffer->lock[parity])
    {
        if (pullup_alloc_buffer(c, c->last->buffer))
        {
            return NULL;
        }
        else
        {
            return pullup_lock_buffer(c->last->buffer, parity);
        }
    }

    /* Prefer a buffer with both fields open */
    for (int i = 0; i < c->nbuffers; i++)
    {
        if (c->buffers[i].lock[0]) continue;
        if (c->buffers[i].lock[1]) continue;
        if (pullup_alloc_buffer(c, &c->buffers[i]))
        {
            return NULL;
        }
        else
        {
            return pullup_lock_buffer(&c->buffers[i], parity);
        }
    }

    if (parity == 2) return 0;

    /* Search for any half-free buffer */
    for (int i = 0; i < c->nbuffers; i++)
    {
        if (((parity+1) & 1) && c->buffers[i].lock[0]) continue;
        if (((parity+1) & 2) && c->buffers[i].lock[1]) continue;
        if (pullup_alloc_buffer( c, &c->buffers[i]))
        {
            return NULL;
        }
        else
        {
            return pullup_lock_buffer(&c->buffers[i], parity);
        }
    }

    return NULL;
}

/*
 *
 * PULLUP FRAME FUNCTIONS
 *
 */

struct pullup_frame * pullup_get_frame( struct pullup_context * c )
{
    int i;
    struct pullup_frame * fr = c->frame;
    int n = pullup_decide_frame_length( c );
    int aff = c->first->next->affinity;

    if ( !n ) return 0;
    if ( fr->lock ) return 0;

    if ( c->verbose )
    {
        pullup_print_aff_and_breaks(c, c->first);
        printf("duration: %d    \n", n);
    }

    fr->lock++;
    fr->length = n;
    fr->parity = c->first->parity;
    fr->buffer = 0;
    for( i = 0; i < n; i++ )
    {
        /* We cheat and steal the buffer without release+relock */
        fr->ifields[i] = c->first->buffer;
        c->first->buffer = 0;
        c->first = c->first->next;
    }

    if( n == 1 )
    {
        fr->ofields[fr->parity] = fr->ifields[0];
        fr->ofields[fr->parity^1] = 0;
    }
    else if( n == 2 )
    {
        fr->ofields[fr->parity] = fr->ifields[0];
        fr->ofields[fr->parity^1] = fr->ifields[1];
    }
    else if( n == 3 )
    {
        if( aff == 0 )
        {
            aff = (fr->ifields[0] == fr->ifields[1]) ? -1 : 1;
        }
        fr->ofields[fr->parity]   = fr->ifields[1+aff];
        fr->ofields[fr->parity^1] = fr->ifields[1];
    }
    pullup_lock_buffer( fr->ofields[0], 0 );
    pullup_lock_buffer( fr->ofields[1], 1 );

    if( fr->ofields[0] == fr->ofields[1] )
    {
        fr->buffer = fr->ofields[0];
        pullup_lock_buffer(fr->buffer, 2);
        return fr;
    }
    return fr;
}

int pullup_pack_frame(struct pullup_context *c, struct pullup_frame *fr)
{
    if (fr->buffer) return 0;
    if (fr->length < 2) return -1; /* FIXME: deal with this */

    for (int i = 0; i < 2; i++)
    {
        if( fr->ofields[i]->lock[i^1] ) continue;
        fr->buffer = fr->ofields[i];
        pullup_lock_buffer(fr->buffer, 2);
        pullup_copy_field( c, fr->buffer, fr->ofields[i^1], i^1 );
        return 0;
    }

    fr->buffer = pullup_get_buffer(c, 2);

    if (fr->buffer == NULL)
    {
        return -1;
    }

    pullup_copy_field(c, fr->buffer, fr->ofields[0], 0);
    pullup_copy_field(c, fr->buffer, fr->ofields[1], 1);

    return 0;
}

void pullup_release_frame( struct pullup_frame * fr )
{
    int i;
    for( i = 0; i < fr->length; i++ )
    {
        pullup_release_buffer( fr->ifields[i], fr->parity ^ (i&1) );
    }
    pullup_release_buffer( fr->ofields[0], 0 );
    pullup_release_buffer( fr->ofields[1], 1 );
    if (fr->buffer) pullup_release_buffer( fr->buffer, 2 );
    fr->lock--;
}

/*
 *
 * PULLUP FIELD FUNCTIONS
 *
 */

void pullup_submit_field( struct pullup_context * c,
                          struct pullup_buffer * b,
                          int parity )
{
    struct pullup_field * f;

    /* Grow the circular list if needed */
    pullup_check_field_queue( c );

    /* Cannot have two fields of same parity in a row; drop the new one */
    if( c->last && c->last->parity == parity ) return;

    f = c->head;
    f->parity = parity;
    f->buffer = pullup_lock_buffer( b, parity );
    f->flags = 0;
    f->breaks = 0;
    f->affinity = 0;

    pullup_compute_metric( c, f, parity, f->prev->prev,
                           parity, c->diff, f->diffs );
    pullup_compute_metric( c, parity?f->prev:f, 0,
                           parity?f:f->prev, 1, c->comb, f->comb );
    pullup_compute_metric( c, f, parity, f,
                           -1, c->var, f->var );

    /* Advance the circular list */
    if( !c->first ) c->first = c->head;
    c->last = c->head;
    c->head = c->head->next;
}

void pullup_flush_fields( struct pullup_context * c )
{
    struct pullup_field * f;

    for( f = c->first; f && f != c->head; f = f->next )
    {
        pullup_release_buffer( f->buffer, f->parity );
        f->buffer = 0;
    }
    c->first = c->last = 0;
}

/*
 *
 * DETELECINE FILTER FUNCTIONS
 *
 */

static int hb_detelecine_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init )
{
    filter->private_data = calloc(1, sizeof(struct hb_filter_private_s));
    if (filter->private_data == NULL)
    {
        hb_error("detelecine: calloc failed");
        return -1;
    }

    hb_filter_private_t   * pv = filter->private_data;
    struct pullup_context * ctx;

    pv->input      = *init;
    pv->pullup_ctx = ctx = pullup_alloc_context();

    // "Skip" array [top, bottom, left, right]
    int top, bottom, left, right;

    left = right = ctx->junk_left = ctx->junk_right  = 1;
    top = bottom = ctx->junk_top  = ctx->junk_bottom = 4;
    ctx->strict_breaks = -1;
    ctx->metric_plane  = 0;
    ctx->parity = -1;

    hb_dict_extract_int(&top,    filter->settings, "skip-top");
    hb_dict_extract_int(&bottom, filter->settings, "skip-bottom");
    hb_dict_extract_int(&left,   filter->settings, "skip-left");
    hb_dict_extract_int(&right,  filter->settings, "skip-right");
    // Enforce safety zones
    ctx->junk_top    = top    > ctx->junk_top    ? top    : ctx->junk_top;
    ctx->junk_bottom = bottom > ctx->junk_bottom ? bottom : ctx->junk_bottom;
    ctx->junk_left   = left   > ctx->junk_left   ? left   : ctx->junk_left;
    ctx->junk_right  = right  > ctx->junk_right  ? right  : ctx->junk_right;

    hb_dict_extract_int(&ctx->strict_breaks, filter->settings, "strict-breaks");
    hb_dict_extract_int(&ctx->metric_plane, filter->settings, "plane");
    hb_dict_extract_int(&ctx->parity, filter->settings, "parity");

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);

    ctx->format     = PULLUP_FMT_Y;
    ctx->nplanes    = desc->nb_components;
    ctx->depth      = desc->comp[0].depth;
    ctx->field_stride_shift = ctx->depth > 8 ? 0 : 1;
    ctx->half_value         = (1 << ctx->depth) / 2;
    ctx->quarter_value      = (1 << ctx->depth) / 4;

    if (pullup_preinit_context(ctx))
    {
        hb_error("detelecine: pullup_preinit_context failed");
        goto fail;
    }

    ctx->bpp[0] = ctx->bpp[1] = ctx->bpp[2] = 8 * (ctx->depth > 8 ? 2 : 1);
    ctx->background[1] = ctx->background[2] = ctx->half_value;

    for (int p = 0; p < ctx->nplanes; p++)
    {
        ctx->w[p]      = hb_image_width(init->pix_fmt, init->geometry.width, p);
        ctx->h[p]      = hb_image_height(init->pix_fmt, init->geometry.height, p);
        ctx->stride[p] = hb_image_stride(init->pix_fmt, init->geometry.width, p);
    }

#if 0
    ctx->verbose = 1;
#endif

    if (ctx->metric_plane >= ctx->nplanes || ctx->metric_plane < 0)
    {
        ctx->metric_plane = 0;
    }

    if (pullup_init_context(ctx))
    {
        hb_error("detelecine: pullup_init_context failed");
        goto fail;
    }

    pv->pullup_fakecount = 1;
    pv->pullup_skipflag = 0;

    pv->output = *init;

    return 0;

fail:
    hb_detelecine_close(filter);
    return -1;
}

static void hb_detelecine_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    if( pv->pullup_ctx )
    {
        pullup_free_context( pv->pullup_ctx );
    }

    free( pv );
    filter->private_data = NULL;
}


static int hb_detelecine_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in, * out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    struct pullup_context * ctx = pv->pullup_ctx;
    struct pullup_buffer  * buf;
    struct pullup_frame   * frame;

    buf = pullup_get_buffer( ctx, 2 );
    if (!buf)
    {
        frame = pullup_get_frame(ctx);
        pullup_release_frame(frame);
        hb_log( "Could not get buffer from pullup!" );
        return HB_FILTER_FAILED;
    }

    for (int pp = 0; pp < 3; pp++)
    {
        if (buf->planes[pp] == NULL)
        {
            frame = pullup_get_frame( ctx );
            pullup_release_frame( frame );
            hb_log( "Could not get buffer from pullup!" );
            return HB_FILTER_FAILED;
        }
    }

    /* Copy input buffer into pullup buffer */
    for (int pp = 0; pp < 3; pp++)
    {
        if (in->plane[pp].stride == ctx->stride[pp])
        {
            memcpy(buf->planes[pp], in->plane[pp].data, buf->size[pp]);
        }
        else
        {
            const int stride_src = in->plane[pp].stride;
            const int stride_dst = ctx->stride[pp];
            const int height = in->plane[pp].height;
            const int size = stride_src < stride_dst ? ABS(stride_src) : stride_dst;
            uint8_t *dst = buf->planes[pp];
            uint8_t *src = in->plane[pp].data;

            for (int yy = 0; yy < height; yy++)
            {
                memcpy(dst, src, size);
                dst += stride_dst;
                src += stride_src;
            }
        }
    }

    /* Submit buffer fields based on buffer flags.
       Detelecine assumes BFF when the TFF flag isn't present. */
    int parity = 1;
    if( in->s.flags & PIC_FLAG_TOP_FIELD_FIRST )
    {
        /* Source signals TFF */
        parity = 0;
    }
    else if( ctx->parity == 0 )
    {
        /* Many non-MPEG-2 sources lack parity flags even though
           they are TFF, so this allow users to override. */
        parity = 0;
    }
    if( ctx->parity == 1 )
    {
        /* Override autodetected parity with BFF */
        parity = 1;
    }
    pullup_submit_field( ctx, buf, parity );
    pullup_submit_field( ctx, buf, parity^1 );
    if( in->s.flags & PIC_FLAG_REPEAT_FIRST_FIELD )
    {
        pullup_submit_field( ctx, buf, parity );
    }
    pullup_release_buffer( buf, 2 );

    /* Get frame and check if pullup is ready */
    frame = pullup_get_frame( ctx );
    if( !frame )
    {
        if( pv->pullup_fakecount )
        {
            pv->pullup_fakecount--;

            *buf_in = NULL;
            *buf_out = in;

            goto output_frame;
        }
        else
        {
            goto discard_frame;
        }
    }

    /* Check to see if frame should be dropped */
    if( frame->length < 2 )
    {
        pullup_release_frame( frame );
        frame = pullup_get_frame( ctx );

        if (!frame)
        {
            goto discard_frame;
        }
        if( frame->length < 2 )
        {
            pullup_release_frame( frame );

            if( !(in->s.flags & PIC_FLAG_REPEAT_FIRST_FIELD) )
            {
                goto discard_frame;
            }

            frame = pullup_get_frame( ctx );

            if( !frame )
            {
                goto discard_frame;
            }
            if( frame->length < 2 )
            {
                pullup_release_frame( frame );
                goto discard_frame;
            }
        }
    }

    /* Check to see if frame buffer is ready for export */
    if( !frame->buffer )
    {
        pullup_pack_frame( ctx, frame );
    }

    out = hb_frame_buffer_init(pv->output.pix_fmt, in->f.width, in->f.height);
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;

    /* Copy pullup frame buffer into output buffer */
    memcpy( out->plane[0].data, frame->buffer->planes[0], frame->buffer->size[0] );
    memcpy( out->plane[1].data, frame->buffer->planes[1], frame->buffer->size[1] );
    memcpy( out->plane[2].data, frame->buffer->planes[2], frame->buffer->size[2] );

    pullup_release_frame( frame );

    hb_buffer_copy_props(out, in);
    *buf_out = out;

output_frame:

    return HB_FILTER_OK;

/* This and all discard_frame calls shown above are
   the result of me restoring the functionality in
   pullup that huevos_rancheros disabled because
   HB couldn't handle it.                           */
discard_frame:
    return HB_FILTER_OK;

}


