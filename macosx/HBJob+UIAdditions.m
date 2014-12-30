/*  HBJob+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+UIAdditions.h"
#include "hb.h"

@implementation HBJob (UIAdditions)

- (BOOL)mp4OptionsEnabled
{
    return ((self.container & HB_MUX_MASK_MP4) != 0);
}

- (BOOL)mp4iPodCompatibleEnabled
{
    return ((self.container & HB_MUX_MASK_MP4) != 0) && (self.video.encoder & HB_VCODEC_H264_MASK);
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

- (NSArray *)containers
{
    NSMutableArray *containers = [NSMutableArray array];

    for (const hb_container_t *container = hb_container_get_next(NULL);
         container != NULL;
         container  = hb_container_get_next(container))
    {
        NSString *title = nil;
        if (container->format & HB_MUX_MASK_MP4)
        {
            title = NSLocalizedString(@"MP4 File", @"");
        }
        else if (container->format & HB_MUX_MASK_MKV)
        {
            title = NSLocalizedString(@"MKV File", @"");
        }
        else
        {
            title = [NSString stringWithUTF8String:container->name];
        }
        [containers addObject:title];
    }

    return [[containers copy] autorelease];
}

@end

@implementation HBContainerTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    int container = [value intValue];
    if (container & HB_MUX_MASK_MP4)
    {
        return NSLocalizedString(@"MP4 File", @"");
    }
    else if (container & HB_MUX_MASK_MKV)
    {
        return NSLocalizedString(@"MKV File", @"");
    }
    else
    {
        const char *name = hb_container_get_name(container);
        if (name)
        {
            return @(name);
        }
        else
        {
            return nil;
        }
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return @(hb_container_get_from_name([value UTF8String]));
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

