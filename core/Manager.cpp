/* $Id: Manager.cpp,v 1.47 2003/10/05 14:28:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Manager.h"
#include "Fifo.h"
#include "Scanner.h"
#include "DVDReader.h"
#include "MpegDemux.h"
#include "Mpeg2Decoder.h"
#include "Mpeg4Encoder.h"
#include "Ac3Decoder.h"
#include "Mp3Encoder.h"
#include "AviMuxer.h"
#include "Resizer.h"

#include <ffmpeg/avcodec.h>

/* Public methods */

HBManager::HBManager( bool debug )
    : HBThread( "manager" )
{
    fPid         = 0;

    fStopScan    = false;
    fStopRip     = false;
    fRipDone     = false;
    fError       = false;

    fScanner     = NULL;

    fStatus.fMode = HB_MODE_NEED_VOLUME;
    fNeedUpdate   = true;

    /* See Log() in Common.cpp */
    if( debug )
    {
        setenv( "HB_DEBUG", "1", 1 );
    }
    else
    {
        unsetenv( "HB_DEBUG" );
    }

    fCurTitle    = NULL;
    fCurAudio1   = NULL;
    fCurAudio2   = NULL;

    /* Init ffmpeg's libavcodec */
    avcodec_init();
//    register_avcodec( &mpeg4_encoder );
    avcodec_register_all();

    Run();
}

HBManager::~HBManager()
{
    /* Stop ripping if needed */
    StopRip();
    while( fStopRip )
    {
        Snooze( 10000 );
    }

    Stop();

    /* Stop scanning if needed */
    if( fScanner )
    {
        delete fScanner;
    }

    /* Remove temp files */
    char command[1024]; memset( command, 0, 1024 );
    sprintf( command, "rm -f /tmp/HB.%d.*", fPid );
    system( command );
}

void HBManager::DoWork()
{
    fPid = (int) getpid();

    while( !fDie )
    {
        /* Terminate dying threads */
        if( fStopScan )
        {
            fStopScan = false;

            delete fScanner;
            fScanner = NULL;

            if( fStatus.fTitleList && fStatus.fTitleList->CountItems() )
            {
                fStatus.fMode = HB_MODE_READY_TO_RIP;
            }
            else
            {
                fStatus.fMode = HB_MODE_INVALID_VOLUME;
            }
            fNeedUpdate = true;
        }

        if( fStopRip || fError )
        {
            delete fCurTitle->fDVDReader;
            delete fCurTitle->fMpegDemux;
            delete fCurTitle->fMpeg2Decoder;
            delete fCurTitle->fResizer;
            delete fCurTitle->fMpeg4Encoder;
            delete fCurTitle->fAviMuxer;

            if( fCurAudio1 )
            {
                delete fCurAudio1->fAc3Decoder;
                delete fCurAudio1->fMp3Encoder;
            }

            if( fCurAudio2 )
            {
                delete fCurAudio2->fAc3Decoder;
                delete fCurAudio2->fMp3Encoder;
            }

            delete fCurTitle->fPSFifo;
            delete fCurTitle->fMpeg2Fifo;
            delete fCurTitle->fRawFifo;
            delete fCurTitle->fResizedFifo;
            delete fCurTitle->fMpeg4Fifo;

            if( fCurAudio1 )
            {
                delete fCurAudio1->fAc3Fifo;
                delete fCurAudio1->fRawFifo;
                delete fCurAudio1->fMp3Fifo;
            }

            if( fCurAudio2 )
            {
                delete fCurAudio2->fAc3Fifo;
                delete fCurAudio2->fRawFifo;
                delete fCurAudio2->fMp3Fifo;
            }

            fStatus.fMode = fError ? HB_MODE_ERROR : HB_MODE_CANCELED;
            fStopRip = false;
            fError = false;
            fNeedUpdate = true;
        }

        if( fRipDone )
        {
            /* This is UGLY ! */
            while( fCurTitle->fPSFifo->Size() ||
                   fCurTitle->fMpeg2Fifo->Size() ||
                   fCurTitle->fRawFifo->Size() ||
                   fCurTitle->fResizedFifo->Size() ||
                   ( fCurAudio1 && fCurAudio1->fAc3Fifo->Size() ) ||
                   ( fCurAudio1 && fCurAudio1->fRawFifo->Size() ) ||
                   ( fCurAudio2 && fCurAudio2->fAc3Fifo->Size() ) ||
                   ( fCurAudio2 && fCurAudio2->fRawFifo->Size() ) )
            {
                Snooze( 10000 );
            }

            while( fCurTitle->fMpeg4Fifo->Size() &&
                   ( !fCurAudio1 || fCurAudio1->fMp3Fifo->Size() ) &&
                   ( !fCurAudio2 || fCurAudio2->fMp3Fifo->Size() ) )
            {
                Snooze( 10000 );
            }

            delete fCurTitle->fDVDReader;
            delete fCurTitle->fMpegDemux;
            delete fCurTitle->fMpeg2Decoder;
            delete fCurTitle->fResizer;
            delete fCurTitle->fMpeg4Encoder;
            delete fCurTitle->fAviMuxer;

            if( fCurAudio1 )
            {
                delete fCurAudio1->fAc3Decoder;
                delete fCurAudio1->fMp3Encoder;
            }

            if( fCurAudio2 )
            {
                delete fCurAudio2->fAc3Decoder;
                delete fCurAudio2->fMp3Encoder;
            }

            delete fCurTitle->fPSFifo;
            delete fCurTitle->fMpeg2Fifo;
            delete fCurTitle->fRawFifo;
            delete fCurTitle->fResizedFifo;
            delete fCurTitle->fMpeg4Fifo;

            if( fCurAudio1 )
            {
                delete fCurAudio1->fAc3Fifo;
                delete fCurAudio1->fRawFifo;
                delete fCurAudio1->fMp3Fifo;
            }

            if( fCurAudio2 )
            {
                delete fCurAudio2->fAc3Fifo;
                delete fCurAudio2->fRawFifo;
                delete fCurAudio2->fMp3Fifo;
            }

            fStatus.fMode = HB_MODE_DONE;
            fRipDone = false;
            fNeedUpdate = true;
        }

        Snooze( 10000 );
    }
}

