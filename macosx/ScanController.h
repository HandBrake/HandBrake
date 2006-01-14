/* $Id: ScanController.h,v 1.4 2005/03/21 12:37:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"

@interface ScanController : NSObject
{
    hb_handle_t                  * fHandle;
    
    IBOutlet NSWindow            * fWindow;
    IBOutlet NSPanel             * fPanel;
    IBOutlet NSTextField         * fSelectString;
    IBOutlet NSMatrix            * fMatrix;
    IBOutlet NSButtonCell        * fDetectedCell;
    IBOutlet NSPopUpButton       * fDetectedPopUp;
    IBOutlet NSButtonCell        * fFolderCell;
    IBOutlet NSTextField         * fFolderField;
    IBOutlet NSButton            * fBrowseButton;
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fIndicator;
    IBOutlet NSButton            * fCancelButton;
    IBOutlet NSButton            * fOpenButton;

    uint64_t                       fLastCheck;
}

- (void)     TranslateStrings;
- (void)     SetHandle:     (hb_handle_t *) handle;
- (void)     DetectDrives:  (NSNotification *) notification;
- (void)     UpdateUI:      (hb_state_t *) state;

- (IBAction) MatrixChanged: (id) sender;
- (IBAction) Browse:        (id) sender;
- (IBAction) Open:          (id) sender;
- (IBAction) Cancel:        (id) sender;

- (void) Browse2: (id) sender;
- (void) BrowseDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void) BrowseDone2: (id) sender;

@end
