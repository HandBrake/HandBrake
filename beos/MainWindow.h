/* $Id: MainWindow.h,v 1.10 2003/10/10 01:08:42 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */


#ifndef HB_MAIN_WINDOW_H
#define HB_MAIN_WINDOW_H

#include <Window.h>
class ScanView;
class RipView;

#include "Common.h"

class MainWindow : public BWindow
{
    public:
                        MainWindow();
        virtual bool    QuitRequested();
        virtual void    MessageReceived( BMessage * message );

    private:
        static void     UpdateInterface( MainWindow * _this );
        void            _UpdateInterface();

        HBManager     * fManager;
        int             fUpdateThread;
        volatile bool   fDie;

        ScanView      * fScanView;
        RipView       * fRipView;
};

#endif
