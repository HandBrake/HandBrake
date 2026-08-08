/*  HBAudioFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioFilters.h"

#include "handbrake/handbrake.h"

NS_ASSUME_NONNULL_BEGIN

@implementation HBAudioFilters

- (NSArray<NSNumber *> *)availableFilters
{
    return @[
        @(HB_AUDIO_FILTER_ACOMPRESSOR),
        @(HB_AUDIO_FILTER_AGATE),
    ];
}

- (NSArray<HBFilterGroup *> *)availableFilterGroups
{
    NSMutableArray<HBFilterGroup *> *groups = [NSMutableArray array];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_AUDIO_FILTER_ACOMPRESSOR]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_AUDIO_FILTER_AGATE]];
    return groups;
}

@end

NS_ASSUME_NONNULL_END
