/* $Id: HBAviMuxer.h,v 1.8 2003/08/23 16:20:59 titer Exp $ */

#ifndef HB_AVI_MUXER_H
#define HB_AVI_MUXER_H

#include "HBThread.h"
class HBManager;
class HBFifo;
class HBBuffer;
class HBAudioInfo;
class HBTitleInfo;

#define FOURCC(a) \
    ( ( a[3] << 24 ) | ( a[2] << 16 ) | ( a[1] << 8 ) | a[0] )

/* Misc structures used in AVI headers */
typedef struct __attribute__((__packed__)) BitmapInfo
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
} BitmapInfo;

typedef struct __attribute__((__packed__)) WaveFormatEx
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
} WaveFormatEx;

typedef struct __attribute__((__packed__)) AviStreamHeader
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
} AviStreamHeader;

typedef struct __attribute__((__packed__)) AviMainHeader
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
} AviMainHeader;

typedef struct AviOldIndexEntry
{
    char     StreamNb[2];
    char     Code[2];
    uint32_t Flags;
    uint32_t Offset;
    uint32_t Size;
} AviOldIndexEntry;

class HBAviMuxer : public HBThread
{
    public:
        HBAviMuxer( HBManager * manager, HBTitleInfo * titleInfo,
                    HBAudioInfo * audio1Info, HBAudioInfo * audio2Info,
                    char * fileName );
    
    private:
        void          DoWork();
        bool          AddVideoChunk();
        bool          AddAudioChunk( int which );
        void          UpdateMainHeader();
    
        HBManager   * fManager;
        HBTitleInfo * fTitleInfo;
        HBAudioInfo * fAudio1Info;
        HBAudioInfo * fAudio2Info;
        char        * fFileName;
        
        FILE        * fFile;

        /* The main header */
        AviMainHeader     fMainHeader;

        /* The video track */
        AviStreamHeader   fVideoStreamHeader;
        BitmapInfo        fVideoStreamFormat;

        /* The audio tracks */
        AviStreamHeader   fAudio1StreamHeader;
        WaveFormatEx      fAudio1StreamFormat;
        AviStreamHeader   fAudio2StreamHeader;
        WaveFormatEx      fAudio2StreamFormat;

        uint32_t          fRiffBytesCount;
        uint32_t          fHdrlBytesCount;
        uint32_t          fMoviBytesCount;

        HBBuffer        * fIndex;

};

#endif
