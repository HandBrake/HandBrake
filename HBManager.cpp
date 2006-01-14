/* $Id: HBManager.cpp,v 1.28 2003/08/24 21:56:03 titer Exp $ */

#include "HBCommon.h"
#include "HBManager.h"
#include "HBWindow.h"
#include "HBFifo.h"
#include "HBDVDReader.h"
#include "HBMpegDemux.h"
#include "HBMpeg2Decoder.h"
#include "HBMpeg4Encoder.h"
#include "HBAc3Decoder.h"
#include "HBMp3Encoder.h"
#include "HBAviMuxer.h"

#include <Directory.h>
#include <Drivers.h>
#include <Path.h>
#include <Query.h>
#include <String.h>
#include <VolumeRoster.h>

#include <fs_info.h>
#include <sys/ioctl.h>

/* Public methods */

HBManager::HBManager( HBWindow * window )
    : BLooper( "manager" )
{
    fWindow     = window;
    fFifoList   = new BList();
    fThreadList = new BList();
    fVolumeList = new BList();
    
    Run();
}

HBManager::~HBManager()
{
    delete fFifoList;
    delete fThreadList;
    delete fVolumeList;
}

void HBManager::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case DETECT_VOLUMES:
        {
            DetectVolumes();
            break;
        }

        default:
            BLooper::MessageReceived( message );
    }
}

void HBManager::SetPosition( float position )
{
    if( position - fPosition < 0.0001 )
        /* No need to be more precise ;) */
        return;

    fPosition = position;
    
    char statusText[128]; memset( statusText, 0, 128 );
    sprintf( statusText, "Encoding : %.2f %% - %.2f fps (average : %.2f fps)",
             100 * fPosition, fCurrentFrameRate, fAverageFrameRate );
    Status( statusText, fPosition, ENABLE_ENCODING );
}

void HBManager::SetFrameRate( float current, float average )
{
    fCurrentFrameRate = current;
    fAverageFrameRate = average;
    
    char statusText[128]; memset( statusText, 0, 128 );
    sprintf( statusText, "Encoding : %.2f %% - %.2f fps (average : %.2f fps)",
             100 * fPosition, fCurrentFrameRate, fAverageFrameRate );
    Status( statusText, fPosition, ENABLE_ENCODING );
}

void HBManager::Start( HBVolumeInfo   * volumeInfo,
                       HBTitleInfo    * titleInfo,
                       HBAudioInfo    * audio1Info,
                       HBAudioInfo    * audio2Info,
                       char           * file )
{
    fPosition         = 0;
    fCurrentFrameRate = 0;
    fAverageFrameRate = 0;
    
    /* Remember the fifos that should be freezed in Stop() */
    fFifoList->AddItem( ( titleInfo->fPSFifo    = new HBFifo( 1024 ) ) );
    fFifoList->AddItem( ( titleInfo->fMpeg2Fifo = new HBFifo( 5 ) ) );
    fFifoList->AddItem( ( titleInfo->fRawFifo   = new HBFifo( 5 ) ) );
    fFifoList->AddItem( ( titleInfo->fMpeg4Fifo = new HBFifo( 5 ) ) );
    
    /* Create the threads */
    fThreadList->AddItem( new HBDVDReader( this, titleInfo ) );
    fThreadList->AddItem( new HBMpegDemux( this, titleInfo,
                                           audio1Info, audio2Info ) );
    fThreadList->AddItem( new HBMpeg2Decoder( this, titleInfo ) );
    fThreadList->AddItem( new HBMpeg4Encoder( this, titleInfo ) );
    
    if( audio1Info->fId )
    {
        fFifoList->AddItem( ( audio1Info->fAc3Fifo = new HBFifo( 5 ) ) );
        fFifoList->AddItem( ( audio1Info->fRawFifo = new HBFifo( 5 ) ) );
        fFifoList->AddItem( ( audio1Info->fMp3Fifo = new HBFifo( 5 ) ) );
        fThreadList->AddItem( new HBAc3Decoder( this, audio1Info ) );
        fThreadList->AddItem( new HBMp3Encoder( this, audio1Info ) );
    }
    
    if( audio2Info->fId )
    {
        fFifoList->AddItem( ( audio2Info->fAc3Fifo = new HBFifo( 5 ) ) );
        fFifoList->AddItem( ( audio2Info->fRawFifo = new HBFifo( 5 ) ) );
        fFifoList->AddItem( ( audio2Info->fMp3Fifo = new HBFifo( 5 ) ) );
        fThreadList->AddItem( new HBAc3Decoder( this, audio2Info ) );
        fThreadList->AddItem( new HBMp3Encoder( this, audio2Info ) );
    }
    
    fThreadList->AddItem( new HBAviMuxer( this, titleInfo, audio1Info,
                                          audio2Info, file ) );
    
    /* Run ! */
    HBThread * thread;
    for( int i = 0; i < fThreadList->CountItems(); i++ )
    {
        thread = (HBThread*) fThreadList->ItemAt( i );
        thread->Run();
    }
}

