/* $Id: MainWindow.cpp,v 1.4 2003/11/09 21:35:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Alert.h>
#include <Application.h>
#include <Screen.h>

#include "MainWindow.h"
#include "ScanView.h"
#include "RipView.h"

MainWindow::MainWindow()
    : BWindow( BRect( 0,0,10,10 ), "HandBrake " VERSION, B_TITLED_WINDOW,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    fHandle = HBInit( 1, 0 );

    /* Add the scan view */
    fScanView = new ScanView( fHandle );
    fRipView  = new RipView( fHandle );
    AddChild( fScanView );

    /* Resize to fit */
    ResizeTo( fScanView->Bounds().Width(), fScanView->Bounds().Height() );

    BScreen screen;
    MoveTo( ( screen.Frame().Width() - fRipView->Bounds().Width() ) / 2,
            ( screen.Frame().Height() - fRipView->Bounds().Height() ) / 2 );

    /* Update the interface */
    fDie = false;
    fUpdateThread = spawn_thread( (int32 (*)(void *)) UpdateInterface,
                                  "interface", B_DISPLAY_PRIORITY, this );
    resume_thread( fUpdateThread );
}

bool MainWindow::QuitRequested()
{
    /* Clean up */
    fDie = true;
    long exit_value;
    wait_for_thread( fUpdateThread, &exit_value );
    HBClose( &fHandle );

    /* Stop the application */
    be_app->PostMessage( B_QUIT_REQUESTED );
    return true;
}

void MainWindow::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case B_ABOUT_REQUESTED:
        {
            BAlert * alert;
            alert = new BAlert( "About HandBrake",
                "HandBrake " VERSION "\n\n"
                "by Eric Petit <titer@videolan.org>\n"
                "Homepage : <http://handbrake.m0k.org/>\n\n"
                "No, you don't want to know where this stupid app "
                "name comes from.\n\n"
                "Thanks to BGA for pointing out very cool bugs ;)",
                "Woot !" );
            alert->Go( NULL );
            break;
        }

        case B_REFS_RECEIVED:
        case SCAN_RADIO:
        case SCAN_BROWSE_BUTTON:
        case SCAN_OPEN:
            fScanView->MessageReceived( message );
            break;

        case B_SAVE_REQUESTED:
        case RIP_TITLE_POPUP:
        case RIP_BITRATE_RADIO:
        case RIP_TARGET_CONTROL:
        case RIP_CROP_BUTTON:
        case RIP_BROWSE_BUTTON:
        case RIP_SUSPEND_BUTTON:
        case RIP_RIP_BUTTON:
            fRipView->MessageReceived( message );
            break;

        default:
            BWindow::MessageReceived( message );
            break;
    }
}

void MainWindow::UpdateInterface( MainWindow * _this )
{
    uint64_t time;
    int64_t  wait;

    while( !_this->fDie )
    {
        /* Update every 0.1 sec */
        time = system_time();

        _this->_UpdateInterface();

        wait = 100000 - ( system_time() - time );
        if( wait > 0 )
        {
            snooze( wait );
        }
    }
}

void MainWindow::_UpdateInterface()
{
    if( !Lock() )
    {
        fprintf( stderr, "Lock() failed\n" );
        return;
    }
    
    int      modeChanged;
    HBStatus status;
    
    modeChanged = HBGetStatus( fHandle, &status );

    switch( status.mode )
    {
        case HB_MODE_UNDEF:
        case HB_MODE_NEED_DEVICE:
            break;

        case HB_MODE_SCANNING:
        case HB_MODE_INVALID_DEVICE:
            fScanView->UpdateIntf( status, modeChanged );
            break;

        case HB_MODE_READY_TO_RIP:
            if( !modeChanged )
                break;
            
            RemoveChild( fScanView );
            ResizeTo( fRipView->Bounds().Width(),
                      fRipView->Bounds().Height() );
            AddChild( fRipView );
            fRipView->UpdateIntf( status, modeChanged );
            break;

        case HB_MODE_ENCODING:
        case HB_MODE_PAUSED:
        case HB_MODE_DONE:
        case HB_MODE_CANCELED:
        case HB_MODE_ERROR:
            fRipView->UpdateIntf( status, modeChanged );
            break;

        default:
            break;
    }

    Unlock();
}

