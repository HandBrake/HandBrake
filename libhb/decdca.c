/* $Id: decdca.c,v 1.14 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "dca.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    /* libdca handle */
    dca_state_t * state;

    int           flags_in;
    int           flags_out;
    int           rate;
    int           bitrate;
    int           frame_length;
    float         level;

    int           error;
    int           sync;
    int           size;

    /* max frame size of the 16 bits version is 16384 */
    /* max frame size of the 14 bits version is 18726 */
    uint8_t       frame[18726];

    hb_list_t   * list;

	int           out_discrete_channels;

};

int  decdcaInit( hb_work_object_t *, hb_job_t * );
int  decdcaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void decdcaClose( hb_work_object_t * );

hb_work_object_t hb_decdca =
{
    WORK_DECDCA,
    "DCA decoder",
    decdcaInit,
    decdcaWork,
    decdcaClose
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w );

/***********************************************************************
 * hb_work_decdca_init
 ***********************************************************************
 * Allocate the work object, initialize libdca
 **********************************************************************/
int decdcaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    w->private_data = pv;

    pv->job   = job;

    pv->list      = hb_list_init();
    pv->state     = dca_init( 0 );

	/* Decide what format we want out of libdca
	work.c has already done some of this deduction for us in do_job() */

	pv->flags_out = HB_AMIXDOWN_GET_DCA_FORMAT(audio->config.out.mixdown);

	/* pass the number of channels used into the private work data */
	/* will only be actually used if we're not doing AC3 passthru */
    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

    pv->level     = 32768.0;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
void decdcaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    dca_free( pv->state );
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
int decdcaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    /* If we got more than a frame, chain raw buffers */
    *buf_out = buf = Decode( w );
    while( buf )
    {
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
    int           i, j, k;
    uint64_t      pts, pos;
    int           num_blocks;

    /* Get a frame header if don't have one yet */
    if( !pv->sync )
    {
        while( hb_list_bytes( pv->list ) >= 14 )
        {
            /* We have 14 bytes, check if this is a correct header */
            hb_list_seebytes( pv->list, pv->frame, 14 );
            pv->size = dca_syncinfo( pv->state, pv->frame, &pv->flags_in, &pv->rate,
                                    &pv->bitrate, &pv->frame_length );
            if( pv->size )
            {
                /* It is. W00t. */
                if( pv->error )
                {
                    hb_log( "dca_syncinfo ok" );
                }
                pv->error = 0;
                pv->sync  = 1;
                break;
            }

            /* It is not */
            if( !pv->error )
            {
                hb_log( "dca_syncinfo failed" );
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

    /* Feed libdca */
    dca_frame( pv->state, pv->frame, &pv->flags_out, &pv->level, 0 );

    /* find out how many blocks are in this frame */
    num_blocks = dca_blocks_num( pv->state );

    /* num_blocks blocks per frame, 256 samples per block, channelsused channels */
    buf        = hb_buffer_init( num_blocks * 256 * pv->out_discrete_channels * sizeof( float ) );
    buf->start = pts + ( pos / pv->size ) * num_blocks * 256 * 90000 / pv->rate;
    buf->stop  = buf->start + num_blocks * 256 * 90000 / pv->rate;

    for( i = 0; i < num_blocks; i++ )
    {
        dca_sample_t * samples_in;
        float    * samples_out;

        dca_block( pv->state );
        samples_in  = dca_samples( pv->state );
        samples_out = ((float *) buf->data) + 256 * pv->out_discrete_channels * i;

        /* Interleave */
        for( j = 0; j < 256; j++ )
        {
			for ( k = 0; k < pv->out_discrete_channels; k++ )
			{
				samples_out[(pv->out_discrete_channels*j)+k]   = samples_in[(256*k)+j] * 16384;
			}
        }

    }

    pv->sync = 0;
    return buf;
}

