/* $Id: HandBrake.c,v 1.47 2004/03/21 22:58:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libavcodec */
#include "ffmpeg/avcodec.h"

/* Local prototypes */
static void HandBrakeThread( void * );
static void _StopRip( HBHandle * );
static void FixPictureSettings( HBTitle * );
static int  GetCPUCount();

struct HBHandle
{
    int            cpuCount;
    HBCallbacks    cb;

    int            stopScan;
    int            stopRip;
    int            ripDone;
    int            error;

    HBScan       * scan;
    HBList       * titleList;
    HBTitle      * curTitle;
    uint64_t       beginDate;
    uint64_t       pauseDate;
    uint64_t       lastPosUpdate;
    uint64_t       lastFpsUpdate;
    int            framesSinceBegin;
    int            framesSinceFps;
    float          curFrameRate;
    float          avgFrameRate;
    int            remainingTime;

    HBLock       * lock;
    HBLock       * pauseLock;
    volatile int   die;
    HBThread     * thread;
    int            pid;
};

HBHandle * HBInit( int debug, int cpuCount )
{
    HBHandle * h;
    if( !( h = calloc( sizeof( HBHandle ), 1 ) ) )
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

    h->lock      = HBLockInit();
    h->pauseLock = HBLockInit();
    h->thread    = HBThreadInit( "libhb", HandBrakeThread, h,
                                 HB_NORMAL_PRIORITY );
    return h;
}

void HBSetCallbacks( HBHandle * h, HBCallbacks callbacks )
{
    HBLockLock( h->lock );
    h->cb = callbacks;
    HBLockUnlock( h->lock );
}

void HBScanDVD( HBHandle * h, const char * dvd, int title )
{
    HBLockLock( h->lock );
    h->scan = HBScanInit( h, dvd, title );
    HBLockUnlock( h->lock );
}

void HBStartRip( HBHandle * h, HBTitle * title )
{
    int i;
    HBAudio * audio;

    HBLockLock( h->lock );

    h->beginDate        = HBGetDate();
    h->lastPosUpdate    = 0;
    h->lastFpsUpdate    = 0;
    h->framesSinceBegin = 0;
    h->framesSinceFps   = 0;

    FixPictureSettings( title );

    /* Video fifos */
    title->inFifo     = HBFifoInit( 2048 );
    title->rawFifo    = HBFifoInit( 1 );
    title->scaledFifo = HBFifoInit( 1 );
    title->outFifo    = HBFifoInit( 1 );

    /* Video work objects */
    title->decoder    = HBMpeg2DecInit( h, title );
    title->scale      = HBScaleInit( h, title );
    if( title->codec == HB_CODEC_FFMPEG )
        title->encoder = HBFfmpegEncInit( h, title );
    else if( title->codec == HB_CODEC_XVID )
        title->encoder = HBXvidEncInit( h, title );
    else if( title->codec == HB_CODEC_X264 )
        title->encoder = HBX264EncInit( h, title );

    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );

        /* Audio fifos */
        audio->inFifo  = HBFifoInit( 2048 );
        audio->rawFifo = HBFifoInit( 1 );
        audio->outFifo = HBFifoInit( 4 ); /* At least 4 for Vorbis */

        /* Audio work objects */
        if( audio->inCodec == HB_CODEC_AC3 )
            audio->decoder = HBAc3DecInit( h, audio );
        else if( audio->inCodec == HB_CODEC_LPCM )
            audio->decoder = HBLpcmDecInit( h, audio );

        if( audio->outCodec == HB_CODEC_MP3 )
            audio->encoder = HBMp3EncInit( h, audio );
        else if( audio->outCodec == HB_CODEC_AAC )
            audio->encoder = HBFaacEncInit( h, audio );
        else if( audio->outCodec == HB_CODEC_VORBIS )
            audio->encoder = HBVorbisEncInit( h, audio );
    }

    /* Create threads */
    title->dvdRead = HBDVDReadInit( h, title );

    if( title->mux == HB_MUX_AVI )
        title->aviMux  = HBAviMuxInit( h, title );
    else if( title->mux == HB_MUX_MP4 )
        title->mp4Mux  = HBMp4MuxInit( h, title );
    else if( title->mux == HB_MUX_OGM )
        title->ogmMux  = HBOgmMuxInit( h, title );

    for( i = 0; i < h->cpuCount; i++ )
    {
        title->workThreads[i] = HBWorkThreadInit( h, title, i ? 0 : 1 );
    }

    h->curTitle = title;

    HBLockUnlock( h->lock );
}

void HBPauseRip( HBHandle * h )
{
    HBLockLock( h->lock );
    h->pauseDate = HBGetDate();
    HBLockLock( h->pauseLock );
    HBLockUnlock( h->lock );
}

