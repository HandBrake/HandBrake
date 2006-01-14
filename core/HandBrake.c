/* $Id: HandBrake.c,v 1.18 2003/11/13 01:17:33 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HandBrakeInternal.h"

#include <ffmpeg/avcodec.h>

#include "Ac3Dec.h"
#include "AviMux.h"
#include "DVDRead.h"
#include "FfmpegEnc.h"
#include "Fifo.h"
#include "Mp3Enc.h"
#include "Mpeg2Dec.h"
#include "Scale.h"
#include "Scan.h"
#include "Thread.h"
#include "Work.h"
#include "XvidEnc.h"

/* Local prototypes */
static void HandBrakeThread( void * );
static void _StopRip( HBHandle * );
static void FixPictureSettings( HBTitle * );
static int  GetCPUCount();

struct HBHandle
{
    HBThread * thread;
    int        die;
    int        pid;

    int        cpuCount;

    int        stopScan;
    int        stopRip;
    int        ripDone;
    int        error;

    HBScan   * scan;

    HBLock   * lock;
    HBStatus   status;
    int        modeChanged;
    HBTitle  * curTitle;
    HBAudio  * curAudio;
    HBAudio  * curOptAudio;

    int        frames;
    uint64_t   beginDate;
    int        framesSinceFpsUpdate;
    uint64_t   lastFpsUpdate;
    uint64_t   pauseDate;

    HBLock   * pauseLock;
};

HBHandle * HBInit( int debug, int cpuCount )
{
    HBHandle * h;
    if( !( h = malloc( sizeof( HBHandle ) ) ) )
    {
        HBLog( "HBInit: malloc() failed, gonna crash" );
        return NULL;
    }

    /* See HBLog() in Utils.cpp */
    if( debug )
    {
        putenv( "HB_DEBUG=1" );
    }

    /* Init libavcodec */
    avcodec_init();
    register_avcodec( &mpeg4_encoder );

    /* Check CPU count */
    if( !cpuCount )
    {
        h->cpuCount = GetCPUCount();
        HBLog( "HBInit: %d CPU%s detected", h->cpuCount,
             ( h->cpuCount > 1 ) ? "s" : "" );
    }
    else
    {
        if( cpuCount < 1 )
        {
            HBLog( "HBInit: invalid CPU count (%d), using 1",
                 cpuCount );
            h->cpuCount = 1;
        }
        else if( cpuCount > 8 )
        {
            HBLog( "HBInit: invalid CPU count (%d), using 8",
                 cpuCount );
            h->cpuCount = 8;
        }
        else
        {
            HBLog( "HBInit: user specified %d CPU%s",
                 cpuCount, ( cpuCount > 1 ) ? "s" : "" );
            h->cpuCount = cpuCount;
        }
    }
    
    /* Initializations */
    h->stopScan = 0;
    h->stopRip  = 0;
    h->ripDone  = 0;
    h->error    = 0;

    h->scan = NULL;

    h->lock        = HBLockInit();
    h->modeChanged = 1;
    h->status.mode = HB_MODE_NEED_DEVICE;
    h->status.titleList = NULL;
    h->curTitle    = NULL;
    h->curAudio    = NULL;
    h->curOptAudio = NULL;

    h->pauseLock = HBLockInit();

    h->die    = 0;
    h->thread = HBThreadInit( "libhb", HandBrakeThread, h,
                              HB_NORMAL_PRIORITY );

    return h;
}

int HBGetStatus( HBHandle * h, HBStatus * status )
{
    HBLockLock( h->lock );
    memcpy( status, &h->status, sizeof( HBStatus ) );

    if( !h->modeChanged )
    {
        HBLockUnlock( h->lock );
        return 0;
    }

    h->modeChanged = 0;
    HBLockUnlock( h->lock );
    return 1;
}

