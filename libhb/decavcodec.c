/* $Id: decavcodec.c,v 1.6 2005/03/06 04:08:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/* This module is Handbrake's interface to the ffmpeg decoder library
   (libavcodec & small parts of libavformat). It contains four Handbrake
   "work objects":

    decavcodec  connects HB to an ffmpeg audio decoder
    decavcodecv connects HB to an ffmpeg video decoder

        (Two different routines are needed because the ffmpeg library
        has different decoder calling conventions for audio & video.
        The audio decoder should have had its name changed to "decavcodeca"
        but I got lazy.) These work objects are self-contained & follow all
        of HB's conventions for a decoder module. They can be used like
        any other HB decoder (deca52, decmpeg2, etc.).

    decavcodecai "internal" (incestuous?) version of decavcodec
    decavcodecvi "internal" (incestuous?) version of decavcodecv

        These routine are functionally equivalent to the routines above but
        can only be used by the ffmpeg-based stream reader in libhb/stream.c.
        The reason they exist is because the ffmpeg library leaves some of
        the information needed by the decoder in the AVStream (the data
        structure used by the stream reader) and we need to retrieve it
        to successfully decode frames. But in HB the reader and decoder
        modules are in completely separate threads and nothing goes between
        them but hb_buffers containing frames to be decoded. I.e., there's
        no easy way for the ffmpeg stream reader to pass a pointer to its
        AVStream over to the ffmpeg video or audio decoder. So the *i work
        objects use a private back door to the stream reader to get access
        to the AVStream (routines hb_ffmpeg_avstream and hb_ffmpeg_context)
        and the codec_param passed to these work objects is the key to this
        back door (it's basically an index that allows the correct AVStream
        to be retrieved).

    The normal & *i objects share a lot of code (the basic frame decoding
    and bitstream info code is factored out into subroutines that can be
    called by either) but the top level routines of the *i objects
    (decavcodecviWork, decavcodecviInfo, etc.) are different because:
     1) they *have* to use the AVCodecContext that's contained in the
        reader's AVStream rather than just allocating & using their own,
     2) the Info routines have access to stuff kept in the AVStream in addition
        to stuff kept in the AVCodecContext. This shouldn't be necessary but
        crucial information like video frame rate that should be in the
        AVCodecContext is either missing or wrong in the version of ffmpeg
        we're currently using.

    A consequence of the above is that the non-i work objects *can't* use
    information from the AVStream because there isn't one - they get their
    data from either the dvd reader or the mpeg reader, not the ffmpeg stream
    reader. That means that they have to make up for deficiencies in the
    AVCodecContext info by using stuff kept in the HB "title" struct. It
    also means that ffmpeg codecs that randomly scatter state needed by
    the decoder across both the AVCodecContext & the AVStream (e.g., the
    VC1 decoder) can't easily be used by the HB mpeg stream reader.
 */

#include "hb.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

static int  decavcodecInit( hb_work_object_t *, hb_job_t * );
static int  decavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void decavcodecClose( hb_work_object_t * );
static int decavcodecInfo( hb_work_object_t *, hb_work_info_t * );
static int decavcodecBSInfo( hb_work_object_t *, const hb_buffer_t *, hb_work_info_t * );

hb_work_object_t hb_decavcodec =
{
    WORK_DECAVCODEC,
    "MPGA decoder (libavcodec)",
    decavcodecInit,
    decavcodecWork,
    decavcodecClose,
    decavcodecInfo,
    decavcodecBSInfo
};

struct hb_work_private_s
{
    hb_job_t             *job;
    AVCodecContext       *context;
    AVCodecParserContext *parser;
    hb_list_t            *list;
    double               pts_next;  // next pts we expect to generate
    int64_t              pts;       // (video) pts passing from parser to decoder
    int64_t              chap_time; // time of next chap mark (if new_chap != 0)
    int                  new_chap;
    int                  ignore_pts; // workaround M$ bugs
    int                  nframes;
    int                  ndrops;
    double               duration;  // frame duration (for video)
};



