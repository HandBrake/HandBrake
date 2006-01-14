/* $Id: HBMpeg4Encoder.h,v 1.4 2003/08/05 16:47:19 titer Exp $ */

#ifndef _HB_MPEG4_ENCODER_H
#define _HB_MPEG4_ENCODER_H

#include "HBThread.h"
class HBManager;
class HBFifo;
class HBAudioInfo;
class HBTitleInfo;
class HBBuffer;

class HBMpeg4Encoder : public HBThread
{
    public:
        HBMpeg4Encoder( HBManager * manager, HBTitleInfo * titleInfo );
    
    private:
        void DoWork();
    
        HBManager   * fManager;
        HBTitleInfo * fTitleInfo;
        
        HBBuffer    * fRawBuffer;
};

#endif
