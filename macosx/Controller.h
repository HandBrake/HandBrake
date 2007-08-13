/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>
#include <Growl/Growl.h>

#include "hb.h"


#include "ChapterTitles.h"
#include "ScanController.h"
#include "PictureController.h"
#include "QueueController.h"
#import "MVMenuButton.h"
@class HBOutputPanelController;

@interface HBController : NSObject <GrowlApplicationBridgeDelegate>

{
    IBOutlet NSWindow            * fWindow;
	
    /* Scan panel */
	IBOutlet ScanController      * fScanController;
    IBOutlet NSPanel             * fScanPanel;
	
    /* Picture panel */
    IBOutlet PictureController   * fPictureController;
    IBOutlet NSPanel             * fPicturePanel;
	
    /* Queue panel */
    IBOutlet QueueController     * fQueueController;
    IBOutlet NSPanel             * fQueuePanel;
    IBOutlet NSTextField         * fQueueStatus;
	
    /* Source box */
	IBOutlet NSProgressIndicator * fScanIndicator;
	NSString                     * sourceDisplayName;
	IBOutlet NSTextField         * fScanStatus;
    IBOutlet NSTextField         * fSrcDVD1Field;
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
	IBOutlet NSTextField         * fPicSettingARkeepDsply;
	IBOutlet NSTextField         * fPicSettingPARDsply;
	IBOutlet NSTextField         * fPicSettingAutoCropLabel;
	IBOutlet NSTextField         * fPicSettingAutoCrop;
	IBOutlet NSTextField         * fPicSettingAutoCropDsply;
	IBOutlet NSTextField         * fPicSettingDeinterlaceDsply;
	IBOutlet NSTextField         * fPicSettingDetelecine;
	IBOutlet NSTextField         * fPicSettingDetelecineLabel;

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
	ChapterTitles       * fChapterTitlesDelegate;
	
    /* Bottom */
    IBOutlet NSButton            * fPictureButton;
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
    IBOutlet NSButton            * fShowQuButton;
    IBOutlet NSButton            * fAddToQuButton;
    IBOutlet NSButton            * fPauseButton;
    IBOutlet NSButton            * fRipButton;
	
	/* Advanced Tab for opts fX264optView*/
	IBOutlet NSView              * fX264optView;
	IBOutlet NSTextField         * fX264optViewTitleLabel;
	IBOutlet NSTextField         * fDisplayX264OptionsLabel;
	IBOutlet NSTextField         * fDisplayX264Options;
	IBOutlet NSTextField         * fX264optBframesLabel;
	IBOutlet NSPopUpButton       * fX264optBframesPopUp;
	IBOutlet NSTextField         * fX264optRefLabel;
	IBOutlet NSPopUpButton       * fX264optRefPopUp;
	IBOutlet NSTextField         * fX264optNfpskipLabel;
	IBOutlet NSButton            * fX264optNfpskipSwitch;
	IBOutlet NSTextField         * fX264optNodctdcmtLabel;
	IBOutlet NSButton            * fX264optNodctdcmtSwitch;
	IBOutlet NSTextField         * fX264optSubmeLabel;
	IBOutlet NSPopUpButton       * fX264optSubmePopUp;
	IBOutlet NSTextField         * fX264optTrellisLabel;
	IBOutlet NSPopUpButton       * fX264optTrellisPopUp;
    IBOutlet NSTextField         * fX264optMixedRefsLabel;
    IBOutlet NSButton            * fX264optMixedRefsSwitch;
    IBOutlet NSTextField         * fX264optMotionEstLabel;
    IBOutlet NSPopUpButton       * fX264optMotionEstPopUp;
    IBOutlet NSTextField         * fX264optMERangeLabel;
    IBOutlet NSPopUpButton       * fX264optMERangePopUp;
    IBOutlet NSTextField         * fX264optWeightBLabel;
    IBOutlet NSButton            * fX264optWeightBSwitch;
    IBOutlet NSTextField         * fX264optBRDOLabel;
    IBOutlet NSButton            * fX264optBRDOSwitch;
    IBOutlet NSTextField         * fX264optBPyramidLabel;
    IBOutlet NSButton            * fX264optBPyramidSwitch;
    IBOutlet NSTextField         * fX264optBiMELabel;
    IBOutlet NSButton            * fX264optBiMESwitch;
    IBOutlet NSTextField         * fX264optDirectPredLabel;
    IBOutlet NSPopUpButton       * fX264optDirectPredPopUp;
    IBOutlet NSTextField         * fX264optDeblockLabel;
    IBOutlet NSPopUpButton       * fX264optAlphaDeblockPopUp;
    IBOutlet NSPopUpButton       * fX264optBetaDeblockPopUp;
    IBOutlet NSTextField         * fX264optAnalyseLabel;
    IBOutlet NSPopUpButton       * fX264optAnalysePopUp;
    IBOutlet NSTextField         * fX264opt8x8dctLabel;
    IBOutlet NSButton            * fX264opt8x8dctSwitch;
    IBOutlet NSTextField         * fX264optCabacLabel;
    IBOutlet NSButton            * fX264optCabacSwitch;
    
	/* User Preset variables here */
	
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSTextField         * fPresetNewDesc;
	IBOutlet NSPopUpButton       * fPresetNewPicSettingsPopUp;
	IBOutlet NSTextField         * fPresetSelectedDisplay;
	
	NSString                     * AppSupportDirectory;
	NSString                     * UserPresetsFile;
	NSString                     * x264ProfilesFile;
	NSMutableArray               * UserPresets;
	NSMutableArray               * x264Profiles;
	NSMutableArray               * UserPresetssortedArray;
	NSMutableDictionary          * chosenPreset;
    int                            curUserPresetChosenNum;
	 
