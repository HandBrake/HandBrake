/* encavcodecaudio.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "audio_remap.h"

struct hb_work_private_s
{
    hb_job_t       * job;
    AVCodecContext * context;

    int              out_discrete_channels;
    int              samples_per_frame;
    unsigned long    input_samples;
    unsigned long    output_bytes;
    hb_list_t      * list;
    uint8_t        * buf;

    AVAudioResampleContext *avresample;
    int                    *remap_table;
};

static int  encavcodecaInit( hb_work_object_t *, hb_job_t * );
static int  encavcodecaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void encavcodecaClose( hb_work_object_t * );

hb_work_object_t hb_encavcodeca =
{
    WORK_ENCAVCODEC_AUDIO,
    "AVCodec Audio encoder (libavcodec)",
    encavcodecaInit,
    encavcodecaWork,
    encavcodecaClose
};

static int encavcodecaInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    hb_audio_t * audio = w->audio;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    codec = avcodec_find_encoder( w->codec_param );
    if( !codec )
    {
        hb_log( "encavcodecaInit: avcodec_find_encoder "
                "failed" );
        return 1;
    }
    context = avcodec_alloc_context3(codec);

    int mode;
    context->channel_layout = hb_ff_mixdown_xlat(audio->config.out.mixdown, &mode);
    pv->out_discrete_channels = hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);

    if (pv->out_discrete_channels > 2 &&
        audio->config.in.channel_map != &hb_libav_chan_map)
    {
        pv->remap_table = hb_audio_remap_build_table(context->channel_layout,
                                                     audio->config.in.channel_map,
                                                     &hb_libav_chan_map);
    }
    else
    {
        pv->remap_table = NULL;
    }

    AVDictionary *av_opts = NULL;
    if (w->codec_param == CODEC_ID_AAC)
    {
        av_dict_set(&av_opts, "stereo_mode", "ms_off", 0);
    }
    else if (w->codec_param == CODEC_ID_AC3 && mode != AV_MATRIX_ENCODING_NONE)
    {
        av_dict_set(&av_opts, "dsur_mode", "on", 0);
    }

    if( audio->config.out.bitrate > 0 )
        context->bit_rate = audio->config.out.bitrate * 1000;
    else if( audio->config.out.quality >= 0 )
    {
        context->global_quality = audio->config.out.quality * FF_QP2LAMBDA;
        context->flags |= CODEC_FLAG_QSCALE;
    }

    if( audio->config.out.compression_level >= 0 )
        context->compression_level = audio->config.out.compression_level;

    context->sample_rate = audio->config.out.samplerate;
    context->channels = pv->out_discrete_channels;
    // Try to set format to float.  Fallback to whatever is supported.
    hb_ff_set_sample_fmt( context, codec );

    if( hb_avcodec_open( context, codec, &av_opts, 0 ) )
    {
        hb_log( "encavcodecaInit: avcodec_open failed" );
        return 1;
    }
    // avcodec_open populates the opts dictionary with the
    // things it didn't recognize.
    AVDictionaryEntry *t = NULL;
    while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
    {
        hb_log( "encavcodecaInit: Unknown avcodec option %s", t->key );
    }
    av_dict_free( &av_opts );

    pv->context = context;

    audio->config.out.samples_per_frame = pv->samples_per_frame = context->frame_size;
    pv->input_samples = pv->samples_per_frame * pv->out_discrete_channels;

    // Set a reasonable maximum output size
    pv->output_bytes = context->frame_size * 
        av_get_bytes_per_sample(context->sample_fmt) * 
        context->channels;

    pv->buf = malloc( pv->input_samples * sizeof( float ) );

    pv->list = hb_list_init();

    if ( context->extradata )
    {
        memcpy( w->config->extradata.bytes, context->extradata, context->extradata_size );
        w->config->extradata.length = context->extradata_size;
    }

    // Check if sample format conversion is necessary
    if (AV_SAMPLE_FMT_FLT != pv->context->sample_fmt)
    {
        // Set up avresample to do conversion
        pv->avresample = avresample_alloc_context();
        if (pv->avresample == NULL)
        {
            hb_error("Failed to initialize avresample");
            return 1;
        }

        uint64_t layout;
        layout = hb_ff_layout_xlat(context->channel_layout, context->channels);
        av_opt_set_int(pv->avresample, "in_channel_layout", layout, 0);
        av_opt_set_int(pv->avresample, "out_channel_layout", layout, 0);
        av_opt_set_int(pv->avresample, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        av_opt_set_int(pv->avresample, "out_sample_fmt", context->sample_fmt, 0);

        if (avresample_open(pv->avresample) < 0)
        {
            hb_error("Failed to open avresample");
            avresample_free(&pv->avresample);
            return 1;
        }
    }
    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
// Some encoders (e.g. flac) require a final NULL encode in order to
// finalize things.
static void Finalize( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    // Finalize with NULL input needed by FLAC to generate md5sum
    // in context extradata

    // Prepare output packet
    AVPacket pkt;
    int got_packet;
    buf = hb_buffer_init( pv->output_bytes );
    av_init_packet(&pkt);
    pkt.data = buf->data;
    pkt.size = buf->alloc;

    avcodec_encode_audio2( pv->context, &pkt, NULL, &got_packet);
    hb_buffer_close( &buf );

    // Then we need to recopy the header since it was modified
    if ( pv->context->extradata )
    {
        memcpy( w->config->extradata.bytes, pv->context->extradata, pv->context->extradata_size );
        w->config->extradata.length = pv->context->extradata_size;
    }
}

static void encavcodecaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        if( pv->context )
        {
            Finalize( w );
            hb_deep_log( 2, "encavcodeca: closing libavcodec" );
            if ( pv->context->codec )
                avcodec_flush_buffers( pv->context );
            hb_avcodec_close( pv->context );
        }

        if ( pv->buf )
        {
            free( pv->buf );
            pv->buf = NULL;
        }

        if ( pv->list )
            hb_list_empty( &pv->list );

        if (pv->avresample != NULL)
        {
            avresample_free(&pv->avresample);
        }

        free( pv );
        w->private_data = NULL;
    }
}

static void convertAudioFormat( hb_work_private_t *pv, AVFrame *frame )
{
    if (pv->avresample != NULL)
    {
        int out_samples, out_linesize;

        av_samples_get_buffer_size(&out_linesize, pv->context->channels,
                                   frame->nb_samples, pv->context->sample_fmt, 0);

        out_samples = avresample_convert(pv->avresample,
                                         (void **)frame->data, out_linesize, frame->nb_samples,
                                         (void **)frame->data, frame->linesize[0], frame->nb_samples);

        if (out_samples < 0)
        {
            hb_error("avresample_convert() failed");
        }
    }
}

static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    uint64_t pts, pos;
    hb_audio_t * audio = w->audio;
    hb_buffer_t * buf;

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pts, &pos);
    if (pv->remap_table != NULL)
    {
        hb_audio_remap(pv->out_discrete_channels, pv->samples_per_frame,
                       (hb_sample_t*)pv->buf, pv->remap_table);
    }

    // Prepare input frame
    AVFrame frame;
    frame.nb_samples= pv->samples_per_frame;
    int size = av_samples_get_buffer_size(NULL, pv->context->channels,
                            frame.nb_samples, pv->context->sample_fmt, 1);
    avcodec_fill_audio_frame(&frame, pv->context->channels, 
                             pv->context->sample_fmt, pv->buf, size, 1);
    frame.pts = pts + 90000 * pos / pv->out_discrete_channels / sizeof( float ) / audio->config.out.samplerate;

    // libav requires that timebase of audio input frames to be
    // in sample_rate units.
    frame.pts = av_rescale( frame.pts, pv->context->sample_rate, 90000);

    // Do we need to convert our internal float format?
    convertAudioFormat(pv, &frame);

    // Prepare output packet
    AVPacket pkt;
    int got_packet;
    buf = hb_buffer_init( pv->output_bytes );
    av_init_packet(&pkt);
    pkt.data = buf->data;
    pkt.size = buf->alloc;

    // Encode
    int ret = avcodec_encode_audio2( pv->context, &pkt, &frame, &got_packet);
    if ( ret < 0 )
    {
        hb_log( "encavcodeca: avcodec_encode_audio failed" );
        hb_buffer_close( &buf );
        return NULL;
    }

    if ( got_packet && pkt.size )
    {
        buf->size = pkt.size;

        // The output pts from libav is in context->time_base.  Convert
        // it back to our timebase.
        //
        // Also account for the "delay" factor that libav seems to arbitrarily
        // subtract from the packet.  Not sure WTH they think they are doing
        // by offseting the value in a negative direction.
        buf->s.start = av_rescale_q( pkt.pts + pv->context->delay,
                pv->context->time_base, (AVRational){ 1, 90000 });

        buf->s.stop  = buf->s.start + 90000 * pv->samples_per_frame / audio->config.out.samplerate;

        buf->s.type = AUDIO_BUF;
        buf->s.frametype = HB_FRAME_AUDIO;
    }
    else
    {
        hb_buffer_close( &buf );
        return Encode( w );
    }

    return buf;
}

static hb_buffer_t * Flush( hb_work_object_t * w )
{
    hb_buffer_t *first, *buf, *last;

    first = last = buf = Encode( w );
    while( buf )
    {
        last = buf;
        buf->next = Encode( w );
        buf = buf->next;
    }

    if( last )
    {
        last->next = hb_buffer_init( 0 );
    }
    else
    {
        first = hb_buffer_init( 0 );
    }

    return first;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int encavcodecaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in, * buf;

    if ( in->size <= 0 )
    {
        /* EOF on input - send it downstream & say we're done */
        *buf_out = Flush( w );
        return HB_WORK_DONE;
    }

    if ( pv->context == NULL || pv->context->codec == NULL )
    {
        // No encoder context. Nothing we can do.
        return HB_WORK_OK;
    }

    hb_list_add( pv->list, in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while ( buf )
    {
        buf->next = Encode( w );
        buf = buf->next;
    }

    return HB_WORK_OK;
}