void HBScanDevice( HBHandle * h, char * device, int title )
{
    if( !( h->status.mode & ( HB_MODE_NEED_DEVICE |
                              HB_MODE_INVALID_DEVICE ) ) )
    {
        HBLog( "HBScanDevice: current mode is %d, aborting",
               h->status.mode );
        return;
    }
    
    HBLockLock( h->lock );
    h->modeChanged         = 1;
    h->status.mode         = HB_MODE_SCANNING;
    h->status.scannedTitle = 0;
    HBLockUnlock( h->lock );

    h->scan = HBScanInit( h, device, title );
}

void HBStartRip( HBHandle * h, HBTitle * t,
                 HBAudio * a1, HBAudio * a2 )
{
    int i;

    if( !( h->status.mode & ( HB_MODE_READY_TO_RIP | HB_MODE_DONE |
                              HB_MODE_CANCELED | HB_MODE_ERROR ) ) )
    {
        HBLog( "HBStartRip: current mode is %d, aborting",
               h->status.mode );
        return;
    }

    HBLockLock( h->lock );
    h->modeChanged          = 1;
    h->status.mode          = HB_MODE_ENCODING;
    h->status.position      = 0.0;
    h->status.pass          = 1;
    h->status.passCount     = t->twoPass ? 2 : 1;
    h->frames               = 0;
    h->framesSinceFpsUpdate = 0;
    HBLockUnlock( h->lock );
    
    FixPictureSettings( t );

    /* Create fifos */
    t->mpeg2Fifo  = HBFifoInit( 1024 );
    t->rawFifo    = HBFifoInit( 1 );
    t->scaledFifo = HBFifoInit( 1 );
    t->mpeg4Fifo  = HBFifoInit( 1 );
    a1->ac3Fifo   = HBFifoInit( 1024 );
    a1->rawFifo   = HBFifoInit( 1 );
    a1->mp3Fifo   = HBFifoInit( 1 );
    if( a2 )
    {
        a2->ac3Fifo = HBFifoInit( 1024 );
        a2->rawFifo = HBFifoInit( 1 );
        a2->mp3Fifo = HBFifoInit( 1 );
    }

    /* Create work objects */
    t->mpeg2Dec  = HBMpeg2DecInit( h, t );
    t->scale     = HBScaleInit( h, t );
    
    if( t->codec == HB_CODEC_FFMPEG )
        t->ffmpegEnc = HBFfmpegEncInit( h, t );
    else if( t->codec == HB_CODEC_XVID )
        t->xvidEnc = HBXvidEncInit( h, t );

    a1->ac3Dec   = HBAc3DecInit( h, a1 );
    a1->mp3Enc   = HBMp3EncInit( h, a1 );
    if( a2 )
    {
        a2->ac3Dec = HBAc3DecInit( h, a2 );
        a2->mp3Enc = HBMp3EncInit( h, a2 );
    }
   
    /* Create threads */
    t->dvdRead = HBDVDReadInit( h, t, a1, a2 );
    t->aviMux  = HBAviMuxInit( h, t, a1, a2 );
    for( i = 0; i < h->cpuCount; i++ )
    {
        t->workThreads[i] = HBWorkThreadInit( h, t, a1, a2, i ? 0 : 1 );
    }

    h->curTitle    = t;
    h->curAudio    = a1;
    h->curOptAudio = a2;
}

void HBPauseRip( HBHandle * h )
{
    if( h->status.mode != HB_MODE_ENCODING )
    {
        HBLog( "HBPauseRip: current mode is %d, aborting",
               h->status.mode );
        return;
    }
    
    h->pauseDate = HBGetDate();
    HBLockLock( h->pauseLock );
    HBLockLock( h->lock );
    h->status.mode = HB_MODE_PAUSED;
    h->modeChanged = 1;
    HBLockUnlock( h->lock );
}

