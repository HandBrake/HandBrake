/* $Id: MainWindow.h,v 1.9 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */


#ifndef HB_MAIN_WINDOW_H
#define HB_MAIN_WINDOW_H

/* BeOS headers */
#include <Box.h>
#include <MenuItem.h>
#include <Window.h>
class BButton;
class BMenuField;
class BPopUpMenu;
class BSlider;
class BStatusBar;
class BStringView;
class BTextControl;

/* libhb headers */
#include "Manager.h"

class HBVolumeItem : public BMenuItem
{
    public:
                   HBVolumeItem( HBVolume * volume );

        HBVolume * fVolume;
};

class HBTitleItem : public BMenuItem
{
    public:
                  HBTitleItem( HBTitle * title );

        HBTitle * fTitle;
};

class HBAudioItem : public BMenuItem
{
    public:
                  HBAudioItem( HBAudio * audio );

        HBAudio * fAudio;
};

class HBBox : public BBox
{
    public:
             HBBox( BRect );
        void Draw( BRect );
};

class HBWindow : public BWindow
{
    public:
                        HBWindow( bool debug );
        virtual bool    QuitRequested();
        virtual void    MessageReceived( BMessage * message );

        void            ScanVolumes();

    private:
        static void     UpdateInterface( HBWindow * _this );
        void            _UpdateInterface();
        void            EnableInterface( HBMode mode );

        HBManager     * fManager;

        /* GUI */
        HBBox         * fBox;
        BButton       * fAdvancedButton;
        BButton       * fFileButton;
        BButton       * fPictureButton;
        BButton       * fStartButton;
        BButton       * fSuspendButton;
        BMenuField    * fAudio1Field;
        BMenuField    * fAudio2Field;
        BMenuField    * fTitleField;
        BMenuField    * fVolumeField;
        BPopUpMenu    * fAudio1PopUp;
        BPopUpMenu    * fAudio2PopUp;
        BPopUpMenu    * fTitlePopUp;
        BPopUpMenu    * fVolumePopUp;
        BSlider       * fAudioSlider;
        BSlider       * fVideoSlider;
        BStatusBar    * fStatusBar;
        BStringView   * fFileString;
        BTextControl  * fFileControl;

        int             fUpdateThread;

        /* Used to SetEnabled() GUI items only if needed */
        HBMode          fOldMode;
};

#endif
