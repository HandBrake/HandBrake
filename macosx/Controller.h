/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"

#include "ScanController.h"
#include "PictureController.h"
#include "QueueController.h"

@interface HBController : NSObject

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
    IBOutlet NSButton            * fQueueCheck;
    IBOutlet NSButton            * fQueueAddButton;
    IBOutlet NSButton            * fQueueShowButton;

    /* Source box */
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
    IBOutlet NSTextField         * fDstCodecsField;
    IBOutlet NSPopUpButton       * fDstCodecsPopUp;
    IBOutlet NSTextField         * fDstFile1Field;
    IBOutlet NSTextField         * fDstFile2Field;
    IBOutlet NSButton            * fDstBrowseButton;
	IBOutlet NSButton            * fCreateChapterMarkers;

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
	IBOutlet NSTextField         * fPicSettingDeinterlaceDsply;
	IBOutlet NSTextField         * fPicLabelAnamorphic;
	IBOutlet NSTextField         * fPicLabelPAROutp;
	IBOutlet NSTextField         * fPicLabelPAROutputX;
	IBOutlet NSTextField         * fPicSettingPARWidth;
	IBOutlet NSTextField         * fPicSettingPARHeight;
	
	
    /* Subtitles box */
    IBOutlet NSTextField         * fSubField;
    IBOutlet NSPopUpButton       * fSubPopUp;

    /* Audio box */
    IBOutlet NSTextField         * fAudLang1Field;
    IBOutlet NSPopUpButton       * fAudLang1PopUp;
    IBOutlet NSButton		     * fAudLang1SurroundCheck;
    IBOutlet NSTextField         * fAudLang2Field;
    IBOutlet NSPopUpButton       * fAudLang2PopUp;
    IBOutlet NSTextField         * fAudRateField;
    IBOutlet NSPopUpButton       * fAudRatePopUp;
    IBOutlet NSTextField         * fAudBitrateField;
    IBOutlet NSPopUpButton       * fAudBitratePopUp;

    /* Bottom */
    IBOutlet NSButton            * fPictureButton;
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
    IBOutlet NSButton            * fShowQuButton;
    IBOutlet NSButton            * fAddToQuButton;
    IBOutlet NSButton            * fPauseButton;
    IBOutlet NSButton            * fRipButton;

	/* User Preset variables here fPresetNewPicSettingsApply*/
	
	IBOutlet NSDrawer            * fPresetDrawer;
	IBOutlet NSTextField         * fPresetNewName;
	IBOutlet NSButton            * fPresetNewPicSettingsApply;
	IBOutlet NSTextField         * fPresetSelectedDisplay;
	
	NSString * AppSupportDirectory;
	NSString * UserPresetsFile;
	NSString * x264ProfilesFile;
	NSMutableArray * UserPresets;
	NSMutableArray * x264Profiles;
	NSMutableArray *UserPresetssortedArray;
	NSMutableDictionary * chosenPreset;

	
    IBOutlet NSPanel            * fAddPresetPanel;
	IBOutlet NSTableView         * tableView;
	IBOutlet NSButton            * fPresetsAdd;
	IBOutlet NSButton            * fPresetsDelete;
    hb_handle_t                  * fHandle;
	hb_title_t                   * fTitle;
}

- (void)     TranslateStrings;

- (void)     UpdateUI: (NSTimer *) timer;
- (void)     EnableUI: (bool) enable;

- (IBAction) ShowScanPanel: (id) sender;

- (IBAction) TitlePopUpChanged: (id) sender;
- (IBAction) ChapterPopUpChanged: (id) sender;

- (IBAction) FormatPopUpChanged: (id) sender;
- (IBAction) CodecsPopUpChanged: (id) sender;
- (IBAction) EncoderPopUpChanged: (id) sender;
- (IBAction) Check6ChannelAACExtraction: (id) sender;
- (IBAction) LanguagePopUpChanged: (id) sender;

- (IBAction) BrowseFile: (id) sender;
- (void)     BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;

- (IBAction) VideoMatrixChanged: (id) sender;
- (IBAction) QualitySliderChanged: (id) sender;

- (IBAction) ShowPicturePanel: (id) sender;
- (IBAction) CalculatePictureSizing: (id) sender;

- (IBAction) EnableQueue: (id) sender;
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

// Preset Methods Here

- (IBAction) ShowAddPresetPanel: (id) sender;
- (IBAction) CloseAddPresetPanel: (id) sender;
- (NSDictionary *)CreatePreset;
- (NSDictionary *)CreateIpodPreset;
- (NSDictionary *)CreateAppleTVPreset;  

- (void) savePreset;
- (IBAction)AddFactoryPresets:(id)sender;
- (IBAction)AddUserPreset:(id)sender;
- (void)AddPreset;
- (IBAction)InsertPreset:(id)sender;
- (IBAction)DeletePreset:(id)sender;
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



@end

