/* $Id: HandBrake.cpp,v 1.6 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>

#include "HandBrake.h"
#include "MainWindow.h"

void SigHandler( int signal )
{
    ((HBApp*) be_app)->fWindow->PostMessage( B_QUIT_REQUESTED );
}

int main( int argc, char ** argv )
{
    signal( SIGINT,  SigHandler );
    signal( SIGHUP,  SigHandler );
    signal( SIGQUIT, SigHandler );

    int c;
    bool debug = false;
    while( ( c = getopt( argc, argv, "v" ) ) != -1 )
    {
        switch( c )
        {
            case 'v':
                debug = true;
                break;

            default:
                break;
        }
    }

    /* Run the BApplication */
    HBApp * app = new HBApp( debug );
    app->Run();
    delete app;
    return 0;
}

/* Constructor */
HBApp::HBApp( bool debug )
    : BApplication( "application/x-vnd.titer-handbrake" )
{
    fWindow = new HBWindow( debug );
    fWindow->Show();
}

