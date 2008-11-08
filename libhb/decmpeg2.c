/* $Id: decmpeg2.c,v 1.12 2005/03/03 16:30:42 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "mpeg2dec/mpeg2.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

/* Cadence tracking */
#ifndef PIC_FLAG_REPEAT_FIRST_FIELD
#define PIC_FLAG_REPEAT_FIRST_FIELD 256
#endif
#define TOP_FIRST PIC_FLAG_TOP_FIELD_FIRST
#define PROGRESSIVE PIC_FLAG_PROGRESSIVE_FRAME
#define COMPOSITE PIC_FLAG_COMPOSITE_DISPLAY
#define SKIP PIC_FLAG_SKIP
#define TAGS PIC_FLAG_TAGS
#define REPEAT_FIRST PIC_FLAG_REPEAT_FIRST_FIELD
#define COMPOSITE_MASK PIC_MASK_COMPOSITE_DISPLAY
#define TB 8
#define BT 16
#define BT_PROG 32
#define BTB_PROG 64
#define TB_PROG 128
#define TBT_PROG 256
static int cadence[12];
static int flag = 0;

/**********************************************************************
 * hb_libmpeg2_t
 *********************************************************************/
typedef struct hb_libmpeg2_s
{
    mpeg2dec_t         * libmpeg2;
    const mpeg2_info_t * info;
    hb_job_t           * job;
    int                  width;
    int                  height;
    int                  rate;
    double               aspect_ratio;
    int                  got_iframe;        /* set when we get our first iframe */
    int                  look_for_iframe;   /* need an iframe to add chap break */
    int                  look_for_break;    /* need gop start to add chap break */
    uint32_t             nframes;           /* number of frames we've decoded */
    int64_t              last_pts;
} hb_libmpeg2_t;

/**********************************************************************
 * hb_libmpeg2_init
 **********************************************************************
 *
 *********************************************************************/
static hb_libmpeg2_t * hb_libmpeg2_init()
{
    hb_libmpeg2_t * m = calloc( sizeof( hb_libmpeg2_t ), 1 );

    m->libmpeg2 = mpeg2_init();
    m->info     = mpeg2_info( m->libmpeg2 );
    m->last_pts = -1;

    return m;
}

static hb_buffer_t *hb_copy_frame( hb_job_t *job, int width, int height,
                                   uint8_t* y, uint8_t *u, uint8_t *v )
{
    int dst_w = width, dst_h = height;
    if ( job )
    {
        dst_w = job->title->width;
        dst_h = job->title->height;
    }
    int dst_wh = dst_w * dst_h;
    hb_buffer_t *buf  = hb_video_buffer_init( dst_w, dst_h );

    if ( dst_w != width || dst_h != height )
    {
        // we're encoding and the frame dimensions don't match the title dimensions -
        // rescale & matte Y, U, V into our output buf.
        AVPicture in, out;
        avpicture_alloc(&in,  PIX_FMT_YUV420P, width, height );
        avpicture_alloc(&out, PIX_FMT_YUV420P, dst_w, dst_h );

        int src_wh = width * height;
        memcpy( in.data[0], y, src_wh );
        memcpy( in.data[1], u, src_wh >> 2 );
        memcpy( in.data[2], v, src_wh >> 2 );
        struct SwsContext *context = sws_getContext( width, height, PIX_FMT_YUV420P,
                                                     dst_w, dst_h, PIX_FMT_YUV420P,
                                                     SWS_LANCZOS|SWS_ACCURATE_RND,
                                                     NULL, NULL, NULL );
        sws_scale( context, in.data, in.linesize, 0, height, out.data, out.linesize );
        sws_freeContext( context );

        u_int8_t *data = buf->data;
        memcpy( data, out.data[0], dst_wh );
        data += dst_wh;
        // U & V planes are 1/4 the size of Y plane.
        dst_wh >>= 2;
        memcpy( data, out.data[1], dst_wh );
        data += dst_wh;
        memcpy( data, out.data[2], dst_wh );

        avpicture_free( &out );
        avpicture_free( &in );
    }
    else
    {
        // we're scanning or the frame dimensions match the title's dimensions
        // so we can do a straight copy.
        u_int8_t *data = buf->data;
        memcpy( data, y, dst_wh );
        data += dst_wh;
        // U & V planes are 1/4 the size of Y plane.
        dst_wh >>= 2;
        memcpy( data, u, dst_wh );
        data += dst_wh;
        memcpy( data, v, dst_wh );
    }
    return buf;
}

