/* decmpeg2.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

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
    enum AVPixelFormat   pixfmt;
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
    uint8_t              cc_tag_pending;

    struct SwsContext *sws_context; // if we have to rescale or convert color space
    int             sws_width;
    int             sws_height;
    int             sws_pix_fmt;
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
        m->tags[i].start = -2;
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
        cpy->s = cc_buf->s;
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
    if( m->tags[m->cur_tag].cc_buf == m->last_cc1_buf )
        m->last_cc1_buf = NULL;

    m->cur_tag = ( m->cur_tag + 1 ) & (NTAGS-1);
    if ( m->tags[m->cur_tag].start >= 0 || m->tags[m->cur_tag].cc_buf )
    {
        if ( m->tags[m->cur_tag].start < 0 ||
             ( m->got_iframe && m->tags[m->cur_tag].start >= m->first_pts ) )
            hb_log("mpeg2 tag botch: pts %"PRId64", tag pts %"PRId64" buf %p",
                   buf_es->s.start, m->tags[m->cur_tag].start, m->tags[m->cur_tag].cc_buf);
        if ( m->tags[m->cur_tag].cc_buf )
            hb_buffer_close( &m->tags[m->cur_tag].cc_buf );
    }
    m->tags[m->cur_tag].start = buf_es->s.start;
    mpeg2_tag_picture( m->libmpeg2, m->cur_tag, 0 );
}

static hb_buffer_t *hb_copy_frame( hb_libmpeg2_t *m )
{
    hb_job_t * job = m->job;
    int width = m->info->sequence->width;
    int height = m->info->sequence->height;
    enum AVPixelFormat pixfmt = m->pixfmt;
    uint8_t *y = m->info->display_fbuf->buf[0];
    uint8_t *u = m->info->display_fbuf->buf[1];
    uint8_t *v = m->info->display_fbuf->buf[2];
    int crop[4] = {0};

    int dst_w, dst_h;
    int src_w, src_h;

    if ( m->info->sequence->picture_width < m->info->sequence->width )
    {
        crop[3] = m->info->sequence->width - m->info->sequence->picture_width;
    }
    if ( m->info->sequence->picture_height < m->info->sequence->height )
    {
        crop[1] = m->info->sequence->height - m->info->sequence->picture_height;
    }

    src_w = width - (crop[2] + crop[3]);
    src_h = height - (crop[0] + crop[1]);
    if ( job )
    {
        dst_w = job->title->width;
        dst_h = job->title->height;
    }
    else
    {
        dst_w = src_w;
        dst_h = src_h;
    }

    hb_buffer_t *buf  = hb_video_buffer_init( dst_w, dst_h );
    buf->s.start = -1;

    AVPicture in, out, pic_crop;

    in.data[0] = y;
    in.data[1] = u;
    in.data[2] = v;
    in.linesize[0] = width;
    in.linesize[1] = width>>1;
    in.linesize[2] = width>>1;
    hb_avpicture_fill( &out, buf );

    av_picture_crop( &pic_crop, &in, pixfmt, crop[0], crop[2] );

    if ( !m->sws_context ||
         m->sws_width != src_w ||
         m->sws_height != src_h ||
         m->sws_pix_fmt != pixfmt )
    {
        // Source and Dest dimensions may be the same.  There is no speed
        // cost to using sws_scale to simply copy the data.
        m->sws_context = hb_sws_get_context( src_w, src_h, pixfmt,
                                             dst_w, dst_h, buf->f.fmt,
                                             SWS_LANCZOS|SWS_ACCURATE_RND);
        m->sws_width = src_w;
        m->sws_height = src_h;
        m->sws_pix_fmt = pixfmt;

        if ( m->sws_context == NULL )
        {
            hb_buffer_close( &buf );
            return NULL;
        }

    }

    sws_scale( m->sws_context, (const uint8_t* const *)pic_crop.data, 
               pic_crop.linesize, 0, src_h, out.data, out.linesize );

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
    mpeg2_state_t    state;
    hb_buffer_t    * buf;

    if ( buf_es->size )
    {
        /* Feed libmpeg2 */
        if( buf_es->s.start >= 0 )
        {
            next_tag( m, buf_es );
            m->cc_tag_pending = 1;
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
                // If we have not set a CC tag for the picture and
                // we have a new CC buffer, make a new tag.
                if (!m->cc_tag_pending && m->tags[m->cur_tag].cc_buf != NULL)
                {
                    next_tag( m, buf_es );
                }
                else if (m->tags[m->cur_tag].cc_buf)
                {
                    hb_log("mpeg2 tag botch2: pts %"PRId64", tag pts %"PRId64" buf %p",
                           buf_es->s.start, m->tags[m->cur_tag].start, m->tags[m->cur_tag].cc_buf);
                    hb_buffer_close( &m->tags[m->cur_tag].cc_buf );
                }
                // see if we already made a tag for the timestamp. If so we
                // can just use it, otherwise make a new tag.
                if (m->tags[m->cur_tag].start == -2)
                {
                    next_tag( m, buf_es );
                }
                m->tags[m->cur_tag].cc_buf = m->last_cc1_buf;
            }
        }
        if( state == STATE_PICTURE )
        {
            m->cc_tag_pending = 0;
        }
        else if( state == STATE_SEQUENCE )
        {
            if( !( m->width && m->height && m->rate ) )
            {
                m->width  = m->info->sequence->picture_width;
                m->height = m->info->sequence->picture_height;
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
            if ( m->info->sequence->width >> 1 == m->info->sequence->chroma_width &&
                 m->info->sequence->height >> 1 == m->info->sequence->chroma_height )
            {
                m->pixfmt = AV_PIX_FMT_YUV420P;
            }
            else
            {
                m->pixfmt = AV_PIX_FMT_YUV422P;
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
                buf  = hb_copy_frame( m );
                if ( buf == NULL )
                    continue;

                buf->sequence = buf_es->sequence;

                hb_buffer_t *cc_buf = NULL;
                if( m->info->display_picture->flags & PIC_FLAG_TAGS )
                {
                    int t = m->info->display_picture->tag;
                    buf->s.start = m->tags[t].start;
                    cc_buf = m->tags[t].cc_buf;
                    m->tags[t].start = -2;
                    m->tags[t].cc_buf = NULL;
                }
                if( buf->s.start < 0 && m->last_pts >= 0 )
                {
                    /* For some reason nb_fields is sometimes 1 while it
                       should be 2 */
                    buf->s.start = m->last_pts +
                        MAX( 2, m->info->display_picture->nb_fields ) *
                        m->info->sequence->frame_period / 600;
                }
                if ( buf->s.start >= 0 )
                {
                    m->last_pts = buf->s.start;
                }

                // if we were accumulating captions we now know the timestamp
                // so ship them to the decoder.
                if ( cc_buf )
                {
                    cc_buf->s.start = m->last_pts;
                    cc_send_to_decoder( m, cc_buf );
                }

                if( m->look_for_iframe && ( m->info->display_picture->flags &
                      PIC_MASK_CODING_TYPE ) == PIC_FLAG_CODING_TYPE_I )
                {
                    // we were waiting for an iframe to insert a chapter mark
                    // and we have one.
                    int new_chap = m->look_for_iframe;
                    buf->s.new_chap = new_chap;

                    m->look_for_iframe = 0;
                    const char *chap_name = "";
                    if ( m->job && new_chap > 0 &&
                         hb_list_item( m->job->list_chapter,
                                       new_chap - 1 ) )
                    {
                        hb_chapter_t * c = hb_list_item( 
                                                 m->job->list_chapter,
                                                 new_chap - 1 );
                        chap_name = c->title;
                    }
                    hb_log( "mpeg2: \"%s\" (%d) at frame %u time %"PRId64,
                            chap_name, new_chap, 
                            m->nframes, buf->s.start );
                }
                else if ( m->nframes == 0 )
                {
                    // this is the first frame returned by the decoder
                    m->first_pts = buf->s.start;
                    if ( m->job && hb_list_item( m->job->list_chapter,
                                                 m->job->chapter_start - 1 ) )
                    {
                        hb_chapter_t * c = hb_list_item( m->job->list_chapter,
                                                         m->job->chapter_start - 1 );
                        hb_log( "mpeg2: \"%s\" (%d) at frame %u time %"PRId64,
                                c->title, m->job->chapter_start, m->nframes, buf->s.start );
                    }
                }
                ++m->nframes;

                m->flag = m->info->display_picture->flags;

/*  Uncomment this block to see frame-by-frame picture flags, as the video encodes.
               hb_log("***** MPEG 2 Picture Info for PTS %"PRId64" *****", buf->s.start);
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
                    hb_log("%fs: Video -> Film", (float)buf->s.start / 90000);
                if ( (m->cadence[2] > TB) && (m->cadence[1] <= TB) && (m->cadence[0] <= TB) && (m->cadence[11]) )
                    hb_log("%fs: Film -> Video", (float)buf->s.start / 90000);

                buf->s.flags = m->info->display_picture->flags;

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
                subtitle->codec = WORK_DECCC608;
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
                              "%s", audio->config.lang.iso639_2);
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

    if ( m->sws_context )
    {
        sws_freeContext( m->sws_context );
    }

    int i;
    for ( i = 0; i < NTAGS; ++i )
    {
        if ( m->tags[i].cc_buf )
        {
            if ( m->tags[i].cc_buf == m->last_cc1_buf )
                m->last_cc1_buf = NULL;
            hb_buffer_close( &m->tags[i].cc_buf );
        }
    }
    if ( m->last_cc1_buf )
    {
        hb_buffer_close( &m->last_cc1_buf );
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
    hb_buffer_t * in = *buf_in;
    int status = HB_WORK_OK;

    if( w->title && pv && pv->libmpeg2 && !pv->libmpeg2->title ) {
        pv->libmpeg2->title = w->title;
    }

    // The reader found a chapter break. Remove it from the input 
    // stream. If we're reading (as opposed to scanning) start looking
    // for the next GOP start since that's where the chapter begins.
    if( in->s.new_chap )
    {
        if ( pv->libmpeg2->job )
        {
            pv->libmpeg2->look_for_break = in->s.new_chap;
        }
        in->s.new_chap = 0;
    }

    hb_libmpeg2_decode( pv->libmpeg2, in, pv->list );

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        hb_list_add( pv->list, in );
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
                if ( pv->libmpeg2->tags[pv->libmpeg2->cur_tag].cc_buf == 
                     pv->libmpeg2->last_cc1_buf )
                {
                    pv->libmpeg2->tags[pv->libmpeg2->cur_tag].cc_buf = NULL;
                }
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
    hb_list_empty( &pv->list );
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

        if( pv->libmpeg2->info->sequence->flags & SEQ_FLAG_COLOUR_DESCRIPTION )
        {
            switch( pv->libmpeg2->info->sequence->colour_primaries )
            {
                case 1: // ITU-R Recommendation 709
                    info->color_prim = HB_COLR_PRI_BT709;
                    break;
                case 5: // ITU-R Recommendation 624-4 System B, G
                    info->color_prim = HB_COLR_PRI_EBUTECH;
                    break;
                case 4: // ITU-R Recommendation 624-4 System M
                case 6: // SMPTE 170M
                case 7: // SMPTE 240M
                    info->color_prim = HB_COLR_PRI_SMPTEC;
                    break;
                default:
                    info->color_prim = HB_COLR_PRI_UNDEF;
                    break;
            }
            switch( pv->libmpeg2->info->sequence->transfer_characteristics )
            {
                case 1: // ITU-R Recommendation 709
                case 4: // ITU-R Recommendation 624-4 System M
                case 5: // ITU-R Recommendation 624-4 System B, G
                case 6: // SMPTE 170M
                    info->color_transfer = HB_COLR_TRA_BT709;
                    break;
                case 7: // SMPTE 240M
                    info->color_transfer = HB_COLR_TRA_SMPTE240M;
                    break;
                default:
                    info->color_transfer = HB_COLR_TRA_UNDEF;
                    break;
            }
            switch( pv->libmpeg2->info->sequence->matrix_coefficients )
            {
                case 1: // ITU-R Recommendation 709
                    info->color_matrix = HB_COLR_MAT_BT709;
                    break;
                case 4: // FCC
                case 5: // ITU-R Recommendation 624-4 System B, G
                case 6: // SMPTE 170M
                    info->color_matrix = HB_COLR_MAT_SMPTE170M;
                    break;
                case 7: // SMPTE 240M
                    info->color_matrix = HB_COLR_MAT_SMPTE240M;
                    break;
                default:
                    info->color_matrix = HB_COLR_MAT_UNDEF;
                    break;
            }
        }
        else if( ( info->width >= 1280 || info->height >= 720 ) ||
                 ( info->width >   720 && info->height >  576 ) )
        {
            // ITU BT.709 HD content
            info->color_prim     = HB_COLR_PRI_BT709;
            info->color_transfer = HB_COLR_TRA_BT709;
            info->color_matrix   = HB_COLR_MAT_BT709;
        }
        else if( info->rate_base == 1080000 )
        {
            // ITU BT.601 DVD or SD TV content (PAL)
            info->color_prim     = HB_COLR_PRI_EBUTECH;
            info->color_transfer = HB_COLR_TRA_BT709;
            info->color_matrix   = HB_COLR_MAT_SMPTE170M;
        }
        else
        {
            // ITU BT.601 DVD or SD TV content (NTSC)
            info->color_prim     = HB_COLR_PRI_SMPTEC;
            info->color_transfer = HB_COLR_TRA_BT709;
            info->color_matrix   = HB_COLR_MAT_SMPTE170M;
        }

        return 1;
    }
    return 0;
}

static void decmpeg2Flush( hb_work_object_t *w )
{
    hb_work_private_t * pv = w->private_data;

    mpeg2_reset( pv->libmpeg2->libmpeg2, 0 );
    pv->libmpeg2->got_iframe = 0;
}

hb_work_object_t hb_decmpeg2 =
{
    .id = WORK_DECMPEG2,
    .name = "MPEG-2 decoder (libmpeg2)",
    .init = decmpeg2Init,
    .work = decmpeg2Work,
    .close = decmpeg2Close,
    .flush = decmpeg2Flush,
    .info = decmpeg2Info
};