	int                            presetHbDefault; // this is 1 in "Default" preset key
	int                            presetUserDefault;// this is 2 in "Default" preset key
    IBOutlet NSPanel             * fAddPresetPanel;
	IBOutlet NSTableView         * tableView;
	IBOutlet NSButton            * fPresetsAdd;
	IBOutlet NSButton            * fPresetsDelete;
    IBOutlet MVMenuButton           * fPresetsActionButton;
    IBOutlet NSMenu                         * fPresetsActionMenu;
	
    hb_handle_t                  * fHandle;
	hb_title_t                   * fTitle;
    /* integer to set to determine the previous state
		of encode 0==idle, 1==encoding, 2==cancelled*/
    int                            fEncodeState;
	int                            currentScanCount;
	int                            currentSuccessfulScanCount;
	NSString                      * currentSource;
	HBOutputPanelController *outputPanel;
	
	BOOL                         startButtonEnabled;
    BOOL                         pauseButtonEnabled;
    BOOL                         AddToQueueButtonEnabled;
	BOOL                         stopOrStart;
	BOOL                         resumeOrPause;
}

- (void)     TranslateStrings;

- (void)     UpdateUI: (NSTimer *) timer;
- (void)     EnableUI: (bool) enable;
- (IBAction) ShowNewScan: (id) sender;
- (IBAction) ShowScanPanel: (id) sender;

- (IBAction) TitlePopUpChanged: (id) sender;
- (IBAction) ChapterPopUpChanged: (id) sender;

- (IBAction) FormatPopUpChanged: (id) sender;
- (IBAction) CodecsPopUpChanged: (id) sender;
- (IBAction) EncoderPopUpChanged: (id) sender;
- (IBAction) TwoPassCheckboxChanged: (id) sender;
- (IBAction ) VideoFrameRateChanged: (id) sender;
- (IBAction) SetEnabledStateOfAudioMixdownControls: (id) sender;
- (IBAction) AddAllAudioTracksToPopUp: (id) sender;
- (IBAction) SelectAudioTrackInPopUp: (id) sender searchPrefixString: (NSString *) searchPrefixString selectIndexIfNotFound: (int) selectIndexIfNotFound;
- (IBAction) AudioTrackPopUpChanged: (id) sender;
- (IBAction) AudioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse;
- (IBAction) AudioTrackMixdownChanged: (id) sender;

- (IBAction) BrowseFile: (id) sender;
- (void)     BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) VideoMatrixChanged: (id) sender;
- (IBAction) QualitySliderChanged: (id) sender;

- (IBAction) ShowPicturePanel: (id) sender;
- (IBAction) CalculatePictureSizing: (id) sender;
- (IBAction) OpenMainWindow: (id) sender;

- (IBAction) AddToQueue: (id) sender;
- (IBAction) ShowQueuePanel: (id) sender;

- (IBAction) Rip: (id) sender;
- (void)     OverwriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     UpdateAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;
- (void)     _Rip;
- (IBAction) Cancel: (id) sender;
- (void)     _Cancel: (NSWindow *) sheet returnCode: (int) returnCode
    contextInfo: (void *) contextInfo;
- (IBAction) Pause: (id) sender;

- (IBAction) CalculateBitrate: (id) sender;
- (void) controlTextDidBeginEditing: (NSNotification *) notification;
- (void) controlTextDidEndEditing: (NSNotification *) notification;
- (void) controlTextDidChange: (NSNotification *) notification;

- (IBAction) OpenHomepage: (id) sender;
- (IBAction) OpenForums:   (id) sender;
- (IBAction) OpenUserGuide:   (id) sender;

// x264 Advanced Panel Methods
- (IBAction) X264AdvancedOptionsSet: (id) sender;
- (IBAction) X264AdvancedOptionsStandardizeOptString: (id) sender;
- (IBAction) X264AdvancedOptionsSetCurrentSettings: (id) sender;
- (NSString *)  X264AdvancedOptionsStandardizeOptNames:(NSString *) cleanOptNameString;
- (IBAction) X264AdvancedOptionsChanged: (id) sender;

// Preset Methods Here
- (IBAction) CustomSettingUsed: (id) sender;
- (IBAction) ShowAddPresetPanel: (id) sender;
- (IBAction) CloseAddPresetPanel: (id) sender;
- (NSDictionary *)CreatePreset;
- (NSDictionary *)CreateIpodLowPreset;
- (NSDictionary *)CreateIpodHighPreset;
- (NSDictionary *)CreateAppleTVPreset;
- (NSDictionary *)CreatePSThreePreset;  
- (NSDictionary *)CreatePSPPreset;
- (NSDictionary *)CreateNormalPreset;
- (NSDictionary *)CreateClassicPreset;
- (NSDictionary *)CreateQuickTimePreset;
- (NSDictionary *)CreateFilmPreset;
- (NSDictionary *)CreateTelevisionPreset;
- (NSDictionary *)CreateAnimationPreset;
- (NSDictionary *)CreateBedlamPreset;
- (NSDictionary *)CreateiPhonePreset;
- (NSDictionary *)CreateDeuxSixQuatrePreset;
- (NSDictionary *)CreateBrokePreset;
- (NSDictionary *)CreateBlindPreset;

- (IBAction) RevertPictureSizeToMax:(id)sender;

- (IBAction)SetDefaultPreset:(id)sender;
- (IBAction)SelectDefaultPreset:(id)sender;
- (void) savePreset;
- (IBAction)AddFactoryPresets:(id)sender;
- (IBAction)DeleteFactoryPresets:(id)sender;
- (IBAction)AddUserPreset:(id)sender;
- (void)AddPreset;
- (IBAction)InsertPreset:(id)sender;
- (IBAction)DeletePreset:(id)sender;
- (IBAction)GetDefaultPresets:(id)sender;
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