/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job   = job;

    int codec_id = w->codec_param;
    /*XXX*/
    if ( codec_id == 0 )
        codec_id = CODEC_ID_MP2;
    codec = avcodec_find_decoder( codec_id );
    pv->parser = av_parser_init( codec_id );

    pv->context = avcodec_alloc_context();
    avcodec_open( pv->context, codec );

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void decavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    if ( pv->parser )
	{
		av_parser_close(pv->parser);
	}
    if ( pv->context && pv->context->codec )
    {
        avcodec_close( pv->context );
    }
    if ( pv->list )
    {
        hb_list_close( &pv->list );
    }
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in, * buf, * last = NULL;
    int   pos, len, out_size, i, uncompressed_len;
    short buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    uint64_t cur;
    unsigned char *parser_output_buffer;
    int parser_output_buffer_len;

    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = *buf_in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = NULL;

    cur = ( in->start < 0 )? pv->pts_next : in->start;

    pos = 0;
    while( pos < in->size )
    {
        len = av_parser_parse( pv->parser, pv->context,
                               &parser_output_buffer, &parser_output_buffer_len,
                               in->data + pos, in->size - pos, cur, cur );
        out_size = 0;
        uncompressed_len = 0;
        if (parser_output_buffer_len)
        {
            out_size = sizeof(buffer);
            uncompressed_len = avcodec_decode_audio2( pv->context, buffer,
                                                      &out_size,
                                                      parser_output_buffer,
                                                      parser_output_buffer_len );
        }
        if( out_size )
        {
            short * s16;
            float * fl32;

            buf = hb_buffer_init( 2 * out_size );

            int sample_size_in_bytes = 2;   // Default to 2 bytes
            switch (pv->context->sample_fmt)
            {
              case SAMPLE_FMT_S16:
                sample_size_in_bytes = 2;
                break;
              /* We should handle other formats here - but that needs additional format conversion work below */
              /* For now we'll just report the error and try to carry on */
              default:
                hb_log("decavcodecWork - Unknown Sample Format from avcodec_decode_audio (%d) !", pv->context->sample_fmt);
                break;
            }

            buf->start = cur;
            buf->stop  = cur + 90000 * ( out_size / (sample_size_in_bytes * pv->context->channels) ) /
                         pv->context->sample_rate;
            cur = buf->stop;

            s16  = buffer;
            fl32 = (float *) buf->data;
            for( i = 0; i < out_size / 2; i++ )
            {
                fl32[i] = s16[i];
            }

            if( last )
            {
                last = last->next = buf;
            }
            else
            {
                *buf_out = last = buf;
            }
        }

        pos += len;
    }

    pv->pts_next = cur;

    return HB_WORK_OK;
}

static int decavcodecInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        AVCodecContext *context = pv->context;
        info->bitrate = context->bit_rate;
        info->rate = context->time_base.num;
        info->rate_base = context->time_base.den;
        info->profile = context->profile;
        info->level = context->level;
        return 1;
    }
    return 0;
}

static int decavcodecBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        return decavcodecInfo( w, info );
    }
    // XXX
    // We should parse the bitstream to find its parameters but for right
    // now we just return dummy values if there's a codec that will handle it.
    AVCodec *codec = avcodec_find_decoder( w->codec_param? w->codec_param :
                                                           CODEC_ID_MP2 );
    if ( codec )
    {
        static char codec_name[64];

        info->name =  strncpy( codec_name, codec->name, sizeof(codec_name)-1 );
        info->bitrate = 384000;
        info->rate = 48000;
        info->rate_base = 1;
        info->channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
        return 1;
    }
    return -1;
}

/* -------------------------------------------------------------
 * General purpose video decoder using libavcodec
 */

static uint8_t *copy_plane( uint8_t *dst, uint8_t* src, int dstride, int sstride,
                            int h )
{
    if ( dstride == sstride )
    {
        memcpy( dst, src, dstride * h );
        return dst + dstride * h;
    }
    int lbytes = dstride <= sstride? dstride : sstride;
    while ( --h >= 0 )
    {
        memcpy( dst, src, lbytes );
        src += sstride;
        dst += dstride;
    }
    return dst;
}

/* Note: assumes frame format is PIX_FMT_YUV420P */
static hb_buffer_t *copy_frame( AVCodecContext *context, AVFrame *frame )
{
    int w = context->width, h = context->height;
    hb_buffer_t *buf = hb_buffer_init( w * h * 3 / 2 );
    uint8_t *dst = buf->data;

    dst = copy_plane( dst, frame->data[0], w, frame->linesize[0], h );
    w >>= 1; h >>= 1;
    dst = copy_plane( dst, frame->data[1], w, frame->linesize[1], h );
    dst = copy_plane( dst, frame->data[2], w, frame->linesize[2], h );

    return buf;
}

