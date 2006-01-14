/* $Id: HBFifo.h,v 1.9 2003/08/24 13:27:41 titer Exp $ */

#ifndef _HB_FIFO_H
#define _HB_FIFO_H

#define DVD_DATA     0x01
#define MPEG2_VIDEO  0x02
#define RAW_VIDEO    0x04
#define RAW2_VIDEO   0x08
#define MPEG4_VIDEO  0x10
#define AC3_AUDIO    0x20
#define RAW_AUDIO    0x40
#define MP3_AUDIO    0x80

class BLocker;

class HBBuffer
{
    public:
        /* Common functions */
        HBBuffer( int size );
        ~HBBuffer();
        void      ReAlloc( int size );
        
        /* Common members */
        uint32_t  fAllocSize;
        uint32_t  fSize;
        uint8_t * fData;

        /* Misc */
        float     fPosition;
        uint32_t  fStreamId;
        bool      fKeyFrame;
        uint64_t  fPTS;
};

class HBFifo
{
    public:
        HBFifo( int capacity );
        void Die();
        ~HBFifo();

        int Size();
        int Capacity();
        bool Push( HBBuffer * buffer );
        HBBuffer * Pop();
        
    private:
        void Lock();
        void Unlock();

        int fCapacity;
        int fWhereToPush;
        int fWhereToPop;
        HBBuffer ** fBuffers;
        BLocker * fLocker;
        volatile bool fDie;
};

#endif
