/* $Id: AviMux.c,v 1.5 2003/11/06 18:35:53 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "AviMux.h"
#include "Fifo.h"
#include "Thread.h"

/* Local structures */
typedef struct AviMainHeader   AviMainHeader;
typedef struct AviStreamHeader AviStreamHeader;
typedef struct BitmapInfo      BitmapInfo;
typedef struct WaveFormatEx    WaveFormatEx;

/* Local prototypes */
static void       AviMuxThread( void * );
static void       InitAviHeaders( HBAviMux * );
static void       AddChunk( HBAviMux *, HBBuffer **, uint32_t,
                            AviStreamHeader * );
static void       AddIndex( HBAviMux * );
static void       WriteInt8( FILE *, uint8_t );
static void       WriteInt16( FILE *, uint16_t );
static void       WriteInt32( FILE *, uint32_t );
static void       WriteBuffer( FILE *, HBBuffer * );
static void       WriteMainHeader( FILE *, AviMainHeader * );
static void       WriteStreamHeader( FILE *, AviStreamHeader * );
static void       WriteBitmapInfo( FILE *, BitmapInfo * );
static void       WriteWaveFormatEx( FILE *, WaveFormatEx * );
static void       IndexAddInt32( HBBuffer * buffer, uint32_t );
static HBBuffer * Pop( HBAviMux *, HBFifo * );

#define AVIF_HASINDEX  0x10
#define AVIIF_KEYFRAME 0x10
#define FOURCC(a)      ((a[3]<<24)|(a[2]<<16)|(a[1]<<8)|a[0])

/* Structures definitions */
struct __attribute__((__packed__)) AviMainHeader
{
    uint32_t FourCC;
    uint32_t BytesCount;
    uint32_t MicroSecPerFrame;
    uint32_t MaxBytesPerSec;
    uint32_t PaddingGranularity;
    uint32_t Flags;
    uint32_t TotalFrames;
    uint32_t InitialFrames;
    uint32_t Streams;
    uint32_t SuggestedBufferSize;
    uint32_t Width;
    uint32_t Height;
    uint32_t Reserved[4];
};

struct __attribute__((__packed__)) AviStreamHeader
{
    uint32_t FourCC;
    uint32_t BytesCount;
    uint32_t Type;
    uint32_t Handler;
    uint32_t Flags;
    uint16_t Priority;
    uint16_t Language;
    uint32_t InitialFrames;
    uint32_t Scale;
    uint32_t Rate;
    uint32_t Start;
    uint32_t Length;
    uint32_t SuggestedBufferSize;
    uint32_t Quality;
    uint32_t SampleSize;
    int16_t  Left;
    int16_t  Top;
    int16_t  Right;
    int16_t  Bottom;
};

struct __attribute__((__packed__)) BitmapInfo
{
    uint32_t FourCC;
    uint32_t BytesCount;
    uint32_t Size;
    uint32_t Width;
    uint32_t Height;
    uint16_t Planes;
    uint16_t BitCount;
    uint32_t Compression;
    uint32_t SizeImage;
    uint32_t XPelsPerMeter;
    uint32_t YPelsPerMeter;
    uint32_t ClrUsed;
    uint32_t ClrImportant;
    uint8_t  Blue;
    uint8_t  Green;
    uint8_t  Red;
    uint8_t  Reserved;
};

struct __attribute__((__packed__)) WaveFormatEx
{
    uint32_t FourCC;
    uint32_t BytesCount;
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint16_t Size;

    /* mp3 specific */
    uint16_t Id;
    uint32_t Flags;
    uint16_t BlockSize;
    uint16_t FramesPerBlock;
    uint16_t CodecDelay;
};

struct HBAviMux
{
    HBHandle      * handle;
    HBTitle       * title;
    HBAudio       * audio;
    HBAudio       * optAudio;

    AviMainHeader   mainHeader;
    AviStreamHeader videoHeader;
    BitmapInfo      videoFormat;
    AviStreamHeader audioHeader;
    WaveFormatEx    audioFormat;
    AviStreamHeader optAudioHeader;
    WaveFormatEx    optAudioFormat;

