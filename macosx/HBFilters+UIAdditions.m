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
        NSString *name = @(preset->name);
        if ([name isEqualToString:@"Off"]) {
            name = NSLocalizedString(@"Off", @"HBFilters -> off display name");
        }
        [presets addObject:name];
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
        NSString *name = @(preset->name);
        if ([name isEqualToString:@"Off"]) {
            name = NSLocalizedString(@"Off", @"HBFilters -> off display name");
        }
        [presets setObject:@(preset->short_name) forKey:name];
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

@implementation HBCombDetectionTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters combDetectionPresetsDict];

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

@implementation HBSharpenPresetTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters sharpenPresetDict];

    return self;
}

@end

@implementation HBSharpenTuneTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters sharpenTunesDict];

    return self;
}

@end

@implementation HBSharpenTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters sharpenTypesDict];

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

static NSDictionary *combDetectionPresetsDict = nil;

static NSDictionary *deinterlaceTypesDict = nil;
static NSDictionary *decombPresetsDict = nil;
static NSDictionary *deinterlacePresetsDict = nil;

static NSDictionary *denoisePresetDict = nil;
static NSDictionary *nlmeansTunesDict = nil;
static NSDictionary *denoiseTypesDict = nil;

static NSDictionary *sharpenPresetDict = nil;
static NSDictionary *sharpenTunesDict = nil;
static NSDictionary *sharpenTypesDict = nil;

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

+ (NSDictionary *)combDetectionPresetsDict
{
    if (!combDetectionPresetsDict)
    {
        combDetectionPresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_COMB_DETECT);
    }
    return combDetectionPresetsDict;
}


+ (NSDictionary *)deinterlaceTypesDict
{
    if (!deinterlaceTypesDict)
    {
        deinterlaceTypesDict = @{NSLocalizedString(@"Off", @"HBFilters -> filter display name"):        @"off",
                                 NSLocalizedString(@"Yadif", @"HBFilters -> filter display name"):      @"deinterlace",
                                 NSLocalizedString(@"Decomb", @"HBFilters -> filter display name"):     @"decomb"};;
    }
    return deinterlaceTypesDict;
}

- (NSArray *)deinterlaceTypes
{
    return @[NSLocalizedString(@"Off", @"HBFilters -> filter display name"), NSLocalizedString(@"Yadif", @"HBFilters -> filter display name"), NSLocalizedString(@"Decomb", @"HBFilters -> filter display name")];
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
        denoiseTypesDict = @{NSLocalizedString(@"Off", @"HBFilters -> filter display name"):      @"off",
                             NSLocalizedString(@"NLMeans", @"HBFilters -> filter display name"):  @"nlmeans",
                             NSLocalizedString(@"HQDN3D", @"HBFilters -> filter display name"):   @"hqdn3d"};;
    }
    return denoiseTypesDict;
}

+ (NSDictionary *)sharpenPresetDict
{
    if (!sharpenPresetDict)
    {
        sharpenPresetDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_UNSHARP);
    }
    return sharpenPresetDict;
}

+ (NSDictionary *)sharpenTunesDict
{
    if (!sharpenTunesDict)
    {
        NSDictionary *unsharpenTunesDict = filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_UNSHARP);
        NSDictionary *lapsharpTunesDict = filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_LAPSHARP);

        sharpenTunesDict = [NSMutableDictionary dictionary];
        [sharpenTunesDict setValuesForKeysWithDictionary:unsharpenTunesDict];
        [sharpenTunesDict setValuesForKeysWithDictionary:lapsharpTunesDict];

    }
    return sharpenTunesDict;
}

+ (NSDictionary *)sharpenTypesDict
{
    if (!sharpenTypesDict)
    {
        sharpenTypesDict = @{NSLocalizedString(@"Off", @"HBFilters -> filter display name"):      @"off",
                             NSLocalizedString(@"Unsharp", @"HBFilters -> filter display name"):  @"unsharp",
                             NSLocalizedString(@"Lapsharp", @"HBFilters -> filter display name"): @"lapsharp"};;
    }
    return sharpenTypesDict;
}

- (NSArray *)detelecineSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DETELECINE);
}

- (NSArray *)combDetectionSettings
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_COMB_DETECT);
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
    return @[NSLocalizedString(@"Off", @"HBFilters -> filter display name"), NSLocalizedString(@"NLMeans", @"HBFilters -> filter display name"), NSLocalizedString(@"HQDN3D", @"HBFilters -> filter display name")];
}

- (NSArray *)denoisePresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_NLMEANS);
}

- (NSArray *)denoiseTunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_NLMEANS);
}

- (NSArray *)sharpenTypes
{
    return @[NSLocalizedString(@"Off", @"HBFilters -> filter display name"), NSLocalizedString(@"Unsharp", @"HBFilters -> filter display name"), NSLocalizedString(@"Lapsharp", @"HBFilters -> filter display name")];
}

- (NSArray *)sharpenPresets
{
    if ([self.sharpen isEqualToString:@"unsharp"])
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_UNSHARP);
    }
    else
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_LAPSHARP);
    }
}

- (NSArray *)sharpenTunes
{
    if ([self.sharpen isEqualToString:@"unsharp"])
    {
        return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_UNSHARP);
    }
    else
    {
        return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_LAPSHARP);
    }
}

- (BOOL)customDetelecineSelected
{
    return [self.detelecine isEqualToString:@"custom"] ? YES : NO;
}

- (BOOL)customCombDetectionSelected
{
    return [self.combDetection isEqualToString:@"custom"] ? YES : NO;
}

- (BOOL)customDeinterlaceSelected
{
    return [self.deinterlacePreset isEqualToString:@"custom"] && [self deinterlaceEnabled];
}

- (BOOL)denoiseEnabled
{
    return ![self.denoise isEqualToString:@"off"];
}

- (BOOL)sharpenEnabled
{
    return ![self.sharpen isEqualToString:@"off"];
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

- (BOOL)customSharpenSelected
{
    return [self.sharpenPreset isEqualToString:@"custom"] && [self sharpenEnabled];
}

- (BOOL)sharpenTunesAvailable
{
    return ([self.sharpen isEqualToString:@"unsharp"] || [self.sharpen isEqualToString:@"lapsharp"]) && ![self.sharpenPreset isEqualToString:@"custom"];
}

- (NSString *)deblockSummary
{
    if (self.deblock == 0)
    {
        return NSLocalizedString(@"Off", @"HBFilters -> filter summary");
    }
    else
    {
        return [NSString stringWithFormat: @"%.0ld", (long)self.deblock];
    }
}

@end