bool HBManager::NeedUpdate()
{
    if( fNeedUpdate )
    {
        fNeedUpdate = false;
        return true;
    }

    return false;
}

HBStatus HBManager::GetStatus()
{
    return fStatus;
}

int HBManager::GetPid()
{
    return fPid;
}

void HBManager::ScanVolumes( char * device )
{
    if( !( fStatus.fMode &
           ( HB_MODE_NEED_VOLUME | HB_MODE_INVALID_VOLUME ) ) )
    {
        Log( "HBManager::ScanVolumes : current mode is %d, aborting",
             fStatus.fMode  );
        return;
    }
    
    fScanner = new HBScanner( this, device );
    fScanner->Run();

    fStatus.fMode          = HB_MODE_SCANNING;
    fStatus.fScannedVolume = strdup( device );
    fStatus.fScannedTitle  = 0;
    fNeedUpdate            = true;
}

void HBManager::StartRip( HBTitle * title, HBAudio * audio1,
                          HBAudio * audio2, char * file )
{
    if( !title || !file )
    {
        Log( "HBManager::StartRip : error (title = %p, file = %s)",
             title, file );
        return;
    }

    if( !( fStatus.fMode & ( HB_MODE_READY_TO_RIP | HB_MODE_DONE |
                             HB_MODE_CANCELED | HB_MODE_ERROR ) ) )
    {
        Log( "HBManager::StartRip : current mode is %d, aborting",
             fStatus.fMode  );
        return;
    }

    FixPictureSettings( title );

    Log( "HBManager::StartRip : device: %s, title: %d",
         title->fDevice, title->fIndex );
    Log( "    - video : %dx%d -> %dx%d, bitrate = %d, 2-pass = %s",
         title->fInWidth, title->fInHeight,
         title->fOutWidth, title->fOutHeight,
         title->fBitrate, title->fTwoPass ? "yes" : "no" );
    Log( "    - cropping: top=%d, bottom=%d, left=%d, right=%d",
         title->fTopCrop, title->fBottomCrop,
         title->fLeftCrop, title->fRightCrop );
    if( audio1 )
    {
        Log( "    - audio 1: lang = %s (%x), bitrate = %d",
             audio1->fDescription, audio1->fId, audio1->fOutBitrate );
    }
    if( audio2 )
    {
        Log( "    - audio 2: lang = %s (%x), bitrate = %d",
             audio2->fDescription, audio1->fId, audio2->fOutBitrate );
    }

    /* Create fifos & threads */

    title->fPSFifo       = new HBFifo();
    title->fMpeg2Fifo    = new HBFifo();
    title->fRawFifo      = new HBFifo();
    title->fResizedFifo  = new HBFifo();
    title->fMpeg4Fifo    = new HBFifo();

    title->fDVDReader    = new HBDVDReader( this, title );
    title->fMpegDemux    = new HBMpegDemux( this, title, audio1,
                                            audio2 );
    title->fMpeg2Decoder = new HBMpeg2Decoder( this, title );
    title->fResizer      = new HBResizer( this, title );
    title->fMpeg4Encoder = new HBMpeg4Encoder( this, title );
    title->fAviMuxer     = new HBAviMuxer( this, title, audio1, audio2,
                                           file );

    if( audio1 )
    {
        audio1->fAc3Fifo    = new HBFifo();
        audio1->fRawFifo    = new HBFifo();
        audio1->fMp3Fifo    = new HBFifo();
        audio1->fAc3Decoder = new HBAc3Decoder( this, audio1 );
        audio1->fMp3Encoder = new HBMp3Encoder( this, audio1 );
    }

    if( audio2 )
    {
        audio2->fAc3Fifo    = new HBFifo();
        audio2->fRawFifo    = new HBFifo();
        audio2->fMp3Fifo    = new HBFifo();
        audio2->fAc3Decoder = new HBAc3Decoder( this, audio2 );
        audio2->fMp3Encoder = new HBMp3Encoder( this, audio2 );
    }

    /* Launch the threads */

    title->fDVDReader->Run();
    title->fMpegDemux->Run();
    title->fMpeg2Decoder->Run();
    title->fResizer->Run();
    title->fMpeg4Encoder->Run();
    title->fAviMuxer->Run();

    if( audio1 )
    {
        audio1->fAc3Decoder->Run();
        audio1->fMp3Encoder->Run();
    }

    if( audio2 )
    {
        audio2->fAc3Decoder->Run();
        audio2->fMp3Encoder->Run();
    }

    fCurTitle  = title;
    fCurAudio1 = audio1;
    fCurAudio2 = audio2;

    fStatus.fMode          = HB_MODE_ENCODING;
    fStatus.fPosition      = 0;
    fStatus.fFrameRate     = 0;
    fStatus.fFrames        = 0;
    fStatus.fStartDate     = 0;
    fStatus.fRemainingTime = 0;
    fStatus.fSuspendDate   = 0;
    fNeedUpdate = true;
}