    /* Data size in bytes, not including headers */
    unsigned        size;
    FILE          * file;
    HBBuffer      * index;

    int             die;
    HBThread      * thread;
};

HBAviMux * HBAviMuxInit( HBHandle * handle, HBTitle * title,
                         HBAudio * audio, HBAudio * optAudio )
{
    HBAviMux * a;
    if( !( a = malloc( sizeof( HBAviMux ) ) ) )
    {
        HBLog( "HBAviMuxInit: malloc() failed, gonna crash" );
        return NULL;
    }

    a->handle   = handle;
    a->title    = title;
    a->audio    = audio;
    a->optAudio = optAudio;

    a->size  = 0;
    a->file  = NULL;
    a->index = HBBufferInit( 1024 * 1024 );
    a->index->size = 0;

    a->die    = 0;
    a->thread = HBThreadInit( "avi muxer", AviMuxThread, a,
                              HB_NORMAL_PRIORITY );
    return a;
}

void HBAviMuxClose( HBAviMux ** _a )
{
    HBAviMux * a = *_a;

    a->die = 1;
    HBThreadClose( &a->thread );
    free( a );

    *_a = NULL;
}

static void AviMuxThread( void * _a )
{
    HBAviMux * a        = (HBAviMux*) _a;
    HBTitle  * title    = a->title;
    HBAudio  * audio    = a->audio;
    HBAudio  * optAudio = a->optAudio;

    HBBuffer * videoBuffer    = NULL;
    HBBuffer * audioBuffer    = NULL;
    HBBuffer * optAudioBuffer = NULL;

    /* Open destination file */
    HBLog( "HBAviMux: opening %s", title->file );
    if( !( a->file = fopen( title->file, "w" ) ) )
    {
        HBLog( "HBAviMux: fopen() failed" );
        HBErrorOccured( a->handle, HB_ERROR_AVI_WRITE );
        return;
    }

    /* Get a buffer for each track */
    videoBuffer = Pop( a, title->mpeg4Fifo );
    audioBuffer = Pop( a, audio->mp3Fifo );
    if( optAudio )
    {
        optAudioBuffer = Pop( a, optAudio->mp3Fifo );
    }

    /* Failed ? Then forget it */
    if( !videoBuffer || !audioBuffer ||
        ( optAudio && !optAudioBuffer ) )
    {
        fclose( a->file );
        a->file = NULL;
        
        HBLog( "HBAviMux: deleting %s", title->file );
        unlink( title->file );
        return;
    }

    InitAviHeaders( a );

    for( ;; )
    {
        /* Get a buffer for each track */
        if( !videoBuffer )
        {
            videoBuffer  = Pop( a, title->mpeg4Fifo );
        }
        if( !audioBuffer )
        {
            audioBuffer = Pop( a, audio->mp3Fifo );
        }
        if( optAudio && !optAudioBuffer )
        {
            optAudioBuffer = Pop( a, optAudio->mp3Fifo );
        }

        if( !videoBuffer && !audioBuffer && !optAudioBuffer )
        {
            /* Couldn't get anything -> must exit NOW */
            break;
        }

        /* Interleave frames in the same order than they were in the
           original MPEG stream */
        if( videoBuffer &&
            ( !audioBuffer ||
              videoBuffer->position < audioBuffer->position ) &&
            ( !optAudioBuffer ||
              videoBuffer->position < optAudioBuffer->position ) )
        {
            AddChunk( a, &videoBuffer, FOURCC( "00dc" ),
                      &a->videoHeader );
        }
        else if( audioBuffer &&
                 ( !optAudioBuffer ||
                   audioBuffer->position < optAudioBuffer->position ) )
        {
            AddChunk( a, &audioBuffer, FOURCC( "01wb" ),
                      &a->audioHeader );
        }
        else
        {
            AddChunk( a, &optAudioBuffer, FOURCC( "02wb" ),
                      &a->optAudioHeader );
        }
    }

    AddIndex( a );

    HBLog( "HBAviMux: closing %s", title->file );
    fclose( a->file );
}

