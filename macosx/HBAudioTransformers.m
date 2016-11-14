//
//  HBAudioTransformers.m
//  HandBrake
//
//  Created by Damiano Galassi on 26/08/2016.
//
//

#import "HBAudioTransformers.h"
#include "hb.h"

#pragma mark - Value Trasformers

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