/**********************************************************************
 * hb_libmpeg2_decode
 **********************************************************************
 *
 *********************************************************************/
static int hb_libmpeg2_decode( hb_libmpeg2_t * m, hb_buffer_t * buf_es,
                        hb_list_t * list_raw )
{
    mpeg2_state_t   state;
    hb_buffer_t   * buf;

    if ( buf_es->size )
    {
        /* Feed libmpeg2 */
        if( buf_es->start > -1 )
        {
            mpeg2_tag_picture( m->libmpeg2, buf_es->start >> 32,
                               buf_es->start & 0xFFFFFFFF );
        }
        mpeg2_buffer( m->libmpeg2, buf_es->data,
                      buf_es->data + buf_es->size );
    }

    for( ;; )
    {
        state = mpeg2_parse( m->libmpeg2 );
        if( state == STATE_BUFFER )
        {
            /* Require some more data */
            break;
        }
        else if( state == STATE_SEQUENCE )
        {
            if( !( m->width && m->height && m->rate ) )
            {
                m->width  = m->info->sequence->width;
                m->height = m->info->sequence->height;
                m->rate   = m->info->sequence->frame_period;
                if ( m->aspect_ratio <= 0 && m->height &&
                     m->info->sequence->pixel_height )
                {
                    /* mpeg2_parse doesn't store the aspect ratio. Instead
                     * it keeps the pixel width & height that would cause
                     * the storage width & height to come out in the correct
                     * aspect ratio. Convert these back to aspect ratio.
                     */
                    double ar_numer = m->width * m->info->sequence->pixel_width;
                    double ar_denom = m->height * m->info->sequence->pixel_height;
                    m->aspect_ratio = ar_numer / ar_denom;
                }
            }
        }
        else if( state == STATE_GOP && m->look_for_break)
        {
            // we were looking for a gop to add a chapter break - we found it
            // so now start looking for an iframe.
            m->look_for_iframe = m->look_for_break;
            m->look_for_break = 0;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 m->info->display_fbuf )
        {
            if( ( m->info->display_picture->flags &
                  PIC_MASK_CODING_TYPE ) == PIC_FLAG_CODING_TYPE_I )
            {
                // we got an iframe so we can start decoding video now
                m->got_iframe = 1;
            }

            if( m->got_iframe )
            {
                buf  = hb_copy_frame( m->job, m->info->sequence->width,
                                      m->info->sequence->height,
                                      m->info->display_fbuf->buf[0],
                                      m->info->display_fbuf->buf[1],
                                      m->info->display_fbuf->buf[2] );
                buf->sequence = buf_es->sequence;

                if( m->info->display_picture->flags & PIC_FLAG_TAGS )
                {
                    buf->start =
                        ( (uint64_t) m->info->display_picture->tag << 32 ) |
                        ( (uint64_t) m->info->display_picture->tag2 );
                }
                else if( m->last_pts > -1 )
                {
                    /* For some reason nb_fields is sometimes 1 while it
                       should be 2 */
                    buf->start = m->last_pts +
                        MAX( 2, m->info->display_picture->nb_fields ) *
                        m->info->sequence->frame_period / 600;
                }
                else
                {
                    buf->start = -1;
                }
                m->last_pts = buf->start;

                if( m->look_for_iframe && ( m->info->display_picture->flags &
                      PIC_MASK_CODING_TYPE ) == PIC_FLAG_CODING_TYPE_I )
                {
                    // we were waiting for an iframe to insert a chapter mark
                    // and we have one.
                    buf->new_chap = m->look_for_iframe;
                    m->look_for_iframe = 0;
                    const char *chap_name = "";
                    if ( m->job && buf->new_chap > 0 &&
                         hb_list_item( m->job->title->list_chapter,
                                       buf->new_chap - 1 ) )
                    {
                        hb_chapter_t * c = hb_list_item( m->job->title->list_chapter,
                                                         buf->new_chap - 1 );
                        chap_name = c->title;
                    }
                    hb_log( "mpeg2: \"%s\" (%d) at frame %u time %lld",
                            chap_name, buf->new_chap, m->nframes, buf->start );
                } else if ( m->nframes == 0 && m->job &&
                            hb_list_item( m->job->title->list_chapter,
                                          m->job->chapter_start - 1 ) )
                {
                    hb_chapter_t * c = hb_list_item( m->job->title->list_chapter,
                                                     m->job->chapter_start - 1 );
                    hb_log( "mpeg2: \"%s\" (%d) at frame %u time %lld", c->title,
                            m->job->chapter_start, m->nframes, buf->start );
                }
                ++m->nframes;

                flag = m->info->display_picture->flags;

/*  Uncomment this block to see frame-by-frame picture flags, as the video encodes.
               hb_log("***** MPEG 2 Picture Info for PTS %lld *****", buf->start);
                if( flag & TOP_FIRST )
                    hb_log("MPEG2 Flag: Top field first");
                if( flag & PROGRESSIVE )
                    hb_log("MPEG2 Flag: Progressive");
                if( flag & COMPOSITE )
                    hb_log("MPEG2 Flag: Composite");
                if( flag & SKIP )
                    hb_log("MPEG2 Flag: Skip!");
                if( flag & TAGS )
                    hb_log("MPEG2 Flag: TAGS");
                if(flag & REPEAT_FIRST )
                    hb_log("MPEG2 Flag: Repeat first field");
                if( flag & COMPOSITE_MASK )
                    hb_log("MPEG2 Flag: Composite mask");
                hb_log("fields: %d", m->info->display_picture->nb_fields);
*/
                /*  Rotate the cadence tracking. */
                int i = 0;
                for(i=11; i > 0; i--)
                {
                    cadence[i] = cadence[i-1];
                }

                if ( !(flag & PROGRESSIVE) && !(flag & TOP_FIRST) )
                {
                    /* Not progressive, not top first...
                       That means it's probably bottom
                       first, 2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Bottom field first, 2 fields displayed.");
                    cadence[0] = BT;
                }
                else if ( !(flag & PROGRESSIVE) && (flag & TOP_FIRST) )
                {
                    /* Not progressive, top is first,
                       Two fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Top field first, 2 fields displayed.");
                    cadence[0] = TB;
                }
                else if ( (flag & PROGRESSIVE) && !(flag & TOP_FIRST) && !( flag & REPEAT_FIRST )  )
                {
                    /* Progressive, but noting else.
                       That means Bottom first,
                       2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive. Bottom field first, 2 fields displayed.");
                    cadence[0] = BT_PROG;
                }
                else if ( (flag & PROGRESSIVE) && !(flag & TOP_FIRST) && ( flag & REPEAT_FIRST )  )
                {
                    /* Progressive, and repeat. .
                       That means Bottom first,
                       3 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive repeat. Bottom field first, 3 fields displayed.");
                    cadence[0] = BTB_PROG;
                }
                else if ( (flag & PROGRESSIVE) && (flag & TOP_FIRST) && !( flag & REPEAT_FIRST )  )
                {
                    /* Progressive, top first.
                       That means top first,
                       2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive. Top field first, 2 fields displayed.");
                    cadence[0] = TB_PROG;
                }
                else if ( (flag & PROGRESSIVE) && (flag & TOP_FIRST) && ( flag & REPEAT_FIRST )  )
                {
                    /* Progressive, top, repeat.
                       That means top first,
                       3 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive repeat. Top field first, 3 fields displayed.");
                    cadence[0] = TBT_PROG;
                }

                if ( (cadence[2] <= TB) && (cadence[1] <= TB) && (cadence[0] > TB) && (cadence[11]) )
                    hb_log("%fs: Video -> Film", (float)buf->start / 90000);
                if ( (cadence[2] > TB) && (cadence[1] <= TB) && (cadence[0] <= TB) && (cadence[11]) )
                    hb_log("%fs: Film -> Video", (float)buf->start / 90000);

                /* Store picture flags for later use by filters */
                buf->flags = m->info->display_picture->flags;

                hb_list_add( list_raw, buf );
            }
        }
        else if( state == STATE_INVALID )
        {
            mpeg2_reset( m->libmpeg2, 0 );
        }
    }
    return 1;
}

/**********************************************************************
 * hb_libmpeg2_close
 **********************************************************************
 *
 *********************************************************************/
static void hb_libmpeg2_close( hb_libmpeg2_t ** _m )
{
    hb_libmpeg2_t * m = *_m;

    mpeg2_close( m->libmpeg2 );

    free( m );
    *_m = NULL;
}

/**********************************************************************
 * The decmpeg2 work object
 **********************************************************************
 *
 *********************************************************************/
struct hb_work_private_s
{
    hb_libmpeg2_t * libmpeg2;
    hb_list_t     * list;
};

/**********************************************************************
 * hb_work_decmpeg2_init
 **********************************************************************
 *
 *********************************************************************/
static int decmpeg2Init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->libmpeg2 = hb_libmpeg2_init();
    pv->list     = hb_list_init();

    pv->libmpeg2->job = job;

    return 0;
}