void HBManager::SuspendRip()
{
    if( fStatus.fMode != HB_MODE_ENCODING )
    {
        Log( "HBManager::SuspendRip : current mode is %d, aborting",
             fStatus.fMode );
        return;
    }

    fCurTitle->fDVDReader->Suspend();
    fCurTitle->fMpegDemux->Suspend();
    fCurTitle->fMpeg2Decoder->Suspend();
    fCurTitle->fResizer->Suspend();
    fCurTitle->fMpeg4Encoder->Suspend();
    fCurTitle->fAviMuxer->Suspend();

    if( fCurAudio1 )
    {
        fCurAudio1->fAc3Decoder->Suspend();
        fCurAudio1->fMp3Encoder->Suspend();
    }

    if( fCurAudio2 )
    {
        fCurAudio2->fAc3Decoder->Suspend();
        fCurAudio2->fMp3Encoder->Suspend();
    }

    fStatus.fMode        = HB_MODE_SUSPENDED;
    fStatus.fSuspendDate = GetDate();
    fNeedUpdate = true;
}

void HBManager::ResumeRip()
{
    if( fStatus.fMode != HB_MODE_SUSPENDED )
    {
        Log( "HBManager::ResumeRip : current mode is %d, aborting",
             fStatus.fMode );
        return;
    }

    fCurTitle->fDVDReader->Resume();
    fCurTitle->fMpegDemux->Resume();
    fCurTitle->fMpeg2Decoder->Resume();
    fCurTitle->fResizer->Resume();
    fCurTitle->fMpeg4Encoder->Resume();
    fCurTitle->fAviMuxer->Resume();

    if( fCurAudio1 )
    {
        fCurAudio1->fAc3Decoder->Resume();
        fCurAudio1->fMp3Encoder->Resume();
    }

    if( fCurAudio2 )
    {
        fCurAudio2->fAc3Decoder->Resume();
        fCurAudio2->fMp3Encoder->Resume();
    }

    fStatus.fMode       = HB_MODE_ENCODING;
    fStatus.fStartDate += GetDate() - fStatus.fSuspendDate;
    fNeedUpdate = true;
}

