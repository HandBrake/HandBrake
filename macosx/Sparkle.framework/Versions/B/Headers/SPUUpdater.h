//
//  SPUUpdater.h
//  Sparkle
//
//  Created by Andy Matuschak on 1/4/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#import "SPUUserDriver.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#import <Sparkle/SPUUserDriver.h>
#endif

NS_ASSUME_NONNULL_BEGIN

@class SUAppcastItem, SUAppcast;

@protocol SPUUpdaterDelegate;

/**
 The main API in Sparkle for controlling the update mechanism.

 This class is used to configure the update parameters as well as manually and automatically schedule and control checks for updates.
 
 For convenience, you can create a standard or nib instantiable updater by using `SPUStandardUpdaterController`.
 
 Prefer to set initial properties in your bundle's Info.plist as described in [Customizing Sparkle](https://sparkle-project.org/documentation/customization/).
 
 Otherwise only if you need dynamic behavior for user settings should you set properties on the updater such as:
 - `automaticallyChecksForUpdates`
 - `updateCheckInterval`
 - `automaticallyDownloadsUpdates`
 - `feedURL`
 
 Please view the documentation on each of these properties for more detail if you are to configure them dynamically.
 
 This class must be used on the main thread.
 */
SU_EXPORT NS_SWIFT_UI_ACTOR @interface SPUUpdater : NSObject

/**
 Initializes a new `SPUUpdater` instance
 
 This creates an updater, but to start it and schedule update checks `-startUpdater:` needs to be invoked first.
 
 Related: See `SPUStandardUpdaterController` which wraps a `SPUUpdater` instance and is suitable for instantiating inside of nib files.
 
 @param hostBundle The bundle that should be targeted for updating.
 @param applicationBundle The application bundle that should be waited for termination and relaunched (unless overridden). Usually this can be the same as hostBundle. This may differ when updating a plug-in or other non-application bundle.
 @param userDriver The user driver that Sparkle uses for user update interaction.
 @param delegate The delegate for `SPUUpdater`. Note the updater weakly references the delegate, so you are responsible for keeping it alive.
 */
- (instancetype)initWithHostBundle:(NSBundle *)hostBundle applicationBundle:(NSBundle *)applicationBundle userDriver:(id <SPUUserDriver>)userDriver delegate:(nullable id<SPUUpdaterDelegate>)delegate;

/**
 Use `-initWithHostBundle:applicationBundle:userDriver:delegate:` or `SPUStandardUpdaterController` standard adapter instead.
 
 If you want to drop an updater into a nib, use `SPUStandardUpdaterController`.
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Starts the updater.

 This method first checks if Sparkle is configured properly. A valid feed URL should be set before this method is invoked.

 If the configuration is valid, an update cycle is started in the next main runloop cycle.
 During this cycle, a permission prompt may be brought up (if needed) for checking if the user wants automatic update checking.
 Otherwise if automatic update checks are enabled, a scheduled update alert may be brought up if enough time has elapsed since the last check.
 See `automaticallyChecksForUpdates` for more information.

 After starting the updater and before the next runloop cycle, one of `-checkForUpdates`, `-checkForUpdatesInBackground`, or `-checkForUpdateInformation` can be invoked.
 This may be useful if you want to check for updates immediately or without showing a potential permission prompt.
 
 If the updater cannot be started (i.e, due to a configuration issue in the application), you may want to fall back appropriately.
 For example, the standard updater controller (`SPUStandardUpdaterController`) alerts the user that the app is misconfigured and to contact the developer.

 This must be called on the main thread.

 @param error The error that is populated if this method fails. Pass NULL if not interested in the error information.
 @return YES if the updater started otherwise NO with a populated error
 */
- (BOOL)startUpdater:(NSError * __autoreleasing *)error;

/**
 Checks for new updates, and displays progress while doing so if needed.
 
 This is meant for users initiating a new update check or checking the current update progress.
 
 If an update hasn't started, the user may be shown that a new check for updates is occurring.
 If an update has already been downloaded or begun installing from a previous session, the user may be presented to install that update.
 If the user is already being presented with an update or update permission prompt, that notice may be shown to the user in active focus
 (as long as the user driver is the standard `SPUStandardUserDriver` or if it implements `-[SPUUserDriver showUpdateInFocus]`).
 
 This will find updates that the user has previously opted into skipping.
 See `canCheckForUpdates` property which can determine when this method may be invoked.

 This must be called on the main thread.
 */