void HBResumeRip( HBHandle * h )
{
    if( h->status.mode != HB_MODE_PAUSED )
    {
        HBLog( "HBResumeRip: current mode is %d, aborting",
               h->status.mode );
        return;
    }

    h->beginDate     += HBGetDate() - h->pauseDate;
    h->lastFpsUpdate += HBGetDate() - h->pauseDate;
    HBLockUnlock( h->pauseLock );
    HBLockLock( h->lock );
    h->modeChanged = 1;
    h->status.mode = HB_MODE_ENCODING;
    HBLockUnlock( h->lock );
}

void HBStopRip( HBHandle * h )
{
    if( !( h->status.mode & ( HB_MODE_ENCODING | HB_MODE_PAUSED ) ) )
    {
        HBLog( "HBStopRip: current mode is %d, aborting",
               h->status.mode );
        return;
    }
    
    if( h->status.mode & HB_MODE_PAUSED )
    {
        HBLockUnlock( h->pauseLock );
    }

    HBLockLock( h->lock );
    h->modeChanged = 1;
    h->status.mode = HB_MODE_STOPPING;
    HBLockUnlock( h->lock );
    h->stopRip = 1;
}

uint8_t * HBGetPreview( HBHandle * h, HBTitle * t, int picture )
{
    AVPicture pic1, pic2, pic3, pic4;
    uint8_t * buf1, * buf2, * buf3, * buf4;
    char fileName[1024];
    FILE * file;
    ImgReSampleContext * resampleContext;
    int8_t * preview, * pen;
    int i;

    FixPictureSettings( t );

    buf1 = malloc( 3 * t->inWidth * t->inHeight / 2 );
    buf2 = malloc( 3 * t->inWidth * t->inHeight / 2 );
    buf3 = malloc( 3 * t->outWidth * t->outHeight / 2 );
    buf4 = malloc( 4 * t->outWidth * t->outHeight );

    if( !buf1 || !buf2 || !buf3 || !buf4 )
    {
        HBLog( "HBGetPreview: malloc() failed, gonna crash" );
        return NULL;
    }
    
    /* Original YUV picture */
    avpicture_fill( &pic1, buf1, PIX_FMT_YUV420P, t->inWidth,
                    t->inHeight );

    /* Deinterlaced YUV picture */
    avpicture_fill( &pic2, buf2, PIX_FMT_YUV420P,
                    t->inWidth, t->inHeight );

    /* Scaled YUV picture */
    avpicture_fill( &pic3, buf3, PIX_FMT_YUV420P, t->outWidth,
                    t->outHeight );

    /* Scaled RGB picture ) */
    avpicture_fill( &pic4, buf4, PIX_FMT_RGBA32, t->outWidth,
                    t->outHeight );

    /* Get the original image from the temp file */
    memset( fileName, 0, 1024 );
    sprintf( fileName, "/tmp/HB.%d.%d.%d", h->pid, t->index,
             picture );
    file = fopen( fileName, "r" );
    if( file )
    {
        fread( buf1, 3 * t->inWidth * t->inHeight / 2, 1, file );
        fclose( file );
    }
    else
    {
        HBLog( "HBGetPreview: could not open %s", fileName );
        memset( buf1, 0, 3 * t->inWidth * t->inHeight / 2 );
    }

    /* Deinterlace if needed, and scale */
    resampleContext =
        img_resample_full_init( t->outWidth, t->outHeight,
                                t->inWidth, t->inHeight,
                                t->topCrop, t->bottomCrop,
                                t->leftCrop, t->rightCrop );
    if( t->deinterlace )
    {
        avpicture_deinterlace( &pic2, &pic1, PIX_FMT_YUV420P,
                               t->inWidth, t->inHeight );
        img_resample( resampleContext, &pic3, &pic2 );
    }
    else
    {
        img_resample( resampleContext, &pic3, &pic1 );
    }

    /* Convert to RGB */
    img_convert( &pic4, PIX_FMT_RGBA32, &pic3, PIX_FMT_YUV420P,
                 t->outWidth, t->outHeight );

    /* Create the final preview */
    preview = malloc( 4 * ( t->outWidthMax + 2 ) *
                      ( t->outHeightMax + 2 ) );

    if( !preview )
    {
        HBLog( "HBGetPreview: malloc() failed, gonna crash" );
        return NULL;
    }

    /* Blank it */
    memset( preview, 0x80,
            4 * ( t->outWidthMax + 2 ) * ( t->outHeightMax + 2 ) );

    /* Draw the picture (centered) and draw the cropping zone */
    pen = preview + ( t->outHeightMax - t->outHeight ) *
                    ( t->outWidthMax + 2 ) * 2 +
              ( t->outWidthMax - t->outWidth ) * 2;

    memset( pen, 0xFF, 4 * ( t->outWidth + 2 ) );
    pen += 4 * ( t->outWidthMax + 2 );

    for( i = 0; i < t->outHeight; i++ )
    {
        uint8_t * nextLine = pen + 4 * ( t->outWidthMax + 2 );
        
        memset( pen, 0xFF, 4 );
        pen += 4;
        memcpy( pen, buf4 + 4 * t->outWidth * i, 4 * t->outWidth );
        pen += 4 * t->outWidth;
        memset( pen, 0xFF, 4 );

        pen = nextLine;
    }

    memset( pen, 0xFF, 4 * ( t->outWidth + 2 ) );

    /* Free memory */
    free( buf1 );
    free( buf2 );
    free( buf3 );
    free( buf4 );

    return preview;
    return NULL;
}

