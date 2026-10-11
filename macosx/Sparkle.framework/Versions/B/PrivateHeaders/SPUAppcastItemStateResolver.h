//
//  SPUAppcastItemStateResolver.h
//  Sparkle
//
//  Created by Mayur Pawashe on 5/31/21.
//  Copyright © 2021 Sparkle Project. All rights reserved.
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

@class SUStandardVersionComparator, SPUAppcastItemState;
@protocol SUVersionComparison;

/**
 Private exposed class used to resolve Appcast Item properties that rely on external factors such as a host.
 This resolver is used for constructing appcast items.
 */
SU_EXPORT @interface SPUAppcastItemStateResolver : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithHostVersion:(NSString *)hostVersion applicationVersionComparator:(id<SUVersionComparison>)applicationVersionComparator standardVersionComparator:(SUStandardVersionComparator *)standardVersionComparator;

@end

NS_ASSUME_NONNULL_END
