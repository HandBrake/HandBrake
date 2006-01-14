/* $Id: AviMuxer.cpp,v 1.17 2003/10/09 23:33:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "AviMuxer.h"
#include "Manager.h"

#define AVIF_HASINDEX      0x10
#define AVIIF_KEYFRAME     0x10

#define FOURCC(a) ( ( a[3] << 24 ) | ( a[2] << 16 ) | ( a[1] << 8 ) | a[0] )

/* TODO : check return values from fputc/fwrite in case disk is full
   or something */

void WriteInt8( FILE * file, uint8_t val )
{
    fputc( val, file );
}

void WriteInt16( FILE * file, uint16_t val )
{
    fputc( val & 0xFF, file );
    fputc( val >> 8, file );
}

void WriteInt32( FILE * file, uint32_t val )
{
    fputc( val & 0xFF, file );
    fputc( ( val >> 8 ) & 0xFF, file );
    fputc( ( val >> 16 ) & 0xFF, file );
    fputc( val >> 24, file );
}

void WriteBuffer( FILE * file, HBBuffer * buffer )
{
    fwrite( buffer->fData, buffer->fSize, 1, file );
}

void WriteBitmapInfo( FILE * file, BitmapInfo * bitmapInfo )
{
    WriteInt32( file, bitmapInfo->FourCC );
    WriteInt32( file, bitmapInfo->BytesCount );
    WriteInt32( file, bitmapInfo->Size );
    WriteInt32( file, bitmapInfo->Width );
    WriteInt32( file, bitmapInfo->Height );
    WriteInt16( file, bitmapInfo->Planes );
    WriteInt16( file, bitmapInfo->BitCount );
    WriteInt32( file, bitmapInfo->Compression );
    WriteInt32( file, bitmapInfo->SizeImage );
    WriteInt32( file, bitmapInfo->XPelsPerMeter );
    WriteInt32( file, bitmapInfo->YPelsPerMeter );
    WriteInt32( file, bitmapInfo->ClrUsed );
    WriteInt32( file, bitmapInfo->ClrImportant );
    WriteInt8( file, bitmapInfo->Blue );
    WriteInt8( file, bitmapInfo->Green );
    WriteInt8( file, bitmapInfo->Red );
    WriteInt8( file, bitmapInfo->Reserved );
}

void WriteWaveFormatEx( FILE * file, WaveFormatEx * waveFormatEx )
{
    WriteInt32( file, waveFormatEx->FourCC );
    WriteInt32( file, waveFormatEx->BytesCount );
    WriteInt16( file, waveFormatEx->FormatTag );
    WriteInt16( file, waveFormatEx->Channels );
    WriteInt32( file, waveFormatEx->SamplesPerSec );
    WriteInt32( file, waveFormatEx->AvgBytesPerSec );
    WriteInt16( file, waveFormatEx->BlockAlign );
    WriteInt16( file, waveFormatEx->BitsPerSample );
    WriteInt16( file, waveFormatEx->Size );
    WriteInt16( file, waveFormatEx->Id );
    WriteInt32( file, waveFormatEx->Flags );
    WriteInt16( file, waveFormatEx->BlockSize );
    WriteInt16( file, waveFormatEx->FramesPerBlock );
    WriteInt16( file, waveFormatEx->CodecDelay );
}

void WriteMainHeader( FILE * file, AviMainHeader * mainHeader )
{
    WriteInt32( file, mainHeader->FourCC );
    WriteInt32( file, mainHeader->BytesCount );
    WriteInt32( file, mainHeader->MicroSecPerFrame );
    WriteInt32( file, mainHeader->MaxBytesPerSec );
    WriteInt32( file, mainHeader->PaddingGranularity );
    WriteInt32( file, mainHeader->Flags );
    WriteInt32( file, mainHeader->TotalFrames );
    WriteInt32( file, mainHeader->InitialFrames );
    WriteInt32( file, mainHeader->Streams );
    WriteInt32( file, mainHeader->SuggestedBufferSize );
    WriteInt32( file, mainHeader->Width );
    WriteInt32( file, mainHeader->Height );
    WriteInt32( file, mainHeader->Reserved[0] );
    WriteInt32( file, mainHeader->Reserved[1] );
    WriteInt32( file, mainHeader->Reserved[2] );
    WriteInt32( file, mainHeader->Reserved[3] );
}

