/* enclame.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#include "hb.h"

#include "lame/lame.h"

int  enclameInit( hb_work_object_t *, hb_job_t * );
int  enclameWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void enclameClose( hb_work_object_t * );

hb_work_object_t hb_enclame =
{
    WORK_ENCLAME,
    "MP3 encoder (libmp3lame)",
    enclameInit,
    enclameWork,
    enclameClose
};

struct hb_work_private_s
{
    hb_job_t   * job;

    /* LAME handle */
    lame_global_flags * lame;

    int             out_discrete_channels;
    unsigned long   input_samples;
    unsigned long   output_bytes;
    uint8_t       * buf;

    hb_list_t     * list;
    int64_t         pts;
};

int enclameInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;

    w->private_data = pv;

    pv->job   = job;

    hb_log( "enclame: opening libmp3lame" );

    pv->lame = lame_init();
    // use ABR
    lame_set_scale( pv->lame, 32768.0 );
    if( audio->config.out.compression_level >= 0 )
    {
        lame_set_quality( pv->lame, audio->config.out.compression_level );
    }
    if( audio->config.out.bitrate > 0 )
    {
        lame_set_VBR( pv->lame, vbr_abr );
        lame_set_VBR_mean_bitrate_kbps( pv->lame, audio->config.out.bitrate );
    }
    else if( audio->config.out.quality >= 0 )
    {
        lame_set_brate( pv->lame, 0 );
        lame_set_VBR( pv->lame, vbr_default );
        lame_set_VBR_quality( pv->lame, audio->config.out.quality );
    }
    lame_set_in_samplerate( pv->lame, audio->config.out.samplerate );
    lame_set_out_samplerate( pv->lame, audio->config.out.samplerate );

    pv->out_discrete_channels = hb_mixdown_get_discrete_channel_count( audio->config.out.mixdown );
    // Lame's default encoding mode is JOINT_STEREO.  This subtracts signal
    // that is "common" to left and right (within some threshold) and encodes
    // it separately.  This improves quality at low bitrates, but hurts 
    // imaging (channel separation) at higher bitrates.  So if the bitrate
    // is suffeciently high, use regular STEREO mode.
    if ( pv->out_discrete_channels == 1 )
    {
        lame_set_mode( pv->lame, MONO );
        lame_set_num_channels( pv->lame, 1 );
    }
    else if ( audio->config.out.bitrate >= 128 )
    {
        lame_set_mode( pv->lame, STEREO );
    }
    lame_init_params( pv->lame );

    pv->input_samples = 1152 * pv->out_discrete_channels;
    pv->output_bytes = LAME_MAXMP3BUFFER;
    pv->buf  = malloc( pv->input_samples * sizeof( float ) );
    audio->config.out.samples_per_frame = 1152;

    pv->list = hb_list_init();
    pv->pts  = -1;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void enclameClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    lame_close( pv->lame );
    hb_list_empty( &pv->list );
    free( pv->buf );
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
    hb_audio_t * audio = w->audio;
    hb_buffer_t * buf;
    float samples[2][1152];
    uint64_t pts, pos;
    int      i, j;

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pts, &pos);

    for( i = 0; i < 1152; i++ )
    {
        for( j = 0; j < pv->out_discrete_channels; j++ )
        {
            samples[j][i] = ((float *) pv->buf)[(pv->out_discrete_channels * i + j)];
        }
    }

    buf        = hb_buffer_init( pv->output_bytes );
    buf->s.start = pts + 90000 * pos / pv->out_discrete_channels / sizeof( float ) / audio->config.out.samplerate;
    buf->s.stop  = buf->s.start + 90000 * 1152 / audio->config.out.samplerate;
    pv->pts = buf->s.stop;
    buf->size  = lame_encode_buffer_float( 
            pv->lame, samples[0], samples[1],
            1152, buf->data, LAME_MAXMP3BUFFER );

    buf->s.type = AUDIO_BUF;
    buf->s.frametype = HB_FRAME_AUDIO;

    if( !buf->size )
    {
        /* Encoding was successful but we got no data. Try to encode
           more */
        hb_buffer_close( &buf );
        return Encode( w );
    }
    else if( buf->size < 0 )
    {
        hb_log( "enclame: lame_encode_buffer failed" );
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
int enclameWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_audio_t * audio = w->audio;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * buf;

    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input - send it downstream & say we're done */

        buf = hb_buffer_init( pv->output_bytes );
        buf->size = lame_encode_flush( pv->lame, buf->data, LAME_MAXMP3BUFFER );
        buf->s.start = pv->pts;
        buf->s.stop  = buf->s.start + 90000 * 1152 / audio->config.out.samplerate;

        buf->s.type = AUDIO_BUF;
        buf->s.frametype = HB_FRAME_AUDIO;

        if( buf->size <= 0 )
        {
            hb_buffer_close( &buf );
        }

        // Add the flushed data
        *buf_out = buf;

        // Add the eof
        if ( buf )
        {
            buf->next = in;
        }
        else
        {
            *buf_out = in;
        }

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

