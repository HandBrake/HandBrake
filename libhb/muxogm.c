/* $Id: muxogm.c,v 1.4 2005/02/20 00:41:56 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include <ogg/ogg.h>

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    FILE * file;
};

struct hb_mux_data_s
{
    int              codec;
    ogg_stream_state os;
    int              i_packet_no;
};

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

static int OGMFlush( hb_mux_object_t * m, hb_mux_data_t * mux_data )
{
    for( ;; )
    {
        ogg_page og;
        if( ogg_stream_flush( &mux_data->os, &og ) == 0 )
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

/**********************************************************************
 * OGMInit
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int OGMInit( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;

    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;
    int i;

    ogg_packet          op;
    ogg_stream_header_t h;

    /* Open output file */
    if( ( m->file = fopen( job->file, "wb" ) ) == NULL )
    {
        hb_log( "muxogm: failed to open `%s'", job->file );
        return -1;
    }
    hb_log( "muxogm: `%s' opened", job->file );

    /* Video track */
    mux_data              = malloc( sizeof( hb_mux_data_t ) );
    mux_data->codec       = job->vcodec;
    mux_data->i_packet_no = 0;
    job->mux_data         = mux_data;
    ogg_stream_init( &mux_data->os, 0 );

    /* Audio */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio                 = hb_list_item( title->list_audio, i );
        mux_data              = malloc( sizeof( hb_mux_data_t ) );
        mux_data->codec       = audio->config.out.codec;
        mux_data->i_packet_no = 0;
        audio->priv.mux_data       = mux_data;
        ogg_stream_init( &mux_data->os, i + 1 );
    }


    /* First pass: all b_o_s packets */
    hb_log("muxogm: Writing b_o_s header packets");
    /* Video */
    mux_data = job->mux_data;
    switch( job->vcodec )
    {
        case HB_VCODEC_THEORA:
            memcpy(&op, job->config.theora.headers[0], sizeof(op));
            op.packet = job->config.theora.headers[0] + sizeof(op);
            ogg_stream_packetin( &mux_data->os, &op );
            break;
        case HB_VCODEC_XVID:
        case HB_VCODEC_X264:
        case HB_VCODEC_FFMPEG:
        {
                memset( &h, 0, sizeof( ogg_stream_header_t ) );
                h.i_packet_type = 0x01;
                memcpy( h.stream_type, "video    ", 8 );
                if( mux_data->codec == HB_VCODEC_X264 )
                {
                    memcpy( h.sub_type, "H264", 4 );
                }
                else if( mux_data->codec == HB_VCODEC_XVID )
                {
                    memcpy( h.sub_type, "XVID", 4 );
                }
                else
                {
                    memcpy( h.sub_type, "DX50", 4 );
                }
                SetDWLE( &h.i_size, sizeof( ogg_stream_header_t ) - 1);
                SetQWLE( &h.i_time_unit, (int64_t) 10 * 1000 * 1000 *
                         (int64_t) job->vrate_base / (int64_t) job->vrate );
                SetQWLE( &h.i_samples_per_unit, 1 );
                SetDWLE( &h.i_default_len, 0 );
                SetDWLE( &h.i_buffer_size, 1024*1024 );
                SetWLE ( &h.i_bits_per_sample, 0 );
                SetDWLE( &h.header.video.i_width,  job->width );
                SetDWLE( &h.header.video.i_height, job->height );
                op.packet   = (unsigned char*)&h;
                op.bytes    = sizeof( ogg_stream_header_t );
                op.b_o_s    = 1;
                op.e_o_s    = 0;
                op.granulepos = 0;
                op.packetno = mux_data->i_packet_no++;
                ogg_stream_packetin( &mux_data->os, &op );
                break;
            }
        default:
            hb_error( "muxogm: unhandled video codec" );
            *job->die = 1;
    }
    OGMFlush( m, mux_data );

    /* Audio */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio   = hb_list_item( title->list_audio, i );
        mux_data = audio->priv.mux_data;
        memset( &h, 0, sizeof( ogg_stream_header_t ) );
        switch( audio->config.out.codec )
        {
            case HB_ACODEC_LAME:
            {
                h.i_packet_type = 0x01;
                memcpy( h.stream_type, "audio    ", 8 );
                memcpy( h.sub_type, "55  ", 4 );

                SetDWLE( &h.i_size, sizeof( ogg_stream_header_t ) - 1);
                SetQWLE( &h.i_time_unit, 0 );
                SetQWLE( &h.i_samples_per_unit, audio->config.out.samplerate );
                SetDWLE( &h.i_default_len, 1 );
                SetDWLE( &h.i_buffer_size, 30 * 1024 );
                SetWLE ( &h.i_bits_per_sample, 0 );

                SetDWLE( &h.header.audio.i_channels, 2 );
                SetDWLE( &h.header.audio.i_block_align, 0 );
                SetDWLE( &h.header.audio.i_avgbytespersec,
                         audio->config.out.bitrate / 8 );

                op.packet   = (unsigned char*) &h;
                op.bytes    = sizeof( ogg_stream_header_t );
                op.b_o_s    = 1;
                op.e_o_s    = 0;
                op.granulepos = 0;
                op.packetno = mux_data->i_packet_no++;
                ogg_stream_packetin( &mux_data->os, &op );
                break;
            }
            case HB_ACODEC_VORBIS:
            {
                memcpy( &op, audio->priv.config.vorbis.headers[0],
                        sizeof( ogg_packet ) );
                op.packet = audio->priv.config.vorbis.headers[0] +
                                sizeof( ogg_packet );
                ogg_stream_packetin( &mux_data->os, &op );
                break;
            }
            default:
                hb_log( "muxogm: unhandled codec" );
                break;
        }
        OGMFlush( m, mux_data );
    }

    /* second pass: all non b_o_s packets */
    hb_log("muxogm: Writing non b_o_s header packets");
    /* Video */
    mux_data = job->mux_data;
    switch( job->vcodec )
    {
        case HB_VCODEC_THEORA:
            for (i = 1; i < 3; i++)
            {
                memcpy(&op, job->config.theora.headers[i], sizeof(op));
                op.packet = job->config.theora.headers[i] + sizeof(op);
                ogg_stream_packetin( &mux_data->os, &op );
                OGMFlush( m, mux_data );
            }
            break;
        case HB_VCODEC_XVID:
        case HB_VCODEC_X264:
        case HB_VCODEC_FFMPEG:
            break;
        default:
            hb_error( "muxogm: unhandled video codec" );
            *job->die = 1;
    }

    /* Audio */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->config.out.codec == HB_ACODEC_VORBIS )
        {
            int       j;
            mux_data = audio->priv.mux_data;

            for( j = 1; j < 3; j++ )
            {
                memcpy( &op, audio->priv.config.vorbis.headers[j],
                        sizeof( ogg_packet ) );
                op.packet = audio->priv.config.vorbis.headers[j] +
                                sizeof( ogg_packet );
                ogg_stream_packetin( &mux_data->os, &op );

                OGMFlush( m, mux_data );
            }
        }
    }
    hb_log( "muxogm: headers written" );

    return 0;
}

