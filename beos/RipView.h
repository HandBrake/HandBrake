/* $Id: RipView.h,v 1.4 2003/11/09 21:35:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#ifndef HB_RIP_VIEW_H
#define HB_RIP_VIEW_H

#include <View.h>
class BBox;
class BButton;
class BCheckBox;
class BFilePanel;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BStatusBar;
class BTextControl;

#include "HandBrake.h"

#define RIP_TITLE_POPUP       'rtip'
#define RIP_BITRATE_RADIO     'rbir'
#define RIP_TARGET_CONTROL    'rtac'
#define RIP_CROP_BUTTON       'rcrb'
#define RIP_BROWSE_BUTTON     'rbrb'
#define RIP_SUSPEND_BUTTON    'rsub'
#define RIP_RIP_BUTTON        'rrib'

class RipView : public BView
{
    public:
                       RipView( HBHandle * handle );
        void           MessageReceived( BMessage * message );
        void           UpdateIntf( HBStatus status, int modeChanged );

    private:
        HBHandle     * fHandle;
        HBList       * fTitleList;
        
        BBox         * fVideoBox;
        BPopUpMenu   * fTitlePopUp;
        BMenuField   * fTitleField;
        BPopUpMenu   * fVideoCodecPopUp;
        BMenuField   * fVideoCodecField;
        BRadioButton * fCustomBitrateRadio;
        BTextControl * fCustomBitrateControl;
        BRadioButton * fTargetSizeRadio;
        BTextControl * fTargetSizeControl;
        BCheckBox    * fTwoPassCheck;
        BButton      * fCropButton;

        BBox         * fAudioBox;
        BPopUpMenu   * fLanguagePopUp;
        BMenuField   * fLanguageField;
        BPopUpMenu   * fSecondaryLanguagePopUp;
        BMenuField   * fSecondaryLanguageField;
        BPopUpMenu   * fAudioCodecPopUp;
        BMenuField   * fAudioCodecField;
        BPopUpMenu   * fAudioBitratePopUp;
        BMenuField   * fAudioBitrateField;

        BBox         * fDestinationBox;
        BPopUpMenu   * fFileFormatPopUp;
        BMenuField   * fFileFormatField;
        BTextControl * fFileControl;
        BButton      * fFileButton;
        BFilePanel   * fFilePanel;

        BStatusBar   * fStatusBar;
        BButton      * fSuspendButton;
        BButton      * fStartButton;
};

#endif
