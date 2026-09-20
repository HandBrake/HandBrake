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
        @(HB_AUDIO_FILTER_ADECLICK),
        @(HB_AUDIO_FILTER_ADECLIP),
        @(HB_AUDIO_FILTER_AFFTDN),
        @(HB_AUDIO_FILTER_ANLMDN),
        @(HB_AUDIO_FILTER_AGATE),
        @(HB_AUDIO_FILTER_ACOMPRESSOR),
        @(HB_AUDIO_FILTER_ALIMITER),
        @(HB_AUDIO_FILTER_DIALOGUENHANCE),
        @(HB_AUDIO_FILTER_CROSSFEED),
        @(HB_AUDIO_FILTER_STEREOWIDEN),
        @(HB_AUDIO_FILTER_LOUDNORM)
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
