/* $Id: VorbisEnc.c,v 1.5 2004/03/08 11:32:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libvorbis */
#include <vorbis/vorbisenc.h>

#define OGGVORBIS_FRAME_SIZE 1024

typedef struct HBVorbisEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;

    int                inited;
    vorbis_info        vi;
    vorbis_comment     vc;
    vorbis_dsp_state   vd;
    vorbis_block       vb;

    HBBuffer           *rawBuffer;
    int                 rawBufferPos; /* in bytes */
    float               position;
    int32_t           * inputBuffer;
    unsigned long       samplesGot;
    unsigned long       inputSamples;

    HBBuffer           *vorbisBuffer;

    HBBuffer           *header[3];
} HBVorbisEnc;

/* Local prototypes */
static int VorbisEncWork( HBWork * );
static int GetSamples( HBVorbisEnc * );

HBWork *HBVorbisEncInit ( HBHandle *handle, HBAudio *audio )
{
    HBVorbisEnc *enc = malloc( sizeof( HBVorbisEnc ) );

    enc->name = strdup( "VorbisEnc" );
    enc->work = VorbisEncWork;

    enc->handle     = handle;
    enc->audio      = audio;

    enc->inited      = 0;
    enc->rawBuffer   = NULL;
    enc->inputSamples = 2 * OGGVORBIS_FRAME_SIZE;
    enc->inputBuffer = malloc( 2 * OGGVORBIS_FRAME_SIZE * sizeof( int32_t ) );
    enc->samplesGot  = 0;

    enc->vorbisBuffer = NULL;

    return (HBWork*) enc;
}

void HBVorbisEncClose( HBWork **_enc )
{
    HBVorbisEnc *enc = (HBVorbisEnc*) *_enc;

    if( enc->inited )
    {
        vorbis_block_clear( &enc->vb );
        vorbis_dsp_clear( &enc->vd );
        vorbis_comment_clear( &enc->vc );
        vorbis_info_clear( &enc->vi );
    }

    free( enc->name );
    free( enc );

    *_enc = NULL;
}

static HBBuffer *PacketToBuffer( ogg_packet *op )
{
    HBBuffer *buf = HBBufferInit( sizeof( ogg_packet ) + op->bytes );

    memcpy( buf->data, op, sizeof( ogg_packet ) );
    memcpy( buf->data + sizeof( ogg_packet ), op->packet, op->bytes );

    return buf;
}

static int VorbisEncWork( HBWork *w )
{
    HBVorbisEnc *enc = (HBVorbisEnc*)w;
    HBAudio     *audio = enc->audio;
    int         didSomething = 0;

    float **buffer;
    int i;


    if( !enc->inited )
    {
        ogg_packet header[3];

        /* Get a first buffer so we know that audio->inSampleRate is correct */
        if( ( enc->rawBuffer = HBFifoPop( audio->rawFifo ) ) == NULL )
        {
            return 0;
        }
        enc->inited = 1;

        didSomething = 1;
        enc->rawBufferPos = 0;
        enc->position     = enc->rawBuffer->position;

        /* No resampling */
        audio->outSampleRate = audio->inSampleRate;

        /* init */
        vorbis_info_init( &enc->vi );
        if( vorbis_encode_setup_managed( &enc->vi, 2,
              audio->inSampleRate, -1, 1000 * audio->outBitrate, -1 ) ||
            vorbis_encode_ctl( &enc->vi, OV_ECTL_RATEMANAGE_AVG, NULL ) ||
              vorbis_encode_setup_init( &enc->vi ) )
        {
            HBLog( "VorbisEnc: vorbis_encode_setup_managed failed" );
            return 0;
        }
        /* add a comment */
        vorbis_comment_init( &enc->vc );
        vorbis_comment_add_tag( &enc->vc, "ENCODER", "HandBrake");

        /* set up the analysis state and auxiliary encoding storage */
        vorbis_analysis_init( &enc->vd, &enc->vi);
        vorbis_block_init( &enc->vd, &enc->vb);


        /* get the 3 headers */
        vorbis_analysis_headerout( &enc->vd, &enc->vc,
                                   &header[0], &header[1], &header[2] );

        enc->header[0] = PacketToBuffer( &header[0] );
        enc->header[1] = PacketToBuffer( &header[1] );
        enc->header[2] = PacketToBuffer( &header[2] );
    }

    if( enc->header[0] )
    {
        HBLog( "VorbisEncWork: sending header 1" );
        if( !HBFifoPush( audio->outFifo, &enc->header[0] ) )
        {
            return didSomething;
        }
        didSomething = 1;
    }
    if( enc->header[1] )
    {
        HBLog( "VorbisEncWork: sending header 2" );
        if( !HBFifoPush( audio->outFifo, &enc->header[1] ) )
        {
            return didSomething;
        }
        didSomething = 1;
    }
    if( enc->header[2] )
    {
        HBLog( "VorbisEncWork: sending header 3" );
        if( !HBFifoPush( audio->outFifo, &enc->header[2] ) )
        {
            return didSomething;
        }
        didSomething = 1;
    }

    /* Push already encoded data */
    if( enc->vorbisBuffer )
    {
        if( !HBFifoPush( audio->outFifo, &enc->vorbisBuffer ) )
        {
            return didSomething;
        }
    }

    /* Try to extract more data */
    if( vorbis_analysis_blockout( &enc->vd, &enc->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &enc->vb, NULL );
        vorbis_bitrate_addblock( &enc->vb );

        if( vorbis_bitrate_flushpacket( &enc->vd, &op ) )
        {
            enc->vorbisBuffer = PacketToBuffer( &op );
            enc->vorbisBuffer->position = enc->position;
            return 1;
        }
    }

    /* FUCK -Werror ! */
    if( !GetSamples( enc ) )
    {
        return didSomething;
    }

    didSomething = 1;

    buffer = vorbis_analysis_buffer( &enc->vd, OGGVORBIS_FRAME_SIZE );
    for( i = 0; i < OGGVORBIS_FRAME_SIZE; i++ )
    {
        buffer[0][i] = (float)enc->inputBuffer[2 * i + 0]/32768.f;
        buffer[1][i] = (float)enc->inputBuffer[2 * i + 1]/32768.f;
    }
    vorbis_analysis_wrote( &enc->vd, OGGVORBIS_FRAME_SIZE );

    enc->samplesGot = 0;

    return 1;
}

static int GetSamples( HBVorbisEnc * f )
{
    while( f->samplesGot < f->inputSamples )
    {
        int i, copy;

        if( !f->rawBuffer )
        {
            if( !( f->rawBuffer = HBFifoPop( f->audio->rawFifo ) ) )
            {
                return 0;
            }

            f->rawBufferPos = 0;
            f->position     = f->rawBuffer->position;
        }

        copy = MIN( f->inputSamples - f->samplesGot,
                    ( f->rawBuffer->samples - f->rawBufferPos ) * 2 );

        for( i = 0; i < copy; i += 2 )
        {
            f->inputBuffer[f->samplesGot++] =
                f->rawBuffer->left[f->rawBufferPos];
            f->inputBuffer[f->samplesGot++] =
                f->rawBuffer->right[f->rawBufferPos];
            f->rawBufferPos++;
        }

        if( f->rawBufferPos == f->rawBuffer->samples )
        {
            HBBufferClose( &f->rawBuffer );
        }
    }

    return 1;
}
