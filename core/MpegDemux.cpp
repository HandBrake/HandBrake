/* $Id: MpegDemux.cpp,v 1.10 2003/10/04 12:12:48 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "MpegDemux.h"
#include "Manager.h"
#include "Fifo.h"

extern "C" {
#include <a52dec/a52.h>
}

HBMpegDemux::HBMpegDemux( HBManager * manager, HBTitle * title,
                          HBAudio * audio1, HBAudio * audio2 )
    : HBThread( "mpegdemux" )
{
    fManager        = manager;
    fTitle          = title;
    fAudio1         = audio1;
    fAudio2         = audio2;

    fPSBuffer       = NULL;
    fESBuffer       = NULL;
    fESBufferList   = NULL;

    fFirstVideoPTS  = -1;
    fFirstAudio1PTS = -1;
    fFirstAudio2PTS = -1;
}

void HBMpegDemux::DoWork()
{
    for( ;; )
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        /* Get a PS packet */
        if( !( fPSBuffer = Pop( fTitle->fPSFifo ) ) )
        {
            break;
        }

        /* Get the ES data in it */
        PStoES( fPSBuffer, &fESBufferList );

        if( !fESBufferList )
        {
            continue;
        }

        while( ( fESBuffer = (HBBuffer*) fESBufferList->ItemAt( 0 ) ) )
        {
            fESBufferList->RemoveItem( fESBuffer );

            if( fESBuffer->fPass == 1 && fESBuffer->fStreamId != 0xE0 )
            {
                delete fESBuffer;
                continue;
            }

            /* Look for a decoder for this ES */
            if( fESBuffer->fStreamId == 0xE0 )
            {
                if( fFirstVideoPTS < 0 )
                {
                    fFirstVideoPTS = fESBuffer->fPTS;
                }
                Push( fTitle->fMpeg2Fifo, fESBuffer );
            }
            else if( fAudio1 &&
                     fESBuffer->fStreamId == fAudio1->fId )
            {
                /* If the audio track starts later than the video,
                   repeat the first frame as long as needed  */
                if( fFirstAudio1PTS < 0 )
                {
                    fFirstAudio1PTS = fESBuffer->fPTS;

                    if( fFirstAudio1PTS > fFirstVideoPTS )
                    {
                        Log( "HBMpegDemux::DoWork() : audio track %x "
                             "is late (%lld)", fAudio1->fId,
                             fFirstAudio1PTS - fFirstVideoPTS );
                        InsertSilence( fFirstAudio1PTS - fFirstVideoPTS,
                                       fAudio1->fAc3Fifo,
                                       fESBuffer );
                    }
                }
                Push( fAudio1->fAc3Fifo, fESBuffer );
            }
            else if( fAudio2 &&
                     fESBuffer->fStreamId == fAudio2->fId )
            {
                if( fFirstAudio2PTS < 0 )
                {
                    fFirstAudio2PTS = fESBuffer->fPTS;

                    if( fFirstAudio2PTS > fFirstVideoPTS )
                    {
                        Log( "HBMpegDemux::DoWork() : audio track %x "
                             "is late (%lld)", fAudio2->fId,
                             fFirstAudio2PTS - fFirstVideoPTS );
                        InsertSilence( fFirstAudio2PTS - fFirstVideoPTS,
                                       fAudio2->fAc3Fifo,
                                       fESBuffer );
                    }
                }
                Push( fAudio2->fAc3Fifo, fESBuffer );
            }
            else
            {
                delete fESBuffer;
            }
        }
        delete fESBufferList;
    }
}

void HBMpegDemux::InsertSilence( int64_t time, HBFifo * fifo,
                                 HBBuffer * buffer )
{
    int        flags      = 0;
    int        sampleRate = 0;
    int        bitrate    = 0;
    int        frameSize  = a52_syncinfo( buffer->fData, &flags,
                                          &sampleRate, &bitrate );

    if( !frameSize )
    {
        Log( "HBMpegDemux::InsertSilence() : a52_syncinfo() failed" );
        return;
    }

    uint32_t frames = ( ( sampleRate * time / 90000 ) + ( 3 * 256 ) )
                          / ( 6 * 256 );

    if( !frames )
    {
        return;
    }

    Log( "HBMpegDemux::InsertSilence() : adding %d frames", frames );

    HBBuffer * buffer2;
    for( uint32_t i = 0; i < frames; i++ )
    {
        buffer2 = new HBBuffer( frameSize );
        buffer2->fPosition = buffer->fPosition;
        memcpy( buffer2->fData, buffer->fData, frameSize );
        Push( fifo, buffer2 );
    }
}

