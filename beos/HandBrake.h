/* $Id: HandBrake.h,v 1.7 2003/10/13 22:23:02 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HANDBRAKE_H
#define HB_HANDBRAKE_H

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
