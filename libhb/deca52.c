/* $Id: deca52.c,v 1.14 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "a52dec/a52.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    /* liba52 handle */
    a52_state_t * state;

    int           flags_in;
    int           flags_out;
    int           rate;
    int           bitrate;
    float         level;
    float         dynamic_range_compression;

    int           error;
    int           sync;
    int           size;

    int64_t       next_expected_pts;

    int64_t       sequence;

    uint8_t       frame[3840];

    hb_list_t   * list;

	int           out_discrete_channels;

};

static int  deca52Init( hb_work_object_t *, hb_job_t * );
static int  deca52Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void deca52Close( hb_work_object_t * );
static int deca52BSInfo( hb_work_object_t * , const hb_buffer_t *,
                         hb_work_info_t * );

hb_work_object_t hb_deca52 =
{
    WORK_DECA52,
    "AC3 decoder",
    deca52Init,
    deca52Work,
    deca52Close,
    0,
    deca52BSInfo
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w );

/***********************************************************************
 * dynrng_call
 ***********************************************************************
 * Boosts soft audio -- taken from gbooker's work in A52Decoder, comment and all..
 * Two cases
 * 1) The user requested a compression of 1 or less, return the typical power rule
 * 2) The user requested a compression of more than 1 (decompression):
 *    If the stream's requested compression is less than 1.0 (loud sound), return the normal compression
 *    If the stream's requested compression is more than 1.0 (soft sound), use power rule (which will make
 *   it louder in this case).
 *
 **********************************************************************/
static sample_t dynrng_call (sample_t c, void *data)
{
        float *level = (float *)data;
        float levelToUse = (float)*level;
        if(c > 1.0 || levelToUse <= 1.0)
        {
            return powf(c, levelToUse);
        }
        else
                return c;
}

/***********************************************************************
 * hb_work_deca52_init
 ***********************************************************************
 * Allocate the work object, initialize liba52
 **********************************************************************/
static int deca52Init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    w->private_data = pv;

    pv->job   = job;

    pv->list      = hb_list_init();
    pv->state     = a52_init( 0 );

	/* Decide what format we want out of a52dec
	work.c has already done some of this deduction for us in do_job() */

	pv->flags_out = HB_AMIXDOWN_GET_A52_FORMAT(audio->config.out.mixdown);

	/* pass the number of channels used into the private work data */
	/* will only be actually used if we're not doing AC3 passthru */
    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

    pv->level     = 32768.0;
    pv->dynamic_range_compression = audio->config.out.dynamic_range_compression;

    pv->next_expected_pts = 0;
    pv->sequence = 0;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
static void deca52Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    a52_free( pv->state );
    hb_list_empty( &pv->list );
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * Add the given buffer to the data we already have, and decode as much
 * as we can
 **********************************************************************/
static int deca52Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = *buf_in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    pv->sequence = (*buf_in)->sequence;

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    /* If we got more than a frame, chain raw buffers */
    *buf_out = buf = Decode( w );
    while( buf )
    {
        buf->sequence = pv->sequence;
        buf->next = Decode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}

