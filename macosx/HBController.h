/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBQueueController;

@class HBPictureViewController;
@class HBVideoController;
@class HBAudioController;
@class HBSubtitlesController;
@class HBAdvancedController;
@class HBChapterTitlesController;

@class HBPreviewController;

@class HBPreferencesController;
@class HBPresetsViewController;
@class HBPresetsManager;

@class HBJob;

@interface HBController : NSWindowController <NSDrawerDelegate>
{
    IBOutlet NSTabView *fMainTabView;

    // Picture controller
    HBPictureViewController * fPictureViewController;
    IBOutlet NSTabViewItem  * fPictureTab;

    // Video view controller
    HBVideoController       * fVideoController;
    IBOutlet NSTabViewItem  * fVideoTab;

    // Subtitles view controller
	HBSubtitlesController   * fSubtitlesViewController;
    IBOutlet NSTabViewItem  * fSubtitlesTab;

	// Audio view controller
	HBAudioController       * fAudioController;
    IBOutlet NSTabViewItem  * fAudioTab;

	// Chapters view controller
	HBChapterTitlesController    * fChapterTitlesController;
    IBOutlet NSTabViewItem       * fChaptersTitlesTab;

    // Advanced options tab
    HBAdvancedController         * fAdvancedOptions;
	IBOutlet NSTabViewItem       * fAdvancedTab;

    // Picture Preview
    HBPreviewController           * fPreviewController;
    HBPreferencesController       * fPreferencesController;
    
    // Queue panel
    HBQueueController            * fQueueController;
	
    // Source box
	IBOutlet NSProgressIndicator * fScanIndicator;
	IBOutlet NSBox               * fScanHorizontalLine;
    
    IBOutlet NSTextField         * fSrcDVD2Field;
    IBOutlet NSPopUpButton       * fSrcTitlePopUp;
    
    // pts based start / stop
    IBOutlet NSTextField         * fSrcTimeStartEncodingField;
    IBOutlet NSTextField         * fSrcTimeEndEncodingField;
    // frame based based start / stop
    IBOutlet NSTextField         * fSrcFrameStartEncodingField;
    IBOutlet NSTextField         * fSrcFrameEndEncodingField;

    IBOutlet NSPopUpButton       * fSrcChapterStartPopUp;
    IBOutlet NSPopUpButton       * fSrcChapterEndPopUp;

    // Bottom
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSTextField         * fQueueStatus;
    IBOutlet NSProgressIndicator * fRipIndicator;
	BOOL                           fRipIndicatorShown;

	// User Preset
	HBPresetsManager             * presetManager;
    HBPresetsViewController      * fPresetsView;

	IBOutlet NSDrawer            * fPresetDrawer;
}

- (instancetype)initWithQueue:(HBQueueController *)queueController presetsManager:(HBPresetsManager *)manager;

- (void)launchAction;

- (BOOL)openURL:(NSURL *)fileURL;
- (BOOL)openJob:(HBJob *)job;

- (IBAction)browseSources:(id)sender;

- (IBAction)showPreviewWindow:(id)sender;

// Queue
- (IBAction)addToQueue:(id)sender;
- (IBAction)addAllTitlesToQueue:(id)sender;

- (void)setQueueState:(NSString *)info;
- (void)setQueueInfo:(NSString *)info progress:(double)progress hidden:(BOOL)hidden;

- (IBAction)rip:(id)sender;
- (IBAction)pause:(id)sender;

- (IBAction)selectPresetFromMenu:(id)sender;

// Manage User presets
- (IBAction)showAddPresetPanel:(id)sender;
- (IBAction)selectDefaultPreset:(id)sender;

@end