void HBClose( HBHandle ** _h )
{
    char command[1024];
    
    HBHandle * h = *_h;
    
    h->die = 1;
    HBThreadClose( &h->thread );

    if( h->status.mode == HB_MODE_SCANNING )
    {
        HBScanClose( &h->scan );
    }
    else if( h->status.mode == HB_MODE_PAUSED )
    {
        HBLockUnlock( h->pauseLock );
        _StopRip( h );
    }
    else if( h->status.mode == HB_MODE_ENCODING )
    {
        _StopRip( h );
    }

    memset( command, 0, 1024 );
    sprintf( command, "rm -f /tmp/HB.%d.*", h->pid );
    system( command );

    if( h->status.titleList )
    {
        HBTitle * title;
        while( ( title = HBListItemAt( h->status.titleList, 0 ) ) )
        {
            HBListRemove( h->status.titleList, title );
            HBTitleClose( &title );
        }
        HBListClose( &h->status.titleList );
    }
    
    HBLockClose( &h->lock );
    HBLockClose( &h->pauseLock );
    free( h );
    
    *_h = NULL;
}

/* Following functions are called by libhb's internal threads */
void HBCheckPaused( HBHandle * h )
{
    HBLockLock( h->pauseLock );
    HBLockUnlock( h->pauseLock );
}

void HBScanning( HBHandle * h, int title )
{
    HBLockLock( h->lock );
    h->status.scannedTitle = title;
    HBLockUnlock( h->lock );
}

void HBScanDone( HBHandle * h, HBList * titleList )
{
    h->status.titleList = titleList;
    h->stopScan         = 1;
}

int HBGetPid( HBHandle * h )
{
    return h->pid;
}

void HBDone( HBHandle * h )
{
    h->ripDone = 1;
}

