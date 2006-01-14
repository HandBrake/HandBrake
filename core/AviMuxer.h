/* $Id: AviMuxer.h,v 1.10 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_AVI_MUXER_H
#define HB_AVI_MUXER_H

#include "Common.h"
#include "Fifo.h"
#include "Thread.h"

/* Misc structures used in AVI headers */
#define BITMAP_INFO_SIZE 52
typedef struct BitmapInfo
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

#define WAVE_FORMAT_EX_SIZE 38
typedef struct WaveFormatEx
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

#define AVI_STREAM_HEADER_SIZE 64
typedef struct AviStreamHeader
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

#define AVI_MAIN_HEADER_SIZE 64
typedef struct AviMainHeader
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

class HBAviIndex : public HBBuffer
{
    public:
             HBAviIndex( uint32_t size );
        void WriteInt32( uint32_t val );
};

class HBAviMuxer : public HBThread
{
    public:
                         HBAviMuxer( HBManager * manager,
                                     HBTitle * title, HBAudio * audio1,
                                     HBAudio * audio2,
                                     char * fileName );

    private:
        void             DoWork();
        bool             AddVideoChunk();
        bool             AddAudioChunk( int track );
        void             UpdateMainHeader();

        HBManager      * fManager;
        HBTitle        * fTitle;
        HBAudio        * fAudio1;
        HBAudio        * fAudio2;
        char           * fFileName;

        FILE           * fFile;
        HBBuffer       * fVideoBuffer;
        HBBuffer       * fAudio1Buffer;
        HBBuffer       * fAudio2Buffer;

        /* The main header */
        AviMainHeader    fMainHeader;

        /* The video track */
        AviStreamHeader  fVideoStreamHeader;
        BitmapInfo       fVideoStreamFormat;

        /* The audio tracks */
        AviStreamHeader  fAudio1StreamHeader;
        WaveFormatEx     fAudio1StreamFormat;
        AviStreamHeader  fAudio2StreamHeader;
        WaveFormatEx     fAudio2StreamFormat;

        uint32_t         fRiffBytesCount;
        uint32_t         fHdrlBytesCount;
        uint32_t         fMoviBytesCount;

        HBAviIndex     * fIndex;

};

#endif
