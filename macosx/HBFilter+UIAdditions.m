/*  HBFilter+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilter+UIAdditions.h"
#import "HBLocalizationUtilities.h"
#import "HBFilters.h"

#import "handbrake/handbrake.h"

///  Converts a hb_filter_param_t * array to a NSArray of NSString.
///  @param f a function which returns a hb_filter_param_t * array
///  @return a NSArray that contains the name field of hb_filter_param_t.
static NSArray<NSString *> * filterParamsToNamesArray(hb_filter_param_t * (f)(int), int filter_id)
{
    NSMutableArray *presets = [NSMutableArray array];

    for (hb_filter_param_t *preset = f(filter_id); preset != NULL && preset->short_name != NULL; preset++)
    {
        NSString *name = @(preset->name);
        if ([name isEqualToString:@"Off"])
        {
            continue;
        }
        else if ([name isEqualToString:@"Custom"])
        {
            name = NSLocalizedStringFromTableInBundle(@"Custom", nil, [NSBundle bundleForClass:[HBFilter class]], "HBFilters -> custom display name");
        }
        else if ([name isEqualToString:@"Default"])
        {
            name = NSLocalizedStringFromTableInBundle(@"Default", nil, [NSBundle bundleForClass:[HBFilter class]], "HBFilters -> default display name");
        }
        [presets addObject:name];
    }

    return [presets copy];
}

///  Converts a hb_filter_param_t * array to a NSDictionary, with name as the key and short_name as the value.
///  @param f a function which returns a hb_filter_param_t * array
///  @return a NSDictionary
static NSDictionary * filterParamsToNamesDict(hb_filter_param_t * (f)(int), int filter_id) {
    NSMutableDictionary *presets = [NSMutableDictionary dictionary];

    for (hb_filter_param_t *preset = f(filter_id); preset != NULL && preset->name != NULL; preset++)
    {
        NSString *name = @(preset->name);
        if ([name isEqualToString:@"Off"]) {
            name = NSLocalizedStringFromTableInBundle(@"Off", nil, [NSBundle bundleForClass:[HBFilter class]], "HBFilters -> off display name");
        }
        else if ([name isEqualToString:@"Custom"]) {
            name = NSLocalizedStringFromTableInBundle(@"Custom", nil, [NSBundle bundleForClass:[HBFilter class]], "HBFilters -> custom display name");
        }
        else if ([name isEqualToString:@"Default"]) {
            name = NSLocalizedStringFromTableInBundle(@"Default", nil, [NSBundle bundleForClass:[HBFilter class]], "HBFilters -> default display name");
        }
        [presets setObject:@(preset->short_name) forKey:name];
    }

    return [presets copy];
}


@implementation HBFilter (UIAdditions)

- (BOOL)customSelected
{
    return [self.preset isEqualToString:@"custom"];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomSelected
{
    return [NSSet setWithObjects:@"preset", nil];
}

+ (NSString *)localizedNameForFilterID:(int)filterID
{
    switch (filterID)
    {
        case HB_FILTER_DETELECINE:
            return HBKitLocalizedString(@"Detelecine", @"Detelecine filter");
        case HB_FILTER_COMB_DETECT:
            return HBKitLocalizedString(@"Comb Detect", @"Detelecine filter");
        case HB_FILTER_DECOMB:
            return HBKitLocalizedString(@"Decomb", @"Detelecine filter");
        case HB_FILTER_YADIF:
            return HBKitLocalizedString(@"Yadif", @"Detelecine filter");
        case HB_FILTER_BWDIF:
            return HBKitLocalizedString(@"Bwdif", @"Detelecine filter");
        case HB_FILTER_DEBLOCK:
            return HBKitLocalizedString(@"Deblock", @"Detelecine filter");
        case HB_FILTER_DEBAND:
            return HBKitLocalizedString(@"Deband", @"Detelecine filter");
        case HB_FILTER_DENOISE:
            return HBKitLocalizedString(@"HQDN3D", @"HQDN3D filter");
        case HB_FILTER_BM3D:
            return HBKitLocalizedString(@"BM3D", @"Detelecine filter");
        case HB_FILTER_NLMEANS:
            return HBKitLocalizedString(@"NLMeans", @"Detelecine filter");
        case HB_FILTER_CHROMA_SMOOTH:
            return HBKitLocalizedString(@"Chroma Smooth", @"Detelecine filter");
        case HB_FILTER_LAPSHARP:
            return HBKitLocalizedString(@"Lapsharp", @"Detelecine filter");
        case HB_FILTER_UNSHARP:
            return HBKitLocalizedString(@"Unsharp", @"Detelecine filter");
        case HB_FILTER_GRAYSCALE:
            return HBKitLocalizedString(@"Grayscale", @"Detelecine filter");
        case HB_FILTER_COLORSPACE:
            return HBKitLocalizedString(@"Colorspace", @"Detelecine filter");
        case HB_FILTER_INVALID:
        default:
            return HBKitLocalizedString(@"Invalid", @"Detelecine filter");
    }
}

- (NSString *)localizedName
{
    return [HBFilter localizedNameForFilterID:self.filterID];
}

- (NSArray<NSString *> *)presets
{
    return filterParamsToNamesArray(hb_filter_param_get_presets, self.filterID);
}

- (NSArray<NSString *> *)tunes
{
    return filterParamsToNamesArray(hb_filter_param_get_tunes, self.filterID);
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingTunesAvailable
{
    return [NSSet setWithObjects:@"preset", nil];
}

- (BOOL)tunesAvailable
{
    return [filterParamsToNamesArray(hb_filter_param_get_tunes, self.filterID) count] != 0 && self.customSelected == NO;
}

@end

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

@implementation HBFilterTuneTransformer

- (instancetype)initWithWithFilterID:(int)filterID
{
    if (self = [super init])
    {
        self.dict = filterParamsToNamesDict(hb_filter_param_get_tunes, filterID);
    }

    return self;
}

@end

@implementation HBFilterPresetTransformer

- (instancetype)initWithWithFilterID:(int)filterID
{
    if (self = [super init])
    {
        self.dict = filterParamsToNamesDict(hb_filter_param_get_presets, filterID);
    }

    return self;
}

@end