void WriteStreamHeader( FILE * file, AviStreamHeader * streamHeader )
{
    WriteInt32( file, streamHeader->FourCC );
    WriteInt32( file, streamHeader->BytesCount );
    WriteInt32( file, streamHeader->Type );
    WriteInt32( file, streamHeader->Handler );
    WriteInt32( file, streamHeader->Flags );
    WriteInt16( file, streamHeader->Priority );
    WriteInt16( file, streamHeader->Language );
    WriteInt32( file, streamHeader->InitialFrames );
    WriteInt32( file, streamHeader->Scale );
    WriteInt32( file, streamHeader->Rate );
    WriteInt32( file, streamHeader->Start );
    WriteInt32( file, streamHeader->Length );
    WriteInt32( file, streamHeader->SuggestedBufferSize );
    WriteInt32( file, streamHeader->Quality );
    WriteInt32( file, streamHeader->SampleSize );
    WriteInt16( file, streamHeader->Left );
    WriteInt16( file, streamHeader->Top );
    WriteInt16( file, streamHeader->Right );
    WriteInt16( file, streamHeader->Bottom );
}

HBAviIndex::HBAviIndex( uint32_t size )
    : HBBuffer( size )
{
    fSize = 0;
}

void HBAviIndex::WriteInt32( uint32_t val )
{
    if( fSize + 16 > fAllocSize )
    {
        /* Realloc if needed */
        Log( "HBAviIndex::WriteInt32() : reallocing index (%d -> %d MB)",
             fAllocSize / ( 1024 * 1024 ),
             1 + fAllocSize / ( 1024 * 1024 ) );
        ReAlloc( fAllocSize + 1024 * 1024 );
    }

    fData[fSize] = val & 0xFF;
    fData[fSize + 1] = ( val >> 8 ) & 0xFF;
    fData[fSize + 2] = ( val >> 16 ) & 0xFF;
    fData[fSize + 3] = val >> 24;

    fSize += 4;
}

HBAviMuxer::HBAviMuxer( HBManager * manager, HBTitle * title,
                        HBAudio * audio1, HBAudio * audio2,
                        char * fileName )
    : HBThread( "avimuxer", HB_NORMAL_PRIORITY )
{
    fManager        = manager;
    fTitle          = title;
    fAudio1         = audio1;
    fAudio2         = audio2;
    fFileName       = strdup( fileName );

    fVideoBuffer    = NULL;
    fAudio1Buffer   = NULL;
    fAudio2Buffer   = NULL;

    fRiffBytesCount = 2040;
    fMoviBytesCount = 4;

    Run();
}

void HBAviMuxer::DoWork()
{
    /* Open the destination file */
    if( !( fFile = fopen( fFileName, "w" ) ) )
    {
        Log( "HBAviMuxer: fopen failed" );
        fManager->Error( HB_ERROR_AVI_WRITE );
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
    fIndex = new HBAviIndex( 1024 * 1024 );

    /* Main loop */
    for( ;; )
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        if( !fVideoBuffer )
        {
            fVideoBuffer = Pop( fTitle->fMpeg4Fifo );
        }
        if( fAudio1 && !fAudio1Buffer )
        {
            fAudio1Buffer = Pop( fAudio1->fMp3Fifo );
        }
        if( fAudio2 && !fAudio2Buffer )
        {
            fAudio2Buffer = Pop( fAudio2->fMp3Fifo );
        }

        if( !fVideoBuffer && !fAudio1Buffer && !fAudio2Buffer )
        {
            break;
        }

        if( fVideoBuffer &&
            ( !fAudio1Buffer ||
              fVideoBuffer->fPosition < fAudio1Buffer->fPosition ) &&
            ( !fAudio2Buffer ||
              fVideoBuffer->fPosition < fAudio2Buffer->fPosition ) )
        {
            AddVideoChunk();
        }
        else if( fAudio1Buffer &&
                 ( !fAudio2Buffer ||
                   fAudio1Buffer->fPosition < fAudio2Buffer->fPosition ) )
        {
            AddAudioChunk( 1 );
        }
        else
        {
            AddAudioChunk( 2 );
        }
    }

    /* Write the index */
    fseek( fFile, 0, SEEK_END );
    WriteInt32( fFile, FOURCC( "idx1" ) );
    WriteInt32( fFile, fIndex->fSize );
    WriteBuffer( fFile, fIndex );

    /* Update the headers */
    fRiffBytesCount   += 8 + fIndex->fSize;
    fMainHeader.Flags |= AVIF_HASINDEX;
    UpdateMainHeader();

    delete fIndex;

    fclose( fFile );
}

