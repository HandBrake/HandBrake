/* $Id: HBApp.h,v 1.1.1.1 2003/06/24 13:43:48 titer Exp $ */

#ifndef _HB_APP_H
#define _HB_APP_H

#include <Application.h>

class HBWindow;
class HBManager;

class HBApp : public BApplication
{
    public:
        HBApp();
        virtual void MessageReceived( BMessage * message );
        virtual bool QuitRequested();
    
    private:
        HBWindow     * fWindow;
        HBManager    * fManager;
};

#endif
