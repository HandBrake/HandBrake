/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"

#define HB_NUM_HBLIB_PICTURES      10   // hbilb generates 10 preview pictures

@interface PictureController : NSObject
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    NSMutableDictionary      * fPicturePreviews;        // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;

    IBOutlet NSPanel         * fPicturePanel;

    IBOutlet NSImageView     * fPictureView;
    IBOutlet NSBox           * fPictureViewArea;
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
	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSButton        * fPrevButton;
    IBOutlet NSButton        * fNextButton;
    IBOutlet NSTextField     * fInfoField;
	
    int     MaxOutputWidth;
    int     MaxOutputHeight;
    BOOL    autoCrop;
    BOOL    allowLooseAnamorphic;
    int output_width, output_height, output_par_width, output_par_height;
    int display_width;
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
- (void) displayPreview;

- (IBAction) SettingsChanged: (id) sender;
- (IBAction) PreviousPicture: (id) sender;
- (IBAction) NextPicture: (id) sender;
- (IBAction) ClosePanel: (id) sender;

- (BOOL) autoCrop;
- (void) setAutoCrop: (BOOL) setting;

- (BOOL) allowLooseAnamorphic;
- (void) setAllowLooseAnamorphic: (BOOL) setting;

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

+ (NSImage *) makeImageForPicture: (int)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title
                removeBorders:(BOOL)removeBorders;
- (NSImage *) imageForPicture: (int) pictureIndex;
- (void) purgeImageCache;
@end

@interface NSObject (PictureControllertDelegateMethod)
- (void)pictureSettingsDidChange;
@end
