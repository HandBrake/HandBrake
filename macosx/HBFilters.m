/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"
#import "HBCodingUtilities.h"
#import "HBLocalizationUtilities.h"
#import "NSDictionary+HBAdditions.h"
#import "HBMutablePreset.h"

#include "handbrake/handbrake.h"

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
        _chromaSmooth = @"off";
        _chromaSmoothTune = @"none";
        _chromaSmoothCustomString = @"";
        _sharpen = @"off";
        _sharpenCustomString = @"";
        _sharpenPreset = @"medium";
        _sharpenTune = @"none";
        _deblock = @"off";
        _deblockTune = @"none";
        _deblockCustomString = @"";
        _colorspace = @"off";
        _colorspaceCustomString = @"";

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

#define LAPSHARP_DEFAULT_PRESET      "medium"
#define UNSHARP_DEFAULT_PRESET       "medium"
#define CHROMA_SMOOTH_DEFAULT_PRESET "medium"
#define NLMEANS_DEFAULT_PRESET       "medium"
#define YADIF_DEFAULT_PRESET         "default"
#define DECOMB_DEFAULT_PRESET        "default"
#define BWDIF_DEFAULT_PRESET         "default"
#define DETELECINE_DEFAULT_PRESET    "default"
#define COMB_DETECT_DEFAULT_PRESET   "default"
#define HQDN3D_DEFAULT_PRESET        "medium"
#define DEBLOCK_DEFAULT_PRESET       "medium"
#define COLORSPACE_DEFAULT_PRESET    "bt709"

- (NSString *)filterKeysDescription:(int)filter_id
{
    char **keys = hb_filter_get_keys(filter_id);
    char  *colon = "";

    NSMutableString *result = [[NSMutableString alloc] init];
    [result appendString:HBKitLocalizedString(@"Syntax: ", @"HBFilters -> invalid filter custom settings error recovery suggestion")];

    for (int ii = 0; keys[ii] != NULL; ii++)
    {
        int c = tolower(keys[ii][0]);
        [result appendFormat:@"%s%s=%c", colon, keys[ii], c];
        colon = ":";
    }
    hb_str_vfree(keys);

    [result appendString:@"\n\n"];
    [result appendString:HBKitLocalizedString(@"Default: ", @"HBFilters -> invalid filter custom settings error recovery suggestion")];

    const char *preset = "default";
    switch (filter_id)
    {
        case HB_FILTER_UNSHARP:
            preset = UNSHARP_DEFAULT_PRESET;
            break;
        case HB_FILTER_LAPSHARP:
            preset = LAPSHARP_DEFAULT_PRESET;
            break;
        case HB_FILTER_CHROMA_SMOOTH:
            preset = CHROMA_SMOOTH_DEFAULT_PRESET;
            break;
        case HB_FILTER_NLMEANS:
            preset = NLMEANS_DEFAULT_PRESET;
            break;
        case HB_FILTER_YADIF:
            preset = YADIF_DEFAULT_PRESET;
            break;
        case HB_FILTER_BWDIF:
            preset = BWDIF_DEFAULT_PRESET;
            break;
        case HB_FILTER_DECOMB:
            preset = DECOMB_DEFAULT_PRESET;
            break;
        case HB_FILTER_DETELECINE:
            preset = DETELECINE_DEFAULT_PRESET;
            break;
        case HB_FILTER_HQDN3D:
            preset = HQDN3D_DEFAULT_PRESET;
            break;
        case HB_FILTER_COMB_DETECT:
            preset = COMB_DETECT_DEFAULT_PRESET;
            break;
        case HB_FILTER_DEBLOCK:
            preset = DEBLOCK_DEFAULT_PRESET;
            break;
        default:
            break;
    }
    switch (filter_id)
    {
        case HB_FILTER_YADIF:
        case HB_FILTER_BWDIF:
        case HB_FILTER_NLMEANS:
        case HB_FILTER_CHROMA_SMOOTH:
        case HB_FILTER_COLORSPACE:
        case HB_FILTER_UNSHARP:
        case HB_FILTER_LAPSHARP:
        case HB_FILTER_DECOMB:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_HQDN3D:
        case HB_FILTER_COMB_DETECT:
        case HB_FILTER_DEBLOCK:
        {
            hb_dict_t *settings;
            settings = hb_generate_filter_settings(filter_id, preset, NULL, NULL);
            char *str = hb_filter_settings_string(filter_id, settings);
            hb_value_free(&settings);

            char **split = hb_str_vsplit(str, ':');
            colon = "";

            for (int ii = 0; split[ii] != NULL; ii++)
            {
                [result appendFormat:@"%s%s", colon, split[ii]];
                colon = ":";
            }
            hb_str_vfree(split);
            free(str);
        } break;
        default:
            break;
    }

    return result;
}

