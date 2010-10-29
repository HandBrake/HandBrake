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
#include "hbffmpeg.h"
#include "downmix.h"
#include "libavcodec/audioconvert.h"

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

#define HEAP_SIZE 8
typedef struct {
    // there are nheap items on the heap indexed 1..nheap (i.e., top of
    // heap is 1). The 0th slot is unused - a marker is put there to check
    // for overwrite errs.
    int64_t h[HEAP_SIZE+1];
    int     nheap;
} pts_heap_t;

struct hb_work_private_s
{
    hb_job_t        *job;
    AVCodecContext  *context;
    AVCodecParserContext *parser;
    hb_list_t       *list;
    double          duration;   // frame duration (for video)
    double          pts_next;   // next pts we expect to generate
    int64_t         pts;        // (video) pts passing from parser to decoder
    int64_t         chap_time;  // time of next chap mark (if new_chap != 0)
    int             new_chap;   // output chapter mark pending
    uint32_t        nframes;
    uint32_t        ndrops;
    uint32_t        decode_errors;
    int             brokenByMicrosoft; // video stream may contain packed b-frames
    hb_buffer_t*    delayq[HEAP_SIZE];
    pts_heap_t      pts_heap;
    void*           buffer;
    struct SwsContext *sws_context; // if we have to rescale or convert color space
    hb_downmix_t    *downmix;
    hb_sample_t     *downmix_buffer;
    int cadence[12];
    hb_chan_map_t   *out_map;
};

static void decodeAudio( hb_audio_t * audio, hb_work_private_t *pv, uint8_t *data, int size );
static hb_buffer_t *link_buf_list( hb_work_private_t *pv );


static int64_t heap_pop( pts_heap_t *heap )
{
    int64_t result;

    if ( heap->nheap <= 0 )
    {
        return -1;
    }

    // return the top of the heap then put the bottom element on top,
    // decrease the heap size by one & rebalence the heap.
    result = heap->h[1];

    int64_t v = heap->h[heap->nheap--];
    int parent = 1;
    int child = parent << 1;
    while ( child <= heap->nheap )
    {
        // find the smallest of the two children of parent
        if (child < heap->nheap && heap->h[child] > heap->h[child+1] )
            ++child;

        if (v <= heap->h[child])
            // new item is smaller than either child so it's the new parent.
            break;

        // smallest child is smaller than new item so move it up then
        // check its children.
        int64_t hp = heap->h[child];
        heap->h[parent] = hp;
        parent = child;
        child = parent << 1;
    }
    heap->h[parent] = v;
    return result;
}

