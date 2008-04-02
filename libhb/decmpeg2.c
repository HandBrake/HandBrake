/* $Id: decmpeg2.c,v 1.12 2005/03/03 16:30:42 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "mpeg2dec/mpeg2.h"

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
int cadence[12];
int flag = 0;

/**********************************************************************
 * hb_libmpeg2_t
 **********************************************************************
 * A convenient libmpeg wrapper, used both here and in scan.c
 *********************************************************************/
struct hb_libmpeg2_s
{
    mpeg2dec_t         * libmpeg2;
    const mpeg2_info_t * info;
    int                  width;
    int                  height;
    int                  rate;
    int                  aspect_ratio;
    int                  got_iframe;
    int                  look_for_break;
    int64_t              last_pts;
};

/**********************************************************************
 * hb_libmpeg2_init
 **********************************************************************
 *
 *********************************************************************/
hb_libmpeg2_t * hb_libmpeg2_init()
{
    hb_libmpeg2_t * m = calloc( sizeof( hb_libmpeg2_t ), 1 );

    m->libmpeg2 = mpeg2_init();
    m->info     = mpeg2_info( m->libmpeg2 );
    m->last_pts = -1;
    m->look_for_break = 0;

    return m;
}

/**********************************************************************
 * hb_libmpeg2_decode
 **********************************************************************
 *
 *********************************************************************/
int hb_libmpeg2_decode( hb_libmpeg2_t * m, hb_buffer_t * buf_es,
                        hb_list_t * list_raw )
{
    mpeg2_state_t   state;
    hb_buffer_t   * buf;
    uint8_t       * data;
    int             chap_break = 0;

    /* Feed libmpeg2 */
    if( buf_es->start > -1 )
    {
        mpeg2_tag_picture( m->libmpeg2, buf_es->start >> 32,
                           buf_es->start & 0xFFFFFFFF );
    }
    mpeg2_buffer( m->libmpeg2, buf_es->data,
                  buf_es->data + buf_es->size );

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
                     * We do the calc in floating point to get the rounding right.
                     * We round in the second decimal digit because we scale
                     * the (integer) aspect by 9 to preserve the 1st digit.
                     */
                    double ar_numer = m->width * m->info->sequence->pixel_width;
                    double ar_denom = m->height * m->info->sequence->pixel_height;
                    m->aspect_ratio = ( ar_numer / ar_denom + .05 ) * HB_ASPECT_BASE;
                }
            }
        }
        else if( state == STATE_GOP && m->look_for_break == 2)
        {
            hb_log("MPEG2: Group of pictures found, searching for I-Frame");
            m->look_for_break = 1;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 m->info->display_fbuf )
        {
            if( ( m->info->display_picture->flags &
                  PIC_MASK_CODING_TYPE ) == PIC_FLAG_CODING_TYPE_I )
            {
                m->got_iframe = 1;

                // If we are looking for a break, insert the chapter break on an I-Frame
                if( m->look_for_break == 1 )
                {
                    hb_log("MPEG2: I-Frame Found");
                    m->look_for_break = 0;
                    chap_break = 1;
                }
            }

            if( m->got_iframe )
            {
                buf  = hb_buffer_init( m->width * m->height * 3 / 2 );
                data = buf->data;

                buf->sequence = buf_es->sequence;

                // Was a good break point found?
                if( chap_break )
                {
                    hb_log("MPEG2: Chapter Break Inserted");
                    chap_break = 0;
                    buf->new_chap = 1;
                }

                memcpy( data, m->info->display_fbuf->buf[0],
                        m->width * m->height );
                data += m->width * m->height;
                memcpy( data, m->info->display_fbuf->buf[1],
                        m->width * m->height / 4 );
                data += m->width * m->height / 4;
                memcpy( data, m->info->display_fbuf->buf[2],
                        m->width * m->height / 4 );

                if( m->info->display_picture->flags & PIC_FLAG_TAGS )
                {
                    buf->start =
                        ( (uint64_t) m->info->display_picture->tag << 32 ) |
                        ( (uint64_t) m->info->display_picture->tag2 );
		    /*
		      * Add back in again to track PTS of MPEG2 frames
		      * hb_log("MPEG2: Normal buf->start = %lld", buf->start);
		    */
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
 * hb_libmpeg2_info
 **********************************************************************
 *
 *********************************************************************/
void hb_libmpeg2_info( hb_libmpeg2_t * m, int * width, int * height,
                        int * rate, int *aspect_ratio )
{
    *width  = m->width;
    *height = m->height;
    if (m->info->display_fbuf)
    {
        if( (m->info->display_picture->flags & PROGRESSIVE) && (m->height == 480) )
        {
            /* The frame is progressive and it's NTSC DVD height, so change its FPS to 23.976.
               This might not be correct for the title. It's really just for scan.c's benefit.
               Scan.c will reset the fps to 29.97, until a simple majority of the preview
               frames report at 23.976.
            */
            //hb_log("Detecting NTSC Progressive Frame");
            m->rate = 1126125;
        }
    }
    *rate   = m->rate;
    *aspect_ratio = m->aspect_ratio;
}

int hb_libmpeg2_clear_aspect_ratio( hb_libmpeg2_t * m )
{
    int ar = m->aspect_ratio;
    m->aspect_ratio = 0;
    return ar;
}

/**********************************************************************
 * hb_libmpeg2_close
 **********************************************************************
 *
 *********************************************************************/
void hb_libmpeg2_close( hb_libmpeg2_t ** _m )
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
int decmpeg2Init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv;

    pv              = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->libmpeg2 = hb_libmpeg2_init();
    pv->list     = hb_list_init();

    return 0;
}

/**********************************************************************
 * Work
 **********************************************************************
 *
 *********************************************************************/
int decmpeg2Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                   hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf, * last = NULL;

    // The reader found a chapter break, consume it completely, and remove it from the
    // stream. We need to shift it.
    if( (*buf_in)->new_chap )
    {
        hb_log("MPEG2: Chapter Break Cell Found, searching for GOP");
        pv->libmpeg2->look_for_break = 2;
        (*buf_in)->new_chap = 0;
    }

    hb_libmpeg2_decode( pv->libmpeg2, *buf_in, pv->list );

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

    return HB_WORK_OK;
}

/**********************************************************************
 * Close
 **********************************************************************
 *
 *********************************************************************/
void decmpeg2Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_list_close( &pv->list );
    hb_libmpeg2_close( &pv->libmpeg2 );
    free( pv );
}

hb_work_object_t hb_decmpeg2 =
{
    WORK_DECMPEG2,
    "MPEG-2 decoder (libmpeg2)",
    decmpeg2Init,
    decmpeg2Work,
    decmpeg2Close
};

