/*  HBAudioTransformers.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTransformers.h"
#import "HBLocalizationUtilities.h"
#include "hb.h"

#pragma mark - Value Transformers

@implementation HBFallbackEncodersTransformer

+ (Class)transformedValueClass
{
    return [NSArray class];
}

- (id)transformedValue:(id)value
{
    if (value != nil)
    {
        NSMutableArray *localizedArray = [NSMutableArray array];

        for (NSString *text in value)
        {
            if ([text isEqualToString:@"None"])
            {
                [localizedArray addObject:HBKitLocalizedString(@"None", @"HBSubtitles -> none track name")];
            }
            else
            {
                [localizedArray addObject:text];
            }
        }
        return localizedArray;
    }

    return value;
}

+ (BOOL)allowsReverseTransformation
{
    return NO;
}

@end

@implementation HBFallbackEncoderTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_audio_encoder_get_name([value intValue]);
    if (name)
    {
        return @(name);
    }
    else
    {
        return HBKitLocalizedString(@"None", @"HBSubtitles -> none track name");
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value isEqualTo:HBKitLocalizedString(@"None", @"HBSubtitles -> none track name")])
    {
        return @(hb_audio_encoder_get_from_name("none"));
    }
    else
    {
        return @(hb_audio_encoder_get_from_name([value UTF8String]));
    }
}

@end


@implementation HBEncoderTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_audio_encoder_get_name([value intValue]);
    if (name)
    {
        return @(name);
    }
    else
    {
        return nil;
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return @(hb_audio_encoder_get_from_name([value UTF8String]));
}

@end

@implementation HBMixdownTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_mixdown_get_name([value intValue]);
    if (name)
    {
        return @(name);
    }
    else
    {
        return nil;
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return @(hb_mixdown_get_from_name([value UTF8String]));
}

@end

@implementation HBSampleRateTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_audio_samplerate_get_name([value intValue]);
    if (name)
    {
        return @(name);
    }
    else
    {
        return nil;
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    int sampleRate = hb_audio_samplerate_get_from_name([value UTF8String]);
    if (sampleRate < 0)
    {
        sampleRate = 0;
    }
    return @(sampleRate);
}

@end

@implementation HBIntegerTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    // treat -1 as a special invalid value
    // e.g. passthru has no bitrate since we have no source
    if ([value intValue] == -1)
    {
        return @"N/A";
    }
    return [value stringValue];
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return @([value intValue]);
}

@end
