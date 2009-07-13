/* $Id: decmpeg2.c,v 1.12 2005/03/03 16:30:42 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hbffmpeg.h"
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

#define NTAGS 8

/**********************************************************************
 * hb_libmpeg2_t
 *********************************************************************/
typedef struct hb_libmpeg2_s
{
    mpeg2dec_t         * libmpeg2;
    const mpeg2_info_t * info;
    hb_job_t           * job;
    hb_title_t         * title;
    int                  width;
    int                  height;
    int                  rate;
    double               aspect_ratio;
    int                  got_iframe;        /* set when we get our first iframe */
    int                  look_for_iframe;   /* need an iframe to add chap break */
    int                  look_for_break;    /* need gop start to add chap break */
    int                  cur_tag;           /* index of current tag */
    uint32_t             nframes;           /* number of frames we've decoded */
    int64_t              last_pts;
    int64_t              first_pts;
    int cadence[12];
    int flag;
    hb_list_t          * list_subtitle;
    hb_buffer_t        * last_cc1_buf;
    struct {
        int64_t          start;             // start time of this frame
        hb_buffer_t    * cc_buf;            // captions for this frame
    } tags[NTAGS];
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
    m->first_pts = -1;

    int i;
    for ( i = 0; i < NTAGS; ++i )
    {
        m->tags[i].start = -1;
    }

    return m;
}

// send cc_buf to the CC decoder(s)
static void cc_send_to_decoder( hb_libmpeg2_t *m, hb_buffer_t *cc_buf )
{
    hb_subtitle_t *subtitle;

    // if there's more than one decoder for the captions send a copy
    // of the buffer to all but the last. Then send the buffer to
    // the last one (usually there's just one decoder so the 'while' is skipped).
    int i = 0, n = hb_list_count( m->list_subtitle );
    while ( --n > 0 )
    {
        // make a copy of the buf then forward it to the decoder
        hb_buffer_t *cpy = hb_buffer_init( cc_buf->size );
        hb_buffer_copy_settings( cpy, cc_buf );
        memcpy( cpy->data, cc_buf->data, cc_buf->size );

        subtitle = hb_list_item( m->list_subtitle, i++ );
        hb_fifo_push( subtitle->fifo_in, cpy );
    }
    subtitle = hb_list_item( m->list_subtitle, i );
    hb_fifo_push( subtitle->fifo_in, cc_buf );
}

static void hb_mpeg2_cc( hb_libmpeg2_t *m, const uint8_t *cc_block )
{
    uint8_t cc_hdr = *cc_block;
    
    if ( ( cc_hdr & 0x4 ) == 0 )
        // not valid - ignore
        return;

    switch (cc_hdr & 3)
    {
        case 0:
            // CC1 stream
            if ( ( cc_block[1] & 0x7f ) == 0 && ( cc_block[2] & 0x7f ) == 0 )
                // just padding - ignore
                return;

            if ( m->last_cc1_buf )
            {
                // new data from the same time as last call - add to buffer
                int len = m->last_cc1_buf->size;
                hb_buffer_realloc( m->last_cc1_buf, len + 2 );
                memcpy( m->last_cc1_buf->data + len, cc_block+1, 2 );
                m->last_cc1_buf->size = len + 2;
                return;
            }

            // allocate a new buffer and copy the caption data into it.
            // (we don't send it yet because we don't know what timestamp to use).
            hb_buffer_t *cc_buf = hb_buffer_init( 2 );
            if( !cc_buf )
                return;

            memcpy( cc_buf->data, cc_block+1, 2 );
            m->last_cc1_buf = cc_buf;
            break;
#ifdef notyet
        case 1:
            // CC2 stream
            //process608( cc_block+1, 2, &m->cc608 );
            break;
        case 2: //EIA-708
            // DTVCC packet data
            // Fall through
        case 3: //EIA-708
            {
                uint8_t temp[4];
                temp[0]=cc_valid;
                temp[1]=cc_type;
                temp[2]=cc_block[1];
                temp[3]=cc_block[2];
                do_708 ((const unsigned char *) temp, 4);
            }
            break;
#endif
        default:
            break;
    } 
} 

static inline int have_captions( const uint8_t *user_data, uint32_t len )
{
    return len >= 6 && 
           ( ( user_data[0] == 0x43 && user_data[1] == 0x43 ) ||
             ( user_data[0] == 0x47 && user_data[1] == 0x41 &&
               user_data[2] == 0x39 && user_data[3] == 0x34 &&
               user_data[4] == 3 && (user_data[5] & 0x40) ) );
}

