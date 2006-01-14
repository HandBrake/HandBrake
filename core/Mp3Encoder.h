/* $Id: Mp3Encoder.h,v 1.9 2003/10/07 22:48:31 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MP3_ENCODER_H
#define HB_MP3_ENCODER_H

#include "Common.h"

class HBMp3Encoder
{
    public:
                    HBMp3Encoder( HBManager * manager,
                                  HBAudio * audio );
        bool        Work();

    private:
        bool        Lock();
        void        Unlock();
        bool        GetSamples();

        HBManager * fManager;
        HBAudio   * fAudio;

        HBLock    * fLock;
        bool        fUsed;

        HBBuffer  * fRawBuffer;
        uint32_t    fPosInBuffer;  /* in samples */
        uint32_t    fSamplesNb;
        float     * fLeftSamples;
        float     * fRightSamples;

        float       fPosition;
        lame_global_flags * fGlobalFlags;
        bool fInitDone;
        HBBuffer * fMp3Buffer;
        uint32_t fCount;
};

#endif
