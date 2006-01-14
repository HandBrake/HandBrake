/* $Id: DVDReader.h,v 1.5 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_DVD_READER_H
#define HB_DVD_READER_H

#include "Common.h"
#include "Thread.h"

class HBDVDReader : public HBThread
{
    public:
                    HBDVDReader( HBManager * manager, HBTitle * title );

    private:
        void        DoWork();

        HBManager * fManager;
        HBTitle   * fTitle;
};

#endif
