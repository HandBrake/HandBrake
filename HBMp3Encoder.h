/* $Id: HBMp3Encoder.h,v 1.3 2003/08/23 19:22:59 titer Exp $ */

#ifndef HB_MP3_ENCODER_H
#define HB_MP3_ENCODER_H

#include "HBThread.h"
class HBAudioInfo;
class HBManager;
class HBBuffer;
class HBFifo;

class HBMp3Encoder : public HBThread
{
    public:
        HBMp3Encoder( HBManager * manager, HBAudioInfo * audioInfo );
    
    private:
        void DoWork();
        bool GetSamples( uint32_t count );
        
        HBManager         * fManager;
        HBAudioInfo       * fAudioInfo;
        
        HBBuffer          * fRawBuffer;
        uint32_t            fPosInBuffer;  /* in samples */
        float             * fLeftSamples;
        float             * fRightSamples;
};

#endif
