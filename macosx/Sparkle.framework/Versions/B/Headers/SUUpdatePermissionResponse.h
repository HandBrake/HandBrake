//
//  SUUpdatePermissionResponse.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/8/16.
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
 This class represents a response for permission to check updates.
*/
SU_EXPORT NS_SWIFT_SENDABLE @interface SUUpdatePermissionResponse : NSObject<NSSecureCoding>

/**
 Initializes a new update permission response instance.
 
 @param automaticUpdateChecks Flag to enable automatic update checks.
 @param sendSystemProfile Flag for if system profile information should be sent to the server hosting the appcast.
 */
- (instancetype)initWithAutomaticUpdateChecks:(BOOL)automaticUpdateChecks sendSystemProfile:(BOOL)sendSystemProfile;

/**
 Initializes a new update permission response instance.
 
 @param automaticUpdateChecks Flag to enable automatic update checks.
 @param automaticUpdateDownloading Flag to enable automatic downloading and installing of updates. If this is nil, this option will be ignored.
 @param sendSystemProfile Flag for if system profile information should be sent to the server hosting the appcast.
 */
- (instancetype)initWithAutomaticUpdateChecks:(BOOL)automaticUpdateChecks automaticUpdateDownloading:(NSNumber * _Nullable)automaticUpdateDownloading sendSystemProfile:(BOOL)sendSystemProfile;

/*
 Use -initWithAutomaticUpdateChecks:sendSystemProfile: instead.
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 A read-only property indicating if update checks should be done automatically.
 */
@property (nonatomic, readonly) BOOL automaticUpdateChecks;

/**
 A read-only property indicating if updates should be automatically downloaded and installed.
 
 If this property is `nil`, then no user choice was made for this option.
 
 If  `automaticUpdateChecks` is `NO` then this property should not be `@(YES)`.
 Set it to `NO` if the user was given the choice of automatically downloading and installing updates,
 otherwise set it to `nil`.
 */
@property (nonatomic, readonly, nullable) NSNumber *automaticUpdateDownloading;

/**
 A read-only property indicating if system profile should be sent or not.
 */
@property (nonatomic, readonly) BOOL sendSystemProfile;

@end

NS_ASSUME_NONNULL_END
