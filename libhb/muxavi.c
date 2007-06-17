/* $Id: muxavi.c,v 1.10 2005/03/30 18:17:29 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

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

} hb_avi_main_header_t;

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

} hb_avi_stream_header_t;

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

} hb_bitmap_info_t;

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

} hb_wave_formatex_t;

typedef struct __attribute__((__packed__))
{
    uint16_t Id;
    uint32_t Flags;
    uint16_t BlockSize;
    uint16_t FramesPerBlock;
    uint16_t CodecDelay;

} hb_wave_mp3_t;

static void WriteBuffer( FILE * file, hb_buffer_t * buf )
{
    fwrite( buf->data, buf->size, 1, file );
}

/* Little-endian write routines */

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

static void WriteBitmapInfo( FILE * file, hb_bitmap_info_t * info )
{
    WriteInt32( file, info->FourCC );
    WriteInt32( file, info->BytesCount );
    WriteInt32( file, info->Size );
    WriteInt32( file, info->Width );
    WriteInt32( file, info->Height );
    WriteInt16( file, info->Planes );
    WriteInt16( file, info->BitCount );
    WriteInt32( file, info->Compression );
    WriteInt32( file, info->SizeImage );
    WriteInt32( file, info->XPelsPerMeter );
    WriteInt32( file, info->YPelsPerMeter );
    WriteInt32( file, info->ClrUsed );
    WriteInt32( file, info->ClrImportant );
}

static void WriteWaveFormatEx( FILE * file, hb_wave_formatex_t * format )
{
    WriteInt32( file, format->FourCC );
    WriteInt32( file, format->BytesCount );
    WriteInt16( file, format->FormatTag );
    WriteInt16( file, format->Channels );
    WriteInt32( file, format->SamplesPerSec );
    WriteInt32( file, format->AvgBytesPerSec );
    WriteInt16( file, format->BlockAlign );
    WriteInt16( file, format->BitsPerSample );
    WriteInt16( file, format->Size );
}

static void WriteWaveMp3( FILE * file, hb_wave_mp3_t * mp3 )
{
    WriteInt16( file, mp3->Id );
    WriteInt32( file, mp3->Flags );
    WriteInt16( file, mp3->BlockSize );
    WriteInt16( file, mp3->FramesPerBlock );
    WriteInt16( file, mp3->CodecDelay );
}

static void WriteMainHeader( FILE * file, hb_avi_main_header_t * header )
{
    WriteInt32( file, header->FourCC );
    WriteInt32( file, header->BytesCount );
    WriteInt32( file, header->MicroSecPerFrame );
    WriteInt32( file, header->MaxBytesPerSec );
    WriteInt32( file, header->PaddingGranularity );
    WriteInt32( file, header->Flags );
    WriteInt32( file, header->TotalFrames );
    WriteInt32( file, header->InitialFrames );
    WriteInt32( file, header->Streams );
    WriteInt32( file, header->SuggestedBufferSize );
    WriteInt32( file, header->Width );
    WriteInt32( file, header->Height );
    WriteInt32( file, header->Reserved[0] );
    WriteInt32( file, header->Reserved[1] );
    WriteInt32( file, header->Reserved[2] );
    WriteInt32( file, header->Reserved[3] );
}

static void WriteStreamHeader( FILE * file, hb_avi_stream_header_t * header )
{
    WriteInt32( file, header->FourCC );
    WriteInt32( file, header->BytesCount );
    WriteInt32( file, header->Type );
    WriteInt32( file, header->Handler );
    WriteInt32( file, header->Flags );
    WriteInt16( file, header->Priority );
    WriteInt16( file, header->Language );
    WriteInt32( file, header->InitialFrames );
    WriteInt32( file, header->Scale );
    WriteInt32( file, header->Rate );
    WriteInt32( file, header->Start );
    WriteInt32( file, header->Length );
    WriteInt32( file, header->SuggestedBufferSize );
    WriteInt32( file, header->Quality );
    WriteInt32( file, header->SampleSize );
    WriteInt16( file, header->Left );
    WriteInt16( file, header->Top );
    WriteInt16( file, header->Right );
    WriteInt16( file, header->Bottom );
}

static void IndexAddInt32( hb_buffer_t * b, uint32_t val )
{
    if( b->size + 16 > b->alloc )
    {
        hb_log( "muxavi: reallocing index (%d MB)",
                1 + b->alloc / 1024 / 1024 );
        hb_buffer_realloc( b, b->alloc + 1024 * 1024 );
    }

    b->data[b->size++] = val & 0xFF;
    b->data[b->size++] = ( val >> 8 ) & 0xFF;
    b->data[b->size++] = ( val >> 16 ) & 0xFF;
    b->data[b->size++] = val >> 24;
}

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    /* Data size in bytes, not including headers */
    unsigned               size;
    FILE                 * file;
    hb_buffer_t          * index;
    hb_avi_main_header_t   main_header;
};

struct hb_mux_data_s
{
    uint32_t               fourcc;
    hb_avi_stream_header_t header;
    union
    {
        hb_bitmap_info_t   v;
        struct
        {
            hb_wave_formatex_t f;
            hb_wave_mp3_t      m;
        } a;
    } format;
};