/**********************************************************************
 * Work
 **********************************************************************
 *
 *********************************************************************/
static int decmpeg2Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                   hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf, * last = NULL;
    int status = HB_WORK_OK;

    // The reader found a chapter break, consume it completely, and remove it from the
    // stream. We need to shift it.
    if( (*buf_in)->new_chap )
    {
        pv->libmpeg2->look_for_break = (*buf_in)->new_chap;
        (*buf_in)->new_chap = 0;
    }

    hb_libmpeg2_decode( pv->libmpeg2, *buf_in, pv->list );

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( (*buf_in)->size == 0 )
    {
        hb_list_add( pv->list, *buf_in );
        *buf_in = NULL;
        status = HB_WORK_DONE;
    }

    *buf_out = NULL;
    while( ( buf = hb_list_item( pv->list, 0 ) ) )
    {
        hb_list_rem( pv->list, buf );
        if( last )
        {
            last->next = buf;
            last       = buf;
        }
        else
        {
            *buf_out = buf;
            last     = buf;
        }
    }

    return status;
}

/**********************************************************************
 * Close
 **********************************************************************
 *
 *********************************************************************/
static void decmpeg2Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    // don't log during scan
    if ( pv->libmpeg2->job )
    {
        hb_log( "mpeg2 done: %d frames", pv->libmpeg2->nframes );
    }
    hb_list_close( &pv->list );
    hb_libmpeg2_close( &pv->libmpeg2 );
    free( pv );
}

