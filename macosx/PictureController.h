/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h"
/* Needed for Quicktime movie previews */
#import <QTKit/QTKit.h> 

@class HBController;

#define HB_NUM_HBLIB_PICTURES      20   // # of preview pictures libhb should generate

@interface PictureController : NSWindowController
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             *fHBController;        // reference to HBController
    IBOutlet NSWindow        * fPictureWindow;
    NSMutableDictionary      * fPicturePreviews;        // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;

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
    IBOutlet NSButton        * fDecombCheck;
	IBOutlet NSButton        * fDetelecineCheck;
    IBOutlet NSButton        * fDeblockCheck;
    IBOutlet NSTextField     * fDeblockField;
    IBOutlet NSSlider        * fDeblockSlider;
	IBOutlet NSPopUpButton   * fDenoisePopUp;
	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSSlider        * fPictureSlider;
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
        int     deinterlace;
        int     decomb;
        int     denoise;
        int     deblock;
    } fPictureFilterSettings;

    id delegate;
    
    /* Movie Previews */
    IBOutlet NSButton               * fCreatePreviewMovieButton;
    IBOutlet NSButton               * fShowPreviewMovieButton;
    NSString                        * fPreviewMoviePath;
    IBOutlet NSProgressIndicator    * fMovieCreationProgressIndicator;
    hb_handle_t                     * fPreviewLibhb;           // private libhb for creating previews
    NSTimer                         * fLibhbTimer;             // timer for retrieving state from libhb
    IBOutlet NSTextField            * fPreviewMovieStatusField;
    BOOL                              play_movie; // flag used to determine whether or not to automatically play the movie when done.   
    IBOutlet QTMovieView            * fMovieView;
    IBOutlet NSPopUpButton          * fPreviewMovieLengthPopUp; // popup of choices for length of preview in seconds
}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void)setHBController: (HBController *)controller;
- (IBAction) showPictureWindow: (id)sender;

- (void) setInitialPictureFilters;
- (void) displayPreview;

- (IBAction) SettingsChanged: (id) sender;
- (IBAction) pictureSliderChanged: (id) sender;

/* Movie Previews */
- (void) startReceivingLibhbNotifications;
- (void) stopReceivingLibhbNotifications;

- (IBAction) createMoviePreview: (id) sender;
- (void) libhbStateChanged: (hb_state_t &) state;
- (IBAction) showMoviePreview: (NSString *) path;
- (IBAction) previewDurationPopUpChanged: (id) sender;

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

+ (NSImage *) makeImageForPicture: (int)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title
                removeBorders:(BOOL)removeBorders;
- (NSImage *) imageForPicture: (int) pictureIndex;
- (void) purgeImageCache;
@end

