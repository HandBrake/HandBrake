//
//  SUVersionComparisonProtocol.h
//  Sparkle
//
//  Created by Andy Matuschak on 12/21/07.
//  Copyright 2007 Andy Matuschak. All rights reserved.
//

#ifndef SUVERSIONCOMPARISONPROTOCOL_H
#define SUVERSIONCOMPARISONPROTOCOL_H

#if __has_feature(modules)
@import Foundation;
#else
#import <Foundation/Foundation.h>
#endif
#import "SUExport.h"

NS_ASSUME_NONNULL_BEGIN

/*!
    Provides version comparison facilities for Sparkle.
*/
@protocol SUVersionComparison

/*!
    An abstract method to compare two version strings.

    Should return NSOrderedAscending if b > a, NSOrderedDescending if b < a,
    and NSOrderedSame if they are equivalent.
*/
- (NSComparisonResult)compareVersion:(NSString *)versionA toVersion:(NSString *)versionB; // *** MAY BE CALLED ON NON-MAIN THREAD!

@end

NS_ASSUME_NONNULL_END
#endif
