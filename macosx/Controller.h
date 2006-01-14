/* $Id: Controller.h,v 1.14 2004/02/13 13:45:51 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "HandBrake.h"
#include "PictureGLView.h"
#include "TargetSizeField.h"

@interface HBController : NSObject

{
    IBOutlet NSWindow            * fWindow;

    /* Scan view */
    IBOutlet NSView              * fScView;
    IBOutlet NSTextField         * fScWelcomeField;
    IBOutlet NSTextField         * fScSelectField;
    IBOutlet NSMatrix            * fScMatrix;
    IBOutlet NSButtonCell        * fScDetectedCell;
    IBOutlet NSPopUpButton       * fScDetectedPopUp;
    IBOutlet NSButtonCell        * fScFolderCell;
    IBOutlet NSTextField         * fScFolderField;
    IBOutlet NSButton            * fScBrowseButton;
    IBOutlet NSTextField         * fScStatusField;
    IBOutlet NSProgressIndicator * fScProgress;
    IBOutlet NSButton            * fScOpenButton;

    IBOutlet NSView              * fTempView;

    /* Rip view */
    IBOutlet NSView              * fRipView;

    /* General box */
    IBOutlet NSTextField         * fRipGeneralField;
    IBOutlet NSTextField         * fRipTitleField;
    IBOutlet NSPopUpButton       * fRipTitlePopUp;
    IBOutlet NSTextField         * fRipFormatField;
    IBOutlet NSPopUpButton       * fRipFormatPopUp;
    IBOutlet NSTextField         * fRipFileField1;
    IBOutlet NSTextField         * fRipFileField2;
    IBOutlet NSButton            * fRipBrowseButton;

    /* Video box */
    IBOutlet NSTextField         * fRipVideoField;
    IBOutlet NSTextField         * fRipEncoderField;
    IBOutlet NSPopUpButton       * fRipEncoderPopUp;
    IBOutlet NSTextField         * fRipBitrateField;
    IBOutlet NSMatrix            * fRipVideoMatrix;
    IBOutlet NSButtonCell        * fRipCustomCell;
    IBOutlet NSTextField         * fRipCustomField;
    IBOutlet NSButtonCell        * fRipTargetCell;
    IBOutlet HBTargetSizeField   * fRipTargetField;
    IBOutlet NSButton            * fRipTwoPassCheck;
    IBOutlet NSButton            * fRipCropButton;

    /* Audio box */
    IBOutlet NSTextField         * fRipAudioField;
    IBOutlet NSTextField         * fRipLang1Field;
    IBOutlet NSPopUpButton       * fRipLang1PopUp;
    IBOutlet NSTextField         * fRipLang2Field;
    IBOutlet NSPopUpButton       * fRipLang2PopUp;
    IBOutlet NSTextField         * fRipAudBitField;
    IBOutlet NSPopUpButton       * fRipAudBitPopUp;

    /* Bottom */
    IBOutlet NSTextField         * fRipStatusField;
    IBOutlet NSTextField         * fRipInfoField;
    IBOutlet NSProgressIndicator * fRipProgress;
    IBOutlet NSButton            * fRipPauseButton;
    IBOutlet NSButton            * fRipRipButton;

    /* "Done" alert panel */
    IBOutlet NSPanel             * fDonePanel;

    /* Crop & scale panel */
    IBOutlet NSPanel             * fPicturePanel;
    IBOutlet HBPictureGLView     * fPictureGLView;
    IBOutlet NSTextField         * fWidthField1;
    IBOutlet NSTextField         * fWidthField2;
    IBOutlet NSStepper           * fWidthStepper;
    IBOutlet NSButton            * fDeinterlaceCheck;
    IBOutlet NSTextField         * fTopField1;
    IBOutlet NSTextField         * fTopField2;
    IBOutlet NSStepper           * fTopStepper;
    IBOutlet NSTextField         * fBottomField1;
    IBOutlet NSTextField         * fBottomField2;
    IBOutlet NSStepper           * fBottomStepper;
    IBOutlet NSTextField         * fLeftField1;
    IBOutlet NSTextField         * fLeftField2;
    IBOutlet NSStepper           * fLeftStepper;
    IBOutlet NSTextField         * fRightField1;
    IBOutlet NSTextField         * fRightField2;
    IBOutlet NSStepper           * fRightStepper;
    IBOutlet NSButton            * fPreviousButton;
    IBOutlet NSButton            * fNextButton;
    IBOutlet NSButton            * fAutocropButton;
    IBOutlet NSButton            * fOpenGLCheck;
    IBOutlet NSTextField         * fInfoField;
    IBOutlet NSButton            * fCloseButton;
    int                            fPicture;

    HBHandle                     * fHandle;
    int                            fTitle;
    int                            fTitleCount;
    HBList                       * fTitleList;
    float                          fPosition;
    int                            fPass;
    int                            fPassCount;
    float                          fCurFrameRate;
    float                          fAvgFrameRate;
    int                            fRemainingTime;
    int                            fResult;
}

- (IBAction) ScanMatrixChanged: (id) sender;
- (IBAction) BrowseDVD: (id) sender;
- (void)     BrowseDVDDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) Scan: (id) sender;

- (IBAction) TitlePopUpChanged: (id) sender;
- (IBAction) FormatPopUpChanged: (id) sender;
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
- (void)     _Cancel: (NSWindow *) sheet returnCode: (int) returnCode
    contextInfo: (void *) contextInfo;
- (IBAction) Pause: (id) sender;
- (IBAction) Resume: (id) sender;

- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) UpdatePicture: (id) sender;
- (IBAction) AutoCrop: (id) sender;

- (void)     DetectDrives: (NSNotification *) notification;

/* libhb callbacks */
- (void) Scanning: (id) sender;
- (void) ScanDone: (id) sender;
- (void) Encoding: (id) sender;
- (void) RipDone:  (id) sender;

@end

