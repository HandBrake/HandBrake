/* $Id: HBController.h,v 1.19 2003/10/06 21:13:45 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "Manager.h"
#include "PictureGLView.h"

@interface HBController : NSObject

{
    IBOutlet NSWindow            * fWindow;

    IBOutlet NSView              * fScanView;
    IBOutlet NSMatrix            * fScanMatrix;
    IBOutlet NSPopUpButton       * fDVDPopUp;
    IBOutlet NSTextField         * fDVDFolderField;
    IBOutlet NSButton            * fScanBrowseButton;
    IBOutlet NSTextField         * fScanStatusField;
    IBOutlet NSProgressIndicator * fScanProgress;
    IBOutlet NSButton            * fScanButton;

    IBOutlet NSView              * fRipView;
    IBOutlet NSPopUpButton       * fTitlePopUp;
    IBOutlet NSPopUpButton       * fAudioPopUp;
    IBOutlet NSTextField         * fVideoField;
    IBOutlet NSStepper           * fVideoStepper;
    IBOutlet NSTextField         * fAudioField;
    IBOutlet NSStepper           * fAudioStepper;
    IBOutlet NSButton            * fTwoPassCheck;
    IBOutlet NSButton            * fCropButton;
    IBOutlet NSTextField         * fFileField;
    IBOutlet NSButton            * fRipBrowseButton;
    IBOutlet NSTextField         * fRipStatusField;
    IBOutlet NSProgressIndicator * fRipProgress;
    IBOutlet NSButton            * fSuspendButton;
    IBOutlet NSButton            * fRipButton;
    IBOutlet NSPanel             * fDonePanel;

    IBOutlet NSPanel             * fPicturePanel;
    IBOutlet PictureGLView       * fPictureGLView;
    IBOutlet NSTextField         * fWidthField;
    IBOutlet NSStepper           * fWidthStepper;
    IBOutlet NSButton            * fDeinterlaceCheck;
    IBOutlet NSTextField         * fTopField;
    IBOutlet NSStepper           * fTopStepper;
    IBOutlet NSTextField         * fBottomField;
    IBOutlet NSStepper           * fBottomStepper;
    IBOutlet NSTextField         * fLeftField;
    IBOutlet NSStepper           * fLeftStepper;
    IBOutlet NSTextField         * fRightField;
    IBOutlet NSStepper           * fRightStepper;
    IBOutlet NSTextField         * fInfoField;
    int                            fPicture;

    IBOutlet NSView              * fBlankView;
    HBManager                    * fManager;
    HBList                       * fTitleList;
}

- (IBAction) BrowseDVD: (id) sender;
- (void)     BrowseDVDDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) Scan: (id) sender;

- (IBAction) BrowseFile: (id) sender;
- (void)     BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) ShowPicturePanel: (id) sender;
- (IBAction) ClosePanel: (id) sender;
- (IBAction) Rip: (id) sender;
- (IBAction) Cancel: (id) sender;
- (IBAction) Suspend: (id) sender;
- (IBAction) Resume: (id) sender;

- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) UpdatePicture: (id) sender;

- (void)     UpdateIntf: (NSTimer *) timer;
- (void)     DetectDrives;
- (void)     ScanEnableIntf: (id) sender;
- (void)     UpdatePopUp: (id) sender;

@end
