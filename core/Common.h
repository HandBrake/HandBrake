/* $Id: Common.h,v 1.28 2003/10/03 16:18:08 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_COMMON_H
#define HB_COMMON_H

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
typedef uint8_t byte_t;

/* Misc structures */
typedef struct dvdplay_s * dvdplay_ptr;
typedef struct mpeg2dec_s mpeg2dec_t;
typedef struct AVPicture AVPicture;
typedef struct AVFrame AVFrame;
typedef struct AVCodecContext AVCodecContext;
typedef struct ImgReSampleContext ImgReSampleContext;

/* Classes */
class HBAc3Decoder;
class HBAudio;
class HBAviIndex;
class HBAviMuxer;
class HBBuffer;
class HBDVDReader;
class HBFifo;
class HBList;
class HBLock;
class HBManager;
class HBMp3Encoder;
class HBMpeg2Decoder;
class HBMpeg4Encoder;
class HBMpegDemux;
class HBResizer;
class HBScanner;
class HBStatus;
class HBThread;
class HBTitle;

/* Handy macros */
#ifndef MIN
#  define MIN( a, b )    ( ( (a) > (b) ) ? (b) : (a) )
#endif
#ifndef MAX
#  define MAX( a, b )    ( ( (a) > (b) ) ? (a) : (b) )
#endif
#define EVEN( a )        ( ( (a) & 0x1 ) ? ( (a) + 1 ) : (a) )
#define MULTIPLE_16( a ) ( 16 * ( ( (a) + 8 ) / 16 ) )

#define VOUT_ASPECT_FACTOR 432000

/* Global prototypes */
void     Snooze( uint64_t time );
void     Log( char * log, ... );
char   * LanguageForCode( int code );
uint64_t GetDate();

/* Possible states */
typedef enum
{
    HB_MODE_UNDEF          = 0000,
    HB_MODE_NEED_VOLUME    = 0001,
    HB_MODE_SCANNING       = 0002,
    HB_MODE_INVALID_VOLUME = 0004,
    HB_MODE_READY_TO_RIP   = 0010,
    HB_MODE_ENCODING       = 0020,
    HB_MODE_SUSPENDED      = 0040,
    HB_MODE_DONE           = 0100,
    HB_MODE_CANCELED       = 0200,
    HB_MODE_ERROR          = 0400
} HBMode;

class HBStatus
{
    public:
        HBMode   fMode;

        /* HB_MODE_SCANNING */
        char   * fScannedVolume;
        int      fScannedTitle;

        /* HB_MODE_SCANDONE */
        HBList * fTitleList;

        /* HB_MODE_ENCODING || HB_MODE_SUSPENDED */
        float    fPosition;
        float    fFrameRate;
        uint32_t fFrames;
        uint64_t fStartDate;
        uint32_t fRemainingTime; /* in seconds */
        uint64_t fSuspendDate;
};

class HBList
{
    public:
        HBList();
        ~HBList();
        uint32_t    CountItems();
        void        AddItem( void * item );
        void        RemoveItem( void * item );
        void      * ItemAt( uint32_t index );

    private:
        void     ** fItems;
        uint32_t    fAllocItems;
        uint32_t    fNbItems;
};

class HBAudio
{
    public:
        HBAudio( int id, char * description );
        ~HBAudio();

        /* Ident */
        uint32_t       fId;
        char         * fDescription;

        /* Settings */
        int            fInSampleRate;
        int            fOutSampleRate;
        int            fInBitrate;
        int            fOutBitrate;

        /* Fifos */
        HBFifo       * fAc3Fifo;
        HBFifo       * fRawFifo;
        HBFifo       * fMp3Fifo;

        /* Threads */
        HBAc3Decoder * fAc3Decoder;
        HBMp3Encoder * fMp3Encoder;
};

class HBTitle
{
    public:
                         HBTitle( char * device, int index );
                         ~HBTitle();

        char           * fDevice;
        int              fIndex;
        uint64_t         fLength;

        /* Video input */
        uint32_t         fInWidth;
        uint32_t         fInHeight;
        uint32_t         fAspect;
        uint32_t         fRate;
        uint32_t         fScale;

        /* Video output */
        bool             fDeinterlace;
        uint32_t         fOutWidth;
        uint32_t         fOutHeight;
        uint32_t         fOutWidthMax;
        uint32_t         fOutHeightMax;
        uint32_t         fTopCrop;
        uint32_t         fBottomCrop;
        uint32_t         fLeftCrop;
        uint32_t         fRightCrop;
        uint32_t         fBitrate;
        bool             fTwoPass;

        /* Audio infos */
        HBList         * fAudioList;

        /* Fifos */
        HBFifo         * fPSFifo;
        HBFifo         * fMpeg2Fifo;
        HBFifo         * fRawFifo;
        HBFifo         * fResizedFifo;
        HBFifo         * fMpeg4Fifo;

        /* Threads */
        HBDVDReader    * fDVDReader;
        HBMpegDemux    * fMpegDemux;
        HBMpeg2Decoder * fMpeg2Decoder;
        HBResizer      * fResizer;
        HBMpeg4Encoder * fMpeg4Encoder;
        HBAviMuxer     * fAviMuxer;
};

#endif
