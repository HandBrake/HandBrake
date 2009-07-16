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

@class HBOutputPanelController;

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
    
    /* Advanced options tab */
    HBAdvancedController         * fAdvancedOptions;
	IBOutlet NSBox               * fAdvancedView;
    
    HBPreferencesController      * fPreferencesController;
    
    /* Queue panel */
    HBQueueController            * fQueueController;
    IBOutlet NSTextField         * fQueueStatus;
    
    /* Output panel */
    HBOutputPanelController       *outputPanel;
	
    /* Source box */
	IBOutlet NSProgressIndicator * fScanIndicator;
	NSString                     * sourceDisplayName;
    IBOutlet NSTextField         * fSrcDVD2Field;
    IBOutlet NSTextField         * fSrcTitleField;
    IBOutlet NSPopUpButton       * fSrcTitlePopUp;
    
    
    /* lib dvd nav specific */
    IBOutlet NSTextField         * fSrcAngleLabel;
    IBOutlet NSPopUpButton       * fSrcAnglePopUp;
    
    IBOutlet NSTextField         * fSrcChapterField;
    IBOutlet NSPopUpButton       * fSrcChapterStartPopUp;
    IBOutlet NSTextField         * fSrcChapterToField;
    IBOutlet NSPopUpButton       * fSrcChapterEndPopUp;
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
    IBOutlet NSTextField         * fVidRateField;
    IBOutlet NSPopUpButton       * fVidRatePopUp;
    IBOutlet NSTextField         * fVidEncoderField;
    IBOutlet NSPopUpButton       * fVidEncoderPopUp;
    IBOutlet NSTextField         * fVidQualityField;
    IBOutlet NSTextField         * fVidQualityRFLabel;
    IBOutlet NSTextField         * fVidQualityRFField;
    IBOutlet NSMatrix            * fVidQualityMatrix;
    IBOutlet NSButtonCell        * fVidTargetCell;
    IBOutlet NSTextField         * fVidTargetSizeField;
    IBOutlet NSButtonCell        * fVidBitrateCell;
    IBOutlet NSTextField         * fVidBitrateField;
    IBOutlet NSButtonCell        * fVidConstantCell;
    IBOutlet NSSlider            * fVidQualitySlider;
    IBOutlet NSButton            * fVidTwoPassCheck;
    IBOutlet NSButton            * fVidTurboPassCheck;
	
	/* Status read out fileds for picture sizing */
    IBOutlet NSTextField         * fPictureSizeField;
    IBOutlet NSTextField         * fPictureCroppingField;
	
    /* Status read out fileds for video filters */
    IBOutlet NSTextField         * fVideoFiltersField;
	
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
    
	
    /* Audio box */
    /* Track Labels */
    IBOutlet NSTextField         * fAudSourceLabel;
    IBOutlet NSTextField         * fAudCodecLabel;
    IBOutlet NSTextField         * fAudMixdownLabel;
    IBOutlet NSTextField         * fAudSamplerateLabel;
    IBOutlet NSTextField         * fAudBitrateLabel;
    IBOutlet NSTextField         * fAudDrcLabel;
    
    IBOutlet NSTextField         * fAudTrack1Label;
    IBOutlet NSTextField         * fAudTrack2Label;
    IBOutlet NSTextField         * fAudTrack3Label;
    IBOutlet NSTextField         * fAudTrack4Label;
    
    /* Source Audio PopUps */
    IBOutlet NSPopUpButton       * fAudLang1PopUp;
    IBOutlet NSPopUpButton       * fAudLang2PopUp;
    IBOutlet NSPopUpButton       * fAudLang3PopUp;
    IBOutlet NSPopUpButton       * fAudLang4PopUp;
    
    /* Codec Popups */
    IBOutlet NSPopUpButton       * fAudTrack1CodecPopUp;
    IBOutlet NSPopUpButton       * fAudTrack2CodecPopUp;
    IBOutlet NSPopUpButton       * fAudTrack3CodecPopUp;
    IBOutlet NSPopUpButton       * fAudTrack4CodecPopUp;
    
	/* Mixdown PopUps */
	IBOutlet NSPopUpButton       * fAudTrack1MixPopUp;
    IBOutlet NSPopUpButton       * fAudTrack2MixPopUp;
    IBOutlet NSPopUpButton       * fAudTrack3MixPopUp;
    IBOutlet NSPopUpButton       * fAudTrack4MixPopUp;
	
    /* Samplerate PopUps */
	IBOutlet NSPopUpButton       * fAudTrack1RatePopUp;
    IBOutlet NSPopUpButton       * fAudTrack2RatePopUp;
    IBOutlet NSPopUpButton       * fAudTrack3RatePopUp;
    IBOutlet NSPopUpButton       * fAudTrack4RatePopUp;
    
    /* Bitrate PopUps */
    IBOutlet NSPopUpButton       * fAudTrack1BitratePopUp;
    IBOutlet NSPopUpButton       * fAudTrack2BitratePopUp;
    IBOutlet NSPopUpButton       * fAudTrack3BitratePopUp;
    IBOutlet NSPopUpButton       * fAudTrack4BitratePopUp;
    
    /* Dynamic Range Compression */
    IBOutlet NSSlider            * fAudTrack1DrcSlider;
    IBOutlet NSTextField         * fAudTrack1DrcField;
    IBOutlet NSSlider            * fAudTrack2DrcSlider;
    IBOutlet NSTextField         * fAudTrack2DrcField;
    IBOutlet NSSlider            * fAudTrack3DrcSlider;
    IBOutlet NSTextField         * fAudTrack3DrcField;
    IBOutlet NSSlider            * fAudTrack4DrcSlider;
    IBOutlet NSTextField         * fAudTrack4DrcField;
    
    /* Chapters box */
    IBOutlet NSButton            * fCreateChapterMarkers;
    IBOutlet NSTableView         * fChapterTable;
	ChapterTitles                * fChapterTitlesDelegate;
	
    /* Bottom */
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
	BOOL                           fRipIndicatorShown;
    
    /* Queue File variables */
    NSString                     * QueueFile;
	NSMutableArray               * QueueFileArray;
    int                            currentQueueEncodeIndex; // Used to track the currently encoding queueu item
    
	/* User Preset variables here */
	HBPresets                    * fPresetsBuiltin;
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSTextField         * fPresetNewDesc;
	IBOutlet NSPopUpButton       * fPresetNewPicSettingsPopUp;
    IBOutlet NSButton            * fPresetNewPicFiltersCheck;
    IBOutlet NSButton            * fPresetNewFolderCheck;
	IBOutlet NSTextField         * fPresetSelectedDisplay;
	
	NSString                     * AppSupportDirectory;
	NSString                     * UserPresetsFile;
	NSMutableArray               * UserPresets;
	NSMutableArray               * UserPresetssortedArray;
	NSMutableDictionary          * chosenPreset;
    int                            curUserPresetChosenNum;
	 
	NSMutableDictionary          *presetHbDefault; // this is 1 in "Default" preset key
	NSMutableDictionary          *presetUserDefault;// this is 2 in "Default" preset key
    NSMutableDictionary          *presetUserDefaultParent;
    NSMutableDictionary          *presetUserDefaultParentParent;
    int                        presetCurrentBuiltInCount; // keeps track of the current number of built in presets
    IBOutlet NSPanel             * fAddPresetPanel;
	
    /* NSOutline View for the presets */
    NSArray                      *fDraggedNodes;
    IBOutlet HBPresetsOutlineView * fPresetsOutlineView;
    IBOutlet NSButton            * fPresetsAdd;
	IBOutlet NSButton            * fPresetsDelete;
    IBOutlet NSPopUpButton       * fPresetsActionButton;

    hb_handle_t                  * fHandle;
    
    /* Queue variables */
    hb_handle_t              * fQueueEncodeLibhb;           // libhb for HB Encoding
	hb_title_t                   * fTitle;
    hb_title_t                   * fQueueEncodeTitle;
    int                          fEncodingQueueItem;     // corresponds to the index of fJobGroups encoding item
    int                          fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    int                          fCompletedCount;
    int                          fCanceledCount;
    int                          fWorkingCount;
    
    
    /* integer to set to determine the previous state
		of encode 0==idle, 1==encoding, 2==cancelled*/
    int                            fEncodeState;
	int                            currentScanCount;
	int                            currentSuccessfulScanCount;
    BOOL                           SuccessfulScan;
    BOOL                           applyQueueToScan;
	NSString                      * currentSource;
    NSString                     * browsedSourceDisplayName;
    
    double                         dockIconProgress;
}

