/*  HBRange+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRange+UIAdditions.h"
#import "HBTitle.h"

@implementation HBRange (UIAdditions)

- (NSArray *)chapters
{
    NSMutableArray *chapters = [NSMutableArray array];
    [self.title.chapters enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        [chapters addObject:[NSString stringWithFormat: @"%lu", idx + 1]];
    }];
    return chapters;
}

- (NSArray *)types
{
    return @[NSLocalizedString(@"Chapters", @""),
             NSLocalizedString(@"Seconds", @""),
             NSLocalizedString(@"Frames", @"")];
}

- (BOOL)chaptersSelected
{
    return self.type == HBRangeTypeChapters;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingChaptersSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

- (BOOL)secondsSelected
{
    return self.type == HBRangeTypeSeconds;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingSecondsSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

- (BOOL)framesSelected
{
    return self.type == HBRangeTypeFrames;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingFramesSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

@end