/***********************************************************************
 * Decode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    hb_audio_t  * audio = w->audio;
    int           i, j, k;
    uint64_t      pts, pos;

    /* Get a frame header if don't have one yet */
    if( !pv->sync )
    {
        while( hb_list_bytes( pv->list ) >= 7 )
        {
            /* We have 7 bytes, check if this is a correct header */
            hb_list_seebytes( pv->list, pv->frame, 7 );
            pv->size = a52_syncinfo( pv->frame, &pv->flags_in, &pv->rate,
                                    &pv->bitrate );
            if( pv->size )
            {
                /* It is. W00t. */
                if( pv->error )
                {
                    hb_log( "a52_syncinfo ok" );
                }
                pv->error = 0;
                pv->sync  = 1;
                break;
            }

            /* It is not */
            if( !pv->error )
            {
                hb_log( "a52_syncinfo failed" );
                pv->error = 1;
            }

            /* Try one byte later */
            hb_list_getbytes( pv->list, pv->frame, 1, NULL, NULL );
        }
    }

    if( !pv->sync ||
        hb_list_bytes( pv->list ) < pv->size )
    {
        /* Need more data */
        return NULL;
    }

    /* Get the whole frame */
    hb_list_getbytes( pv->list, pv->frame, pv->size, &pts, &pos );
    if (pts == -1)
    {
        pts = pv->next_expected_pts;
    }

    /* AC3 passthrough: don't decode the AC3 frame */
    if( audio->config.out.codec == HB_ACODEC_AC3 )
    {
        buf = hb_buffer_init( pv->size );
        memcpy( buf->data, pv->frame, pv->size );
        buf->start = pts + ( pos / pv->size ) * 6 * 256 * 90000 / pv->rate;
        buf->stop  = buf->start + 6 * 256 * 90000 / pv->rate;
        pv->next_expected_pts = buf->stop;
        pv->sync = 0;
        return buf;
    }

    /* Feed liba52 */
    a52_frame( pv->state, pv->frame, &pv->flags_out, &pv->level, 0 );

    if ( pv->dynamic_range_compression > 1.0 )
    {
        a52_dynrng( pv->state, dynrng_call, &pv->dynamic_range_compression);
    }

    /* 6 blocks per frame, 256 samples per block, channelsused channels */
    buf        = hb_buffer_init( 6 * 256 * pv->out_discrete_channels * sizeof( float ) );
    buf->start = pts + ( pos / pv->size ) * 6 * 256 * 90000 / pv->rate;
    buf->stop  = buf->start + 6 * 256 * 90000 / pv->rate;

    /*
       * To track AC3 PTS add this back in again.
        *hb_log("AC3: pts is %lld, buf->start %lld buf->stop %lld", pts, buf->start, buf->stop);
        */

    pv->next_expected_pts = buf->stop;

    for( i = 0; i < 6; i++ )
    {
        sample_t * samples_in;
        float    * samples_out;

        a52_block( pv->state );
        samples_in  = a52_samples( pv->state );
        samples_out = ((float *) buf->data) + 256 * pv->out_discrete_channels * i;

        /* Interleave */
        for( j = 0; j < 256; j++ )
        {
			for ( k = 0; k < pv->out_discrete_channels; k++ )
			{
				samples_out[(pv->out_discrete_channels*j)+k]   = samples_in[(256*k)+j];
			}
        }

    }

    pv->sync = 0;
    return buf;
}

static int deca52BSInfo( hb_work_object_t *w, const hb_buffer_t *b,
                         hb_work_info_t *info )
{
    int i;
    int rate = 0, bitrate = 0, flags = 0;
    int old_rate = 0, old_bitrate = 0;
    uint8_t raw;

    memset( info, 0, sizeof(*info) );

    /* since AC3 frames don't line up with MPEG ES frames scan the
     * entire frame for an AC3 sync pattern.  */
    for ( i = 0; i < b->size - 7; ++i )
    {
        if( a52_syncinfo( &b->data[i], &flags, &rate, &bitrate ) != 0 )
        {
            /*
             * Got sync apparently, save these values and check that they
             * also match when we get sync again.
             */
            if( old_rate ) 
            {
                if( rate == old_rate && bitrate == old_bitrate )
                {
                    break;
                } 
            } 
            
            old_rate = rate;
            old_bitrate = bitrate;
            raw = b->data[i+5];
        }
    }
    if ( rate == 0 || bitrate == 0 )
    {
        /* didn't find AC3 sync */
        return 0;
    }

    /*
     * bsid | bsmod | acmod | cmixlev | surmixlev | dsurmod | lfeon | dialnorm | compre
     *    5       3       3         2           2         2       1          5        1
     * [    byte1  ][         byte2                    ][   byte3                     ]
     */


    info->name = "AC-3";
    info->rate = rate;
    info->rate_base = 1;
    info->bitrate = bitrate;
    info->flags = flags;
    info->version = raw >> 3;    /* bsid is the first 5 bits */
    info->mode = raw & 0x7;      /* bsmod is the following 3 bits */

    if ( (flags & A52_CHANNEL_MASK) == A52_DOLBY )
    {
        info->flags |= AUDIO_F_DOLBY;
    }

    switch( flags & A52_CHANNEL_MASK )
    {
        /* mono sources */
        case A52_MONO:
        case A52_CHANNEL1:
        case A52_CHANNEL2:
            info->channel_layout = HB_INPUT_CH_LAYOUT_MONO;
            break;
        /* stereo input */
        case A52_CHANNEL:
        case A52_STEREO:
            info->channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
            break;
        /* dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input */
        case A52_DOLBY:
            info->channel_layout = HB_INPUT_CH_LAYOUT_DOLBY;
            break;
        /* 3F/2R input */
        case A52_3F2R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F2R;
            break;
        /* 3F/1R input */
        case A52_3F1R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F1R;
            break;
        /* other inputs */
        case A52_3F:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F;
            break;
        case A52_2F1R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_2F1R;
            break;
        case A52_2F2R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_2F2R;
            break;
        /* unknown */
        default:
            info->channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
    }

    if (flags & A52_LFE)
    {
        info->channel_layout |= HB_INPUT_CH_LAYOUT_HAS_LFE;
    }

    return 1;
}
