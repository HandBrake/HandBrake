/* encfaac.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "audio_remap.h"

#include "faac.h"

struct hb_work_private_s
{
    hb_job_t   * job;

    faacEncHandle * faac;
    unsigned long   input_samples;
    unsigned long   output_bytes;
    uint8_t       * buf;
    uint8_t       * obuf;
    hb_list_t     * list;
    double          pts;
    double          framedur;
	int             out_discrete_channels;
};

int  encfaacInit( hb_work_object_t *, hb_job_t * );
int  encfaacWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encfaacClose( hb_work_object_t * );

hb_work_object_t hb_encfaac =
{
    WORK_ENCFAAC,
    "AAC encoder (libfaac)",
    encfaacInit,
    encfaacWork,
    encfaacClose
};

/***********************************************************************
 * hb_work_encfaac_init
 ***********************************************************************
 *
 **********************************************************************/
int encfaacInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    faacEncConfigurationPtr cfg;
    uint8_t * bytes;
    unsigned long length;

    w->private_data = pv;

    pv->job   = job;

	/* pass the number of channels used into the private work data */
    pv->out_discrete_channels = hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);

    pv->faac = faacEncOpen( audio->config.out.samplerate, pv->out_discrete_channels,
                            &pv->input_samples, &pv->output_bytes );
    pv->buf  = malloc( pv->input_samples * sizeof( float ) );
    pv->obuf = malloc( pv->output_bytes );
    pv->framedur = 90000.0 * pv->input_samples /
                   ( audio->config.out.samplerate * pv->out_discrete_channels );
    audio->config.out.samples_per_frame = pv->input_samples / pv->out_discrete_channels;

    cfg                = faacEncGetCurrentConfiguration( pv->faac );
    cfg->mpegVersion   = MPEG4;
    cfg->aacObjectType = LOW;
    cfg->allowMidside  = 1;

    /* channel configuration & remapping */
    uint64_t layout = hb_ff_mixdown_xlat(audio->config.out.mixdown, NULL);
    hb_audio_remap_build_table(&hb_aac_chan_map, audio->config.in.channel_map,
                               layout, cfg->channel_map);

    switch (audio->config.out.mixdown)
    {
        case HB_AMIXDOWN_7POINT1:
            cfg->channelConfiguration = 0;
            cfg->numFrontChannels     = 3;
            cfg->numSideChannels      = 2;
            cfg->numBackChannels      = 2;
            cfg->numLFEChannels       = 1;
            break;

        case HB_AMIXDOWN_6POINT1:
            cfg->channelConfiguration = 0;
            cfg->numFrontChannels     = 3;
            cfg->numSideChannels      = 0;
            cfg->numBackChannels      = 3;
            cfg->numLFEChannels       = 1;
            break;

        case HB_AMIXDOWN_5_2_LFE:
            cfg->channelConfiguration = 7;
            break;

        default:
            cfg->channelConfiguration =
                hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown);
            break;
    }

    cfg->useTns        = 0;
    cfg->bitRate       = audio->config.out.bitrate * 1000 / pv->out_discrete_channels; /* Per channel */
    cfg->bandWidth     = 0;
    cfg->outputFormat  = 0;
    cfg->inputFormat   =  FAAC_INPUT_FLOAT;

    if( !faacEncSetConfiguration( pv->faac, cfg ) )
    {
        hb_log( "faacEncSetConfiguration failed" );
        *job->die = 1;
        return 0;
    }

    if( faacEncGetDecoderSpecificInfo( pv->faac, &bytes, &length ) < 0 )
    {
        hb_log( "faacEncGetDecoderSpecificInfo failed" );
        *job->die = 1;
        return 0;
    }
    memcpy( w->config->extradata.bytes, bytes, length );
    w->config->extradata.length = length;
    free( bytes );

    pv->list = hb_list_init();

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encfaacClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    if ( pv )
    {
        if ( pv->faac )
        {
            faacEncClose( pv->faac );
            pv->faac = NULL;
        }
        if ( pv->buf )
        {
            free( pv->buf );
            pv->buf = NULL;
        }
        if ( pv->obuf )
        {
            free( pv->obuf );
            pv->obuf = NULL;
        }
        if ( pv->list )
            hb_list_empty( &pv->list );

        free( pv );
        w->private_data = NULL;
    }
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        /* Need more data */
        return NULL;
    }

    uint64_t pts, pos;
    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pts, &pos );

    int i;
    float *fltBuf = (float*)pv->buf;
    for ( i = 0; i < pv->input_samples; i++ )
        fltBuf[i] *= 32768.0;
        
    int size = faacEncEncode( pv->faac, (int32_t *)pv->buf, pv->input_samples,
                              pv->obuf, pv->output_bytes );

    // AAC needs four frames before it can start encoding so we'll get nothing
    // on the first three calls to the encoder.
    if ( size > 0 )
    {
        hb_buffer_t * buf = hb_buffer_init( size );
        memcpy( buf->data, pv->obuf, size );
        buf->size = size;
        buf->s.start = pv->pts;
        pv->pts += pv->framedur;
        buf->s.stop = pv->pts;
        buf->s.type = AUDIO_BUF;
        buf->s.frametype = HB_FRAME_AUDIO;
        return buf;
    }
    return NULL;
}

static hb_buffer_t *Flush( hb_work_object_t *w, hb_buffer_t *bufin )
{
    hb_work_private_t *pv = w->private_data;

    // pad whatever data we have out to four input frames.
    int nbytes = hb_list_bytes( pv->list );
    int pad = pv->input_samples * sizeof(float) * 4 - nbytes;
    if ( pad > 0 )
    {
        hb_buffer_t *tmp = hb_buffer_init( pad );
        memset( tmp->data, 0, pad );
        hb_list_add( pv->list, tmp );
    }

    // There are up to three frames buffered in the encoder plus one
    // in our list buffer so four calls to Encode should get them all.
    hb_buffer_t *bufout = NULL, *buf = NULL;
    while ( hb_list_bytes( pv->list ) >= pv->input_samples * sizeof(float) )
    {
        hb_buffer_t *b = Encode( w );
        if ( b )
        {
            if ( bufout == NULL )
            {
                bufout = b;
            }
            else
            {
                buf->next = b;
            }
            buf = b;
        }
    }
    // add the eof marker to the end of our buf chain
    if ( buf )
        buf->next = bufin;
    else
        bufout = bufin;
    return bufout;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encfaacWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if ( (*buf_in)->size <= 0 )
    {
        // EOF on input. Finish encoding what we have buffered then send
        // it & the eof downstream.

        *buf_out = Flush( w, *buf_in );
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while( buf )
    {
        buf->next = Encode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}

