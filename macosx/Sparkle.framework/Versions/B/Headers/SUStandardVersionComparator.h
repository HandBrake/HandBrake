//
//  SUStandardVersionComparator.h
//  Sparkle
//
//  Created by Andy Matuschak on 12/21/07.
//  Copyright 2007 Andy Matuschak. All rights reserved.
//

#ifndef SUSTANDARDVERSIONCOMPARATOR_H
#define SUSTANDARDVERSIONCOMPARATOR_H

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#import "SUVersionComparisonProtocol.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#import <Sparkle/SUVersionComparisonProtocol.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
    Sparkle's default version comparator.

    This comparator is adapted from MacPAD, by Kevin Ballard.
    It's "dumb" in that it does essentially string comparison,
    in components split by character type.
*/
SU_EXPORT @interface SUStandardVersionComparator : NSObject <SUVersionComparison>

/**
    Initializes a new instance of the standard version comparator.
*/
- (instancetype)init;

/**
    A singleton instance of the comparator.
 */
@property (nonatomic, class, readonly) SUStandardVersionComparator *defaultComparator;

/**
    Compares two version strings through textual analysis.
 
    These version strings should be in the format of x, x.y, or x.y.z where each component is a number.
    For example, valid version strings include "1.5.3", "500", or "4000.1"
    These versions that are compared correspond to the @c CFBundleVersion values of the updates.
 
    @param versionA The first version string to compare.
    @param versionB The second version string to compare.
    @return A comparison result between @c versionA and @c versionB
*/
- (NSComparisonResult)compareVersion:(NSString *)versionA toVersion:(NSString *)versionB;

@end

NS_ASSUME_NONNULL_END
#endif
