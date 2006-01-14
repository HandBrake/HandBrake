/* $Id: MpegDemux.h,v 1.6 2003/10/04 12:12:48 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MPEG_DEMUX_H
#define HB_MPEG_DEMUX_H

#include "Common.h"
#include "Thread.h"

bool PStoES( HBBuffer * psBuffer, HBList ** _esBufferList );

class HBMpegDemux : public HBThread
{
    public:
                   HBMpegDemux( HBManager * manager, HBTitle * title,
                                HBAudio * audio1, HBAudio * audio2 );

    private:
        void        DoWork();
        void        InsertSilence( int64_t time, HBFifo * fifo,
                                   HBBuffer * buffer );

        HBManager * fManager;
        HBTitle   * fTitle;
        HBAudio   * fAudio1;
        HBAudio   * fAudio2;

        HBBuffer  * fPSBuffer;
        HBBuffer  * fESBuffer;
        HBList    * fESBufferList;

        int64_t     fFirstVideoPTS;
        int64_t     fFirstAudio1PTS;
        int64_t     fFirstAudio2PTS;
};

#endif
