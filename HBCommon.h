/* $Id: HBCommon.h,v 1.9 2003/08/24 19:28:18 titer Exp $ */

#ifndef _HB_COMMON_H
#define _HB_COMMON_H

/* standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
typedef uint8_t byte_t; 

/* BeOS headers */
#include <Looper.h>
#include <MenuItem.h>

/* Internal headers */
class HBFifo;
class HBPictureWin;

/* Misc structures */
typedef struct dvdplay_s * dvdplay_ptr;
typedef struct iso639_lang_t
{
    char * engName;        /* Description in English */
    char * nativeName;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
} iso639_lang_t;

/* BMessages */
#define MANAGER_CREATED    'macr'
#define PRINT_MESSAGE      'prme'
#define DETECT_VOLUMES     'devo'
#define START_CONVERT      'stac'
#define STOP_CONVERT       'stoc'
#define SUSPEND_CONVERT    'suco'
#define RESUME_CONVERT     'reco'
#define VOLUMES_DETECTED   'vode'
#define REFRESH_VOLUMES    'revo'
#define VIDEO_SLIDER       'visl'
#define AUDIO_SLIDER       'ausl'
#define PICTURE_WIN        'piwi'
#define NOT_IMPLEMENTED    'noim'
#define VOLUME_SELECTED   'vose'
#define TITLE_SELECTED    'tise'
#define LANGUAGE_SELECTED 'lase'
#define CHANGE_STATUS     'chst'

/* Handy macros */
#define EVEN( a )        ( ( (a) & 0x1 ) ? ( (a) + 1 ) : (a) )
#define MULTIPLE_16( a ) ( ( ( (a) % 16 ) < 8 ) ? ( (a) - ( (a) % 16 ) ) \
                           : ( (a) - ( (a) % 16 ) + 16 ) )

/* Global prototypes */
void   Log( char * log, ... );
void   Status( char * text, float pos, int mode );
char * LanguageForCode( int code );

/* Possible modes in Status() */
#define ENABLE_DETECTING 0x1
#define ENABLE_READY     0x2
#define ENABLE_ENCODING  0x4

/* Classes */

class HBAudioInfo : public BMenuItem
{
    public:
        /* Common methods and members */
        HBAudioInfo( int id, char * description );
        HBAudioInfo( HBAudioInfo * audioInfo );
        ~HBAudioInfo();
        
        uint32_t fId;
        HBFifo * fAc3Fifo;
        HBFifo * fRawFifo;
        HBFifo * fMp3Fifo;

        int      fInSampleRate;
        int      fOutSampleRate;
        int      fInBitrate;
        int      fOutBitrate;
};

class HBTitleInfo : public BMenuItem
{
    public:
        HBTitleInfo( dvdplay_ptr vmg, int index, char * device );
        ~HBTitleInfo();
        bool InitCheck();
    
        bool           fInitOK;
        char         * fDevice;
        int            fIndex;
        uint64_t       fLength;
        
        /* MPEG2-PS data */
        HBFifo       * fPSFifo;
       
        /* Video info */
        bool           DecodeFrame( dvdplay_ptr vmg, int i );

        HBFifo       * fMpeg2Fifo;
        HBFifo       * fRawFifo;
        HBFifo       * fMpeg4Fifo;
        
        /* Video input */
        uint32_t       fInWidth;
        uint32_t       fInHeight;
        uint32_t       fPixelWidth;
        uint32_t       fPixelHeight;
        uint32_t       fRate;
        uint32_t       fScale;
        
        /* Video output */
        bool           fDeinterlace;
        uint32_t       fOutWidth;
        uint32_t       fOutHeight;
        uint32_t       fTopCrop;
        uint32_t       fBottomCrop;
        uint32_t       fLeftCrop;
        uint32_t       fRightCrop;
        uint32_t       fBitrate;
        
        uint8_t      * fPictures[10];
        HBPictureWin * fPictureWin;
        
        /* Audio infos */
        BList        * fAudioInfoList1;
        BList        * fAudioInfoList2;
};

class HBVolumeInfo : public BMenuItem
{
    public:
        HBVolumeInfo( char * name, char * device );
        ~HBVolumeInfo();
        bool InitCheck();
    
        bool    fInitOK;
        char  * fName;
        char  * fDevice;
        BList * fTitleList;
};

#endif
