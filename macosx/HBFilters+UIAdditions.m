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

@implementation HBDetelecineTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters detelecinePresetsDict];

    return self;
}

@end

@implementation HBDeinterlaceTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters deinterlaceTypesDict];

    return self;
}

@end

@implementation HBDeinterlacePresetTransformer

- (instancetype)init
{
    if (self = [super init])
    {
        NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[HBFilters deinterlacePresetsDict]];
        [dict addEntriesFromDictionary:[HBFilters decombPresetsDict]];
        self.dict = [dict copy];
    }

    return self;
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

static NSDictionary *detelecinePresetsDict = nil;

static NSDictionary *deinterlaceTypesDict = nil;
static NSDictionary *decombPresetsDict = nil;
static NSDictionary *deinterlacePresetsDict = nil;

static NSDictionary *denoisePresetDict = nil;
static NSDictionary *nlmeansTunesDict = nil;
static NSDictionary *denoiseTypesDict = nil;

@implementation HBFilters (UIAdditions)

#pragma mark - Valid values

+ (NSDictionary *)detelecinePresetsDict
{
    if (!detelecinePresetsDict)
    {
        detelecinePresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_DETELECINE);
    }
    return detelecinePresetsDict;
}

+ (NSDictionary *)deinterlaceTypesDict
{
    if (!deinterlaceTypesDict)
    {
        deinterlaceTypesDict = @{NSLocalizedString(@"Off", nil):      @"off",
                                 NSLocalizedString(@"Deinterlace", nil):  @"deinterlace",
                                 NSLocalizedString(@"Decomb", nil):   @"decomb"};;
    }
    return deinterlaceTypesDict;
}

- (NSArray *)deinterlaceTypes
{
    return @[@"Off", @"Deinterlace", @"Decomb"];
}

+ (NSDictionary *)decombPresetsDict
{
    if (!decombPresetsDict)
    {
        decombPresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_DECOMB);
    }
    return decombPresetsDict;
}

+ (NSDictionary *)deinterlacePresetsDict
{
    if (!deinterlacePresetsDict)
    {
        deinterlacePresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_DEINTERLACE);
    }
    return deinterlacePresetsDict;
}

+ (NSDictionary *)denoisePresetDict
{
    if (!denoisePresetDict)
    {
        denoisePresetDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_NLMEANS);
    }
    return denoisePresetDict;
}

+ (NSDictionary *)nlmeansTunesDict
{
    if (!nlmeansTunesDict)
    {
        nlmeansTunesDict = filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_NLMEANS);
    }
    return nlmeansTunesDict;
}

+ (NSDictionary *)denoiseTypesDict
{
    if (!denoiseTypesDict)
    {
        denoiseTypesDict = @{NSLocalizedString(@"Off", nil):      @"off",
                             NSLocalizedString(@"NLMeans", nil):  @"nlmeans",
                             NSLocalizedString(@"HQDN3D", nil):   @"hqdn3d"};;
    }
    return denoiseTypesDict;
}

- (NSArray *)detelecineSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DETELECINE);
}

- (NSArray *)deinterlacePresets
{
    if ([self.deinterlace isEqualToString:@"deinterlace"])
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DEINTERLACE);
    }
    else
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DECOMB);
    }
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

- (BOOL)customDetelecineSelected
{
    return [self.detelecine isEqualToString:@"custom"] ? YES : NO;
}

- (BOOL)customDeinterlaceSelected
{
    return [self.deinterlacePreset isEqualToString:@"custom"] && [self deinterlaceEnabled];
}

- (BOOL)denoiseEnabled
{
    return ![self.denoise isEqualToString:@"off"];
}

- (BOOL)deinterlaceEnabled
{
    return ![self.deinterlace isEqualToString:@"off"];
}

- (BOOL)customDenoiseSelected
{
    return [self.denoisePreset isEqualToString:@"custom"] && [self denoiseEnabled];
}

- (BOOL)denoiseTunesAvailable
{
    return [self.denoise isEqualToString:@"nlmeans"] && ![self.denoisePreset isEqualToString:@"custom"];
}

- (NSString *)deblockSummary
{
    if (self.deblock == 0)
    {
        return NSLocalizedString(@"Off", nil);
    }
    else
    {
        return [NSString stringWithFormat: @"%.0ld", (long)self.deblock];
    }
}

- (NSString *)summary
{
    NSMutableString *summary = [NSMutableString string];

    // Detelecine
    if (![self.detelecine isEqualToString:@"off"])
    {
        if ([self.detelecine isEqualToString:@"custom"])
        {
            [summary appendFormat:@" - Detelecine (%@)", self.detelecineCustomString];
        }
        else
        {
            [summary appendFormat:@" - Detelecine (%@)", [[[HBFilters detelecinePresetsDict] allKeysForObject:self.detelecine] firstObject]];
        }
    }
    else if (![self.deinterlace isEqualToString:@"off"])
    {
        // Deinterlace or Decomb
        NSString *type =  [[[HBFilters deinterlaceTypesDict] allKeysForObject:self.deinterlace] firstObject];

        if ([self.deinterlacePreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@" - %@ (%@)", type, self.deinterlaceCustomString];
        }
        else
        {
            if ([self.deinterlace isEqualToString:@"decomb"])
            {
                [summary appendFormat:@" - %@ (%@)", type, [[[HBFilters decombPresetsDict] allKeysForObject:self.deinterlacePreset] firstObject]];
            }
            else if ([self.deinterlace isEqualToString:@"deinterlace"])
            {
                [summary appendFormat:@" - %@ (%@)", type, [[[HBFilters deinterlacePresetsDict] allKeysForObject:self.deinterlacePreset] firstObject]];
            }
        }
    }

    // Deblock
    if (self.deblock > 0)
    {
        [summary appendFormat:@" - Deblock (%d)", self.deblock];
    }

    // Denoise
    if (![self.denoise isEqualToString:@"off"])
    {
        [summary appendFormat:@" - Denoise (%@", [[[HBFilters denoiseTypesDict] allKeysForObject:self.denoise] firstObject]];
        if (![self.denoisePreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", [[[HBFilters denoisePresetDict] allKeysForObject:self.denoisePreset] firstObject]];

            if ([self.denoise isEqualToString:@"nlmeans"])
            {
                [summary appendFormat:@", %@", [[[HBFilters nlmeansTunesDict] allKeysForObject:self.denoiseTune] firstObject]];
            }
        }
        else
        {
            [summary appendFormat:@", %@", self.denoiseCustomString];
        }

        [summary appendString:@")"];

    }

    // Grayscale
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
