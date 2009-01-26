/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h" 

@class HBController;
@class PreviewController;


//#define HB_NUM_HBLIB_PICTURES      20   // # of preview pictures libhb should generate

@interface PictureFilterController : NSWindowController
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             *fHBController;
    PreviewController        *fPreviewController;        // reference to HBController
    IBOutlet NSWindow        * fFilterWindow;

    IBOutlet NSBox           * fPictureFilterBox;

    IBOutlet NSPopUpButton   * fDeinterlacePopUp;
    IBOutlet NSButton        * fDecombCheck;
	IBOutlet NSButton        * fDetelecineCheck;
    IBOutlet NSButton        * fDeblockCheck;
    IBOutlet NSTextField     * fDeblockField;
    IBOutlet NSSlider        * fDeblockSlider;
	IBOutlet NSPopUpButton   * fDenoisePopUp;
	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSSlider        * fPictureSlider;
    IBOutlet NSTextField     * fInfoField;
	
    IBOutlet NSButton        * fPreviewOpenButton;
    IBOutlet NSButton        * fPictureSizeOpenButton;
        
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
        int     deinterlace;
        int     decomb;
        int     denoise;
        int     deblock;
    } fPictureFilterSettings;


}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void)setHBController: (HBController *)controller;
- (IBAction) showFilterWindow: (id)sender;
- (IBAction) showPreviewWindow: (id)sender;

- (void) setInitialPictureFilters;


- (IBAction) FilterSettingsChanged: (id) sender;



- (BOOL) autoCrop;
- (void) setAutoCrop: (BOOL) setting;

- (BOOL) allowLooseAnamorphic;
- (void) setAllowLooseAnamorphic: (BOOL) setting;
- (IBAction) deblockSliderChanged: (id) sender;
- (int) detelecine;
- (void) setDetelecine: (int) setting;
- (int) deinterlace;
- (void) setDeinterlace: (int) setting;
- (int) decomb;
- (void) setDecomb: (int) setting;
- (int) denoise;
- (void) setDenoise: (int) setting;
- (int) deblock;
- (void) setDeblock: (int) setting;

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title;
- (IBAction) showPictureSettingsWindow: (id)sender;

- (void) setToFullScreenMode;
- (void) setToWindowedMode;


@end

