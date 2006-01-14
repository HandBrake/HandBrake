/* $Id: MainWindow.cpp,v 1.19 2003/10/13 22:23:02 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <Alert.h>
#include <Application.h>
#include <Screen.h>

#include "Manager.h"
#include "MainWindow.h"
#include "ScanView.h"
#include "RipView.h"

MainWindow::MainWindow()
    : BWindow( BRect( 0,0,10,10 ), "HandBrake " VERSION, B_TITLED_WINDOW,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    /* Init libhb & launch the manager thread */
    fManager = new HBManager( true );

    /* Add the scan view */
    fScanView = new ScanView( fManager );
    fRipView  = new RipView( fManager );
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
    delete fManager;

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
                "Homepage : <http://beos.titer.org/handbrake/>\n\n"
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

    while( !_this->fDie )
    {
        /* Update every 0.1 sec */
        time = system_time();
        _this->_UpdateInterface();
        snooze( 100000 - ( system_time() - time ) );
    }
}

void MainWindow::_UpdateInterface()
{
    if( !fManager->NeedUpdate() )
    {
        return;
    }

    HBStatus status = fManager->GetStatus();

    if( !Lock() )
    {
        fprintf( stderr, "Lock() failed\n" );
        return;
    }

    switch( status.fMode )
    {
        case HB_MODE_UNDEF:
        case HB_MODE_NEED_VOLUME:
            break;
        
        case HB_MODE_SCANNING:
        case HB_MODE_INVALID_VOLUME:
            fScanView->UpdateIntf( status );
            break;

        case HB_MODE_READY_TO_RIP:
            RemoveChild( fScanView );
            ResizeTo( fRipView->Bounds().Width(),
                      fRipView->Bounds().Height() );
            AddChild( fRipView );
            fRipView->UpdateIntf( status );
            break;

        case HB_MODE_ENCODING:
        case HB_MODE_SUSPENDED:
        case HB_MODE_DONE:
        case HB_MODE_CANCELED:
        case HB_MODE_ERROR:
            fRipView->UpdateIntf( status );
            break;

        default:
            break;
    }

    Unlock();
}

