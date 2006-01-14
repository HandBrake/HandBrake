/* $Id: AviMux.c,v 1.15 2004/02/18 17:07:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

int64_t videoFrames;
int64_t videoBytes;
int64_t audioFrames;
int64_t audioBytes;

/* Local prototypes */
static void       AviMuxThread( void * );
static void       InitAviHeaders( HBAviMux * );
static void       AddChunk( HBAviMux *, HBBuffer *, uint32_t,
                            HBAviStreamHeader * );
static void       AddIndex( HBAviMux * );
static void       WriteInt8( FILE *, uint8_t );
static void       WriteInt16( FILE *, uint16_t );
static void       WriteInt32( FILE *, uint32_t );
static void       WriteBuffer( FILE *, HBBuffer * );
static void       WriteMainHeader( FILE *, HBAviMainHeader * );
static void       WriteStreamHeader( FILE *, HBAviStreamHeader * );
static void       WriteHBBitmapInfo( FILE *, HBBitmapInfo * );
static void       WriteHBWaveFormatEx( FILE *, HBWaveFormatEx * );
static void       IndexAddInt32( HBBuffer * buffer, uint32_t );

#define AVIF_HASINDEX  0x10
#define AVIIF_KEYFRAME 0x10
#define FOURCC(a)      ((a[3]<<24)|(a[2]<<16)|(a[1]<<8)|a[0])

/* Structures definitions */
struct __attribute__((__packed__)) HBAviMainHeader
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

struct __attribute__((__packed__)) HBAviStreamHeader
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

struct __attribute__((__packed__)) HBBitmapInfo
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
};

struct __attribute__((__packed__)) HBWaveFormatEx
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

    /* Data size in bytes, not including headers */
    unsigned        size;

    FILE          * file;
    HBBuffer      * index;

    volatile int    die;
    HBThread      * thread;
};

HBAviMux * HBAviMuxInit( HBHandle * handle, HBTitle * title )
{
    HBAviMux * a;
    if( !( a = malloc( sizeof( HBAviMux ) ) ) )
    {
        HBLog( "HBAviMuxInit: malloc() failed, gonna crash" );
        return NULL;
    }

    a->handle   = handle;
    a->title    = title;

    videoFrames = 0;
    videoBytes  = 0;
    audioFrames = 0;
    audioBytes  = 0;

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
    FILE * file;
    long   size;

    a->die = 1;
    HBThreadClose( &a->thread );

    file = fopen( a->title->file, "r" );
    fseek( file, 0, SEEK_END );
    size = ftell( file );
    fclose( file );

    HBLog( "HBAviMux: videoFrames=%lld, %lld bytes", videoFrames, videoBytes );
    HBLog( "HBAviMux: audioFrames=%lld, %lld bytes", audioFrames, audioBytes );
    HBLog( "HBAviMux: overhead=%.2f bytes / frame",
            ( (float) size - videoBytes - audioBytes ) /
            ( videoFrames + audioFrames ) );

    free( a );

    *_a = NULL;
}

static void AviMuxThread( void * _a )
{
    HBAviMux * a          = (HBAviMux*) _a;
    HBTitle  * title      = a->title;
    int        audioCount = HBListCount( title->ripAudioList );

    HBAudio  * audio;
    HBBuffer * buffer;
    int i;

    /* Open destination file */
    HBLog( "HBAviMux: opening %s", title->file );
    if( !( a->file = fopen( title->file, "w" ) ) )
    {
        HBLog( "HBAviMux: fopen() failed" );
        HBErrorOccured( a->handle, HB_ERROR_AVI_WRITE );
        return;
    }

    /* Wait until we have one encoded frame for each track */
    while( !a->die && !HBFifoSize( title->outFifo ) )
    {
        HBSnooze( 10000 );
    }
    for( i = 0; i < audioCount; i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        while( !a->die && !HBFifoSize( audio->outFifo ) )
        {
            HBSnooze( 10000 );
        }
    }

    if( a->die )
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
        /* Wait until we have one encoded frame for each track */
        if( !HBFifoWait( title->outFifo ) )
        {
            a->die = 1;
        }
        for( i = 0; i < audioCount; i++ )
        {
            audio = HBListItemAt( title->ripAudioList, i );
            if( !HBFifoWait( audio->outFifo ) )
            {
                a->die = 1;
                break;
            }
        }

        if( a->die )
        {
            break;
        }

        /* Interleave frames in the same order than they were in the
           original MPEG stream */
        audio = NULL;
        for( i = 0; i < audioCount; i++ )
        {
            HBAudio * otherAudio;
            otherAudio = HBListItemAt( title->ripAudioList, i );
            if( !audio || HBFifoPosition( otherAudio->outFifo ) <
                          HBFifoPosition( audio->outFifo ) )
            {
                audio = otherAudio;
            }
        }

        if( audio == NULL ||
            HBFifoPosition( title->outFifo ) <  HBFifoPosition( audio->outFifo ) )
        {
            buffer = HBFifoPop( title->outFifo );
            AddChunk( a, buffer, FOURCC( "00dc" ), title->aviVideoHeader );
            videoFrames++;
            videoBytes += buffer->size;
            HBBufferClose( &buffer );
        }
        else
        {
            buffer = HBFifoPop( audio->outFifo );
            AddChunk( a, buffer, audio->aviFourCC, audio->aviAudioHeader );
            audioFrames++;
            audioBytes += buffer->size;
            HBBufferClose( &buffer );
        }
    }

    AddIndex( a );

    HBLog( "HBAviMux: closing %s", title->file );
    fclose( a->file );
}

