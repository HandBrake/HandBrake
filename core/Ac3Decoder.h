/* $Id: Ac3Decoder.h,v 1.6 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_AC3_DECODER_H
#define HB_AC3_DECODER_H

#include "Common.h"
#include "Thread.h"

class HBAc3Decoder : public HBThread
{
    public:
                    HBAc3Decoder( HBManager * manager,
                                  HBAudio * audio );
                    ~HBAc3Decoder();

    private:
        void        DoWork();
        bool        GetBytes( uint32_t size );

        HBManager * fManager;
        HBAudio   * fAudio;

        HBBuffer  * fAc3Frame;
        HBBuffer  * fAc3Buffer;
        uint32_t    fPosInBuffer;
        float       fPosition;
};

#endif