static void InitAviHeaders( HBAviMux * a )
{
    HBTitle         * title          = a->title;
    HBAudio         * audio          = a->audio;
    HBAudio         * optAudio       = a->optAudio;
    AviMainHeader   * mainHeader     = &a->mainHeader;
    AviStreamHeader * videoHeader    = &a->videoHeader;
    BitmapInfo      * videoFormat    = &a->videoFormat;
    AviStreamHeader * audioHeader    = &a->audioHeader;
    WaveFormatEx    * audioFormat    = &a->audioFormat;
    AviStreamHeader * optAudioHeader = &a->optAudioHeader;
    WaveFormatEx    * optAudioFormat = &a->optAudioFormat;
    FILE            * file           = a->file;
    int               hdrlBytes;
    int               i;

    /* AVI main header */
    memset( mainHeader, 0, sizeof( AviMainHeader ) );
    mainHeader->FourCC           = FOURCC( "avih" );
    mainHeader->BytesCount       = sizeof( AviMainHeader ) - 8;
    mainHeader->MicroSecPerFrame = (uint64_t) 1000000 *
                                  title->rateBase / title->rate;
    mainHeader->Streams          = optAudio ? 3 : 2;
    mainHeader->Width            = title->outWidth;
    mainHeader->Height           = title->outHeight;

    /* Video stream header */
    memset( videoHeader, 0, sizeof( AviStreamHeader ) );
    videoHeader->FourCC     = FOURCC( "strh" );
    videoHeader->BytesCount = sizeof( AviStreamHeader ) - 8;
    videoHeader->Type       = FOURCC( "vids" );
    
    if( title->codec == HB_CODEC_FFMPEG )
        videoHeader->Handler    = FOURCC( "divx" );
    else if( title->codec == HB_CODEC_XVID )
        videoHeader->Handler    = FOURCC( "xvid" );
    
    videoHeader->Scale      = title->rateBase;
    videoHeader->Rate       = title->rate;

    /* Video stream format */
    memset( videoFormat, 0, sizeof( BitmapInfo ) );
    videoFormat->FourCC      = FOURCC( "strf" );
    videoFormat->BytesCount  = sizeof( BitmapInfo ) - 8;
    videoFormat->Size        = sizeof( BitmapInfo ) - 8;
    videoFormat->Width       = title->outWidth;
    videoFormat->Height      = title->outHeight;
    videoFormat->Planes      = 1;
    videoFormat->BitCount    = 24;
    if( title->codec == HB_CODEC_FFMPEG )
        videoFormat->Compression = FOURCC( "DX50" );
    else if( title->codec == HB_CODEC_XVID )
        videoFormat->Compression = FOURCC( "XVID" );

    /* Audio stream header */
    memset( audioHeader, 0, sizeof( AviStreamHeader ) );
    audioHeader->FourCC        = FOURCC( "strh" );
    audioHeader->BytesCount    = sizeof( AviStreamHeader ) - 8;
    audioHeader->Type          = FOURCC( "auds" );
    audioHeader->InitialFrames = 1;
    audioHeader->Scale         = 1152;
    audioHeader->Rate          = audio->outSampleRate;
    audioHeader->Quality       = 0xFFFFFFFF;

    /* Audio stream format */
    memset( audioFormat, 0, sizeof( WaveFormatEx ) );
    audioFormat->FourCC         = FOURCC( "strf" );
    audioFormat->BytesCount     = sizeof( WaveFormatEx ) - 8;
    audioFormat->FormatTag      = 0x55;
    audioFormat->Channels       = 2;
    audioFormat->SamplesPerSec  = audio->outSampleRate;
    audioFormat->AvgBytesPerSec = audio->outBitrate * 1024 / 8;
    audioFormat->BlockAlign     = 1152;
    audioFormat->Size           = 12;
    audioFormat->Id             = 1;
    audioFormat->Flags          = 2;
    audioFormat->BlockSize      = 1152;
    audioFormat->FramesPerBlock = 1;
    audioFormat->CodecDelay     = 1393;

    if( optAudio )
    {
        /* optAudio stream header */
        memset( optAudioHeader, 0, sizeof( AviStreamHeader ) );
        optAudioHeader->FourCC        = FOURCC( "strh" );
        optAudioHeader->BytesCount    = sizeof( AviStreamHeader ) - 8;
        optAudioHeader->Type          = FOURCC( "auds" );
        optAudioHeader->InitialFrames = 1;
        optAudioHeader->Scale         = 1152;
        optAudioHeader->Rate          = optAudio->outSampleRate;
        optAudioHeader->Quality       = 0xFFFFFFFF;

        /* optAudio stream format */
        memset( optAudioFormat, 0, sizeof( WaveFormatEx ) );
        optAudioFormat->FourCC         = FOURCC( "strf" );
        optAudioFormat->BytesCount     = sizeof( WaveFormatEx ) - 8;
        optAudioFormat->FormatTag      = 0x55;
        optAudioFormat->Channels       = 2;
        optAudioFormat->SamplesPerSec  = optAudio->outSampleRate;
        optAudioFormat->AvgBytesPerSec = optAudio->outBitrate * 1024 / 8;
        optAudioFormat->BlockAlign     = 1152;
        optAudioFormat->Size           = 12;
        optAudioFormat->Id             = 1;
        optAudioFormat->Flags          = 2;
        optAudioFormat->BlockSize      = 1152;
        optAudioFormat->FramesPerBlock = 1;
        optAudioFormat->CodecDelay     = 1393;
    }

    hdrlBytes = 4 + sizeof( AviMainHeader ) +
                ( optAudio ? 3 : 2 ) * ( 12 + sizeof( AviStreamHeader ) ) +
                sizeof( BitmapInfo ) +
                ( optAudio ? 2 : 1 ) * sizeof( WaveFormatEx );

    /* Here we really start to write into the file */

    WriteInt32( file, FOURCC( "RIFF" ) );
    WriteInt32( file, 2040 );
    WriteInt32( file, FOURCC( "AVI " ) );
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, hdrlBytes );
    WriteInt32( file, FOURCC( "hdrl" ) );
    WriteMainHeader( file, mainHeader );
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, 4 + sizeof( AviStreamHeader ) +
                      sizeof( BitmapInfo ) );
    WriteInt32( file, FOURCC( "strl" ) );
    WriteStreamHeader( file, videoHeader );
    WriteBitmapInfo( file, videoFormat );
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, 4 + sizeof( AviStreamHeader ) +
                      sizeof( WaveFormatEx ) );
    WriteInt32( file, FOURCC( "strl" ) );
    WriteStreamHeader( file, audioHeader );
    WriteWaveFormatEx( file, audioFormat );
    if( optAudio )
    {
        WriteInt32( file, FOURCC( "LIST" ) );
        WriteInt32( file, 4 + sizeof( AviStreamHeader ) +
                          sizeof( WaveFormatEx ) );
        WriteInt32( file, FOURCC( "strl" ) );
        WriteStreamHeader( file, optAudioHeader );
        WriteWaveFormatEx( file, optAudioFormat );
    }
    WriteInt32( file, FOURCC( "JUNK" ) );
    WriteInt32( file, 2008 - hdrlBytes );
    for( i = 0; i < 2008 - hdrlBytes; i++ )
    {
        WriteInt8( file, 0 );
    }
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, 4 );
    WriteInt32( file, FOURCC( "movi" ) );
}

