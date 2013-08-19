/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <Growl/Growl.h>

#include "hb.h"

#import "ChapterTitles.h"
#import "HBSubtitles.h"
#import "PictureController.h"
#import "HBPreviewController.h"
#import "HBQueueController.h"
#import "HBAdvancedController.h"
#import "HBPreferencesController.h"
#import "HBPresets.h"
#import "HBAudioController.h"

extern NSString *HBContainerChangedNotification;
extern NSString *keyContainerTag;
extern NSString *HBTitleChangedNotification;
extern NSString *keyTitleTag;

@class HBOutputPanelController;
@class DockTextField;

/* We subclass NSView so that our drags show both the icon as well as PresetName columns */
@interface HBPresetsOutlineView : NSOutlineView
{

BOOL                        fIsDragging;

}
@end

@interface HBController : NSObject <GrowlApplicationBridgeDelegate>
{
    NSImage                      * fApplicationIcon;
    IBOutlet NSWindow            * fWindow;

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
    PictureController            * fPictureController;
    
    /* Picture Preview */
    PreviewController            * fPreviewController;
    
    /* x264 Presets Box */
    NSArray                      * fX264PresetNames;
    NSUInteger                     fX264MediumPresetIndex;
    IBOutlet NSButton            * fX264UseAdvancedOptionsCheck;
    IBOutlet NSBox               * fX264PresetsBox;
    IBOutlet NSSlider            * fX264PresetsSlider;
    IBOutlet NSTextField         * fX264PresetSliderLabel;
    IBOutlet NSTextField         * fX264PresetSelectedTextField;
    IBOutlet NSPopUpButton       * fX264TunePopUp;
    IBOutlet NSTextField         * fX264TunePopUpLabel;
    IBOutlet NSPopUpButton       * fX264ProfilePopUp;
    IBOutlet NSTextField         * fX264ProfilePopUpLabel;
    IBOutlet NSPopUpButton       * fX264LevelPopUp;
    IBOutlet NSTextField         * fX264LevelPopUpLabel;
    IBOutlet NSButton            * fX264FastDecodeCheck;
    IBOutlet NSTextField         * fDisplayX264PresetsAdditonalOptionsTextField;
    IBOutlet NSTextField         * fDisplayX264PresetsAdditonalOptionsLabel;
    // Text Field to show the expanded opts from unparse()
    IBOutlet NSTextField         * fDisplayX264PresetsUnparseTextField;
    char                         * fX264PresetsUnparsedUTF8String;
    NSUInteger                     fX264PresetsHeightForUnparse;
    NSUInteger                     fX264PresetsWidthForUnparse;
    
    /* Advanced options tab */
    HBAdvancedController         * fAdvancedOptions;
	IBOutlet NSBox               * fAdvancedView;
    
    HBPreferencesController      * fPreferencesController;
    
    /* Queue panel */
    HBQueueController            * fQueueController;
    IBOutlet NSTextField         * fQueueStatus;
    
    /* Output panel */
    HBOutputPanelController      * outputPanel;
	
    /* Source box */
	IBOutlet NSProgressIndicator * fScanIndicator;
	IBOutlet NSBox               * fScanHorizontalLine;
    
	NSString                     * sourceDisplayName;
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
    
    IBOutlet NSTextField         * fSrcChapterField;
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
    // Creates 64 bit mp4's that allow file sizes over 4gb
    IBOutlet NSButton            * fDstMp4LargeFileCheck;
    // Optimizes mp4's for http
    IBOutlet NSButton            * fDstMp4HttpOptFileCheck;
    // Creates iPod compatible mp4's (add ipod uuid atom)
    IBOutlet NSButton            * fDstMp4iPodFileCheck;
	
    /* Video box */
    
    /* Framerate */
    /* Radio Button Framerate Controls */
    IBOutlet NSMatrix            * fFramerateMatrix;
    IBOutlet NSButtonCell        * fFramerateVfrPfrCell;
    IBOutlet NSButtonCell        * fFramerateCfrCell;
    