static int get_frame_buf( AVCodecContext *context, AVFrame *frame )
{
    hb_work_private_t *pv = context->opaque;
    frame->pts = pv->pts;
    pv->pts = -1;

    return avcodec_default_get_buffer( context, frame );
}

static void log_chapter( hb_work_private_t *pv, int chap_num, int64_t pts )
{
    hb_chapter_t *c = hb_list_item( pv->job->title->list_chapter, chap_num - 1 );
    hb_log( "%s: \"%s\" (%d) at frame %u time %lld", pv->context->codec->name,
            c->title, chap_num, pv->nframes, pts );
}

static int decodeFrame( hb_work_private_t *pv, uint8_t *data, int size )
{
    int got_picture;
    AVFrame frame;

    avcodec_decode_video( pv->context, &frame, &got_picture, data, size );
    if( got_picture )
    {
        // ffmpeg makes it hard to attach a pts to a frame. if the MPEG ES
        // packet had a pts we handed it to av_parser_parse (if the packet had
        // no pts we set it to -1 but before the parse we can't distinguish between
        // the start of a video frame with no pts & an intermediate packet of
        // some frame which never has a pts). we hope that when parse returns
        // the frame to us the pts we originally handed it will be in parser->pts.
        // we put this pts into pv->pts so that when a avcodec_decode_video
        // finally gets around to allocating an AVFrame to hold the decoded
        // frame we can stuff that pts into the frame. if all of these relays
        // worked at this point frame.pts should hold the frame's pts from the
        // original data stream or -1 if it didn't have one. in the latter case
        // we generate the next pts in sequence for it.
        double pts = frame.pts;
        if ( pts < 0 )
        {
            pts = pv->pts_next;
        }
        if ( pv->duration == 0 )
        {
            pv->duration = 90000. * pv->context->time_base.num /
                           pv->context->time_base.den;
        }
        double frame_dur = pv->duration;
        frame_dur += frame.repeat_pict * frame_dur * 0.5;
        pv->pts_next = pts + frame_dur;

        hb_buffer_t *buf = copy_frame( pv->context, &frame );
        buf->start = pts;

        if ( pv->new_chap && buf->start >= pv->chap_time )
        {
            buf->new_chap = pv->new_chap;
            pv->new_chap = 0;
            pv->chap_time = 0;
            if ( pv->job )
            {
                log_chapter( pv, buf->new_chap, buf->start );
            }
        }
        else if ( pv->job && pv->nframes == 0 )
        {
            log_chapter( pv, pv->job->chapter_start, buf->start );
        }
        hb_list_add( pv->list, buf );
        ++pv->nframes;
    }
    return got_picture;
}

static void decodeVideo( hb_work_private_t *pv, uint8_t *data, int size,
                         int64_t pts, int64_t dts )
{
    /*
     * The following loop is a do..while because we need to handle both
     * data & the flush at the end (signaled by size=0). At the end there's
     * generally a frame in the parser & one or more frames in the decoder
     * (depending on the bframes setting).
     */
    int pos = 0;
    do {
        uint8_t *pout;
        int pout_len;
        int len = av_parser_parse( pv->parser, pv->context, &pout, &pout_len,
                                   data + pos, size - pos, pts, dts );
        pos += len;

        if ( pout_len > 0 )
        {
            pv->pts = pv->parser->pts;
            decodeFrame( pv, pout, pout_len );
        }
    } while ( pos < size );

    /* the stuff above flushed the parser, now flush the decoder */
    while ( size == 0 && decodeFrame( pv, NULL, 0 ) )
    {
    }
}

static hb_buffer_t *link_buf_list( hb_work_private_t *pv )
{
    hb_buffer_t *head = hb_list_item( pv->list, 0 );

    if ( head )
    {
        hb_list_rem( pv->list, head );

        hb_buffer_t *last = head, *buf;

        while ( ( buf = hb_list_item( pv->list, 0 ) ) != NULL )
        {
            hb_list_rem( pv->list, buf );
            last->next = buf;
            last = buf;
        }
    }
    return head;
}