static void do_one_dvd_cc( hb_libmpeg2_t *m, const uint8_t *header, int field1 )
{
    uint8_t data[3];

    data[0] = ( header[0] == 0xff && 0 == field1 )? 0x04 : 0x05;
    data[1] = header[1];
    data[2] = header[2];
    hb_mpeg2_cc( m, data );

    data[0] = ( header[3] == 0xff && 1 == field1 )? 0x04 : 0x05;
    data[1] = header[4];
    data[2] = header[5];
    hb_mpeg2_cc( m, data );
}

// extract all the captions in the current frame and send them downstream
// to the decoder.
//
// (this routine should only be called if there are captions in the current
// frame. I.e., only if a call to 'have_captions' returns true.)
static void extract_mpeg2_captions( hb_libmpeg2_t *m )
{
    const uint8_t *user_data = m->info->user_data;
    int dvd_captions = user_data[0] == 0x43;
    int capcount, field1packet = 0;
    const uint8_t *header = &user_data[4];
    if ( !dvd_captions )
    {
        // ATSC encapsulated captions - header starts one byte later
        // and has an extra unused byte following it.
        capcount = header[1] & 0x1f;
        header += 3;
    }
    else
    {
        // DVD captions
        if ( ( header[0] & 0x80 ) == 0x00 ) 
            field1packet=1; /* expect Field 1 second */
        capcount=(header[0] & 0x1e) / 2;
        header++;
    }

    int i;
    for( i=0; i<capcount; i++ )
    {
        if ( !dvd_captions )
        {
            hb_mpeg2_cc( m, header );
            header += 3;
        }
        else
        {
            do_one_dvd_cc( m, header, field1packet );
            header += 6;
        }
    }
    if ( dvd_captions )
    {
        // Deal with extra closed captions some DVDs have.
        while( header[0]==0xfe || header[0]==0xff )
        {
            do_one_dvd_cc( m, header, field1packet );
            header += 6;
        }   
    }   
}

