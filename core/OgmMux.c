/* $Id: OgmMux.c,v 1.6 2004/02/13 15:12:09 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include <ogg/ogg.h>

static void OgmMuxThread( void * );
static int  OgmStart( HBOgmMux * );
static int  OgmFlush( HBOgmMux *, int );
static int  OgmEnd( HBOgmMux * );

struct HBOgmMux
{
    HBHandle        *handle;
    HBTitle         *title;

    volatile int    die;
    HBThread        *thread;

    FILE            *file;

    int             i_tk;
    struct
    {
        HBFifo           *fifo;

        int               codec;

        ogg_stream_state os;
        int              i_packet_no;

    } tk[100];  /* The first who set more than 100 stream !! */
};

HBOgmMux * HBOgmMuxInit( HBHandle * handle, HBTitle * title )
{
    HBOgmMux *ogm = malloc( sizeof( HBOgmMux ) );

    ogm->handle = handle;
    ogm->title  = title;

    ogm->file   = NULL;
    ogm->i_tk   = 0;

    ogm->die    = 0;
    ogm->thread = HBThreadInit( "ogm muxer", OgmMuxThread, ogm, HB_NORMAL_PRIORITY );
    return ogm;
}

void HBOgmMuxClose( HBOgmMux ** _ogm )
{
    HBOgmMux *ogm = *_ogm;

    ogm->die = 1;
    HBThreadClose( &ogm->thread );

    free( ogm );

    *_ogm = NULL;
}

static int OgmDataWait( HBOgmMux *ogm )
{
    int i;

    for( i = 0; i < ogm->i_tk; i++ )
    {
        while( !ogm->die && HBFifoSize( ogm->tk[i].fifo ) <= 0 )
        {
            HBSnooze( 10000 );
        }
    }
    return ogm->die ? -1 : 0;
}

static void OgmMuxThread( void * _this )
{
    HBOgmMux *ogm = _this;

    HBTitle  *title = ogm->title;

    int i;

    /* Open output file */
    if( ( ogm->file = fopen( title->file, "w" ) ) == NULL )
    {
        HBLog( "HBOgmMux: failed to open `%s'", title->file );
        /* FIXME */
        HBErrorOccured( ogm->handle, HB_ERROR_AVI_WRITE );
        return;
    }
    HBLog( "HBOgmMux: `%s' opened", title->file );


    /* Wait for data in each fifo */
    HBLog( "HBOgmMux: waiting video/audio data" );
    if( OgmDataWait( ogm ) < 0 )
    {
        HBLog( "HBOgmMux: exiting" );
        fclose( ogm->file );
        unlink( title->file );
        return;
    }

    if( OgmStart( ogm ) < 0 )
    {
        HBLog( "HBOgmMux: failed to write headers" );
        fclose( ogm->file );
        unlink( title->file );
        return;
    }

    HBLog( "HBOgmMux: headers written" );

    for( ;; )
    {
        HBBuffer    *buffer;
        ogg_packet  op;
        int         i_tk;

        /* Wait data */
        if( OgmDataWait( ogm ) < 0 )
        {
            break;
        }

        /* Choose the right track to write (interleaved data) */
        for( i = 0, i_tk = -1; i < ogm->i_tk; i++ )
        {
            if( i_tk < 0 ||
                HBFifoPosition( ogm->tk[i].fifo ) < HBFifoPosition( ogm->tk[i_tk].fifo ) )
            {
                i_tk = i;
            }
        }

        buffer = HBFifoPop( ogm->tk[i_tk].fifo );

        switch( ( ogm->tk[i_tk].codec ) )
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
                op.granulepos  = ogm->tk[i_tk].i_packet_no;
                op.packetno    = ogm->tk[i_tk].i_packet_no++;
                break;
            case HB_CODEC_MP3:
                op.bytes  = buffer->size + 1;
                op.packet = malloc( op.bytes );
                op.packet[0] = 0x08;
                memcpy( &op.packet[1], buffer->data, buffer->size );
                op.b_o_s       = 0;
                op.e_o_s       = 0;
                op.granulepos  = ogm->tk[i_tk].i_packet_no * 1152;
                op.packetno    = ogm->tk[i_tk].i_packet_no++;
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
            ogg_stream_packetin( &ogm->tk[i_tk].os, &op );

            for( ;; )
            {
                ogg_page og;
                if( ogg_stream_pageout( &ogm->tk[i_tk].os, &og ) == 0 )
                {
                    break;
                }

                if( fwrite( og.header, og.header_len, 1, ogm->file ) <= 0 ||
                    fwrite( og.body, og.body_len, 1, ogm->file ) <= 0 )
                {
                    HBLog( "HBOgmMux: write failed" );
                    break;
                }
            }
            free( op.packet );
        }

        HBBufferClose( &buffer );
    }

    if( OgmEnd( ogm ) < 0 )
    {
        HBLog( "HBOgmMux: flush failed" );
    }

    fclose( ogm->file );
    HBLog( "HBOgmMux: `%s' closed", title->file );
}

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

