/* $Id: Mpeg2Decoder.h,v 1.16 2003/10/14 14:35:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MPEG2_DECODER_H
#define HB_MPEG2_DECODER_H

#include "Common.h"

class HBMpeg2Decoder
{
    public:
                     HBMpeg2Decoder( HBManager * manager,
                                     HBTitle * title );
        bool         Work();

    private:
        bool         Lock();
        void         Unlock();
        
        void         Init();
        void         DecodeBuffer();

        HBManager  * fManager;
        HBTitle    * fTitle;

        HBLock     * fLock;
        bool         fUsed;

        uint32_t     fPass;
        HBBuffer   * fMpeg2Buffer;
        HBBuffer   * fRawBuffer;
        HBList     * fRawBufferList;
        mpeg2dec_t * fHandle;
        bool         fLateField;
};

#endif
