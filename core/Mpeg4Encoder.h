/* $Id: Mpeg4Encoder.h,v 1.8 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MPEG4_ENCODER_H
#define HB_MPEG4_ENCODER_H

#include "Common.h"
#include "Thread.h"

class HBMpeg4Encoder : public HBThread
{
    public:
                    HBMpeg4Encoder( HBManager * manager,
                                    HBTitle * title );

    private:
        void        DoWork();
        void        Init();
        void        Close();
        void        EncodeBuffer();

        HBManager * fManager;
        HBTitle   * fTitle;

        uint32_t    fPass;
        HBBuffer  * fResizedBuffer;
        AVCodecContext * fContext;
        AVFrame   * fFrame;
        FILE      * fFile;
        char      * fLog;
};

#endif
