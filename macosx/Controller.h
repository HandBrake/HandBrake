/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <Growl/Growl.h>

#import "PictureController.h"
#import "HBPreviewController.h"

#import "HBQueueController.h"

#import "HBVideoController.h"
#import "HBAudioController.h"
#import "HBSubtitlesController.h"
#import "HBAdvancedController.h"
#import "HBChapterTitlesController.h"

#import "HBPreferencesController.h"

extern NSString *HBContainerChangedNotification;
extern NSString *keyContainerTag;

@class HBOutputPanelController;
@class HBPresetsViewController;
@class HBPresetsManager;
@class HBDockTile;

@interface HBController : NSObject <NSApplicationDelegate, GrowlApplicationBridgeDelegate, HBPictureControllerDelegate, NSDrawerDelegate>
{
    IBOutlet NSWindow            * fWindow;

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
    
    /* Queue File variables */
    FSEventStreamRef               QueueStream;
    NSString                     * QueueFile;
	NSMutableArray               * QueueFileArray;
    NSInteger                      currentQueueEncodeIndex; // Used to track the currently encoding queueu item
    
	/* User Preset variables here */
	HBPresetsManager             * presetManager;
    HBPresetsViewController      * fPresetsView;

    IBOutlet NSMenu              * presetsMenu;
	IBOutlet NSDrawer            * fPresetDrawer;
    IBOutlet NSTextField         * fPresetSelectedDisplay;
    
    /* Queue variables */
    int                          hbInstanceNum; //stores the number of HandBrake instances currently running
    int                          fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    int                          fWorkingCount;
    
    NSInteger                      fqueueEditRescanItemNum; // queue array item to be reloaded into the main window
    pid_t                          pidNum; // The pid number for this instance
    NSString                     * currentQueueEncodeNameString;
    
    /* integer to set to determine the previous state
		of encode 0==idle, 1==encoding, 2==cancelled*/
    int                            fEncodeState;
    BOOL                           applyQueueToScan;
    NSString                      * browsedSourceDisplayName;
    
    /* Dock progress variables */
    double                          dockIconProgress;

    HBDockTile  *dockTile;
}

- (IBAction) browseSources: (id) sender;
- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) showSourceTitleScanPanel: (id) sender;
- (IBAction) closeSourceTitleScanPanel: (id) sender;  
- (void) performScan:(NSString *) scanPath scanTitleNum: (NSInteger) scanTitleNum;
- (IBAction) showNewScan: (id) sender;

- (void)     enableUI:                                 (BOOL)     enable;

- (IBAction) encodeStartStopPopUpChanged: (id) sender;


- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;
- (IBAction) startEndSecValueChanged: (id) sender;
- (IBAction) startEndFrameValueChanged: (id) sender;


- (IBAction) formatPopUpChanged: (id) sender;
- (IBAction) autoSetM4vExtension: (id) sender;

- (IBAction) browseFile: (id) sender;

- (IBAction) showPicturePanel: (id) sender;
- (IBAction) showPreviewWindow: (id) sender;
- (void)pictureSettingsDidChange;
- (IBAction) openMainWindow: (id) sender;

/* Text summaries of various settings */
- (NSString*) pictureSettingsSummary;
- (NSString*) muxerOptionsSummary;

/* Add All titles to the queue */
- (IBAction) addAllTitlesToQueue: (id) sender;
- (void) addAllTitlesToQueueAlertDone: (NSWindow *) sheet
                           returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void) doAddAllTitlesToQueue;

/* Queue File Stuff */
- (void) initQueueFSEvent;
- (void) closeQueueFSEvent;
- (void) loadQueueFile;
- (void) reloadQueue;
- (NSDictionary *)createQueueFileItem;
- (void)saveQueueFileItem;
- (void) incrementQueueItemDone:(NSInteger) queueItemDoneIndexNum;
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (NSInteger) scanTitleNum;
- (void) processNewQueueEncode;
- (void) clearQueueEncodedItems;
/* Queue Editing */
- (IBAction)applyQueueSettingsToMainWindow:(id)sender;
- (IBAction)rescanQueueItemToMainWindow:(NSString *) scanPath scanTitleNum: (NSUInteger) scanTitleNum selectedQueueItem: (NSUInteger) selectedQueueItem;


- (void) removeQueueFileItem:(NSUInteger) queueItemToRemove;
- (void) clearQueueAllItems;
- (void)moveObjectsInQueueArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex;
- (void)getQueueStats;
- (void)setQueueEncodingItemsAsPending;
- (IBAction) addToQueue: (id) sender;
- (void) overwriteAddToQueueAlertDone: (NSWindow *) sheet
                           returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     doAddToQueue;

- (IBAction) showQueueWindow:(id)sender;

- (IBAction)showPreferencesWindow:(id)sender;

- (IBAction) Rip: (id) sender;
- (void)     overWriteAlertDone: (NSWindow *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     doRip;

- (IBAction) Cancel: (id) sender;
- (void)     doCancelCurrentJob;
- (void) doCancelCurrentJobAndStop;
- (IBAction) Pause: (id) sender;

- (IBAction) openHomepage: (id) sender;
- (IBAction) openForums:   (id) sender;
- (IBAction) openUserGuide:   (id) sender;

// Preset Methods Here
/* Export / Import Presets */
- (IBAction) browseExportPresetFile: (id) sender;
- (IBAction) browseImportPresetFile: (id) sender;

/* Manage User presets */    
- (IBAction) customSettingUsed: (id) sender;
- (IBAction) showAddPresetPanel: (id) sender;

- (IBAction)selectDefaultPreset:(id)sender;
- (IBAction)addFactoryPresets:(id)sender;

-(void)sendToMetaX:(NSString *) filePath;
// Growl methods
- (NSDictionary *) registrationDictionaryForGrowl;
-(void)showGrowlDoneNotification:(NSString *) filePath;
- (IBAction)showDebugOutputPanel:(id)sender;

- (void) prepareJobForPreview;
- (void) remindUserOfSleepOrShutdown;

- (int) hbInstances;

@end