void HBPosition( HBHandle * h, float position )
{
    if( !h->frames )
    {
        h->beginDate     = HBGetDate();
        h->lastFpsUpdate = h->beginDate;
    }
    
    h->frames++;
    h->framesSinceFpsUpdate++;
    
    HBLockLock( h->lock );
    h->status.position = position;
    if( h->curTitle->twoPass )
    {
        h->status.pass = ( position < 0.5 ) ? 1 : 2;
    }
    else
    {
        h->status.pass = 1;
    }

    if( HBGetDate() - h->lastFpsUpdate > 1000000 )
    {
        h->status.frameRate = 1000000.0 * h->framesSinceFpsUpdate /
                                  ( HBGetDate() - h->lastFpsUpdate );
        h->status.avFrameRate = 1000000.0 * h->frames /
                                  ( HBGetDate() - h->beginDate );
        h->status.remainingTime = ( 1.0 - h->status.position ) *
                                  ( HBGetDate() - h->beginDate ) /
                                  h->status.position / 1000000;
        
        HBLog( "Progress: %.2f %%", position * 100 );
        HBLog( "Speed: %.2f fps (average: %.2f fps, "
               "remaining: %02d:%02d:%02d)",
               h->status.frameRate, h->status.avFrameRate,
               h->status.remainingTime / 3600,
               ( h->status.remainingTime / 60 ) % 60,
               h->status.remainingTime % 60 );
        
        h->lastFpsUpdate        = HBGetDate();
        h->framesSinceFpsUpdate = 0;
    }
    HBLockUnlock( h->lock );
}

void HBErrorOccured( HBHandle * h, HBError error )
{
    if( !( h->status.mode & ( HB_MODE_ENCODING | HB_MODE_PAUSED ) ) )
    {
        return;
    }

    h->status.error = error;
    h->error         = 1;
}

/* Local functions */
static void HandBrakeThread( void * _h )
{
    HBHandle * h = (HBHandle*) _h;

    h->pid = getpid();

    while( !h->die )
    {
        if( h->stopScan )
        {
            HBScanClose( &h->scan );
            HBLockLock( h->lock );
            h->modeChanged = 1;
            h->status.mode = HBListCountItems( h->status.titleList ) ?
                HB_MODE_READY_TO_RIP : HB_MODE_INVALID_DEVICE;
            HBLockUnlock( h->lock );
            h->stopScan = 0;
        }

        if( h->stopRip )
        {
            _StopRip( h );
            
            HBLockLock( h->lock );
            h->modeChanged = 1;
            h->status.mode = HB_MODE_CANCELED;
            HBLockUnlock( h->lock );
            
            h->stopRip = 0;
        }

        if( h->ripDone )
        {
            /* Wait a bit */
            HBSnooze( 500000 );

            _StopRip( h );
            HBLockLock( h->lock );
            h->modeChanged = 1;
            h->status.mode = HB_MODE_DONE;
            HBLockUnlock( h->lock );
            
            h->ripDone = 0;
        }

        if( h->error )
        {
            _StopRip( h );
            
            HBLockLock( h->lock );
            h->modeChanged = 1;
            h->status.mode = HB_MODE_ERROR;
            HBLockUnlock( h->lock );
            
            h->error = 0;
        }

        HBSnooze( 10000 );
    }
}

static void _StopRip( HBHandle * h )
{
    int i;

    /* Stop threads */
    HBDVDReadClose( &h->curTitle->dvdRead );
    HBAviMuxClose( &h->curTitle->aviMux );
    for( i = 0; i < h->cpuCount; i++ )
    {
        HBWorkThreadClose( &h->curTitle->workThreads[h->cpuCount-i-1] );
    }

    /* Clean up */
    HBMpeg2DecClose( &h->curTitle->mpeg2Dec );
    HBScaleClose( &h->curTitle->scale );

    if( h->curTitle->codec == HB_CODEC_FFMPEG )
        HBFfmpegEncClose( &h->curTitle->ffmpegEnc );
    else if( h->curTitle->codec == HB_CODEC_XVID )
        HBXvidEncClose( &h->curTitle->xvidEnc );
    
    HBAc3DecClose( &h->curAudio->ac3Dec );
    HBMp3EncClose( &h->curAudio->mp3Enc );
    if( h->curOptAudio )
    {
        HBAc3DecClose( &h->curOptAudio->ac3Dec );
        HBMp3EncClose( &h->curOptAudio->mp3Enc );
    }

    /* Destroy fifos */
    HBFifoClose( &h->curTitle->mpeg2Fifo );
    HBFifoClose( &h->curTitle->rawFifo );
    HBFifoClose( &h->curTitle->scaledFifo );
    HBFifoClose( &h->curTitle->mpeg4Fifo );
    HBFifoClose( &h->curAudio->ac3Fifo );
    HBFifoClose( &h->curAudio->rawFifo );
    HBFifoClose( &h->curAudio->mp3Fifo );
    if( h->curOptAudio )
    {
        HBFifoClose( &h->curOptAudio->ac3Fifo );
        HBFifoClose( &h->curOptAudio->rawFifo );
        HBFifoClose( &h->curOptAudio->mp3Fifo );
    }
}

