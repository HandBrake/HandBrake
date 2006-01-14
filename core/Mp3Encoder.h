/* $Id: Mp3Encoder.h,v 1.5 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MP3_ENCODER_H
#define HB_MP3_ENCODER_H

#include "Common.h"
#include "Thread.h"

class HBMp3Encoder : public HBThread
{
    public:
                    HBMp3Encoder( HBManager * manager,
                                  HBAudio * audio );

    private:
        void        DoWork();
        bool        GetSamples( uint32_t count );

        HBManager * fManager;
        HBAudio   * fAudio;

        HBBuffer  * fRawBuffer;
        uint32_t    fPosInBuffer;  /* in samples */
        float     * fLeftSamples;
        float     * fRightSamples;

        float       fPosition;
};

#endif
