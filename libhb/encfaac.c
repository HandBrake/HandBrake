/* $Id: encfaac.c,v 1.13 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "faac.h"

struct hb_work_private_s
{
    hb_job_t   * job;

    faacEncHandle * faac;
    unsigned long   input_samples;
    unsigned long   output_bytes;
    uint8_t       * buf;

    hb_list_t     * list;
    int64_t         pts;

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
    faacEncConfigurationPtr cfg;
    uint8_t * bytes;
    unsigned long length;

    w->private_data = pv;

    pv->job   = job;

	/* pass the number of channels used into the private work data */
	pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(w->amixdown);

    pv->faac = faacEncOpen( job->arate, pv->out_discrete_channels, &pv->input_samples,
                           &pv->output_bytes );
    pv->buf  = malloc( pv->input_samples * sizeof( float ) );

    cfg                = faacEncGetCurrentConfiguration( pv->faac );
    cfg->mpegVersion   = MPEG4;
    cfg->aacObjectType = LOW;
    cfg->allowMidside  = 1;

	if (pv->out_discrete_channels == 6) {
		/* we are preserving 5.1 audio into 6-channel AAC,
		so indicate that we have an lfe channel */
		cfg->useLfe    = 1;
	} else {
		cfg->useLfe    = 0;
	}

    cfg->useTns        = 0;
    cfg->bitRate       = job->abitrate * 1000 / pv->out_discrete_channels; /* Per channel */
    cfg->bandWidth     = 0;
    cfg->outputFormat  = 0;
    cfg->inputFormat   =  FAAC_INPUT_FLOAT;

	if (w->amixdown == HB_AMIXDOWN_6CH && w->source_acodec == HB_ACODEC_AC3)
    {
        /* we are preserving 5.1 AC-3 audio into 6-channel AAC, and need to
        re-map the output of deca52 into our own mapping - the mapping
        below is the default mapping expected by QuickTime */
        /* DTS output from libdca is already in the right mapping for QuickTime */
        /* This doesn't seem to be correct for VLC on Linux */
        cfg->channel_map[0] = 2;
        cfg->channel_map[1] = 1;
        cfg->channel_map[2] = 3;
        cfg->channel_map[3] = 4;
        cfg->channel_map[4] = 5;
        cfg->channel_map[5] = 0;
	}

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
    memcpy( w->config->aac.bytes, bytes, length );
    w->config->aac.length = length;
    free( bytes );

    pv->list = hb_list_init();
    pv->pts  = -1;

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
    faacEncClose( pv->faac );
    free( pv->buf );
    hb_list_empty( &pv->list );
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    uint64_t      pts, pos;

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        /* Need more data */
        return NULL;
    }

    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pts, &pos );

    buf        = hb_buffer_init( pv->output_bytes );
    buf->start = pts + 90000 * pos / pv->out_discrete_channels / sizeof( float ) / pv->job->arate;
    buf->stop  = buf->start + 90000 * pv->input_samples / pv->job->arate / pv->out_discrete_channels;
    buf->size  = faacEncEncode( pv->faac, (int32_t *) pv->buf,
            pv->input_samples, buf->data, pv->output_bytes );
    buf->frametype   = HB_FRAME_AUDIO;

    if( !buf->size )
    {
        /* Encoding was successful but we got no data. Try to encode
           more */
        hb_buffer_close( &buf );
        return Encode( w );
    }
    else if( buf->size < 0 )
    {
        hb_log( "faacEncEncode failed" );
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
int encfaacWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

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

