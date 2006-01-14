/* $Id: HBMpeg2Decoder.h,v 1.9 2003/08/16 10:17:38 titer Exp $ */

#ifndef _HB_MPEG2_DECODER_H
#define _HB_MPEG2_DECODER_H

#include "HBThread.h"
class HBManager;
class HBBuffer;
class HBFifo;

typedef struct mpeg2dec_s   mpeg2dec_t;
typedef struct AVPicture AVPicture;
typedef struct ImgReSampleContext ImgReSampleContext;

class HBMpeg2Decoder : public HBThread
{
    public:
        HBMpeg2Decoder( HBManager * manager, HBTitleInfo * titleInfo );
    
    private:
        void DoWork();

        HBManager    * fManager;
        HBTitleInfo  * fTitleInfo;
};

#endif
