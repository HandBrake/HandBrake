//
//  SUVersionDisplayProtocol.h
//  EyeTV
//
//  Created by Uli Kusterer on 08.12.09.
//  Copyright 2009 Elgato Systems GmbH. All rights reserved.
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

@class SUAppcastItem;

NS_ASSUME_NONNULL_BEGIN

/**
 Applies special display formatting to version numbers of the bundle to update and the update before presenting them to the user.
*/
SU_EXPORT @protocol SUVersionDisplay <NSObject>

/**
 Formats an update's version string and bundle's version string for display.
 
 This method is used to format both the display version of the update and the display version of the bundle to update.
 
 The display versions returned by this method are then used for presenting to the user when a new update is available,
 or when the user cannot download/install the latest update for a specific reason, or when the user has a newer version
 installed than the latest known version in the update feed.
 
 On input, the `update.displayVersionString` and `*inOutBundleDisplayVersion` may be the same, but the
 `update.versionString` and `bundleVersion` will differ. To differentiate between these display versions, you may
 choose to return different display version strings for the update and bundle.
 
 @param update The update to format the update display version from. You can query `update.displayVersionString` and `update.versionString` to retrieve the update's version information.
 @param inOutBundleDisplayVersion On input, the display version string (or `CFBundleShortVersionString`) of the bundle to update. On output, this is the display version string of the bundle to show to the user.
 @param bundleVersion The version (or CFBundleVersion) of the bundle to update.
 @return A new display version string of the `update.displayVersionString` to show to the user.
 */
- (NSString *)formatUpdateDisplayVersionFromUpdate:(SUAppcastItem *)update andBundleDisplayVersion:(NSString * _Nonnull __autoreleasing * _Nonnull)inOutBundleDisplayVersion withBundleVersion:(NSString *)bundleVersion;

@optional

/**
 Formats a bundle's version string for display.
 
 This method is used to format the display version of the bundle.
 This method may be used when no new update is available and the user is already on the latest known version.
 In this case, no new update version is shown to the user.
 
 This method is optional. If it's not implemented, Sparkle will default to using the `bundleDisplayVersion` passed to this method.
 
 @param bundleDisplayVersion The display version string (or `CFBundleShortVersionString`) of the bundle to update.
 @param bundleVersion The version (or `CFBundleVersion`) of the bundle to update.
 @param matchingUpdate The update in the feed that corresponds to the current bundle, or `nil` if no matching update item could be found in the feed.
 @return A new display version string of the bundle to show to the user.
 */
- (NSString *)formatBundleDisplayVersion:(NSString *)bundleDisplayVersion withBundleVersion:(NSString *)bundleVersion matchingUpdate:(SUAppcastItem * _Nullable)matchingUpdate;

/**
 Formats two version strings.
 
 Both versions are provided so that important distinguishing information
 can be displayed while also leaving out unnecessary/confusing parts.
*/
- (void)formatVersion:(NSString *_Nonnull*_Nonnull)inOutVersionA andVersion:(NSString *_Nonnull*_Nonnull)inOutVersionB __deprecated_msg("Please use -formatUpdateDisplayVersionFromUpdate:andBundleDisplayVersion:withBundleVersion:");

@end

NS_ASSUME_NONNULL_END
