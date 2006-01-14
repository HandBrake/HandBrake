/* $Id: Mpeg4Encoder.h,v 1.11 2003/10/08 11:56:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MPEG4_ENCODER_H
#define HB_MPEG4_ENCODER_H

#include "Common.h"

class HBMpeg4Encoder
{
    public:
                    HBMpeg4Encoder( HBManager * manager,
                                    HBTitle * title );
        bool        Work();

    private:
        bool        Lock();
        void        Unlock();

        void        Init();
        void        EncodeBuffer();

        HBManager * fManager;
        HBTitle   * fTitle;

        HBLock    * fLock;
        bool        fUsed;

        uint32_t    fPass;
        HBBuffer  * fResizedBuffer;
        AVCodecContext * fContext;
        AVFrame   * fFrame;
        FILE      * fFile;
        char      * fLog;
        HBBuffer * fMpeg4Buffer;
};

#endif
