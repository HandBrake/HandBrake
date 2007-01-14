#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <interface/Window.h>
#include <interface/View.h>

#include "hb.h"

class BButton;
class BCheckBox;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BSlider;
class BStatusBar;
class BStringView;
class BTextControl;
class BFilePanel;

class PicWindow;
class QueueWindow;

class MainView : public BView
{
    public:
        MainView( hb_handle_t * handle );

        void HandleMessage( BMessage * msg );
        void Update( hb_state_t * s );

    private:
        void EnableUI( bool );
        void TitlePopUpChanged();
        void ChapterPopUpChanged();
        void FormatPopUpChanged();
        void CodecsPopUpChanged();
        void CheckExtension();
        void QualityRadioChanged();
        void SliderChanged();
        void AddJob();

        hb_handle_t  * fHandle;

        BStringView  * fSrcDVD1String;
        BStringView  * fSrcDVD2String;
        BPopUpMenu   * fSrcTitlePopUp;
        BMenuField   * fSrcTitleMenu;
        int            fSrcTitle;
        BStringView  * fSrcChapString;
        BPopUpMenu   * fSrcChapStartPopUp;
        BMenuField   * fSrcChapStartMenu;
        BStringView  * fSrcChapToString;
        BPopUpMenu   * fSrcChapEndPopUp;
        BMenuField   * fSrcChapEndMenu;
        BStringView  * fSrcDur1String;
        BStringView  * fSrcDur2String;

        BPopUpMenu   * fDstFormatPopUp;
        BMenuField   * fDstFormatMenu;
        int            fDstFormat;
        BPopUpMenu   * fDstCodecsPopUp;
        BMenuField   * fDstCodecsMenu;
        BTextControl * fDstFileControl;
        BButton      * fBrowseButton;

        BPopUpMenu   * fVidRatePopUp;
        BMenuField   * fVidRateMenu;
        BPopUpMenu   * fVidEncoderPopUp;
        BMenuField   * fVidEncoderMenu;
        BStringView  * fVidQualityString;
        BRadioButton * fVidTargetRadio;
        BTextControl * fVidTargetControl;
        BRadioButton * fVidAverageRadio;
        BTextControl * fVidAverageControl;
        BRadioButton * fVidConstantRadio;
        BSlider      * fVidConstantSlider;
        BCheckBox    * fVidGrayCheck;
        BCheckBox    * fVidTwoPassCheck;

        BPopUpMenu   * fSubPopUp;
        BMenuField   * fSubMenu;

        BPopUpMenu   * fAudLang1PopUp;
        BMenuField   * fAudLang1Menu;
        BPopUpMenu   * fAudLang2PopUp;
        BMenuField   * fAudLang2Menu;
        BPopUpMenu   * fAudRatePopUp;
        BMenuField   * fAudRateMenu;
        BPopUpMenu   * fAudBitratePopUp;
        BMenuField   * fAudBitrateMenu;

        BButton      * fPictureButton;

        BStatusBar   * fProgressBar;
        BCheckBox    * fQueueCheck;
        BButton      * fAddButton;
        BButton      * fShowButton;
        BButton      * fPauseButton;
        BButton      * fRipButton;

        BFilePanel   * fFilePanel;
        PicWindow    * fPicWin;
        QueueWindow  * fQueueWin;
};

class MainWindow : public BWindow
{
    public:
        MainWindow( hb_handle_t * handle );
        void MessageReceived( BMessage * msg );
        bool QuitRequested();

        void Update( hb_state_t * s );

    private:
        MainView * fView;

        hb_handle_t * fHandle;
};

#endif