static int decavcodecvInit( hb_work_object_t * w, hb_job_t * job )
{

    hb_work_private_t *pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job   = job;
    pv->list = hb_list_init();

    int codec_id = w->codec_param;
    pv->parser = av_parser_init( codec_id );
    pv->context = avcodec_alloc_context2( CODEC_TYPE_VIDEO );

    /* we have to wrap ffmpeg's get_buffer to be able to set the pts (?!) */
    pv->context->opaque = pv;
    pv->context->get_buffer = get_frame_buf;

    AVCodec *codec = avcodec_find_decoder( codec_id );

    // we can't call the avstream funcs but the read_header func in the
    // AVInputFormat may set up some state in the AVContext. In particular 
    // vc1t_read_header allocates 'extradata' to deal with header issues
    // related to Microsoft's bizarre engineering notions. We alloc a chunk
    // of space to make vc1 work then associate the codec with the context.
    pv->context->extradata_size = 32;
    pv->context->extradata = av_malloc(pv->context->extradata_size);
    avcodec_open( pv->context, codec );

    return 0;
}

static int decavcodecvWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    int64_t pts = -1;
    int64_t dts = pts;

    *buf_in = NULL;

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        decodeVideo( pv, in->data, in->size, pts, dts );
        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        hb_log( "%s done: %d frames", pv->context->codec->name, pv->nframes );
        return HB_WORK_DONE;
    }

    if( in->start >= 0 )
    {
        pts = in->start;
        dts = in->renderOffset;
    }
    if ( in->new_chap )
    {
        pv->new_chap = in->new_chap;
        pv->chap_time = pts >= 0? pts : pv->pts_next;
    }
    decodeVideo( pv, in->data, in->size, pts, dts );
    hb_buffer_close( &in );
    *buf_out = link_buf_list( pv );
    return HB_WORK_OK;
}

static int decavcodecvInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        AVCodecContext *context = pv->context;
        info->bitrate = context->bit_rate;
        info->width = context->width;
        info->height = context->height;

        /* ffmpeg gives the frame rate in frames per second while HB wants
         * it in units of the 27MHz MPEG clock. */
        info->rate = 27000000;
        info->rate_base = (int64_t)context->time_base.num * 27000000LL /
                          context->time_base.den;
        
        /* Sometimes there's no pixel aspect set in the source. In that case,
           assume a 1:1 PAR. Otherwise, preserve the source PAR.             */
        info->pixel_aspect_width = context->sample_aspect_ratio.num ?
                                        context->sample_aspect_ratio.num : 1;
        info->pixel_aspect_height = context->sample_aspect_ratio.den ?
                                        context->sample_aspect_ratio.den : 1;

        /* ffmpeg returns the Pixel Aspect Ratio (PAR). Handbrake wants the
         * Display Aspect Ratio so we convert by scaling by the Storage
         * Aspect Ratio (w/h). We do the calc in floating point to get the
         * rounding right. */
        info->aspect = (double)info->pixel_aspect_width * 
                       (double)context->width /
                       (double)info->pixel_aspect_height /
                       (double)context->height;

        info->profile = context->profile;
        info->level = context->level;
        info->name = context->codec->name;
        return 1;
    }
    return 0;
}

static int decavcodecvBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    return 0;
}

hb_work_object_t hb_decavcodecv =
{
    WORK_DECAVCODECV,
    "Video decoder (libavcodec)",
    decavcodecvInit,
    decavcodecvWork,
    decavcodecClose,
    decavcodecvInfo,
    decavcodecvBSInfo
};


// This is a special decoder for ffmpeg streams. The ffmpeg stream reader
// includes a parser and passes information from the parser to the decoder
// via a codec context kept in the AVStream of the reader's AVFormatContext.
// We *have* to use that codec context to decode the stream or we'll get
// garbage. ffmpeg_title_scan put a cookie that can be used to get to that
// codec context in our codec_param.

// this routine gets the appropriate context pointer from the ffmpeg
// stream reader. it can't be called until we get the first buffer because
// we can't guarantee that reader will be called before the our init
// routine and if our init is called first we'll get a pointer to the
// old scan stream (which has already been closed).
static void init_ffmpeg_context( hb_work_object_t *w )
{
    hb_work_private_t *pv = w->private_data;
    pv->context = hb_ffmpeg_context( w->codec_param );

    // during scan the decoder gets closed & reopened which will
    // close the codec so reopen it if it's not there
    if ( ! pv->context->codec )
    {
        AVCodec *codec = avcodec_find_decoder( pv->context->codec_id );
        avcodec_open( pv->context, codec );
    }
    // set up our best guess at the frame duration.
    // the frame rate in the codec is usually bogus but it's sometimes
    // ok in the stream.
    AVStream *st = hb_ffmpeg_avstream( w->codec_param );
    AVRational tb;
    // XXX because the time bases are so screwed up, we only take values
    // in the range 8fps - 64fps.
    if ( st->time_base.num * 64 > st->time_base.den &&
         st->time_base.den > st->time_base.num * 8 )
    {
        tb = st->time_base;
    }
    else if ( st->codec->time_base.num * 64 > st->codec->time_base.den &&
              st->codec->time_base.den > st->codec->time_base.num * 8 )
    {
        tb = st->codec->time_base;
    }
    else if ( st->r_frame_rate.den * 64 > st->r_frame_rate.num &&
              st->r_frame_rate.num > st->r_frame_rate.den * 8 )
    {
        tb.num = st->r_frame_rate.den;
        tb.den = st->r_frame_rate.num;
    }
    else
    {
        tb.num = 1001;  /*XXX*/
        tb.den = 30000; /*XXX*/
    }
    pv->duration = 90000. * tb.num / tb.den;

    // we have to wrap ffmpeg's get_buffer to be able to set the pts (?!)
    pv->context->opaque = pv;
    pv->context->get_buffer = get_frame_buf;
}