static void InitAviHeaders( HBAviMux * a )
{
    HBTitle           * title      = a->title;
    FILE              * file       = a->file;
    int                 audioCount = HBListCount( title->ripAudioList );

    HBAudio           * audio;
    HBAviMainHeader   * mainHeader;
    HBAviStreamHeader * videoHeader;
    HBBitmapInfo      * videoFormat;
    HBAviStreamHeader * audioHeader;
    HBWaveFormatEx    * audioFormat;
    int                 hdrlBytes;
    int                 i;

    /* AVI main header */
    mainHeader = calloc( sizeof( HBAviMainHeader ), 1 );

    mainHeader->FourCC           = FOURCC( "avih" );
    mainHeader->BytesCount       = sizeof( HBAviMainHeader ) - 8;
    mainHeader->MicroSecPerFrame = (uint64_t) 1000000 *
                                  title->rateBase / title->rate;
    mainHeader->Streams          = 1 + audioCount;
    mainHeader->Width            = title->outWidth;
    mainHeader->Height           = title->outHeight;

    title->aviMainHeader = mainHeader;

    /* Video stream header */
    videoHeader = calloc( sizeof( HBAviStreamHeader ), 1 );

    videoHeader->FourCC     = FOURCC( "strh" );
    videoHeader->BytesCount = sizeof( HBAviStreamHeader ) - 8;
    videoHeader->Type       = FOURCC( "vids" );

    if( title->codec == HB_CODEC_FFMPEG )
        videoHeader->Handler    = FOURCC( "divx" );
    else if( title->codec == HB_CODEC_XVID )
        videoHeader->Handler    = FOURCC( "xvid" );
    else if( title->codec == HB_CODEC_X264 )
        videoHeader->Handler    = FOURCC( "H264" );

    videoHeader->Scale      = title->rateBase;
    videoHeader->Rate       = title->rate;

    title->aviVideoHeader = videoHeader;

    /* Video stream format */
    videoFormat = calloc( sizeof( HBBitmapInfo ), 1 );

    videoFormat->FourCC      = FOURCC( "strf" );
    videoFormat->BytesCount  = sizeof( HBBitmapInfo ) - 8;
    videoFormat->Size        = sizeof( HBBitmapInfo ) - 8;
    videoFormat->Width       = title->outWidth;
    videoFormat->Height      = title->outHeight;
    videoFormat->Planes      = 1;
    videoFormat->BitCount    = 24;
    if( title->codec == HB_CODEC_FFMPEG )
        videoFormat->Compression = FOURCC( "DX50" );
    else if( title->codec == HB_CODEC_XVID )
        videoFormat->Compression = FOURCC( "XVID" );
    else if( title->codec == HB_CODEC_X264 )
        videoFormat->Compression = FOURCC( "H264" );

    title->aviVideoFormat = videoFormat;

    for( i = 0; i < audioCount; i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );

        /* Audio stream header */
        audioHeader = calloc( sizeof( HBAviStreamHeader ), 1 );

        audioHeader->FourCC        = FOURCC( "strh" );
        audioHeader->BytesCount    = sizeof( HBAviStreamHeader ) - 8;
        audioHeader->Type          = FOURCC( "auds" );
        audioHeader->InitialFrames = 1;
        audioHeader->Scale         = 1152;
        audioHeader->Rate          = audio->outSampleRate;
        audioHeader->Quality       = 0xFFFFFFFF;

        audio->aviAudioHeader = audioHeader;

        /* Audio stream format */
        audioFormat = calloc( sizeof( HBWaveFormatEx ), 1 );

        audioFormat->FourCC         = FOURCC( "strf" );
        audioFormat->BytesCount     = sizeof( HBWaveFormatEx ) - 8;
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

        audio->aviAudioFormat = audioFormat;
    }

    hdrlBytes = 4 + sizeof( HBAviMainHeader ) + ( 1 + audioCount ) *
        ( 12 + sizeof( HBAviStreamHeader ) ) + sizeof( HBBitmapInfo ) +
        audioCount * sizeof( HBWaveFormatEx );

    /* Here we really start to write into the file */

    WriteInt32( file, FOURCC( "RIFF" ) );
    WriteInt32( file, 2040 );
    WriteInt32( file, FOURCC( "AVI " ) );
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, hdrlBytes );
    WriteInt32( file, FOURCC( "hdrl" ) );
    WriteMainHeader( file, title->aviMainHeader );
    WriteInt32( file, FOURCC( "LIST" ) );
    WriteInt32( file, 4 + sizeof( HBAviStreamHeader ) +
                      sizeof( HBBitmapInfo ) );
    WriteInt32( file, FOURCC( "strl" ) );
    WriteStreamHeader( file, title->aviVideoHeader );
    WriteHBBitmapInfo( file, title->aviVideoFormat );

    for( i = 0; i < audioCount; i++ )
    {
        char fourCC[5];

        audio = HBListItemAt( title->ripAudioList, i );

        snprintf( fourCC, 5, "%02dwb", i + 1 );
        audio->aviFourCC = FOURCC( fourCC );

        WriteInt32( file, FOURCC( "LIST" ) );
        WriteInt32( file, 4 + sizeof( HBAviStreamHeader ) +
                          sizeof( HBWaveFormatEx ) );
        WriteInt32( file, FOURCC( "strl" ) );
        WriteStreamHeader( file, audio->aviAudioHeader );
        WriteHBWaveFormatEx( file, audio->aviAudioFormat );
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

static void AddChunk( HBAviMux * a, HBBuffer * buffer,
                      uint32_t fourCC, HBAviStreamHeader * header )
{
    HBTitle * title = a->title;

    HBAudio * audio;
    int i;

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

    /* HBAviStreamHeader's lengths */
    fseek( a->file, 140, SEEK_SET );
    WriteInt32( a->file, title->aviVideoHeader->Length );

    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio*) HBListItemAt( title->ripAudioList, i );
        fseek( a->file, 268 + i * 114, SEEK_SET );
        WriteInt32( a->file, audio->aviAudioHeader->Length );
    }

    /* movi size */
    fseek( a->file, 2040, SEEK_SET );
    WriteInt32( a->file, 4 + a->size );
}

