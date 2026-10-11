//
//  SPUUserDriver.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/14/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SPUUserUpdateState.h"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SPUUserUpdateState.h>
#import <Sparkle/SUExport.h>
#endif

NS_ASSUME_NONNULL_BEGIN

@class SPUUpdatePermissionRequest, SUUpdatePermissionResponse, SUAppcastItem, SPUDownloadData;

/**
 The API in Sparkle for controlling the user interaction.
 
 This protocol is used for implementing a user interface for the Sparkle updater. Sparkle's internal drivers tell
 an object that implements this protocol what actions to take and show to the user.
 
 Every method in this protocol can be assumed to be called from the main thread.
 */
SU_EXPORT NS_SWIFT_UI_ACTOR @protocol SPUUserDriver <NSObject>

/**
 * Show an updater permission request to the user
 *
 * Ask the user for their permission regarding update checks.
 * This is typically only called once per app installation.
 *
 * @param request The update permission request.
 * @param reply A reply with a update permission response.
 */
- (void)showUpdatePermissionRequest:(SPUUpdatePermissionRequest *)request reply:(void (^)(SUUpdatePermissionResponse *))reply;

/**
 * Show the user initiating an update check
 *
 * Respond to the user initiating an update check. Sparkle uses this to show the user a window with an indeterminate progress bar.
 *
 * @param cancellation Invoke this cancellation block to cancel the update check before the update check is completed.
 */
- (void)showUserInitiatedUpdateCheckWithCancellation:(void (^)(void))cancellation;

/**
 * Show the user a new update is found.
 *
 * Let the user know a new update is found and ask them what they want to do.
 * Before this point, `-showUserInitiatedUpdateCheckWithCancellation:` may be called.
 *
 *  The potential  `stage`s on the updater @c state are:
 *
 *  `SPUUpdateStateNotDownloaded` - Update has not been downloaded yet.
 *
 *  `SPUUpdateStateDownloaded` - Update has already been downloaded in the background automatically (via `SUAutomaticallyUpdate`) but not started installing yet.
 *
 *  `SPUUpdateStateInstalling` - Update has been downloaded and already started installing.
 *
 *  The `userInitiated` property on the @c state indicates if the update was initiated by the user or if it was automatically scheduled in the background.
 *
 *  Additionally, these properties on the @c appcastItem are of importance:
 *
 *  @c appcastItem.informationOnlyUpdate indicates if the update is only informational and should not be downloaded. You can direct the user to the infoURL property of the appcastItem in their web browser. Sometimes information only updates are used as a fallback in case a bad update is shipped, so you'll want to support this case.
 *
 *  @c appcastItem.majorUpgrade indicates if the update is a major or paid upgrade.
 *
 *  @c appcastItem.criticalUpdate indicates if the update is a critical update.
 *
 *  @c appcastItem.signingValidationStatus indicates the signing validation status of the appcast, which may be applicable if appcast signing is required.
 *
 * A reply of `SPUUserUpdateChoiceInstall` begins or resumes downloading, extracting, or installing the update.
 * If the state.stage is `SPUUserUpdateStateInstalling`, this may send a quit event to the application and relaunch it immediately (in this state, this behaves as a fast "install and Relaunch").
 * If the state.stage is `SPUUpdateStateNotDownloaded` or `SPUUpdateStateDownloaded` the user may be presented an authorization prompt to install the update after `-showDownloadDidStartExtractingUpdate` is called if authorization is required for installation. For example, this may occur if the update on disk is owned by a different user (e.g. root or admin for non-admin users), or if the update is a package install.
 * Do not use a reply of `SPUUserUpdateChoiceInstall` if @c appcastItem.informationOnlyUpdate is YES.
 *
 * A reply of `SPUUserUpdateChoiceDismiss` dismisses the update for the time being. The user may be reminded of the update at a later point.
 * If the state.stage is `SPUUserUpdateStateDownloaded`, the downloaded update is kept after dismissing until the next time an update is shown to the user.
 * If the state.stage is `SPUUserUpdateStateInstalling`, the installing update is also preserved after dismissing. In this state however, the update will also still be installed after the application is terminated.
 *
 * A reply of `SPUUserUpdateChoiceSkip` skips this particular version and won't notify the user again, unless they initiate an update check themselves.
 * If @c appcastItem.majorUpgrade is YES, the major update and any future minor updates to that major release are skipped, unless a future minor update specifies a `<sparkle:ignoreSkippedUpgradesBelowVersion>` requirement.
 * If the state.stage is `SPUUpdateStateInstalling`, the installation is also canceled when the update is skipped.
 *
 * @param appcastItem The Appcast Item containing information that reflects the new update.
 * @param state The current state of the user update. See above discussion for notable properties.
 * @param reply The reply which indicates if the update should be installed, dismissed, or skipped. See above discussion for more details.
 */
