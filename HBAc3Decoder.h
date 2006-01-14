/* $Id: HBAc3Decoder.h,v 1.6 2003/08/25 19:47:14 titer Exp $ */

#ifndef HB_AC3_DECODER_H
#define HB_AC3_DECODER_H

#include "HBThread.h"
class HBAudioInfo;
class HBBuffer;
class HBManager;

class HBAc3Decoder : public HBThread
{
    public:
        HBAc3Decoder( HBManager * manager, HBAudioInfo * audioInfo );
    
    private:
        void DoWork();
        bool GetBytes( uint32_t size );
        void PushSilence();
        
        HBManager         * fManager;
        HBAudioInfo       * fAudioInfo;
        
        HBBuffer          * fAc3Frame;
        HBBuffer          * fAc3Buffer;
        uint32_t            fPosInBuffer;
};

#endif