void HBResumeRip( HBHandle * h )
{
    HBLockLock( h->lock );
    h->beginDate     += HBGetDate() - h->pauseDate;
    h->lastPosUpdate += HBGetDate() - h->pauseDate;
    h->lastFpsUpdate += HBGetDate() - h->pauseDate;
    HBLockUnlock( h->pauseLock );
    HBLockUnlock( h->lock );
}

void HBStopRip( HBHandle * h )
{
    HBLockLock( h->lock );
    h->stopRip = 1;
    HBLockUnlock( h->lock );
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

int HBGetBitrateForSize( HBTitle * title, int size, int muxer,
                         int audioCount, int audioBitrate )
{
    int64_t available;
    int     overheadPerFrame;
    int     sampleRate;
    int     samplesPerFrame;

    switch( muxer )
    {
        case HB_MUX_MP4:
            overheadPerFrame = 5;     /* hopefully */
            sampleRate       = 48000; /* No resampling */
            samplesPerFrame  = 1024;  /* AAC */
            break;
        case HB_MUX_AVI:
            overheadPerFrame = 24;
            sampleRate       = 44100; /* Resampling */
            samplesPerFrame  = 1152;  /* MP3 */
            break;
        case HB_MUX_OGM:
            overheadPerFrame = 0;     /* XXX */
            sampleRate       = 48000; /* No resampling */
            samplesPerFrame  = 1024;  /* Vorbis */
            break;
        default:
            return 0;
    }

    /* Actually target 1 MB less */
    available  = (int64_t) ( size - 1 ) * 1024 * 1024;

    /* Audio data */
    available -= audioCount * title->length * audioBitrate * 128;

    /* Video headers */
    available -= (int64_t) title->length * title->rate *
        overheadPerFrame / title->rateBase;

    /* Audio headers */
    available -= (int64_t) audioCount * title->length * sampleRate *
        overheadPerFrame / samplesPerFrame;

    if( available < 0 )
    {
        return 0;
    }
    return( available / ( 128 * title->length ) );
}

void HBClose( HBHandle ** _h )
{
    char command[1024];

    HBHandle * h = *_h;

    h->die = 1;
    HBThreadClose( &h->thread );

    if( h->scan )
    {
        HBScanClose( &h->scan );
    }
    if( h->curTitle )
    {
        _StopRip( h );
    }
    if( h->titleList )
    {
        HBTitle * title;
        while( ( title = (HBTitle*) HBListItemAt( h->titleList, 0 ) ) )
        {
            HBListRemove( h->titleList, title );
            HBTitleClose( &title );
        }
    }

    memset( command, 0, 1024 );
    sprintf( command, "rm -f /tmp/HB.%d.*", h->pid );
    system( command );

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

void HBScanning( HBHandle * h, int title, int titleCount )
{
    h->cb.scanning( h->cb.data, title, titleCount );
}

void HBScanDone( HBHandle * h, HBList * titleList )
{
    HBLockLock( h->lock );
    h->stopScan  = 1;
    h->titleList = titleList;
    HBLockUnlock( h->lock );
    h->cb.scanDone( h->cb.data, titleList );
}

int HBGetPid( HBHandle * h )
{
    return h->pid;
}

void HBDone( HBHandle * h )
{
    HBLockLock( h->lock );
    h->ripDone = 1;
    HBLockUnlock( h->lock );
}

void HBPosition( HBHandle * h, float position )
{
    int pass, passCount;

    h->framesSinceBegin++;
    h->framesSinceFps++;

    if( h->curTitle->twoPass )
    {
        pass      = ( position < 0.5 ) ? 1 : 2;
        passCount = 2;
    }
    else
    {
        passCount = pass = 1;
    }

    if( HBGetDate() - h->lastPosUpdate < 200000 )
    {
        return;
    }

    h->lastPosUpdate  = HBGetDate();

    if( HBGetDate() - h->lastFpsUpdate > 1000000 )
    {
        h->curFrameRate = 1000000.0 * h->framesSinceFps /
            ( HBGetDate() - h->lastFpsUpdate );
        h->avgFrameRate = 1000000.0 * h->framesSinceBegin /
            ( HBGetDate() - h->beginDate );
        h->remainingTime = ( 1.0 - position ) *
            ( HBGetDate() - h->beginDate ) / position / 1000000;

        h->lastFpsUpdate  = HBGetDate();
        h->framesSinceFps = 0;
    }

    h->cb.encoding( h->cb.data, position, pass, passCount,
                           h->curFrameRate, h->avgFrameRate,
                           h->remainingTime );
}

void HBErrorOccured( HBHandle * h, int error )
{
    HBLockLock( h->lock );
    h->error = error;
    HBLockUnlock( h->lock );
}

/* Local functions */
static void HandBrakeThread( void * _h )
{
    HBHandle * h = (HBHandle*) _h;

    h->pid = getpid();

    while( !h->die )
    {
        HBLockLock( h->lock );

        if( h->stopScan )
        {
            HBScanClose( &h->scan );
            h->stopScan = 0;
            HBLockUnlock( h->lock );
            continue;
        }

        if( h->stopRip )
        {
            _StopRip( h );
            h->stopRip = 0;
            HBLockUnlock( h->lock );
            h->cb.ripDone( h->cb.data, HB_CANCELED );
            continue;
        }

        if( h->ripDone )
        {
            HBTitle * title = h->curTitle;
            HBAudio * audio;
            int       i, ok = 0;

            /* Wait until we're done with the decoding of one track */
            for( ;; )
            {
                if( !HBFifoSize( title->inFifo ) &&
                    !HBFifoSize( title->rawFifo ) &&
                    !HBFifoSize( title->scaledFifo ) )
                {
                    break;
                }
                for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
                {
                    audio = (HBAudio*) HBListItemAt( title->ripAudioList, i );
                    if( !HBFifoSize( title->inFifo ) &&
                        !HBFifoSize( title->rawFifo ) )
                    {
                        ok = 1;
                        break;
                    }
                }
                if( ok )
                {
                    break;
                }
                HBSnooze( 5000 );
            }

            HBSnooze( 500000 );
            _StopRip( h );
            h->ripDone = 0;
            HBLockUnlock( h->lock );
            h->cb.ripDone( h->cb.data, HB_SUCCESS );
            continue;
        }

        if( h->error )
        {
            _StopRip( h );
            h->error = 0;
            HBLockUnlock( h->lock );
            h->cb.ripDone( h->cb.data, h->error );
            continue;
        }

        HBLockUnlock( h->lock );
        HBSnooze( 10000 );
    }
}

static void _StopRip( HBHandle * h )
{
    HBTitle * title = h->curTitle;
    HBAudio * audio;
    int i;

    if( !title )
    {
        return;
    }

    /* Stop input and work threads */
    HBDVDReadClose( &title->dvdRead );
    for( i = 0; i < h->cpuCount; i++ )
    {
        HBWorkThreadClose( &title->workThreads[h->cpuCount-i-1] );
    }

    /* Invalidate fifos */
    HBFifoDie( title->outFifo );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        HBFifoDie( audio->outFifo );
    }

    /* Stop mux thread */
    if( title->mux == HB_MUX_AVI )
        HBAviMuxClose( &title->aviMux );
    else if( title->mux == HB_MUX_MP4 )
        HBMp4MuxClose( &title->mp4Mux );
    else if( title->mux == HB_MUX_OGM )
        HBOgmMuxClose( &title->ogmMux );

    /* Clean up */
    HBMpeg2DecClose( &title->decoder );
    HBScaleClose( &title->scale );

    if( title->codec == HB_CODEC_FFMPEG )
        HBFfmpegEncClose( &title->encoder );
    else if( title->codec == HB_CODEC_XVID )
        HBXvidEncClose( &title->encoder );
    else if( title->codec == HB_CODEC_X264 )
        HBX264EncClose( &title->encoder );

    HBFifoClose( &title->inFifo );
    HBFifoClose( &title->rawFifo );
    HBFifoClose( &title->scaledFifo );
    HBFifoClose( &title->outFifo );

    while( ( audio = HBListItemAt( title->ripAudioList, 0 ) ) )
    {
        /* Audio work objects */
        if( audio->inCodec == HB_CODEC_AC3 )
            HBAc3DecClose( &audio->decoder );
        else if( audio->inCodec == HB_CODEC_LPCM )
            HBLpcmDecClose( &audio->decoder );

        if( audio->outCodec == HB_CODEC_MP3 )
            HBMp3EncClose( &audio->encoder );
        else if( audio->outCodec == HB_CODEC_AAC )
            HBFaacEncClose( &audio->encoder );
        else if( audio->outCodec == HB_CODEC_VORBIS )
            HBVorbisEncClose( &audio->encoder );

        /* Audio fifos */
        HBFifoClose( &audio->inFifo );
        HBFifoClose( &audio->rawFifo );
        HBFifoClose( &audio->outFifo );

        HBListRemove( title->ripAudioList, audio );
    }

    h->curTitle = NULL;
}

static void FixPictureSettings( HBTitle * t )
{
    /* Sanity checks */
    t->outWidth   = MULTIPLE_16( t->outWidth );
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

#if defined( HB_BEOS )
    system_info info;
    get_system_info( &info );
    CPUCount = info.cpu_count;

#elif defined( HB_MACOSX )
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

#elif defined( HB_LINUX )
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

#elif defined( HB_CYGWIN )
    /* TODO */
    CPUCount = 1;

#endif
    CPUCount = MAX( 1, CPUCount );
    CPUCount = MIN( CPUCount, 8 );

    return CPUCount;
}

