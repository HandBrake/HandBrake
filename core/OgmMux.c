/* $Id: OgmMux.c,v 1.13 2004/05/13 21:10:56 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include <ogg/ogg.h>

static int OgmStart( HBMux * );
static int OgmMux( HBMux *, void *, HBBuffer * );
static int OgmEnd( HBMux * );

struct HBMux
{
    HB_MUX_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle  * title;
    FILE     * file;
};

typedef struct
{
    int              codec;
    ogg_stream_state os;
    int              i_packet_no;

} OgmMuxData;

typedef struct __attribute__((__packed__))
{
    uint8_t i_packet_type;

    char stream_type[8];
    char sub_type[4];

    int32_t i_size;

    int64_t i_time_unit;
    int64_t i_samples_per_unit;
    int32_t i_default_len;

    int32_t i_buffer_size;
    int16_t i_bits_per_sample;
    int16_t i_padding_0;            // hum hum
    union
    {
        struct
        {
            int32_t i_width;
            int32_t i_height;

        } video;
        struct
        {
            int16_t i_channels;
            int16_t i_block_align;
            int32_t i_avgbytespersec;
        } audio;
    } header;

} ogg_stream_header_t;

#define SetWLE( p, v ) _SetWLE( (uint8_t*)p, v)
static void _SetWLE( uint8_t *p, uint16_t i_dw )
{
    p[1] = ( i_dw >>  8 )&0xff;
    p[0] = ( i_dw       )&0xff;
}

#define SetDWLE( p, v ) _SetDWLE( (uint8_t*)p, v)
static void _SetDWLE( uint8_t *p, uint32_t i_dw )
{
    p[3] = ( i_dw >> 24 )&0xff;
    p[2] = ( i_dw >> 16 )&0xff;
    p[1] = ( i_dw >>  8 )&0xff;
    p[0] = ( i_dw       )&0xff;
}
#define SetQWLE( p, v ) _SetQWLE( (uint8_t*)p, v)
static void _SetQWLE( uint8_t *p, uint64_t i_qw )
{
    SetDWLE( p,   i_qw&0xffffffff );
    SetDWLE( p+4, ( i_qw >> 32)&0xffffffff );
}

HBMux * HBOgmMuxInit( HBHandle * handle, HBTitle * title )
{
    HBMux   * m;
    HBAudio * audio;
    int       i;

    if( !( m = calloc( sizeof( HBMux ), 1 ) ) )
    {
        HBLog( "HBOgmMux: malloc failed, gonna crash" );
        return NULL;
    }
    m->start    = OgmStart;
    m->muxVideo = OgmMux;
    m->muxAudio = OgmMux;
    m->end      = OgmEnd;

    m->handle   = handle;
    m->title    = title;

    /* Alloc muxer data */
    title->muxData = calloc( sizeof( OgmMuxData ), 1 );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        audio->muxData = calloc( sizeof( OgmMuxData ), 1 );
    }

    return m;
}

void HBOgmMuxClose( HBMux ** _m )
{
    HBMux   * m     = *_m;
    HBTitle * title = m->title;
    HBAudio * audio;
    int       i;

    /* Free muxer data */
    free( title->muxData );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        free( audio->muxData );
    }

    free( m );
    *_m = NULL;
}

static int OgmFlush( HBMux * m, OgmMuxData * muxData )
{
    for( ;; )
    {
        ogg_page og;
        if( ogg_stream_flush( &muxData->os, &og ) == 0 )
        {
            break;
        }
        if( fwrite( og.header, og.header_len, 1, m->file ) <= 0 ||
            fwrite( og.body, og.body_len, 1, m->file ) <= 0 )
        {
            return -1;
        }
    }
    return 0;
}

