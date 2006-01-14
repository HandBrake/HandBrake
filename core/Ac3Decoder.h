/* $Id: Ac3Decoder.h,v 1.10 2003/10/07 20:58:12 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_AC3_DECODER_H
#define HB_AC3_DECODER_H

#include "Common.h"

class HBAc3Decoder
{
    public:
                      HBAc3Decoder( HBManager * manager,
                                    HBAudio * audio );
                      ~HBAc3Decoder();
        bool          Work();

    private:
        bool          Lock();
        void          Unlock();
        bool          GetBytes( uint32_t size );

        HBManager   * fManager;
        HBAudio     * fAudio;
        
        HBLock      * fLock;
        bool          fUsed;

        /* liba52 */
        a52_state_t * fState;
        int           fInFlags;
        int           fOutFlags;
        float         fSampleLevel;

        /* buffers */
        HBBuffer    * fAc3Frame;
        HBBuffer    * fAc3Buffer;
        uint32_t      fPosInBuffer;
        HBBuffer    * fRawBuffer;

        float         fPosition;
        int           fFrameSize;
};

#endif