static int  OgmFlush( HBOgmMux *ogm, int i_tk )
{
    for( ;; )
    {
        ogg_page og;
        if( ogg_stream_flush( &ogm->tk[i_tk].os, &og ) == 0 )
        {
            break;
        }
        if( fwrite( og.header, og.header_len, 1, ogm->file ) <= 0 ||
            fwrite( og.body, og.body_len, 1, ogm->file ) <= 0 )
        {
            return -1;
        }
    }
    return 0;
}

static int  OgmStart( HBOgmMux *ogm )
{
    HBTitle  *title = ogm->title;
    int      i;

    ogg_packet      op;

    /* Init track */
    ogm->tk[0].codec    = title->codec;
    ogm->tk[0].fifo     = title->outFifo;
    ogm->tk[0].i_packet_no = 0;
    ogg_stream_init (&ogm->tk[0].os, 0 );

    for( i = 1; i < HBListCount( title->ripAudioList ) + 1; i++ )
    {
        HBAudio *audio = HBListItemAt( title->ripAudioList, i - 1 );

        ogm->tk[i].codec   = audio->codec;
        ogm->tk[i].fifo    = audio->outFifo;
        ogm->tk[i].i_packet_no = 0;
        ogg_stream_init (&ogm->tk[i].os, i );

    }
    ogm->i_tk = 1 + HBListCount( title->ripAudioList );

    /* Wait data for each track */
    for( i = 0; i < ogm->i_tk; i++ )
    {
        while( !ogm->die &&
               ( ( ogm->tk[i].codec == HB_CODEC_VORBIS && HBFifoSize( ogm->tk[i].fifo ) <= 3 ) ||
                 HBFifoSize( ogm->tk[i].fifo ) <= 0 ) )
        {
            HBSnooze( 10000 );
        }
    }

    if( ogm->die )
    {
        return -1;
    }

    /* First pass: all b_o_s packets */
    for( i = 0; i < ogm->i_tk; i++ )
    {
        ogg_stream_header_t h;

        memset( &h, 0, sizeof( ogg_stream_header_t ) );

        switch( ogm->tk[i].codec )
        {
            case HB_CODEC_FFMPEG:
            case HB_CODEC_XVID:
            case HB_CODEC_X264:
                h.i_packet_type = 0x01;
                memcpy( h.stream_type, "video    ", 8 );
                if( ogm->tk[i].codec == HB_CODEC_X264 )
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
                op.packetno = ogm->tk[i].i_packet_no++;
                ogg_stream_packetin( &ogm->tk[i].os, &op );
                break;

            case HB_CODEC_MP3:
            {
                HBAudio *audio = HBListItemAt( title->ripAudioList, i - 1 );

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
                SetDWLE( &h.header.audio.i_avgbytespersec, audio->outBitrate / 8 );


                op.packet   = (char*)&h;
                op.bytes    = sizeof( ogg_stream_header_t );
                op.b_o_s    = 1;
                op.e_o_s    = 0;
                op.granulepos = 0;
                op.packetno = ogm->tk[i].i_packet_no++;
                ogg_stream_packetin( &ogm->tk[i].os, &op );
                break;
            }
            case HB_CODEC_VORBIS:
            {
                HBBuffer *h = HBFifoPop( ogm->tk[i].fifo );

                memcpy( &op, h->data, sizeof( ogg_packet ) );
                op.packet = h->data + sizeof( ogg_packet );
                ogg_stream_packetin( &ogm->tk[i].os, &op );
                break;
            }
            case HB_CODEC_AAC:
                break;
            default:
                HBLog( "unhandled codec" );
                break;
        }
        OgmFlush( ogm, i );
    }

    /* second pass: all non b_o_s packets */
    for( i = 0; i < ogm->i_tk; i++ )
    {
        if( ogm->tk[i].codec == HB_CODEC_VORBIS )
        {
            HBBuffer *h;
            int       j;

            for( j = 0; j < 2; j++ )
            {
                HBFifoWait( ogm->tk[i].fifo );
                h = HBFifoPop( ogm->tk[i].fifo );

                memcpy( &op, h->data, sizeof( ogg_packet ) );
                op.packet = h->data + sizeof( ogg_packet );
                ogg_stream_packetin( &ogm->tk[i].os, &op );

                OgmFlush( ogm, i );
            }
        }
#if 0
        else
        {
            /* Home made commentary */
            op.packet     = "\003Handbrake";
            op.bytes      = strlen( "\003Handbrake" );;
            op.b_o_s      = 0;
            op.e_o_s      = 0;
            op.granulepos = 0;
            op.packetno   = ogm->tk[i].i_packet_no++;

            ogg_stream_packetin( &ogm->tk[i].os, &op );
            OgmFlush( ogm, i );
        }
#endif
    }

    return 0;
}

static int  OgmEnd( HBOgmMux *ogm )
{
    int i;

    for( i = 0; i < ogm->i_tk; i++ )
    {
        if( OgmFlush( ogm, i ) < 0 )
        {
            return -1;
        }
    }
    return 0;
}