static void AddIndex( hb_mux_object_t * m )
{
    fseek( m->file, 0, SEEK_END );

    /* Write the index at the end of the file */
    WriteInt32( m->file, FOURCC( "idx1" ) );
    WriteInt32( m->file, m->index->size );
    WriteBuffer( m->file, m->index );

    /* Update file size */
    m->size += 8 + m->index->size;
    fseek( m->file, 4, SEEK_SET );
    WriteInt32( m->file, 2040 + m->size );

    /* Update HASINDEX flag */
    m->main_header.Flags |= AVIF_HASINDEX;
    fseek( m->file, 24, SEEK_SET );
    WriteMainHeader( m->file, &m->main_header );
}


/**********************************************************************
 * AVIInit
 **********************************************************************
 * Allocates things, create file, initialize and write headers
 *********************************************************************/
static int AVIInit( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;

    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;

    int audio_count = hb_list_count( title->list_audio );
    int is_ac3      = ( job->acodec & HB_ACODEC_AC3 );
    int hdrl_bytes;
    int i;

    /* Allocate index */
    m->index       = hb_buffer_init( 1024 * 1024 );
    m->index->size = 0;

    /* Open destination file */
    hb_log( "muxavi: opening %s", job->file );
    m->file = fopen( job->file, "wb" );

#define m m->main_header
    /* AVI main header */
    m.FourCC           = FOURCC( "avih" );
    m.BytesCount       = sizeof( hb_avi_main_header_t ) - 8;
    m.MicroSecPerFrame = (uint64_t) 1000000 * job->vrate_base / job->vrate;
    m.Streams          = 1 + audio_count;
    m.Width            = job->width;
    m.Height           = job->height;
#undef m

    /* Video track */
    mux_data = calloc( sizeof( hb_mux_data_t ), 1 );
    job->mux_data = mux_data;
    
#define h mux_data->header
    /* Video stream header */
    h.FourCC     = FOURCC( "strh" );
    h.BytesCount = sizeof( hb_avi_stream_header_t ) - 8;
    h.Type       = FOURCC( "vids" );

    if( job->vcodec == HB_VCODEC_FFMPEG )
        h.Handler = FOURCC( "divx" );
    else if( job->vcodec == HB_VCODEC_XVID )
        h.Handler = FOURCC( "xvid" );
    else if( job->vcodec == HB_VCODEC_X264 )
        h.Handler = FOURCC( "h264" );

    h.Scale      = job->vrate_base;
    h.Rate       = job->vrate;
#undef h

#define f mux_data->format.v
    /* Video stream format */
    f.FourCC      = FOURCC( "strf" );
    f.BytesCount  = sizeof( hb_bitmap_info_t ) - 8;
    f.Size        = f.BytesCount;
    f.Width       = job->width;
    f.Height      = job->height;
    f.Planes      = 1;
    f.BitCount    = 24;
    if( job->vcodec == HB_VCODEC_FFMPEG )
        f.Compression = FOURCC( "DX50" );
    else if( job->vcodec == HB_VCODEC_XVID )
        f.Compression = FOURCC( "XVID" );
    else if( job->vcodec == HB_VCODEC_X264 )
        f.Compression = FOURCC( "H264" );
#undef f

    /* Audio tracks */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );

        mux_data = calloc( sizeof( hb_mux_data_t ), 1 );
        audio->mux_data = mux_data;

#define h mux_data->header
#define f mux_data->format.a.f
#define m mux_data->format.a.m
        /* Audio stream header */
        h.FourCC        = FOURCC( "strh" );
        h.BytesCount    = sizeof( hb_avi_stream_header_t ) - 8;
        h.Type          = FOURCC( "auds" );
        h.InitialFrames = 1;
        h.Scale         = 1;
        h.Rate          = is_ac3 ? ( audio->bitrate / 8 ) :
                                   ( job->abitrate * 1000 / 8 );
        h.Quality       = 0xFFFFFFFF;
        h.SampleSize    = 1;

        /* Audio stream format */
        f.FourCC         = FOURCC( "strf" );
        if( is_ac3 )
        {
            f.BytesCount     = sizeof( hb_wave_formatex_t ) - 8;
            f.FormatTag      = 0x2000;
            f.Channels       = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(audio->input_channel_layout);
            f.SamplesPerSec  = audio->rate;
        }
        else
        {
            f.BytesCount     = sizeof( hb_wave_formatex_t ) +
                               sizeof( hb_wave_mp3_t ) - 8;
            f.FormatTag      = 0x55;
            f.Channels       = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(job->audio_mixdowns[i]);
            f.SamplesPerSec  = job->arate;
        }
        f.AvgBytesPerSec = h.Rate;
        f.BlockAlign     = 1;
        if( is_ac3 )
        {
            f.Size       = 0;
        }
        else
        {
            f.Size           = sizeof( hb_wave_mp3_t );
            m.Id             = 1;
            m.Flags          = 2;
            m.BlockSize      = 1152 * f.AvgBytesPerSec / job->arate;
            m.FramesPerBlock = 1;
            m.CodecDelay     = 1393;
        }
