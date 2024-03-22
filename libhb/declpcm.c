/* declpcm.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/audio_resample.h"

struct hb_work_private_s
{
    hb_job_t    *job;
    uint32_t    size;       /* frame size in bytes */
    uint32_t    nchunks;     /* number of samples pairs if paired */
    uint32_t    nsamples;   /* frame size in samples */
    uint32_t    pos;        /* buffer offset for next input data */

    int64_t     next_pts;   /* pts for next output frame */
    int         scr_sequence;

    /* the following is frame info for the frame we're currently accumulating */
    uint64_t    duration;   /* frame duration (in 90KHz ticks) */
    uint32_t    offset;     /* where in buf frame starts */
    uint32_t    samplerate; /* sample rate in bits/sec */
    uint8_t     nchannels;
    uint8_t     sample_size; /* bits per sample */

    uint8_t     frame[HB_DVD_READ_BUFFER_SIZE*2];
    uint8_t   * data;
    uint32_t    alloc_size;

    hb_audio_resample_t *resample;
};

static hb_buffer_t * Decode( hb_work_object_t * w );
static int  declpcmInit( hb_work_object_t *, hb_job_t * );
static int  declpcmWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void declpcmClose( hb_work_object_t * );
static int  declpcmBSInfo( hb_work_object_t *, const hb_buffer_t *,
                           hb_work_info_t * );

hb_work_object_t hb_declpcm =
{
    WORK_DECLPCM,
    "LPCM decoder",
    declpcmInit,
    declpcmWork,
    declpcmClose,
    0,
    declpcmBSInfo
};

static const int hdr2samplerate[] = { 48000, 96000, 44100, 32000 };
static const int hdr2samplesize[] = { 16, 20, 24, 16 };
static const uint64_t hdr2layout[] =
{
    AV_CH_LAYOUT_MONO,         AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_2_1,          AV_CH_LAYOUT_QUAD,
    AV_CH_LAYOUT_5POINT0_BACK, AV_CH_LAYOUT_6POINT0_FRONT,
    AV_CH_LAYOUT_6POINT1,      AV_CH_LAYOUT_7POINT1,
};

static void lpcmInfo( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t * pv = w->private_data;

    /*
     * LPCM packets have a 7 byte header (the substream id is stripped off
     * before we get here so it's numbered -1 below)::
     * byte -1  Substream id
     * byte 0   Number of frames that begin in this packet
     *          (last frame may finish in next packet)
     * byte 1,2 offset to first frame that begins in this packet (not including hdr)
     * byte 3:
     *   bits 0-4  continuity counter (increments modulo 20)
     *   bit   5   reserved
     *   bit   6   audio mute on/off
     *   bit   7   audio emphasis on/off
     * byte 4:
     *   bits 0-2  #channels - 1 (e.g., stereo = 1)
     *   bit   3   reserved
     *   bits 4-5  sample rate (0=48K,1=96K,2=44.1K,3=32K)
     *   bits 6-7  bits per sample (0=16 bit, 1=20 bit, 2=24 bit)
     * byte 5   Dynamic range control (0x80 = off)
     *
     * The audio is viewed as "frames" of 150 90KHz ticks each (80 samples @ 48KHz).
     * The frames are laid down continuously without regard to MPEG packet
     * boundaries. E.g., for 48KHz stereo, the first packet will contain 6
     * frames plus the start of the 7th, the second packet will contain the
     * end of the 7th, 8-13 & the start of 14, etc. The frame structure is
     * important because the PTS on the packet gives the time of the first
     * frame that starts in the packet *NOT* the time of the first sample
     * in the packet. Also samples get split across packet boundaries
     * so we can't assume that we can consume all the data in one packet
     * on every call to the work routine.
     */
    pv->offset = ( ( in->data[1] << 8 ) | in->data[2] ) + 2;
    if ( pv->offset >= HB_DVD_READ_BUFFER_SIZE )
    {
        hb_log( "declpcm: illegal frame offset %d", pv->offset );
        pv->offset = 2; /*XXX*/
    }
    pv->nchannels   = ( in->data[4] & 7 ) + 1;
    pv->samplerate  = hdr2samplerate[ ( in->data[4] >> 4 ) & 0x3 ];
    pv->sample_size = hdr2samplesize[in->data[4] >> 6];

    // 20 and 24 bit lpcm is always encoded in sample pairs.  So take this
    // into account when computing sizes.
    int chunk_size = pv->sample_size / 8;
    int samples_per_chunk = 1;

    switch( pv->sample_size )
    {
        case 20:
            chunk_size = 5;
            samples_per_chunk = 2;
            break;
        case 24:
            chunk_size = 6;
            samples_per_chunk = 2;
            break;
    }

    /*
     * PCM frames have a constant duration (150 90KHz ticks).
     * We need to convert that to the amount of data expected.  It's the
     * duration divided by the sample rate (to get #samples) times the number
     * of channels times the bits per sample divided by 8 to get bytes.
     * (we have to compute in bits because 20 bit samples are not an integral
     * number of bytes). We do all the multiplies first then the divides to
     * avoid truncation errors.
     */
    /*
     * Don't trust the number of frames given in the header.  We've seen
     * streams for which this is incorrect, and it can be computed.
     * pv->duration = in->data[0] * 150;
     */
    int chunks = ( in->size - pv->offset ) / chunk_size;
    int samples = chunks * samples_per_chunk;

    // Calculate number of frames that start in this packet
    int frames = ( 90000 * samples / ( pv->samplerate * pv->nchannels ) +
                   149 ) / 150;

    pv->duration = frames * 150;
    pv->nchunks =  ( pv->duration * pv->nchannels * pv->samplerate +
                    samples_per_chunk - 1 ) / ( 90000 * samples_per_chunk );
    pv->nsamples = ( pv->duration * pv->samplerate ) / 90000;
    pv->size = pv->nchunks * chunk_size;

    if (in->s.start != AV_NOPTS_VALUE)
    {
        pv->next_pts     = in->s.start;
    }
    pv->scr_sequence = in->s.scr_sequence;
}