    /* Video Encoder */
    IBOutlet NSTextField         * fVidRateField;
    IBOutlet NSPopUpButton       * fVidRatePopUp;
    IBOutlet NSTextField         * fVidEncoderField;
    IBOutlet NSPopUpButton       * fVidEncoderPopUp;
    IBOutlet NSTextField         * fVidQualityField;
    IBOutlet NSTextField         * fVidQualityRFLabel;
    IBOutlet NSTextField         * fVidQualityRFField;
    IBOutlet NSMatrix            * fVidQualityMatrix;
    IBOutlet NSButtonCell        * fVidBitrateCell;
    IBOutlet NSTextField         * fVidBitrateField;
    IBOutlet NSButtonCell        * fVidConstantCell;
    IBOutlet NSSlider            * fVidQualitySlider;
    IBOutlet NSButton            * fVidTwoPassCheck;
    IBOutlet NSButton            * fVidTurboPassCheck;
	
    /* Status read out fields for picture settings and video filters */
    IBOutlet NSTextField         * fPictureSettingsField;
    IBOutlet NSTextField         * fPictureFiltersField;
	
	/* Picture variables */
	int                        PicOrigOutputWidth;
	int                        PicOrigOutputHeight;
	int                        AutoCropTop;
	int                        AutoCropBottom;
	int                        AutoCropLeft;
	int                        AutoCropRight;
    /* Subtitles box */
    IBOutlet NSTextField         * fSubField;
    IBOutlet NSPopUpButton       * fSubPopUp;
	IBOutlet NSButton            * fSubForcedCheck;
    
    
    IBOutlet NSTableView         * fSubtitlesTable;
	HBSubtitles                  * fSubtitlesDelegate;
    IBOutlet NSButton            * fBrowseSrtFileButton;
    
	/* New Audio box */
	IBOutlet HBAudioController   * fAudioDelegate;
    
    /* New Audio Auto Passthru box */
    IBOutlet NSBox               * fAudioAutoPassthruBox;
    IBOutlet NSButton            * fAudioAllowAACPassCheck;
    IBOutlet NSButton            * fAudioAllowAC3PassCheck;
    IBOutlet NSButton            * fAudioAllowDTSHDPassCheck;
    IBOutlet NSButton            * fAudioAllowDTSPassCheck;
    IBOutlet NSButton            * fAudioAllowMP3PassCheck;
    IBOutlet NSPopUpButton       * fAudioFallbackPopUp;
    
    	    
    /* Chapters box */
    IBOutlet NSButton            * fCreateChapterMarkers;
    IBOutlet NSTableView         * fChapterTable;
	IBOutlet NSButton            * fLoadChaptersButton;
	IBOutlet NSButton            * fSaveChaptersButton;
	IBOutlet NSTableColumn       * fChapterTableNameColumn;
	ChapterTitles                * fChapterTitlesDelegate;
	
    /* Bottom */
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
	BOOL                           fRipIndicatorShown;
    
    /* Queue File variables */
    FSEventStreamRef               QueueStream;
    NSString                     * QueueFile;
	NSMutableArray               * QueueFileArray;
    int                            currentQueueEncodeIndex; // Used to track the currently encoding queueu item
    
	/* User Preset variables here */
	HBPresets                    * fPresetsBuiltin;
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSTextField         * fPresetNewDesc;
	IBOutlet NSPopUpButton       * fPresetNewPicSettingsPopUp;
    IBOutlet NSTextField         * fPresetNewPicWidth;
    IBOutlet NSTextField         * fPresetNewPicHeight;
    IBOutlet NSBox               * fPresetNewPicWidthHeightBox;
    
    IBOutlet NSButton            * fPresetNewPicFiltersCheck;
    IBOutlet NSButton            * fPresetNewFolderCheck;
	IBOutlet NSTextField         * fPresetSelectedDisplay;
	
	NSString                     * AppSupportDirectory;
	NSString                     * UserPresetsFile;
	NSMutableArray               * UserPresets;
	NSMutableArray               * UserPresetssortedArray;
	NSMutableDictionary          * chosenPreset;
	 
	NSMutableDictionary          *presetHbDefault; // this is 1 in "Default" preset key
	NSMutableDictionary          *presetUserDefault;// this is 2 in "Default" preset key
    NSMutableDictionary          *presetUserDefaultParent;
    NSMutableDictionary          *presetUserDefaultParentParent;
    int                           presetCurrentBuiltInCount; // keeps track of the current number of built in presets
    IBOutlet NSPanel             * fAddPresetPanel;
	
    /* NSOutline View for the presets */
    NSArray                      *fDraggedNodes;
    IBOutlet HBPresetsOutlineView * fPresetsOutlineView;
    IBOutlet NSButton            * fPresetsAdd;
	IBOutlet NSButton            * fPresetsDelete;
    IBOutlet NSPopUpButton       * fPresetsActionButton;