static void prepare_ffmpeg_buffer( hb_buffer_t * in )
{
    // ffmpeg requires an extra 8 bytes of zero at the end of the buffer and
    // will seg fault in odd, data dependent ways if it's not there. (my guess
    // is this is a case of a local performance optimization creating a global
    // performance degradation since all the time wasted by extraneous data
    // copies & memory zeroing has to be huge compared to the minor reduction
    // in inner-loop instructions this affords - modern cpus bottleneck on
    // memory bandwidth not instruction bandwidth).
    if ( in->size + FF_INPUT_BUFFER_PADDING_SIZE > in->alloc )
    {
        // have to realloc to add the padding
        hb_buffer_realloc( in, in->size + FF_INPUT_BUFFER_PADDING_SIZE );
    }
    memset( in->data + in->size, 0, FF_INPUT_BUFFER_PADDING_SIZE );
}

static int decavcodecviInit( hb_work_object_t * w, hb_job_t * job )
{

    hb_work_private_t *pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job   = job;
    pv->list = hb_list_init();

    return 0;
}

static int decavcodecviWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    if ( ! pv->context )
    {
        init_ffmpeg_context( w );

        switch ( pv->context->codec_id )
        {
            // These are the only formats whose timestamps we'll believe.
            // All others are treated as CFR (i.e., we take the first timestamp
            // then generate all the others from the frame rate). The reason for
            // this is that the M$ encoders are so frigging buggy with garbage
            // like packed b-frames (vfw divx mpeg4) that believing their timestamps
            // results in discarding more than half the video frames because they'll
            // be out of sequence (and attempting to reseqence them doesn't work
            // because it's the timestamps that are wrong, not the decoded frame
            // order). All hail Redmond, ancestral home of the rich & stupid.
            case CODEC_ID_MPEG2VIDEO:
            case CODEC_ID_RAWVIDEO:
            case CODEC_ID_H264:
            case CODEC_ID_VC1:
                break;

            default:
                pv->ignore_pts = 1;
                break;
        }
    }
    hb_buffer_t *in = *buf_in;
    int64_t pts = -1;

    *buf_in = NULL;

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        /* flush any frames left in the decoder */
        while ( decodeFrame( pv, NULL, 0 ) )
        {
        }
        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        hb_log( "%s done: %d frames %d drops", pv->context->codec->name,
                pv->nframes, pv->ndrops );
        return HB_WORK_DONE;
    }

    if( in->start >= 0 )
    {
        // use the first timestamp as our 'next expected' pts
        if ( pv->pts_next <= 0 )
        {
            pv->pts_next = in->start;
        }

        if ( ! pv->ignore_pts )
        {
            pts = in->start;
            if ( pv->pts > 0 )
            {
                hb_log( "overwriting pts %lld with %lld (diff %d)",
                        pv->pts, pts, pts - pv->pts );
            }
            if ( pv->pts_next - pts >= pv->duration )
            {
                // this frame starts more than a frame time before where
                // the nominal frame rate says it should - drop it.
                // log the first 10 drops so we'll know what's going on.
                if ( pv->ndrops++ < 10 )
                {
                    hb_log( "time reversal next %.0f pts %lld (diff %g)",
                            pv->pts_next, pts, pv->pts_next - pts );
                }
                hb_buffer_close( &in );
                return HB_WORK_OK;
            }
            pv->pts = pts;
        }
    }

    if ( in->new_chap )
    {
        pv->new_chap = in->new_chap;
        pv->chap_time = pts >= 0? pts : pv->pts_next;
    }
    prepare_ffmpeg_buffer( in );
    decodeFrame( pv, in->data, in->size );
    hb_buffer_close( &in );
    *buf_out = link_buf_list( pv );
    return HB_WORK_OK;
}

