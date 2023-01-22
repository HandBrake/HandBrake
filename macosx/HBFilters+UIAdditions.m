/*  HBFilters+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters+UIAdditions.h"
#import "HBLocalizationUtilities.h"

#import "handbrake/handbrake.h"

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
            name = NSLocalizedStringFromTableInBundle(@"Off", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> off display name");
        }
        else if ([name isEqualToString:@"Custom"]) {
            name = NSLocalizedStringFromTableInBundle(@"Custom", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> custom display name");
        }
        else if ([name isEqualToString:@"Default"]) {
            name = NSLocalizedStringFromTableInBundle(@"Default", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> default display name");
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
            name = NSLocalizedStringFromTableInBundle(@"Off", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> off display name");
        }
        else if ([name isEqualToString:@"Custom"]) {
            name = NSLocalizedStringFromTableInBundle(@"Custom", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> custom display name");
        }
        else if ([name isEqualToString:@"Default"]) {
            name = NSLocalizedStringFromTableInBundle(@"Default", nil, [NSBundle bundleForClass:[HBFilters class]], "HBFilters -> default display name");
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
    if (value)
    {
        return [[self.dict allKeysForObject:value] firstObject];
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
        NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[HBFilters yadifPresetsDict]];
        [dict addEntriesFromDictionary:[HBFilters decombPresetsDict]];
        [dict addEntriesFromDictionary:[HBFilters bwdifPresetsDict]];
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

@implementation HBChromaSmoothTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters chromaSmoothPresetDict];

    return self;
}

@end

@implementation HBChromaSmoothTuneTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters chromaSmoothTunesDict];

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

@implementation HBDeblockTuneTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters deblockTunesDict];

    return self;
}

@end

@implementation HBDeblockTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters deblockPresetDict];

    return self;
}

@end

@implementation HBColorspaceTransformer

- (instancetype)init
{
    if (self = [super init])
        self.dict = [HBFilters colorspacePresetDict];

    return self;
}

@end

static NSDictionary *detelecinePresetsDict = nil;

static NSDictionary *combDetectionPresetsDict = nil;

static NSDictionary *deinterlaceTypesDict = nil;
static NSDictionary *decombPresetsDict = nil;
static NSDictionary *yadifPresetsDict = nil;
static NSDictionary *bwdifPresetsDict = nil;

static NSDictionary *denoisePresetDict = nil;
static NSDictionary *nlmeansTunesDict = nil;
static NSDictionary *denoiseTypesDict = nil;

static NSDictionary *chromaSmoothPresetDict = nil;
static NSDictionary *chromaSmoothTunesDict = nil;

static NSDictionary *sharpenPresetDict = nil;
static NSDictionary *sharpenTunesDict = nil;
static NSDictionary *sharpenTypesDict = nil;

static NSDictionary *deblockPresetDict = nil;
static NSDictionary *deblockTunesDict = nil;

static NSDictionary *colorspacePresetDict = nil;

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
        deinterlaceTypesDict = @{HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"):        @"off",
                                 HBKitLocalizedString(@"Yadif", @"HBFilters -> filter display name"):      @"deinterlace",
                                 HBKitLocalizedString(@"Decomb", @"HBFilters -> filter display name"):     @"decomb",
                                 HBKitLocalizedString(@"Bwdif", @"HBFilters -> filter display name"):      @"bwdif"};;
    }
    return deinterlaceTypesDict;
}

- (NSArray *)deinterlaceTypes
{
    return @[HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"),
             HBKitLocalizedString(@"Yadif", @"HBFilters -> filter display name"),
             HBKitLocalizedString(@"Decomb", @"HBFilters -> filter display name"),
             HBKitLocalizedString(@"Bwdif", @"HBFilters -> filter display name")];
}

+ (NSDictionary *)decombPresetsDict
{
    if (!decombPresetsDict)
    {
        decombPresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_DECOMB);
    }
    return decombPresetsDict;
}

+ (NSDictionary *)yadifPresetsDict
{
    if (!yadifPresetsDict)
    {
        yadifPresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_YADIF);
    }
    return yadifPresetsDict;
}

+ (NSDictionary *)bwdifPresetsDict
{
    if (!bwdifPresetsDict)
    {
        bwdifPresetsDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_BWDIF);
    }
    return bwdifPresetsDict;
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
        denoiseTypesDict = @{HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"):      @"off",
                             HBKitLocalizedString(@"NLMeans", @"HBFilters -> filter display name"):  @"nlmeans",
                             HBKitLocalizedString(@"HQDN3D", @"HBFilters -> filter display name"):   @"hqdn3d"};;
    }
    return denoiseTypesDict;
}

+ (NSDictionary *)chromaSmoothPresetDict
{
    if (!chromaSmoothPresetDict)
    {
        chromaSmoothPresetDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_CHROMA_SMOOTH);
    }
    return chromaSmoothPresetDict;
}

+ (NSDictionary *)chromaSmoothTunesDict
{
    if (!chromaSmoothTunesDict)
    {
        chromaSmoothTunesDict = filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_CHROMA_SMOOTH);
    }
    return chromaSmoothTunesDict;
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
        sharpenTypesDict = @{HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"):      @"off",
                             HBKitLocalizedString(@"Unsharp", @"HBFilters -> filter display name"):  @"unsharp",
                             HBKitLocalizedString(@"Lapsharp", @"HBFilters -> filter display name"): @"lapsharp"};
    }
    return sharpenTypesDict;
}

+ (NSDictionary *)deblockPresetDict
{
    if (!deblockPresetDict)
    {
        deblockPresetDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_DEBLOCK);
    }
    return deblockPresetDict;
}

+ (NSDictionary *)deblockTunesDict
{
    if (!deblockTunesDict)
    {
        deblockTunesDict = filterParamsToNamesDict(hb_filter_param_get_tunes, HB_FILTER_DEBLOCK);
    }
    return deblockTunesDict;
}

+ (NSDictionary *)colorspacePresetDict
{
    if (!colorspacePresetDict)
    {
        colorspacePresetDict = filterParamsToNamesDict(hb_filter_param_get_presets, HB_FILTER_COLORSPACE);
    }
    return colorspacePresetDict;
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
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_YADIF);
    }
    else if ([self.deinterlace isEqualToString:@"bwdif"])
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_BWDIF);
    }
    else
    {
        return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DECOMB);
    }
}

- (NSArray *)denoiseTypes
{
    return @[HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"), HBKitLocalizedString(@"NLMeans", @"HBFilters -> filter display name"), HBKitLocalizedString(@"HQDN3D", @"HBFilters -> filter display name")];
}

- (NSArray *)denoisePresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_NLMEANS);
}

- (NSArray *)denoiseTunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_NLMEANS);
}

- (NSArray *)chromaSmoothPresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_CHROMA_SMOOTH);
}

- (NSArray *)chromaSmoothTunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_CHROMA_SMOOTH);
}

- (NSArray *)sharpenTypes
{
    return @[HBKitLocalizedString(@"Off", @"HBFilters -> filter display name"), HBKitLocalizedString(@"Unsharp", @"HBFilters -> filter display name"), HBKitLocalizedString(@"Lapsharp", @"HBFilters -> filter display name")];
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

- (NSArray *)deblockPresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_DEBLOCK);
}

- (NSArray *)deblockTunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, HB_FILTER_DEBLOCK);
}

- (NSArray *)colorspacePresets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, HB_FILTER_COLORSPACE);
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

- (BOOL)chromaSmoothEnabled
{
    return ![self.chromaSmooth isEqualToString:@"off"];
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

- (BOOL)customChromaSmoothSelected
{
    return [self.chromaSmooth isEqualToString:@"custom"];
}

- (BOOL)customSharpenSelected
{
    return [self.sharpenPreset isEqualToString:@"custom"] && [self sharpenEnabled];
}

- (BOOL)sharpenTunesAvailable
{
    return ([self.sharpen isEqualToString:@"unsharp"] || [self.sharpen isEqualToString:@"lapsharp"]) && ![self.sharpenPreset isEqualToString:@"custom"];
}

- (BOOL)deblockTunesAvailable
{
    return ![self.deblock isEqualToString:@"off"] && ![self.deblock isEqualToString:@"custom"];
}

- (BOOL)customDeblockSelected
{
    return [self.deblock isEqualToString:@"custom"];
}

- (BOOL)customColorspaceSelected
{
    return [self.colorspace isEqualToString:@"custom"];
}

@end
