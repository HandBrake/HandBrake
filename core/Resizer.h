/* $Id: Resizer.h,v 1.2 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_RESIZER_H
#define HB_RESIZER_H

#include "Common.h"
#include "Thread.h"

class HBResizer : public HBThread
{
    public:
                     HBResizer( HBManager * manager, HBTitle * title );

    private:
        void         DoWork();

        HBManager  * fManager;
        HBTitle    * fTitle;
};

#endif
