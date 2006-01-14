/* $Id: HBManager.h,v 1.25 2003/08/24 13:55:18 titer Exp $ */

#ifndef _HB_MANAGER_H
#define _HB_MANAGER_H

#include <Looper.h>

class HBWindow;
class HBPictureWin;
class HBFifo;

class HBManager : public BLooper
{
    public:
        HBManager( HBWindow * window );
        ~HBManager();
        virtual void MessageReceived( BMessage * message );
        
        /* Methods called by the interface */
        void     Start( HBVolumeInfo   * volumeInfo,
                        HBTitleInfo    * titleInfo,
                        HBAudioInfo    * audio1Info,
                        HBAudioInfo    * audio2Info,
                        char           * file );
        void     Suspend();
        void     Resume();
        bool     Cancel();
        
        /* Methods called by the working threads */
        void     SetPosition( float position );
        void     SetFrameRate( float current, float average );
        void     Done();
        void     Error();
    
    private:
        void     Stop();
        void     DetectVolumes();
    
        /* Interface */
        HBWindow     * fWindow;

        /* Fifos & threads */
        BList        * fThreadList;
        BList        * fFifoList;
        
        /* DVD infos */
        BList        * fVolumeList;
        
        /* Status infos */
        float          fPosition;
        float          fCurrentFrameRate;
        float          fAverageFrameRate;
};

#endif
