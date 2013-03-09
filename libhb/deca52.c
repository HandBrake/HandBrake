/* deca52.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "audio_remap.h"
#include "audio_resample.h"

#include "a52dec/a52.h"
#include "libavutil/crc.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    /* liba52 handle */
    a52_state_t * state;

    int           flags;
    int           rate;
    int           bitrate;
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
    uint8_t       buf[6][6][256 * sizeof(float)]; // decoded frame (up to 6 channels, 6 blocks * 256 samples)
    uint8_t      *samples[6];                     // pointers to the start of each plane (1 per channel)

    int                 nchannels;
    int                 use_mix_levels;
    uint64_t            channel_layout;
    hb_audio_remap_t    *remap;
    hb_audio_resample_t *resample;
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

/* Translate acmod and lfeon on AV_CH_LAYOUT */
static const uint64_t acmod2layout[] =
{
    AV_CH_LAYOUT_STEREO,         // A52_CHANNEL   (0)
    AV_CH_LAYOUT_MONO,           // A52_MONO      (1)
    AV_CH_LAYOUT_STEREO,         // A52_STEREO    (2)
    AV_CH_LAYOUT_SURROUND,       // A52_3F        (3)
    AV_CH_LAYOUT_2_1,            // A52_2F1R      (4)
    AV_CH_LAYOUT_4POINT0,        // A52_3F1R      (5)
    AV_CH_LAYOUT_2_2,            // A52_2F2R      (6)
    AV_CH_LAYOUT_5POINT0,        // A52_3F2R      (7)
    AV_CH_LAYOUT_MONO,           // A52_CHANNEL1  (8)
    AV_CH_LAYOUT_MONO,           // A52_CHANNEL2  (9)
    AV_CH_LAYOUT_STEREO_DOWNMIX, // A52_DOLBY    (10)
    AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_STEREO,     // A52_CHANNEL_MASK (15)
};

