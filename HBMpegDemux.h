/* $Id: HBMpegDemux.h,v 1.7 2003/08/24 13:27:41 titer Exp $ */

#ifndef _HB_MPEG_DEMUX_H
#define _HB_MPEG_DEMUX_H

#include "HBThread.h"
class HBManager;
class HBBuffer;
class BList;

BList * PStoES( HBBuffer * psBuffer );

class HBMpegDemux : public HBThread
{
    public:
        HBMpegDemux( HBManager * manager, HBTitleInfo * titleInfo,
                     HBAudioInfo * audio1Info, HBAudioInfo * audio2Info );
    
    private:
        void DoWork();
        void InsertSilence( int64_t time, HBFifo * fifo,
                            HBBuffer * buffer );
        
        HBManager   * fManager;
        HBTitleInfo * fTitleInfo;
        HBAudioInfo * fAudio1Info;
        HBAudioInfo * fAudio2Info;
        
        HBBuffer    * fPSBuffer;
        HBBuffer    * fESBuffer;
        BList       * fESBufferList;
        
        int64_t      fFirstVideoPTS;
        int64_t      fFirstAudio1PTS;
        int64_t      fFirstAudio2PTS;
};

#endif
