/* $Id: Utils.h,v 1.6 2003/11/07 21:22:17 titer Exp $

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
#ifdef SYS_BEOS
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

typedef struct HBAc3Dec     HBAc3Dec;
typedef struct HBAudio      HBAudio;
typedef struct HBAviMux     HBAviMux;
typedef struct HBBuffer     HBBuffer;
typedef struct HBDVDRead    HBDVDRead;
typedef struct HBFifo       HBFifo;
typedef struct HBList       HBList;
typedef struct HBLock       HBLock;
typedef struct HBHandle     HBHandle;
typedef struct HBMp3Enc     HBMp3Enc;
typedef struct HBMpeg2Dec   HBMpeg2Dec;
typedef struct HBFfmpegEnc  HBFfmpegEnc;
typedef struct HBMadDec     HBMadDec;
typedef struct HBScale      HBScale;
typedef struct HBScan       HBScan;
typedef struct HBStatus     HBStatus;
typedef struct HBThread     HBThread;
typedef struct HBTitle      HBTitle;
typedef struct HBWork       HBWork;
typedef struct HBWorkThread HBWorkThread;
typedef struct HBXvidEnc    HBXvidEnc;

/* Misc functions which may be used from anywhere */
void     HBSnooze( int time );
void     HBLog( char * log, ... );
uint64_t HBGetDate();
int      HBPStoES( HBBuffer ** psBuffer, HBList * esBufferList );

/* HBList functions */
HBList * HBListInit();
int      HBListCountItems( HBList * );
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

/* Possible states */
typedef enum
{
    HB_MODE_UNDEF          = 00000,
    HB_MODE_NEED_DEVICE    = 00001,
    HB_MODE_SCANNING       = 00002,
    HB_MODE_INVALID_DEVICE = 00004,
    HB_MODE_READY_TO_RIP   = 00010,
    HB_MODE_ENCODING       = 00020,
    HB_MODE_PAUSED         = 00040,
    HB_MODE_STOPPING       = 00100,
    HB_MODE_DONE           = 00200,
    HB_MODE_CANCELED       = 00400,
    HB_MODE_ERROR          = 01000,
    HB_MODE_EXITING        = 02000
} HBMode;

/* Possible errors */
typedef enum
{
    HB_ERROR_A52_SYNC = 0,
    HB_ERROR_AVI_WRITE,
    HB_ERROR_DVD_OPEN,
    HB_ERROR_DVD_READ,
    HB_ERROR_MP3_INIT,
    HB_ERROR_MP3_ENCODE,
    HB_ERROR_MPEG4_INIT
} HBError;

/* Possible codecs */
typedef enum
{
    HB_CODEC_FFMPEG = 0,
    HB_CODEC_XVID
} HBCodec;

struct HBStatus
{
    HBMode   mode;

    /* HB_MODE_SCANNING */
    int      scannedTitle;

    /* HB_MODE_SCANDONE */
    HBList * titleList;

    /* HB_MODE_ENCODING || HB_MODE_PAUSED */
    float    position;
    int      pass;
    int      passCount;
    float    frameRate;
    float    avFrameRate;
    uint32_t remainingTime; /* in seconds */

    /* HB_MODE_ERROR */
    HBError  error;
};

struct HBTitle
{
    char         * device;
    int            index;
    int            length;
    char         * file;

    /* Video input */
    int            inWidth;
    int            inHeight;
    int            aspect;
    int            rate;
    int            rateBase;

    /* Video output */
    int            outWidth;
    int            outHeight;
    int            outWidthMax;
    int            outHeightMax;
    int            topCrop;
    int            bottomCrop;
    int            leftCrop;
    int            rightCrop;
    int            deinterlace;
    HBCodec        codec;
    int            bitrate;
    int            twoPass;

    /* Audio infos */
    HBList       * audioList;

    /* Fifos */
    HBFifo       * mpeg2Fifo;
    HBFifo       * rawFifo;
    HBFifo       * scaledFifo;
    HBFifo       * mpeg4Fifo;

    /* Threads */
    HBDVDRead    * dvdRead;
    HBMpeg2Dec   * mpeg2Dec;
    HBScale      * scale;
    HBFfmpegEnc  * ffmpegEnc;
    HBXvidEnc    * xvidEnc;
    HBAviMux     * aviMux;
    HBWorkThread * workThreads[8];
};

struct HBAudio
{
        /* Ident */
        uint32_t       id;
        char         * language;

        /* Settings */
        int            inSampleRate;
        int            outSampleRate;
        int            inBitrate;
        int            outBitrate;

        int            delay; /* in ms */

        /* Fifos */
        HBFifo       * ac3Fifo;
        HBFifo       * rawFifo;
        HBFifo       * mp3Fifo;

        /* Threads */
        HBAc3Dec * ac3Dec;
        HBMp3Enc     * mp3Enc;
};



#endif
