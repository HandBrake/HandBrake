/* $Id: HBMpeg2Decoder.cpp,v 1.25 2003/08/23 19:38:47 titer Exp $ */

#include "HBCommon.h"
#include "HBManager.h"
#include "HBMpeg2Decoder.h"
#include "HBFifo.h"

extern "C" {
#include <mpeg2dec/mpeg2.h>
}
#include <ffmpeg/avcodec.h>

HBMpeg2Decoder::HBMpeg2Decoder( HBManager * manager, HBTitleInfo * titleInfo )
    : HBThread( "mpeg2decoder" )
{
    fManager   = manager;
    fTitleInfo = titleInfo;
}

void HBMpeg2Decoder::DoWork()
{
    /* Statistics */
    uint32_t framesSinceLast  = 0;
    uint32_t framesSinceBegin = 0;
    uint64_t lastTime         = 0;
    uint64_t beginTime        = 0;

    /* Init buffers */
    HBBuffer  * mpeg2Buffer         = NULL;
    HBBuffer  * rawBuffer           = NULL;
    HBBuffer  * deinterlacedBuffer  = NULL;
    HBBuffer  * resizedBuffer       = NULL;
    AVPicture * rawPicture          = (AVPicture*) malloc( sizeof( AVPicture ) );
    AVPicture * deinterlacedPicture = (AVPicture*) malloc( sizeof( AVPicture ) );
    AVPicture * resizedPicture      = (AVPicture*) malloc( sizeof( AVPicture ) );
    
    /* Init libmpeg2 */
    mpeg2dec_t         * handle = mpeg2_init();
    const mpeg2_info_t * info   = mpeg2_info( handle );
    
    /* libavcodec */
    ImgReSampleContext * resampleContext = NULL;
    
    /* NTSC 3:2 pulldown kludge - UGLY ! */
    if( fTitleInfo->fScale == 900900 )
    {
        fTitleInfo->fScale = 1125000;
    }
    
    /* Resizing & cropping initializations */
    resampleContext = img_resample_full_init( fTitleInfo->fOutWidth, fTitleInfo->fOutHeight,
                                              fTitleInfo->fInWidth, fTitleInfo->fInHeight,
                                              fTitleInfo->fTopCrop, fTitleInfo->fBottomCrop,
                                              fTitleInfo->fLeftCrop, fTitleInfo->fRightCrop );
    rawBuffer          = new HBBuffer( 3 * fTitleInfo->fInWidth * fTitleInfo->fInHeight / 2 );
    deinterlacedBuffer = new HBBuffer( 3 * fTitleInfo->fInWidth * fTitleInfo->fInHeight / 2 );
    avpicture_fill( rawPicture, rawBuffer->fData,
                    PIX_FMT_YUV420P, fTitleInfo->fInWidth, fTitleInfo->fInHeight );
    avpicture_fill( deinterlacedPicture, deinterlacedBuffer->fData,
                    PIX_FMT_YUV420P, fTitleInfo->fInWidth, fTitleInfo->fInHeight );
                            
    /* Init statistics */
    lastTime         = system_time();
    beginTime        = system_time();
            
    Log( "HBMpeg2Decoder : %dx%d -> %dx%d, %.2f fps",
         fTitleInfo->fInWidth, fTitleInfo->fInHeight,
         fTitleInfo->fOutWidth, fTitleInfo->fOutHeight,
         (float) fTitleInfo->fRate / fTitleInfo->fScale );
    
    /* Main loop */
    mpeg2_state_t state;
    for( ;; )
    {
        state = mpeg2_parse( handle );
        
        if( state == STATE_BUFFER )
        {
            /* Free the previous buffer */
            if( mpeg2Buffer )
                delete mpeg2Buffer;
            
            /* Get a new one */
            if( !( mpeg2Buffer = fTitleInfo->fMpeg2Fifo->Pop() ) )
                break;
            
            /* Feed libmpeg2 */
            mpeg2_buffer( handle, mpeg2Buffer->fData,
                          mpeg2Buffer->fData + mpeg2Buffer->fSize );
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 info->display_fbuf )
        {
            /* Got a raw picture */

            /* Copy it */
            /* TODO : make libmpeg2 write directly in our buffer */
            memcpy( rawBuffer->fData,
                    info->display_fbuf->buf[0],
                    fTitleInfo->fInWidth * fTitleInfo->fInHeight );
            memcpy( rawBuffer->fData + fTitleInfo->fInWidth * fTitleInfo->fInHeight,
                    info->display_fbuf->buf[1],
                    fTitleInfo->fInWidth * fTitleInfo->fInHeight / 4 );
            memcpy( rawBuffer->fData + fTitleInfo->fInWidth * fTitleInfo->fInHeight +
                        fTitleInfo->fInWidth * fTitleInfo->fInHeight / 4,
                    info->display_fbuf->buf[2],
                    fTitleInfo->fInWidth * fTitleInfo->fInHeight / 4 );

            resizedBuffer = new HBBuffer( 3 * fTitleInfo->fOutWidth * fTitleInfo->fOutHeight / 2 );
            avpicture_fill( resizedPicture, resizedBuffer->fData, PIX_FMT_YUV420P,
                            fTitleInfo->fOutWidth, fTitleInfo->fOutHeight );
    
            if( fTitleInfo->fDeinterlace )
            {
                avpicture_deinterlace( deinterlacedPicture, rawPicture, PIX_FMT_YUV420P,
                                       fTitleInfo->fInWidth, fTitleInfo->fInHeight );
                img_resample( resampleContext, resizedPicture,
                              deinterlacedPicture );
            }
            else
            {
                img_resample( resampleContext, resizedPicture, rawPicture );
            }

            /* Send it to the encoder */
            if( !( fTitleInfo->fRawFifo->Push( resizedBuffer ) ) )
            {
                break;
            }

            /* Update GUI position */
            fManager->SetPosition( mpeg2Buffer->fPosition );
            
            /* Statistics every 0.5 second */
            framesSinceLast++;
            framesSinceBegin++;
            if( system_time() - lastTime > 500000 )
            {
                fManager->SetFrameRate( 1000000 * (float) framesSinceLast /
                                        (float) ( system_time() - lastTime ),
                                        1000000 * (float) framesSinceBegin /
                                        (float) ( system_time() - beginTime ) );
                lastTime        = system_time();
                framesSinceLast = 0;
            }
            
        }
        else if( state == STATE_INVALID )
        {
            /* Shouldn't happen on a DVD */
            Log( "HBMpeg2Decoder : STATE_INVALID" );
        }
    }
    
    /* Close libmpeg2 */
    mpeg2_close( handle );
    
    /* Close libavcodec */
    img_resample_close( resampleContext );

    /* Free structures & buffers */
    if( mpeg2Buffer )         delete mpeg2Buffer;
    if( rawBuffer )           delete rawBuffer;
    if( deinterlacedBuffer )  delete deinterlacedBuffer;
    if( rawPicture )          free( rawPicture );
    if( deinterlacedPicture ) free( deinterlacedPicture );
    if( resizedPicture )      free( resizedPicture );
}