static void next_tag( hb_libmpeg2_t *m, hb_buffer_t *buf_es )
{
    m->cur_tag = ( m->cur_tag + 1 ) & (NTAGS-1);
    if ( m->tags[m->cur_tag].start >= 0 || m->tags[m->cur_tag].cc_buf )
    {
        if ( m->tags[m->cur_tag].start < 0 ||
             ( m->got_iframe && m->tags[m->cur_tag].start >= m->first_pts ) )
            hb_log("mpeg2 tag botch: pts %lld, tag pts %lld buf 0x%p",
                   buf_es->start, m->tags[m->cur_tag].start, m->tags[m->cur_tag].cc_buf);
        if ( m->tags[m->cur_tag].cc_buf )
            hb_buffer_close( &m->tags[m->cur_tag].cc_buf );
    }
    m->tags[m->cur_tag].start = buf_es->start;
    mpeg2_tag_picture( m->libmpeg2, m->cur_tag, 0 );
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
    buf->start = -1;

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

        uint8_t *data = buf->data;
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
        uint8_t *data = buf->data;
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
        if( buf_es->start >= 0 )
        {
            next_tag( m, buf_es );
        }
        mpeg2_buffer( m->libmpeg2, buf_es->data, buf_es->data + buf_es->size );
    }

    for( ;; )
    {
        state = mpeg2_parse( m->libmpeg2 );
        if( state == STATE_BUFFER )
        {
            /* Require some more data */
            break;
        }

        // if the user requested captions, process
        // any captions found in the current frame.
        if ( m->list_subtitle && m->last_pts >= 0 &&
             have_captions( m->info->user_data, m->info->user_data_len ) )
        {
            extract_mpeg2_captions( m );
            // if we don't have a tag for the captions, make one
            if ( m->last_cc1_buf && m->tags[m->cur_tag].cc_buf != m->last_cc1_buf )
            {
                if (m->tags[m->cur_tag].cc_buf)
                {
                    hb_log("mpeg2 tag botch2: pts %lld, tag pts %lld buf 0x%p",
                           buf_es->start, m->tags[m->cur_tag].start, m->tags[m->cur_tag].cc_buf);
                    hb_buffer_close( &m->tags[m->cur_tag].cc_buf );
                }
                // see if we already made a tag for the timestamp. If so we
                // can just use it, otherwise make a new tag.
                if (m->tags[m->cur_tag].start < 0)
                {
                    next_tag( m, buf_es );
                }
                m->tags[m->cur_tag].cc_buf = m->last_cc1_buf;
            }
        }
        if( state == STATE_SEQUENCE )
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
            m->last_cc1_buf = NULL;

            if( !m->got_iframe && ( m->info->display_picture->flags &
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

                hb_buffer_t *cc_buf = NULL;
                if( m->info->display_picture->flags & PIC_FLAG_TAGS )
                {
                    int t = m->info->display_picture->tag;
                    buf->start = m->tags[t].start;
                    cc_buf = m->tags[t].cc_buf;
                    m->tags[t].start = -1;
                    m->tags[t].cc_buf = NULL;
                }
                if( buf->start < 0 && m->last_pts >= 0 )
                {
                    /* For some reason nb_fields is sometimes 1 while it
                       should be 2 */
                    buf->start = m->last_pts +
                        MAX( 2, m->info->display_picture->nb_fields ) *
                        m->info->sequence->frame_period / 600;
                }
                if ( buf->start >= 0 )
                {
                    m->last_pts = buf->start;
                }

                // if we were accumulating captions we now know the timestamp
                // so ship them to the decoder.
                if ( cc_buf )
                {
                    cc_buf->start = m->last_pts;
                    cc_send_to_decoder( m, cc_buf );
                }

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
                    hb_log( "mpeg2: \"%s\" (%d) at frame %u time %"PRId64,
                            chap_name, buf->new_chap, m->nframes, buf->start );
                }
                else if ( m->nframes == 0 )
                {
                    // this is the first frame returned by the decoder
                    m->first_pts = buf->start;
                    if ( m->job && hb_list_item( m->job->title->list_chapter,
                                                 m->job->chapter_start - 1 ) )
                    {
                        hb_chapter_t * c = hb_list_item( m->job->title->list_chapter,
                                                         m->job->chapter_start - 1 );
                        hb_log( "mpeg2: \"%s\" (%d) at frame %u time %"PRId64,
                                c->title, m->job->chapter_start, m->nframes, buf->start );
                    }
                }
                ++m->nframes;

                m->flag = m->info->display_picture->flags;

/*  Uncomment this block to see frame-by-frame picture flags, as the video encodes.
               hb_log("***** MPEG 2 Picture Info for PTS %lld *****", buf->start);
                if( m->flag & TOP_FIRST )
                    hb_log("MPEG2 Flag: Top field first");
                if( m->flag & PROGRESSIVE )
                    hb_log("MPEG2 Flag: Progressive");
                if( m->flag & COMPOSITE )
                    hb_log("MPEG2 Flag: Composite");
                if( m->flag & SKIP )
                    hb_log("MPEG2 Flag: Skip!");
                if( m->flag & TAGS )
                    hb_log("MPEG2 Flag: TAGS");
                if(fm->lag & REPEAT_FIRST )
                    hb_log("MPEG2 Flag: Repeat first field");
                if( m->flag & COMPOSITE_MASK )
                    hb_log("MPEG2 Flag: Composite mask");
                hb_log("fields: %d", m->info->display_picture->nb_fields);
*/
                /*  Rotate the cadence tracking. */
                int i = 0;
                for(i=11; i > 0; i--)
                {
                    m->cadence[i] = m->cadence[i-1];
                }

                if ( !(m->flag & PROGRESSIVE) && !(m->flag & TOP_FIRST) )
                {
                    /* Not progressive, not top first...
                       That means it's probably bottom
                       first, 2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Bottom field first, 2 fields displayed.");
                    m->cadence[0] = BT;
                }
                else if ( !(m->flag & PROGRESSIVE) && (m->flag & TOP_FIRST) )
                {
                    /* Not progressive, top is first,
                       Two fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Top field first, 2 fields displayed.");
                    m->cadence[0] = TB;
                }
                else if ( (m->flag & PROGRESSIVE) && !(m->flag & TOP_FIRST) && !( m->flag & REPEAT_FIRST )  )
                {
                    /* Progressive, but noting else.
                       That means Bottom first,
                       2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive. Bottom field first, 2 fields displayed.");
                    m->cadence[0] = BT_PROG;
                }
                else if ( (m->flag & PROGRESSIVE) && !(m->flag & TOP_FIRST) && ( m->flag & REPEAT_FIRST )  )
                {
                    /* Progressive, and repeat. .
                       That means Bottom first,
                       3 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive repeat. Bottom field first, 3 fields displayed.");
                    m->cadence[0] = BTB_PROG;
                }
                else if ( (m->flag & PROGRESSIVE) && (m->flag & TOP_FIRST) && !( m->flag & REPEAT_FIRST )  )
                {
                    /* Progressive, top first.
                       That means top first,
                       2 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive. Top field first, 2 fields displayed.");
                    m->cadence[0] = TB_PROG;
                }
                else if ( (m->flag & PROGRESSIVE) && (m->flag & TOP_FIRST) && ( m->flag & REPEAT_FIRST )  )
                {
                    /* Progressive, top, repeat.
                       That means top first,
                       3 fields displayed.
                    */
                    //hb_log("MPEG2 Flag: Progressive repeat. Top field first, 3 fields displayed.");
                    m->cadence[0] = TBT_PROG;
                }

                if ( (m->cadence[2] <= TB) && (m->cadence[1] <= TB) && (m->cadence[0] > TB) && (m->cadence[11]) )
                    hb_log("%fs: Video -> Film", (float)buf->start / 90000);
                if ( (m->cadence[2] > TB) && (m->cadence[1] <= TB) && (m->cadence[0] <= TB) && (m->cadence[11]) )
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

        /*
         * Add closed captions to the title if we are scanning (no job).
         *
         * Just because we don't add this doesn't mean that there aren't any when 
         * we scan, just that noone said anything. So you should be able to add
         * closed captions some other way (See decmpeg2Init() for alternative
         * approach of assuming that there are always CC, which is probably
         * safer - however I want to leave the autodetect in here for now to
         * see how it goes).
         */
        if( !m->job && m->title &&
            have_captions( m->info->user_data, m->info->user_data_len ) )
        {
            hb_subtitle_t *subtitle;
            int i = 0;
            
            while ( ( subtitle = hb_list_item( m->title->list_subtitle, i++ ) ) )
            {
                /*
                 * Let's call them 608 subs for now even if they aren't,
                 * since they are the only types we grok.
                 */
                if( subtitle->source == CC608SUB ) 
                {
                    break;
                }
            }
            
            if( ! subtitle )
            {
                subtitle = calloc( sizeof( hb_subtitle_t ), 1 );
                subtitle->track = 0;
                subtitle->id = 0;
                subtitle->format = TEXTSUB;
                subtitle->source = CC608SUB;
                subtitle->config.dest = PASSTHRUSUB;
                subtitle->type = 5; 
                snprintf( subtitle->lang, sizeof( subtitle->lang ), "Closed Captions");
                /*
                 * The language of the subtitles will be the same as the first audio
                 * track, i.e. the same as the video.
                 */
                hb_audio_t *audio = hb_list_item( m->title->list_audio, 0 );
                if( audio )
                {
                    snprintf( subtitle->iso639_2, sizeof( subtitle->iso639_2 ), 
                              audio->config.lang.iso639_2);
                } else {
                    snprintf( subtitle->iso639_2, sizeof( subtitle->iso639_2 ), "und");
                }
                hb_list_add( m->title->list_subtitle, subtitle );
            }
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

    int i;
    for ( i = 0; i < NTAGS; ++i )
    {
        if ( m->tags[i].cc_buf )
            hb_buffer_close( &m->tags[i].cc_buf );
    }

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

    if( job && job->title ) {
        pv->libmpeg2->title = job->title;
    }

    /*
     * If not scanning, then are we supposed to extract Closed Captions
     * and send them to the decoder? 
     */
    if( job && hb_list_count( job->list_subtitle ) > 0 )
    {
        hb_subtitle_t *subtitle;
        int i = 0;
        
        for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
        while ( ( subtitle = hb_list_item( job->list_subtitle, i++ ) ) != NULL )
        {
            if( subtitle->source == CC608SUB ) 
            {
                if ( ! pv->libmpeg2->list_subtitle )
                {
                    pv->libmpeg2->list_subtitle = hb_list_init();
                }
                hb_list_add(pv->libmpeg2->list_subtitle, subtitle);
            }
        }
    }

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

    if( w->title && pv && pv->libmpeg2 && !pv->libmpeg2->title ) {
        pv->libmpeg2->title = w->title;
    }

    // The reader found a chapter break. Remove it from the input 
    // stream. If we're reading (as opposed to scanning) start looking
    // for the next GOP start since that's where the chapter begins.
    if( (*buf_in)->new_chap )
    {
        if ( pv->libmpeg2->job )
        {
            pv->libmpeg2->look_for_break = (*buf_in)->new_chap;
        }
        (*buf_in)->new_chap = 0;
    }

    hb_libmpeg2_decode( pv->libmpeg2, *buf_in, pv->list );

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( (*buf_in)->size == 0 )
    {
        hb_list_add( pv->list, *buf_in );
        *buf_in = NULL;
        status = HB_WORK_DONE;

        /*
         * Purge any pending caption buffer then let the Closed Captions decoder
         * know that it is the end of the data.
         */
        if ( pv->libmpeg2->list_subtitle )
        {
            if ( pv->libmpeg2->last_cc1_buf )
            {
                cc_send_to_decoder( pv->libmpeg2, pv->libmpeg2->last_cc1_buf );
                pv->libmpeg2->last_cc1_buf = NULL;
            }
            cc_send_to_decoder( pv->libmpeg2, hb_buffer_init( 0 ) );
        }
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
    if ( pv->libmpeg2->last_cc1_buf )
    {
        hb_buffer_close( &pv->libmpeg2->last_cc1_buf );
    }
    hb_list_close( &pv->list );
    if ( pv->libmpeg2->list_subtitle )
    {
        hb_list_close( &pv->libmpeg2->list_subtitle );
    }
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

