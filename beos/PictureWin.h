/* $Id: PictureWin.h,v 1.1.1.1 2003/11/03 12:03:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_PICTURE_WIN_H
#define HB_PICTURE_WIN_H

#include <View.h>
#include <Window.h>
class BSlider;
class BCheckBox;

#include "HandBrake.h"

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
        HBPictureWin( HBHandle * handle, HBTitle * title );
        virtual void MessageReceived( BMessage * message );

        void UpdateBitmap( int which );


    private:
        HBHandle      * fHandle;
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