    hb_handle_t                  * fHandle;
    
    /* Queue variables */
    int                          hbInstanceNum; //stores the number of HandBrake instances currently running
    hb_handle_t                  * fQueueEncodeLibhb;           // libhb for HB Encoding
	hb_title_t                   * fTitle;
    hb_title_t                   * fQueueEncodeTitle;
    int                          fEncodingQueueItem;     // corresponds to the index of fJobGroups encoding item
    int                          fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    int                          fCompletedCount;
    int                          fCanceledCount;
    int                          fWorkingCount;
    
    int                          fqueueEditRescanItemNum; // queue array item to be reloaded into the main window
    int                          pidNum; // The pid number for this instance
    NSString                     * currentQueueEncodeNameString;
    
    /* integer to set to determine the previous state
		of encode 0==idle, 1==encoding, 2==cancelled*/
    int                            fEncodeState;
	int                            currentScanCount;
	int                            currentSuccessfulScanCount;
    BOOL                           SuccessfulScan;
    BOOL                           applyQueueToScan;
	NSString                      * currentSource;
    NSString                      * browsedSourceDisplayName;
    
    /* Dock progress variables */
    double                          dockIconProgress;
    
    BOOL                            fWillScan;
    NSDockTile                    * dockTile;
    DockTextField                 * percentField;
    DockTextField                 * timeField;
}
- (int) getPidnum;
- (IBAction) showAboutPanel:(id)sender;

- (void) writeToActivityLog:(const char *) format, ...;
- (IBAction) browseSources: (id) sender;
- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) showSourceTitleScanPanel: (id) sender;
- (IBAction) closeSourceTitleScanPanel: (id) sender;  
- (void) performScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum;
- (IBAction) showNewScan: (id) sender;


- (IBAction) cancelScanning:(id)sender;

- (void)     updateUI:                                 (NSTimer*) timer;
- (void)     enableUI:                                 (bool)     enable;
- (void)     setupX264PresetsWidgets:                  (id)       sender;
- (void)     enableX264Widgets:                        (bool)     enable;
- (IBAction) updateX264Widgets:                        (id)       sender;
- (IBAction) x264PresetsChangedDisplayExpandedOptions: (id)       sender;

- (IBAction) encodeStartStopPopUpChanged: (id) sender;


- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;
- (IBAction) startEndSecValueChanged: (id) sender;
- (IBAction) startEndFrameValueChanged: (id) sender;


- (IBAction) formatPopUpChanged: (id) sender;
- (IBAction) videoEncoderPopUpChanged: (id) sender;
- (IBAction) autoSetM4vExtension: (id) sender;
- (IBAction) twoPassCheckboxChanged: (id) sender;
- (IBAction) videoFrameRateChanged: (id) sender;
- (void) prepareJob;
- (IBAction) browseFile: (id) sender;
- (void)     browseFileDone: (NSSavePanel *) sheet
                 returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) videoMatrixChanged: (id) sender;

- (IBAction) qualitySliderChanged: (id) sender;
- (void) setupQualitySlider;

- (IBAction) browseImportSrtFile: (id) sender;
- (void) browseImportSrtFileDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) showPicturePanel: (id) sender;
- (void) picturePanelWindowed;
- (IBAction) showPreviewWindow: (id) sender;
- (void)pictureSettingsDidChange;
- (IBAction) calculatePictureSizing: (id) sender;
- (IBAction) openMainWindow: (id) sender;

/* Text summaries of various settings */
- (NSString*) pictureSettingsSummary;
- (NSString*) pictureFiltersSummary;
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
- (void) incrementQueueItemDone:(int) queueItemDoneIndexNum;
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum;
- (void) processNewQueueEncode;
- (void) clearQueueEncodedItems;
/* Queue Editing */
- (IBAction)applyQueueSettingsToMainWindow:(id)sender;
- (IBAction)rescanQueueItemToMainWindow:(NSString *) scanPath scanTitleNum: (int) scanTitleNum selectedQueueItem: (int) selectedQueueItem;


- (void) removeQueueFileItem:(int) queueItemToRemove;
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

- (IBAction) calculateBitrate: (id) sender;
- (void) controlTextDidChange: (NSNotification *) notification;

- (IBAction) openHomepage: (id) sender;
- (IBAction) openForums:   (id) sender;
- (IBAction) openUserGuide:   (id) sender;

// Preset Methods Here
    
