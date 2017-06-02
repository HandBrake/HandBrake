/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"
#import "HBCodingUtilities.h"
#import "NSDictionary+HBAdditions.h"
#import "HBMutablePreset.h"

#include "hb.h"

NSString * const HBFiltersChangedNotification = @"HBFiltersChangedNotification";

@interface HBFilters ()

@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@end

@implementation HBFilters

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _detelecine = @"off";
        _detelecineCustomString = @"";
        _combDetection = @"off";
        _combDetectionCustomString = @"";
        _deinterlace = @"off";
        _deinterlaceCustomString = @"";
        _deinterlacePreset = @"default";
        _denoise = @"off";
        _denoiseCustomString = @"";
        _denoisePreset = @"medium";
        _denoiseTune = @"none";
        _sharpen = @"off";
        _sharpenCustomString = @"";
        _sharpenPreset = @"medium";
        _sharpenTune = @"none";

        _notificationsEnabled = YES;
    }
    return self;
}

- (void)postChangedNotification
{
    if (self.areNotificationsEnabled)
    {
        [[NSNotificationCenter defaultCenter] postNotification: [NSNotification notificationWithName:HBFiltersChangedNotification
                                                                                              object:self
                                                                                            userInfo:nil]];
    }
}

#pragma mark - Setters

- (void)setDetelecine:(NSString *)detelecine
{
    if (![detelecine isEqualToString:_detelecine])
    {
        [[self.undo prepareWithInvocationTarget:self] setDetelecine:_detelecine];
    }
    if (detelecine)
    {
        _detelecine = [detelecine copy];
    }
    else
    {
        _detelecine = @"off";
    }
    [self postChangedNotification];
}

// Override setter to avoid nil values.
- (void)setDetelecineCustomString:(NSString *)detelecineCustomString
{
    if (![detelecineCustomString isEqualToString:_detelecineCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setDetelecineCustomString:_detelecineCustomString];
    }
    if (detelecineCustomString)
    {
        _detelecineCustomString = [detelecineCustomString copy];
    }
    else
    {
        _detelecineCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setCombDetection:(NSString *)combDetection
{
    if (![combDetection isEqualToString:_combDetection])
    {
        [[self.undo prepareWithInvocationTarget:self] setCombDetection:_combDetection];
    }
    if (combDetection)
    {
        _combDetection = [combDetection copy];
    }
    else
    {
        _combDetection = @"off";
    }
    [self postChangedNotification];
}

- (void)setCombDetectionCustomString:(NSString *)combDetectionCustomString
{
    if (![combDetectionCustomString isEqualToString:_combDetectionCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setCombDetectionCustomString:_combDetectionCustomString];
    }
    if (combDetectionCustomString)
    {
        _combDetectionCustomString = [combDetectionCustomString copy];
    }
    else
    {
        _combDetectionCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setDeinterlace:(NSString *)deinterlace
{
    if (![deinterlace isEqualToString:_deinterlace])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeinterlace:_deinterlace];
    }
    if (deinterlace)
    {
        _deinterlace = [deinterlace copy];
    }
    else
    {
        _deinterlace = @"off";
    }

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateDeinterlacePreset];
    }
    [self postChangedNotification];
}

- (void)setDeinterlacePreset:(NSString *)deinterlacePreset
{
    if (![deinterlacePreset isEqualToString:_deinterlacePreset])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeinterlacePreset:_deinterlacePreset];
    }

    if (deinterlacePreset)
    {
        _deinterlacePreset = [deinterlacePreset copy];
    }

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateDeinterlacePreset];
    }
    [self postChangedNotification];
}

- (void)validateDeinterlacePreset
{
    int filter_id = HB_FILTER_DECOMB;
    if ([self.deinterlace isEqualToString:@"deinterlace"])
    {
        filter_id = HB_FILTER_DEINTERLACE;
    }

    if (hb_validate_filter_preset(filter_id, self.deinterlacePreset.UTF8String, NULL, NULL))
    {
        self.deinterlacePreset = @"default";
    }
}