void HBManager::Suspend()
{
    HBThread * thread;
    for( int i = 0; i < fThreadList->CountItems(); i++ )
    {
        thread = (HBThread*) fThreadList->ItemAt( i );
        thread->Suspend();
    }
}

void HBManager::Resume()
{
    HBThread * thread;
    for( int i = 0; i < fThreadList->CountItems(); i++ )
    {
        thread = (HBThread*) fThreadList->ItemAt( i );
        thread->Resume();
    }
}

bool HBManager::Cancel()
{
    if( !( fFifoList->CountItems() ) )
        /* Not running */
        return false;
    
    Status( "Cancelled.", 0.0, ENABLE_READY );
    Stop();
    
    return true;
}

/* Called by the DVD reader */
void HBManager::Done()
{
    HBFifo * fifo = NULL;
    for( int i = 0; i < fFifoList->CountItems(); i++ )
    {
        fifo = (HBFifo*) fFifoList->ItemAt( i );
        
        /* Wait until all threads have finished */
        while( fifo->Size() > 0 )
            snooze( 5000 );
    }
    
    char statusText[128]; memset( statusText, 0, 128 );
    sprintf( statusText, "Done (%.2f fps).", fAverageFrameRate );
    Status( statusText, 1.0, ENABLE_READY );
    Stop();
}

void HBManager::Error()
{
    Status( "An error occured.", 0.0, ENABLE_READY );
    Stop();
}

/* Private */

void HBManager::Stop()
{
    /* Freeze fifos */
    for( int i = 0; i < fFifoList->CountItems(); i++ )
    {
        ((HBFifo*) fFifoList->ItemAt( i ))->Die();
    }
    
    /* Stop threads */
    HBThread * thread;
    while( ( thread = (HBThread*) fThreadList->ItemAt( 0 ) ) )
    {
        fThreadList->RemoveItem( thread );
        delete thread;
    }
    
    /* Destroy fifos */
    HBFifo * fifo;
    while( ( fifo = (HBFifo*) fFifoList->ItemAt( 0 ) ) )
    {
        fFifoList->RemoveItem( fifo );
        delete fifo;
    }
}

void HBManager::DetectVolumes()
{
    /* Empty the current list */
    HBVolumeInfo * volumeInfo;
    while( ( volumeInfo = (HBVolumeInfo*) fVolumeList->ItemAt( 0 ) ) )
    {
        fVolumeList->RemoveItem( volumeInfo );
        delete volumeInfo;
    }

    /* Detect the DVD drives by parsing mounted volumes */
    BVolumeRoster   * roster = new BVolumeRoster();
    BVolume         * volume = new BVolume();
    fs_info           info;
    int               device;
    device_geometry   geometry;
    
    while( roster->GetNextVolume( volume ) == B_NO_ERROR )
    {
        /* open() and ioctl() for more informations */
        fs_stat_dev( volume->Device(), &info );
        if( ( device = open( info.device_name, O_RDONLY ) ) < 0 )
            continue;
        
        if( ioctl( device, B_GET_GEOMETRY, &geometry,
                   sizeof( geometry ) ) < 0 )
            continue;

        /* Get the volume name */
        char volumeName[B_FILE_NAME_LENGTH];
        volume->GetName( volumeName );

        if( volume->IsReadOnly() && geometry.device_type == B_CD )
        {
            /* May be a DVD */
            
            /* Try to open the device */
            volumeInfo = new HBVolumeInfo( volumeName,
                                           info.device_name );
        
            if( !volumeInfo->InitCheck() )
            {
                delete volumeInfo;
                continue;
            }
        
            fVolumeList->AddItem( volumeInfo );
        }
        else if( geometry.device_type == B_DISK )
        {
            /* May be a hard drive. Look for VIDEO_TS folders on it */
            
            BQuery * query = new BQuery();
            
            if( query->SetVolume( volume ) != B_OK )
            {
                delete query;
                continue;
            }
            
            if( query->SetPredicate( "name = VIDEO_TS.BUP" ) != B_OK )
            {
                delete query;
                continue;
            }

            query->Fetch();
            
            BEntry entry, parentEntry;
            BPath  path;
            while( query->GetNextEntry( &entry ) == B_OK )
            {
                entry.GetParent( &parentEntry );
                parentEntry.GetPath( &path );
                
                /* Try to open the folder */
                volumeInfo = new HBVolumeInfo( (char*) path.Path(),
                                               (char*) path.Path() );
        
                if( !volumeInfo->InitCheck() )
                {
                    delete volumeInfo;
                    continue;
                }
        
                fVolumeList->AddItem( volumeInfo );
            }
            
            delete query;
        }
    }
    
    /* Refresh the interface */
    BMessage * message = new BMessage( VOLUMES_DETECTED );
    message->AddPointer( "list", fVolumeList );
    fWindow->PostMessage( message );
    delete message;
}
