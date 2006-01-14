/* $Id: PictureWin.h,v 1.3 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_PICTURE_WIN_H
#define HB_PICTURE_WIN_H

#include <View.h>
#include <Window.h>
class BSlider;
class BCheckBox;

/* libhb headers */
#include "Common.h"

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
        HBPictureWin( HBManager * manager, HBTitle * title );
        virtual void MessageReceived( BMessage * message );

        void UpdateBitmap( int which );


    private:
        HBManager     * fManager;
        HBTitle       * fTitle;

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
};

#endif
