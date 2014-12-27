/*  HBJob+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+UIAdditions.h"
#include "hb.h"

@implementation HBJob (UIAdditions)

- (BOOL)mp4OptionsEnabled
{
    if (self.container & HB_MUX_MASK_MP4)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

- (NSArray *)angles
{
    NSMutableArray *angles = [NSMutableArray array];
    for (int i = 0; i < self.title.angles; i++)
    {
        [angles addObject:[NSString stringWithFormat: @"%d", i + 1]];
    }
    return angles;
}

@end

@implementation HBURLTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    if (value)
        return [value path];
    else
        return nil;
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return [NSURL fileURLWithPath:value];
}

@end

