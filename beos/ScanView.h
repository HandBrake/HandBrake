/* $Id: ScanView.h,v 1.2 2003/10/13 22:23:02 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
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

#include "Common.h"

#define SCAN_RADIO         'scra'
#define SCAN_BROWSE_BUTTON 'sbrb'
#define SCAN_OPEN          'scop'

class ScanView : public BView
{
    public:
                       ScanView( HBManager * manager );
        void           MessageReceived( BMessage * message );
        void           UpdateIntf( HBStatus status );

    private:
        void           DetectVolumes();

        HBManager    * fManager;
        
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
