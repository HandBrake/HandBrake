/* $Id: deca52.c,v 1.14 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "a52dec/a52.h"
#include "libavutil/crc.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    /* liba52 handle */
    a52_state_t * state;

    int           flags_in;
    int           flags_out;
    int           rate;
    int           bitrate;
    int           out_discrete_channels;
    int           error;
    int           frames;                   // number of good frames decoded
    int           crc_errors;               // number of frames with crc errors
    int           bytes_dropped;            // total bytes dropped while resyncing
    float         level;
    float         dynamic_range_compression;
    double        next_expected_pts;
    int64_t       last_buf_pts;
    hb_list_t    *list;
    const AVCRC  *crc_table;
    uint8_t       frame[3840];
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

    pv->crc_table = av_crc_get_table( AV_CRC_16_ANSI );
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

    if ( pv->crc_errors )
    {
        hb_log( "deca52: %d frames decoded, %d crc errors, %d bytes dropped",
                pv->frames, pv->crc_errors, pv->bytes_dropped );
    }
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

    if ( (*buf_in)->start < -1 && pv->next_expected_pts == 0 )
    {
        // discard buffers that start before video time 0
        *buf_out = NULL;
        return HB_WORK_OK;
    }

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
    hb_audio_t  * audio = w->audio;
    int           i, j, k;
    int           size = 0;

    // check that we're at the start of a valid frame and align to the
    // start of a valid frame if we're not.
    // we have to check the header & crc so we need at least
    // 7 (the header size) + 128 (the minimum frame size) bytes
    while( hb_list_bytes( pv->list ) >= 7+128 )
    {
        /* check if this is a valid header */
        hb_list_seebytes( pv->list, pv->frame, 7 );
        size = a52_syncinfo( pv->frame, &pv->flags_in, &pv->rate, &pv->bitrate );
        if ( size > 0 )
        {
            // header looks valid - check the crc1
            if( size > hb_list_bytes( pv->list ) )
            {
                // don't have all the frame's data yet
                return NULL;
            }
            int crc1size = (size >> 1) + (size >> 3);
            hb_list_seebytes( pv->list, pv->frame, crc1size );
            if ( av_crc( pv->crc_table, 0, pv->frame + 2, crc1size - 2 ) == 0 )
            {
                // crc1 is ok - say we have valid frame sync
                if( pv->error )
                {
                    hb_log( "output track %d: ac3 in sync after skipping %d bytes",
                            audio->config.out.track, pv->error );
                    pv->bytes_dropped += pv->error;
                    pv->error = 0;
                }
                break;
            }
        }
        // no sync - discard one byte then try again
        hb_list_getbytes( pv->list, pv->frame, 1, NULL, NULL );
        ++pv->error;
    }

    // we exit the above loop either in error state (we didn't find sync
    // or don't have enough data yet to validate sync) or in sync. If we're
    // not in sync we need more data so just return.
    if( pv->error || size <= 0 || hb_list_bytes( pv->list ) < size )
    {
        /* Need more data */
        return NULL;
    }

    // Get the whole frame and check its CRC. If the CRC is wrong
    // discard the frame - we'll resync on the next call.

    uint64_t ipts;
    hb_list_getbytes( pv->list, pv->frame, size, &ipts, NULL );
    if ( av_crc( pv->crc_table, 0, pv->frame + 2, size - 2 ) != 0 )
    {
        ++pv->crc_errors;
        return NULL;
    }
    ++pv->frames;
    if ( ipts != pv->last_buf_pts )
    {
        pv->last_buf_pts = ipts;
    }
    else
    {
        // spec says that the PTS is the start time of the first frame
        // that starts in the PES frame so we only use the PTS once then
        // get the following frames' PTS from the frame length.
        ipts = -1;
    }

    double pts = ( ipts != -1 ) ? ipts : pv->next_expected_pts;
    double frame_dur = (6. * 256. * 90000.) / pv->rate;

    /* AC3 passthrough: don't decode the AC3 frame */
    if( audio->config.out.codec == HB_ACODEC_AC3 )
    {
        buf = hb_buffer_init( size );
        memcpy( buf->data, pv->frame, size );
        buf->start = pts;
        pts += frame_dur;
        buf->stop  = pts;
        pv->next_expected_pts = pts;
        return buf;
    }

    /* Feed liba52 */
    a52_frame( pv->state, pv->frame, &pv->flags_out, &pv->level, 0 );

    /* If a user specifies strong dynamic range compression (>1), adjust it.
       If a user specifies default dynamic range compression (1), leave it alone.
       If a user specifies no dynamic range compression (0), call a null function. */
    if( pv->dynamic_range_compression > 1.0 )
    {
        a52_dynrng( pv->state, dynrng_call, &pv->dynamic_range_compression );
    }
    else if( !pv->dynamic_range_compression )
    {
        a52_dynrng( pv->state, NULL, NULL );
    }

    /* 6 blocks per frame, 256 samples per block, channelsused channels */
    buf        = hb_buffer_init( 6 * 256 * pv->out_discrete_channels * sizeof( float ) );
    buf->start = pts;
    pts += frame_dur;
    buf->stop  = pts;
    pv->next_expected_pts = pts;

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
    return buf;
}