- (void)setDeinterlaceCustomString:(NSString *)deinterlaceCustomString
{
    if (![deinterlaceCustomString isEqualToString:_deinterlaceCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeinterlaceCustomString:_deinterlaceCustomString];
    }
    if (deinterlaceCustomString)
    {
        _deinterlaceCustomString = [deinterlaceCustomString copy];
    }
    else
    {
        _deinterlaceCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setDenoise:(NSString *)denoise
{
    if (![denoise isEqualToString:_denoise])
    {
        [[self.undo prepareWithInvocationTarget:self] setDenoise:_denoise];
    }
    if (denoise)
    {
        _denoise = [denoise copy];
    }
    else
    {
        _denoise = @"off";
    }

    [self postChangedNotification];
}

- (void)setDenoisePreset:(NSString *)denoisePreset
{
    if (![denoisePreset isEqualToString:_denoisePreset])
    {
        [[self.undo prepareWithInvocationTarget:self] setDenoisePreset:_denoisePreset];
    }
    if (denoisePreset)
    {
        _denoisePreset = [denoisePreset copy];
    }
    else
    {
        _denoisePreset = @"medium";
    }

    [self postChangedNotification];
}

- (void)setDenoiseTune:(NSString *)denoiseTune
{
    if (![denoiseTune isEqualToString:_denoiseTune])
    {
        [[self.undo prepareWithInvocationTarget:self] setDenoiseTune:_denoiseTune];
    }
    if (denoiseTune)
    {
        _denoiseTune = [denoiseTune copy];
    }
    else
    {
        _denoiseTune = @"none";
    }

    [self postChangedNotification];
}

- (void)setDenoiseCustomString:(NSString *)denoiseCustomString
{
    if (![denoiseCustomString isEqualToString:_denoiseCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setDenoiseCustomString:_denoiseCustomString];
    }
    if (denoiseCustomString)
    {
        _denoiseCustomString = [denoiseCustomString copy];
    }
    else
    {
        _denoiseCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setSharpen:(NSString *)sharpen
{
    if (![sharpen isEqualToString:_sharpen])
    {
        [[self.undo prepareWithInvocationTarget:self] setSharpen:_sharpen];
    }
    if (sharpen)
    {
        _sharpen = [sharpen copy];
    }
    else
    {
        _sharpen = @"off";
    }

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateSharpenPreset];
        [self validateSharpenTune];
    }
    [self postChangedNotification];
}

- (void)setSharpenPreset:(NSString *)sharpenPreset
{
    if (![sharpenPreset isEqualToString:_sharpenPreset])
    {
        [[self.undo prepareWithInvocationTarget:self] setSharpenPreset:_sharpenPreset];
    }
    if (sharpenPreset)
    {
        _sharpenPreset = [sharpenPreset copy];
    }
    else
    {
        _sharpenPreset = @"medium";
    }

    [self postChangedNotification];
}

- (void)validateSharpenPreset
{
    int filter_id = HB_FILTER_UNSHARP;
    if ([self.sharpen isEqualToString:@"lapsharp"])
    {
        filter_id = HB_FILTER_LAPSHARP;
    }

    if (hb_validate_filter_preset(filter_id, self.sharpenPreset.UTF8String, NULL, NULL))
    {
        self.sharpenPreset = @"medium";
    }
}

- (void)setSharpenTune:(NSString *)sharpenTune
{
    if (![sharpenTune isEqualToString:_sharpenTune])
    {
        [[self.undo prepareWithInvocationTarget:self] setSharpenTune:_sharpenTune];
    }
    if (sharpenTune)
    {
        _sharpenTune = [sharpenTune copy];
    }
    else
    {
        _sharpenTune = @"none";
    }

    [self postChangedNotification];
}

- (void)validateSharpenTune
{
    int filter_id = HB_FILTER_UNSHARP;
    if ([self.sharpen isEqualToString:@"lapsharp"])
    {
        filter_id = HB_FILTER_LAPSHARP;
    }

    if (hb_validate_filter_preset(filter_id, self.sharpenPreset.UTF8String, self.sharpenTune.UTF8String, NULL))
    {
        self.sharpenTune = @"none";
    }
}

- (void)setSharpenCustomString:(NSString *)sharpenCustomString
{
    if (![sharpenCustomString isEqualToString:_sharpenCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setSharpenCustomString:_sharpenCustomString];
    }
    if (sharpenCustomString)
    {
        _sharpenCustomString = [sharpenCustomString copy];
    }
    else
    {
        _sharpenCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setDeblock:(int)deblock
{
    if (deblock != _deblock)
    {
        [[self.undo prepareWithInvocationTarget:self] setDeblock:_deblock];
    }
    _deblock = deblock;
    [self postChangedNotification];
}

- (void)setGrayscale:(BOOL)grayscale
{
    if (grayscale != _grayscale)
    {
        [[self.undo prepareWithInvocationTarget:self] setGrayscale:_grayscale];
    }
    _grayscale = grayscale;
    [self postChangedNotification];
}

- (void)setRotate:(int)rotate
{
    if (rotate != _rotate)
    {
        [[self.undo prepareWithInvocationTarget:self] setRotate:_rotate];
    }
    _rotate = rotate;
    [self postChangedNotification];
}

- (void)setFlip:(BOOL)flip
{
    if (flip != _flip)
    {
        [[self.undo prepareWithInvocationTarget:self] setFlip:_flip];
    }
    _flip = flip;
    [self postChangedNotification];
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"summary"])
    {
        retval = [NSSet setWithObjects:@"detelecine", @"detelecineCustomString", @"deinterlace", @"deinterlacePreset", @"deinterlaceCustomString", @"denoise", @"denoisePreset", @"denoiseTune", @"denoiseCustomString", @"deblock", @"grayscale", @"sharpen", @"sharpenPreset", @"sharpenTune", @"sharpenCustomString", nil];
    }
    else if ([key isEqualToString:@"customDetelecineSelected"])
    {
        retval = [NSSet setWithObjects:@"detelecine", nil];
    }
    else if ([key isEqualToString:@"customCombDetectionSelected"])
    {
        retval = [NSSet setWithObjects:@"combDetection", nil];
    }
    else if ([key isEqualToString:@"denoiseTunesAvailable"] ||
             [key isEqualToString:@"customDenoiseSelected"])
    {
        retval = [NSSet setWithObjects:@"denoise", @"denoisePreset", nil];
    }
    else if ([key isEqualToString:@"denoiseEnabled"])
    {
        retval = [NSSet setWithObject:@"denoise"];
    }
    else if ([key isEqualToString:@"sharpenTunesAvailable"] ||
             [key isEqualToString:@"customSharpenSelected"])
    {
        retval = [NSSet setWithObjects:@"sharpen", @"sharpenPreset", nil];
    }
    else if ([key isEqualToString:@"sharpenEnabled"]  ||
             [key isEqualToString:@"sharpenPresets"] ||
             [key isEqualToString:@"sharpenTunes"])
    {
        retval = [NSSet setWithObject:@"sharpen"];
    }
    else if ([key isEqualToString:@"deinterlaceEnabled"])
    {
        retval = [NSSet setWithObject:@"deinterlace"];
    }
    else if ([key isEqualToString:@"customDeinterlaceSelected"] ||
             [key isEqualToString:@"deinterlacePresets"])
    {
        retval = [NSSet setWithObjects:@"deinterlace", @"deinterlacePreset", nil];
    }
    else if ([key isEqualToString:@"deblockSummary"])
    {
        retval = [NSSet setWithObject:@"deblock"];
    }
    else
    {
        retval = [NSSet set];
    }

    return retval;
}


#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBFilters *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_detelecine = [_detelecine copy];
        copy->_detelecineCustomString = [_detelecineCustomString copy];

        copy->_combDetection = [_combDetection copy];
        copy->_combDetectionCustomString = [_combDetectionCustomString copy];

        copy->_deinterlace = [_deinterlace copy];
        copy->_deinterlacePreset = [_deinterlacePreset copy];
        copy->_deinterlaceCustomString = [_deinterlaceCustomString copy];

        copy->_denoise = [_denoise copy];
        copy->_denoisePreset = [_denoisePreset copy];
        copy->_denoiseTune = [_denoiseTune copy];
        copy->_denoiseCustomString = [_denoiseCustomString copy];

        copy->_sharpen = [_sharpen copy];
        copy->_sharpenPreset = [_sharpenPreset copy];
        copy->_sharpenTune = [_sharpenTune copy];
        copy->_sharpenCustomString = [_sharpenCustomString copy];

        copy->_deblock = _deblock;
        copy->_grayscale = _grayscale;
        copy->_rotate = _rotate;
        copy->_flip = _flip;
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
    [coder encodeInt:2 forKey:@"HBFiltersVersion"];

    encodeObject(_detelecine);
    encodeObject(_detelecineCustomString);

    encodeObject(_combDetection);
    encodeObject(_combDetectionCustomString);

    encodeObject(_deinterlace);
    encodeObject(_deinterlacePreset);
    encodeObject(_deinterlaceCustomString);

    encodeObject(_denoise);
    encodeObject(_denoisePreset);
    encodeObject(_denoiseTune);
    encodeObject(_denoiseCustomString);

    encodeObject(_sharpen);
    encodeObject(_sharpenPreset);
    encodeObject(_sharpenTune);
    encodeObject(_sharpenCustomString);

    encodeInt(_deblock);
    encodeBool(_grayscale);
    encodeInt(_rotate);
    encodeBool(_flip);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeObjectOrFail(_detelecine, NSString);
    decodeObjectOrFail(_detelecineCustomString, NSString);

    decodeObjectOrFail(_combDetection, NSString);
    decodeObjectOrFail(_combDetectionCustomString, NSString);

    decodeObjectOrFail(_deinterlace, NSString);
    decodeObjectOrFail(_deinterlacePreset, NSString)
    decodeObjectOrFail(_deinterlaceCustomString, NSString);

    decodeObjectOrFail(_denoise, NSString);
    decodeObjectOrFail(_denoisePreset, NSString);
    decodeObjectOrFail(_denoiseTune, NSString);
    decodeObjectOrFail(_denoiseCustomString, NSString);

    decodeObjectOrFail(_sharpen, NSString);
    decodeObjectOrFail(_sharpenPreset, NSString);
    decodeObjectOrFail(_sharpenTune, NSString);
    decodeObjectOrFail(_sharpenCustomString, NSString);

    decodeInt(_deblock);
    decodeBool(_grayscale);
    decodeInt(_rotate);
    decodeBool(_flip);

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
}

#pragma mark - Presets and queue

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset[@"PictureDeinterlaceFilter"] = self.deinterlace;
    preset[@"PictureDeinterlacePreset"] = self.deinterlacePreset;
    preset[@"PictureDeinterlaceCustom"] = self.deinterlaceCustomString;

    preset[@"PictureCombDetectPreset"] = self.combDetection;
    preset[@"PictureCombDetectCustom"] = self.combDetectionCustomString;

    preset[@"PictureDetelecine"] = self.detelecine;
    preset[@"PictureDetelecineCustom"] = self.detelecineCustomString;

    preset[@"PictureDenoiseFilter"] = self.denoise;
    preset[@"PictureDenoisePreset"] = self.denoisePreset;
    preset[@"PictureDenoiseTune"] = self.denoiseTune;
    preset[@"PictureDenoiseCustom"] = self.denoiseCustomString;

    preset[@"PictureSharpenFilter"] = self.sharpen;
    preset[@"PictureSharpenPreset"] = self.sharpenPreset;
    preset[@"PictureSharpenTune"] = self.sharpenTune;
    preset[@"PictureSharpenCustom"] = self.sharpenCustomString;

    preset[@"PictureDeblock"] = @(self.deblock);
    preset[@"VideoGrayScale"] = @(self.grayscale);
    preset[@"PictureRotate"] = [NSString stringWithFormat:@"angle=%d:hflip=%d", self.rotate, self.flip];
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    self.notificationsEnabled = NO;

    // If the preset has an objectForKey:@"UsesPictureFilters", and handle the filters here
    if ([preset[@"UsesPictureFilters"] boolValue])
    {
        // Deinterlace
        self.deinterlace = preset[@"PictureDeinterlaceFilter"];
        self.deinterlacePreset = preset[@"PictureDeinterlacePreset"];
        self.deinterlaceCustomString = preset[@"PictureDeinterlaceCustom"];

        // Comb detection
        self.combDetection = preset[@"PictureCombDetectPreset"];
        self.combDetectionCustomString = preset[@"PictureCombDetectCustom"];

        // Detelecine
        self.detelecine = preset[@"PictureDetelecine"];
        self.detelecineCustomString = preset[@"PictureDetelecineCustom"];

        // Denoise
        self.denoise = preset[@"PictureDenoiseFilter"];
        self.denoisePreset = preset[@"PictureDenoisePreset"];
        self.denoiseTune = preset[@"PictureDenoiseTune"];

        self.denoiseCustomString = preset[@"PictureDenoiseCustom"];

        // Sharpen
        self.sharpen = preset[@"PictureSharpenFilter"];
        self.sharpenPreset = preset[@"PictureSharpenPreset"];
        self.sharpenTune = preset[@"PictureSharpenTune"];
        self.sharpenCustomString = preset[@"PictureSharpenCustom"];

        self.deblock = [preset[@"PictureDeblock"] intValue];
        self.grayscale = [preset[@"VideoGrayScale"] boolValue];

        // Rotate
        NSString *rotate = preset[@"PictureRotate"];
        hb_dict_t *hbdict = hb_parse_filter_settings(rotate.UTF8String);
        NSDictionary *dict = [[NSDictionary alloc] initWithHBDict:hbdict];
        hb_value_free(&hbdict);

        self.rotate = [dict[@"angle"] intValue];
        self.flip = [dict[@"hflip"] boolValue];
    }

    self.notificationsEnabled = YES;
}

@end