#undef h
#undef f
#undef m
    }

    hdrl_bytes =
        /* Main header */
        4 + sizeof( hb_avi_main_header_t ) +
        /* strh for video + audios */
        ( 1 + audio_count ) * ( 12 + sizeof( hb_avi_stream_header_t ) ) +
        /* video strf */
        sizeof( hb_bitmap_info_t ) +
        /* audios strf */
        audio_count * ( sizeof( hb_wave_formatex_t ) +
                        ( is_ac3 ? 0 : sizeof( hb_wave_mp3_t ) ) );

    /* Here we really start to write into the file */

    /* Main headers */
    WriteInt32( m->file, FOURCC( "RIFF" ) );
    WriteInt32( m->file, 2040 );
    WriteInt32( m->file, FOURCC( "AVI " ) );
    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, hdrl_bytes );
    WriteInt32( m->file, FOURCC( "hdrl" ) );
    WriteMainHeader( m->file, &m->main_header );

    /* Video track */
    mux_data          = job->mux_data;
    mux_data->fourcc = FOURCC( "00dc" );

    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, 4 + sizeof( hb_avi_stream_header_t ) +
                sizeof( hb_bitmap_info_t ) );
    WriteInt32( m->file, FOURCC( "strl" ) );
    WriteStreamHeader( m->file, &mux_data->header );
    WriteBitmapInfo( m->file, &mux_data->format.v );

    /* Audio tracks */
    for( i = 0; i < audio_count; i++ )
    {
        char fourcc[4] = "00wb";
        
        audio    = hb_list_item( title->list_audio, i );
        mux_data = audio->mux_data;

        fourcc[1] = '1' + i; /* This is fine as we don't allow more
                                than 8 tracks */
        mux_data->fourcc = FOURCC( fourcc );

        WriteInt32( m->file, FOURCC( "LIST" ) );
        WriteInt32( m->file, 4 + sizeof( hb_avi_stream_header_t ) +
                             sizeof( hb_wave_formatex_t ) +
                             ( is_ac3 ? 0 : sizeof( hb_wave_mp3_t ) ) );
        WriteInt32( m->file, FOURCC( "strl" ) );
        WriteStreamHeader( m->file, &mux_data->header );
        WriteWaveFormatEx( m->file, &mux_data->format.a.f );
        if( !is_ac3 )
        {
            WriteWaveMp3( m->file, &mux_data->format.a.m );
        }
    }

    WriteInt32( m->file, FOURCC( "JUNK" ) );
    WriteInt32( m->file, 2020 - hdrl_bytes );
    for( i = 0; i < 2020 - hdrl_bytes; i++ )
    {
        WriteInt8( m->file, 0 );
    }
    WriteInt32( m->file, FOURCC( "LIST" ) );
    WriteInt32( m->file, 4 );
    WriteInt32( m->file, FOURCC( "movi" ) );

    return 0;
}

static int AVIMux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;

    hb_audio_t * audio;
    int i;

    /* Update index */
    IndexAddInt32( m->index, mux_data->fourcc );
    IndexAddInt32( m->index, (buf->frametype & HB_FRAME_KEY) ? AVIIF_KEYFRAME : 0 );
    IndexAddInt32( m->index, 4 + m->size );
    IndexAddInt32( m->index, buf->size );

    /* Write the chunk to the file */
    fseek( m->file, 0, SEEK_END );
    WriteInt32( m->file, mux_data->fourcc );
    WriteInt32( m->file, buf->size );
    WriteBuffer( m->file, buf );

    /* Chunks must be 2-bytes aligned */
    if( buf->size & 1 )
    {
        WriteInt8( m->file, 0 );
    }

    /* Update headers */ 
    m->size += 8 + EVEN( buf->size );
    mux_data->header.Length++;

    /* RIFF size */ 
    fseek( m->file, 4, SEEK_SET );
    WriteInt32( m->file, 2052 + m->size );

    /* Mmmmh that's not nice */
    fseek( m->file, 140, SEEK_SET );
    WriteInt32( m->file, job->mux_data->header.Length );
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        fseek( m->file, 264 + i *
               ( 102 + ( ( job->acodec & HB_ACODEC_AC3 ) ? 0 :
                 sizeof( hb_wave_mp3_t ) ) ), SEEK_SET );
        WriteInt32( m->file, audio->mux_data->header.Length );
    }

    /* movi size */
    fseek( m->file, 2052, SEEK_SET );
    WriteInt32( m->file, 4 + m->size );
    return 0;
}

static int AVIEnd( hb_mux_object_t * m )
{
    hb_job_t * job = m->job;

    hb_log( "muxavi: writing index" );
    AddIndex( m );

    hb_log( "muxavi: closing %s", job->file );
    fclose( m->file );

    hb_buffer_close( &m->index );

    return 0;
}

hb_mux_object_t * hb_mux_avi_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = AVIInit;
    m->mux       = AVIMux;
    m->end       = AVIEnd;
    m->job       = job;
    return m;
}

