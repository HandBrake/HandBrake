/* $Id: HBDVDReader.h,v 1.3 2003/08/12 20:10:50 titer Exp $ */

#ifndef HB_DVD_READER_H
#define HB_DVD_READER_H

#include "HBThread.h"
class HBManager;
class HBTitleInfo;
class HBFifo;

class HBDVDReader : public HBThread
{
    public:
        HBDVDReader( HBManager * manager, HBTitleInfo * titleInfo );
    
    private:
        void DoWork();
    
        HBManager   * fManager;
        HBTitleInfo * fTitleInfo;
};

#endif