bool PStoES( HBBuffer * psBuffer, HBList ** _esBufferList )
{
#define psData (psBuffer->fData)

    HBList   * esBufferList = new HBList();
    HBBuffer * esBuffer;
    uint32_t   pos = 0;

    /* pack_header */
    if( psData[pos] != 0 || psData[pos+1] != 0 ||
        psData[pos+2] != 0x1 || psData[pos+3] != 0xBA )
    {
        Log( "PStoES : not a PS packet (%02x%02x%02x%02x)",
             psData[pos] << 24, psData[pos+1] << 16,
             psData[pos+2] << 8, psData[pos+3] );
        delete psBuffer;
        (*_esBufferList) = NULL;
        return false;
    }
    pos += 4;                         /* pack_start_code */
    pos += 9;                         /* pack_header */
    pos += 1 + ( psData[pos] & 0x7 ); /* stuffing bytes */

    /* system_header */
    if( psData[pos] == 0 && psData[pos+1] == 0 &&
        psData[pos+2] == 0x1 && psData[pos+3] == 0xBB )
    {
        uint32_t header_length;

        pos           += 4; /* system_header_start_code */
        header_length  = ( psData[pos] << 8 ) + psData[pos+1];
        pos           += 2 + header_length;
    }

    /* PES */
    while( pos + 2 < psBuffer->fSize &&
           psData[pos] == 0 && psData[pos+1] == 0 && psData[pos+2] == 0x1 )
    {
        uint32_t streamId;
        uint32_t PES_packet_length;
        uint32_t PES_packet_end;
        uint32_t PES_header_data_length;
        uint32_t PES_header_end;
        bool     hasPTS;
        uint64_t PTS = 0;

        pos               += 3;               /* packet_start_code_prefix */
        streamId           = psData[pos++];

        PES_packet_length  = ( psData[pos] << 8 ) + psData[pos+1];
        pos               += 2;               /* PES_packet_length */
        PES_packet_end     = pos + PES_packet_length;

        hasPTS             = ( ( psData[pos+1] >> 6 ) & 0x2 );
        pos               += 2;               /* Required headers */

        PES_header_data_length  = psData[pos];
        pos                    += 1;
        PES_header_end          = pos + PES_header_data_length;

        if( hasPTS )
        {
            PTS = ( ( ( psData[pos] >> 1 ) & 0x7 ) << 30 ) +
                  ( psData[pos+1] << 22 ) +
                  ( ( psData[pos+2] >> 1 ) << 15 ) +
                  ( psData[pos+3] << 7 ) +
                  ( psData[pos+4] >> 1 );
        }

        pos = PES_header_end;

        if( streamId != 0xE0 && streamId != 0xBD )
        {
            /* Not interesting */
            continue;
        }

        if( streamId == 0xBD )
        {
            /* A52 : don't ask */
            streamId |= ( psData[pos] << 8 );
            pos += 4;
        }

        /* Here we hit we ES payload */
        esBuffer = new HBBuffer( PES_packet_end - pos );

        esBuffer->fPosition = psBuffer->fPosition;
        esBuffer->fPass     = psBuffer->fPass;
        esBuffer->fStreamId = streamId;
        esBuffer->fPTS      = PTS;
        memcpy( esBuffer->fData, psBuffer->fData + pos,
                PES_packet_end - pos );

        esBufferList->AddItem( esBuffer );

        pos = PES_packet_end;
    }

    delete psBuffer;

    if( esBufferList && !esBufferList->CountItems() )
    {
        delete esBufferList;
        esBufferList = NULL;
    }

    (*_esBufferList) = esBufferList;
    return true;

#undef psData
}
