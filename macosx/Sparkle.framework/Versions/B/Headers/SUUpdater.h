//
//  SUUpdater.h
//  Sparkle
//
//  Created by Andy Matuschak on 1/4/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#ifndef SUUPDATER_H
#define SUUPDATER_H

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#import "SUVersionComparisonProtocol.h"
#import "SUVersionDisplayProtocol.h"
#import "SUUpdaterDelegate.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#import <Sparkle/SUVersionComparisonProtocol.h>
#import <Sparkle/SUVersionDisplayProtocol.h>
#import <Sparkle/SUUpdaterDelegate.h>
#endif

@class SUAppcastItem, SUAppcast, NSMenuItem;

@protocol SUUpdaterDelegate;

/**
 The legacy API in Sparkle for controlling the update mechanism.

 This class is now deprecated and acts as a thin wrapper around `SPUUpdater` and `SPUStandardUserDriver`.
 
 If you are migrating to Sparkle 2, use `SPUStandardUpdaterController` instead, or `SPUUpdater` if you need more control.
 */
__deprecated_msg("Deprecated in Sparkle 2. Use SPUStandardUpdaterController instead, or SPUUpdater if you need more control.")
SU_EXPORT @interface SUUpdater : NSObject

@property (unsafe_unretained, nonatomic) IBOutlet id<SUUpdaterDelegate> delegate;

/*!
 The shared updater for the main bundle.
 
 This is equivalent to passing [NSBundle mainBundle] to SUUpdater::updaterForBundle:
 */
+ (SUUpdater *)sharedUpdater;

/*!
 The shared updater for a specified bundle.
 If an updater has already been initialized for the provided bundle, that shared instance will be returned.
 */
+ (SUUpdater *)updaterForBundle:(NSBundle *)bundle;

/*!
 Designated initializer for SUUpdater.
 
 If an updater has already been initialized for the provided bundle, that shared instance will be returned.
 */
- (instancetype)initForBundle:(NSBundle *)bundle;

/*!
 Explicitly checks for updates and displays a progress dialog while doing so.

 This method is meant for a main menu item.
 Connect any menu item to this action in Interface Builder,
 and Sparkle will check for updates and report back its findings verbosely
 when it is invoked.

 This will find updates that the user has opted into skipping.
 */
- (IBAction)checkForUpdates:(id)sender;

/*!
 The menu item validation used for the -checkForUpdates: action
 */
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;

/*!
 Checks for updates, but does not display any UI unless an update is found.

 This is meant for programmatically initiating a check for updates. That is,
 it will display no UI unless it actually finds an update, in which case it
 proceeds as usual.

 If automatic downloading of updates it turned on and allowed, however,
 this will invoke that behavior, and if an update is found, it will be downloaded
 in the background silently and will be prepped for installation.

 This will not find updates that the user has opted into skipping.
 */
- (void)checkForUpdatesInBackground;

/*!
 A property indicating whether or not to check for updates automatically.

 Setting this property will persist in the host bundle's user defaults.
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately
 */
@property (nonatomic) BOOL automaticallyChecksForUpdates;

/*!
 A property indicating whether or not updates can be automatically downloaded in the background.

 Note that automatic downloading of updates can be disallowed by the developer.
 In this case, -automaticallyDownloadsUpdates will return NO regardless of how this property is set.

 Setting this property will persist in the host bundle's user defaults.
 */
@property (nonatomic) BOOL automaticallyDownloadsUpdates;

/*!
 A property indicating the current automatic update check interval.

 Setting this property will persist in the host bundle's user defaults.
 The update schedule cycle will be reset in a short delay after the property's new value is set.
 This is to allow reverting this property without kicking off a schedule change immediately
 */
@property (nonatomic) NSTimeInterval updateCheckInterval;

/*!
 Begins a "probing" check for updates which will not actually offer to
 update to that version.

 However, the delegate methods
 SUUpdaterDelegate::updater:didFindValidUpdate: and
 SUUpdaterDelegate::updaterDidNotFindUpdate: will be called,
 so you can use that information in your UI.

 Updates that have been skipped by the user will not be found.
 */
- (void)checkForUpdateInformation;

/*!
 The URL of the appcast used to download update information.

 Setting this property will persist in the host bundle's user defaults.
 If you don't want persistence, you may want to consider instead implementing
 SUUpdaterDelegate::feedURLStringForUpdater: or SUUpdaterDelegate::feedParametersForUpdater:sendingSystemProfile:

 This property must be called on the main thread.
 */
@property (nonatomic, copy) NSURL *feedURL;

/*!
 The host bundle that is being updated.
 */
@property (readonly, nonatomic) NSBundle *hostBundle;

/*!
 The bundle this class (SUUpdater) is loaded into.
 */
@property (nonatomic, readonly) NSBundle *sparkleBundle;

/*!
 The user agent used when checking for and downloading updates.

 The default implementation can be overridden.
 */
@property (nonatomic, copy) NSString *userAgentString;

/*!
 The HTTP headers used when checking for and downloading updates.

 The keys of this dictionary are HTTP header fields (NSString) and values are corresponding values (NSString)
 */
@property (copy, nonatomic) NSDictionary<NSString *, NSString *> *httpHeaders;

/*!
 A property indicating whether or not the user's system profile information is sent when checking for updates.

 Setting this property will persist in the host bundle's user defaults.
 */
@property (nonatomic) BOOL sendsSystemProfile;

/*!
 A property indicating the decryption password used for extracting updates shipped as Apple Disk Images (dmg)
 */
@property (nonatomic, copy) NSString *decryptionPassword;

/*!
 Returns the date of last update check.

 \returns \c nil if no check has been performed.
 */
@property (nonatomic, readonly, copy) NSDate *lastUpdateCheckDate;

/*!
 Appropriately schedules or cancels the update checking timer according to
 the preferences for time interval and automatic checks.

 This call does not change the date of the next check,
 but only the internal NSTimer.
 */
- (void)resetUpdateCycle;

/*!
 A property indicating whether or not an update is in progress.

 Note this property is not indicative of whether or not user initiated updates can be performed.
 Use SUUpdater::validateMenuItem: for that instead.
 */
@property (nonatomic, readonly) BOOL updateInProgress;

@end

#endif