static const uint64_t lfeon2layout[] =
{
    0,
    AV_CH_LOW_FREQUENCY,
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
static int deca52Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    hb_audio_t *audio = w->audio;
    w->private_data = pv;

    pv->job       = job;
    pv->state     = a52_init(0);
    pv->list      = hb_list_init();
    pv->crc_table = av_crc_get_table(AV_CRC_16_ANSI);

    /*
     * Decoding, remapping, downmixing
     */
    if (audio->config.out.codec != HB_ACODEC_AC3_PASS)
    {
        /*
         * Output AV_SAMPLE_FMT_FLT samples
         */
        pv->resample =
            hb_audio_resample_init(AV_SAMPLE_FMT_FLT,
                                   audio->config.out.mixdown,
                                   audio->config.out.normalize_mix_level);
        if (pv->resample == NULL)
        {
            hb_error("deca52Init: hb_audio_resample_init() failed");
            return 1;
        }

        /*
         * Decode to AV_SAMPLE_FMT_FLTP
         */
        pv->level = 1.0;
        pv->dynamic_range_compression =
            audio->config.out.dynamic_range_compression;
        hb_audio_resample_set_sample_fmt(pv->resample, AV_SAMPLE_FMT_FLTP);

        /*
         * liba52 doesn't provide Lt/Rt mix levels, only Lo/Ro.
         *
         * When doing an Lt/Rt downmix, ignore mix levels
         * (this matches what liba52's own downmix code does).
         */
        pv->use_mix_levels =
            !(audio->config.out.mixdown == HB_AMIXDOWN_DOLBY ||
              audio->config.out.mixdown == HB_AMIXDOWN_DOLBYPLII);

        /*
         * Remap from liba52 to Libav channel order
         */
        pv->remap = hb_audio_remap_init(AV_SAMPLE_FMT_FLTP, &hb_libav_chan_map,
                                        &hb_liba52_chan_map);
        if (pv->remap == NULL)
        {
            hb_error("deca52Init: hb_audio_remap_init() failed");
            return 1;
        }
    }
    else
    {
        pv->remap    = NULL;
        pv->resample = NULL;
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
static void deca52Close(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    w->private_data = NULL;

    if (pv->crc_errors)
    {
        hb_log("deca52: %d frames decoded, %d crc errors, %d bytes dropped",
               pv->frames, pv->crc_errors, pv->bytes_dropped);
    }

    hb_audio_resample_free(pv->resample);
    hb_audio_remap_free(pv->remap);
    hb_list_empty(&pv->list);
    a52_free(pv->state);
    free(pv);
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

    if ( (*buf_in)->s.start < -1 && pv->next_expected_pts == 0 )
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
static hb_buffer_t* Decode(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    hb_audio_t *audio = w->audio;
    hb_buffer_t *out;
    int size = 0;

    // check that we're at the start of a valid frame and align to the
    // start of a valid frame if we're not.
    // we have to check the header & crc so we need at least
    // 7 (the header size) + 128 (the minimum frame size) bytes
    while( hb_list_bytes( pv->list ) >= 7+128 )
    {
        /* check if this is a valid header */
        hb_list_seebytes( pv->list, pv->frame, 7 );
        size = a52_syncinfo(pv->frame, &pv->flags, &pv->rate, &pv->bitrate);
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

    double frame_dur = (6. * 256. * 90000.) / pv->rate;
    double pts       = (ipts != -1) ? (double)ipts : pv->next_expected_pts;

    /* AC3 passthrough: don't decode the AC3 frame */
    if (audio->config.out.codec == HB_ACODEC_AC3_PASS)
    {
        out = hb_buffer_init(size);
        memcpy(out->data, pv->frame, size);
    }
    else
    {
        int i, j;
        float *block_samples;

        /*
         * Feed liba52
         */
        a52_frame(pv->state, pv->frame, &pv->flags, &pv->level, 0);

        /*
         * If the user requested strong  DRC (>1), adjust it.
         * If the user requested default DRC  (1), leave it alone.
         * If the user requested no      DRC  (0), call a null function.
         *
         * a52_frame() resets the callback so it must be called for each frame.
         */
        if (pv->dynamic_range_compression > 1.0)
        {
            a52_dynrng(pv->state, dynrng_call, &pv->dynamic_range_compression);
        }
        else if (!pv->dynamic_range_compression)
        {
            a52_dynrng(pv->state, NULL, NULL);
        }

        /*
         * Update input channel layout, prepare remapping and downmixing
         */
        uint64_t new_layout = (acmod2layout[(pv->flags & A52_CHANNEL_MASK)] |
                               lfeon2layout[(pv->flags & A52_LFE) != 0]);

        if (new_layout != pv->channel_layout)
        {
            pv->channel_layout = new_layout;
            pv->nchannels      = av_get_channel_layout_nb_channels(new_layout);
            hb_audio_remap_set_channel_layout(pv->remap,
                                              pv->channel_layout,
                                              pv->nchannels);
            hb_audio_resample_set_channel_layout(pv->resample,
                                                 pv->channel_layout,
                                                 pv->nchannels);
        }
        if (pv->use_mix_levels)
        {
            hb_audio_resample_set_mix_levels(pv->resample,
                                             (double)pv->state->slev,
                                             (double)pv->state->clev);
        }
        if (hb_audio_resample_update(pv->resample))
        {
            hb_log("deca52: hb_audio_resample_update() failed");
            return NULL;
        }

        /*
         * decode all blocks before downmixing
         */
        for (i = 0; i < 6; i++)
        {
            a52_block(pv->state);
            block_samples = (float*)a52_samples(pv->state);

            /*
             * reset pv->samples (may have been modified by hb_audio_remap)
             *
             * copy samples to our internal buffer
             */
            for (j = 0; j < pv->nchannels; j++)
            {
                pv->samples[j] = (uint8_t*)pv->buf[j];
                memcpy(pv->buf[j][i], block_samples, 256 * sizeof(float));
                block_samples += 256;
            }
        }

        hb_audio_remap(pv->remap, pv->samples, 1536);
        out = hb_audio_resample(pv->resample, pv->samples, 1536);
    }

    if (out != NULL)
    {
        out->s.start          = pts;
        out->s.duration       = frame_dur;
        pts                  += frame_dur;
        out->s.stop           = pts;
        pv->next_expected_pts = pts;
    }
    return out;
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
            memmove( buf, buf + len - newlen, newlen );
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
    info->samples_per_frame = 1536;

    info->channel_layout = (acmod2layout[(flags & A52_CHANNEL_MASK)] |
                            lfeon2layout[(flags & A52_LFE) != 0]);

    // we remap to Libav order in Decode()
    info->channel_map = &hb_libav_chan_map;

    return 1;
}