static int decavcodecviInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    if ( decavcodecvInfo( w, info ) )
    {
        // There are at least three different video frame rates in ffmpeg:
        //  - time_base in the AVStream
        //  - time_base in the AVCodecContext
        //  - r_frame_rate in the AVStream
        // There's no guidence on which if any of these to believe but the
        // routine compute_frame_duration tries the stream first then the codec.
        // In general the codec time base seems bogus & the stream time base is
        // ok except for wmv's where the stream time base is also bogus but
        // r_frame_rate is sometimes ok & sometimes a random number.
        AVStream *st = hb_ffmpeg_avstream( w->codec_param );
        AVRational tb;
        // XXX because the time bases are so screwed up, we only take values
        // in the range 8fps - 64fps.
        if ( st->time_base.num * 64 > st->time_base.den &&
             st->time_base.den > st->time_base.num * 8 )
        {
            tb = st->time_base;
        }
        else if ( st->codec->time_base.num * 64 > st->codec->time_base.den &&
                  st->codec->time_base.den > st->codec->time_base.num * 8 )
        {
            tb = st->codec->time_base;
        }
        else if ( st->r_frame_rate.den * 64 > st->r_frame_rate.num &&
                  st->r_frame_rate.num > st->r_frame_rate.den * 8 )
        {
            tb.num = st->r_frame_rate.den;
            tb.den = st->r_frame_rate.num;
        }
        else
        {
            tb.num = 1001;  /*XXX*/
            tb.den = 30000; /*XXX*/
        }

        // ffmpeg gives the frame rate in frames per second while HB wants
        // it in units of the 27MHz MPEG clock. */
        info->rate = 27000000;
        info->rate_base = (int64_t)tb.num * 27000000LL / tb.den;
        return 1;
    }
    return 0;
}

static void decodeAudio( hb_work_private_t *pv, uint8_t *data, int size )
{
    AVCodecContext *context = pv->context;
    int pos = 0;

    while ( pos < size )
    {
        int16_t buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
        int out_size = sizeof(buffer);
        int len = avcodec_decode_audio2( context, buffer, &out_size,
                                         data + pos, size - pos );
        if ( len <= 0 )
        {
            return;
        }
        pos += len;
        if( out_size > 0 )
        {
            hb_buffer_t *buf = hb_buffer_init( 2 * out_size );

            double pts = pv->pts_next;
            buf->start = pts;
            out_size >>= 1;
            pts += out_size * pv->duration;
            buf->stop  = pts;
            pv->pts_next = pts;

            float *fl32 = (float *)buf->data;
            int i;
            for( i = 0; i < out_size; ++i )
            {
                fl32[i] = buffer[i];
            }
            hb_list_add( pv->list, buf );
        }
    }
}

static int decavcodecaiWork( hb_work_object_t *w, hb_buffer_t **buf_in,
                    hb_buffer_t **buf_out )
{
    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = *buf_in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    hb_work_private_t *pv = w->private_data;
    if ( ! pv->context )
    {
        init_ffmpeg_context( w );
        pv->duration = 90000. /
                    (double)( pv->context->sample_rate * pv->context->channels );
    }
    hb_buffer_t *in = *buf_in;

    if ( in->start >= 0 &&
         ( pv->pts_next < 0 || ( in->start - pv->pts_next ) > 90*100 ) )
    {
        pv->pts_next = in->start;
    }
    prepare_ffmpeg_buffer( in );
    decodeAudio( pv, in->data, in->size );
    *buf_out = link_buf_list( pv );

    return HB_WORK_OK;
}

hb_work_object_t hb_decavcodecvi =
{
    WORK_DECAVCODECVI,
    "Video decoder (ffmpeg streams)",
    decavcodecviInit,
    decavcodecviWork,
    decavcodecClose,
    decavcodecviInfo,
    decavcodecvBSInfo
};

hb_work_object_t hb_decavcodecai =
{
    WORK_DECAVCODECAI,
    "Audio decoder (ffmpeg streams)",
    decavcodecviInit,
    decavcodecaiWork,
    decavcodecClose,
    decavcodecInfo,
    decavcodecBSInfo
};
