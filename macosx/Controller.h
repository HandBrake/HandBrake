/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <Growl/Growl.h>

#include "hb.h"

#import "ChapterTitles.h"
#import "PictureController.h"
#import "HBQueueController.h"
#import "MVMenuButton.h"
#import "HBAdvancedController.h"
#import "HBPreferencesController.h"
#import "HBPresets.h"
@class HBOutputPanelController;

@interface HBController : NSObject <GrowlApplicationBridgeDelegate>

{
    IBOutlet NSWindow            * fWindow;
    NSToolbar                    * toolbar;
    
    /* Main Menu Outlets */
    NSMenuItem                   * fOpenSourceTitleMMenu;
    
    /* Source Title Scan Outlets */
    IBOutlet NSPanel              * fScanSrcTitlePanel;
    IBOutlet NSTextField          * fScanSrcTitlePathField;
    IBOutlet NSTextField          * fSrcDsplyNameTitleScan;
    IBOutlet NSTextField          * fScanSrcTitleNumField;
    IBOutlet NSButton             * fScanSrcTitleCancelButton;
    IBOutlet NSButton             * fScanSrcTitleOpenButton;
    
    /* Picture panel */
    PictureController            * fPictureController;
    
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
    IBOutlet NSTextField         * fSrcChapterField;
    IBOutlet NSPopUpButton       * fSrcChapterStartPopUp;
    IBOutlet NSTextField         * fSrcChapterToField;
    IBOutlet NSPopUpButton       * fSrcChapterEndPopUp;
    IBOutlet NSTextField         * fSrcDuration1Field;
    IBOutlet NSTextField         * fSrcDuration2Field;
	
    /* Destination box */
    IBOutlet NSTextField         * fDstFormatField;
	IBOutlet NSPopUpButton       * fDstFormatPopUp;
	IBOutlet NSTextField         * fDstCodecsField;
    IBOutlet NSPopUpButton       * fDstCodecsPopUp;
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
    IBOutlet NSMatrix            * fVidQualityMatrix;
    IBOutlet NSButtonCell        * fVidTargetCell;
    IBOutlet NSTextField         * fVidTargetSizeField;
    IBOutlet NSButtonCell        * fVidBitrateCell;
    IBOutlet NSTextField         * fVidBitrateField;
    IBOutlet NSButtonCell        * fVidConstantCell;
    IBOutlet NSSlider            * fVidQualitySlider;
    IBOutlet NSButton            * fVidGrayscaleCheck;
    IBOutlet NSButton            * fVidTwoPassCheck;
    IBOutlet NSButton            * fVidTurboPassCheck;
	
	/* Picture Settings box */
	IBOutlet NSTextField         * fPicLabelSettings;
	IBOutlet NSTextField         * fPicLabelSrc;
    IBOutlet NSTextField         * fPicSettingsSrc;
	IBOutlet NSTextField         * fPicLabelOutp;
    IBOutlet NSTextField         * fPicSettingsOutp;
    IBOutlet NSTextField         * fPicLabelAnamorphic;
    IBOutlet NSTextField         * fPicSettingsAnamorphic;
    
    IBOutlet NSTextField         * fPicLabelAr;
	IBOutlet NSTextField         * fPicLabelAutoCrop;
    IBOutlet NSTextField         * fPicLabelDetelecine;
	IBOutlet NSTextField         * fPicLabelDeinterlace;
    IBOutlet NSTextField         * fPicLabelDenoise;
    IBOutlet NSTextField         * fPicLabelDeblock;
	IBOutlet NSTextField         * fPicSettingDeinterlace;
	IBOutlet NSTextField         * fPicSettingARkeep;
	IBOutlet NSTextField         * fPicSettingPAR;
	IBOutlet NSTextField         * fPicSettingAutoCrop;
	IBOutlet NSTextField         * fPicSettingDetelecine;
	IBOutlet NSTextField         * fPicSettingDenoise;
    IBOutlet NSTextField         * fPicSettingDeblock;
	
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
	
    /* Audio box */
    IBOutlet NSTextField         * fAudLang1Field;
    IBOutlet NSPopUpButton       * fAudLang1PopUp;
    IBOutlet NSTextField         * fAudLang2Field;
    IBOutlet NSPopUpButton       * fAudLang2PopUp;
	/* New Audio Mix PopUps */
	/* Track info */
    IBOutlet NSTextField         * fAudTrack1MixLabel;
	IBOutlet NSPopUpButton       * fAudTrack1MixPopUp;
    IBOutlet NSTextField         * fAudTrack2MixLabel;
	IBOutlet NSPopUpButton       * fAudTrack2MixPopUp;
	
