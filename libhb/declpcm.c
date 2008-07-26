/* $Id: declpcm.c,v 1.8 2005/11/04 14:44:01 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

struct hb_work_private_s
{
    hb_job_t    *job;
    uint32_t    size;       /* frame size in bytes */
    uint32_t    count;      /* frame size in samples */
    uint32_t    pos;        /* buffer offset for next input data */

    int64_t     next_pts;   /* pts for next output frame */
    int64_t     sequence;

    /* the following is frame info for the frame we're currently accumulating */
    uint64_t    duration;   /* frame duratin (in 90KHz ticks) */
    uint32_t    offset;     /* where in buf frame starts */
    uint32_t    samplerate; /* sample rate in bits/sec */
    uint8_t     nchannels;
    uint8_t     sample_size; /* bits per sample */

    uint8_t     frame[HB_DVD_READ_BUFFER_SIZE*2];
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
static const int hdr2layout[] = {
        HB_INPUT_CH_LAYOUT_MONO,   HB_INPUT_CH_LAYOUT_STEREO,
        HB_INPUT_CH_LAYOUT_2F1R,   HB_INPUT_CH_LAYOUT_2F2R,
        HB_INPUT_CH_LAYOUT_3F2R,   HB_INPUT_CH_LAYOUT_4F2R,
        HB_INPUT_CH_LAYOUT_STEREO, HB_INPUT_CH_LAYOUT_STEREO,
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
    pv->samplerate = hdr2samplerate[ ( in->data[4] >> 4 ) & 0x3 ];
    pv->nchannels  = ( in->data[4] & 7 ) + 1;
    pv->sample_size = hdr2samplesize[in->data[4] >> 6];

    /*
     * PCM frames have a constant duration (150 90KHz ticks).
     * We need to convert that to the amount of data expected.  It's the
     * duration divided by the sample rate (to get #samples) times the number
     * of channels times the bits per sample divided by 8 to get bytes.
     * (we have to compute in bits because 20 bit samples are not an integral
     * number of bytes). We do all the multiplies first then the divides to
     * avoid truncation errors. 
     */
    pv->duration = in->data[0] * 150;
    pv->count = ( pv->duration * pv->nchannels * pv->samplerate ) / 90000;
    pv->size = ( pv->count * pv->sample_size ) / 8;

    pv->next_pts = in->start;
}

static int declpcmInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job   = job;
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

    if ( in->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    pv->sequence = in->sequence;

    /* if we have a frame to finish, add enough data from this buf to finish it */
    if ( pv->size )
    {
        memcpy( pv->frame + pv->pos, in->data + 6, pv->size - pv->pos );
        buf = Decode( w );
    }
    *buf_out = buf;

    /* save the (rest of) data from this buf in our frame buffer */
    lpcmInfo( w, in );
    int off = pv->offset;
    int amt = in->size - off;
    pv->pos = amt;
    memcpy( pv->frame, in->data + off, amt );
    if ( amt >= pv->size )
    {
        if ( buf )
        {
            buf->next = Decode( w );
        }
        else
        {
            *buf_out = Decode( w );
        }
        pv->size = 0;
    }
    return HB_WORK_OK;
}

static hb_buffer_t *Decode( hb_work_object_t *w )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *out = hb_buffer_init( pv->count * sizeof( float ) );
 
    out->start  = pv->next_pts;
    pv->next_pts += pv->duration;
    out->stop = pv->next_pts;

    uint8_t *frm = pv->frame;
    float *odat = (float *)out->data;
    int count = pv->count;

    switch( pv->sample_size )
    {
        case 16: // 2 byte, big endian, signed (the right shift sign extends)
            while ( --count >= 0 )
            {
                *odat++ = ( (int)( frm[0] << 24 ) >> 16 ) | frm[1];
                frm += 2;
            }
            break;
        case 20:
            // 20 bit big endian signed (5 bytes for 2 samples = 2.5 bytes/sample
            // so we do two samples per iteration).
            count /= 2;
            while ( --count >= 0 )
            {
                *odat++ = (float)( ( (int)( frm[0] << 24 ) >> 12 ) |
                                   ( frm[1] << 4 ) | ( frm[2] >> 4 ) ) / 16.;
                *odat++ = (float)( ( (int)( frm[2] << 28 ) >> 16 ) |
                                   ( frm[3] << 8 ) | frm[4] ) / 16.;
                frm += 5;
            }
            break;
        case 24:
            // This format is bizarre. It's 24 bit samples but some confused
            // individual apparently thought they would be easier to interpret
            // as 16 bits if they were scrambled in the following way:
            // Things are stored in 4 sample (12 byte) chunks. Each chunk has
            // 4 samples containing the two top bytes of the actual samples in
            // 16 bit big-endian order followed by the four least significant bytes
            // of each sample.
            count /= 4; // the loop has to work in 4 sample chunks
            while ( --count >= 0 )
            {
                *odat++ = (float)( ( (int)( frm[0] << 24 ) >> 8 ) |
                            ( frm[1] << 8 ) | frm[8] ) / 256.;
                *odat++ = (float)( ( (int)( frm[2] << 24 ) >> 8 ) |
                            ( frm[3] << 8 ) | frm[9] ) / 256.;
                *odat++ = (float)( ( (int)( frm[4] << 24 ) >> 8 ) |
                            ( frm[5] << 8 ) | frm[10] ) / 256.;
                *odat++ = (float)( ( (int)( frm[6] << 24 ) >> 8 ) |
                            ( frm[7] << 8 ) | frm[11] ) / 256.;
                frm += 12;
            }
            break;
    }
    return out;
}

static void declpcmClose( hb_work_object_t * w )
{
    if ( w->private_data )
    {
        free( w->private_data );
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

    memset( info, 0, sizeof(*info) );

    info->name = "LPCM";
    info->rate = rate;
    info->rate_base = 1;
    info->bitrate = bitrate;
    info->flags = ( b->data[3] << 16 ) | ( b->data[4] << 8 ) | b->data[5];
    info->channel_layout = hdr2layout[nchannels - 1];

    return 1;
}
