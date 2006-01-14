/* $Id: VorbisEnc.c,v 1.9 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libvorbis */
#include <vorbis/vorbisenc.h>

#define OGGVORBIS_FRAME_SIZE 1024

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;

    int                inited;
    vorbis_info        vi;
    vorbis_comment     vc;
    vorbis_dsp_state   vd;
    vorbis_block       vb;
    float position;

    int32_t           * inputBuffer;

};

/* Local prototypes */
static int VorbisEncWork( HBWork * );

HBWork * HBVorbisEncInit ( HBHandle * handle, HBAudio * audio )
{
    HBWork * w = calloc( sizeof( HBWork ), 1 );

    w->name = strdup( "VorbisEnc" );
    w->work = VorbisEncWork;

    w->handle = handle;
    w->audio  = audio;

    w->inputBuffer = malloc( 2 * OGGVORBIS_FRAME_SIZE * sizeof( int32_t ) );

    return w;
}

void HBVorbisEncClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->inited )
    {
        vorbis_block_clear( &w->vb );
        vorbis_dsp_clear( &w->vd );
        vorbis_comment_clear( &w->vc );
        vorbis_info_clear( &w->vi );
    }

    free( w->name );
    free( w );

    *_w = NULL;
}

static HBBuffer *PacketToBuffer( ogg_packet *op )
{
    HBBuffer *buf = HBBufferInit( sizeof( ogg_packet ) + op->bytes );

    memcpy( buf->data, op, sizeof( ogg_packet ) );
    memcpy( buf->data + sizeof( ogg_packet ), op->packet, op->bytes );

    return buf;
}

static int VorbisEncWork( HBWork * w )
{
    HBAudio     * audio = w->audio;

    float **buffer;
    int i;
    float inputBuffer[OGGVORBIS_FRAME_SIZE * 2];
    HBBuffer * vorbisBuffer;

    if( HBFifoIsHalfFull( audio->outFifo ) )
    {
        return 0;
    }

    if( !w->inited )
    {
        ogg_packet header[3];

        if( !HBFifoSize( audio->resampleFifo ) )
        {
            return 0;
        }

        w->inited = 1;

        /* init */
        vorbis_info_init( &w->vi );
        if( vorbis_encode_setup_managed( &w->vi, 2,
              audio->outSampleRate, -1, 1000 * audio->outBitrate, -1 ) ||
            vorbis_encode_ctl( &w->vi, OV_ECTL_RATEMANAGE_AVG, NULL ) ||
              vorbis_encode_setup_init( &w->vi ) )
        {
            HBLog( "HBVorbisEnc: vorbis_encode_setup_managed failed" );
            return 0;
        }
        /* add a comment */
        vorbis_comment_init( &w->vc );
        vorbis_comment_add_tag( &w->vc, "ENCODER", "HandBrake");

        /* set up the analysis state and auxiliary encoding storage */
        vorbis_analysis_init( &w->vd, &w->vi);
        vorbis_block_init( &w->vd, &w->vb);


        /* get the 3 headers */
        vorbis_analysis_headerout( &w->vd, &w->vc,
                                   &header[0], &header[1], &header[2] );
        for( i = 0; i < 3; i++ )
        {
            vorbisBuffer = PacketToBuffer( &header[i] );
            if( !HBFifoPush( audio->outFifo, &vorbisBuffer ) )
            {
                HBLog( "HBVorbisEnc: HBFifoPush failed" );
            }
        }
    }

    /* Try to extract more data */
    if( vorbis_analysis_blockout( &w->vd, &w->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &w->vb, NULL );
        vorbis_bitrate_addblock( &w->vb );

        if( vorbis_bitrate_flushpacket( &w->vd, &op ) )
        {
            vorbisBuffer = PacketToBuffer( &op );
            vorbisBuffer->position = w->position;
            if( !HBFifoPush( audio->outFifo, &vorbisBuffer ) )
            {
                HBLog( "HBVorbisEnc: HBFifoPush failed" );
            }
            return 1;
        }
    }

    if( !HBFifoGetBytes( audio->resampleFifo, (uint8_t*) inputBuffer,
                         OGGVORBIS_FRAME_SIZE * 2 * sizeof( float ),
                         &w->position ) )
    {
        return 0;
    }

    buffer = vorbis_analysis_buffer( &w->vd, OGGVORBIS_FRAME_SIZE );
    for( i = 0; i < OGGVORBIS_FRAME_SIZE; i++ )
    {
        buffer[0][i] = inputBuffer[2*i]   / 32768.f;
        buffer[1][i] = inputBuffer[2*i+1] / 32768.f;
    }
    vorbis_analysis_wrote( &w->vd, OGGVORBIS_FRAME_SIZE );

    return 1;
}

