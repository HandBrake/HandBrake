/* $Id: encavcodeca.c,v 1.23 2005/10/13 23:47:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hbffmpeg.h"
#include "downmix.h"
#include "libavcodec/audioconvert.h"

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

    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

    codec = avcodec_find_encoder( w->codec_param );
    if( !codec )
    {
        hb_log( "encavcodecaInit: avcodec_find_encoder "
                "failed" );
        return 1;
    }
    context = avcodec_alloc_context();
    avcodec_get_context_defaults3(context, codec);

    int ret = av_set_string3( context, "stereo_mode", "ms_off", 1, NULL );
    /* Let avutil sanity check the options for us*/
    if( ret == AVERROR(ENOENT) )
        hb_log( "avcodec options: Unknown option %s", "stereo_mode" );
    if( ret == AVERROR(EINVAL) )
        hb_log( "avcodec options: Bad argument %s=%s", "stereo_mode", "ms_off" ? "ms_off" : "(null)" );

    context->channel_layout = AV_CH_LAYOUT_STEREO;
    switch( audio->config.out.mixdown )
    {
        case HB_AMIXDOWN_MONO:
            context->channel_layout = AV_CH_LAYOUT_MONO;
            break;

        case HB_AMIXDOWN_STEREO:
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            context->channel_layout = AV_CH_LAYOUT_STEREO;
            break;

        case HB_AMIXDOWN_6CH:
            context->channel_layout = AV_CH_LAYOUT_5POINT1;
            break;

        default:
            hb_log(" encavcodecaInit: bad mixdown" );
            break;
    }

    context->bit_rate = audio->config.out.bitrate * 1000;
    context->sample_rate = audio->config.out.samplerate;
    context->channels = pv->out_discrete_channels;
    // Try to set format to float.  Fallback to whatever is supported.
    hb_ff_set_sample_fmt( context, codec );

    if( hb_avcodec_open( context, codec ) )
    {
        hb_log( "encavcodecaInit: avcodec_open failed" );
        return 1;
    }
    pv->context = context;

    pv->samples_per_frame = context->frame_size;
    pv->input_samples = pv->samples_per_frame * pv->out_discrete_channels;

    // Set a reasonable maximum output size
    pv->output_bytes = context->frame_size * 
        av_get_bytes_per_sample(context->sample_fmt) * 
        context->channels;

    pv->buf = malloc( pv->input_samples * sizeof( float ) );

    pv->list = hb_list_init();

    if ( w->codec_param == CODEC_ID_AAC && context->extradata )
    {
        memcpy( w->config->aac.bytes, context->extradata, context->extradata_size );
        w->config->aac.length = context->extradata_size;
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void encavcodecaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        if( pv->context )
        {
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

        free( pv );
        w->private_data = NULL;
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

    hb_chan_map_t *map = NULL;
    if ( audio->config.in.codec == HB_ACODEC_AC3 )
    {
        map = &hb_ac3_chan_map;
    }
    else if ( audio->config.in.codec == HB_ACODEC_DCA ||
              audio->config.in.codec == HB_ACODEC_LPCM )
    {
        map = &hb_qt_chan_map;
    }
    if ( map )
    {
        int layout;
        switch (audio->config.out.mixdown)
        {
            case HB_AMIXDOWN_MONO:
                layout = HB_INPUT_CH_LAYOUT_MONO;
                break;
            case HB_AMIXDOWN_STEREO:
            case HB_AMIXDOWN_DOLBY:
            case HB_AMIXDOWN_DOLBYPLII:
                layout = HB_INPUT_CH_LAYOUT_STEREO;
                break;
            case HB_AMIXDOWN_6CH:
            default:
                layout = HB_INPUT_CH_LAYOUT_3F2R | HB_INPUT_CH_LAYOUT_HAS_LFE;
                break;
        }
        hb_layout_remap( map, &hb_smpte_chan_map, layout, 
                        (float*)pv->buf, pv->samples_per_frame);
    }

    // Do we need to convert our internal float format?
    if ( pv->context->sample_fmt != AV_SAMPLE_FMT_FLT )
    {
        int isamp, osamp;
        AVAudioConvert *ctx;

        isamp = av_get_bytes_per_sample( AV_SAMPLE_FMT_FLT );
        osamp = av_get_bytes_per_sample( pv->context->sample_fmt );
        ctx = av_audio_convert_alloc( pv->context->sample_fmt, 1,
                                      AV_SAMPLE_FMT_FLT, 1,
                                      NULL, 0 );

        // get output buffer size then malloc a buffer
        //nsamples = out_size / isamp;
        //buffer = av_malloc( nsamples * sizeof(hb_sample_t) );

        // we're doing straight sample format conversion which 
        // behaves as if there were only one channel.
        const void * const ibuf[6] = { pv->buf };
        void * const obuf[6] = { pv->buf };
        const int istride[6] = { isamp };
        const int ostride[6] = { osamp };

        av_audio_convert( ctx, obuf, ostride, ibuf, istride, pv->input_samples );
        av_audio_convert_free( ctx );
    }
    
    buf = hb_buffer_init( pv->output_bytes );
    buf->size = avcodec_encode_audio( pv->context, buf->data, buf->alloc,
                                      (short*)pv->buf );

    buf->start = pts + 90000 * pos / pv->out_discrete_channels / sizeof( float ) / audio->config.out.samplerate;
    buf->stop  = buf->start + 90000 * pv->samples_per_frame / audio->config.out.samplerate;

    buf->frametype = HB_FRAME_AUDIO;

    if ( !buf->size )
    {
        hb_buffer_close( &buf );
        return Encode( w );
    }
    else if (buf->size < 0)
    {
        hb_log( "encavcodeca: avcodec_encode_audio failed" );
        hb_buffer_close( &buf );
        return NULL;
    }

    return buf;
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
        *buf_out = in;
        *buf_in = NULL;
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


