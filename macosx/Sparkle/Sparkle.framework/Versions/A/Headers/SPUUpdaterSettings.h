//
//  SPUUpdaterSettings.h
//  Sparkle
//
//  Created by Mayur Pawashe on 3/27/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Sparkle/SUExport.h>

NS_ASSUME_NONNULL_BEGIN

/*!
 This class can be used for reading certain updater settings.
 
 It retrieves the settings by first looking into the host's user defaults.
 If the setting is not found in there, then the host's Info.plist file is looked at.
 */
SU_EXPORT @interface SPUUpdaterSettings : NSObject

- (instancetype)initWithHostBundle:(NSBundle *)hostBundle;

/*!
 * Indicates whether or not automatic update checks are enabled.
 */
@property (readonly, nonatomic) BOOL automaticallyChecksForUpdates;

/*!
 * The regular update check interval.
 */
@property (readonly, nonatomic) NSTimeInterval updateCheckInterval;

/*!
 * Indicates whether or not automatically downloading updates is allowed to be turned on by the user.
 */
@property (readonly, nonatomic) BOOL allowsAutomaticUpdates;

/*!
 * Indicates whether or not automatically downloading updates is enabled by the user or developer.
 *
 * Note this does not indicate whether or not automatic downloading of updates is allowable.
 * See `-allowsAutomaticUpdates` property for that.
 */
@property (readonly, nonatomic) BOOL automaticallyDownloadsUpdates;

/*!
 * Indicates whether or not anonymous system profile information is sent when checking for updates.
 */
@property (readonly, nonatomic) BOOL sendsSystemProfile;

@end

NS_ASSUME_NONNULL_END