static int find_sync( const uint8_t *buf, int len )
{
    int i;

    // since AC3 frames don't line up with MPEG ES frames scan the
    // frame for an AC3 sync pattern.
    for ( i = 0; i < len - 16; ++i )
    {
        int rate, bitrate, flags;
        int size = a52_syncinfo( (uint8_t *)buf + i, &flags, &rate, &bitrate );
        if( size > 0 )
        {
            // we have a plausible sync header - see if crc1 checks
            int crc1size = (size >> 1) + (size >> 3); 
            if ( i + crc1size > len )
            {
                // don't have enough data to check crc1
                break;
            }
            if ( av_crc( av_crc_get_table( AV_CRC_16_ANSI ), 0,
                         buf + i + 2, crc1size - 2 ) == 0 )
            {
                // crc checks - we've got sync
                return i;
            }
        }
    }
    return -1;
}

static int deca52BSInfo( hb_work_object_t *w, const hb_buffer_t *b,
                         hb_work_info_t *info )
{
    memset( info, 0, sizeof(*info) );

    // We don't know if the way that AC3 frames are fragmented into whatever
    // packetization the container uses will give us enough bytes per fragment
    // to check the CRC (we need at least 5/8 of the the frame). So we
    // copy the fragment we got into an accumulation buffer in the audio object
    // then look for sync over all the frags we've accumulated so far.
    uint8_t *buf = w->audio->priv.config.a52.buf;
    int len = w->audio->priv.config.a52.len, blen = b->size;
    if ( len + blen > sizeof(w->audio->priv.config.a52.buf) )
    {
        // we don't have enough empty space in the accumulation buffer to
        // hold the new frag - make room for it by discarding the oldest data.
        if ( blen >= sizeof(w->audio->priv.config.a52.buf) )
        {
            // the frag is bigger than our accumulation buffer - copy all
            // that will fit (the excess doesn't matter since the buffer
            // is many times the size of a max length ac3 frame).
            blen = sizeof(w->audio->priv.config.a52.buf);
            len = 0;
        }
        else
        {
            // discard enough bytes from the front of the buffer to make
            // room for the new stuff
            int newlen = sizeof(w->audio->priv.config.a52.buf) - blen;
            memcpy( buf, buf + len - newlen, newlen );
            len = newlen;
        }
    }
    // add the new frag to the buffer
    memcpy( buf+len, b->data, blen );
    len += blen;

    int i;
    if ( ( i = find_sync( buf, len ) ) < 0 )
    {
        // didn't find sync - wait for more data
        w->audio->priv.config.a52.len = len;
        return 0;
    }

    // got sync - extract and canoncalize the bitstream parameters
    int rate = 0, bitrate = 0, flags = 0;
    uint8_t raw = buf[i + 5];
    a52_syncinfo( buf + i, &flags, &rate, &bitrate );

    if ( rate == 0 || bitrate == 0 )
    {
        // invalid AC-3 parameters - toss what we have so we'll start over
        // with the next buf otherwise we'll keep syncing on this junk.
        w->audio->priv.config.a52.len = 0;
        return 0;
    }

    // bsid | bsmod | acmod | cmixlv | surmixlv | dsurmod | lfeon | dialnorm | compre
    //   5       3      3        2         2         2        1          5        1
    //      byte1   |          byte2                 |    byte3  

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
