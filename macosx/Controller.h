/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBQueueController;

@class HBVideoController;
@class HBAudioController;
@class HBSubtitlesController;
@class HBAdvancedController;
@class HBChapterTitlesController;

@class HBPictureController;
@class HBPreviewController;

@class HBPreferencesController;
@class HBOutputPanelController;
@class HBPresetsViewController;
@class HBPresetsManager;
@class HBDockTile;

@class HBJob;

@interface HBController : NSObject <NSApplicationDelegate, NSDrawerDelegate>
{
    IBOutlet NSWindow *fWindow;

    IBOutlet NSTabView *fMainTabView;

    /* Video view controller */
    HBVideoController       * fVideoController;
    IBOutlet NSTabViewItem  * fVideoTab;

    /* Subtitles view controller */
	HBSubtitlesController   * fSubtitlesViewController;
    IBOutlet NSTabViewItem  * fSubtitlesTab;

	/* Audio view controller */
	HBAudioController       * fAudioController;
    IBOutlet NSTabViewItem  * fAudioTab;

	/* Chapters view controller */
	HBChapterTitlesController    * fChapterTitlesController;
    IBOutlet NSTabViewItem       * fChaptersTitlesTab;

    /* Advanced options tab */
    HBAdvancedController         * fAdvancedOptions;
	IBOutlet NSTabViewItem       * fAdvancedTab;

    /* Main Menu Outlets */
    NSMenuItem                   * fOpenSourceTitleMMenu;
    
    /* Source Title Scan Outlets */
    IBOutlet NSPanel              * fScanSrcTitlePanel;
    IBOutlet NSTextField          * fScanSrcTitlePathField;
    IBOutlet NSTextField          * fSrcDsplyNameTitleScan;
    IBOutlet NSTextField          * fScanSrcTitleNumField;
    IBOutlet NSButton             * fScanSrcTitleCancelButton;
    IBOutlet NSButton             * fScanSrcTitleOpenButton;

    /* Picture Settings */
    HBPictureController            * fPictureController;
    
    /* Picture Preview */
    HBPreviewController            * fPreviewController;

    HBPreferencesController      * fPreferencesController;
    
    /* Queue panel */
    HBQueueController            * fQueueController;
    IBOutlet NSTextField         * fQueueStatus;
    
    /* Output panel */
    HBOutputPanelController      * outputPanel;
	
    /* Source box */
	IBOutlet NSProgressIndicator * fScanIndicator;
	IBOutlet NSBox               * fScanHorizontalLine;
    
    IBOutlet NSTextField         * fSrcDVD2Field;
    IBOutlet NSTextField         * fSrcTitleField;
    IBOutlet NSPopUpButton       * fSrcTitlePopUp;
    
    
    /* lib dvd nav specific */
    IBOutlet NSTextField         * fSrcAngleLabel;
    IBOutlet NSPopUpButton       * fSrcAnglePopUp;
    
    /* Source start and end points */
    IBOutlet NSPopUpButton       * fEncodeStartStopPopUp;
    /* pts based start / stop */
    IBOutlet NSTextField         * fSrcTimeStartEncodingField;
    IBOutlet NSTextField         * fSrcTimeEndEncodingField;
    /* frame based based start / stop */
    IBOutlet NSTextField         * fSrcFrameStartEncodingField;
    IBOutlet NSTextField         * fSrcFrameEndEncodingField;
    
    IBOutlet NSPopUpButton       * fSrcChapterStartPopUp;
    IBOutlet NSTextField         * fSrcChapterToField;
    IBOutlet NSPopUpButton       * fSrcChapterEndPopUp;
    
    /* Source duration information */
    IBOutlet NSTextField         * fSrcDuration1Field;
    IBOutlet NSTextField         * fSrcDuration2Field;
	
    /* Destination box */
    IBOutlet NSTextField         * fDstFormatField;
	IBOutlet NSPopUpButton       * fDstFormatPopUp;
	
    IBOutlet NSTextField         * fDstFile1Field;
    IBOutlet NSTextField         * fDstFile2Field;
    IBOutlet NSButton            * fDstBrowseButton;
    /* MP4 Options */
    // Optimizes mp4's for http
    IBOutlet NSButton            * fDstMp4HttpOptFileCheck;
    // Creates iPod compatible mp4's (add ipod uuid atom)
    IBOutlet NSButton            * fDstMp4iPodFileCheck;

    /* Bottom */
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
	BOOL                           fRipIndicatorShown;
    
	/* User Preset variables here */
	HBPresetsManager             * presetManager;
    HBPresetsViewController      * fPresetsView;

    IBOutlet NSMenu              * presetsMenu;
	IBOutlet NSDrawer            * fPresetDrawer;
}

@property (nonatomic, readonly) NSWindow *window;

- (IBAction) browseSources: (id) sender;
- (IBAction) showSourceTitleScanPanel: (id) sender;
- (IBAction) closeSourceTitleScanPanel: (id) sender;  
- (void) performScan:(NSURL *)scanURL scanTitleNum:(NSInteger)scanTitleNum;

- (void)enableUI:(BOOL)enable;

- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;

- (IBAction) autoSetM4vExtension: (id) sender;

- (IBAction) browseFile: (id) sender;

- (IBAction) showPicturePanel: (id) sender;
- (IBAction) showPreviewWindow: (id) sender;
- (void)pictureSettingsDidChange;
- (IBAction) openMainWindow: (id) sender;

// Queue
- (IBAction)addToQueue:(id)sender;
- (IBAction)addAllTitlesToQueue:(id)sender;

- (void)rescanJobToMainWindow:(HBJob *)queueItem;
- (void)setQueueState:(NSString *)info;
- (void)setQueueInfo:(NSString *)info progress:(double)progress hidden:(BOOL)hidden;

- (IBAction)showQueueWindow:(id)sender;

- (IBAction)showPreferencesWindow:(id)sender;

- (IBAction)rip:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)pause:(id)sender;

- (IBAction) openHomepage: (id) sender;
- (IBAction) openForums:   (id) sender;
- (IBAction) openUserGuide:   (id) sender;

// Preset Methods Here
/* Export / Import Presets */
- (IBAction) browseExportPresetFile: (id) sender;
- (IBAction) browseImportPresetFile: (id) sender;

/* Manage User presets */    
- (IBAction) showAddPresetPanel: (id) sender;
- (IBAction)selectDefaultPreset:(id)sender;
- (IBAction)addFactoryPresets:(id)sender;
- (IBAction)showDebugOutputPanel:(id)sender;

@end
