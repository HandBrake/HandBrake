/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h" 

@class HBController;
@class PreviewController;



//#define HB_NUM_HBLIB_PICTURES      20   // # of preview pictures libhb should generate

@interface PictureController : NSWindowController
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             *fHBController;
    PreviewController        *fPreviewController;        // reference to HBController
    
    IBOutlet NSWindow        * fPictureWindow;
    
    IBOutlet NSTabView       * fSizeFilterView;
    
    /* Picture Sizing */
    
    NSMutableDictionary      * fPicturePreviews;        // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;


    IBOutlet NSBox           * fPictureSizeBox;
    IBOutlet NSBox           * fPictureCropBox;
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

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSTextField     * fSizeInfoField;
	
    IBOutlet NSButton        * fPreviewOpenButton;
    IBOutlet NSButton        * fPictureFiltersOpenButton;
        
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
    
    
    /* Video Filters */
    
    IBOutlet NSBox           * fPictureFilterBox;

    IBOutlet NSBox           * fDetelecineBox;
    IBOutlet NSPopUpButton   * fDetelecinePopUp;
    IBOutlet NSTextField     * fDetelecineField;
    
    IBOutlet NSBox           * fDecombDeinterlaceBox;
    IBOutlet NSSlider        * fDecombDeinterlaceSlider;
    
    IBOutlet NSBox           * fDecombBox;
    IBOutlet NSPopUpButton   * fDecombPopUp;
    IBOutlet NSTextField     * fDecombField;
    
    IBOutlet NSBox           * fDeinterlaceBox;
    IBOutlet NSPopUpButton   * fDeinterlacePopUp;
    IBOutlet NSTextField     * fDeinterlaceField;

    IBOutlet NSBox           * fDenoiseBox;
    IBOutlet NSPopUpButton   * fDenoisePopUp;
    IBOutlet NSTextField     * fDenoiseField;
	
    IBOutlet NSButton        * fDeblockCheck;
    IBOutlet NSTextField     * fDeblockField;
    IBOutlet NSSlider        * fDeblockSlider;
    
    IBOutlet NSButton        * fGrayscaleCheck;

    IBOutlet NSTextField     * fInfoField;
	

    
        struct {
        int     detelecine;
        int     deinterlace;
        int     decomb;
        int     usedecomb;
        int     denoise;
        int     deblock;
        int     grayscale;
    } fPictureFilterSettings;
    
}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void)setHBController: (HBController *)controller;
- (IBAction) showPictureWindow: (id)sender;
- (IBAction) showPreviewWindow: (id)sender;
- (BOOL) previewFullScreenMode;
- (IBAction) previewGoWindowed: (id)sender;




- (IBAction) SettingsChanged: (id) sender;

- (NSString*) getPictureSizeInfoString;
- (void)reloadStillPreview;

- (BOOL) autoCrop;
- (void) setAutoCrop: (BOOL) setting;

- (BOOL) allowLooseAnamorphic;
- (void) setAllowLooseAnamorphic: (BOOL) setting;

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title;


- (void) setToFullScreenMode;
- (void) setToWindowedMode;

/* Filter Actions */
- (void) setInitialPictureFilters;
- (IBAction) FilterSettingsChanged: (id) sender;
- (void) adjustFilterDisplay: (id) sender;
- (IBAction) modeDecombDeinterlaceSliderChanged: (id) sender;
- (IBAction) deblockSliderChanged: (id) sender;

- (int) detelecine;
- (NSString*) detelecineCustomString;
- (void) setDetelecine: (int) setting;
- (void) setDetelecineCustomString: (NSString*) string;

- (int) useDecomb;
- (void) setUseDecomb: (int) setting;

- (int) decomb;
- (NSString*) decombCustomString;
- (void) setDecomb: (int) setting;
- (void) setDecombCustomString: (NSString*) string;

- (int) deinterlace;
- (NSString*) deinterlaceCustomString;
- (void) setDeinterlace: (int) setting;
- (void) setDeinterlaceCustomString: (NSString*) string; 

- (int) denoise;
- (NSString*) denoiseCustomString;
- (void) setDenoise: (int) setting;
- (void) setDenoiseCustomString: (NSString*) string;

- (int) deblock;
- (void) setDeblock: (int) setting;

- (int) grayscale;
- (void) setGrayscale: (int) setting;



@end

