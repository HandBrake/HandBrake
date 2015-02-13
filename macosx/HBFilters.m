/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"

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

static NSDictionary *_denoiseTypesDict;
static NSDictionary *_denoisePresetsDict;
static NSDictionary *_nlmeansTunesDict;

@implementation HBFilters

+ (void)initialize
{
    if (self == [HBFilters class]) {
        _denoiseTypesDict = [@{NSLocalizedString(@"Off", nil):      @"off",
                               NSLocalizedString(@"NLMeans", nil):  @"nlmeans",
                               NSLocalizedString(@"HQDN3D", nil):   @"hqdn3d"} retain];

        _denoisePresetsDict = [@{NSLocalizedString(@"Custom", nil):     @"none",
                                 NSLocalizedString(@"Ultralight", nil): @"ultralight",
                                 NSLocalizedString(@"Light", nil):      @"light",
                                 NSLocalizedString(@"Medium", nil) :    @"medium",
                                 NSLocalizedString(@"Strong", nil) :    @"strong"} retain];

        _nlmeansTunesDict = [@{NSLocalizedString(@"None", nil):         @"none",
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
    }
    return self;
}

- (void)prepareFiltersForPreset:(NSMutableDictionary *)preset
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

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
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
            if ([[_denoiseTypesDict allValues] containsObject:preset[@"PictureDenoiseFilter"]])
            {
                self.denoise = preset[@"PictureDenoiseFilter"];
            }
            else
            {
                self.denoise = [[_denoiseTypesDict allValues] firstObject];
            }

            if ([[_denoisePresetsDict allValues] containsObject:preset[@"PictureDenoisePreset"]])
            {
                self.denoisePreset = preset[@"PictureDenoisePreset"];
            }
            else
            {
                self.denoisePreset = [[_denoisePresetsDict allValues] firstObject];
            }

            if ([[_nlmeansTunesDict allValues] containsObject:preset[@"PictureDenoiseTune"]])
            {
                self.denoiseTune = preset[@"PictureDenoiseTune"];
            }
            else
            {
                self.denoiseTune = [[_nlmeansTunesDict allKeys] firstObject];
            }

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
        [summary appendFormat:@" - Denoise (%@", [[_denoiseTypesDict allKeysForObject:self.denoise] firstObject]];
        if (![self.denoisePreset isEqualToString:@"none"])
        {
            [summary appendFormat:@", %@", [[_denoisePresetsDict allKeysForObject:self.denoisePreset] firstObject]];

            if ([self.denoise isEqualToString:@"nlmeans"])
            {
                [summary appendFormat:@", %@", [[_nlmeansTunesDict allKeysForObject:self.denoiseTune] firstObject]];
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
}

#pragma mark - Valid values

+ (NSDictionary *)denoisePresetDict
{
    return _denoisePresetsDict;
}

+ (NSDictionary *)nlmeansTunesDict
{
    return _nlmeansTunesDict;
}

+ (NSDictionary *)denoiseTypesDict
{
    return _denoiseTypesDict;
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

@end