- (void)checkForUpdates;

/**
 Checks for new updates in the background.
 
 You usually should not call this method directly. By default Sparkle calls this method automatically
 on a scheduled basis if automatic update checks are enabled. This is done by checking the current state of
 `automaticallyChecksForUpdates`, `updateCheckInterval`,  `lastUpdateCheckDate`, and
 `SUScheduledImpatientCheckInterval`.
 
 If you want to additionally force an update check on every app launch though, it's recommended to only call this method immediately after starting the updater,
 and only when automatic update checks are enabled (by checking `automaticallyChecksForUpdates` is `YES`). Calling this method at later points
 could interfere with Sparkle's scheduler in unexpected ways.
 
 If you want to reset the updater's cycle after an updater setting change, please use `resetUpdateCycle` or `resetUpdateCycleAfterShortDelay` instead.
 
 Updates that are found may not be presented immediately to the user, either due to automatic downloading/installing of updates being on or
 due to gentle reminders https://sparkle-project.org/documentation/gentle-reminders/ for example.
 
 Updates that have been skipped by the user will not be found.
 
 This method does not do anything if there is a `sessionInProgress`.

 This must be called on the main thread.
 */
- (void)checkForUpdatesInBackground;

/**
 Begins a "probing" check for updates which will not actually offer to
 update to that version.
 
 However, the delegate methods
 `-[SPUUpdaterDelegate updater:didFindValidUpdate:]` and
 `-[SPUUpdaterDelegate updaterDidNotFindUpdate:]` will be called,
 so you can use that information in your UI.
 
 `-[SPUUpdaterDelegate updater:didFinishUpdateCycleForUpdateCheck:error:]` will be called when
 this probing check is completed.
 
 Updates that have been skipped by the user will not be found.
 
 This method does not do anything if there is a `sessionInProgress`.

 This must be called on the main thread.
 */
- (void)checkForUpdateInformation;

/**
 A property indicating whether or not updates can be checked by the user.
 
 An update check can be made by the user when an update session isn't in progress.
 An update check can also be made when an update or its progress is being shown to the user
 (as long as the user driver is the standard `SPUStandardUserDriver` or if it implements `-[SPUUserDriver showUpdateInFocus]`).
 
 A user cannot check for updates when data (such as the feed or an update) is still being downloaded automatically in the background.
 
 This property is suitable to use for menu item validation for seeing if `-checkForUpdates` can be invoked.
 
 This property is also KVO-compliant.
 
 Note this property does not reflect whether or not an update session is in progress. Please see `sessionInProgress` property instead.
 */
@property (nonatomic, readonly) BOOL canCheckForUpdates;

/**
 A property indicating whether or not an update session is in progress.
 
 An update session is in progress when the appcast is being downloaded, an update is being downloaded,
 an update is being shown, update permission is being requested, or the installer is being started.
 
 An active session is when Sparkle's fired scheduler is running.
 
 Note an update session may not be running even though Sparkle's installer (ran as a separate process) may be running,
 or even though the update has been downloaded but the installation has been deferred. In both of these cases, a new update session
 may be activated with the update resumed at a later point (automatically or manually).
 
 See also:
 - `canCheckForUpdates` property which is more suited for menu item validation and deciding if the user can initiate update checks.
 -  `-[SPUUpdaterDelegate updater:didFinishUpdateCycleForUpdateCheck:error:]` which lets the updater delegate know when an update cycle and session finishes.
 */
@property (nonatomic, readonly) BOOL sessionInProgress;