#pragma mark - Detelecine

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

- (BOOL)validateDetelecineCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_DETELECINE;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid custom detelecine settings.",
                                                                                        @"HBFilters -> invalid detelecine custom string description"),
                                           NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - Comb Detect

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

- (BOOL)validateCombDetectionCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_COMB_DETECT;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid custom comb detect settings.",
                                                                                        @"HBFilters -> invalid comb detect custom string description"),
                                           NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - Deinterlace

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
        filter_id = HB_FILTER_YADIF;
    }
    else if ([self.deinterlace isEqualToString:@"bwdif"])
    {
        filter_id = HB_FILTER_BWDIF;
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

- (BOOL)validateDeinterlaceCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_DECOMB;
        if ([self.deinterlace isEqualToString:@"deinterlace"])
        {
            filter_id = HB_FILTER_YADIF;
        }
        else if ([self.deinterlace isEqualToString:@"bwdif"])
        {
            filter_id = HB_FILTER_BWDIF;
        }
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                if (filter_id == HB_FILTER_YADIF)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid Yadif custom settings.",
                                                                                            @"HBFilters -> invalid Yadif custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
                else if (filter_id == HB_FILTER_DECOMB)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid Decomb custom settings.",
                                                                                            @"HBFilters -> invalid Decomb custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
                else if (filter_id == HB_FILTER_BWDIF)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid Bwdif custom settings.",
                                                                                            @"HBFilters -> invalid Bwdif custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
            }
        }
    }

    return retval;
}

#pragma mark - Denoise

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

- (BOOL)validateDenoiseCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_HQDN3D;
        if ([self.denoise isEqualToString:@"nlmeans"])
        {
            filter_id = HB_FILTER_NLMEANS;
        }
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                if (filter_id == HB_FILTER_HQDN3D)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid custom HQDN3D settings",
                                                                                            @"HBFilters -> invalid denoise custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
                else if (filter_id == HB_FILTER_NLMEANS)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid custom NLMeans settings",
                                                                                            @"HBFilters -> invalid denoise custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
            }
        }
    }

    return retval;
}

#pragma mark - Chroma Smooth

- (void)setChromaSmooth:(NSString *)chromaSmooth
{
    if (![chromaSmooth isEqualToString:_chromaSmooth])
    {
        [[self.undo prepareWithInvocationTarget:self] setChromaSmooth:_chromaSmooth];
    }
    if (chromaSmooth)
    {
        _chromaSmooth = [chromaSmooth copy];
    }
    else
    {
        _chromaSmooth = @"off";
    }

    [self postChangedNotification];
}

- (void)setChromaSmoothTune:(NSString *)chromaSmoothTune
{
    if (![chromaSmoothTune isEqualToString:_chromaSmoothTune])
    {
        [[self.undo prepareWithInvocationTarget:self] setChromaSmoothTune:_chromaSmoothTune];
    }
    if (chromaSmoothTune)
    {
        _chromaSmoothTune = [chromaSmoothTune copy];
    }
    else
    {
        _chromaSmoothTune = @"none";
    }

    [self postChangedNotification];
}

- (void)setChromaSmoothCustomString:(NSString *)chromaSmoothCustomString
{
    if (![chromaSmoothCustomString isEqualToString:_chromaSmoothCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setChromaSmoothCustomString:_chromaSmoothCustomString];
    }
    if (chromaSmoothCustomString)
    {
        _chromaSmoothCustomString = [chromaSmoothCustomString copy];
    }
    else
    {
        _chromaSmoothCustomString = @"";
    }

    [self postChangedNotification];
}

- (BOOL)validateChromaSmoothCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_CHROMA_SMOOTH;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid chroma smooth custom settings.",
                                                                                               @"HBFilters -> invalid chroma smooth custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - Sharpen

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

- (BOOL)validateSharpenCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_UNSHARP;
        if ([self.sharpen isEqualToString:@"lapsharp"])
        {
            filter_id = HB_FILTER_LAPSHARP;
        }
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                if (filter_id == HB_FILTER_UNSHARP)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid unsharp custom settings.",
                                                                                            @"HBFilters -> invalid unsharp custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
                else if (filter_id == HB_FILTER_LAPSHARP)
                {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid lapsharp custom settings.",
                                                                                            @"HBFilters -> invalid lapsharp custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
                }
            }
        }
    }

    return retval;
}

#pragma mark - Deblock

