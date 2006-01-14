#include "HBApp.h"
#include "MainWindow.h"
#include "ScanWindow.h"

#include "hb.h"

int main()
{
    HBApp * app = new HBApp();
    app->Run();
    delete app;
    return 0;
}

HBApp::HBApp()
    : BApplication( "application/x-vnd.titer-handbrake" )
{
    fHandle = hb_init( HB_DEBUG_ALL, 0 );
    
    fMainWin = new MainWindow( fHandle );
    fScanWin = new ScanWindow( fHandle );
    fScanWin->AddToSubset( fMainWin );

    fMainWin->Show();
    fScanWin->Show();

    SetPulseRate( 200000 ); /* 0.2 second */
}

void HBApp::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        default:
            BApplication::MessageReceived( message );
            break;
    }
}

void HBApp::RefsReceived( BMessage * message )
{
}

void HBApp::Pulse()
{
    hb_state_t s;
    hb_get_state( fHandle, &s );

    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;

        case HB_STATE_SCANNING:
            fScanWin->Update( &s );
            break;

        case HB_STATE_SCANDONE:
            if( hb_list_count( hb_get_titles( fHandle ) ) )
            {
                /* Success */
                fScanWin->Hide();
                fMainWin->Update( &s );
            }
            else
            {
                /* Invalid volume */
                fScanWin->Update( &s );
            }
            break;

        case HB_STATE_WORKING:
        case HB_STATE_PAUSED:
        case HB_STATE_WORKDONE:
            fMainWin->Update( &s );
            break;
    }
}

bool HBApp::QuitRequested()
{
    hb_close( &fHandle );
    return BApplication::QuitRequested();
}

