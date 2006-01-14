/* $Id: HBApp.cpp,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>

#include "HBApp.h"
#include "MainWindow.h"

void SigHandler( int signal )
{
    /* Ugly way to exit cleanly when hitting Ctrl-C */
    ((HBApp*) be_app)->fWindow->PostMessage( B_QUIT_REQUESTED );
}

int main()
{
    signal( SIGINT,  SigHandler );
    signal( SIGHUP,  SigHandler );
    signal( SIGQUIT, SigHandler );

    /* Run the BApplication */
    HBApp * app = new HBApp();
    app->Run();
    delete app;
    return 0;
}

/* Constructor */
HBApp::HBApp()
    : BApplication("application/x-vnd.titer-handbrake" )
{
    fWindow = new MainWindow();
    fWindow->Show();
}

void HBApp::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case B_SAVE_REQUESTED:
            fWindow->PostMessage( message );
            break;

        default:
            BApplication::MessageReceived( message );
            break;
    }
}

void HBApp::RefsReceived( BMessage * message )
{
    fWindow->PostMessage( message );
}

