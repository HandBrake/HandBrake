/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>
#include <Growl/Growl.h>

#include "hb.h"


#include "ChapterTitles.h"
#include "PictureController.h"
#include "HBQueueController.h"
#import "MVMenuButton.h"
#import "HBAdvancedController.h"

@class HBOutputPanelController;

@interface HBController : NSObject <GrowlApplicationBridgeDelegate>

{
    IBOutlet NSWindow            * fWindow;
	
    /* Picture panel */
    IBOutlet PictureController   * fPictureController;
    IBOutlet NSPanel             * fPicturePanel;
	IBOutlet NSBox               * fAdvancedView;
    /* Queue panel */
    HBQueueController            * fQueueController;
    IBOutlet NSTextField         * fQueueStatus;
	
	/* Menu Items */
	/* File Menu */
	IBOutlet NSMenu * fMenuBarFileMenu;
	IBOutlet NSMenuItem * fMenuOpenSource;
	IBOutlet NSMenuItem * fMenuAddToQueue;
	IBOutlet NSMenuItem * fMenuStartEncode;
	IBOutlet NSMenuItem * fMenuPauseEncode;
	/* Window Menu */
	IBOutlet NSMenu * fMenuBarWindowMenu;
	IBOutlet NSMenuItem * fMenuPresetsDrawerToggle;
	IBOutlet NSMenuItem * fMenuQueuePanelShow;
	IBOutlet NSMenuItem * fMenuPicturePanelShow;
	IBOutlet NSMenuItem * fMenuActivityWindowShow;
	
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
	IBOutlet NSButton            * fDstMpgLargeFileCheck;
    IBOutlet NSTextField         * fDstCodecsField;
    IBOutlet NSPopUpButton       * fDstCodecsPopUp;
    IBOutlet NSTextField         * fDstFile1Field;
    IBOutlet NSTextField         * fDstFile2Field;
    IBOutlet NSButton            * fDstBrowseButton;
	
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
	IBOutlet NSTextField         * fPicLabelOutp;
	IBOutlet NSTextField         * fPicLabelAr;
	IBOutlet NSTextField         * fPicLabelDeinter;
	IBOutlet NSTextField         * fPicLabelSrcX;
	IBOutlet NSTextField         * fPicLabelOutputX;
	
	IBOutlet NSTextField         * fPicSrcWidth;
	IBOutlet NSTextField         * fPicSrcHeight;
	IBOutlet NSTextField         * fPicSettingWidth;
	IBOutlet NSTextField         * fPicSettingHeight;
	IBOutlet NSTextField         * fPicSettingARkeep;
	IBOutlet NSTextField         * fPicSettingPAR;
	IBOutlet NSTextField         * fPicSettingDeinterlace;
	IBOutlet NSTextField         * fPicSettingDeinterlaceDsply;
	IBOutlet NSTextField         * fPicSettingARkeepDsply;
	IBOutlet NSTextField         * fPicSettingPARDsply;
	IBOutlet NSTextField         * fPicSettingAutoCropLabel;
	IBOutlet NSTextField         * fPicSettingAutoCrop;
	IBOutlet NSTextField         * fPicSettingAutoCropDsply;
	IBOutlet NSTextField         * fPicSettingDetelecine;
	IBOutlet NSTextField         * fPicSettingDetelecineLabel;
	IBOutlet NSTextField         * fPicSettingDenoise;
	IBOutlet NSTextField         * fPicSettingDenoiseDsply;
	IBOutlet NSTextField         * fPicSettingDenoiseLabel;

	IBOutlet NSTextField         * fPicLabelAnamorphic;
	IBOutlet NSTextField         * fPicLabelPAROutp;
	IBOutlet NSTextField         * fPicLabelPAROutputX;
	IBOutlet NSTextField         * fPicSettingPARWidth;
	IBOutlet NSTextField         * fPicSettingPARHeight;
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
	IBOutlet NSTextField         * fAudTrack1MixLabel;
	IBOutlet NSPopUpButton       * fAudTrack1MixPopUp;
    IBOutlet NSTextField         * fAudTrack2MixLabel;
	IBOutlet NSPopUpButton       * fAudTrack2MixPopUp;
	