static void heap_push( pts_heap_t *heap, int64_t v )
{
    if ( heap->nheap < HEAP_SIZE )
    {
        ++heap->nheap;
    }

    // stick the new value on the bottom of the heap then bubble it
    // up to its correct spot.
	int child = heap->nheap;
	while (child > 1) {
		int parent = child >> 1;
		if (heap->h[parent] <= v)
			break;
		// move parent down
		int64_t hp = heap->h[parent];
		heap->h[child] = hp;
		child = parent;
	}
	heap->h[child] = v;
}


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
    pv->list  = hb_list_init();

    int codec_id = w->codec_param;
    /*XXX*/
    if ( codec_id == 0 )
        codec_id = CODEC_ID_MP2;

    codec = avcodec_find_decoder( codec_id );
    pv->parser = av_parser_init( codec_id );

    pv->context = avcodec_alloc_context();
    hb_avcodec_open( pv->context, codec );

    if ( w->audio != NULL )
    {
        if ( w->audio->config.out.codec == HB_ACODEC_AC3 )
        {
            // ffmpegs audio encoder expect an smpte chan map as input.
            // So we need to map the decoders output to smpte.
            pv->out_map = &hb_smpte_chan_map;
        }
        else
        {
            pv->out_map = &hb_qt_chan_map;
        }
        if ( hb_need_downmix( w->audio->config.in.channel_layout, 
                              w->audio->config.out.mixdown) )
        {
            pv->downmix = hb_downmix_init(w->audio->config.in.channel_layout, 
                                          w->audio->config.out.mixdown);
            hb_downmix_set_chan_map( pv->downmix, &hb_smpte_chan_map, pv->out_map );
        }
    }

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

    if ( pv )
    {
        if ( pv->job && pv->context && pv->context->codec )
        {
            hb_log( "%s-decoder done: %u frames, %u decoder errors, %u drops",
                    pv->context->codec->name, pv->nframes, pv->decode_errors,
                    pv->ndrops );
        }
        if ( pv->sws_context )
        {
            sws_freeContext( pv->sws_context );
        }
        if ( pv->parser )
        {
            av_parser_close(pv->parser);
        }
        if ( pv->context && pv->context->codec )
        {
            hb_avcodec_close( pv->context );
        }
        if ( pv->list )
        {
            hb_list_close( &pv->list );
        }
        if ( pv->buffer )
        {
            av_free( pv->buffer );
            pv->buffer = NULL;
        }
        if ( pv->downmix )
        {
            hb_downmix_close( &(pv->downmix) );
        }
        if ( pv->downmix_buffer )
        {
            free( pv->downmix_buffer );
            pv->downmix_buffer = NULL;
        }
        free( pv );
        w->private_data = NULL;
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
    hb_buffer_t * in = *buf_in;

    if ( in->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = NULL;

    if ( in->start < -1 && pv->pts_next <= 0 )
    {
        // discard buffers that start before video time 0
        return HB_WORK_OK;
    }

    // if the packet has a timestamp use it 
    if ( in->start != -1 )
    {
        pv->pts_next = in->start;
    }

    int pos, len;
    for ( pos = 0; pos < in->size; pos += len )
    {
        uint8_t *parser_output_buffer;
        int parser_output_buffer_len;
        int64_t cur = pv->pts_next;

        if ( pv->parser != NULL )
        {
            len = av_parser_parse2( pv->parser, pv->context,
                    &parser_output_buffer, &parser_output_buffer_len,
                    in->data + pos, in->size - pos, cur, cur, AV_NOPTS_VALUE );
        }
        else
        {
            parser_output_buffer = in->data;
            len = parser_output_buffer_len = in->size;
        }
        if (parser_output_buffer_len)
        {
            // set the duration on every frame since the stream format can
            // change (it shouldn't but there's no way to guarantee it).
            // duration is a scaling factor to go from #bytes in the decoded
            // frame to frame time (in 90KHz mpeg ticks). 'channels' converts
            // total samples to per-channel samples. 'sample_rate' converts
            // per-channel samples to seconds per sample and the 90000
            // is mpeg ticks per second.
            if ( pv->context->sample_rate && pv->context->channels )
            {
                pv->duration = 90000. /
                            (double)( pv->context->sample_rate * pv->context->channels );
            }
            decodeAudio( w->audio, pv, parser_output_buffer, parser_output_buffer_len );
        }
    }
    *buf_out = link_buf_list( pv );
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
    int ret = 0;

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
    if ( ! codec )
    {
        // there's no ffmpeg codec for this audio type - give up
        return -1;
    }

    static char codec_name[64];
    info->name =  strncpy( codec_name, codec->name, sizeof(codec_name)-1 );

    AVCodecParserContext *parser = av_parser_init( codec->id );
    AVCodecContext *context = avcodec_alloc_context();
    hb_avcodec_open( context, codec );
    uint8_t *buffer = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
    int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    unsigned char *pbuffer;
    int pos, pbuffer_size;

    while ( buf && !ret )
    {
        pos = 0;
        while ( pos < buf->size )
        {
            int len;

            if (parser != NULL )
            {
                len = av_parser_parse2( parser, context, &pbuffer, 
                                        &pbuffer_size, buf->data + pos, 
                                        buf->size - pos, buf->start, 
                                        buf->start, AV_NOPTS_VALUE );
            }
            else
            {
                pbuffer = buf->data;
                len = pbuffer_size = buf->size;
            }
            pos += len;
            if ( pbuffer_size > 0 )
            {
                AVPacket avp;
                av_init_packet( &avp );
                avp.data = pbuffer;
                avp.size = pbuffer_size;

                len = avcodec_decode_audio3( context, (int16_t*)buffer, 
                                             &out_size, &avp );
                if ( len > 0 && context->sample_rate > 0 )
                {
                    info->bitrate = context->bit_rate;
                    info->rate = context->sample_rate;
                    info->rate_base = 1;
                    info->channel_layout = 
                        hb_ff_layout_xlat(context->channel_layout, 
                                          context->channels);
                    ret = 1;
                    break;
                }
            }
        }
        buf = buf->next;
    }

    av_free( buffer );
    if ( parser != NULL )
        av_parser_close( parser );
    hb_avcodec_close( context );
    return ret;
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

// copy one video frame into an HB buf. If the frame isn't in our color space
// or at least one of its dimensions is odd, use sws_scale to convert/rescale it.
// Otherwise just copy the bits.
static hb_buffer_t *copy_frame( hb_work_private_t *pv, AVFrame *frame )
{
    AVCodecContext *context = pv->context;
    int w, h;
    if ( ! pv->job )
    {
        // if the dimensions are odd, drop the lsb since h264 requires that
        // both width and height be even.
        w = ( context->width >> 1 ) << 1;
        h = ( context->height >> 1 ) << 1;
    }
    else
    {
        w =  pv->job->title->width;
        h =  pv->job->title->height;
    }
    hb_buffer_t *buf = hb_video_buffer_init( w, h );
    uint8_t *dst = buf->data;

    if ( context->pix_fmt != PIX_FMT_YUV420P || w != context->width ||
         h != context->height )
    {
        // have to convert to our internal color space and/or rescale
        AVPicture dstpic;
        avpicture_fill( &dstpic, dst, PIX_FMT_YUV420P, w, h );

        if ( ! pv->sws_context )
        {
            pv->sws_context = hb_sws_get_context( context->width, context->height, context->pix_fmt,
                                              w, h, PIX_FMT_YUV420P,
                                              SWS_LANCZOS|SWS_ACCURATE_RND);
        }
        sws_scale( pv->sws_context, frame->data, frame->linesize, 0, h,
                   dstpic.data, dstpic.linesize );
    }
    else
    {
        dst = copy_plane( dst, frame->data[0], w, frame->linesize[0], h );
        w = (w + 1) >> 1; h = (h + 1) >> 1;
        dst = copy_plane( dst, frame->data[1], w, frame->linesize[1], h );
        dst = copy_plane( dst, frame->data[2], w, frame->linesize[2], h );
    }
    return buf;
}

static int get_frame_buf( AVCodecContext *context, AVFrame *frame )
{
    hb_work_private_t *pv = context->opaque;
    frame->pts = pv->pts;
    pv->pts = -1;
    return avcodec_default_get_buffer( context, frame );
}

static int reget_frame_buf( AVCodecContext *context, AVFrame *frame )
{
    hb_work_private_t *pv = context->opaque;
    frame->pts = pv->pts;
    pv->pts = -1;
    return avcodec_default_reget_buffer( context, frame );
}

static void log_chapter( hb_work_private_t *pv, int chap_num, int64_t pts )
{
    hb_chapter_t *c;

    if ( !pv->job )
        return;

    c = hb_list_item( pv->job->title->list_chapter, chap_num - 1 );
    if ( c && c->title )
    {
        hb_log( "%s: \"%s\" (%d) at frame %u time %"PRId64,
                pv->context->codec->name, c->title, chap_num, pv->nframes, pts );
    }
    else
    {
        hb_log( "%s: Chapter %d at frame %u time %"PRId64,
                pv->context->codec->name, chap_num, pv->nframes, pts );
    }
}

static void flushDelayQueue( hb_work_private_t *pv )
{
    hb_buffer_t *buf;
    int slot = pv->nframes & (HEAP_SIZE-1);

    // flush all the video packets left on our timestamp-reordering delay q
    while ( ( buf = pv->delayq[slot] ) != NULL )
    {
        buf->start = heap_pop( &pv->pts_heap );
        hb_list_add( pv->list, buf );
        pv->delayq[slot] = NULL;
        slot = ( slot + 1 ) & (HEAP_SIZE-1);
    }
}

#define TOP_FIRST PIC_FLAG_TOP_FIELD_FIRST
#define PROGRESSIVE PIC_FLAG_PROGRESSIVE_FRAME
#define REPEAT_FIRST PIC_FLAG_REPEAT_FIRST_FIELD
#define TB 8
#define BT 16
#define BT_PROG 32
#define BTB_PROG 64
#define TB_PROG 128
#define TBT_PROG 256

static void checkCadence( int * cadence, uint16_t flags, int64_t start )
{
    /*  Rotate the cadence tracking. */
    int i = 0;
    for(i=11; i > 0; i--)
    {
        cadence[i] = cadence[i-1];
    }

    if ( !(flags & PROGRESSIVE) && !(flags & TOP_FIRST) )
    {
        /* Not progressive, not top first...
           That means it's probably bottom
           first, 2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Bottom field first, 2 fields displayed.");
        cadence[0] = BT;
    }
    else if ( !(flags & PROGRESSIVE) && (flags & TOP_FIRST) )
    {
        /* Not progressive, top is first,
           Two fields displayed.
        */
        //hb_log("MPEG2 Flag: Top field first, 2 fields displayed.");
        cadence[0] = TB;
    }
    else if ( (flags & PROGRESSIVE) && !(flags & TOP_FIRST) && !( flags & REPEAT_FIRST )  )
    {
        /* Progressive, but noting else.
           That means Bottom first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Bottom field first, 2 fields displayed.");
        cadence[0] = BT_PROG;
    }
    else if ( (flags & PROGRESSIVE) && !(flags & TOP_FIRST) && ( flags & REPEAT_FIRST )  )
    {
        /* Progressive, and repeat. .
           That means Bottom first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Bottom field first, 3 fields displayed.");
        cadence[0] = BTB_PROG;
    }
    else if ( (flags & PROGRESSIVE) && (flags & TOP_FIRST) && !( flags & REPEAT_FIRST )  )
    {
        /* Progressive, top first.
           That means top first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Top field first, 2 fields displayed.");
        cadence[0] = TB_PROG;
    }
    else if ( (flags & PROGRESSIVE) && (flags & TOP_FIRST) && ( flags & REPEAT_FIRST )  )
    {
        /* Progressive, top, repeat.
           That means top first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Top field first, 3 fields displayed.");
        cadence[0] = TBT_PROG;
    }

    if ( (cadence[2] <= TB) && (cadence[1] <= TB) && (cadence[0] > TB) && (cadence[11]) )
        hb_log("%fs: Video -> Film", (float)start / 90000);
    if ( (cadence[2] > TB) && (cadence[1] <= TB) && (cadence[0] <= TB) && (cadence[11]) )
        hb_log("%fs: Film -> Video", (float)start / 90000);
}

/*
 * Decodes a video frame from the specified raw packet data ('data', 'size', 'sequence').
 * The output of this function is stored in 'pv->list', which contains a list
 * of zero or more decoded packets.
 * 
 * The returned packets are guaranteed to have their timestamps in the correct order,
 * even if the original packets decoded by libavcodec have misordered timestamps,
 * due to the use of 'packed B-frames'.
 * 
 * Internally the set of decoded packets may be buffered in 'pv->delayq'
 * until enough packets have been decoded so that the timestamps can be
 * correctly rewritten, if this is necessary.
 */
static int decodeFrame( hb_work_private_t *pv, uint8_t *data, int size, int sequence )
{
    int got_picture, oldlevel = 0;
    AVFrame frame;
    AVPacket avp;

    if ( global_verbosity_level <= 1 )
    {
        oldlevel = av_log_get_level();
        av_log_set_level( AV_LOG_QUIET );
    }

    av_init_packet( &avp );
    avp.data = data;
    avp.size = size;
    if ( avcodec_decode_video2( pv->context, &frame, &got_picture, &avp ) < 0 )
    {
        ++pv->decode_errors;     
    }
    if ( global_verbosity_level <= 1 )
    {
        av_log_set_level( oldlevel );
    }
    if( got_picture )
    {
        uint16_t flags = 0;

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
        double frame_dur = pv->duration;
        if ( frame_dur <= 0 )
        {
            frame_dur = 90000. * (double)pv->context->time_base.num /
                        (double)pv->context->time_base.den;
            pv->duration = frame_dur;
        }
        if ( pv->context->ticks_per_frame > 1 )
        {
            frame_dur *= 2;
        }
        if ( frame.repeat_pict )
        {
            frame_dur += frame.repeat_pict * pv->duration;
        }
        // XXX Unlike every other video decoder, the Raw decoder doesn't
        //     use the standard buffer allocation routines so we never
        //     get to put a PTS in the frame. Do it now.
        if ( pv->context->codec_id == CODEC_ID_RAWVIDEO )
        {
            frame.pts = pv->pts;
            pv->pts = -1;
        }
        // If there was no pts for this frame, assume constant frame rate
        // video & estimate the next frame time from the last & duration.
        double pts = frame.pts;
        if ( pts < 0 )
        {
            pts = pv->pts_next;
        }
        pv->pts_next = pts + frame_dur;

        if ( frame.top_field_first )
        {
            flags |= PIC_FLAG_TOP_FIELD_FIRST;
        }
        if ( !frame.interlaced_frame )
        {
            flags |= PIC_FLAG_PROGRESSIVE_FRAME;
        }
        if ( frame.repeat_pict )
        {
            flags |= PIC_FLAG_REPEAT_FIRST_FIELD;
        }

        hb_buffer_t *buf;

        // if we're doing a scan or this content couldn't have been broken
        // by Microsoft we don't worry about timestamp reordering
        if ( ! pv->job || ! pv->brokenByMicrosoft )
        {
            buf = copy_frame( pv, &frame );
            buf->start = pts;
            buf->sequence = sequence;
            buf->flags = flags;
            if ( pv->new_chap && buf->start >= pv->chap_time )
            {
                buf->new_chap = pv->new_chap;
                pv->new_chap = 0;
                pv->chap_time = 0;
                log_chapter( pv, buf->new_chap, buf->start );
            }
            else if ( pv->nframes == 0 && pv->job )
            {
                log_chapter( pv, pv->job->chapter_start, buf->start );
            }
            checkCadence( pv->cadence, buf->flags, buf->start );
            hb_list_add( pv->list, buf );
            ++pv->nframes;
            return got_picture;
        }

        // XXX This following probably addresses a libavcodec bug but I don't
        //     see an easy fix so we workaround it here.
        //
        // The M$ 'packed B-frames' atrocity results in decoded frames with
        // the wrong timestamp. E.g., if there are 2 b-frames the timestamps
        // we see here will be "2 3 1 5 6 4 ..." instead of "1 2 3 4 5 6".
        // The frames are actually delivered in the right order but with
        // the wrong timestamp. To get the correct timestamp attached to
        // each frame we have a delay queue (longer than the max number of
        // b-frames) & a sorting heap for the timestamps. As each frame
        // comes out of the decoder the oldest frame in the queue is removed
        // and associated with the smallest timestamp. Then the new frame is
        // added to the queue & its timestamp is pushed on the heap.
        // This does nothing if the timestamps are correct (i.e., the video
        // uses a codec that Micro$oft hasn't broken yet) but the frames
        // get timestamped correctly even when M$ has munged them.

        // remove the oldest picture from the frame queue (if any) &
        // give it the smallest timestamp from our heap. The queue size
        // is a power of two so we get the slot of the oldest by masking
        // the frame count & this will become the slot of the newest
        // once we've removed & processed the oldest.
        int slot = pv->nframes & (HEAP_SIZE-1);
        if ( ( buf = pv->delayq[slot] ) != NULL )
        {
            buf->start = heap_pop( &pv->pts_heap );

            if ( pv->new_chap && buf->start >= pv->chap_time )
            {
                buf->new_chap = pv->new_chap;
                pv->new_chap = 0;
                pv->chap_time = 0;
                log_chapter( pv, buf->new_chap, buf->start );
            }
            else if ( pv->nframes == 0 && pv->job )
            {
                log_chapter( pv, pv->job->chapter_start, buf->start );
            }
            checkCadence( pv->cadence, buf->flags, buf->start );
            hb_list_add( pv->list, buf );
        }

        // add the new frame to the delayq & push its timestamp on the heap
        buf = copy_frame( pv, &frame );
        buf->sequence = sequence;
        buf->flags = flags;
        pv->delayq[slot] = buf;
        heap_push( &pv->pts_heap, pts );

        ++pv->nframes;
    }

    return got_picture;
}

static void decodeVideo( hb_work_private_t *pv, uint8_t *data, int size, int sequence,
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
        int len = av_parser_parse2( pv->parser, pv->context, &pout, &pout_len,
                                    data + pos, size - pos, pts, dts, AV_NOPTS_VALUE );
        pos += len;

        if ( pout_len > 0 )
        {
            pv->pts = pv->parser->pts;
            decodeFrame( pv, pout, pout_len, sequence );
        }
    } while ( pos < size );

    /* the stuff above flushed the parser, now flush the decoder */
    if ( size <= 0 )
    {
        while ( decodeFrame( pv, NULL, 0, sequence ) )
        {
        }
        flushDelayQueue( pv );
    }
}

/*
 * Removes all packets from 'pv->list', links them together into
 * a linked-list, and returns the first packet in the list.
 */
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
    pv->context->reget_buffer = reget_frame_buf;

    return 0;
}

static int next_hdr( hb_buffer_t *in, int offset )
{
    uint8_t *dat = in->data;
    uint16_t last2 = 0xffff;
    for ( ; in->size - offset > 1; ++offset )
    {
        if ( last2 == 0 && dat[offset] == 0x01 )
            // found an mpeg start code
            return offset - 2;

        last2 = ( last2 << 8 ) | dat[offset];
    }

    return -1;
}

static int find_hdr( hb_buffer_t *in, int offset, uint8_t hdr_type )
{
    if ( in->size - offset < 4 )
        // not enough room for an mpeg start code
        return -1;

    for ( ; ( offset = next_hdr( in, offset ) ) >= 0; ++offset )
    {
        if ( in->data[offset+3] == hdr_type )
            // found it
            break;
    }
    return offset;
}

static int setup_extradata( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;

    // we can't call the avstream funcs but the read_header func in the
    // AVInputFormat may set up some state in the AVContext. In particular 
    // vc1t_read_header allocates 'extradata' to deal with header issues
    // related to Microsoft's bizarre engineering notions. We alloc a chunk
    // of space to make vc1 work then associate the codec with the context.
    if ( w->codec_param != CODEC_ID_VC1 )
    {
        // we haven't been inflicted with M$ - allocate a little space as
        // a marker and return success.
        pv->context->extradata_size = 16;
        pv->context->extradata = av_malloc(pv->context->extradata_size);
        return 0;
    }

    // find the start and and of the sequence header
    int shdr, shdr_end;
    if ( ( shdr = find_hdr( in, 0, 0x0f ) ) < 0 )
    {
        // didn't find start of seq hdr
        return 1;
    }
    if ( ( shdr_end = next_hdr( in, shdr + 4 ) ) < 0 )
    {
        shdr_end = in->size;
    }
    shdr_end -= shdr;

    // find the start and and of the entry point header
    int ehdr, ehdr_end;
    if ( ( ehdr = find_hdr( in, 0, 0x0e ) ) < 0 )
    {
        // didn't find start of entry point hdr
        return 1;
    }
    if ( ( ehdr_end = next_hdr( in, ehdr + 4 ) ) < 0 )
    {
        ehdr_end = in->size;
    }
    ehdr_end -= ehdr;

    // found both headers - allocate an extradata big enough to hold both
    // then copy them into it.
    pv->context->extradata_size = shdr_end + ehdr_end;
    pv->context->extradata = av_malloc(pv->context->extradata_size + 8);
    memcpy( pv->context->extradata, in->data + shdr, shdr_end );
    memcpy( pv->context->extradata + shdr_end, in->data + ehdr, ehdr_end );
    memset( pv->context->extradata + shdr_end + ehdr_end, 0, 8);
    return 0;
}

static int decavcodecvWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    int64_t pts = AV_NOPTS_VALUE;
    int64_t dts = pts;

    *buf_in = NULL;

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        if ( pv->context->codec != NULL )
        {
            decodeVideo( pv, in->data, in->size, in->sequence, pts, dts );
        }
        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        return HB_WORK_DONE;
    }

    // if this is the first frame open the codec (we have to wait for the
    // first frame because of M$ VC1 braindamage).
    if ( pv->context->extradata_size == 0 )
    {
        if ( setup_extradata( w, in ) )
        {
            // we didn't find the headers needed to set up extradata.
            // the codec will abort if we open it so just free the buf
            // and hope we eventually get the info we need.
            hb_buffer_close( &in );
            return HB_WORK_OK;
        }
        AVCodec *codec = avcodec_find_decoder( w->codec_param );
        // There's a mis-feature in ffmpeg that causes the context to be 
        // incorrectly initialized the 1st time avcodec_open is called.
        // If you close it and open a 2nd time, it finishes the job.
        hb_avcodec_open( pv->context, codec );
        hb_avcodec_close( pv->context );
        hb_avcodec_open( pv->context, codec );
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
    decodeVideo( pv, in->data, in->size, in->sequence, pts, dts );
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
        if ( context->ticks_per_frame > 1 )
        {
            // for ffmpeg 0.5 & later, the H.264 & MPEG-2 time base is
            // field rate rather than frame rate so convert back to frames.
            info->rate_base *= context->ticks_per_frame;
        }

        info->pixel_aspect_width = context->sample_aspect_ratio.num;
        info->pixel_aspect_height = context->sample_aspect_ratio.den;

        /* Sometimes there's no pixel aspect set in the source ffmpeg context
         * which appears to come from the video stream. In that case,
         * try the pixel aspect in AVStream (which appears to come from
         * the container). Else assume a 1:1 PAR. */
        if ( info->pixel_aspect_width == 0 ||
             info->pixel_aspect_height == 0 )
        {
            // There will not be an ffmpeg stream if the file is TS
            AVStream *st = hb_ffmpeg_avstream( w->codec_param );
            info->pixel_aspect_width = st && st->sample_aspect_ratio.num ?
                                       st->sample_aspect_ratio.num : 1;
            info->pixel_aspect_height = st && st->sample_aspect_ratio.den ?
                                        st->sample_aspect_ratio.den : 1;
        }
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
        hb_avcodec_open( pv->context, codec );
    }
    // set up our best guess at the frame duration.
    // the frame rate in the codec is usually bogus but it's sometimes
    // ok in the stream.
    AVStream *st = hb_ffmpeg_avstream( w->codec_param );

    if ( st->nb_frames && st->duration )
    {
        // compute the average frame duration from the total number
        // of frames & the total duration.
        pv->duration = ( (double)st->duration * (double)st->time_base.num ) /
                       ( (double)st->nb_frames * (double)st->time_base.den );
    }
    else
    {
        // XXX We don't have a frame count or duration so try to use the
        // far less reliable time base info in the stream.
        // Because the time bases are so screwed up, we only take values
        // in the range 8fps - 64fps.
        AVRational tb;
        if ( st->avg_frame_rate.den * 64 > st->avg_frame_rate.num &&
             st->avg_frame_rate.num > st->avg_frame_rate.den * 8 )
        {
            tb.num = st->avg_frame_rate.den;
            tb.den = st->avg_frame_rate.num;
        }
        else if ( st->time_base.num * 64 > st->time_base.den &&
                  st->time_base.den > st->time_base.num * 8 )
        {
            tb = st->time_base;
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
            tb.den = 24000; /*XXX*/
        }
        pv->duration =  (double)tb.num / (double)tb.den;
    }
    pv->duration *= 90000.;

    // we have to wrap ffmpeg's get_buffer to be able to set the pts (?!)
    pv->context->opaque = pv;
    pv->context->get_buffer = get_frame_buf;
    pv->context->reget_buffer = reget_frame_buf;

    // avi, mkv and possibly mp4 containers can contain the M$ VFW packed
    // b-frames abortion that messes up frame ordering and timestamps.
    // XXX ffmpeg knows which streams are broken but doesn't expose the
    //     info externally. We should patch ffmpeg to add a flag to the
    //     codec context for this but until then we mark all ffmpeg streams
    //     as suspicious.
    pv->brokenByMicrosoft = 1;
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
    pv->pts_next = -1;
    pv->pts = -1;

    if ( w->audio != NULL )
    {
        if ( w->audio->config.out.codec == HB_ACODEC_AC3 )
        {
            // ffmpegs audio encoder expect an smpte chan map as input.
            // So we need to map the decoders output to smpte.
            pv->out_map = &hb_smpte_chan_map;
        }
        else
        {
            pv->out_map = &hb_qt_chan_map;
        }
        if ( hb_need_downmix( w->audio->config.in.channel_layout, 
                              w->audio->config.out.mixdown) )
        {
            pv->downmix = hb_downmix_init(w->audio->config.in.channel_layout, 
                                          w->audio->config.out.mixdown);
            hb_downmix_set_chan_map( pv->downmix, &hb_smpte_chan_map, pv->out_map );
        }
    }

    return 0;
}

