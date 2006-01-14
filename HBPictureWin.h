/* $Id: HBPictureWin.h,v 1.6 2003/08/20 20:30:30 titer Exp $ */

#ifndef _HB_PICTURE_WIN_H
#define _HB_PICTURE_WIN_H

class HBTitleInfo;

#include <View.h>
#include <Window.h>
class BSlider;
class BCheckBox;

class HBPictureView : public BView
{
    public:
        HBPictureView::HBPictureView( BRect rect, BBitmap * bitmap );
        virtual void Draw( BRect rect );
    
    private:
        BBitmap * fBitmap;
};

class HBPictureWin : public BWindow
{
    public:
        HBPictureWin( HBTitleInfo * titleInfo );
        virtual bool QuitRequested();
        virtual void MessageReceived( BMessage * message );
        
        void UpdateBitmap( int which );
        
    
    private:
        HBTitleInfo   * fTitleInfo;

        /* GUI */
        HBPictureView * fPictureView;
        BSlider       * fPictureSlider;
        BBitmap       * fBitmap;
        BSlider       * fWidthSlider;
        BSlider       * fTopCropSlider;
        BSlider       * fBottomCropSlider;
        BSlider       * fLeftCropSlider;
        BSlider       * fRightCropSlider;
        BCheckBox     * fDeinterlaceCheck;

        /* Internal infos */
        uint32_t        fMaxOutWidth;
        uint32_t        fMaxOutHeight;
};

#endif
