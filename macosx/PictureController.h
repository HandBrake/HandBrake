/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"
#include "PictureGLView.h"

@interface PictureController : NSObject
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    bool                       fHasQE;
    uint8_t                  * fBuffer;
    int                        fBufferSize;
    uint8_t                  * fTexBuf[2];
    int                        fTexBufSize;
    int                        fPicture;

    IBOutlet HBPictureGLView * fPictureGLView;
    IBOutlet NSTextField     * fWidthField;
    IBOutlet NSStepper       * fWidthStepper;
    IBOutlet NSTextField     * fHeightField;
    IBOutlet NSStepper       * fHeightStepper;
    IBOutlet NSButton        * fRatioCheck;
    IBOutlet NSMatrix        * fCropMatrix;
    IBOutlet NSTextField     * fCropTopField;
    IBOutlet NSStepper       * fCropTopStepper;
    IBOutlet NSTextField     * fCropBottomField;
    IBOutlet NSStepper       * fCropBottomStepper;
    IBOutlet NSTextField     * fCropLeftField;
    IBOutlet NSStepper       * fCropLeftStepper;
    IBOutlet NSTextField     * fCropRightField;
    IBOutlet NSStepper       * fCropRightStepper;
    IBOutlet NSButton        * fDeinterlaceCheck;
    IBOutlet NSButton        * fEffectsCheck;
    IBOutlet NSButton        * fPrevButton;
    IBOutlet NSButton        * fNextButton;
    IBOutlet NSTextField     * fInfoField;
}

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void) Display: (int) anim;

- (IBAction) SettingsChanged: (id) sender;
- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) ClosePanel: (id) sender;

@end
