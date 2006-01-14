/* $Id: Resizer.h,v 1.5 2003/10/07 22:48:31 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_RESIZER_H
#define HB_RESIZER_H

#include "Common.h"

class HBResizer
{
    public:
                     HBResizer( HBManager * manager, HBTitle * title );
                     ~HBResizer();
        bool         Work();

    private:
        bool         Lock();
        void         Unlock();
        
        HBManager  * fManager;
        HBTitle    * fTitle;

        HBLock     * fLock;
        bool         fUsed;

        ImgReSampleContext * fResampleContext;
        HBBuffer * fRawBuffer;
        HBBuffer * fDeinterlacedBuffer;
        HBBuffer * fResizedBuffer;
        AVPicture * fRawPicture;
        AVPicture * fDeinterlacedPicture;
        AVPicture * fResizedPicture;
};

#endif
