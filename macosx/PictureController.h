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

    IBOutlet NSPanel         * fPicturePanel;

    IBOutlet HBPictureGLView * fPictureGLView;
    IBOutlet NSBox           * fPictureGLViewArea;
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
    IBOutlet NSPopUpButton   * fDeinterlacePopUp;
	IBOutlet NSButton        * fDetelecineCheck;
    IBOutlet NSButton        * fVFRCheck;
    IBOutlet NSButton        * fDeblockCheck;
	IBOutlet NSPopUpButton   * fDenoisePopUp;
	IBOutlet NSButton        * fPARCheck;
    IBOutlet NSButton        * fEffectsCheck;
    IBOutlet NSButton        * fPrevButton;
    IBOutlet NSButton        * fNextButton;
    IBOutlet NSTextField     * fInfoField;
	
    int     MaxOutputWidth;
    int     MaxOutputHeight;
    BOOL    autoCrop;
    
    /* used to track the previous state of the keep aspect
    ratio checkbox when turning anamorphic on, so it can be
    returned to the previous state when anamorphic is turned
    off */
    BOOL    keepAspectRatioPreviousState; 
    
    struct {
        int     detelecine;
        int     vfr;
        int     deinterlace;
        int     denoise;
        int     deblock;
    } fPictureFilterSettings;

    id delegate;
}
- (id)initWithDelegate:(id)del;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void) setInitialPictureFilters;
- (void) Display: (int) anim;

- (IBAction) SettingsChanged: (id) sender;
- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) ClosePanel: (id) sender;

- (BOOL) autoCrop;
- (void) setAutoCrop: (BOOL) setting;

- (int) detelecine;
- (void) setDetelecine: (int) setting;
- (int) vfr;
- (void) setVFR: (int) setting;
- (int) deinterlace;
- (void) setDeinterlace: (int) setting;
- (int) denoise;
- (void) setDenoise: (int) setting;
- (int) deblock;
- (void) setDeblock: (int) setting;

- (void)showPanelInWindow: (NSWindow *)fWindow forTitle: (hb_title_t *)title;
- (BOOL) loadMyNibFile;
@end

@interface NSObject (PictureControllertDelegateMethod)
- (void)pictureSettingsDidChange;
@end
