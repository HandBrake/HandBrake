/* $Id: HBApp.cpp,v 1.6 2003/08/24 20:25:49 titer Exp $ */

#include "HBCommon.h"
#include "HBApp.h"
#include "HBWindow.h"
#include "HBManager.h"

/* Constructor */
HBApp::HBApp()
    : BApplication( "application/x-vnd.titer-handbrake" )
{
    /* Initializations */
    fWindow     = new HBWindow();
    fManager    = new HBManager( fWindow );
    
    /* Tell the interface we now have a manager */
    BMessage * message = new BMessage( MANAGER_CREATED );
    message->AddPointer( "manager", fManager );
    fWindow->PostMessage( message );
    delete message;
    
    /* Show the main window */
    fWindow->Show();
    
    /* Check the available DVDs */
    Status( "Checking DVD volumes...", 0.0, ENABLE_DETECTING );
    fManager->PostMessage( DETECT_VOLUMES );
}

void HBApp::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case PRINT_MESSAGE:
        {
            /* See Log() in HBCommon.cpp */
            char * string;
            message->FindPointer( "string", (void**) &string );
            fprintf( stderr, string );
            free( string );
            break;
        }
        
        case CHANGE_STATUS:
        {
            fWindow->PostMessage( message );
            break;
        }
        
        default:
        {
            BApplication::MessageReceived( message );
        }
    }
}

bool HBApp::QuitRequested()
{
    if( fManager->Cancel() )
    {
        /* We have log messages waiting, quit only after having
           displayed them */
        PostMessage( B_QUIT_REQUESTED );
        return false;
    }
        
    return true;
}
