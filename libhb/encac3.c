/* $Id: encac3.c,v 1.23 2005/10/13 23:47:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hbffmpeg.h"
#include "downmix.h"

struct hb_work_private_s
{
    hb_job_t       * job;
    AVCodecContext * context;

    int              out_discrete_channels;
    unsigned long    input_samples;
    unsigned long    output_bytes;
    hb_list_t      * list;
    uint8_t        * buf;
    int16_t        * samples;
};

int  encac3Init( hb_work_object_t *, hb_job_t * );
int  encac3Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encac3Close( hb_work_object_t * );

#define AC3_SAMPLES_PER_FRAME 1536
#define AC3_MAX_CODED_FRAME_SIZE 3840

hb_work_object_t hb_encac3 =
{
    WORK_ENCAC3,
    "AC-3 encoder (libavcodec)",
    encac3Init,
    encac3Work,
    encac3Close
};

int encac3Init( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    hb_audio_t * audio = w->audio;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);
    pv->input_samples = AC3_SAMPLES_PER_FRAME * pv->out_discrete_channels;
    pv->output_bytes = AC3_MAX_CODED_FRAME_SIZE;

    pv->buf = malloc( pv->input_samples * sizeof( float ) );
    pv->samples = malloc( pv->input_samples * sizeof( int16_t ) );

    codec = avcodec_find_encoder( CODEC_ID_AC3 );
    if( !codec )
    {
        hb_log( "encac3Init: avcodec_find_encoder "
                "failed" );
    }
    context = avcodec_alloc_context();

    context->channel_layout = CH_LAYOUT_STEREO;
    switch( audio->config.out.mixdown )
    {
        case HB_AMIXDOWN_MONO:
            context->channel_layout = CH_LAYOUT_MONO;
            break;

        case HB_AMIXDOWN_STEREO:
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            context->channel_layout = CH_LAYOUT_STEREO;
            break;

        case HB_AMIXDOWN_6CH:
            context->channel_layout = CH_LAYOUT_5POINT0|CH_LOW_FREQUENCY;
            break;

        default:
            hb_log(" encac3Init: bad mixdown" );
            break;
    }

    context->bit_rate = audio->config.out.bitrate * 1000;
    context->sample_rate = audio->config.out.samplerate;
    context->channels = pv->out_discrete_channels;

    if( hb_avcodec_open( context, codec ) )
    {
        hb_log( "encac3Init: avcodec_open failed" );
    }
    pv->context = context;

    pv->list = hb_list_init();

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encac3Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        if( pv->context )
        {
            hb_deep_log( 2, "encac3: closing libavcodec" );
            if ( pv->context->codec )
                avcodec_flush_buffers( pv->context );
            hb_avcodec_close( pv->context );
        }

        if ( pv->buf )
        {
            free( pv->buf );
            pv->buf = NULL;
        }

        if ( pv->samples )
        {
            free( pv->samples );
            pv->samples = NULL;
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
    int ii;

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
    else if ( audio->config.in.codec == HB_ACODEC_DCA )
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
                        (float*)pv->buf, AC3_SAMPLES_PER_FRAME);
    }
    
    for (ii = 0; ii < pv->input_samples; ii++)
    {
        pv->samples[ii] = (int16_t)((float*)pv->buf)[ii];
    }

    buf = hb_buffer_init( pv->output_bytes );
    buf->size = avcodec_encode_audio( pv->context, buf->data, buf->alloc,
                                          pv->samples );

    buf->start = pts + 90000 * pos / pv->out_discrete_channels / sizeof( float ) / audio->config.out.samplerate;
    buf->stop  = buf->start + 90000 * AC3_SAMPLES_PER_FRAME / audio->config.out.samplerate;

    buf->frametype = HB_FRAME_AUDIO;

    if ( !buf->size )
    {
        hb_buffer_close( &buf );
        return Encode( w );
    }
    else if (buf->size < 0)
    {
        hb_log( "encac3: avcodec_encode_audio failed" );
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
int encac3Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
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


