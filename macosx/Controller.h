/* $Id: Controller.h,v 1.6 2003/10/13 23:09:56 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "Common.h"
#include "PictureGLView.h"
#include "TargetSizeField.h"

@interface HBController : NSObject

{
    bool                           fDie;
    
    IBOutlet NSWindow            * fWindow;

    /* Scan view */
    uint64_t                       fLastDVDDetection;
    IBOutlet NSView              * fScanView;
    IBOutlet NSMatrix            * fScanMatrix;
    IBOutlet NSPopUpButton       * fDVDPopUp;
    IBOutlet NSTextField         * fDVDFolderField;
    IBOutlet NSButton            * fScanBrowseButton;
    IBOutlet NSTextField         * fScanStatusField;
    IBOutlet NSProgressIndicator * fScanProgress;
    IBOutlet NSButton            * fScanButton;

    IBOutlet NSView              * fTempView;

    /* Rip view */
    IBOutlet NSView              * fRipView;

    /* Video box */
    IBOutlet NSPopUpButton       * fTitlePopUp;
    IBOutlet NSPopUpButton       * fVideoCodecPopUp;
    IBOutlet NSMatrix            * fVideoMatrix;
    IBOutlet NSTextField         * fCustomBitrateField;
    IBOutlet HBTargetSizeField   * fTargetSizeField;
    IBOutlet NSButton            * fTwoPassCheck;
    IBOutlet NSButton            * fCropButton;

    /* Audio box */
    IBOutlet NSPopUpButton       * fLanguagePopUp;
    IBOutlet NSPopUpButton       * fSecondaryLanguagePopUp;
    IBOutlet NSPopUpButton       * fAudioCodecPopUp;
    IBOutlet NSPopUpButton       * fAudioBitratePopUp;

    /* Destination box */
    IBOutlet NSPopUpButton       * fFileFormatPopUp;
    IBOutlet NSTextField         * fFileField;
    IBOutlet NSButton            * fFileBrowseButton;

    /* Bottom */
    IBOutlet NSTextField         * fRipStatusField;
    IBOutlet NSTextField         * fRipInfoField;
    IBOutlet NSProgressIndicator * fRipProgress;
    IBOutlet NSButton            * fSuspendButton;
    IBOutlet NSButton            * fRipButton;

    /* "Done" alert panel */
    IBOutlet NSPanel             * fDonePanel;

    /* Crop & resize panel */
    IBOutlet NSPanel             * fPicturePanel;
    IBOutlet HBPictureGLView     * fPictureGLView;
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

    HBManager                    * fManager;
    HBList                       * fTitleList;
}

- (IBAction) ScanMatrixChanged: (id) sender;
- (IBAction) BrowseDVD: (id) sender;
- (void)     BrowseDVDDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) Scan: (id) sender;

- (IBAction) TitlePopUpChanged: (id) sender;
- (IBAction) VideoMatrixChanged: (id) sender;
- (IBAction) AudioPopUpChanged: (id) sender;
- (IBAction) BrowseFile: (id) sender;
- (void)     BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) ShowPicturePanel: (id) sender;
- (IBAction) ClosePanel: (id) sender;
- (IBAction) Rip: (id) sender;
- (void)     OverwriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     _Rip;
- (IBAction) Cancel: (id) sender;
- (IBAction) Suspend: (id) sender;
- (IBAction) Resume: (id) sender;

- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) UpdatePicture: (id) sender;

- (void)     UpdateIntf: (NSTimer *) timer;
- (void)     DetectDrives;

@end
