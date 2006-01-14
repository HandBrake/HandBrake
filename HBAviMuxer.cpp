/* $Id: HBAviMuxer.cpp,v 1.24 2003/08/26 18:44:06 titer Exp $ */

#include "HBCommon.h"
#include "HBAviMuxer.h"
#include "HBManager.h"
#include "HBFifo.h"

#define AVIF_HASINDEX      0x10
#define AVIIF_KEYFRAME     0x10

HBAviMuxer::HBAviMuxer( HBManager * manager, HBTitleInfo * titleInfo,
                        HBAudioInfo * audio1Info, HBAudioInfo * audio2Info,
                        char * fileName )
    : HBThread( "avimuxer" )
{
    fManager    = manager;
    fTitleInfo  = titleInfo;
    fAudio1Info = audio1Info;
    fAudio2Info = audio2Info;
    fFileName   = strdup( fileName );
    
    fRiffBytesCount = 2040;
    fMoviBytesCount = 4;
}

void HBAviMuxer::DoWork()
{
    /* Open the destination file */
    if( !( fFile = fopen( fFileName, "w" ) ) )
    {
        Log( "HBAviMuxer: fopen failed" );
        fManager->Error();
        return;
    }
    
    /* Initializations */
    memset( &fMainHeader, 0, sizeof( AviMainHeader ) );
    memset( &fVideoStreamHeader, 0, sizeof( AviStreamHeader ) );
    memset( &fAudio1StreamHeader, 0, sizeof( AviStreamHeader ) );
    memset( &fAudio2StreamHeader, 0, sizeof( AviStreamHeader ) );
    memset( &fVideoStreamFormat, 0, sizeof( BitmapInfo ) );
    memset( &fAudio1StreamFormat, 0, sizeof( WaveFormatEx ) );
    memset( &fAudio2StreamFormat, 0, sizeof( WaveFormatEx ) );
    
    /* Alloc an 1 MB index (to be realloced later if needed) */
    fIndex = new HBBuffer( 1024 * 1024 );
    sprintf( (char*) fIndex->fData, "idx1" );
    fIndex->fSize    = 8;
    
    /* Main loop */
    int video, audio1, audio2;
    while( !fDie )
    {
        /* Find the most filled fifo */
        video  = fTitleInfo->fMpeg4Fifo->Size();
        audio1 = ( !fAudio1Info->fId ) ? 0 : fAudio1Info->fMp3Fifo->Size();
        audio2 = ( !fAudio2Info->fId ) ? 0 : fAudio2Info->fMp3Fifo->Size();
        
        /* Nothing to get - wait a bit and try again */
        if( !video && !audio1 && !audio2 )
        {
            snooze( 10000 );
            continue;
        }
        
        /* Got something - mux it */
        if( video >= MAX( audio1, audio2 ) )
        {
            AddVideoChunk();
        }
        else if( audio1 >= audio2 )
        {
            AddAudioChunk( 1 );
        }
        else
        {
            AddAudioChunk( 2 );
        }
    }
    
    /* Write the index */
    uint32_t size = fIndex->fSize - 8;
    memcpy( fIndex->fData + 4, &size, 4 );
    fseek( fFile, 0, SEEK_END );
    fwrite( fIndex->fData, fIndex->fSize, 1, fFile );
    
    /* Update the headers */
    fRiffBytesCount   += fIndex->fSize;
    fMainHeader.Flags |= AVIF_HASINDEX;
    UpdateMainHeader();
    
    delete fIndex;

    fclose( fFile );
}

