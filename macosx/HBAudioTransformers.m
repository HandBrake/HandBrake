/*  HBAudioTransformers.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTransformers.h"
#import "HBLocalizationUtilities.h"
#include "handbrake/handbrake.h"

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

static NSDictionary<NSString *, NSString *> *localizedMixdownsNames;
static NSDictionary<NSString *, NSNumber *> *localizedReversedMixdownsNames;

@implementation HBMixdownsTransformer

+ (void)initialize
{
    if (self == [HBMixdownsTransformer class]) {
        localizedMixdownsNames =
        @{@"None": HBKitLocalizedString(@"None", @"HBAudio -> Mixdown"),
          @"Mono": HBKitLocalizedString(@"Mono", @"HBAudio -> Mixdown"),
          @"Mono (Left Only)": HBKitLocalizedString(@"Mono (Left Only)", @"HBAudio -> Mixdown"),
          @"Mono (Right Only)": HBKitLocalizedString(@"Mono (Right Only)", @"HBAudio -> Mixdown"),
          @"Stereo": HBKitLocalizedString(@"Stereo", @"HBAudio -> Mixdown"),
          @"Dolby Surround": HBKitLocalizedString(@"Dolby Surround", @"HBAudio -> Mixdown"),
          @"Dolby Pro Logic II": HBKitLocalizedString(@"Dolby Pro Logic II", @"HBAudio -> Mixdown"),
          @"5.1 Channels": HBKitLocalizedString(@"5.1 Channels", @"HBAudio -> Mixdown"),
          @"6.1 Channels": HBKitLocalizedString(@"6.1 Channels", @"HBAudio -> Mixdown"),
          @"7.1 Channels": HBKitLocalizedString(@"7.1 Channels", @"HBAudio -> Mixdown"),
          @"7.1 (5F/2R/LFE)": HBKitLocalizedString(@"7.1 (5F/2R/LFE)", @"HBAudio -> Mixdown"),
          };

        localizedReversedMixdownsNames =
        @{HBKitLocalizedString(@"None", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_NONE),
          HBKitLocalizedString(@"Mono", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_MONO),
          HBKitLocalizedString(@"Mono (Left Only)", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_LEFT),
          HBKitLocalizedString(@"Mono (Right Only)", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_RIGHT),
          HBKitLocalizedString(@"Stereo", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_STEREO),
          HBKitLocalizedString(@"Dolby Surround", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_DOLBY),
          HBKitLocalizedString(@"Dolby Pro Logic II", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_DOLBYPLII),
          HBKitLocalizedString(@"5.1 Channels", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_5POINT1),
          HBKitLocalizedString(@"6.1 Channels", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_6POINT1),
          HBKitLocalizedString(@"7.1 Channels", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_7POINT1),
          HBKitLocalizedString(@"7.1 (5F/2R/LFE)", @"HBAudio -> Mixdown"): @(HB_AMIXDOWN_5_2_LFE),
          };
    }
}

+ (NSString *)localizedNameFromMixdown:(int)mixdown
{
    switch(mixdown)
    {
        case HB_AMIXDOWN_NONE:
            return HBKitLocalizedString(@"None", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_MONO:
            return HBKitLocalizedString(@"Mono", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_LEFT:
            return HBKitLocalizedString(@"Mono (Left Only)", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_RIGHT:
            return HBKitLocalizedString(@"Mono (Right Only)", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_STEREO:
            return HBKitLocalizedString(@"Stereo", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_DOLBY:
            return HBKitLocalizedString(@"Dolby Surround", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_DOLBYPLII:
            return HBKitLocalizedString(@"Dolby Pro Logic II", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_5POINT1:
            return HBKitLocalizedString(@"5.1 Channels", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_6POINT1:
            return HBKitLocalizedString(@"6.1 Channels", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_7POINT1:
            return HBKitLocalizedString(@"7.1 Channels", @"HBAudio -> Mixdown");
        case HB_AMIXDOWN_5_2_LFE:
            return HBKitLocalizedString(@"7.1 (5F/2R/LFE)", @"HBAudio -> Mixdown");
        default:
        {
            const char *name = hb_mixdown_get_name(mixdown);
            return name ? @(name) : nil;
        }
    }
}

+ (NSNumber *)mixdownFromLocalizedName:(NSString *)name
{
    NSNumber *mixdown = localizedReversedMixdownsNames[name];
    return mixdown ? mixdown : @(hb_mixdown_get_from_name(name.UTF8String));
}

+ (Class)transformedValueClass
{
    return [NSArray class];
}

- (id)transformedValue:(id)value
{
    if (value != nil)
    {
        NSMutableArray *localizedArray = [[NSMutableArray alloc] initWithCapacity:[value count]];

        for (NSString *text in value)
        {
            NSString *localizedName = localizedMixdownsNames[text];
            if (localizedName)
            {
                [localizedArray addObject:localizedName];
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

@implementation HBMixdownTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    return [HBMixdownsTransformer localizedNameFromMixdown:[value intValue]];
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return [HBMixdownsTransformer mixdownFromLocalizedName:value];
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
