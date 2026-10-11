//
//  SPUStandardUserDriver.h
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
#import "SPUUserDriver.h"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SPUUserDriver.h>
#import <Sparkle/SUExport.h>
#endif

NS_ASSUME_NONNULL_BEGIN

@protocol SPUStandardUserDriverDelegate;

/**
 Sparkle's standard built-in user driver for updater interactions
 */
SU_EXPORT NS_SWIFT_UI_ACTOR @interface SPUStandardUserDriver : NSObject <SPUUserDriver>

/**
 Initializes a Sparkle's standard user driver for user update interactions
 
 @param hostBundle The target bundle of the host that is being updated.
 @param delegate The optional delegate to this user driver. Note the standard user driver weakly references the delegate, so you are responsible for keeping it alive.
 */
- (instancetype)initWithHostBundle:(NSBundle *)hostBundle delegate:(nullable id<SPUStandardUserDriverDelegate>)delegate;

/**
 Use initWithHostBundle:delegate: instead.
 */
- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