- (IBAction) showAboutPanel:(id)sender;

- (void) writeToActivityLog:(const char *) format, ...;
- (IBAction) browseSources: (id) sender;
- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) showSourceTitleScanPanel: (id) sender;
- (IBAction) closeSourceTitleScanPanel: (id) sender;  
- (void) performScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum;
- (IBAction) showNewScan: (id) sender;

- (void)     updateUI: (NSTimer *) timer;
- (void)     enableUI: (bool) enable;

- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;

- (IBAction) formatPopUpChanged: (id) sender;
- (IBAction) videoEncoderPopUpChanged: (id) sender;
- (IBAction) autoSetM4vExtension: (id) sender;
- (IBAction) twoPassCheckboxChanged: (id) sender;
- (IBAction) videoFrameRateChanged: (id) sender;
- (IBAction) audioAddAudioTrackCodecs: (id)sender;
- (IBAction) audioCodecsPopUpChanged: (id) sender;
- (IBAction) setEnabledStateOfAudioMixdownControls: (id) sender;
- (IBAction) addAllAudioTracksToPopUp: (id) sender;
- (IBAction) selectAudioTrackInPopUp: (id) sender searchPrefixString: (NSString *) searchPrefixString selectIndexIfNotFound: (int) selectIndexIfNotFound;
- (IBAction) audioTrackPopUpChanged: (id) sender;
- (IBAction) audioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse;
- (IBAction) audioTrackMixdownChanged: (id) sender;
- (void) prepareJob;
- (IBAction) browseFile: (id) sender;
- (void)     browseFileDone: (NSSavePanel *) sheet
                 returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) videoMatrixChanged: (id) sender;

- (IBAction) qualitySliderChanged: (id) sender;
- (void) setupQualitySlider;

- (IBAction) audioDRCSliderChanged: (id) sender;
- (IBAction) browseImportSrtFile: (id) sender;
- (void) browseImportSrtFileDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) showPicturePanel: (id) sender;
- (void) picturePanelFullScreen;
- (void) picturePanelWindowed;
- (IBAction) showPreviewWindow: (id) sender;
- (void)pictureSettingsDidChange;
- (IBAction) calculatePictureSizing: (id) sender;
- (IBAction) openMainWindow: (id) sender;

/* Queue File Stuff */
- (void) loadQueueFile;
- (NSDictionary *)createQueueFileItem;
- (void)saveQueueFileItem;
- (void) incrementQueueItemDone:(int) queueItemDoneIndexNum;
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum;
- (void) processNewQueueEncode;
- (void) clearQueueEncodedItems;
- (IBAction)applyQueueSettings:(id)sender;
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




@end

