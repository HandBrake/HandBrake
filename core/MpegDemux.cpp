/* $Id: MpegDemux.cpp,v 1.20 2003/10/16 13:36:17 titer Exp $

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
{
    fManager = manager;
    fTitle   = title;
    fAudio1  = audio1;
    fAudio2  = audio2;

    fLock = new HBLock();
    fUsed = false;

    fPSBuffer      = NULL;
    fESBuffer      = NULL;
    fESBufferList  = NULL;

    fFirstVideoPTS  = -1;
    fFirstAudio1PTS = -1;
    fFirstAudio2PTS = -1;
}

HBMpegDemux::~HBMpegDemux()
{
    /* Free memory */
    if( fESBufferList )
    {
        while( ( fESBuffer = (HBBuffer*) fESBufferList->ItemAt( 0 ) ) )
        {
            fESBufferList->RemoveItem( fESBuffer );
            delete fESBuffer;
        }
    }
    delete fLock;
}

bool HBMpegDemux::Work()
{
    if( !Lock() )
    {
        return false;
    }
    
    /* Push waiting buffers */
    if( fESBufferList )
    {
        for( uint32_t i = 0; i < fESBufferList->CountItems(); )
        {
            fESBuffer = (HBBuffer*) fESBufferList->ItemAt( i );
            
            if( fESBuffer->fPass == 1 && fESBuffer->fStreamId != 0xE0 )
            {
                fESBufferList->RemoveItem( fESBuffer );
                delete fESBuffer;
                continue;
            }

            /* Look for a decoder for this ES */

            if( fESBuffer->fStreamId == 0xE0 )
            {
                if( fFirstVideoPTS < 0 )
                {
                    fFirstVideoPTS = fESBuffer->fPTS;
                    Log( "HBMpegDemux: got first 0xE0 packet (%lld)",
                         fFirstVideoPTS );
                }
                if( fTitle->fMpeg2Fifo->Push( fESBuffer ) )
                {
                    fESBufferList->RemoveItem( fESBuffer );
                }
                else
                {
                    i++;
                }
            }
            else if( fAudio1 &&
                     fESBuffer->fStreamId == fAudio1->fId )
            {
                if( fFirstAudio1PTS < 0 )
                {
                    fFirstAudio1PTS = fESBuffer->fPTS;
                    Log( "HBMpegDemux: got first 0x%x packet (%lld)",
                         fAudio1->fId, fFirstAudio1PTS );

                    fAudio1->fDelay =
                        ( fFirstAudio1PTS - fFirstVideoPTS ) / 90;
                }
                if( fAudio1->fAc3Fifo->Push( fESBuffer ) )
                {
                    fESBufferList->RemoveItem( fESBuffer );
                }
                else
                {
                    i++;
                }
            }
            else if( fAudio2 &&
                     fESBuffer->fStreamId == fAudio2->fId )
            {
                if( fFirstAudio2PTS < 0 )
                {
                    fFirstAudio2PTS = fESBuffer->fPTS;
                    Log( "HBMpegDemux: got first 0x%x packet (%lld)",
                         fAudio2->fId, fFirstAudio2PTS );

                    fAudio2->fDelay =
                        ( fFirstAudio2PTS - fFirstVideoPTS ) / 90;
                }
                if( fAudio2->fAc3Fifo->Push( fESBuffer ) )
                {
                    fESBufferList->RemoveItem( fESBuffer );
                }
                else
                {
                    i++;
                }
            }
            else
            {
                fESBufferList->RemoveItem( fESBuffer );
                delete fESBuffer;
            }
        }

        if( !fESBufferList->CountItems() )
        {
            delete fESBufferList;
            fESBufferList = NULL;
        }
        else
        {
            Unlock();
            return false;
        }
    }

    /* Get a PS packet */
    if( ( fPSBuffer = fTitle->fPSFifo->Pop() ) )
    {
        /* Get the ES data in it */
        PStoES( fPSBuffer, &fESBufferList );
    }
    else
    {
        Unlock();
        return false;
    }

    Unlock();
    return true;
}

bool HBMpegDemux::Lock()
{
    fLock->Lock();
    if( fUsed )
    {
        fLock->Unlock();
        return false;
    }   
    fUsed = true;
    fLock->Unlock();
    return true;
}

void HBMpegDemux::Unlock()
{
    fLock->Lock();
    fUsed = false;
    fLock->Unlock();
}

bool PStoES( HBBuffer * psBuffer, HBList ** _esBufferList )
{
#define psData (psBuffer->fData)

    uint32_t pos = 0;

    /* pack_header */
    if( psData[pos] != 0 || psData[pos+1] != 0 ||
        psData[pos+2] != 0x1 || psData[pos+3] != 0xBA )
    {
        Log( "PStoES: not a PS packet (%02x%02x%02x%02x)",
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

    HBList   * esBufferList = new HBList();
    HBBuffer * esBuffer;

    /* PES */
    while( pos + 6 < psBuffer->fSize &&
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
        streamId           = psData[pos];
        pos               += 1;

        PES_packet_length  = ( psData[pos] << 8 ) + psData[pos+1];
        pos               += 2;               /* PES_packet_length */
        PES_packet_end     = pos + PES_packet_length;

        if( streamId != 0xE0 && streamId != 0xBD )
        {
            /* Not interesting */
            pos = PES_packet_end;
            continue;
        }

        hasPTS             = ( ( psData[pos+1] >> 6 ) & 0x2 );
        pos               += 2;               /* Required headers */

        PES_header_data_length  = psData[pos];
        pos                    += 1;
        PES_header_end          = pos + PES_header_data_length;

        if( hasPTS )
        {
            PTS = ( ( ( (uint64_t) psData[pos] >> 1 ) & 0x7 ) << 30 ) +
                  ( psData[pos+1] << 22 ) +
                  ( ( psData[pos+2] >> 1 ) << 15 ) +
                  ( psData[pos+3] << 7 ) +
                  ( psData[pos+4] >> 1 );
        }

        pos = PES_header_end;

        if( streamId == 0xBD )
        {
            /* A52: don't ask */
            streamId |= ( psData[pos] << 8 );
            pos += 4;
        }

        /* Sanity check */
        if( pos >= PES_packet_end )
        {
            Log( "PStoES: pos >= PES_packet_end" );
            pos = PES_packet_end;
            continue;
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

    if( !esBufferList->CountItems() )
    {
        delete esBufferList;
        esBufferList = NULL;
    }

    (*_esBufferList) = esBufferList;
    return true;

#undef psData
}