	IBOutlet NSTextField         * fAudRateField;
    IBOutlet NSPopUpButton       * fAudRatePopUp;
    IBOutlet NSTextField         * fAudBitrateField;
    IBOutlet NSPopUpButton       * fAudBitratePopUp;
    
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
	
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSTextField         * fPresetNewDesc;
	IBOutlet NSPopUpButton       * fPresetNewPicSettingsPopUp;
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
	IBOutlet NSTableView         * tableView;
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
    int                            SuccessfulScan;
	NSString                      * currentSource;
	HBOutputPanelController       *outputPanel;
    HBAdvancedController          *fAdvancedOptions;
	
    hb_job_t                     * fLastKnownCurrentJob;
	
    NSToolbar                    *toolbar;
}

- (void)     TranslateStrings;

- (void)     updateUI: (NSTimer *) timer;
- (void)     enableUI: (bool) enable;
- (IBAction) showNewScan: (id) sender;
- (IBAction) showScanPanel: (id) sender;
- (IBAction) browseSources: (id) sender;
- (void) browseSourcesDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
    
- (IBAction) titlePopUpChanged: (id) sender;
- (IBAction) chapterPopUpChanged: (id) sender;

- (IBAction) formatPopUpChanged: (id) sender;
- (IBAction) codecsPopUpChanged: (id) sender;
- (IBAction) encoderPopUpChanged: (id) sender;
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

- (IBAction) showPicturePanel: (id) sender;
- (IBAction) calculatePictureSizing: (id) sender;
- (IBAction) openMainWindow: (id) sender;

- (IBAction) addToQueue: (id) sender;
- (IBAction) showQueueWindow:(id)sender;

- (IBAction)showPreferencesWindow:(id)sender;

- (IBAction) Rip: (id) sender;
- (void)     overWriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     updateAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     _Rip;
- (IBAction) Cancel: (id) sender;
- (void)     _Cancel: (NSWindow *) sheet returnCode: (int) returnCode
    contextInfo: (void *) contextInfo;
- (IBAction) Pause: (id) sender;

- (IBAction) calculateBitrate: (id) sender;
- (void) controlTextDidBeginEditing: (NSNotification *) notification;
- (void) controlTextDidEndEditing: (NSNotification *) notification;
- (void) controlTextDidChange: (NSNotification *) notification;

- (IBAction) openHomepage: (id) sender;
- (IBAction) openForums:   (id) sender;
- (IBAction) openUserGuide:   (id) sender;

// Preset Methods Here
- (void) loadPresets;
- (IBAction) customSettingUsed: (id) sender;
- (IBAction) showAddPresetPanel: (id) sender;
- (IBAction) closeAddPresetPanel: (id) sender;
- (NSDictionary *)createPreset;
- (NSDictionary *)createIpodLowPreset;
- (NSDictionary *)createIpodHighPreset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createPSThreePreset;  
- (NSDictionary *)createPSPPreset;
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createClassicPreset;
- (NSDictionary *)createQuickTimePreset;
- (NSDictionary *)createFilmPreset;
- (NSDictionary *)createTelevisionPreset;
- (NSDictionary *)createAnimationPreset;
- (NSDictionary *)createBedlamPreset;
- (NSDictionary *)createiPhonePreset;
- (NSDictionary *)createDeuxSixQuatrePreset;
- (NSDictionary *)createBrokePreset;
- (NSDictionary *)createBlindPreset;
- (NSDictionary *)createCRFPreset;

- (IBAction) revertPictureSizeToMax:(id)sender;

- (IBAction)setDefaultPreset:(id)sender;
- (IBAction)selectDefaultPreset:(id)sender;
- (void) savePreset;
- (IBAction)addFactoryPresets:(id)sender;
- (IBAction)deleteFactoryPresets:(id)sender;
- (IBAction)addUserPreset:(id)sender;
- (void)addPreset;
- (IBAction)insertPreset:(id)sender;
- (IBAction)deletePreset:(id)sender;
- (IBAction)getDefaultPresets:(id)sender;
- (IBAction)tableViewSelected:(id)sender;

// NSTableDataSource methods
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(int)rowIndex;
- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(int)rowIndex;
// To determine user presets cell display properties
- (void)tableView:(NSTableView *)aTableView
		willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn
		 row:(int)rowIndex;

// Growl methods
- (NSDictionary *) registrationDictionaryForGrowl;
-(IBAction)showGrowlDoneNotification:(id)sender;
- (IBAction)showDebugOutputPanel:(id)sender;
- (void)setupToolbar;
@end

