/*  HBRange+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRange+UIAdditions.h"
#import "HBTitle.h"
#import "HBLocalizationUtilities.h"

@implementation HBRange (UIAdditions)

- (NSArray<NSString *> *)chapters
{
    NSMutableArray *chapters = [NSMutableArray array];
    [self.title.chapters enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        [chapters addObject:[NSString stringWithFormat: @"%lu", idx + 1]];
    }];
    return chapters;
}

- (NSArray<NSString *> *)types
{
    return @[HBKitLocalizedString(@"Chapters", @"HBRange -> display name"),
             HBKitLocalizedString(@"Seconds", @"HBRange -> display name"),
             HBKitLocalizedString(@"Frames", @"HBRange -> display name")];
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

@implementation HBTimeInSecondsTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    uint64_t duration = [value integerValue];
    uint64_t hours    = duration / 3600;
    uint64_t minutes  = (duration % 3600) / 60;
    uint64_t seconds  = duration % 60;

    NSString *result = [NSString stringWithFormat:@"%02llu:%02llu:%02llu", hours, minutes, seconds];
    return result;
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    const char *time = [value UTF8String];
    if (time)
    {
        unsigned hour, minute, second, timeval;

        if (sscanf(time, "%2u:%u:%u", &hour, &minute, &second) < 3) {
            if (sscanf(time, "%u:%u:%u", &hour, &minute, &second) < 3) {
                return 0;
            }
        }

        if (second > 60) {
            second = 0;
        }

        timeval = hour * 60 * 60 + minute * 60 + second;

        return @(timeval);
    }
    return @"00:00:00";
}

@end
