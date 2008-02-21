/* $Id: declpcm.c,v 1.8 2005/11/04 14:44:01 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

int  declpcmInit( hb_work_object_t *, hb_job_t * );
int  declpcmWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void declpcmClose( hb_work_object_t * );

hb_work_object_t hb_declpcm =
{
    WORK_DECLPCM,
    "LPCM decoder",
    declpcmInit,
    declpcmWork,
    declpcmClose
};

int declpcmInit( hb_work_object_t * w, hb_job_t * job )
{
    return 0;
}

int declpcmWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * in = *buf_in, * out;
    int samplerate = 0;
    int count;
    uint8_t * samples_u8;
    float   * samples_fl32;
    int i;
    uint64_t duration;

    *buf_out = NULL;

    if( in->data[5] != 0x80 )
    {
        hb_log( "no LPCM frame sync (%02x)", in->data[5] );
        return HB_WORK_OK;
    }

    switch( ( in->data[4] >> 4 ) & 0x3 )
    {
        case 0:
            samplerate = 48000;
            break;
        case 1:
            samplerate = 96000;//32000; /* FIXME vlc says it is 96000 */
            break;
        case 2:
            samplerate = 44100;
            break;
        case 3:
            samplerate = 32000;
            break;
    }

    count       = ( in->size - 6 ) / 2;
    out         = hb_buffer_init( count * sizeof( float ) );
    duration    = count * 90000 / samplerate / 2;
    out->start  = in->start;
    out->stop   = out->start + duration;

    samples_u8   = in->data + 6;
    samples_fl32 = (float *) out->data;

    /* Big endian int16 -> float conversion */
    for( i = 0; i < count; i++ )
    {
#ifdef WORDS_BIGENDIAN
        samples_fl32[0] = *( (int16_t *) samples_u8 );
#else
        samples_fl32[0] = (int16_t) ( ( samples_u8[0] << 8 ) | samples_u8[1] );
#endif
        samples_u8   += 2;
        samples_fl32 += 1;
    }

    *buf_out = out;

    return HB_WORK_OK;
}

void declpcmClose( hb_work_object_t * w )
{
}
