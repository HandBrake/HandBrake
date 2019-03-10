//
//  SPUUpdater.h
//  Sparkle
//
//  Created by Andy Matuschak on 1/4/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#if __has_feature(modules)
@import Foundation;
#else
#import <Foundation/Foundation.h>
#endif
#import <Sparkle/SUExport.h>
#import <Sparkle/SPUUserDriver.h>

NS_ASSUME_NONNULL_BEGIN

@class SUAppcastItem, SUAppcast;

@protocol SPUUpdaterDelegate;

/*!
    The main API in Sparkle for controlling the update mechanism.

    This class is used to configure the update parameters as well as manually
    and automatically schedule and control checks for updates.
 */
SU_EXPORT @interface SPUUpdater : NSObject

/*!
 Initializes a new SPUUpdater instance
 
 This does not start the updater. To start it, see -[SPUUpdater startUpdater:]
 
 Note that this is a normal initializer and doesn't implement the singleton pattern (i.e, instances aren't cached, so no surprises)
 This also means that updater instances can be deallocated, and that they will be torn down properly.
 
 Related: See SPUStandardUpdaterController which wraps a SPUUpdater instance and is suitable for instantiating in nib files
 
 @param hostBundle The bundle that should be targetted for updating. This must not be nil.
 @param applicationBundle The application bundle that should be relaunched and waited for termination. Usually this can be the same as hostBundle. This may differ when updating a plug-in or other non-application bundle.
 @param userDriver The user driver that Sparkle uses for user update interaction
 @param delegate The delegate for SPUUpdater. This may be nil.
 */
- (instancetype)initWithHostBundle:(NSBundle *)hostBundle applicationBundle:(NSBundle *)applicationBundle userDriver:(id <SPUUserDriver>)userDriver delegate:(id<SPUUpdaterDelegate> _Nullable)delegate;

/*!
 Starts the updater.

 This method checks if Sparkle is configured properly. A valid feed URL should be set before this method is invoked.
 Other properties of this SPUUpdater instance can be set before this method is invoked as well, such as automatic update checks.

 If the configuration is valid, this method may bring up a permission prompt (if needed) for checking if the user wants automatic update checking.
 This method then starts the regular update cycle if automatic update checks are enabled.

 One of -checkForUpdates, -checkForUpdatesInBackground, or -checkForUpdateInformation can be invoked before starting the updater.
 This preschedules an update action before starting the updater. When the updater is started, the prescheduled action is immediately invoked.
 This may be useful for example if you want to check for updates right away without a permission prompt potentially showing.

 This must be called on the main thread.

 @param error The error that is populated if this method fails. Pass NULL if not interested in the error information.
 @return YES if the updater started otherwise NO with a populated error
 */
- (BOOL)startUpdater:(NSError * __autoreleasing *)error;

/*!
 Checks for updates, and displays progress while doing so.
 
 This is meant for users initiating an update check.
 This may find a resumable update that has already been downloaded or has begun installing, or
 this may find a new update that can start to be downloaded if the user requests it.
 This will find updates that the user has opted into skipping.
 */
- (void)checkForUpdates;

/*!
 Checks for updates, but does not display any UI unless an update is found.
 
 This is meant for programmatically initating a check for updates.
 That is, it will display no UI unless it finds an update, in which case it proceeds as usual.
 This will not find updates that the user has opted into skipping.
 
 Note if there is no resumable update found, and automated updating is turned on,
 the update will be downloaded in the background without disrupting the user.
 */
- (void)checkForUpdatesInBackground;

/*!
 Begins a "probing" check for updates which will not actually offer to
 update to that version.
 
 However, the delegate methods
 SPUUpdaterDelegate::updater:didFindValidUpdate: and
 SPUUpdaterDelegate::updaterDidNotFindUpdate: will be called,
 so you can use that information in your UI.
 
 Updates that have been skipped by the user will not be found.
 */
- (void)checkForUpdateInformation;

/*!
 A property indicating whether or not to check for updates automatically.
 
 Setting this property will persist in the host bundle's user defaults.
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately
 */
@property (nonatomic) BOOL automaticallyChecksForUpdates;

/*!
 A property indicating the current automatic update check interval.
 
 Setting this property will persist in the host bundle's user defaults.
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately
 */
@property (nonatomic) NSTimeInterval updateCheckInterval;

/*!
 A property indicating whether or not updates can be automatically downloaded in the background.
 
 Note that the developer can disallow automatic downloading of updates from being enabled.
 In this case, -automaticallyDownloadsUpdates will return NO regardless of how this property is set.
 
 Setting this property will persist in the host bundle's user defaults.
 */
@property (nonatomic) BOOL automaticallyDownloadsUpdates;

/*!
 The URL of the appcast used to download update information.
 
 If the updater's delegate implements -[SPUUpdaterDelegate feedURLStringForUpdater:], this will return that feed URL.
 Otherwise if the feed URL has been set before, the feed URL returned will be retrieved from the host bundle's user defaults.
 Otherwise the feed URL in the host bundle's Info.plist will be returned.
 If no feed URL can be retrieved, this will raise an exception.
 
 This property must be called on the main thread.
 */
@property (nonatomic, readonly) NSURL *feedURL;

/*!
 Set the URL of the appcast used to download update information. Using this method is discouraged.
 
 Setting this property will persist in the host bundle's user defaults.
 To avoid this, you should consider instead implementing
 -[SPUUpdaterDelegate feedURLStringForUpdater:] or -[SPUUpdaterDelegate feedParametersForUpdater:sendingSystemProfile:]
 
 Passing nil will remove any feed URL that has been set in the host bundle's user defaults.
 
 This method must be called on the main thread.
 */
- (void)setFeedURL:(NSURL * _Nullable)feedURL;

/*!
 The host bundle that is being updated.
 */
@property (nonatomic, readonly) NSBundle *hostBundle;

/*!
 The bundle this class (SPUUpdater) is loaded into
 */
@property (nonatomic, readonly) NSBundle *sparkleBundle;

/*!
 * The user agent used when checking for updates.
 *
 * The default implementation can be overrided.
 */
@property (nonatomic, copy) NSString *userAgentString;

/*!
 The HTTP headers used when checking for updates.
 
 The keys of this dictionary are HTTP header fields (NSString) and values are corresponding values (NSString)
 */
#if __has_feature(objc_generics)
@property (nonatomic, copy, nullable) NSDictionary<NSString *, NSString *> *httpHeaders;
#else
@property (nonatomic, copy, nullable) NSDictionary *httpHeaders;
#endif

/*!
 A property indicating whether or not the user's system profile information is sent when checking for updates.

 Setting this property will persist in the host bundle's user defaults.
 */
@property (nonatomic) BOOL sendsSystemProfile;

/*!
    Returns the date of last update check.

    \returns \c nil if no check has been performed.
 */
@property (nonatomic, readonly, copy, nullable) NSDate *lastUpdateCheckDate;

/*!
    Appropriately schedules or cancels the update checking timer according to
    the preferences for time interval and automatic checks.

    This call does not change the date of the next check,
    but only the internal timer.
 */
- (void)resetUpdateCycle;

@end

NS_ASSUME_NONNULL_END