/**
 A property indicating whether or not to check for updates automatically.
 
 By default, Sparkle asks users on second launch for permission if they want automatic update checks enabled
 and sets this property based on their response. If `SUEnableAutomaticChecks` is set in the Info.plist,
 this permission request is not performed however.
 
 Setting this property will persist in the host bundle's user defaults.
 Hence developers shouldn't maintain an additional user default for this property.
 Only set this property if the user wants to change the default via a user settings option.
 Do not always set it on launch unless you want to ignore the user's preference.
 For testing environments, you can disable update checks by passing `-SUEnableAutomaticChecks NO`
 to your app's command line arguments instead of setting this property.
 
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately.
 
 This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL automaticallyChecksForUpdates;

/**
 A property indicating the current automatic update check interval in seconds.
 
 Prefer to set SUScheduledCheckInterval directly in your Info.plist for setting the initial value.
 
 Setting this property will persist in the host bundle's user defaults.
 Hence developers shouldn't maintain an additional user default for this property.
 Only set this property if the user wants to change the default via a user settings option.
 Do not always set it on launch unless you want to ignore the user's preference.
 
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately.
 
 This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) NSTimeInterval updateCheckInterval;

/**
 A property indicating whether or not updates can be automatically downloaded in the background.
 
 By default, updates are not automatically downloaded.
 
 By default starting from Sparkle 2.4, users are provided an option to opt in to automatically downloading and installing updates when they are asked if they want automatic update checks enabled.
 The default value for this option is based on what the developer sets `SUAutomaticallyUpdate` in their Info.plist.
 This is not done if `SUEnableAutomaticChecks` is set in the Info.plist however. Please check `automaticallyChecksForUpdates` property for more details.
 
 Note that the developer can disallow automatic downloading of updates from being enabled (via `SUAllowsAutomaticUpdates` Info.plist key).
 In this case, this property will return NO regardless of how this property is set.
 
 Prefer to set `SUAutomaticallyUpdate` directly in your Info.plist for setting the initial value.
 
 Setting this property will persist in the host bundle's user defaults.
 Hence developers shouldn't maintain an additional user default for this property.
 Only set this property if the user wants to change the default via a user settings option.
 Do not always set it on launch unless you want to ignore the user's preference.
 
 This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL automaticallyDownloadsUpdates;

/**
 A property indicating whether or not the *option* to automatically download updates in the background can be turned on.
 
 This property can be used to determine whether an option to automatically download/install updates should be enabled.
 
 Its value depends  on `automaticallyChecksForUpdates`, or the `SUAllowsAutomaticUpdates`in the host bundle's Info.plist if specified.
 Don't set `SUAllowsAutomaticUpdates` in the Info.plist unless you need custom behavior.
 
 This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic, readonly) BOOL allowsAutomaticUpdates;

/**
 The URL of the appcast used to download update information.
 
 If the updater's delegate implements `-[SPUUpdaterDelegate feedURLStringForUpdater:]`, this will return that feed URL.
 Otherwise if the feed URL has been set before using `-[SPUUpdater setFeedURL:]`, the feed URL returned will be retrieved from the host bundle's user defaults.
 Otherwise the feed URL in the host bundle's Info.plist will be returned.
 If no feed URL can be retrieved, returns nil.
 
 For setting a primary feed URL, please set the `SUFeedURL` property in your Info.plist.
 For setting an alternative feed URL, please prefer `-[SPUUpdaterDelegate feedURLStringForUpdater:]` over `-setFeedURL:`.
 Please see the documentation for `-setFeedURL:` for migrating away from that API.
 
 This property must be called on the main thread; calls from background threads will return nil.
 */
@property (nonatomic, readonly, nullable) NSURL *feedURL;

/**
 Set the URL of the appcast used to download update information. This method is deprecated.
 
 Setting this property will persist in the host bundle's user defaults.
 To avoid this undesirable behavior, please consider implementing
 `-[SPUUpdaterDelegate feedURLStringForUpdater:]` instead of using this method.
 
 Calling `-clearFeedURLFromUserDefaults` will remove any feed URL that has been set in the host bundle's user defaults.
 Passing nil to this method can also do this, but using `-clearFeedURLFromUserDefaults` is preferred.
 To migrate away from using this API, you must clear and remove any feed URLs set in the user defaults through this API.
 
 If you do not need to alternate between multiple feeds, set the SUFeedURL in your Info.plist instead of invoking this method.
 
 For beta updates, you may consider migrating to `-[SPUUpdaterDelegate allowedChannelsForUpdater:]` in the future.
 
 Updaters that update other developer's bundles should not call this method.
 
 This method must be called on the main thread; calls from background threads will have no effect.
 */
