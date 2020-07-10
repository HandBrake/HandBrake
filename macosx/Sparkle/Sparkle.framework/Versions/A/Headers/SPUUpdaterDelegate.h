//
//  SPUUpdaterDelegate.h
//  Sparkle
//
//  Created by Mayur Pawashe on 8/12/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Sparkle/SUExport.h>

@protocol SUVersionComparison;
@class SPUUpdater, SUAppcast, SUAppcastItem;

NS_ASSUME_NONNULL_BEGIN

// -----------------------------------------------------------------------------
// SUUpdater Notifications for events that might be interesting to more than just the delegate
// The updater will be the notification object
// -----------------------------------------------------------------------------
SU_EXPORT extern NSString *const SUUpdaterDidFinishLoadingAppCastNotification;
SU_EXPORT extern NSString *const SUUpdaterDidFindValidUpdateNotification;
SU_EXPORT extern NSString *const SUUpdaterDidNotFindUpdateNotification;
SU_EXPORT extern NSString *const SUUpdaterWillRestartNotification;
#define SUUpdaterWillRelaunchApplicationNotification SUUpdaterWillRestartNotification;
#define SUUpdaterWillInstallUpdateNotification SUUpdaterWillRestartNotification;

// Key for the SUAppcastItem object in the SUUpdaterDidFindValidUpdateNotification userInfo
SU_EXPORT extern NSString *const SUUpdaterAppcastItemNotificationKey;
// Key for the SUAppcast object in the SUUpdaterDidFinishLoadingAppCastNotification userInfo
SU_EXPORT extern NSString *const SUUpdaterAppcastNotificationKey;

// -----------------------------------------------------------------------------
//	SPUUpdater Delegate:
// -----------------------------------------------------------------------------

typedef NS_ENUM(NSInteger, SPUUpdateCheck)
{
    SPUUpdateCheckUserInitiated = 0,
    SPUUpdateCheckBackgroundScheduled = 1
};

/*!
 Provides methods to control the behavior of an SPUUpdater object.
 */
@protocol SPUUpdaterDelegate <NSObject>
@optional

/*!
 Called when a background update will be scheduled after a delay.
 
 Automatic update checks need to be enabled for this to trigger.
 
 \param delay The delay until the next scheduled update will occur.
 
 \param updater The updater instance.
 */
- (void)updater:(SPUUpdater *)updater willScheduleUpdateCheckAfterDelay:(NSTimeInterval)delay;

/*!
 Called when no updates will be scheduled in the future.
 
 This may later change if automatic update checks become enabled.
 
 \param updater The updater instance.
 */
- (void)updaterWillIdleSchedulingUpdates:(SPUUpdater *)updater;

/*!
 Returns whether to allow Sparkle to pop up.
 
 For example, this may be used to prevent Sparkle from interrupting a setup assistant.
 Alternatively, you may want to consider starting the updater after eg: the setup assistant finishes
 
 \param updater The updater instance.
 */
- (BOOL)updaterMayCheckForUpdates:(SPUUpdater *)updater;

/*!
 Returns additional parameters to append to the appcast URL's query string.
 
 This is potentially based on whether or not Sparkle will also be sending along the system profile.
 
 \param updater The updater instance.
 \param sendingProfile Whether the system profile will also be sent.
 
 \return An array of dictionaries with keys: "key", "value", "displayKey", "displayValue", the latter two being specifically for display to the user.
 */