static int decavcodecviWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    *buf_in = NULL;

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        /* flush any frames left in the decoder */
        while ( pv->context && decodeFrame( pv, NULL, 0, in->sequence ) )
        {
        }
        flushDelayQueue( pv );
        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        return HB_WORK_DONE;
    }

    if ( ! pv->context )
    {
        init_ffmpeg_context( w );
    }

    int64_t pts = in->start;
    if( pts >= 0 )
    {
        // use the first timestamp as our 'next expected' pts
        if ( pv->pts_next < 0 )
        {
            pv->pts_next = pts;
        }
        pv->pts = pts;
    }

    if ( in->new_chap )
    {
        pv->new_chap = in->new_chap;
        pv->chap_time = pts >= 0? pts : pv->pts_next;
    }
    prepare_ffmpeg_buffer( in );
    decodeFrame( pv, in->data, in->size, in->sequence );
    hb_buffer_close( &in );
    *buf_out = link_buf_list( pv );
    return HB_WORK_OK;
}

static int decavcodecviInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    if ( decavcodecvInfo( w, info ) )
    {
        hb_work_private_t *pv = w->private_data;
        if ( ! pv->context )
        {
            init_ffmpeg_context( w );
        }
        // we have the frame duration in units of the 90KHz pts clock but
        // need it in units of the 27MHz MPEG clock. */
        info->rate = 27000000;
        info->rate_base = pv->duration * 300.;
        return 1;
    }
    return 0;
}

