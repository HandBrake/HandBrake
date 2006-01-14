/* $Id: AviMux.c,v 1.24 2004/05/12 18:02:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#define AVIF_HASINDEX  0x10
#define AVIIF_KEYFRAME 0x10
#define FOURCC(a)      ((a[3]<<24)|(a[2]<<16)|(a[1]<<8)|a[0])

/* Structures definitions */
typedef struct __attribute__((__packed__))
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

} HBAviMainHeader;

typedef struct __attribute__((__packed__))
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

} HBAviStreamHeader;

typedef struct __attribute__((__packed__))
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

} HBBitmapInfo;

typedef struct __attribute__((__packed__))
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

} HBWaveFormatEx;

struct HBMux
{
    HB_MUX_COMMON_MEMBERS

    HBHandle      * handle;
    HBTitle       * title;

    /* Data size in bytes, not including headers */
    unsigned        size;

    FILE          * file;
    HBBuffer      * index;
};

typedef struct
{
    HBAviMainHeader   mainHeader;
    HBAviStreamHeader videoHeader;
    HBBitmapInfo      videoFormat;

} AviVideoMuxData;

typedef struct
{
    uint32_t          fourCC;
    HBAviStreamHeader audioHeader;
    HBWaveFormatEx    audioFormat;

} AviAudioMuxData;

/* Local prototypes */
static int  AviStart( HBMux * );
static int  AviMuxVideo( HBMux *, void *, HBBuffer *);
static int  AviMuxAudio( HBMux *, void *, HBBuffer *);
static int  AviEnd( HBMux * );

static void AddChunk( HBMux *, HBBuffer *, uint32_t,
                      HBAviStreamHeader * );
static void AddIndex( HBMux * );
static void WriteInt8( FILE *, uint8_t );
static void WriteInt16( FILE *, uint16_t );
static void WriteInt32( FILE *, uint32_t );
static void WriteBuffer( FILE *, HBBuffer * );
static void WriteMainHeader( FILE *, HBAviMainHeader * );
static void WriteStreamHeader( FILE *, HBAviStreamHeader * );
static void WriteHBBitmapInfo( FILE *, HBBitmapInfo * );
static void WriteHBWaveFormatEx( FILE *, HBWaveFormatEx * );
static void IndexAddInt32( HBBuffer * buffer, uint32_t );

HBMux * HBAviMuxInit( HBHandle * handle, HBTitle * title )
{
    HBMux   * m;
    HBAudio * audio;
    int i;

    if( !( m = calloc( sizeof( HBMux ), 1 ) ) )
    {
        HBLog( "HBAviMux: malloc() failed, gonna crash" );
        return NULL;
    }
    m->start    = AviStart;
    m->muxVideo = AviMuxVideo;
    m->muxAudio = AviMuxAudio;
    m->end      = AviEnd;
    m->handle   = handle;
    m->title    = title;

    m->file  = NULL;
    m->index = HBBufferInit( 1024 * 1024 );
    m->index->size = 0;

    /* Alloc muxer data */
    title->muxData = calloc( sizeof( AviVideoMuxData ), 1 );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        audio->muxData = calloc( sizeof( AviAudioMuxData ), 1 );
    }

    return m;
}

void HBAviMuxClose( HBMux ** _m )
{
    HBMux   * m     = *_m;
    HBTitle * title = m->title;
    HBAudio * audio;
    int       i;

    /* Free muxer data */
    free( title->muxData );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        free( audio->muxData );
    }

    HBBufferClose( &m->index );

    free( m );

    *_m = NULL;
}