static void AddIndex( HBAviMux * a )
{
    HBTitle * title = a->title;

    fseek( a->file, 0, SEEK_END );

    WriteInt32( a->file, FOURCC( "idx1" ) );
    WriteInt32( a->file, a->index->size );
    WriteBuffer( a->file, a->index );

    a->size += 8 + a->index->size;
    fseek( a->file, 4, SEEK_SET );
    WriteInt32( a->file, 2040 + a->size );
    title->aviMainHeader->Flags |= AVIF_HASINDEX;
    fseek( a->file, 24, SEEK_SET );
    WriteMainHeader( a->file, title->aviMainHeader );
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

static void WriteHBBitmapInfo( FILE * file, HBBitmapInfo * bitmapInfo )
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
#if 0
    WriteInt8( file, bitmapInfo->Blue );
    WriteInt8( file, bitmapInfo->Green );
    WriteInt8( file, bitmapInfo->Red );
    WriteInt8( file, bitmapInfo->Reserved );
#endif
}

static void WriteHBWaveFormatEx( FILE * file, HBWaveFormatEx * waveFormatEx )
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

static void WriteMainHeader( FILE * file, HBAviMainHeader * mainHeader )
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

static void WriteStreamHeader( FILE * file, HBAviStreamHeader * streamHeader )
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
        HBBufferReAlloc( b, b->alloc + 1024 * 1024 );
    }

    b->data[b->size++] = val & 0xFF;
    b->data[b->size++] = ( val >> 8 ) & 0xFF;
    b->data[b->size++] = ( val >> 16 ) & 0xFF;
    b->data[b->size++] = val >> 24;
}

