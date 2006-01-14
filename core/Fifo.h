/* $Id: Fifo.h,v 1.11 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_FIFO_H
#define HB_FIFO_H

#include "Common.h"

class HBBuffer
{
    public:
                   HBBuffer( uint32_t size );
                   ~HBBuffer();
        void       ReAlloc( uint32_t size );

        uint32_t   fAllocSize;
        uint32_t   fSize;
        uint8_t  * fData;

        float      fPosition;
        uint32_t   fStreamId;
        bool       fKeyFrame;
        uint64_t   fPTS;
        uint32_t   fPass;
};

class HBFifo
{
    public:
                    HBFifo( int capacity = 32 );
                    ~HBFifo();

        int         Size();
        int         Capacity();
        bool        Push( HBBuffer * buffer );
        HBBuffer  * Pop();

    private:
        int         fCapacity;
        int         fWhereToPush;
        int         fWhereToPop;
        HBBuffer ** fBuffers;
        HBLock    * fLock;
};

#endif