static int OgmStart( HBMux * m )
{
    HBTitle           * title = m->title;
    HBAudio           * audio;
    OgmMuxData        * muxData;
    ogg_packet          op;
    ogg_stream_header_t h;
    int                 i;

    /* Open output file */
    if( ( m->file = fopen( title->file, "wb" ) ) == NULL )
    {
        HBLog( "HBOgmMux: failed to open `%s'", title->file );
        /* FIXME */
        HBErrorOccured( m->handle, HB_ERROR_AVI_WRITE );
        return -1;
    }
    HBLog( "HBOgmMux: `%s' opened", title->file );

    /* Init tracks */

    /* Video */
    muxData              = (OgmMuxData *) title->muxData;
    muxData->codec       = title->codec;
    muxData->i_packet_no = 0;
    ogg_stream_init( &muxData->os, 0 );

    /* Audio */
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        HBAudio * audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        muxData = (OgmMuxData *) audio->muxData;

        muxData->codec = audio->outCodec;
        muxData->i_packet_no = 0;
        ogg_stream_init( &muxData->os, i + 1 );

    }

    /* First pass: all b_o_s packets */

    /* Video */
    muxData = (OgmMuxData *) title->muxData;
    memset( &h, 0, sizeof( ogg_stream_header_t ) );
    h.i_packet_type = 0x01;
    memcpy( h.stream_type, "video    ", 8 );
    if( muxData->codec == HB_CODEC_X264 )
    {
        memcpy( h.sub_type, "H264", 4 );
    }
    else
    {
        memcpy( h.sub_type, "XVID", 4 );
    }
    SetDWLE( &h.i_size, sizeof( ogg_stream_header_t ) - 1);
    SetQWLE( &h.i_time_unit, (int64_t)10*1000*1000*(int64_t)title->rateBase/(int64_t)title->rate );
    SetQWLE( &h.i_samples_per_unit, 1 );
    SetDWLE( &h.i_default_len, 0 );
    SetDWLE( &h.i_buffer_size, 1024*1024 );
    SetWLE ( &h.i_bits_per_sample, 0 );
    SetDWLE( &h.header.video.i_width,  title->outWidth );
    SetDWLE( &h.header.video.i_height, title->outHeight );
    op.packet   = (char*)&h;
    op.bytes    = sizeof( ogg_stream_header_t );
    op.b_o_s    = 1;
    op.e_o_s    = 0;
    op.granulepos = 0;
    op.packetno = muxData->i_packet_no++;
    ogg_stream_packetin( &muxData->os, &op );
    OgmFlush( m, muxData );

    /* Audio */
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio   = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        muxData = (OgmMuxData *) audio->muxData;
        memset( &h, 0, sizeof( ogg_stream_header_t ) );
        switch( muxData->codec )
        {
            case HB_CODEC_MP3:
            {
                h.i_packet_type = 0x01;
                memcpy( h.stream_type, "audio    ", 8 );
                memcpy( h.sub_type, "55  ", 4 );

                SetDWLE( &h.i_size, sizeof( ogg_stream_header_t ) - 1);
                SetQWLE( &h.i_time_unit, 0 );
                SetQWLE( &h.i_samples_per_unit, audio->outSampleRate );
                SetDWLE( &h.i_default_len, 1 );
                SetDWLE( &h.i_buffer_size, 30*1024 );
                SetWLE ( &h.i_bits_per_sample, 0 );

                SetDWLE( &h.header.audio.i_channels, 2 );
                SetDWLE( &h.header.audio.i_block_align, 0 );
                SetDWLE( &h.header.audio.i_avgbytespersec,
                         audio->outBitrate / 8 );

                op.packet   = (char*)&h;
                op.bytes    = sizeof( ogg_stream_header_t );
                op.b_o_s    = 1;
                op.e_o_s    = 0;
                op.granulepos = 0;
                op.packetno = muxData->i_packet_no++;
                ogg_stream_packetin( &muxData->os, &op );
                break;
            }
            case HB_CODEC_VORBIS:
            {
                HBBuffer *h = HBFifoPop( audio->outFifo );

                memcpy( &op, h->data, sizeof( ogg_packet ) );
                op.packet = h->data + sizeof( ogg_packet );
                ogg_stream_packetin( &muxData->os, &op );
                break;
            }
            default:
                HBLog( "HBOgmMux: unhandled codec" );
                break;
        }
        OgmFlush( m, muxData );
    }

    /* second pass: all non b_o_s packets */
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        if( audio->outCodec == HB_CODEC_VORBIS )
        {
            HBBuffer *h;
            int       j;
            muxData = (OgmMuxData *) audio->muxData;

            for( j = 0; j < 2; j++ )
            {
                h = HBFifoPop( audio->outFifo );

                memcpy( &op, h->data, sizeof( ogg_packet ) );
                op.packet = h->data + sizeof( ogg_packet );
                ogg_stream_packetin( &muxData->os, &op );

                OgmFlush( m, muxData );
            }
        }
    }

    HBLog( "HBOgmMux: headers written" );
    return 0;
}

static int OgmMux( HBMux * m, void * _muxData, HBBuffer * buffer )
{
    OgmMuxData * muxData = (OgmMuxData *) _muxData;
    ogg_packet   op;

    switch( muxData->codec )
    {
        case HB_CODEC_FFMPEG:
        case HB_CODEC_XVID:
        case HB_CODEC_X264:
            op.bytes  = buffer->size + 1;
            op.packet = malloc( op.bytes );
            op.packet[0] = buffer->keyFrame ? 0x08 : 0x00;
            memcpy( &op.packet[1], buffer->data, buffer->size );
            op.b_o_s       = 0;
            op.e_o_s       = 0;
            op.granulepos  = muxData->i_packet_no;
            op.packetno    = muxData->i_packet_no++;
            break;
        case HB_CODEC_MP3:
            op.bytes  = buffer->size + 1;
            op.packet = malloc( op.bytes );
            op.packet[0] = 0x08;
            memcpy( &op.packet[1], buffer->data, buffer->size );
            op.b_o_s       = 0;
            op.e_o_s       = 0;
            op.granulepos  = muxData->i_packet_no * 1152;
            op.packetno    = muxData->i_packet_no++;
            break;
        case HB_CODEC_VORBIS:
            memcpy( &op, buffer->data, sizeof( ogg_packet ) );
            op.packet = malloc( op.bytes );
            memcpy( op.packet, buffer->data + sizeof( ogg_packet ), op.bytes );
            break;

        default:
            HBLog( "HBOgmMux: unhandled codec" );
            op.bytes = 0;
            op.packet = NULL;
            break;
    }

    if( op.packet )
    {
        ogg_stream_packetin( &muxData->os, &op );

        for( ;; )
        {
            ogg_page og;
            if( ogg_stream_pageout( &muxData->os, &og ) == 0 )
            {
                break;
            }

            if( fwrite( og.header, og.header_len, 1, m->file ) <= 0 ||
                fwrite( og.body, og.body_len, 1, m->file ) <= 0 )
            {
                HBLog( "HBOgmMux: write failed" );
                break;
            }
        }
        free( op.packet );
    }
    return 0;
}

static int OgmEnd( HBMux * m )
{
    HBTitle    * title = m->title;
    HBAudio    * audio;
    OgmMuxData * muxData;
    int          i;

    muxData = (OgmMuxData *) title->muxData;
    if( OgmFlush( m, muxData ) < 0 )
    {
        return -1;
    }
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        muxData = (OgmMuxData *) audio->muxData;
        if( OgmFlush( m, muxData ) < 0 )
        {
            return -1;
        }
    }

    fclose( m->file );
    HBLog( "HBOgmMux: `%s' closed", title->file );
    return 0;
}

