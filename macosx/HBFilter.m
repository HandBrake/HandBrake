/*  HBFilter.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilter.h"
#import "HBCodingUtilities.h"
#import "HBLocalizationUtilities.h"
#import "NSDictionary+HBAdditions.h"
#import "HBMutablePreset.h"

#include "handbrake/handbrake.h"

extern NSString * const HBFiltersChangedNotification;

@interface HBFilter ()

@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@end

@implementation HBFilter

- (instancetype)initWithFilterID:(int)filterID
{
    self = [super init];
    if (self)
    {
        _filterID = filterID;
        _preset = @(hb_filter_param_get_default_preset(filterID));
        _tune   = @(hb_filter_param_get_default_tune(filterID));
        _custom = @"";
    }
    return self;
}

- (instancetype)initWithFilter:(NSString *)filter
                        preset:(NSString *)preset
                          tune:(NSString *)tune
                        custom:(NSString *)custom
{
    self = [super init];
    if (self)
    {
        _filterID = [HBFilter filterIDFromFilterName:filter];
        _preset = preset;
        _tune = tune;
        _custom = custom;
    }
    return self;
}

- (void)postChangedNotification
{
    if (self.areNotificationsEnabled)
    {
        NSNotification *notification = [NSNotification notificationWithName:HBFiltersChangedNotification
                                                                     object:self userInfo:nil];
        [NSNotificationCenter.defaultCenter postNotification:notification];
    }
}

- (NSString *)template
{
    NSMutableString *result = [[NSMutableString alloc] init];
    char **keys = hb_filter_get_keys(self.filterID);
    char  *colon = "";

    for (int ii = 0; keys[ii] != NULL; ii++)
    {
        int c = tolower(keys[ii][0]);
        [result appendFormat:@"%s%s=%c", colon, keys[ii], c];
        colon = ":";
    }
    hb_str_vfree(keys);

    return result;
}

- (NSString *)customTemplateForPreset:(NSString *)preset tune:(NSString *)tune
{
    hb_dict_t *settings;
    settings = hb_generate_filter_settings(self.filterID, preset.UTF8String, tune.UTF8String, NULL);
    char *str = hb_filter_settings_string(self.filterID, settings);
    hb_value_free(&settings);

    NSMutableString *result = [[NSMutableString alloc] init];
    char **split = hb_str_vsplit(str, ':');
    char  *colon = "";

    for (int ii = 0; split[ii] != NULL; ii++)
    {
        [result appendFormat:@"%s%s", colon, split[ii]];
        colon = ":";
    }
    hb_str_vfree(split);
    free(str);

    return result;
}

- (NSString *)invalidCustomRecoverySuggestion
{
    NSMutableString *result = [[NSMutableString alloc] init];
    [result appendString:HBKitLocalizedString(@"Syntax: ", @"HBFilters -> invalid filter custom settings error recovery suggestion")];

    [result appendString:[self template]];

    [result appendString:@"\n\n"];
    [result appendString:HBKitLocalizedString(@"Default: ", @"HBFilters -> invalid filter custom settings error recovery suggestion")];

    NSString *defaultCustomSettings = [self customTemplateForPreset:@(hb_filter_param_get_default_preset(self.filterID)) tune:@"none"];
    if (defaultCustomSettings.length)
    {
        [result appendString:defaultCustomSettings];
    }

    return result;
}

- (void)setPreset:(NSString *)preset
{
    if (![preset isEqualToString:_preset])
    {
        [[self.undo prepareWithInvocationTarget:self] setPreset:_preset];
    }

    NSString *oldValue = _preset;

    if (preset)
    {
        _preset = [preset copy];
    }

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validatePreset];

        if ([preset isEqualToString:@"custom"])
        {
            self.custom = [self customTemplateForPreset:oldValue tune:self.tune];
        }
    }
    [self postChangedNotification];
}

- (void)validatePreset
{
    if (hb_validate_filter_preset(self.filterID, self.preset.UTF8String, NULL, NULL))
    {
        self.preset = @(hb_filter_param_get_default_preset(self.filterID));
    }
}

- (void)setTune:(NSString *)tune
{
    if (![tune isEqualToString:_tune])
    {
        [[self.undo prepareWithInvocationTarget:self] setTune:_tune];
    }
    if (tune)
    {
        _tune = [tune copy];
    }
    else
    {
        _tune = @"none";
    }

    [self postChangedNotification];
}

- (void)validateTune
{
    if (hb_validate_filter_preset(self.filterID, self.preset.UTF8String, self.tune.UTF8String, NULL))
    {
        self.tune = @"none";
    }
}

- (void)setCustom:(NSString *)custom
{
    if (![custom isEqualToString:_custom])
    {
        [[self.undo prepareWithInvocationTarget:self] setCustom:_custom];
    }
    if (custom)
    {
        _custom = [custom copy];
    }
    else
    {
        _custom = @"";
    }

    [self postChangedNotification];
}

- (BOOL)validateCustom:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;
        hb_dict_t *filter_dict = hb_generate_filter_settings(self.filterID,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                NSDictionary *userInfo = @{NSLocalizedDescriptionKey:HBKitLocalizedString(@"Invalid custom settings.",
                                                                                        @"HBFilters -> invalid custom string description"),
                                           NSLocalizedRecoverySuggestionErrorKey:[self invalidCustomRecoverySuggestion]};
                *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - Utilities

+ (int)filterIDFromFilterName:(NSString *)name
{
    return hb_filter_get_from_name(name.UTF8String);
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBFilter *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_filterID = _filterID;
        copy->_preset = [_preset copy];
        copy->_tune = [_tune copy];
        copy->_custom = [_custom copy];
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBFilterVersion"];

    encodeInt(_filterID);
    encodeObject(_preset);
    encodeObject(_tune);
    encodeObject(_custom);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_filterID);
    decodeObjectOrFail(_preset, NSString);
    decodeObject(_tune, NSString);
    decodeObject(_custom, NSString);

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset
{
    if (self.filterID == HB_FILTER_DECOMB ||
        self.filterID == HB_FILTER_YADIF ||
        self.filterID == HB_FILTER_YADIF)
    {
        preset[@"PictureDeinterlaceFilter"] = @(hb_filter_get_short_name(self.filterID));
        preset[@"PictureDeinterlacePreset"] = self.preset;
        preset[@"PictureDeinterlaceCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_COMB_DETECT)
    {
        preset[@"PictureCombDetectPreset"] = self.preset;
        preset[@"PictureCombDetectCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_DETELECINE)
    {
        preset[@"PictureDetelecine"]       = self.preset;
        preset[@"PictureDetelecineCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_DEBAND)
    {
        preset[@"PictureDebandPreset"] = self.preset;
        preset[@"PictureDebandCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_HQDN3D ||
             self.filterID == HB_FILTER_NLMEANS ||
             self.filterID == HB_FILTER_BM3D)
    {
        preset[@"PictureDenoiseFilter"] = @(hb_filter_get_short_name(self.filterID));
        preset[@"PictureDenoisePreset"] = self.preset;
        preset[@"PictureDenoiseTune"]   = self.tune;
        preset[@"PictureDenoiseCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_CHROMA_SMOOTH)
    {
        preset[@"PictureChromaSmoothPreset"] = self.preset;
        preset[@"PictureChromaSmoothTune"]   = self.tune;
        preset[@"PictureChromaSmoothCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_LAPSHARP ||
             self.filterID == HB_FILTER_UNSHARP)
    {
        preset[@"PictureSharpenFilter"] = @(hb_filter_get_short_name(self.filterID));
        preset[@"PictureSharpenPreset"] = self.preset;
        preset[@"PictureSharpenTune"]   = self.tune;
        preset[@"PictureSharpenCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_DEBLOCK)
    {
        preset[@"PictureDeblockPreset"] = self.preset;
        preset[@"PictureDeblockTune"]   = self.tune;
        preset[@"PictureDeblockCustom"] = self.custom;
    }
    else if (self.filterID == HB_FILTER_GRAYSCALE)
    {
        preset[@"VideoGrayScale"] = @YES;
    }
    else if (self.filterID == HB_FILTER_COLORSPACE)
    {
        preset[@"PictureColorspacePreset"] = self.preset;
        preset[@"PictureColorspaceCustom"] = self.custom;
    }
}

- (BOOL)applyPreset:(HBPreset *)preset error:(NSError *__autoreleasing *)outError
{
    return YES;
}


@end
