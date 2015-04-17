/*  HBFilters+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters+UIAdditions.h"
#import "hb.h"

/**
 *  Converts a hb_filter_param_t * array to a NSArray of NSString.
 *
 *  @param f a function which returns a hb_filter_param_t * array
 *
 *  @return a NSArray that contains the name field of hb_filter_param_t.
 */
static NSArray * filterParamsToNamesArray(hb_filter_param_t * (f)(int), int filter_id) {
    NSMutableArray *presets = [NSMutableArray array];

    for (hb_filter_param_t *preset = f(filter_id); preset->name != NULL; preset++)
    {
        [presets addObject:@(preset->name)];
    }

    return [presets copy];
}

/**
 *  Converts a hb_filter_param_t * array to a NSDictionary, with name as the key and short_name as the value.
 *
 *  @param f a function which returns a hb_filter_param_t * array
 *
 *  @return a NSDictionary
 */
static NSDictionary * filterParamsToNamesDict(hb_filter_param_t * (f)(int), int filter_id) {
    NSMutableDictionary *presets = [NSMutableDictionary dictionary];

    for (hb_filter_param_t *preset = f(filter_id); preset->name != NULL; preset++)
    {
        [presets setObject:NSLocalizedString(@(preset->short_name), nil) forKey:@(preset->name)];
    }

    return [presets copy];
}

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
    return filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_NLMEANS);
}

+ (NSDictionary *)nlmeansTunesDict
{
    return filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_NLMEANS);
}

+ (NSDictionary *)denoiseTypesDict
{
    return @{NSLocalizedString(@"Off", nil):      @"off",
             NSLocalizedString(@"NLMeans", nil):  @"nlmeans",
             NSLocalizedString(@"HQDN3D", nil):   @"hqdn3d"};;
}

- (NSArray *)detelecineSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DETELECINE);
}

- (NSArray *)decombSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DECOMB);
}

- (NSArray *)deinterlaceSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DEINTERLACE);
}

- (NSArray *)denoiseTypes
{
    return @[@"Off", @"NLMeans", @"HQDN3D"];
}

- (NSArray *)denoisePresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_NLMEANS);
}

- (NSArray *)denoiseTunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_NLMEANS);
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
        [summary appendFormat:@" - Denoise (%@", self.denoise];
        if (![self.denoisePreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", self.denoisePreset];

            if ([self.denoise isEqualToString:@"nlmeans"])
            {
                [summary appendFormat:@", %@", self.denoiseTune];
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
