/* $Id: ScanView.h,v 1.2 2003/11/06 14:36:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_SCAN_VIEW_H
#define HB_SCAN_VIEW_H

#include <View.h>
class BButton;
class BFilePanel;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BStringView;
class BTextControl;

#include "HandBrake.h"

#define SCAN_RADIO         'scra'
#define SCAN_BROWSE_BUTTON 'sbrb'
#define SCAN_OPEN          'scop'

class ScanView : public BView
{
    public:
                       ScanView( HBHandle * handle );
        void           MessageReceived( BMessage * message );
        void           UpdateIntf( HBStatus status, int modeChanged );

    private:
        void           DetectVolumes();

        HBHandle     * fHandle;
        
        BRadioButton * fRadioDetected;
        BRadioButton * fRadioFolder;
        BMenuField   * fField;
        BPopUpMenu   * fPopUp;
        BTextControl * fFolderControl;
        BButton      * fBrowseButton;
        BFilePanel   * fFilePanel;
        BStringView  * fStatusString;
        BButton      * fOpenButton;
};

#endif
