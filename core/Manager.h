/* $Id: Manager.h,v 1.27 2003/09/30 21:21:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MANAGER_H
#define HB_MANAGER_H

#include "Common.h"
#include "Thread.h"

class HBManager : public HBThread
{
    public:
                      HBManager( bool debug = false );
                      ~HBManager();
        void          DoWork();

        /* Methods called by the interface */
        bool          NeedUpdate();
        HBStatus      GetStatus();
        void          ScanVolumes( char * device );
        void          StartRip( HBTitle * title, HBAudio * audio1,
                                HBAudio * audio2, char * file );
        void          SuspendRip();
        void          ResumeRip();
        void          StopRip();
        uint8_t     * GetPreview( HBTitle * title, uint32_t image );

        /* Methods called by the working threads */
        int           GetPid();
        void          Scanning( char * volume, int title );
        void          ScanDone( HBList * titleList );
        void          Done();
        void          Error();
        void          SetPosition( float pos );

    private:
        void          FixPictureSettings( HBTitle * title );

        int           fPid;

        /* Booleans used in DoWork() */
        bool          fStopScan;
        volatile bool fStopRip;
        bool          fRipDone;
        bool          fError;

        /* Scanner thread */
        HBScanner   * fScanner;

        /* Status infos */
        bool          fNeedUpdate;
        HBStatus      fStatus;
        HBTitle     * fCurTitle;
        HBAudio     * fCurAudio1;
        HBAudio     * fCurAudio2;
};

#endif
