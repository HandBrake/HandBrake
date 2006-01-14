#ifndef HB_HB_APP_H
#define HB_HB_APP_H

#include "layout-all.h"

class HBWindow;

class HBApp : public MApplication
{
    public:
             HBApp();
        void MessageReceived( BMessage * message );
        void RefsReceived( BMessage * message );

    private:
        HBWindow * fWindow;
};

#endif

