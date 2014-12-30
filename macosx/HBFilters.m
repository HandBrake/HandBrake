/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"
#import "NSCodingMacro.h"

NSString * const HBFiltersChangedNotification = @"HBFiltersChangedNotification";

NSDictionary *_HandBrake_denoiseTypesDict;
NSDictionary *_HandBrake_denoisePresetsDict;
NSDictionary *_HandBrake_nlmeansTunesDict;

@interface HBFilters ()

@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@end

@implementation HBFilters

+ (void)initialize
{
    if (self == [HBFilters class])
    {
        _HandBrake_denoiseTypesDict = [@{NSLocalizedString(@"Off", nil):      @"off",
                               NSLocalizedString(@"NLMeans", nil):  @"nlmeans",
                               NSLocalizedString(@"HQDN3D", nil):   @"hqdn3d"} retain];

        _HandBrake_denoisePresetsDict = [@{NSLocalizedString(@"Custom", nil):     @"none",
                                 NSLocalizedString(@"Ultralight", nil): @"ultralight",
                                 NSLocalizedString(@"Light", nil):      @"light",
                                 NSLocalizedString(@"Medium", nil) :    @"medium",
                                 NSLocalizedString(@"Strong", nil) :    @"strong"} retain];

        _HandBrake_nlmeansTunesDict = [@{NSLocalizedString(@"None", nil):         @"none",
                               NSLocalizedString(@"Film", nil):         @"film",
                               NSLocalizedString(@"Grain", nil):        @"grain",
                               NSLocalizedString(@"High Motion", nil):  @"highmotion",
                               NSLocalizedString(@"Animation", nil) :   @"animation"} retain];
    }
}

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _detelecineCustomString = @"";
        _deinterlaceCustomString = @"";
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

- (void)setDetelecine:(NSInteger)detelecine
{
    _detelecine = detelecine;
    [self postChangedNotification];
}

// Override setter to avoid nil values.
- (void)setDetelecineCustomString:(NSString *)detelecineCustomString
{
    [_detelecineCustomString autorelease];

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

- (void)setDeinterlace:(NSInteger)deinterlace
{
    _deinterlace = deinterlace;
    [self postChangedNotification];
}

- (void)setDeinterlaceCustomString:(NSString *)deinterlaceCustomString
{
    [_deinterlaceCustomString autorelease];

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

- (void)setDecomb:(NSInteger)decomb
{
    _decomb = decomb;
    [self postChangedNotification];
}

- (void)setDecombCustomString:(NSString *)decombCustomString
{
    [_decombCustomString autorelease];

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
    [_denoise autorelease];
    _denoise = [denoise copy];

    [self postChangedNotification];
}

- (void)setDenoisePreset:(NSString *)denoisePreset
{
    [_denoisePreset autorelease];
    _denoisePreset = [denoisePreset copy];

    [self postChangedNotification];
}


- (void)setDenoiseTune:(NSString *)denoiseTune
{
    [_denoiseTune autorelease];
    _denoiseTune = [denoiseTune copy];

    [self postChangedNotification];
}

- (void)setDenoiseCustomString:(NSString *)denoiseCustomString
{
    [_denoiseCustomString autorelease];

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

- (void)setDeblock:(NSInteger)deblock
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

    return retval;
}


#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBFilters *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_detelecine = _detelecine;
        copy->_detelecineCustomString = [_detelecineCustomString copy];

        copy->_deinterlace = _deinterlace;
        copy->_deinterlaceCustomString = [_deinterlaceCustomString copy];

        copy->_decomb = _decomb;
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

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBFiltersVersion"];

    encodeInteger(_detelecine);
    encodeObject(_detelecineCustomString);

    encodeInteger(_deinterlace);
    encodeObject(_deinterlaceCustomString);

    encodeInteger(_decomb);
    encodeObject(_decombCustomString);

    encodeObject(_denoise);
    encodeObject(_denoisePreset);
    encodeObject(_denoiseTune);
    encodeObject(_denoiseCustomString);

    encodeInteger(_deblock);
    encodeBool(_grayscale);

    encodeBool(_useDecomb);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_detelecine);
    decodeObject(_detelecineCustomString);

    decodeInteger(_deinterlace);
    decodeObject(_deinterlaceCustomString);

    decodeInteger(_decomb);
    decodeObject(_decombCustomString);

    decodeObject(_denoise);
    decodeObject(_denoisePreset);
    decodeObject(_denoiseTune);
    decodeObject(_denoiseCustomString);

    decodeInteger(_deblock);
    decodeBool(_grayscale);

    decodeBool(_useDecomb);

    _notificationsEnabled = YES;

    return self;
}