- (void)setDeblock:(NSString *)deblock
{
    if (![deblock isEqualToString:_deblock])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeblock:_deblock];
    }
    if (deblock)
    {
        _deblock = [deblock copy];
    }
    else
    {
        _deblock = @"off";
    }

    [self postChangedNotification];
}

- (void)setDeblockTune:(NSString *)deblockTune
{
    if (![deblockTune isEqualToString:_deblockTune])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeblockTune:_deblockTune];
    }
    if (deblockTune)
    {
        _deblockTune = [deblockTune copy];
    }
    else
    {
        _deblockTune = @"none";
    }

    [self postChangedNotification];
}

- (void)setDeblockCustomString:(NSString *)deblockCustomString
{
    if (![deblockCustomString isEqualToString:_deblockCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setDeblockCustomString:_deblockCustomString];
    }
    if (deblockCustomString)
    {
        _deblockCustomString = [deblockCustomString copy];
    }
    else
    {
        _deblockCustomString = @"";
    }

    [self postChangedNotification];
}

- (BOOL)validateDeblockCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_DEBLOCK;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid deblock custom settings.",
                                                                                               @"HBFilters -> invalid deblock custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - Grayscale

- (void)setGrayscale:(BOOL)grayscale
{
    if (grayscale != _grayscale)
    {
        [[self.undo prepareWithInvocationTarget:self] setGrayscale:_grayscale];
    }
    _grayscale = grayscale;
    [self postChangedNotification];
}

#pragma mark - Colorspace

- (void)setColorspace:(NSString *)colorspace
{
    if (![colorspace isEqualToString:_colorspace])
    {
        [[self.undo prepareWithInvocationTarget:self] setColorspace:_colorspace];
    }
    if (colorspace)
    {
        _colorspace = [colorspace copy];
    }
    else
    {
        _colorspace = @"off";
    }

    [self postChangedNotification];
}

- (void)setColorspaceCustomString:(NSString *)colorspaceCustomString
{
    if (![colorspaceCustomString isEqualToString:_colorspaceCustomString])
    {
        [[self.undo prepareWithInvocationTarget:self] setColorspaceCustomString:_colorspaceCustomString];
    }
    if (colorspaceCustomString)
    {
        _colorspaceCustomString = [colorspaceCustomString copy];
    }
    else
    {
        _colorspaceCustomString = @"";
    }

    [self postChangedNotification];
}

- (BOOL)validateColorspaceCustomString:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *customValue = *ioValue;

        int filter_id = HB_FILTER_COLORSPACE;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             "custom",
                                                             NULL,
                                                             customValue.UTF8String);

        if (filter_dict == NULL)
        {
            retval = NO;
            if (outError)
            {
                    NSDictionary *userInfo = @{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid colorspace custom settings.",
                                                                                               @"HBFilters -> invalid chroma smooth custom string description"),
                                               NSLocalizedRecoverySuggestionErrorKey: [self filterKeysDescription:filter_id]};
                    *outError = [NSError errorWithDomain:@"HBFilterError" code:0 userInfo:userInfo];
            }
        }
    }

    return retval;
}

