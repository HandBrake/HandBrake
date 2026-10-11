//
//  SPUStandardUserDriver+Private.h
//  Sparkle
//
//  Copyright © 2022 Sparkle Project. All rights reserved.
//

#ifndef SPUStandardUserDriver_Private_h
#define SPUStandardUserDriver_Private_h

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SPUStandardUserDriver.h"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SPUStandardUserDriver.h>
#import <Sparkle/SUExport.h>
#endif

@class NSWindowController;

NS_ASSUME_NONNULL_BEGIN

SU_EXPORT @interface SPUStandardUserDriver (Private)

/**
 Private API for accessing the active update alert's window controller.
 This is the window controller that shows the update's release notes and install choices.
 This can be accessed in -[SPUStandardUserDriverDelegate standardUserDriverWillHandleShowingUpdate:forUpdate:state:]
 */
@property (nonatomic, readonly, nullable) NSWindowController *activeUpdateAlert;

@end

NS_ASSUME_NONNULL_END

#endif /* SPUStandardUserDriver_Private_h */
