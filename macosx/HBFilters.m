/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"
#import "HBCodingUtilities.h"
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
        _deinterlace = @"off";
        _deinterlaceCustomString = @"";
        _decomb = @"off";
        _decombCustomString = @"";
        _denoise = @"off";
        _denoiseCustomString = @"";
        _denoisePreset = @"medium";
        _denoiseTune = @"none";

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

- (void)setDeinterlace:(NSString *)deinterlace
{
    if (deinterlace)
    {
        _deinterlace = [deinterlace copy];
    }
    else
    {
        _deinterlace = @"off";
    }
    [self postChangedNotification];
}

- (void)setDeinterlaceCustomString:(NSString *)deinterlaceCustomString
{
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

- (void)setDecomb:(NSString *)decomb
{
    if (decomb)
    {
        _decomb = [decomb copy];
    }
    else
    {
        _decomb = @"off";
    }
    [self postChangedNotification];
}

- (void)setDecombCustomString:(NSString *)decombCustomString
{
    if (decombCustomString)
    {
        _decombCustomString = [decombCustomString copy];
    }
    else
    {
        _decombCustomString = @"";
    }

    [self postChangedNotification];
}

- (void)setDenoise:(NSString *)denoise
{
    if (denoise)
    {
        _denoise = [denoise copy];
    }
    else
    {
        _denoise = @"";
    }

    [self postChangedNotification];
}

- (void)setDenoisePreset:(NSString *)denoisePreset
{
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

- (void)setDeblock:(int)deblock
{
    _deblock = deblock;
    [self postChangedNotification];
}

- (void)setGrayscale:(BOOL)grayscale
{
    _grayscale = grayscale;
    [self postChangedNotification];
}

- (void)setUseDecomb:(BOOL)useDecomb
{
    _useDecomb = useDecomb;
    [self postChangedNotification];
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"summary"])
    {
        retval = [NSSet setWithObjects:@"detelecine", @"detelecineCustomString", @"useDecomb", @"deinterlace", @"deinterlaceCustomString", @"decomb", @"decombCustomString", @"denoise", @"denoisePreset", @"denoiseTune", @"denoiseCustomString", @"deblock", @"grayscale", nil];
    }
    if ([key isEqualToString:@"customDetelecineSelected"] ||
        [key isEqualToString:@"customDecombSelected"] ||
        [key isEqualToString:@"customDeinterlaceSelected"])
    {
        retval = [NSSet setWithObjects:@"detelecine", @"decomb", @"deinterlace", @"useDecomb", nil];
    }
    if ([key isEqualToString:@"denoiseTunesAvailable"] ||
        [key isEqualToString:@"customDenoiseSelected"])
    {
        retval = [NSSet setWithObjects:@"denoise", @"denoisePreset", nil];
    }
    if ([key isEqualToString:@"denoiseEnabled"])
    {
        retval = [NSSet setWithObject:@"denoise"];
    }
    if ([key isEqualToString:@"deblockSummary"])
    {
        retval = [NSSet setWithObject:@"deblock"];
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

        copy->_deinterlace = [_deinterlace copy];
        copy->_deinterlaceCustomString = [_deinterlaceCustomString copy];

        copy->_decomb = [_decomb copy];
        copy->_decombCustomString = [_decombCustomString copy];

        copy->_denoise = [_denoise copy];
        copy->_denoisePreset = [_denoisePreset copy];
        copy->_denoiseTune = [_denoiseTune copy];
        copy->_denoiseCustomString = [_denoiseCustomString copy];

        copy->_deblock = _deblock;
        copy->_grayscale = _grayscale;

        copy->_useDecomb = _useDecomb;
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
    [coder encodeInt:1 forKey:@"HBFiltersVersion"];

    encodeObject(_detelecine);
    encodeObject(_detelecineCustomString);

    encodeObject(_deinterlace);
    encodeObject(_deinterlaceCustomString);

    encodeObject(_decomb);
    encodeObject(_decombCustomString);

    encodeObject(_denoise);
    encodeObject(_denoisePreset);
    encodeObject(_denoiseTune);
    encodeObject(_denoiseCustomString);

    encodeInt(_deblock);
    encodeBool(_grayscale);

    encodeBool(_useDecomb);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeObject(_detelecine, NSString);
    decodeObject(_detelecineCustomString, NSString);

    decodeObject(_deinterlace, NSString);
    decodeObject(_deinterlaceCustomString, NSString);

    decodeObject(_decomb, NSString);
    decodeObject(_decombCustomString, NSString);

    decodeObject(_denoise, NSString);
    decodeObject(_denoisePreset, NSString);
    decodeObject(_denoiseTune, NSString);
    decodeObject(_denoiseCustomString, NSString);

    decodeInt(_deblock);
    decodeBool(_grayscale);

    decodeBool(_useDecomb);

    _notificationsEnabled = YES;

    return self;
}

#pragma mark - Presets and queue

- (void)writeToPreset:(NSMutableDictionary *)preset
{
    preset[@"PictureDecombDeinterlace"] = @(self.useDecomb);

    preset[@"PictureDeinterlace"] = self.deinterlace;
    preset[@"PictureDeinterlaceCustom"] = self.deinterlaceCustomString;

    preset[@"PictureDecomb"] = self.decomb;
    preset[@"PictureDecombCustom"] = self.decombCustomString;

    preset[@"PictureDetelecine"] = self.detelecine;
    preset[@"PictureDetelecineCustom"] = self.detelecineCustomString;

    preset[@"PictureDenoiseFilter"] = self.denoise;
    preset[@"PictureDenoisePreset"] = self.denoisePreset;
    preset[@"PictureDenoiseTune"] = self.denoiseTune;
    preset[@"PictureDenoiseCustom"] = self.denoiseCustomString;

    preset[@"PictureDeblock"] = @(self.deblock);
    preset[@"VideoGrayScale"] = @(self.grayscale);
}

- (void)applyPreset:(NSDictionary *)preset
{
    self.notificationsEnabled = NO;

    // If the preset has an objectForKey:@"UsesPictureFilters", and handle the filters here
    if ([preset[@"UsesPictureFilters"] boolValue])
    {
        // We only allow *either* Decomb or Deinterlace. So check for the PictureDecombDeinterlace key.
        self.useDecomb = [preset[@"PictureDecombDeinterlace"] boolValue];

        self.decomb = preset[@"PictureDecomb"];
        self.decombCustomString = preset[@"PictureDecombCustom"];

        self.deinterlace = preset[@"PictureDeinterlace"];
        self.deinterlaceCustomString = preset[@"PictureDeinterlaceCustom"];

        //Detelecine
        self.detelecine = preset[@"PictureDetelecine"];
        self.detelecineCustomString = preset[@"PictureDetelecineCustom"];

        // Denoise
        self.denoise = preset[@"PictureDenoiseFilter"];
        self.denoisePreset = preset[@"PictureDenoisePreset"];
        self.denoiseTune = preset[@"PictureDenoiseTune"];

        self.denoiseCustomString = preset[@"PictureDenoiseCustom"];

        // Deblock
        if ([preset[@"PictureDeblock"] intValue] == 1)
        {
            // if its a one, then its the old on/off deblock, set on to 5
            self.deblock = 5;
        }
        else
        {
            // use the settings intValue
            self.deblock = [preset[@"PictureDeblock"] intValue];
        }

        self.grayscale = [preset[@"VideoGrayScale"] boolValue];
    }

    self.notificationsEnabled = YES;
}

@end
