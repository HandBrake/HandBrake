/* $Id: Mpeg2Decoder.h,v 1.10 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MPEG2_DECODER_H
#define HB_MPEG2_DECODER_H

#include "Common.h"
#include "Thread.h"

class HBMpeg2Decoder : public HBThread
{
    public:
                     HBMpeg2Decoder( HBManager * manager,
                                     HBTitle * title );

    private:
        void         DoWork();
        void         Init();
        void         Close();
        void         DecodeBuffer();

        HBManager  * fManager;
        HBTitle    * fTitle;

        uint32_t     fPass;
        HBBuffer   * fMpeg2Buffer;
        mpeg2dec_t * fHandle;
        bool         fLateField;
};

#endif