bool HBAviMuxer::AddVideoChunk()
{
    fRiffBytesCount += 8 + EVEN( fVideoBuffer->fSize );
    fMoviBytesCount += 8 + EVEN( fVideoBuffer->fSize );

    fMainHeader.MicroSecPerFrame   = 1000000 * (uint64_t) fTitle->fScale /
                                         fTitle->fRate;
    fMainHeader.TotalFrames++;
    fMainHeader.Width              = fTitle->fOutWidth;
    fMainHeader.Height             = fTitle->fOutHeight;

    fVideoStreamHeader.FourCC      = FOURCC( "strh" );
    fVideoStreamHeader.BytesCount  = AVI_STREAM_HEADER_SIZE - 8;
    fVideoStreamHeader.Type        = FOURCC( "vids" );
    fVideoStreamHeader.Handler     = FOURCC( "DIVX" );
    fVideoStreamHeader.Scale       = fTitle->fScale;
    fVideoStreamHeader.Rate        = fTitle->fRate;
    fVideoStreamHeader.Length++;

    fVideoStreamFormat.FourCC      = FOURCC( "strf" );
    fVideoStreamFormat.BytesCount  = BITMAP_INFO_SIZE - 8;
    fVideoStreamFormat.Size        = BITMAP_INFO_SIZE - 8;
    fVideoStreamFormat.Width       = fTitle->fOutWidth;
    fVideoStreamFormat.Height      = fTitle->fOutHeight;
    fVideoStreamFormat.Planes      = 1;
    fVideoStreamFormat.BitCount    = 24;
    fVideoStreamFormat.Compression = FOURCC( "DIVX" );;

    UpdateMainHeader();

    fseek( fFile, 0, SEEK_END );

    /* Update the index */
    fIndex->WriteInt32( FOURCC( "00dc" ) );
    fIndex->WriteInt32( fVideoBuffer->fKeyFrame ? AVIIF_KEYFRAME : 0 );
    fIndex->WriteInt32( ftell( fFile ) - 2044 );
    fIndex->WriteInt32( fVideoBuffer->fSize );

    /* Write the chunk */
    WriteInt32( fFile, FOURCC( "00dc" ) );
    WriteInt32( fFile, fVideoBuffer->fSize );
    WriteBuffer( fFile, fVideoBuffer );

    /* Chunks must be 2-bytes aligned */
    if( fVideoBuffer->fSize & 1 )
    {
        WriteInt8( fFile, 0 );
    }

    delete fVideoBuffer;
    fVideoBuffer = NULL;

    return true;
}

bool HBAviMuxer::AddAudioChunk( int track )
{
    HBAudio         * info;
    HBBuffer        * buffer;
    AviStreamHeader * streamHeader;
    WaveFormatEx    * streamFormat;

    if( track == 1 )
    {
        info         = fAudio1;
        buffer       = fAudio1Buffer;
        streamHeader = &fAudio1StreamHeader;
        streamFormat = &fAudio1StreamFormat;
    }
    else
    {
        info         = fAudio2;
        buffer       = fAudio2Buffer;
        streamHeader = &fAudio2StreamHeader;
        streamFormat = &fAudio2StreamFormat;
    }

    fRiffBytesCount += 8 + EVEN( buffer->fSize );
    fMoviBytesCount += 8 + EVEN( buffer->fSize );

    streamHeader->FourCC         = FOURCC( "strh" );
    streamHeader->BytesCount     = AVI_STREAM_HEADER_SIZE - 8;
    streamHeader->Type           = FOURCC( "auds" );
    streamHeader->InitialFrames  = 1;
    streamHeader->Scale          = 1152;
    streamHeader->Rate           = info->fOutSampleRate;
    streamHeader->Length++;
    streamHeader->Quality        = 0xFFFFFFFF;


    streamFormat->FourCC         = FOURCC( "strf" );
    streamFormat->BytesCount     = WAVE_FORMAT_EX_SIZE - 8;
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
    if( track == 1 )
    {
        fIndex->WriteInt32( FOURCC( "01wb" ) );
    }
    else
    {
        fIndex->WriteInt32( FOURCC( "02wb" ) );
    }
    fIndex->WriteInt32( buffer->fKeyFrame ? AVIIF_KEYFRAME : 0 );
    fIndex->WriteInt32( ftell( fFile ) - 2044 );
    fIndex->WriteInt32( buffer->fSize );

    /* Write the chunk */
    WriteInt32( fFile,
                ( track == 1 ) ? FOURCC( "01wb" ) : FOURCC( "02wb" ) );
    WriteInt32( fFile, buffer->fSize );
    WriteBuffer( fFile, buffer );

    /* Chunks must be 2-bytes aligned */
    if( buffer->fSize & 1 )
    {
        WriteInt8( fFile, 0 );
    }

    delete buffer;
    if( track == 1 )
    {
        fAudio1Buffer = NULL;
    }
    else
    {
        fAudio2Buffer = NULL;
    }

    return true;
}

