//
//  SPUUpdaterSettings.h
//  Sparkle
//
//  Created by Mayur Pawashe on 3/27/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 This class can be used for reading and updating updater settings.
 
 It retrieves the settings by first looking into the host's user defaults.
 If the setting is not found in there, then the host's Info.plist file is looked at.
 
 For updating updater settings, changes are made in the host's user defaults.
 */
SU_EXPORT NS_SWIFT_UI_ACTOR @interface SPUUpdaterSettings : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithHostBundle:(NSBundle *)hostBundle;

/**
 * Indicates whether or not automatic update checks are enabled.
 *
 * This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL automaticallyChecksForUpdates;

/**
 * The regular update check interval.
 *
 * This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) NSTimeInterval updateCheckInterval;

/**
 * Indicates whether or not automatically downloading updates is allowed to be turned on by the user.
 *
 * This property is determined by checking `automaticallyChecksForUpdates` and `allowsAutomaticUpdatesOption`.
 *
 * This property is KVO compliant. This property must be called on the main thread.
 */
@property (readonly, nonatomic) BOOL allowsAutomaticUpdates;

/**
 * Indicates whether or not automatically downloading updates is enabled by the user or developer.
 *
 * Note this does not indicate whether or not automatic downloading of updates is allowable.
 * See `-allowsAutomaticUpdates` property for that.
 *
 * This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL automaticallyDownloadsUpdates;

/**
 * Indicates whether or not the developer allows turning on updates being automatically downloaded and installed.
 * If this value is nil, the developer has not explicitly specified this option (which is the default).
 *
 * Please prefer to use `allowsAutomaticUpdates` instead.
 */
@property (readonly, nonatomic, nullable) NSNumber *allowsAutomaticUpdatesOption;

/**
 * The impatient update check interval.
 *
 * If an update has already been downloaded automatically in the background, Sparkle may not notify users of the update immediately,
 * and tries to install the update siliently on quit without notifying the user.
 *
 * Sparkle uses this long impatient update check interval to decide when to notify the user of the update if they haven't quit the app for a long time.
 * By default this check interval is set to 604800 seconds (which is 1 week). This interval must be bigger than the `updateCheckInterval`.
 */
@property (nonatomic, readonly) NSTimeInterval impatientUpdateCheckInterval;

/**
 * Indicates whether or not anonymous system profile information is sent when checking for updates.
 *
 * This property is KVO compliant. This property must be called on the main thread.
 */
@property (nonatomic) BOOL sendsSystemProfile;

@end

NS_ASSUME_NONNULL_END
