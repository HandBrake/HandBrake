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
    return @[NSLocalizedString(@"Chapters", @"HBRange -> display name"),
             NSLocalizedString(@"Seconds", @"HBRange -> display name"),
             NSLocalizedString(@"Frames", @"HBRange -> display name")];
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

@implementation HBTimeTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    uint64_t duration = [value integerValue];
    uint64_t hours    = duration / 90000 / 3600;
    uint64_t minutes  = ((duration / 90000 ) % 3600) / 60;
    uint64_t seconds  = (duration / 90000 ) % 60;

    return [NSString stringWithFormat:@"%02llu:%02llu:%02llu", hours, minutes, seconds];
}

+ (BOOL)allowsReverseTransformation
{
    return NO;
}

- (id)reverseTransformedValue:(id)value
{
    return nil;
}

@end