static int declpcmInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    if (pv == NULL)
    {
        hb_error("declpcmInit: hb_audio_resample_init() failed");
        return 1;
    }
    w->private_data = pv;
    pv->job = job;

    pv->next_pts = (int64_t)AV_NOPTS_VALUE;
    // Currently, samplerate conversion is performed in sync.c
    // So set output samplerate to input samplerate
    // This should someday get reworked to be part of an audio filter pipeline.
    pv->resample =
        hb_audio_resample_init(AV_SAMPLE_FMT_FLT,
                               w->audio->config.in.samplerate,
                               w->audio->config.out.mixdown,
                               w->audio->config.out.normalize_mix_level);
    if (pv->resample == NULL)
    {
        hb_error("declpcmInit: hb_audio_resample_init() failed");
        return 1;
    }

    return 0;
}

/*
 * Convert DVD encapsulated LPCM to floating point PCM audio buffers.
 * The amount of audio in a PCM frame is always <= the amount that will fit
 * in a DVD block (2048 bytes) but the standard doesn't require that the audio
 * frames line up with the DVD frames. Since audio frame boundaries are unrelated
 * to DVD PES boundaries, this routine has to reconstruct then extract the audio
 * frames. Because of the arbitrary alignment, it can output zero, one or two buf's.
 */
static int declpcmWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    hb_buffer_t *buf = NULL;
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    // if we have a frame to finish, add enough data from this buf
    // to finish it
    if (pv->size)
    {
        memcpy(pv->frame + pv->pos, in->data + 6, pv->size - pv->pos);
        buf = Decode( w );
        hb_buffer_list_append(&list, buf);
    }

    /* save the (rest of) data from this buf in our frame buffer */
    lpcmInfo( w, in );
    int off = pv->offset;
    int amt = in->size - off;
    pv->pos = amt;
    memcpy( pv->frame, in->data + off, amt );
    if (amt >= pv->size)
    {
        buf = Decode( w );
        hb_buffer_list_append(&list, buf);
        pv->size = 0;
    }

    *buf_out = hb_buffer_list_clear(&list);
    return HB_WORK_OK;
}