void HBAviMuxer::UpdateMainHeader()
{
    fMainHeader.FourCC     = FOURCC( "avih" );
    fMainHeader.BytesCount = AVI_MAIN_HEADER_SIZE - 8;
    fMainHeader.Streams    = 1 + ( fAudio1 ? 1 : 0 ) +
                             ( fAudio2 ? 1 : 0 );

    fHdrlBytesCount = 4 + AVI_MAIN_HEADER_SIZE + 12 +
                      AVI_STREAM_HEADER_SIZE + BITMAP_INFO_SIZE;

    if( fAudio1 )
    {
        fHdrlBytesCount += 12 + AVI_STREAM_HEADER_SIZE +
                           WAVE_FORMAT_EX_SIZE;
    }
    if( fAudio2 )
    {
        fHdrlBytesCount += 12 + AVI_STREAM_HEADER_SIZE +
                           WAVE_FORMAT_EX_SIZE;
    }

    fseek( fFile, 0, SEEK_SET );
    WriteInt32( fFile, FOURCC( "RIFF" ) );
    WriteInt32( fFile, fRiffBytesCount );
    WriteInt32( fFile, FOURCC( "AVI " ) );
    WriteInt32( fFile, FOURCC( "LIST" ) );
    WriteInt32( fFile, fHdrlBytesCount );
    WriteInt32( fFile, FOURCC( "hdrl" ) );

    WriteMainHeader( fFile, &fMainHeader );

    int strlSize;
    strlSize = 4 + AVI_STREAM_HEADER_SIZE + BITMAP_INFO_SIZE;
    WriteInt32( fFile, FOURCC( "LIST" ) );
    WriteInt32( fFile, strlSize );
    WriteInt32( fFile, FOURCC( "strl" ) );

    WriteStreamHeader( fFile, &fVideoStreamHeader );
    WriteBitmapInfo( fFile, &fVideoStreamFormat );

    if( fAudio1 )
    {
        strlSize = 4 + AVI_STREAM_HEADER_SIZE + WAVE_FORMAT_EX_SIZE;
        WriteInt32( fFile, FOURCC( "LIST" ) );
        WriteInt32( fFile, strlSize );
        WriteInt32( fFile, FOURCC( "strl" ) );
        WriteStreamHeader( fFile, &fAudio1StreamHeader );
        WriteWaveFormatEx( fFile, &fAudio1StreamFormat );
    }

    if( fAudio2 )
    {
        strlSize = 4 + AVI_STREAM_HEADER_SIZE + WAVE_FORMAT_EX_SIZE;
        WriteInt32( fFile, FOURCC( "LIST" ) );
        WriteInt32( fFile, strlSize );
        WriteInt32( fFile, FOURCC( "strl" ) );
        WriteStreamHeader( fFile, &fAudio2StreamHeader );
        WriteWaveFormatEx( fFile, &fAudio2StreamFormat );
    }

    /* a JUNK chunk to fill the free space.
       size = 2048 -/
              12 ("RIFFxxxxAVI ") -
              8  (hdrl's "LIS1Txxxx") -
              fHdrlBytesCount -
              8  ("JUNKxxxx") -
              12 ("LISTxxxxmovi) */
    int junkSize = 2008 - fHdrlBytesCount;
    WriteInt32( fFile, FOURCC( "JUNK" ) );
    WriteInt32( fFile, junkSize );
    for( uint32_t i = 0; i < 2008 - fHdrlBytesCount; i++ )
    {
        WriteInt8( fFile, 0 );
    }

    /* movi list */
    WriteInt32( fFile, FOURCC( "LIST" ) );
    WriteInt32( fFile, fMoviBytesCount );
    WriteInt32( fFile, FOURCC( "movi" ) );
}
