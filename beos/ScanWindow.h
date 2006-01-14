#ifndef SCANWINDOW_H
#define SCANWINDOW_H

#include <interface/Window.h>
#include <interface/View.h>

#include "hb.h"

class BButton;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BStatusBar;
class BTextControl;

class ScanView : public BView
{
    public:
        ScanView( hb_handle_t * handle );

        void HandleMessage( BMessage * msg );
        void Update( hb_state_t * s );
        void RadioChanged();
        void SetEnabled( bool );

    private:
        void DetectVolumes();

        hb_handle_t  * fHandle;

        BRadioButton * fDetectedRadio;
        BPopUpMenu   * fPopUp;
        BMenuField   * fMenu;
        BRadioButton * fFolderRadio;
        BTextControl * fControl;
        BButton      * fBrowseButton;
        BStatusBar   * fBar;
        BButton      * fCancelButton;
        BButton      * fOpenButton;

        BFilePanel  * fFilePanel;
};

class ScanWindow : public BWindow
{
    public:
        ScanWindow( hb_handle_t * handle );
        void MessageReceived( BMessage * msg );

        void Update( hb_state_t * s );

    private:
        ScanView    * fView;
};

#endif