static hb_buffer_t *Decode( hb_work_object_t *w )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *out;

    if (pv->nsamples == 0)
        return NULL;

    int size = pv->nsamples * pv->nchannels * sizeof( float );
    if (pv->alloc_size != size)
    {
        pv->data = realloc( pv->data, size );
        pv->alloc_size = size;
    }

    float *odat = (float *)pv->data;
    int count = pv->nchunks / pv->nchannels;

    switch( pv->sample_size )
    {
        case 16: // 2 byte, big endian, signed (the right shift sign extends)
        {
            uint8_t *frm = pv->frame;
            while ( count-- )
            {
                int cc;
                for( cc = 0; cc < pv->nchannels; cc++ )
                {
                    // Shifts below result in sign extension which gives
                    // us proper signed values. The final division adjusts
                    // the range to [-1.0 ... 1.0]
                    *odat++ = (float)( ( (int)( frm[0] << 24 ) >> 16 ) |
                                       frm[1] ) / 32768.0;
                    frm += 2;
                }
            }
        } break;
        case 20:
        {
            // There will always be 2 groups of samples.  A group is
            // a collection of samples that spans all channels.
            // The data for the samples is split.  The first 2 msb
            // bytes for all samples is encoded first, then the remaining
            // lsb bits are encoded.
            uint8_t *frm = pv->frame;
            while ( count-- )
            {
                int gg, cc;
                int shift = 4;
                uint8_t *lsb = frm + 4 * pv->nchannels;
                for( gg = 0; gg < 2; gg++ )
                {
                    for( cc = 0; cc < pv->nchannels; cc++ )
                    {
                        // Shifts below result in sign extension which gives
                        // us proper signed values. The final division adjusts
                        // the range to [-1.0 ... 1.0]
                        *odat = (float)( ( (int)( frm[0] << 24 ) >> 12 ) |
                                         ( frm[1] << 4 ) |
                                         ( ( ( lsb[0] >> shift ) & 0x0f ) ) ) /
                                       (16. * 32768.0);
                        odat++;
                        lsb += !shift;
                        shift ^= 4;
                        frm += 2;
                    }
                }
                frm = lsb;
            }
        } break;
        case 24:
        {
            // There will always be 2 groups of samples.  A group is
            // a collection of samples that spans all channels.
            // The data for the samples is split.  The first 2 msb
            // bytes for all samples is encoded first, then the remaining
            // lsb bits are encoded.
            uint8_t *frm = pv->frame;
            while ( count-- )
            {
                int gg, cc;
                uint8_t *lsb = frm + 4 * pv->nchannels;
                for( gg = 0; gg < 2; gg++ )
                {
                    for( cc = 0; cc < pv->nchannels; cc++ )
                    {
                        // Shifts below result in sign extension which gives
                        // us proper signed values. The final division adjusts
                        // the range to [-1.0 ... 1.0]
                        *odat++ = (float)( ( (int)( frm[0] << 24 ) >> 8 ) |
                                           ( frm[1] << 8 ) | lsb[0] ) /
                                  (256. * 32768.0);
                        frm += 2;
                        lsb++;
                    }
                }
                frm = lsb;
            }
        } break;
    }

    AVChannelLayout channel_layout;
    av_channel_layout_default(&channel_layout, hdr2layout[pv->nchannels - 1]);
    hb_audio_resample_set_ch_layout(pv->resample,
                                    &channel_layout);
    av_channel_layout_uninit(&channel_layout);

    hb_audio_resample_set_sample_rate(pv->resample,
                                      pv->samplerate);
    if (hb_audio_resample_update(pv->resample))
    {
        hb_log("declpcm: hb_audio_resample_update() failed");
        return NULL;
    }
    out = hb_audio_resample(pv->resample, (const uint8_t **)&pv->data,
                            pv->nsamples);

    if (out != NULL)
    {
        out->s.start         = pv->next_pts;
        out->s.duration      = pv->duration;
        if (pv->next_pts != (int64_t)AV_NOPTS_VALUE)
        {
            pv->next_pts        += pv->duration;
            out->s.stop          = pv->next_pts;
        }
        out->s.scr_sequence  = pv->scr_sequence;
    }
    return out;
}

static void declpcmClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        hb_audio_resample_free(pv->resample);
        free( pv->data );
        free( pv );
        w->private_data = 0;
    }
}

static int declpcmBSInfo( hb_work_object_t *w, const hb_buffer_t *b,
                          hb_work_info_t *info )
{
    int nchannels  = ( b->data[4] & 7 ) + 1;
    int sample_size = hdr2samplesize[b->data[4] >> 6];

    int rate = hdr2samplerate[ ( b->data[4] >> 4 ) & 0x3 ];
    int bitrate = rate * sample_size * nchannels;
    int64_t duration = b->data[0] * 150;

    memset( info, 0, sizeof(*info) );

    info->name = "LPCM";
    info->rate.num = rate;
    info->rate.den = 1;
    info->bitrate = bitrate;
    info->flags = ( b->data[3] << 16 ) | ( b->data[4] << 8 ) | b->data[5];
    info->matrix_encoding = AV_MATRIX_ENCODING_NONE;
    info->channel_layout = hdr2layout[nchannels - 1];
    info->channel_map = &hb_libav_chan_map;
    info->sample_bit_depth = sample_size;
    info->samples_per_frame = ( duration * rate ) / 90000;

    return 1;
}