- (void)showUpdateFoundWithAppcastItem:(SUAppcastItem *)appcastItem state:(SPUUserUpdateState *)state reply:(void (^)(SPUUserUpdateChoice))reply;

/**
 * Show the user the release notes for the new update
 *
 * Display the release notes to the user. This will be called after showing the new update.
 * This is only applicable if the release notes are linked from the appcast, and are not directly embedded inside of the appcast file.
 * That is, this may be invoked if the releaseNotesURL from the appcast item is non-nil.
 *
 * @param downloadData The data for the release notes that was downloaded from the new update's appcast.
 */
- (void)showUpdateReleaseNotesWithDownloadData:(SPUDownloadData *)downloadData;

/**
 * Show the user that the new update's release notes could not be downloaded
 *
 * This will be called after showing the new update.
 * This is only applicable if the release notes are linked from the appcast, and are not directly embedded inside of the appcast file.
 * That is, this may be invoked if the releaseNotesURL from the appcast item is non-nil.
 *
 * @param error The error associated with why the new update's release notes could not be downloaded.
 */
- (void)showUpdateReleaseNotesFailedToDownloadWithError:(NSError *)error;

/**
 * Show the user a new update was not found
 *
 * Let the user know a new update was not found after they tried initiating an update check.
 * Before this point, `-showUserInitiatedUpdateCheckWithCancellation:` may be called.
 *
 * There are various reasons a new update is unavailable and can't be installed.
 * The @c error object is populated with recovery and suggestion strings suitable to be shown in an alert.
 *
 * The @c userInfo dictionary on the @c error is also populated with two keys:
 *
 * `SPULatestAppcastItemFoundKey`: if available, this may provide the latest SUAppcastItem that was found.
 *
 * `SPUNoUpdateFoundReasonKey`: if available, this will provide the `SUNoUpdateFoundReason`. For example the reason could be because
 * the latest version in the feed requires a newer OS version or could be because the user is already on the latest version.
 *
 * @param error The error associated with why a new update was not found. See above discussion for more details.
 * @param acknowledgement Acknowledge to the updater that no update found error was shown.
 */
- (void)showUpdateNotFoundWithError:(NSError *)error acknowledgement:(void (^)(void))acknowledgement NS_SWIFT_ASYNC(2);

/**
 * Show the user an update error occurred
 *
 * Let the user know that the updater failed with an error. This will not be invoked without the user having been
 * aware that an update was in progress.
 *
 * Before this point, any of the non-error user driver methods may have been invoked.
 *
 * @param error The error associated with what update error occurred.
 * @param acknowledgement Acknowledge to the updater that the error was shown.
 */
- (void)showUpdaterError:(NSError *)error acknowledgement:(void (^)(void))acknowledgement NS_SWIFT_ASYNC(2);

/**
 * Show the user that downloading the new update initiated
 *
 * Let the user know that downloading the new update started.
 *
 * @param cancellation Invoke this cancellation block to cancel the download at any point before `-showDownloadDidStartExtractingUpdate` is invoked.
 */
- (void)showDownloadInitiatedWithCancellation:(void (^)(void))cancellation;

/**
 * Show the user the content length of the new update that will be downloaded
 *
 * @param expectedContentLength The expected content length of the new update being downloaded.
 * An implementor should be able to handle if this value is invalid (more or less than actual content length downloaded).
 * Additionally, this method may be called more than once for the same download in rare scenarios.
 */
- (void)showDownloadDidReceiveExpectedContentLength:(uint64_t)expectedContentLength;

/**
 * Show the user that the update download received more data
 *
 * This may be an appropriate time to advance a visible progress indicator of the download
 * @param length The length of the data that was just downloaded
 */
- (void)showDownloadDidReceiveDataOfLength:(uint64_t)length;

/**
 * Show the user that the update finished downloading and started extracting
 *
 * Sparkle uses this to show an indeterminate progress bar.
 *
 * Before this point, `showDownloadDidReceiveDataOfLength:` or `showUpdateFoundWithAppcastItem:state:reply:` may be called.
 * An update can potentially resume at this point after having been automatically downloaded in the background (without the user driver)  before.
 *
 * After extraction starts, the user may be shown an authorization prompt to install the update if authorization is required for installation.
 * For example, this may occur if the update on disk is owned by a different user (e.g. root or admin for non-admin users), or if the update is a package install.
 */
- (void)showDownloadDidStartExtractingUpdate;

/**
 * Show the user that the update is extracting with progress
 *
 * Let the user know how far along the update extraction is.
 *
 * Before this point, `-showDownloadDidStartExtractingUpdate` is called.
 *
 * @param progress The progress of the extraction from a 0.0 to 1.0 scale
 */
- (void)showExtractionReceivedProgress:(double)progress;