#pragma mark - Presets and queue

- (void)writeToPreset:(NSMutableDictionary *)preset
{
    preset[@"PictureDecombDeinterlace"] = @(self.useDecomb);

    preset[@"PictureDeinterlace"] = @(self.deinterlace);
    preset[@"PictureDeinterlaceCustom"] = self.deinterlaceCustomString;

    preset[@"PictureDecomb"] = @(self.decomb);
    preset[@"PictureDecombCustom"] = self.decombCustomString;

    preset[@"PictureDetelecine"] = @(self.detelecine);
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

    /* If the preset has an objectForKey:@"UsesPictureFilters", and handle the filters here */
    if (preset[@"UsesPictureFilters"] && [preset[@"UsesPictureFilters"]  intValue] > 0)
    {
        /* We only allow *either* Decomb or Deinterlace. So check for the PictureDecombDeinterlace key. */
        self.useDecomb = 1;
        self.decomb = 0;
        self.deinterlace = 0;
        if ([preset[@"PictureDecombDeinterlace"] intValue] == 1)
        {
            /* we are using decomb */
            /* Decomb */
            if ([preset[@"PictureDecomb"] intValue] > 0)
            {
                self.decomb = [preset[@"PictureDecomb"] intValue];

                /* if we are using "Custom" in the decomb setting, also set the custom string*/
                if ([preset[@"PictureDecomb"] intValue] == 1)
                {
                    self.decombCustomString = preset[@"PictureDecombCustom"];
                }
            }
        }
        else
        {
            /* We are using Deinterlace */
            /* Deinterlace */
            if ([preset[@"PictureDeinterlace"] intValue] > 0)
            {
                self.useDecomb = 0;
                self.deinterlace = [preset[@"PictureDeinterlace"] intValue];
                /* if we are using "Custom" in the deinterlace setting, also set the custom string*/
                if ([preset[@"PictureDeinterlace"] intValue] == 1)
                {
                    self.deinterlaceCustomString = preset[@"PictureDeinterlaceCustom"];
                }
            }
        }

        /* Detelecine */
        if ([preset[@"PictureDetelecine"] intValue] > 0)
        {
            self.detelecine = [preset[@"PictureDetelecine"] intValue];
            /* if we are using "Custom" in the detelecine setting, also set the custom string*/
            if ([preset[@"PictureDetelecine"] intValue] == 1)
            {
                self.detelecineCustomString = preset[@"PictureDetelecineCustom"];
            }
        }
        else
        {
            self.detelecine = 0;
        }

        /* Denoise */
        if (preset[@"PictureDenoise"])
        {
            // Old preset denoise format, try to map it to the new one
            if ([preset[@"PictureDenoise"] intValue] > 0)
            {
                self.denoise = @"hqdn3d";
                /* if we are using "Custom" in the denoise setting, also set the custom string*/
                if ([preset[@"PictureDenoise"] intValue] == 1)
                {
                    self.denoisePreset = @"custom";
                    self.denoiseCustomString = preset[@"PictureDenoiseCustom"];
                }
                switch ([preset[@"PictureDenoise"] intValue]) {
                    case 2:
                        self.denoisePreset = @"light";
                        break;
                    case 3:
                        self.denoisePreset = @"medium";
                        break;
                    case 4:
                        self.denoisePreset = @"strong";
                        break;
                    default:
                        self.denoisePreset = @"medium";
                        break;
                }
            }
            else
            {
                self.denoise = @"off";
            }
        }
        else
        {
            // New format, read the values directly
            self.denoise = preset[@"PictureDenoiseFilter"];
            self.denoisePreset = preset[@"PictureDenoisePreset"];
            self.denoiseTune = preset[@"PictureDenoiseTune"];
            self.denoiseCustomString = preset[@"PictureDenoiseCustom"];
        }

        /* Deblock */
        if ([preset[@"PictureDeblock"] intValue] == 1)
        {
            /* if its a one, then its the old on/off deblock, set on to 5*/
            self.deblock = 5;
        }
        else
        {
            /* use the settings intValue */
            self.deblock = [preset[@"PictureDeblock"] intValue];
        }

        self.grayscale = [preset[@"VideoGrayScale"] intValue];
    }

    self.notificationsEnabled = YES;
}

@end
