#include "layout-all.h"
#include "HandBrake.h"

class HBWindow : public MWindow
{
    public:
        HBWindow();
        bool QuitRequested();
        void MessageReceived( BMessage * message );

        void Scanning( int title, int titleCount );
        void ScanDone( HBList * titleList );
        void Encoding( float position, int pass, int passCount,
                       float frameRate, float avgFrameRate,
                       int remainingTime );
        void RipDone( int result );

    private:
        void UpdateLanguages();
        
        HBHandle * fHandle;
        HBList   * fTitleList;

        LayeredGroup * fLayers;
        
        MView        * fScanView;
        MRadioGroup  * fScanRadio;
        MPopup       * fScanDetectedPopup;
        MTextControl * fScanFolderControl;
        MButton      * fScanBrowseButton;
        MStringView  * fScanStatusString;
        MProgressBar * fScanProgress;
        MButton      * fScanOpenButton;

        MView * fRipView;
        MPopup * fRipTitlePopup;
        MPopup * fRipLanguage1Popup;
        MPopup * fRipLanguage2Popup;
        MPopup * fRipBitratePopup;
};