static void decodeAudio( hb_audio_t * audio, hb_work_private_t *pv, uint8_t *data, int size )
{
    AVCodecContext *context = pv->context;
    int pos = 0;
    int loop_limit = 256;

    while ( pos < size )
    {
        int16_t *buffer = pv->buffer;
        if ( buffer == NULL )
        {
            pv->buffer = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
            buffer = pv->buffer;
        }

        AVPacket avp;
        av_init_packet( &avp );
        avp.data = data + pos;
        avp.size = size - pos;

        int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        int nsamples;
        int len = avcodec_decode_audio3( context, buffer, &out_size, &avp );
        if ( len < 0 )
        {
            return;
        }
        if ( len == 0 )
        {
            if ( !(loop_limit--) )
                return;
        }
        else
            loop_limit = 256;

        pos += len;
        if( out_size > 0 )
        {
            // We require signed 16-bit ints for the output format. If
            // we got something different convert it.
            if ( context->sample_fmt != SAMPLE_FMT_S16 )
            {
                // Note: av_audio_convert seems to be a work-in-progress but
                //       looks like it will eventually handle general audio
                //       mixdowns which would allow us much more flexibility
                //       in handling multichannel audio in HB. If we were doing
                //       anything more complicated than a one-for-one format
                //       conversion we'd probably want to cache the converter
                //       context in the pv.
                int isamp = av_get_bits_per_sample_format( context->sample_fmt ) / 8;
                AVAudioConvert *ctx = av_audio_convert_alloc( SAMPLE_FMT_S16, 1,
                                                              context->sample_fmt, 1,
                                                              NULL, 0 );
                // get output buffer size (in 2-byte samples) then malloc a buffer
                nsamples = out_size / isamp;
                buffer = av_malloc( nsamples * 2 );

                // we're doing straight sample format conversion which behaves as if
                // there were only one channel.
                const void * const ibuf[6] = { pv->buffer };
                void * const obuf[6] = { buffer };
                const int istride[6] = { isamp };
                const int ostride[6] = { 2 };

                av_audio_convert( ctx, obuf, ostride, ibuf, istride, nsamples );
                av_audio_convert_free( ctx );
            }
            else
            {
                nsamples = out_size / 2;
            }

            hb_buffer_t * buf;

            if ( pv->downmix )
            {
                pv->downmix_buffer = realloc(pv->downmix_buffer, nsamples * sizeof(hb_sample_t));
                
                int i;
                for( i = 0; i < nsamples; ++i )
                {
                    pv->downmix_buffer[i] = buffer[i];
                }

                int n_ch_samples = nsamples / context->channels;
                int channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

                buf = hb_buffer_init( n_ch_samples * channels * sizeof(float) );
                hb_sample_t *samples = (hb_sample_t *)buf->data;
                hb_downmix(pv->downmix, samples, pv->downmix_buffer, n_ch_samples);
            }
            else
            {
                buf = hb_buffer_init( nsamples * sizeof(float) );
                float *fl32 = (float *)buf->data;
                int i;
                for( i = 0; i < nsamples; ++i )
                {
                    fl32[i] = buffer[i];
                }
                int n_ch_samples = nsamples / context->channels;
                hb_layout_remap( &hb_smpte_chan_map, pv->out_map,
                                 audio->config.in.channel_layout, 
                                 fl32, n_ch_samples );
            }

            double pts = pv->pts_next;
            buf->start = pts;
            pts += nsamples * pv->duration;
            buf->stop  = pts;
            pv->pts_next = pts;

            hb_list_add( pv->list, buf );

            // if we allocated a buffer for sample format conversion, free it
            if ( buffer != pv->buffer )
            {
                av_free( buffer );
            }
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

    if ( (*buf_in)->start < -1 && pv->pts_next <= 0 )
    {
        // discard buffers that start before video time 0
        *buf_out = NULL;
        return HB_WORK_OK;
    }

    if ( ! pv->context )
    {
        init_ffmpeg_context( w );
        // duration is a scaling factor to go from #bytes in the decoded
        // frame to frame time (in 90KHz mpeg ticks). 'channels' converts
        // total samples to per-channel samples. 'sample_rate' converts
        // per-channel samples to seconds per sample and the 90000
        // is mpeg ticks per second.
        pv->duration = 90000. /
                    (double)( pv->context->sample_rate * pv->context->channels );
    }
    hb_buffer_t *in = *buf_in;

    // if the packet has a timestamp use it if we don't have a timestamp yet
    // or if there's been a timing discontinuity of more than 100ms.
    if ( in->start >= 0 &&
         ( pv->pts_next < 0 || ( in->start - pv->pts_next ) > 90*100 ) )
    {
        pv->pts_next = in->start;
    }
    prepare_ffmpeg_buffer( in );
    decodeAudio( w->audio, pv, in->data, in->size );
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