static void FixPictureSettings( HBTitle * t )
{
    /* Sanity checks */
    t->topCrop    = EVEN( t->topCrop );
    t->bottomCrop = EVEN( t->bottomCrop );
    t->leftCrop   = EVEN( t->leftCrop );
    t->rightCrop  = EVEN( t->rightCrop );

    t->outWidth   = MIN( t->outWidth, t->outWidthMax );
    t->outWidth   = MAX( 16, t->outWidth );

    t->outHeight  =
        MULTIPLE_16( (uint64_t) t->outWidth * t->inWidth *
                     ( t->inHeight - t->topCrop - t->bottomCrop ) *
                     VOUT_ASPECT_FACTOR /
                     ( (uint64_t) t->inHeight *
                     ( t->inWidth - t->leftCrop - t->rightCrop ) *
                     t->aspect ) );
    t->outHeight  = MAX( 16, t->outHeight );

    if( t->outHeight > t->outHeightMax )
    {
        t->outHeight = t->outHeightMax;
        t->outWidth  =
            MULTIPLE_16( (uint64_t) t->outHeight * t->inHeight *
                         ( t->inWidth - t->leftCrop - t->rightCrop ) *
                         t->aspect /
                         ( (uint64_t) t->inWidth *
                         ( t->inHeight - t->topCrop - t->bottomCrop ) *
                         VOUT_ASPECT_FACTOR ) );
        t->outWidth  = MIN( t->outWidth, t->outWidthMax );
        t->outWidth  = MAX( 16, t->outWidth );
    }
}

static int GetCPUCount()
{
    int CPUCount = 1;

#if defined( SYS_BEOS )
    system_info info;
    get_system_info( &info );
    CPUCount = info.cpu_count;

#elif defined( SYS_MACOSX )
    FILE * info;
    char   buffer[256];

    if( ( info = popen( "/usr/sbin/sysctl hw.ncpu", "r" ) ) )
    {
        if( fgets( buffer, 256, info ) )
        {
            int count;
            if( sscanf( buffer, "hw.ncpu: %d", &count ) == 1 )
            {
                CPUCount = count;
            }
            else
            {
                HBLog( "GetCPUCount: sscanf() failed" );
            }
        }
        else
        {
            HBLog( "GetCPUCount: fgets() failed" );
        }
        fclose( info );
    }
    else
    {
        HBLog( "GetCPUCount: popen() failed" );
    }
   
#elif defined( SYS_LINUX )
    FILE * info;
    char   buffer[256];

    if( ( info = popen( "grep -c '^processor' /proc/cpuinfo", "r" ) ) )
    {
        if( fgets( buffer, 256, info ) )
        {
            int count;
            if( sscanf( buffer, "%d", &count ) == 1 )
            {
                CPUCount = count;
            }
            else
            {
                HBLog( "GetCPUCount: sscanf() failed" );
            }
        }
        else
        {
            HBLog( "GetCPUCount: fgets() failed" );
        }
        fclose( info );
    }
    else
    {
        HBLog( "GetCPUCount: fopen() failed" );
    }
   
#elif defined( SYS_CYGWIN )
    /* TODO */
    CPUCount = 1;
 
#endif
    CPUCount = MAX( 1, CPUCount );
    CPUCount = MIN( CPUCount, 8 );

    return CPUCount;
}