bool HBAviMuxer::AddVideoChunk()
{
    HBBuffer * buffer = fTitleInfo->fMpeg4Fifo->Pop();
    if( !buffer )
    {
        return false;
    }

    fRiffBytesCount += 8 + EVEN( buffer->fSize );
    fMoviBytesCount += 8 + EVEN( buffer->fSize );
    
    fMainHeader.MicroSecPerFrame   = 1000000 * (uint64_t) fTitleInfo->fScale /
                                         fTitleInfo->fRate;
    fMainHeader.TotalFrames++;
    fMainHeader.Width              = fTitleInfo->fOutWidth;
    fMainHeader.Height             = fTitleInfo->fOutHeight;

    fVideoStreamHeader.FourCC      = FOURCC( "strh" );
    fVideoStreamHeader.BytesCount  = sizeof( AviStreamHeader ) - 8;
    fVideoStreamHeader.Type        = FOURCC( "vids" );
    fVideoStreamHeader.Handler     = FOURCC( "DIVX" );
    fVideoStreamHeader.Scale       = fTitleInfo->fScale;
    fVideoStreamHeader.Rate        = fTitleInfo->fRate;
    fVideoStreamHeader.Length++;
        
    fVideoStreamFormat.FourCC      = FOURCC( "strf" );
    fVideoStreamFormat.BytesCount  = sizeof( BitmapInfo ) - 8;
    fVideoStreamFormat.Size        = sizeof( BitmapInfo ) - 8;
    fVideoStreamFormat.Width       = fTitleInfo->fOutWidth;
    fVideoStreamFormat.Height      = fTitleInfo->fOutHeight;
    fVideoStreamFormat.Planes      = 1;
    fVideoStreamFormat.BitCount    = 24;
    fVideoStreamFormat.Compression = FOURCC( "DIVX" );;
 
    UpdateMainHeader();

    fseek( fFile, 0, SEEK_END );

    /* Update the index */
    if( fIndex->fSize + 16 > fIndex->fAllocSize )
    {
        /* Realloc if needed */
        Log( "HBAviMuxer::AddVideoChunk() : reallocing index (%d -> %d MB)",
             fIndex->fAllocSize / ( 1024 * 1024 ),
             1 + fIndex->fAllocSize / ( 1024 * 1024 ) );
        fIndex->ReAlloc( fIndex->fAllocSize + 1024 * 1024 );
    }
    
    uint32_t flags  = buffer->fKeyFrame ? AVIIF_KEYFRAME : 0;
    uint32_t offset = ftell( fFile ) - 2044;
    sprintf( (char*)fIndex->fData + fIndex->fSize, "00dc" );
    
    memcpy( fIndex->fData + fIndex->fSize + 4, &flags, 4 );
    memcpy( fIndex->fData + fIndex->fSize + 8, &offset, 4 );
    memcpy( fIndex->fData + fIndex->fSize + 12, &buffer->fSize, 4 );
    fIndex->fSize += 16;

    /* Write the chunk */
    fwrite( "00dc", 4, 1, fFile );
    fwrite( &buffer->fSize, 4, 1, fFile );
    fwrite( buffer->fData, buffer->fSize, 1, fFile );
    
    /* Chunks must be 2-bytes aligned */
    if( buffer->fSize & 1 )
    {
        fputc( 0, fFile );
    }
    
    delete buffer;

    return true;
}

bool HBAviMuxer::AddAudioChunk( int which )
{
    HBAudioInfo     * info;
    AviStreamHeader * streamHeader;
    WaveFormatEx    * streamFormat;
    
    if( which == 1 )
    {
        info         = fAudio1Info;
        streamHeader = &fAudio1StreamHeader;
        streamFormat = &fAudio1StreamFormat;
    }
    else
    {
        info         = fAudio2Info;
        streamHeader = &fAudio2StreamHeader;
        streamFormat = &fAudio2StreamFormat;
    }

    HBBuffer * buffer = info->fMp3Fifo->Pop();
    if( !buffer )
    {
        return false;
    }

    fRiffBytesCount += 8 + EVEN( buffer->fSize );
    fMoviBytesCount += 8 + EVEN( buffer->fSize );

    streamHeader->FourCC         = FOURCC( "strh" );
    streamHeader->BytesCount     = sizeof( AviStreamHeader ) - 8;
    streamHeader->Type           = FOURCC( "auds" );
    streamHeader->InitialFrames  = 1;
    streamHeader->Scale          = 1152;
    streamHeader->Rate           = info->fOutSampleRate;
    streamHeader->Length++;
    streamHeader->Quality        = 0xFFFFFFFF;


    streamFormat->FourCC         = FOURCC( "strf" );
    streamFormat->BytesCount     = sizeof( WaveFormatEx ) - 8;
    streamFormat->FormatTag      = 0x55;
    streamFormat->Channels       = 2;
    streamFormat->SamplesPerSec  = info->fOutSampleRate;
    streamFormat->AvgBytesPerSec = info->fOutBitrate * 1024 / 8;
    streamFormat->BlockAlign     = 1152;
    
    /* stolen from libavformat/wav.c */
    streamFormat->Size           = 12;
    streamFormat->Id             = 1;
    streamFormat->Flags          = 2;
    streamFormat->BlockSize      = 1152;
    streamFormat->FramesPerBlock = 1;
    streamFormat->CodecDelay     = 1393;

    UpdateMainHeader();

    fseek( fFile, 0, SEEK_END );

    /* Update the index */
    if( fIndex->fSize + 16 > fIndex->fAllocSize )
    {
        /* Realloc if needed */
        Log( "HBAviMuxer::AddAudioChunk() : reallocing index (%d -> %d MB)",
             fIndex->fAllocSize / ( 1024 * 1024 ),
             1 + fIndex->fAllocSize / ( 1024 * 1024 ) );
        fIndex->ReAlloc( fIndex->fAllocSize + 1024 * 1024 );
    }
    
    uint32_t flags  = buffer->fKeyFrame ? AVIIF_KEYFRAME : 0;
    uint32_t offset = ftell( fFile ) - 2044;
    sprintf( (char*)fIndex->fData + fIndex->fSize, "%02dwb", which );
    
    memcpy( fIndex->fData + fIndex->fSize + 4, &flags, 4 );
    memcpy( fIndex->fData + fIndex->fSize + 8, &offset, 4 );
    memcpy( fIndex->fData + fIndex->fSize + 12, &buffer->fSize, 4 );
    fIndex->fSize += 16;

    /* Write the chunk */
    fprintf( fFile, "%02dwb", which );
    fwrite( &buffer->fSize, 4, 1, fFile );
    fwrite( buffer->fData, buffer->fSize, 1, fFile );
    
    /* Chunks must be 2-bytes aligned */
    if( buffer->fSize & 1 )
    {
        fputc( 0, fFile );
    }

    delete buffer;
    
    return true;
}