static void AddChunk( HBAviMux * a, HBBuffer ** _buffer,
                      uint32_t fourCC, AviStreamHeader * header )
{
    HBBuffer * buffer = *_buffer;
    
    /* Update index */
    IndexAddInt32( a->index, fourCC );
    IndexAddInt32( a->index, buffer->keyFrame ? AVIIF_KEYFRAME : 0 );
    IndexAddInt32( a->index, 4 + a->size );
    IndexAddInt32( a->index, buffer->size );

    /* Write the chunk to the file */
    fseek( a->file, 0, SEEK_END );
    WriteInt32( a->file, fourCC );
    WriteInt32( a->file, buffer->size );
    WriteBuffer( a->file, buffer );

    /* Chunks must be 2-bytes aligned */
    if( buffer->size & 1 )
    {
        WriteInt8( a->file, 0 );
    }

    /* Update headers */
    a->size += 8 + EVEN( buffer->size );
    header->Length++;

    /* RIFF size */
    fseek( a->file, 4, SEEK_SET );
    WriteInt32( a->file, 2040 + a->size );

    /* AviStreamHeader's length */
    fseek( a->file, 140, SEEK_SET );
    WriteInt32( a->file, a->videoHeader.Length );
    fseek( a->file, 268, SEEK_SET );
    WriteInt32( a->file, a->audioHeader.Length );
    if( a->optAudio )
    {
        fseek( a->file, 382, SEEK_SET );
        WriteInt32( a->file, a->optAudioHeader.Length );
    }

    /* movi size */
    fseek( a->file, 2040, SEEK_SET );
    WriteInt32( a->file, 4 + a->size );

    HBBufferClose( _buffer );
}

