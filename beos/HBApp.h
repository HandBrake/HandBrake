/* $Id: HBApp.h,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HB_APP_H
#define HB_HB_APP_H

#include <Application.h>

class MainWindow;

class HBApp : public BApplication
{
    public:
                     HBApp();
        void         MessageReceived( BMessage * message );
        void         RefsReceived( BMessage * message );

        MainWindow * fWindow;
};

#endif