void HBAviMuxer::UpdateMainHeader()
{
    fMainHeader.FourCC              = FOURCC( "avih" );
    fMainHeader.BytesCount          = sizeof( AviMainHeader ) - 8;
    fMainHeader.Streams             = 2;

    fHdrlBytesCount = 4 + sizeof( AviMainHeader )
                        + 12 + sizeof( AviStreamHeader ) + sizeof( BitmapInfo );
    
    if( fAudio1Info->fId )
    {
        fHdrlBytesCount += 12 + sizeof( AviStreamHeader ) + sizeof( WaveFormatEx );
    }
    if( fAudio2Info->fId )
    {
        fHdrlBytesCount += 12 + sizeof( AviStreamHeader ) + sizeof( WaveFormatEx );
    }
    
    fseek( fFile, 0, SEEK_SET );
    fwrite( "RIFF", 4, 1, fFile );
    fwrite( &fRiffBytesCount, 4, 1, fFile );
    fwrite( "AVI ", 4, 1, fFile );
    fwrite( "LIST", 4, 1, fFile );
    fwrite( &fHdrlBytesCount, 4, 1, fFile );
    fwrite( "hdrl", 4, 1, fFile );
    
    fwrite( &fMainHeader, sizeof( AviMainHeader ), 1, fFile );
    
    int strlSize;
    strlSize = 4 + sizeof( AviStreamHeader ) + sizeof( BitmapInfo );
    fwrite( "LIST", 4, 1, fFile );
    fwrite( &strlSize, 4, 1, fFile );
    fwrite( "strl", 4, 1, fFile );
    fwrite( &fVideoStreamHeader, sizeof( AviStreamHeader ), 1, fFile );
    fwrite( &fVideoStreamFormat, sizeof( fVideoStreamFormat ), 1, fFile );
    
    if( fAudio1Info->fId )
    {
        strlSize = 4 + sizeof( AviStreamHeader ) + sizeof( WaveFormatEx );
        fwrite( "LIST", 4, 1, fFile );
        fwrite( &strlSize, 4, 1, fFile );
        fwrite( "strl", 4, 1, fFile );
        fwrite( &fAudio1StreamHeader, sizeof( AviStreamHeader ), 1, fFile );
        fwrite( &fAudio1StreamFormat, sizeof( WaveFormatEx ), 1, fFile );
    }
    
    if( fAudio2Info->fId )
    {
        strlSize = 4 + sizeof( AviStreamHeader ) + sizeof( WaveFormatEx );
        fwrite( "LIST", 4, 1, fFile );
        fwrite( &strlSize, 4, 1, fFile );
        fwrite( "strl", 4, 1, fFile );
        fwrite( &fAudio2StreamHeader, sizeof( AviStreamHeader ), 1, fFile );
        fwrite( &fAudio2StreamFormat, sizeof( WaveFormatEx ), 1, fFile );
    }
    
    /* a JUNK chunk to fill the free space.
       size = 2048 -/
              12 ("RIFFxxxxAVI ") - 
              8  (hdrl's "LIS1Txxxx") -
              fHdrlBytesCount -
              8  ("JUNKxxxx") -
              12 ("LISTxxxxmovi) */
    int junkSize = 2008 - fHdrlBytesCount;
    fwrite( "JUNK", 4, 1, fFile );
    fwrite( &junkSize, 4, 1, fFile );
    for( uint32_t i = 0; i < 2008 - fHdrlBytesCount; i++ )
    {
        fputc( 0, fFile );
    }
    
    /* movi list */
    fwrite( "LIST", 4, 1, fFile );
    fwrite( &fMoviBytesCount, 4, 1, fFile );
    fwrite( "movi", 4, 1, fFile );    
}