static void AddIndex( HBAviMux * a )
{
    fseek( a->file, 0, SEEK_END );
    
    WriteInt32( a->file, FOURCC( "idx1" ) );
    WriteInt32( a->file, a->index->size );
    WriteBuffer( a->file, a->index );

    a->size += 8 + a->index->size;
    fseek( a->file, 4, SEEK_SET );
    WriteInt32( a->file, 2040 + a->size );
    a->mainHeader.Flags |= AVIF_HASINDEX;
    fseek( a->file, 24, SEEK_SET );
    WriteMainHeader( a->file, &a->mainHeader );
}

static void WriteInt8( FILE * file, uint8_t val )
{
    fputc( val, file );
}

static void WriteInt16( FILE * file, uint16_t val )
{
    fputc( val & 0xFF, file );
    fputc( val >> 8, file );
}

static void WriteInt32( FILE * file, uint32_t val )
{
    fputc( val & 0xFF, file );
    fputc( ( val >> 8 ) & 0xFF, file );
    fputc( ( val >> 16 ) & 0xFF, file );
    fputc( val >> 24, file );
}

static void WriteBuffer( FILE * file, HBBuffer * buffer )
{
    fwrite( buffer->data, buffer->size, 1, file );
}

static void WriteBitmapInfo( FILE * file, BitmapInfo * bitmapInfo )
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

static void WriteWaveFormatEx( FILE * file, WaveFormatEx * waveFormatEx )
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

static void WriteMainHeader( FILE * file, AviMainHeader * mainHeader )
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

static void WriteStreamHeader( FILE * file, AviStreamHeader * streamHeader )
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

static void IndexAddInt32( HBBuffer * b, uint32_t val )
{
    if( b->size + 16 > b->alloc )
    {
        HBLog( "HBAviMux: reallocing index (%d MB)",
               1 + b->alloc / 1024 / 1024 );
        HBBufferReAlloc( b, b->alloc + 1024 + 1024 );
    }

    b->data[b->size++] = val & 0xFF;
    b->data[b->size++] = ( val >> 8 ) & 0xFF;
    b->data[b->size++] = ( val >> 16 ) & 0xFF;
    b->data[b->size++] = val >> 24;
}

static HBBuffer * Pop( HBAviMux * a, HBFifo * fifo )
{
    HBBuffer * buffer;

    for( ;; )
    {
        HBCheckPaused( a->handle );

        if( ( buffer = HBFifoPop( fifo ) ) )
        {
            return buffer;
        }

        if( a->die )
        {
            break;
        }

        HBSnooze( 10000 );
    }

    return NULL;
}