#if __has_feature(objc_generics)
- (NSArray<NSDictionary<NSString *, NSString *> *> *)feedParametersForUpdater:(SPUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile;
#else
- (NSArray *)feedParametersForUpdater:(SPUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile;
#endif

/*!
 Returns a custom appcast URL.
 
 Override this to dynamically specify the entire URL.
 Alternatively you may want to consider adding a feed parameter using -feedParametersForUpdater:sendingSystemProfile:
 and having the server which appcast to serve.
 
 \param updater The updater instance.
 */
- (nullable NSString *)feedURLStringForUpdater:(SPUUpdater *)updater;

/*!
 Returns whether Sparkle should prompt the user about automatic update checks.
 
 Use this to override the default behavior.
 
 \param updater The updater instance.
 */
- (BOOL)updaterShouldPromptForPermissionToCheckForUpdates:(SPUUpdater *)updater;

/*!
 Called after Sparkle has downloaded the appcast from the remote server.
 
 Implement this if you want to do some special handling with the appcast once it finishes loading.
 
 \param updater The updater instance.
 \param appcast The appcast that was downloaded from the remote server.
 */
- (void)updater:(SPUUpdater *)updater didFinishLoadingAppcast:(SUAppcast *)appcast;

/*!
 Returns the item in the appcast corresponding to the update that should be installed.
 
 If you're using special logic or extensions in your appcast,
 implement this to use your own logic for finding a valid update, if any,
 in the given appcast.
 
 \param appcast The appcast that was downloaded from the remote server.
 \param updater The updater instance.
 \return The best valid appcast item, or nil if you don't want to be delegated this task.
 */
- (nullable SUAppcastItem *)bestValidUpdateInAppcast:(SUAppcast *)appcast forUpdater:(SPUUpdater *)updater;

/*!
 Called when a valid update is found by the update driver.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that is proposed to be installed.
 */
- (void)updater:(SPUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)item;

/*!
 Called when a valid update is not found.
 
 \param updater The updater instance.
 */
- (void)updaterDidNotFindUpdate:(SPUUpdater *)updater;

/*!
 Returns whether the release notes (if available) should be downloaded after an update is found and shown.
 
 This is specifically for the releaseNotesLink element in the appcast.
 
 \param updater The updater instance.
 
 \return \c YES to download and show the release notes if available, otherwise NO. The default behavior is YES.
 */
- (BOOL)updaterShouldDownloadReleaseNotes:(SPUUpdater *)updater;

/*!
 Called immediately before downloading the specified update.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that is proposed to be downloaded.
 \param request The mutable URL request that will be used to download the update.
 */
- (void)updater:(SPUUpdater *)updater willDownloadUpdate:(SUAppcastItem *)item withRequest:(NSMutableURLRequest *)request;

/*!
 Called after the specified update failed to download.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that failed to download.
 \param error The error generated by the failed download.
 */
- (void)updater:(SPUUpdater *)updater failedToDownloadUpdate:(SUAppcastItem *)item error:(NSError *)error;

/*!
 Called when the user clicks the cancel button while and update is being downloaded.
 
 \param updater The updater instance.
 */
- (void)userDidCancelDownload:(SPUUpdater *)updater;

/*!
 Called immediately before installing the specified update.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that is proposed to be installed.
 */
- (void)updater:(SPUUpdater *)updater willInstallUpdate:(SUAppcastItem *)item;

/*!
 Returns whether the relaunch should be delayed in order to perform other tasks.
 
 This is not called if the user didn't relaunch on the previous update,
 in that case it will immediately restart.
 
 This may also not be called if the application is not going to relaunch after it terminates.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that is proposed to be installed.
 \param installHandler The install handler that must be completed before continuing with the relaunch.
 
 \return \c YES to delay the relaunch until \p installHandler is invoked.
 */
- (BOOL)updater:(SPUUpdater *)updater shouldPostponeRelaunchForUpdate:(SUAppcastItem *)item untilInvokingBlock:(void (^)(void))installHandler;

/*!
 Returns whether the application should be relaunched at all.
 
 Some apps \b cannot be relaunched under certain circumstances.
 This method can be used to explicitly prevent a relaunch.
 
 \param updater The updater instance.
 \return YES if the updater should be relaunched, otherwise NO if it shouldn't.
 */
- (BOOL)updaterShouldRelaunchApplication:(SPUUpdater *)updater;

/*!
 Called immediately before relaunching.
 
 \param updater The updater instance.
 */
- (void)updaterWillRelaunchApplication:(SPUUpdater *)updater;

/*!
 Returns an object that compares version numbers to determine their arithmetic relation to each other.
 
 This method allows you to provide a custom version comparator.
 If you don't implement this method or return \c nil,
 the standard version comparator will be used. Note that the
 standard version comparator may be used during installation for preventing
 a downgrade, even if you provide a custom comparator here.
 
 \sa SUStandardVersionComparator
 
 \param updater The updater instance.
 \return The custom version comparator or nil if you don't want to be delegated this task.
 */
- (nullable id<SUVersionComparison>)versionComparatorForUpdater:(SPUUpdater *)updater;

/*!
 Returns whether or not the updater should allow interaction from the installer
 
 Use this to override the default behavior which is to allow interaction with the installer.
 
 If interaction is allowed, then an authorization prompt may show up to the user if they do
 not curently have sufficient privileges to perform the installation of the new update.
 The installer may also show UI and progress when interaction is allowed.
 
 On the other hand, if interaction is not allowed, then an installation may fail if the user does not
 have sufficient privileges to perform the installation. In this case, the feed and update may not even be downloaded.
 
 Note this has no effect if the update has already been downloaded in the background silently and ready to be resumed.
 
 \param updater The updater instance.
 \param updateCheck The type of update check being performed.
 */
- (BOOL)updater:(SPUUpdater *)updater shouldAllowInstallerInteractionForUpdateCheck:(SPUUpdateCheck)updateCheck;

/*!
 Returns the decryption password (if any) which is used to extract the update archive DMG.
 
 Return nil if no password should be used.
 
 \param updater The updater instance.
 \return The password used for decrypting the archive, or nil if no password should be used.
 */
- (nullable NSString *)decryptionPasswordForUpdater:(SPUUpdater *)updater;

/*!
 Called when an update is scheduled to be silently installed on quit after downloading the update automatically.
 
 \param updater The updater instance.
 \param item The appcast item corresponding to the update that is proposed to be installed.
 \param immediateInstallHandler The install handler to immediately install the update. No UI interaction will be shown and the application will be relaunched after installation.
 \return Return YES if the delegate will handle installing the update or NO if the updater should be given responsibility.
 
 If the updater is given responsibility, it can later remind the user an update is available if they have not terminated the application for a long time.
 Also if the updater is given responsibility and the update item is marked critical, the new update will be presented to the user immediately after.
 Even if the immediateInstallHandler is not invoked, the installer will attempt to install the update on termination.
 */
- (BOOL)updater:(SPUUpdater *)updater willInstallUpdateOnQuit:(SUAppcastItem *)item immediateInstallationBlock:(void (^)(void))immediateInstallHandler;

/*!
 Called after an update is aborted due to an error.
 
 \param updater The updater instance.
 \param error The error that caused the abort
 */
- (void)updater:(SPUUpdater *)updater didAbortWithError:(NSError *)error;

/*!
 Called after an update is aborted due to an error during an scheduled update check.
  
 \param updater The updater instance.
 \param error The error that caused the abort
 */
- (void)updater:(SPUUpdater *)updater scheduledUpdateCheckDidAbortWithError:(NSError *)error;

@end

NS_ASSUME_NONNULL_END