/**
 * Show the user that the update is ready to install & relaunch
 *
 * Let the user know that the update is ready to install and relaunch, and ask them whether they want to proceed.
 * Note if the target application has already terminated, this method may not be invoked.
 *
 * A reply of `SPUUserUpdateChoiceInstall` installs the update the new update immediately. The application is relaunched only if it is still running by the time this reply is invoked. If the application terminates on its own, Sparkle will attempt to automatically install the update.
 *
 * A reply of `SPUUserUpdateChoiceDismiss` dismisses the update installation for the time being. Note the update may still be installed automatically after the application terminates.
 *
 * A reply of `SPUUserUpdateChoiceSkip` cancels the current update that has begun installing and dismisses the update. In this circumstance, the update is canceled but this update version is not skipped in the future.
 *
 * Before this point, `-showExtractionReceivedProgress:` or  `-showUpdateFoundWithAppcastItem:state:reply:` may be called.
 *
 * @param reply The reply which indicates if the update should be installed, dismissed, or skipped. See above discussion for more details.
 */
- (void)showReadyToInstallAndRelaunch:(void (^)(SPUUserUpdateChoice))reply;

/**
 * Show the user that the update is installing
 *
 * Let the user know that the update is currently installing.
 *
 * Before this point, `-showReadyToInstallAndRelaunch:` or  `-showUpdateFoundWithAppcastItem:state:reply:` will be called.
 *
 * @param applicationTerminated Indicates if the application has been terminated already.
 * If the application hasn't been terminated, a quit event is sent to the running application before installing the update.
 * If the application or user delays or cancels termination, there may be an indefinite period of time before the application fully quits.
 * It is up to the implementor whether or not to decide to continue showing installation progress in this case.
 *
 * @param retryTerminatingApplication This handler gives a chance for the application to re-try sending a quit event to the running application before installing the update.
 * The application may cancel or delay termination. This handler gives the user driver another chance to allow the user to try terminating the application again.
 * If the application does not delay or cancel application termination, there is no need to invoke this handler. This handler may be invoked multiple times.
 * Note this handler should not be invoked if @c applicationTerminated is already @c YES
 */
- (void)showInstallingUpdateWithApplicationTerminated:(BOOL)applicationTerminated retryTerminatingApplication:(void (^)(void))retryTerminatingApplication;

/**
 * Show the user that the update installation finished
 *
 * Let the user know that the update finished installing.
 *
 * This will only be invoked if the updater process is still alive, which is typically not the case if
 * the updater's lifetime is tied to the application it is updating. This implementation must not try to reference
 * the old bundle prior to the installation, which will no longer be around.
 *
 * Before this point, `-showInstallingUpdateWithApplicationTerminated:retryTerminatingApplication:` will be called.
 *
 * @param relaunched Indicates if the update was relaunched.
 * @param acknowledgement Acknowledge to the updater that the finished installation was shown.
 */
- (void)showUpdateInstalledAndRelaunched:(BOOL)relaunched acknowledgement:(void (^)(void))acknowledgement NS_SWIFT_ASYNC(2);

/**
 * Dismiss the current update installation
 *
 * Stop and tear down everything.
 * Dismiss all update windows, alerts, progress, etc from the user.
 * Basically, stop everything that could have been started. Sparkle may invoke this when aborting or finishing an update.
 */
- (void)dismissUpdateInstallation;

@optional

/**
 * Show the user the current presented update or its progress in utmost focus
 *
 * The user wishes to check for updates while the user is being shown update progress.
 * Bring whatever is on screen to frontmost focus (permission request, update information, downloading or extraction status, choice to install update, etc).
 * Implementing this method is optional.
 */
- (void)showUpdateInFocus;

/*
 * Below are deprecated methods that have been replaced by better alternatives.
 * The deprecated methods will be used if the alternatives have not been implemented yet.
 * In the future support for using these deprecated methods may be removed however.
 */

// Clients should move to non-deprecated methods
// Deprecated methods are only (temporarily) kept around for compatibility reasons

- (void)showUpdateNotFoundWithAcknowledgement:(void (^)(void))acknowledgement __deprecated_msg("Implement -showUpdateNotFoundWithError:acknowledgement: instead");

- (void)showUpdateInstallationDidFinishWithAcknowledgement:(void (^)(void))acknowledgement __deprecated_msg("Implement -showUpdateInstalledAndRelaunched:acknowledgement: instead");

- (void)dismissUserInitiatedUpdateCheck __deprecated_msg("Transition to new UI appropriately when a new update is shown, when no update is found, or when an update error occurs.");

- (void)showInstallingUpdate __deprecated_msg("Implement -showInstallingUpdateWithApplicationTerminated:retryTerminatingApplication: instead.");

- (void)showSendingTerminationSignal __deprecated_msg("Implement -showInstallingUpdateWithApplicationTerminated:retryTerminatingApplication: instead.");

- (void)showInstallingUpdateWithApplicationTerminated:(BOOL)applicationTerminated __deprecated_msg("Implement -showInstallingUpdateWithApplicationTerminated:retryTerminatingApplication: instead.");;

@end

NS_ASSUME_NONNULL_END