void HBManager::StopRip()
{
    if( !( fStatus.fMode & ( HB_MODE_ENCODING | HB_MODE_SUSPENDED ) ) )
    {
        Log( "HBManager::StopRip : current mode is %d, aborting",
             fStatus.fMode );
        return;
    }

    /* Stop the threads */

    fCurTitle->fDVDReader->Stop();
    fCurTitle->fMpegDemux->Stop();
    fCurTitle->fMpeg2Decoder->Stop();
    fCurTitle->fMpeg4Encoder->Stop();
    fCurTitle->fAviMuxer->Stop();

    if( fCurAudio1 )
    {
        fCurAudio1->fAc3Decoder->Stop();
        fCurAudio1->fMp3Encoder->Stop();
    }

    if( fCurAudio2 )
    {
        fCurAudio2->fAc3Decoder->Stop();
        fCurAudio2->fMp3Encoder->Stop();
    }

    fStopRip = true;
}

#define fInWidth      title->fInWidth
#define fInHeight     title->fInHeight
#define fAspect       title->fAspect
#define fDeinterlace  title->fDeinterlace
#define fOutWidth     title->fOutWidth
#define fOutHeight    title->fOutHeight
#define fOutWidthMax  title->fOutWidthMax
#define fOutHeightMax title->fOutHeightMax
#define fTopCrop      title->fTopCrop
#define fBottomCrop   title->fBottomCrop
#define fLeftCrop     title->fLeftCrop
#define fRightCrop    title->fRightCrop

void HBManager::FixPictureSettings( HBTitle * title )
{
    /* Sanity checks */
    fTopCrop    = EVEN( fTopCrop );
    fBottomCrop = EVEN( fBottomCrop );
    fLeftCrop   = EVEN( fLeftCrop );
    fRightCrop  = EVEN( fRightCrop );

    fOutWidth   = MIN( fOutWidth, fOutWidthMax );
    fOutWidth   = MAX( 16, fOutWidth );

    fOutHeight  = MULTIPLE_16( (uint64_t) fOutWidth * fInWidth *
                               ( fInHeight - fTopCrop - fBottomCrop ) *
                               VOUT_ASPECT_FACTOR /
                               ( (uint64_t) fInHeight *
                                 ( fInWidth - fLeftCrop - fRightCrop ) *
                                 fAspect ) );
    fOutHeight  = MAX( 16, fOutHeight );

    if( fOutHeight > fOutHeightMax )
    {
        fOutHeight = fOutHeightMax;
        fOutWidth  = MULTIPLE_16( (uint64_t) fOutHeight * fInHeight *
                                  ( fInWidth - fLeftCrop - fRightCrop ) *
                                  fAspect /
                                  ( (uint64_t) fInWidth *
                                    ( fInHeight - fTopCrop - fBottomCrop ) *
                                    VOUT_ASPECT_FACTOR ) );
        fOutWidth  = MIN( fOutWidth, fOutWidthMax );
        fOutWidth  = MAX( 16, fOutWidth );
    }
}