#define mh videoMuxData->mainHeader
#define vh videoMuxData->videoHeader
#define vf videoMuxData->videoFormat
#define ah audioMuxData->audioHeader
#define af audioMuxData->audioFormat
static int AviStart( HBMux * m )
{
    HBTitle         * title        = m->title;
    int               audioCount   = HBListCount( title->ripAudioList );
    AviVideoMuxData * videoMuxData = (AviVideoMuxData *) title->muxData;
    AviAudioMuxData * audioMuxData;
    HBAudio         * audio;
    int               hdrlBytes;
    int               i;

    /* Open destination file */
    HBLog( "HBAviMux: opening %s", title->file );
    if( !( m->file = fopen( title->file, "wb" ) ) )
    {
        HBLog( "HBAviMux: fopen() failed" );
        HBErrorOccured( m->handle, HB_ERROR_AVI_WRITE );
        return 1;
    }

    /* AVI main header */
    mh.FourCC           = FOURCC( "avih" );
    mh.BytesCount       = sizeof( HBAviMainHeader ) - 8;
    mh.MicroSecPerFrame = (uint64_t) 1000000 * title->rateBase /
                              title->rate;
    mh.Streams          = 1 + audioCount;
    mh.Width            = title->outWidth;
    mh.Height           = title->outHeight;

    /* Video stream header */
    vh.FourCC     = FOURCC( "strh" );
    vh.BytesCount = sizeof( HBAviStreamHeader ) - 8;
    vh.Type       = FOURCC( "vids" );

    if( title->codec == HB_CODEC_FFMPEG )
        vh.Handler = FOURCC( "divx" );
    else if( title->codec == HB_CODEC_XVID )
        vh.Handler = FOURCC( "xvid" );
    else if( title->codec == HB_CODEC_X264 )
        vh.Handler = FOURCC( "h264" );

    vh.Scale      = title->rateBase;
    vh.Rate       = title->rate;

    /* Video stream format */
    vf.FourCC      = FOURCC( "strf" );
    vf.BytesCount  = sizeof( HBBitmapInfo ) - 8;
    vf.Size        = sizeof( HBBitmapInfo ) - 8;
    vf.Width       = title->outWidth;
    vf.Height      = title->outHeight;
    vf.Planes      = 1;
    vf.BitCount    = 24;
    if( title->codec == HB_CODEC_FFMPEG )
        vf.Compression = FOURCC( "DX50" );
    else if( title->codec == HB_CODEC_XVID )
        vf.Compression = FOURCC( "XVID" );
    else if( title->codec == HB_CODEC_X264 )
        vf.Compression = FOURCC( "H264" );

    for( i = 0; i < audioCount; i++ )
    {
        audio        = HBListItemAt( title->ripAudioList, i );
        audioMuxData = (AviAudioMuxData *) audio->muxData;

        /* Audio stream header */
        ah.FourCC        = FOURCC( "strh" );
        ah.BytesCount    = sizeof( HBAviStreamHeader ) - 8;
        ah.Type          = FOURCC( "auds" );
        ah.InitialFrames = 1;
        ah.Scale         = 1;
        ah.Rate          = audio->outBitrate * 1000 / 8;
        ah.SampleSize    = 1;
        ah.Quality       = 0xFFFFFFFF;

        /* Audio stream format */
        af.FourCC         = FOURCC( "strf" );
        af.BytesCount     = sizeof( HBWaveFormatEx ) - 8;
        af.FormatTag      = 0x55;
        af.Channels       = 2;
        af.SamplesPerSec  = audio->outSampleRate;
        af.AvgBytesPerSec = audio->outBitrate * 1000 / 8;
        af.BlockAlign     = 1;
        af.Size           = 12;
        af.Id             = 1;
        af.Flags          = 2;
        af.BlockSize      = 1152 * af.AvgBytesPerSec /
                                audio->outSampleRate;
        af.FramesPerBlock = 1;
        af.CodecDelay     = 1393;
    }

    hdrlBytes = 4 + sizeof( HBAviMainHeader ) + ( 1 + audioCount ) *
        ( 12 + sizeof( HBAviStreamHeader ) ) + sizeof( HBBitmapInfo ) +
        audioCount * sizeof( HBWaveFormatEx );

    /* Here we really start to write into the file */

    WriteInt32( m->file, FOURCC( "RIFF" ) );
    WriteInt32( m->file, 2040 );
    WriteInt32( m->file, FOURCC( "AVI " ) );
    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, hdrlBytes );
    WriteInt32( m->file, FOURCC( "hdrl" ) );
    WriteMainHeader( m->file, &mh );
    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, 4 + sizeof( HBAviStreamHeader ) +
                sizeof( HBBitmapInfo ) );
    WriteInt32( m->file, FOURCC( "strl" ) );
    WriteStreamHeader( m->file, &vh );
    WriteHBBitmapInfo( m->file, &vf );

    for( i = 0; i < audioCount; i++ )
    {
        char fourCC[5];

        audio = HBListItemAt( title->ripAudioList, i );
        audioMuxData = (AviAudioMuxData *) audio->muxData;

        snprintf( fourCC, 5, "%02dwb", i + 1 );
        audioMuxData->fourCC = FOURCC( fourCC );

        WriteInt32( m->file, FOURCC( "LIST" ) );
        WriteInt32( m->file, 4 + sizeof( HBAviStreamHeader ) +
                          sizeof( HBWaveFormatEx ) );
        WriteInt32( m->file, FOURCC( "strl" ) );
        WriteStreamHeader( m->file, &ah );
        WriteHBWaveFormatEx( m->file, &af );
    }

    WriteInt32( m->file, FOURCC( "JUNK" ) );
    WriteInt32( m->file, 2008 - hdrlBytes );
    for( i = 0; i < 2008 - hdrlBytes; i++ )
    {
        WriteInt8( m->file, 0 );
    }
    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, 4 );
    WriteInt32( m->file, FOURCC( "movi" ) );

    return 0;
}

