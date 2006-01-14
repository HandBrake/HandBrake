/* $Id: Utils.h,v 1.22 2004/01/21 18:40:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_UTILS_H
#define HB_UTILS_H

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
typedef uint8_t byte_t;
#ifdef HB_BEOS
#  include <OS.h>
#endif

/* Handy macros */
#ifndef MIN
#define MIN( a, b ) ( ( (a) > (b) ) ? (b) : (a) )
#endif
#ifndef MAX
#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )
#endif
#ifndef EVEN
#define EVEN( a ) ( ( (a) & 0x1 ) ? ( (a) + 1 ) : (a) )
#endif
#ifndef MULTIPLE_16
#define MULTIPLE_16( a ) ( 16 * ( ( (a) + 8 ) / 16 ) )
#endif
#ifndef VOUT_ASPECT_FACTOR
#define VOUT_ASPECT_FACTOR 432000
#endif

typedef struct HBHandle     HBHandle;

/* Utils */
typedef struct HBAudio      HBAudio;
typedef struct HBBuffer     HBBuffer;
typedef struct HBCond       HBCond;
typedef struct HBFifo       HBFifo;
typedef struct HBList       HBList;
typedef struct HBLock       HBLock;
typedef struct HBTitle      HBTitle;
typedef struct HBThread     HBThread;

/* (De)Muxers */
typedef struct HBAviMux     HBAviMux;
typedef struct HBOgmMux     HBOgmMux;
typedef struct HBDVDRead    HBDVDRead;
typedef struct HBMp4Mux     HBMp4Mux;
typedef struct HBScan       HBScan;

typedef struct HBWork       HBWork;
typedef struct HBWorkThread HBWorkThread;

/* AVI stuff */
typedef struct HBAviMainHeader   HBAviMainHeader;
typedef struct HBAviStreamHeader HBAviStreamHeader;
typedef struct HBBitmapInfo      HBBitmapInfo;
typedef struct HBWaveFormatEx    HBWaveFormatEx;

/* Misc functions which may be used from anywhere */
void     HBSnooze( int time );
void     HBLog( char * log, ... );
uint64_t HBGetDate();
int      HBPStoES( HBBuffer ** psBuffer, HBList * esBufferList );

/* HBList functions */
HBList * HBListInit();
int      HBListCount( HBList * );
void     HBListAdd( HBList *, void * item );
void     HBListRemove( HBList *, void * item );
void   * HBListItemAt( HBList *, int index );
void     HBListClose( HBList ** );

/* HBTitle function */
HBTitle * HBTitleInit();
void      HBTitleClose( HBTitle ** );

/* HBAudio functions */
HBAudio * HBAudioInit( int id, char * language );
void      HBAudioClose( HBAudio ** );

#define HB_SUCCESS          0x00
#define HB_CANCELED         0x01
#define HB_ERROR_A52_SYNC   0x02
#define HB_ERROR_AVI_WRITE  0x04
#define HB_ERROR_DVD_OPEN   0x08
#define HB_ERROR_DVD_READ   0x10
#define HB_ERROR_MP3_INIT   0x20
#define HB_ERROR_MP3_ENCODE 0x40
#define HB_ERROR_MPEG4_INIT 0x80

/* Possible codecs */
#define HB_CODEC_MPEG2  0x00
#define HB_CODEC_FFMPEG 0x01
#define HB_CODEC_XVID   0x02
#define HB_CODEC_AC3    0x04
#define HB_CODEC_MP3    0x08
#define HB_CODEC_AAC    0x10
#define HB_CODEC_X264   0x20
#define HB_CODEC_VORBIS 0x40

/* Possible muxers */
#define HB_MUX_AVI 0x00
#define HB_MUX_MP4 0x01
#define HB_MUX_OGM 0x02

struct HBTitle
{
    /* DVD info */
    char * device;
    int    index;
    int    length;

    /* Audio infos */
    HBList * audioList;
    HBList * ripAudioList;

    /* See DVDRead.c */
    int64_t start;

    /* Video input */
    int inWidth;
    int inHeight;
    int aspect;
    int rate;
    int rateBase;

    /* Video output */
    int outWidth;
    int outHeight;
    int outWidthMax;
    int outHeightMax;
    int topCrop;
    int bottomCrop;
    int leftCrop;
    int rightCrop;
    int deinterlace;
    int autoTopCrop;
    int autoBottomCrop;
    int autoLeftCrop;
    int autoRightCrop;

    /* Encoder settings */
    int codec;
    int bitrate;
    int twoPass;

    /* Muxer settings */
    char * file;
    int    mux;

    /* MP4 muxer specific */
    int       track;
    uint8_t * esConfig;
    int       esConfigLength;

    /* AVI muxer specific */
    HBAviMainHeader   * aviMainHeader;
    HBAviStreamHeader * aviVideoHeader;
    HBBitmapInfo      * aviVideoFormat;

    /* Fifos */
    HBFifo  * inFifo;
    HBFifo  * rawFifo;
    HBFifo  * scaledFifo;
    HBFifo  * outFifo;

    /* Threads */
    HBDVDRead    * dvdRead;
    HBAviMux     * aviMux;
    HBMp4Mux     * mp4Mux;
    HBOgmMux     * ogmMux;
    HBWorkThread * workThreads[8];

    /* Work objects */
    HBWork * decoder;
    HBWork * scale;
    HBWork * encoder;
};

struct HBAudio
{
    /* Ident */
    uint32_t    id;
    char      * language;

    /* Settings */
    int         codec;
    int         inSampleRate;
    int         outSampleRate;
    int         inBitrate;
    int         outBitrate;

    int         delay; /* in ms */

    /* See DVDRead.c */
    int64_t     start;

    /* MPEG-4 config, used in the MP4 muxer */
    uint8_t       * esConfig;
    unsigned long   esConfigLength;

    /* MP4 track id */
    int         track;

    /* AVI stuff */
    uint32_t            aviFourCC;
    HBAviStreamHeader * aviAudioHeader;
    HBWaveFormatEx    * aviAudioFormat;

    /* Fifos */
    HBFifo    * inFifo;
    HBFifo    * rawFifo;
    HBFifo    * outFifo;

    /* Work objects */
    HBWork    * decoder;
    HBWork    * encoder;;
};

#endif
