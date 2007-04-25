/* $Id: decmpeg2.c,v 1.12 2005/03/03 16:30:42 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "mpeg2dec/mpeg2.h"

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
                
                if( m->rate == 900900 )
                {
                    /* 29.97 fps. 3:2 pulldown might, or might not be
                       used. I can't find a way to know, so we always
                       output 23.976 */
                    m->rate = 1126125;
                }
            }
        }
        else if( state == STATE_GOP && m->look_for_break == 2)
        {
            printf("MPEG2: Group of pictures found, searching for I-Frame\n");
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
                    printf("MPEG2: I-Frame Found\n");
                    m->look_for_break = 0;
                    chap_break = 1;
                }
            }

            if( m->got_iframe )
            {
                buf  = hb_buffer_init( m->width * m->height * 3 / 2 );
                data = buf->data;

                // Was a good break point found?
                if( chap_break )
                {
                    printf("MPEG2: Chapter Break Inserted\n");
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
                        int * rate )
{
    *width  = m->width;
    *height = m->height;
    *rate   = m->rate;
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
        printf("MPEG2: Chapter Break Cell Found, searching for GOP\n");
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