    /* Quality info */
	IBOutlet NSTextField         * fAudRateField;
    IBOutlet NSPopUpButton       * fAudRatePopUp;
    IBOutlet NSTextField         * fAudBitrateField;
    IBOutlet NSPopUpButton       * fAudBitratePopUp;
    /*Dynamic Range Compression */
    IBOutlet NSSlider            * fAudDrcSlider;
    IBOutlet NSTextField         * fAudDrcField;
    IBOutlet NSTextField         * fAudDrcLabel;
    IBOutlet NSTextField         * fAudDrcDescLabel1;
    IBOutlet NSTextField         * fAudDrcDescLabel2;
    IBOutlet NSTextField         * fAudDrcDescLabel3;
    IBOutlet NSTextField         * fAudDrcDescLabel4;
    
    /* Chapters box */
    IBOutlet NSButton            * fCreateChapterMarkers;
    IBOutlet NSTableView         * fChapterTable;
	ChapterTitles                * fChapterTitlesDelegate;
	
    /* Bottom */
    IBOutlet NSButton            * fPictureButton;
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
	BOOL                           fRipIndicatorShown;
    
	/* User Preset variables here */
	HBPresets                    * fPresetsBuiltin;
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSTextField         * fPresetNewDesc;
	IBOutlet NSPopUpButton       * fPresetNewPicSettingsPopUp;
    IBOutlet NSButton            * fPresetNewPicFiltersCheck;
	IBOutlet NSTextField         * fPresetSelectedDisplay;
	
	NSString                     * AppSupportDirectory;
	NSString                     * UserPresetsFile;
	NSMutableArray               * UserPresets;
	NSMutableArray               * UserPresetssortedArray;
	NSMutableDictionary          * chosenPreset;
    int                            curUserPresetChosenNum;
	 
	int                            presetHbDefault; // this is 1 in "Default" preset key
	int                            presetUserDefault;// this is 2 in "Default" preset key
    IBOutlet NSPanel             * fAddPresetPanel;
	/* new NSOutline View for the presets */
    NSArray                      *fDraggedNodes;
    IBOutlet NSOutlineView       * fPresetsOutlineView;
    IBOutlet NSButton            * fPresetsAdd;
	IBOutlet NSButton            * fPresetsDelete;
    IBOutlet MVMenuButton        * fPresetsActionButton;
    IBOutlet NSMenu              * fPresetsActionMenu;
	
    hb_handle_t                  * fHandle;
	hb_title_t                   * fTitle;
    /* integer to set to determine the previous state
		of encode 0==idle, 1==encoding, 2==cancelled*/
    int                            fEncodeState;
	int                            currentScanCount;
	int                            currentSuccessfulScanCount;
    BOOL                           SuccessfulScan;
	NSString                      * currentSource;
    NSString                     * browsedSourceDisplayName;
}
- (void) writeToActivityLog:(char *) format, ...;
- (IBAction) browseSources: (id) sender;
- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (IBAction) showSourceTitleScanPanel: (id) sender;
- (IBAction) closeSourceTitleScanPanel: (id) sender;  
- (void) performScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum;
- (IBAction) showNewScan: (id) sender;

- (void)     TranslateStrings;

- (void)     updateUI: (NSTimer *) timer;
- (void)     enableUI: (bool) enable;

- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;

- (IBAction) formatPopUpChanged: (id) sender;
- (IBAction) codecsPopUpChanged: (id) sender;
- (IBAction) encoderPopUpChanged: (id) sender;
- (IBAction) autoSetM4vExtension: (id) sender;
- (IBAction) twoPassCheckboxChanged: (id) sender;
- (IBAction) videoFrameRateChanged: (id) sender;
- (IBAction) setEnabledStateOfAudioMixdownControls: (id) sender;
- (IBAction) addAllAudioTracksToPopUp: (id) sender;
- (IBAction) selectAudioTrackInPopUp: (id) sender searchPrefixString: (NSString *) searchPrefixString selectIndexIfNotFound: (int) selectIndexIfNotFound;
- (IBAction) audioTrackPopUpChanged: (id) sender;
- (IBAction) audioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse;
- (IBAction) audioTrackMixdownChanged: (id) sender;
- (IBAction) subtitleSelectionChanged: (id) sender;

- (IBAction) browseFile: (id) sender;
- (void)     browseFileDone: (NSSavePanel *) sheet
                 returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) videoMatrixChanged: (id) sender;
- (IBAction) qualitySliderChanged: (id) sender;
- (IBAction) audioDRCSliderChanged: (id) sender;

- (IBAction) showPicturePanel: (id) sender;
- (IBAction) calculatePictureSizing: (id) sender;
- (IBAction) openMainWindow: (id) sender;

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

/* We use this to actually select the preset and act accordingly */
- (IBAction)selectPreset:(id)sender;    

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


    // Growl methods
- (NSDictionary *) registrationDictionaryForGrowl;
-(IBAction)showGrowlDoneNotification:(id)sender;
- (IBAction)showDebugOutputPanel:(id)sender;
- (void)setupToolbar;

- (void) remindUserOfSleepOrShutdown;
@end

