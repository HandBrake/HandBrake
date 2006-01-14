#ifndef PICWINDOW_H
#define PICWINDOW_H

#include <interface/Window.h>
#include <interface/View.h>

#include "hb.h"

class HBStepper;

class PicView : public BView
{
    public:
        PicView( hb_handle_t * handle, int index );
        ~PicView();
        void Draw( BRect rect );

        void HandleMessage( BMessage * msg );

    private:
        void UpdateBitmap();
        void RadioChanged();
        void UpdateSettings( uint32 what );

        hb_handle_t  * fHandle;
        hb_title_t   * fTitle;
        hb_job_t     * fJob;
        int            fIndex;
        uint8_t      * fRawPic;
        BBitmap      * fBitmap;

        HBStepper    * fWidthStepper;
        HBStepper    * fHeightStepper;
        BCheckBox    * fRatioCheck;
        BRadioButton * fAutoRadio;
        BRadioButton * fCustomRadio;
        HBStepper    * fCropSteppers[4];
        BCheckBox    * fDeintCheck;
        BButton      * fPrevButton;
        BButton      * fNextButton;
        BStringView  * fInfoString;
};

class PicWindow : public BWindow
{
    public:
        PicWindow( hb_handle_t * handle, int index );
        void MessageReceived( BMessage * msg );

    private:
        PicView * fView;
};

#endif