static int OGMMux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    ogg_packet   op;

    switch( mux_data->codec )
    {
        case HB_VCODEC_THEORA:
            memcpy( &op, buf->data, sizeof( ogg_packet ) );
            op.packet = malloc( op.bytes );
            memcpy( op.packet, buf->data + sizeof( ogg_packet ), op.bytes );
            break;
        case HB_VCODEC_FFMPEG:
        case HB_VCODEC_XVID:
        case HB_VCODEC_X264:
            op.bytes  = buf->size + 1;
            op.packet = malloc( op.bytes );
            op.packet[0] = (buf->frametype & HB_FRAME_KEY) ? 0x08 : 0x00;
            memcpy( &op.packet[1], buf->data, buf->size );
            op.b_o_s       = 0;
            op.e_o_s       = 0;
            op.granulepos  = mux_data->i_packet_no;
            op.packetno    = mux_data->i_packet_no++;
            break;
        case HB_ACODEC_LAME:
            op.bytes  = buf->size + 1;
            op.packet = malloc( op.bytes );
            op.packet[0] = 0x08;
            memcpy( &op.packet[1], buf->data, buf->size );
            op.b_o_s       = 0;
            op.e_o_s       = 0;
            op.granulepos  = mux_data->i_packet_no * 1152;
            op.packetno    = mux_data->i_packet_no++;
            break;
        case HB_ACODEC_VORBIS:
            memcpy( &op, buf->data, sizeof( ogg_packet ) );
            op.packet = malloc( op.bytes );
            memcpy( op.packet, buf->data + sizeof( ogg_packet ), op.bytes );
            break;

        default:
            hb_log( "muxogm: unhandled codec" );
            op.bytes = 0;
            op.packet = NULL;
            break;
    }

    if( op.packet )
    {
        ogg_stream_packetin( &mux_data->os, &op );

        for( ;; )
        {
            ogg_page og;
            if( ogg_stream_pageout( &mux_data->os, &og ) == 0 )
            {
                break;
            }

            if( fwrite( og.header, og.header_len, 1, m->file ) <= 0 ||
                fwrite( og.body, og.body_len, 1, m->file ) <= 0 )
            {
                hb_log( "muxogm: write failed" );
                break;
            }
        }
        free( op.packet );
    }
    return 0;
}

static int OGMEnd( hb_mux_object_t * m )
{
    hb_job_t * job = m->job;

    hb_title_t * title = job->title;
    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;
    int          i;

    mux_data = job->mux_data;
    if( OGMFlush( m, mux_data ) < 0 )
    {
        return -1;
    }
    ogg_stream_clear( &mux_data->os );

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        mux_data = audio->priv.mux_data;
        if( OGMFlush( m, mux_data ) < 0 )
        {
            return -1;
        }
        ogg_stream_clear( &mux_data->os );
    }

    fclose( m->file );
    hb_log( "muxogm: `%s' closed", job->file );

    return 0;
}

hb_mux_object_t * hb_mux_ogm_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = OGMInit;
    m->mux       = OGMMux;
    m->end       = OGMEnd;
    m->job       = job;
    return m;
}