static int decmpeg2Info( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->libmpeg2 && pv->libmpeg2->info && pv->libmpeg2->info->sequence )
    {
        hb_libmpeg2_t *m = pv->libmpeg2;

        info->width = m->width;
        info->height = m->height;
        info->pixel_aspect_width = m->info->sequence->pixel_width;
        info->pixel_aspect_height = m->info->sequence->pixel_height;
        info->aspect = m->aspect_ratio;

        // if the frame is progressive & NTSC DVD height report it as 23.976 FPS
        // so that scan can autodetect NTSC film
        info->rate = 27000000;
        info->rate_base = ( m->info->display_fbuf && m->info->display_picture &&
                            (m->info->display_picture->flags & PROGRESSIVE) &&
                            (m->height == 480 ) ) ?  1126125 : m->rate;

        info->bitrate = m->info->sequence->byte_rate * 8;
        info->profile = m->info->sequence->profile_level_id >> 4;
        info->level = m->info->sequence->profile_level_id & 0xf;
        info->name = "mpeg2";
        return 1;
    }
    return 0;
}

hb_work_object_t hb_decmpeg2 =
{
    WORK_DECMPEG2,
    "MPEG-2 decoder (libmpeg2)",
    decmpeg2Init,
    decmpeg2Work,
    decmpeg2Close,
    decmpeg2Info
};

