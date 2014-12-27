/*  HBFilters+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters+UIAdditions.h"

extern NSDictionary *_HandBrake_denoiseTypesDict;
extern NSDictionary *_HandBrake_denoisePresetsDict;
extern NSDictionary *_HandBrake_nlmeansTunesDict;

@implementation HBGenericDictionaryTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    return [[self.dict allKeysForObject:value] firstObject];
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return [self.dict valueForKey:value];
}

@end

@implementation HBDenoisePresetTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters denoisePresetDict];

    return self;
}

@end

@implementation HBDenoiseTuneTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters nlmeansTunesDict];

    return self;
}

@end

@implementation HBDenoiseTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters denoiseTypesDict];

    return self;
}

@end

@implementation HBCustomFilterTransformer

+ (Class)transformedValueClass
{
    return [NSNumber class];
}

- (id)transformedValue:(id)value
{
    if ([value intValue] == 1)
        return @NO;
    else
        return @YES;
}

+ (BOOL)allowsReverseTransformation
{
    return NO;
}

@end

@implementation HBFilters (UIAdditions)

#pragma mark - Valid values

+ (NSDictionary *)denoisePresetDict
{
    return _HandBrake_denoisePresetsDict;
}

+ (NSDictionary *)nlmeansTunesDict
{
    return _HandBrake_nlmeansTunesDict;
}

+ (NSDictionary *)denoiseTypesDict
{
    return _HandBrake_denoiseTypesDict;
}

- (NSArray *)detelecineSettings
{
    return @[@"Off", @"Custom", @"Default"];
}

- (NSArray *)decombSettings
{
    return @[@"Off", @"Custom", @"Default", @"Fast", @"Bob"];
}

- (NSArray *)deinterlaceSettings
{
    return @[@"Off", @"Custom", @"Fast", @"Slow", @"Slower", @"Bob"];
}

- (NSArray *)denoiseTypes
{
    return @[@"Off", @"NLMeans", @"HQDN3D"];
}

- (NSArray *)denoisePresets
{
    return @[@"Custom", @"Ultralight", @"Light", @"Medium", @"Strong"];
}

- (NSArray *)denoiseTunes
{
    return @[@"None", @"Film", @"Grain", @"High Motion", @"Animation"];
}

- (NSString *)summary
{
    NSMutableString *summary = [NSMutableString string];

    /* Detelecine */
    switch (self.detelecine)
    {
        case 1:
            [summary appendFormat:@" - Detelecine (%@)", self.detelecineCustomString];
            break;

        case 2:
            [summary appendString:@" - Detelecine (Default)"];
            break;

        default:
            break;
    }

    if (self.useDecomb)
    {
        /* Decomb */
        switch (self.decomb)
        {
            case 1:
                [summary appendFormat:@" - Decomb (%@)", self.decombCustomString];
                break;

            case 2:
                [summary appendString:@" - Decomb (Default)"];
                break;

            case 3:
                [summary appendString:@" - Decomb (Fast)"];
                break;

            case 4:
                [summary appendString:@" - Decomb (Bob)"];
                break;

            default:
                break;
        }
    }
    else
    {
        /* Deinterlace */
        switch (self.deinterlace)
        {
            case 1:
                [summary appendFormat:@" - Deinterlace (%@)", self.deinterlaceCustomString];
                break;

            case 2:
                [summary appendString:@" - Deinterlace (Fast)"];
                break;

            case 3:
                [summary appendString:@" - Deinterlace (Slow)"];
                break;

            case 4:
                [summary appendString:@" - Deinterlace (Slower)"];
                break;

            case 5:
                [summary appendString:@" - Deinterlace (Bob)"];
                break;

            default:
                break;
        }
    }

    /* Deblock */
    if (self.deblock > 0)
    {
        [summary appendFormat:@" - Deblock (%ld)", self.deblock];
    }

    /* Denoise */
    if (![self.denoise isEqualToString:@"off"])
    {
        [summary appendFormat:@" - Denoise (%@", [[_HandBrake_denoiseTypesDict allKeysForObject:self.denoise] firstObject]];
        if (![self.denoisePreset isEqualToString:@"none"])
        {
            [summary appendFormat:@", %@", [[_HandBrake_denoisePresetsDict allKeysForObject:self.denoisePreset] firstObject]];

            if ([self.denoise isEqualToString:@"nlmeans"])
            {
                [summary appendFormat:@", %@", [[_HandBrake_nlmeansTunesDict allKeysForObject:self.denoiseTune] firstObject]];
            }
        }
        else
        {
            [summary appendFormat:@", %@", self.denoiseCustomString];
        }

        [summary appendString:@")"];

    }

    /* Grayscale */
    if (self.grayscale)
    {
        [summary appendString:@" - Grayscale"];
    }
    
    if ([summary hasPrefix:@" - "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 3)];
    }
    
    return [NSString stringWithString:summary];
}

@end
