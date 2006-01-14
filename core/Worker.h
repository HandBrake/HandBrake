/* $Id: Worker.h,v 1.3 2003/10/16 13:36:17 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_WORKER_H
#define HB_WORKER_H

#include "Common.h"
#include "Thread.h"

class HBWorker : public HBThread
{
    public:
                        HBWorker( HBTitle * title, HBAudio * audio1,
                                  HBAudio * audio2 );
        void            WaitUntilDone();

    private:
        void            DoWork();
        
        HBTitle       * fTitle;
        HBAudio       * fAudio1;
        HBAudio       * fAudio2;
};

#endif