#pragma mark - KVO

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"summary"])
    {
        retval = [NSSet setWithObjects:@"detelecine", @"detelecineCustomString", @"deinterlace", @"deinterlacePreset", @"deinterlaceCustomString", @"denoise", @"denoisePreset", @"denoiseTune", @"denoiseCustomString", @"deblock", @"deblockTune", @"deblockCustomString", @"grayscale", @"sharpen", @"sharpenPreset", @"sharpenTune", @"sharpenCustomString", @"chromaSmooth", @"chromaSmoothTune", @"chromaSmoothCustomString", @"colorspace", @"colorspaceCustomString", nil];
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
    else if ([key isEqualToString:@"chromaSmoothEnabled"] ||
             [key isEqualToString:@"customChromaSmoothSelected"])
    {
        retval = [NSSet setWithObject:@"chromaSmooth"];
    }
    else if ([key isEqualToString:@"sharpenTunesAvailable"] ||
             [key isEqualToString:@"customSharpenSelected"])
    {
        retval = [NSSet setWithObjects:@"sharpen", @"sharpenPreset", nil];
    }
    else if ([key isEqualToString:@"sharpenEnabled"] ||
             [key isEqualToString:@"sharpenPresets"] ||
             [key isEqualToString:@"sharpenTunes"])
    {
        retval = [NSSet setWithObject:@"sharpen"];
    }
    else if ([key isEqualToString:@"deblockTunesAvailable"] ||
             [key isEqualToString:@"customDeblockSelected"])
    {
        retval = [NSSet setWithObject:@"deblock"];
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
    else if ([key isEqualToString:@"customColorspaceSelected"])
    {
        retval = [NSSet setWithObject:@"colorspace"];
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

        copy->_chromaSmooth = [_chromaSmooth copy];
        copy->_chromaSmoothTune = [_chromaSmoothTune copy];
        copy->_chromaSmoothCustomString = [_chromaSmoothCustomString copy];

        copy->_sharpen = [_sharpen copy];
        copy->_sharpenPreset = [_sharpenPreset copy];
        copy->_sharpenTune = [_sharpenTune copy];
        copy->_sharpenCustomString = [_sharpenCustomString copy];

        copy->_deblock = [_deblock copy];
        copy->_deblockTune = [_deblockTune copy];
        copy->_deblockCustomString = [_deblockCustomString copy];

        copy->_grayscale = _grayscale;

        copy->_colorspace = [_colorspace copy];
        copy->_colorspaceCustomString = [_colorspaceCustomString copy];
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

    encodeObject(_chromaSmooth);
    encodeObject(_chromaSmoothTune);
    encodeObject(_chromaSmoothCustomString);

    encodeObject(_sharpen);
    encodeObject(_sharpenPreset);
    encodeObject(_sharpenTune);
    encodeObject(_sharpenCustomString);

    encodeObject(_deblock);
    encodeObject(_deblockTune);
    encodeObject(_deblockCustomString);

    encodeBool(_grayscale);

    encodeObject(_colorspace);
    encodeObject(_colorspaceCustomString);
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

    decodeObjectOrFail(_chromaSmooth, NSString);
    decodeObjectOrFail(_chromaSmoothTune, NSString);
    decodeObjectOrFail(_chromaSmoothCustomString, NSString);

    decodeObjectOrFail(_sharpen, NSString);
    decodeObjectOrFail(_sharpenPreset, NSString);
    decodeObjectOrFail(_sharpenTune, NSString);
    decodeObjectOrFail(_sharpenCustomString, NSString);

    decodeObjectOrFail(_deblock, NSString);
    decodeObjectOrFail(_deblockTune, NSString);
    decodeObjectOrFail(_deblockCustomString, NSString);

    decodeBool(_grayscale);

    decodeObjectOrFail(_colorspace, NSString);
    decodeObjectOrFail(_colorspaceCustomString, NSString);

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

    preset[@"PictureChromaSmoothPreset"] = self.chromaSmooth;
    preset[@"PictureChromaSmoothTune"] = self.chromaSmoothTune;
    preset[@"PictureChromaSmoothCustom"] = self.chromaSmoothCustomString;

    preset[@"PictureSharpenFilter"] = self.sharpen;
    preset[@"PictureSharpenPreset"] = self.sharpenPreset;
    preset[@"PictureSharpenTune"] = self.sharpenTune;
    preset[@"PictureSharpenCustom"] = self.sharpenCustomString;

    preset[@"PictureDeblockPreset"] = self.deblock;
    preset[@"PictureDeblockTune"] = self.deblockTune;
    preset[@"PictureDeblockCustom"] = self.deblockCustomString;

    preset[@"VideoGrayScale"] = @(self.grayscale);

    preset[@"PictureColorspacePreset"] = self.colorspace;
    preset[@"PictureColorspaceCustom"] = self.colorspaceCustomString;
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    self.notificationsEnabled = NO;

    // If the preset has "UsesPictureFilters", handle the filters here
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

        // Chroma Smooth
        self.chromaSmooth = preset[@"PictureChromaSmoothPreset"];
        self.chromaSmoothTune = preset[@"PictureChromaSmoothTune"];
        self.chromaSmoothCustomString = preset[@"PictureChromaSmoothCustom"];

        // Sharpen
        self.sharpen = preset[@"PictureSharpenFilter"];
        self.sharpenPreset = preset[@"PictureSharpenPreset"];
        self.sharpenTune = preset[@"PictureSharpenTune"];
        self.sharpenCustomString = preset[@"PictureSharpenCustom"];

        // Deblock
        self.deblock = preset[@"PictureDeblockPreset"];
        self.deblockTune = preset[@"PictureDeblockTune"];
        self.deblockCustomString = preset[@"PictureDeblockCustom"];

        self.grayscale = [preset[@"VideoGrayScale"] boolValue];

        // Colorspace
        self.colorspace = preset[@"PictureColorspacePreset"];
        self.colorspaceCustomString = preset[@"PictureColorspaceCustom"];
    }

    self.notificationsEnabled = YES;
    [self postChangedNotification];
}

@end