uint8_t * HBManager::GetPreview( HBTitle * title, uint32_t image )
{
    FixPictureSettings( title );

    AVPicture pic1, pic2, pic3, pic4;
    uint8_t * buf1, * buf2, * buf3, * buf4;

    /* Original YUV picture */
    buf1 = (uint8_t*) malloc( 3 * fInWidth * fInHeight / 2 );
    avpicture_fill( &pic1, buf1, PIX_FMT_YUV420P, fInWidth,
                    fInHeight );

    /* Deinterlaced YUV picture */
    buf2 = (uint8_t*) malloc( 3 * fInWidth * fInHeight / 2 );
    avpicture_fill( &pic2, buf2, PIX_FMT_YUV420P,
                    fInWidth, fInHeight );

    /* Resized YUV picture */
    buf3 = (uint8_t*) malloc( 3 * fOutWidth * fOutHeight / 2 );
    avpicture_fill( &pic3, buf3, PIX_FMT_YUV420P, fOutWidth,
                    fOutHeight );

    /* Resized RGB picture ) */
    buf4 = (uint8_t*) malloc( 4 * fOutWidth * fOutHeight );
    avpicture_fill( &pic4, buf4, PIX_FMT_RGBA32, fOutWidth,
                    fOutHeight );

    /* Get the original image from the temp file */
    char fileName[1024]; memset( fileName, 0, 1024 );
    sprintf( fileName, "/tmp/HB.%d.%x.%d", fPid, (uint32_t) title,
             image);
    FILE * file = fopen( fileName, "r" );
    fread( buf1, 3 * fInWidth * fInHeight / 2, 1, file );
    fclose( file );

    /* Deinterlace if needed, and resize */
    ImgReSampleContext * resampleContext =
        img_resample_full_init( fOutWidth, fOutHeight,
                                fInWidth, fInHeight,
                                fTopCrop, fBottomCrop,
                                fLeftCrop, fRightCrop );
    if( fDeinterlace )
    {
        avpicture_deinterlace( &pic2, &pic1, PIX_FMT_YUV420P,
                               fInWidth, fInHeight );
        img_resample( resampleContext, &pic3, &pic2 );
    }
    else
    {
        img_resample( resampleContext, &pic3, &pic1 );
    }

    /* Convert to RGB */
    img_convert( &pic4, PIX_FMT_RGBA32, &pic3, PIX_FMT_YUV420P,
                 fOutWidth, fOutHeight );

    /* Create the final preview */
    uint8_t * preview = (uint8_t*) malloc( 4 * ( fOutWidthMax + 2 ) *
                                           ( fOutHeightMax + 2 ) );

    /* Blank it */
    memset( preview, 0,
            4 * ( fOutWidthMax + 2 ) * ( fOutHeightMax + 2 ) );

    /* Draw the picture (centered) and draw the cropping zone */
    uint32_t leftOffset = 1 + ( fOutWidthMax - fOutWidth ) / 2;
    uint32_t topOffset  = 1 + ( fOutHeightMax - fOutHeight ) / 2;
    
    memset( preview + 4 * ( ( fOutWidthMax + 2 ) * ( topOffset - 1 ) +
                            leftOffset - 1 ),
            0xFF, 4 * ( fOutWidth + 2 ) );
    
    for( uint32_t i = 0; i < fOutHeight; i++ )
    {
        memset( preview + 4 * ( ( fOutWidthMax + 2 ) *
                                ( i + topOffset ) + leftOffset - 1 ),
                0xFF, 4 );
        memcpy( preview + 4 * ( ( fOutWidthMax + 2 ) *
                                ( i + topOffset ) + leftOffset ),
                buf4 + 4 * fOutWidth * i,
                4 * fOutWidth );
        memset( preview + 4 * ( ( fOutWidthMax + 2 ) *
                                ( i + topOffset ) + leftOffset +
                                fOutWidth ),
                0xFF, 4 );
    }
    
    memset( preview + 4 * ( ( fOutWidthMax + 2 ) *
                            ( topOffset + fOutHeight ) +
                            leftOffset - 1 ),
            0xFF, 4 * ( fOutWidth + 2 ) );

    /* Free memory */
    free( buf1 );
    free( buf2 );
    free( buf3 );
    free( buf4 );

    return preview;
}

#undef fInWidth
#undef fInHeight
#undef fAspect
#undef fDeinterlace
#undef fOutWidth
#undef fOutHeight
#undef fOutWidthMax
#undef fOutHeightMax
#undef fTopCrop
#undef fBottomCrop
#undef fLeftCrop
#undef fRightCrop

void HBManager::Scanning( char * volume, int title )
{
    fStatus.fMode       = HB_MODE_SCANNING;
    fStatus.fScannedVolume = volume;
    fStatus.fScannedTitle = title;
    fNeedUpdate = true;
}

void HBManager::ScanDone( HBList * titleList )
{
    fStatus.fTitleList = titleList;;
    fStopScan = true;
}

/* Called by the DVD reader */
void HBManager::Done()
{
    fRipDone = true;
}

void HBManager::Error()
{
    if( fStatus.fMode != HB_MODE_ENCODING )
    {
        return;
    }

    /* Stop the threads */

    fCurTitle->fDVDReader->Stop();
    fCurTitle->fMpegDemux->Stop();
    fCurTitle->fMpeg2Decoder->Stop();
    fCurTitle->fResizer->Stop();
    fCurTitle->fMpeg4Encoder->Stop();
    fCurTitle->fAviMuxer->Stop();

    if( fCurAudio1 )
    {
        fCurAudio1->fAc3Decoder->Stop();
        fCurAudio1->fMp3Encoder->Stop();
    }

    if( fCurAudio2 )
    {
        fCurAudio2->fAc3Decoder->Stop();
        fCurAudio2->fMp3Encoder->Stop();
    }

    fError = true;
}

void HBManager::SetPosition( float pos )
{
    if( !fStatus.fStartDate )
    {
        fStatus.fStartDate = GetDate();
    }

    fStatus.fFrames++;

    if( ( pos - fStatus.fPosition ) * 10000 < 1 )
    {
        return;
    }

    fStatus.fPosition = pos;
    fStatus.fFrameRate = (float) fStatus.fFrames /
        ( ( (float) ( GetDate() - fStatus.fStartDate ) ) / 1000000 ) ;
    fStatus.fRemainingTime =
        (uint32_t) ( (float) ( GetDate() - fStatus.fStartDate ) *
                             ( 1 - fStatus.fPosition ) /
                             ( 1000000 * fStatus.fPosition ) );
    fNeedUpdate = true;
}

