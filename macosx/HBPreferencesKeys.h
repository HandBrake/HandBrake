/*  HBPreferencesKeys.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#ifndef HBPreferencesKeys_h
#define HBPreferencesKeys_h

typedef NS_ENUM(NSUInteger, HBDoneAction) {
    HBDoneActionDoNothing = 0,
    HBDoneActionSleep = 1,
    HBDoneActionShutDown = 2,
    HBDoneActionQuit = 3,
};

extern NSString * const HBShowOpenPanelAtLaunch;
extern NSString * const HBShowSummaryPreview;
extern NSString * const HBKeepPresetEdits;
extern NSString * const HBUseSourceFolderDestination;

extern NSString * const HBRecursiveScan;
extern NSString * const HBExcludedFileExtensions;
extern NSString * const HBLastDestinationDirectoryURL;
extern NSString * const HBLastDestinationDirectoryBookmark;
extern NSString * const HBLastSourceDirectoryURL;

extern NSString * const HBDefaultAutoNaming;
extern NSString * const HBAutoNamingFormat;
extern NSString * const HBAutoNamingRemoveUnderscore;
extern NSString * const HBAutoNamingRemovePunctuation;
extern NSString * const HBAutoNamingTitleCase;
extern NSString * const HBAutoNamingISODateFormat;

extern NSString * const HBDefaultMpegExtension;

extern NSString * const HBCqSliderFractional;
extern NSString * const HBUseDvdNav;
extern NSString * const HBUseHardwareDecoder;
extern NSString * const HBAlwaysUseHardwareDecoder;
extern NSString * const HBMinTitleScanSeconds;
extern NSString * const HBPreviewsNumber;

extern NSString * const HBLoggingLevel;
extern NSString * const HBEncodeLogLocation;
extern NSString * const HBClearOldLogs;

extern NSString * const HBQueuePauseIfLowSpace;
extern NSString * const HBQueueMinFreeSpace;
extern NSString * const HBQueuePauseOnBatteryPower;

extern NSString * const HBQueueAutoClearCompletedItems;
extern NSString * const HBQueueAutoClearCompletedItemsAtLaunch;

extern NSString * const HBQueueWorkerCounts;

extern NSString * const HBResetWhenDoneOnLaunch;
extern NSString * const HBQueueDoneAction;
extern NSString * const HBQueueNotificationWhenDone;
extern NSString * const HBQueueNotificationWhenJobDone;
extern NSString * const HBQueueNotificationPlaySound;
extern NSString * const HBSendToAppEnabled;
extern NSString * const HBSendToApp;

#endif /* HBPreferencesKeys_h */