/* These are required by the NSOutlineView Datasource Delegate */
/* We use this to deterimine children of an item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView child:(NSInteger)index ofItem:(id)item;
/* We use this to determine if an item should be expandable */
- (BOOL)outlineView:(NSOutlineView *)fPresetsOutlineView isItemExpandable:(id)item;
/* used to specify the number of levels to show for each item */
- (int)outlineView:(NSOutlineView *)fPresetsOutlineView numberOfChildrenOfItem:(id)item;
/* Used to tell the outline view which information is to be displayed per item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
/* Use to customize the font and display characteristics of the title cell */
- (void)outlineView:(NSOutlineView *)fPresetsOutlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item;
/* We use this to edit the name field in the outline view */
- (void)outlineView:(NSOutlineView *)fPresetsOutlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
/* We use this to provide tooltips for the items in the presets outline view */
- (NSString *)outlineView:(NSOutlineView *)fPresetsOutlineView toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tc item:(id)item mouseLocation:(NSPoint)mouseLocation;
- (void) checkBuiltInsForUpdates;
/* We use this to actually select the preset and act accordingly */
- (IBAction)selectPreset:(id)sender;

@property (nonatomic, readonly) BOOL hasValidPresetSelected;
- (id)selectedPreset;

/* Export / Import Presets */
- (IBAction) browseExportPresetFile: (id) sender;
- (void) browseExportPresetFileDone: (NSSavePanel *) sheet
             returnCode: (int) returnCode contextInfo: (void *) contextInfo;
             
- (IBAction) browseImportPresetFile: (id) sender;
- (void) browseImportPresetDone: (NSSavePanel *) sheet
                   returnCode: (int) returnCode contextInfo: (void *) contextInfo;

/* Manage User presets */    
- (void) loadPresets;
- (IBAction) customSettingUsed: (id) sender;
- (IBAction) showAddPresetPanel: (id) sender;
- (IBAction) addPresetPicDropdownChanged: (id) sender;
- (IBAction) closeAddPresetPanel: (id) sender;
- (NSDictionary *)createPreset;

- (IBAction) revertPictureSizeToMax:(id)sender;

- (IBAction)setDefaultPreset:(id)sender;
- (IBAction)selectDefaultPreset:(id)sender;
- (void) savePreset;
- (void)sortPresets;
- (IBAction)addFactoryPresets:(id)sender;
- (IBAction)deleteFactoryPresets:(id)sender;
- (IBAction)addUserPreset:(id)sender;
- (void)addPreset;
- (IBAction)insertPreset:(id)sender;
- (IBAction)deletePreset:(id)sender;
- (IBAction)getDefaultPresets:(id)sender;

-(void)sendToMetaX:(NSString *) filePath;
// Growl methods
- (NSDictionary *) registrationDictionaryForGrowl;
-(void)showGrowlDoneNotification:(NSString *) filePath;
- (IBAction)showDebugOutputPanel:(id)sender;
- (void)setupToolbar;

- (void) prepareJobForPreview;
- (void) remindUserOfSleepOrShutdown;

- (void)moveObjectsInPresetsArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex;

- (int) hbInstances;

// Chapter files methods
- (IBAction) browseForChapterFile: (id) sender;
- (void)     browseForChapterFileDone: (NSOpenPanel *) sheet
                 returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) browseForChapterFileSave: (id) sender;
- (void)     browseForChapterFileSaveDone: (NSSavePanel *) sheet
                 returnCode: (int) returnCode contextInfo: (void *) contextInfo;

+ (unsigned int) maximumNumberOfAllowedAudioTracks;
- (IBAction) addAllAudioTracks: (id) sender;

// Drag & Drop methods
- (void)openFiles:(NSArray*)filenames;
- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames;
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;

- (void) updateDockIcon:(double)progress withETA:(NSString*)etaStr;

// x264 system methods
- (NSString*) x264Preset;
- (NSString*) x264Tune;
- (NSString*) x264OptionExtra;
- (NSString*) h264Profile;
- (NSString*) h264Level;
- (void)      setX264Preset:            (NSString*) x264Preset;
- (void)      setX264Tune:              (NSString*) x264Tune;
- (void)      setX264OptionExtra:       (NSString*) x264OptionExtra;
- (void)      setH264Profile:           (NSString*) h264Profile;
- (void)      setH264Level:             (NSString*) h264Level;
- (IBAction)  x264PresetsSliderChanged: (id)        sender;


@end

