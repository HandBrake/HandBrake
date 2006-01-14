/* $Id: HBWindow.h,v 1.8 2003/08/24 19:28:18 titer Exp $ */

#ifndef _HB_WINDOW_H
#define _HB_WINDOW_H

#include <Box.h>
#include <Window.h>

class HBManager;

class BButton;
class BMenuField;
class BPopUpMenu;
class BSlider;
class BStatusBar;
class BStringView;
class BTextControl;

class HBBox : public BBox
{
    public:
        HBBox( BRect );
        virtual void Draw( BRect );
};

class HBWindow : public BWindow
{
    public:
        HBWindow();
        virtual bool QuitRequested();
        virtual void MessageReceived( BMessage * message );
        
        void         RefreshVolumes( BList * volumeList );
    
    private:
        void         Enable( int mode );
        void         SelectionChanged();
    
        HBManager     * fManager;
        HBBox         * fBox;
        BMenuField    * fVolumeField;
        BPopUpMenu    * fVolumePopUp;
        BMenuField    * fTitleField;
        BPopUpMenu    * fTitlePopUp;
        BMenuField    * fAudio1Field;
        BPopUpMenu    * fAudio1PopUp;
        BMenuField    * fAudio2Field;
        BPopUpMenu    * fAudio2PopUp;
        BSlider       * fVideoSlider;
        BSlider       * fAudioSlider;
        BStringView   * fFileString;
        BTextControl  * fFileControl;
        BButton       * fFileButton;
        BButton       * fPictureButton;
        BButton       * fAdvancedButton;
        BStatusBar    * fStatusBar;
        BButton       * fRefreshButton;
        BButton       * fSuspendButton;
        BButton       * fStartButton;
};

#endif