static int AviMuxVideo( HBMux * m, void * _muxData, HBBuffer * buffer )
{
    AviVideoMuxData * muxData = (AviVideoMuxData *) _muxData;
    AddChunk( m, buffer, FOURCC( "00dc" ), &muxData->videoHeader );
    return 0;
}

static int AviMuxAudio( HBMux * m, void * _muxData, HBBuffer * buffer )
{
    AviAudioMuxData * muxData = (AviAudioMuxData *) _muxData;
    AddChunk( m, buffer, muxData->fourCC, &muxData->audioHeader );
    return 0;
}

static int AviEnd( HBMux * m )
{
    AddIndex( m );
    fclose( m->file );
    return 0;
}
#undef mh
#undef vh
#undef vf
#undef ah
#undef af

static void AddChunk( HBMux * m, HBBuffer * buffer,
                      uint32_t fourCC, HBAviStreamHeader * header )
{
    HBTitle * title = m->title;
    AviVideoMuxData * videoMuxData = (AviVideoMuxData *) title->muxData;
    AviAudioMuxData * audioMuxData;

    HBAudio * audio;
    int i;

    /* Update index */
    IndexAddInt32( m->index, fourCC );
    IndexAddInt32( m->index, buffer->keyFrame ? AVIIF_KEYFRAME : 0 );
    IndexAddInt32( m->index, 4 + m->size );
    IndexAddInt32( m->index, buffer->size );

    /* Write the chunk to the file */
    fseek( m->file, 0, SEEK_END );
    WriteInt32( m->file, fourCC );
    WriteInt32( m->file, buffer->size );
    WriteBuffer( m->file, buffer );

    /* Chunks must be 2-bytes aligned */
    if( buffer->size & 1 )
    {
        WriteInt8( m->file, 0 );
    }

    /* Update headers */
    m->size += 8 + EVEN( buffer->size );
    header->Length++;

    /* RIFF size */
    fseek( m->file, 4, SEEK_SET );
    WriteInt32( m->file, 2040 + m->size );

    /* HBAviStreamHeader's lengths */
    fseek( m->file, 140, SEEK_SET );
    WriteInt32( m->file, videoMuxData->videoHeader.Length );

    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio*) HBListItemAt( title->ripAudioList, i );
        audioMuxData = (AviAudioMuxData *) audio->muxData;
        fseek( m->file, 268 + i * 114, SEEK_SET );
        WriteInt32( m->file, audioMuxData->audioHeader.Length );
    }

    /* movi size */
    fseek( m->file, 2040, SEEK_SET );
    WriteInt32( m->file, 4 + m->size );
}

static void AddIndex( HBMux * m )
{
    HBTitle * title = m->title;
    AviVideoMuxData * videoMuxData = (AviVideoMuxData *) title->muxData;

    fseek( m->file, 0, SEEK_END );

    WriteInt32( m->file, FOURCC( "idx1" ) );
    WriteInt32( m->file, m->index->size );
    WriteBuffer( m->file, m->index );

    m->size += 8 + m->index->size;
    fseek( m->file, 4, SEEK_SET );
    WriteInt32( m->file, 2040 + m->size );
    videoMuxData->mainHeader.Flags |= AVIF_HASINDEX;
    fseek( m->file, 24, SEEK_SET );
    WriteMainHeader( m->file, &videoMuxData->mainHeader );
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

