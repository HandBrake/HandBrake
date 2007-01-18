#ifndef HB_APP_H
#define HB_APP_H

#include <Application.h>

#include "hb.h"

class MainWindow;
class ScanWindow;

class HBApp : public BApplication
{
    public:
                     HBApp();
        void         MessageReceived( BMessage * message );
        void         RefsReceived( BMessage * message );
        void         Pulse();
        bool         QuitRequested();

    private:
        MainWindow * fMainWin;
        ScanWindow * fScanWin;
        
        hb_handle_t * fHandle;
};

#endif

