#include "HBApp.h"
#include "HBWindow.h"

int main()
{
    HBApp * app = new HBApp();
    app->Run();
    delete app;
    return 0;
}

HBApp::HBApp()
    : MApplication( "application/x-vnd.titer-handbrake" )
{
    fWindow = new HBWindow();
}

void HBApp::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        default:
            MApplication::MessageReceived( message );
            break;
    }
}

void HBApp::RefsReceived( BMessage * message )
{
}

