/* $Id: MainWindow.h,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#ifndef HB_MAIN_WINDOW_H
#define HB_MAIN_WINDOW_H

#include <Window.h>
class ScanView;
class RipView;

#include "HandBrake.h"

class MainWindow : public BWindow
{
    public:
                        MainWindow();
        virtual bool    QuitRequested();
        virtual void    MessageReceived( BMessage * message );

    private:
        static void     UpdateInterface( MainWindow * _this );
        void            _UpdateInterface();

        HBHandle      * fHandle;
        int             fUpdateThread;
        volatile bool   fDie;

        ScanView      * fScanView;
        RipView       * fRipView;
};

#endif
