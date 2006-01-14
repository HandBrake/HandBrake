/* $Id: Scanner.h,v 1.5 2003/09/30 21:21:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_SCANNER_H
#define HB_SCANNER_H

#include "Common.h"
#include "Thread.h"

class HBScanner : public HBThread
{
    public:
                    HBScanner( HBManager * manager, char * device );

    private:
        void        DoWork();
        bool        ScanTitle( HBTitle * title, dvdplay_ptr vmg );
        bool        DecodeFrame( HBTitle * title, dvdplay_ptr vmg, int i );

        HBManager * fManager;
        char      * fDevice;
};

#endif