- (void)setFeedURL:(nullable NSURL *)feedURL __deprecated_msg("Please call -[SPUUpdater clearFeedURLFromUserDefaults] to migrate away from using this API and transition to either specifying the feed URL in your Info.plist, using channels in Sparkle 2, or using -[SPUUpdaterDelegate feedURLStringForUpdater:] to specify the dynamic feed URL at runtime");

/**
 Clears any feed URL from the host bundle's user defaults that was set via `-setFeedURL:`
 
 You should call this method if you have used `-setFeedURL:` in the past and want to stop using that API.
 Otherwise for compatibility Sparkle will prefer to use the feed URL that was set in the user defaults over the one that was specified in the host bundle's Info.plist,
 which is often undesirable (except for testing purposes).
 
 If a feed URL is found stored in the host bundle's user defaults (from calling `-setFeedURL:`) before it gets cleared,
 then that previously set URL is returned from this method.
 
 This method should be called as soon as possible, after your application finished launching or right after the updater has been started
 if you manually manage starting the updater.
 
 Updaters that update other developer's bundles should not call this method.
 
 This method must be called on the main thread.
 
 @return A previously set feed URL in the host bundle's user defaults, if available, otherwise this returns `nil`
 */
- (nullable NSURL *)clearFeedURLFromUserDefaults;

/**
 The host bundle that is being updated.
 */
@property (nonatomic, readonly) NSBundle *hostBundle;

/**
 The user agent used when checking for updates.
 
 By default the user agent string returned is in the format:
 `$(BundleDisplayName)/$(BundleDisplayVersion) Sparkle/$(SparkleDisplayVersion)`
 
 BundleDisplayVersion is derived from the main application's Info.plist's CFBundleShortVersionString.
 
 Note if Sparkle is being used to update another application, the bundle information retrieved is from the main application performing the updating.
 
 This default implementation can be overridden.
 */
@property (nonatomic, copy) NSString *userAgentString;

/**
 The HTTP headers used when checking for updates, downloading release notes, and downloading updates.
 
 The keys of this dictionary are HTTP header fields and values are corresponding values.
 */
@property (nonatomic, copy, nullable) NSDictionary<NSString *, NSString *> *httpHeaders;

/**
 A property indicating whether or not the user's system profile information is sent when checking for updates.

 Setting this property will persist in the host bundle's user defaults.
 
 This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL sendsSystemProfile;

/**
 The date of the last update check or nil if no check has been performed yet.
 
 For testing purposes, the last update check is stored in the `SULastCheckTime` key in the host bundle's user defaults.
 For example, `defaults delete my-bundle-id SULastCheckTime` can be invoked to clear the last update check time and test
 if update checks are automatically scheduled.
 
 This property must be called on the main thread.
 */
@property (nonatomic, readonly, copy, nullable) NSDate *lastUpdateCheckDate;

/**
 Appropriately re-schedules the update checking timer according to the current updater settings.
 
 This method should only be called in response to a user changing updater settings. This method may trigger a new update check to occur in the background if an updater setting such as the updater's feed or allowed channels has changed.
 
 If the `updateCheckInterval` or `automaticallyChecksForUpdates` properties are changed, this method is automatically invoked after a short delay using `-resetUpdateCycleAfterShortDelay`. In these cases, manually resetting the update cycle is not necessary.
 
 See also `-resetUpdateCycleAfterShortDelay` which gives the user a short delay before triggering a cycle reset.
 
 This must be called on the main thread.
 */
- (void)resetUpdateCycle;

/**
 Appropriately re-schedules the update checking timer according to the current updater settings after a short cancellable delay.
 
 This method calls `resetUpdateCycle` after a short delay to give the user a short amount of time to cancel changing an updater setting.
 If this method is called again, any previous reset request that is still inflight will be cancelled.
 
 For example, if the user changes the `automaticallyChecksForUpdates` setting to `YES`, but quickly undoes their change then
 no cycle reset will be done.
 
 If the `updateCheckInterval` or `automaticallyChecksForUpdates` properties are changed, this method is automatically invoked. In these cases, manually resetting the update cycle is not necessary.
 
 This must be called on the main thread.
 */
- (void)resetUpdateCycleAfterShortDelay;

/**
 The system profile information that is sent when checking for updates.
 */
@property (nonatomic, readonly, copy) NSArray<NSDictionary<NSString *, NSString *> *> *systemProfileArray;

@end

NS_ASSUME_NONNULL_END
