/*  HBVideo+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideo+UIAdditions.h"
#import "HBJob+Private.h"
#import "HBLocalizationUtilities.h"

#include "handbrake/handbrake.h"

@implementation HBVideo (UIAdditions)

@dynamic qualityMaxValue;
@dynamic qualityMinValue;
@dynamic levels;
@dynamic tunes;
@dynamic presets;
@dynamic profiles;

#pragma mark - Possible values

+ (NSSet<NSString *> *)keyPathsForValuesAffectingEncoders
{
    return [NSSet setWithObjects:@"job.container", nil];
}

- (NSArray *)encoders
{
    NSMutableArray *encoders = [NSMutableArray array];
    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & self.job.container)
        {
            [encoders addObject:@(video_encoder->name)];
        }
    }
    return [encoders copy];
}

- (NSArray *)frameRates
{
    NSMutableArray *framerates = [NSMutableArray array];

    [framerates addObject:HBKitLocalizedString(@"Same as source", @"HBVideo -> frame rates display name")];

    for (const hb_rate_t *video_framerate = hb_video_framerate_get_next(NULL);
         video_framerate != NULL;
         video_framerate  = hb_video_framerate_get_next(video_framerate))
    {
        NSString *itemTitle;
        if (!strcmp(video_framerate->name, "23.976"))
        {
            itemTitle = @"23.976 (NTSC Film)";
        }
        else if (!strcmp(video_framerate->name, "25"))
        {
            itemTitle = @"25 (PAL Film/Video)";
        }
        else if (!strcmp(video_framerate->name, "29.97"))
        {
            itemTitle = @"29.97 (NTSC Video)";
        }
        else
        {
            itemTitle = @(video_framerate->name);
        }

        [framerates addObject:itemTitle];
    }
    return [framerates copy];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingFastDecodeSupported
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (BOOL)fastDecodeSupported
{
    if (!(self.encoder & (HB_VCODEC_X264_MASK | HB_VCODEC_SVT_AV1_MASK)))
    {
        return NO;
    }

    const char * const *tunes = hb_video_encoder_get_tunes(self.encoder);

    for (int i = 0; tunes != NULL && tunes[i] != NULL; i++)
    {
        if (!strcasecmp(tunes[i], "fastdecode"))
        {
            return YES;
        }
    }
    return NO;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingTurboMultiPassSupported
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (BOOL)turboMultiPassSupported
{
    return ((self.encoder & HB_VCODEC_X264_MASK) ||
            (self.encoder & HB_VCODEC_X265_MASK));
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMultiPassSupported
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (BOOL)multiPassSupported
{
    return hb_video_multipass_is_supported(self.encoder);
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingConstantQualityLabel
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (NSString *)constantQualityLabel
{
    return @(hb_video_quality_get_name(self.encoder));
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingIsConstantQualitySupported
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (BOOL)isConstantQualitySupported
{
    return hb_video_quality_is_supported(self.encoder);
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingUnparseOptions
{
    return [NSSet setWithObjects:@"encoder", @"preset", @"tune", @"profile", @"level",
            @"videoOptionExtra", @"fastDecode", @"job.picture.width", @"job.picture.height", nil];
}

/**
 *  This is called every time a x264 widget in the video tab is changed to
 *  display the expanded options in a text field via outlet fDisplayX264PresetsUnparseTextField
 */
- (NSString *)unparseOptions
{
    if (!(self.encoder & HB_VCODEC_X264_MASK))
    {
        return @"";
    }

    /* API reference:
     *
     * char * hb_x264_param_unparse(const char *x264_preset,
     *                              const char *x264_tune,
     *                              const char *x264_encopts,
     *                              const char *h264_profile,
     *                              const char *h264_level,
     *                              int width, int height);
     */
    NSString   *tmpString;
    const char *x264_preset   = [self.preset UTF8String];
    const char *x264_tune     = NULL;
    const char *advanced_opts = NULL;
    const char *h264_profile  = NULL;
    const char *h264_level    = NULL;

    // prepare the tune, advanced options, profile and level
    if ([tmpString  = self.completeTune length])
    {
        x264_tune = [tmpString UTF8String];
    }
    if ([(tmpString = self.videoOptionExtra) length])
    {
        advanced_opts = [tmpString UTF8String];
    }
    if ([(tmpString = self.profile) length])
    {
        h264_profile = [tmpString UTF8String];
    }
    if ([(tmpString = self.level) length])
    {
        h264_level = [tmpString UTF8String];
    }

    // now, unparse
    char *fX264PresetsUnparsedUTF8String = hb_x264_param_unparse(
                                    hb_video_encoder_get_depth(self.encoder),
                                    x264_preset, x264_tune, advanced_opts,
                                    h264_profile, h264_level,
                                    self.job.picture.width,
                                    self.job.picture.height);
    // update the text field
    if (fX264PresetsUnparsedUTF8String != NULL)
    {
        tmpString = @(fX264PresetsUnparsedUTF8String);
        free(fX264PresetsUnparsedUTF8String);
    }
    else
    {
        tmpString = @"";
    }

    return tmpString;
}

@end


#pragma mark - Value Transformers

@implementation HBVideoEncoderTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_video_encoder_get_name([value intValue]);
    if (name)
    {
        return @(name);
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
    return @(hb_video_encoder_get_from_name([value UTF8String]));
}

@end

@implementation HBFrameRateTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_video_framerate_get_name([value intValue]);
    if (name)
    {
        if (!strcmp(name, "23.976"))
        {
            return @"23.976 (NTSC Film)";
        }
        else if (!strcmp(name, "25"))
        {
            return @"25 (PAL Film/Video)";
        }
        else if (!strcmp(name, "29.97"))
        {
            return @"29.97 (NTSC Video)";
        }
        else
        {
            return @(name);
        }
    }
    else
    {
        return HBKitLocalizedString(@"Same as source", @"HBVideo -> frame rates display name");
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value isEqualTo:HBKitLocalizedString(@"Same as source", @"HBVideo -> frame rates display name")])
    {
        return @0;
    }
    else
    {
        return @(hb_video_framerate_get_from_name([value UTF8String]));
    }
}

@end

@implementation HBPresetsTransformer
{
    int _encoder;
}

- (instancetype)init
{
    @throw nil;
}

- (instancetype)initWithEncoder:(int)encoder
{
    self = [super init];
    if (self)
    {
        _encoder = encoder;
    }
    return self;
}

+ (Class)transformedValueClass
{
    return [NSNumber class];
}

- (id)transformedValue:(id)value
{
    if (value)
    {
        const char * const *presets = hb_video_encoder_get_presets(_encoder);
        for (int i = 0; presets != NULL && presets[i] != NULL; i++)
        {
            if (!strcasecmp(presets[i], [value UTF8String]))
            {
                return @(i);
            }
        }
    }

    return @(-1);
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    const char * const *presets = hb_video_encoder_get_presets(_encoder);
    for (int i = 0; presets != NULL && presets[i] != NULL; i++)
    {
        if (i == [value intValue])
        {
            return @(presets[i]);
        }
    }

    return @"none";
}

@end

@implementation HBQualityTransformer
{
    BOOL _reverse;
    double _min;
    double _max;
    double _granularity;
}

- (instancetype)init
{
    return [self initWithReversedDirection:NO min:0 max:50 granularity:1];
}

- (instancetype)initWithReversedDirection:(BOOL)reverse min:(double)min max:(double)max granularity:(float)granularity
{
    self = [super init];
    if (self)
    {
        _reverse = reverse;
        _min = min;
        _max = max;
        _granularity = granularity > 0 ? granularity : 1;
    }

    return self;
}

+ (Class)transformedValueClass
{
    return [NSNumber class];
}

- (id)transformedValue:(id)value
{
    if (_reverse)
    {
        double inverseValue = _min + _max - [value doubleValue];
        return @(inverseValue);
    }
    else
    {
        return value;
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if (_reverse)
    {
        double inverseValue = _min + _max - [value doubleValue];
        inverseValue = round(inverseValue / _granularity) * _granularity;
        return @(inverseValue);
    }
    else
    {
        double doubleValue = [value doubleValue];
        doubleValue = round(doubleValue / _granularity) * _granularity;
        return @(doubleValue);
    }
}

@end

@implementation HBTuneTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    if ([value isEqualToString:@"none"])
    {
        return HBKitLocalizedString(@"none", @"HBVideo -> tune");
    }
    else
    {
        return value;
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value isEqualTo:HBKitLocalizedString(@"none", @"HBVideo -> tune")])
    {
        return @"none";
    }
    else
    {
        return value;
    }
}

@end

@implementation HBTunesTransformer

+ (Class)transformedValueClass
{
    return [NSArray class];
}

- (id)transformedValue:(id)value
{
    if (value != nil)
    {
        NSMutableArray *localizedArray = [NSMutableArray array];

        for (NSString *text in value)
        {
            if ([text isEqualToString:@"none"])
            {
                [localizedArray addObject:HBKitLocalizedString(@"none", @"HBVideo -> tune")];
            }
            else
            {
                [localizedArray addObject:text];
            }
        }
        return localizedArray;
    }

    return value;
}

+ (BOOL)allowsReverseTransformation
{
    return NO;
}

@end

@implementation HBVideo (EncoderAdditions)

- (BOOL)isUnparsedSupported:(int)encoder
{
    return (encoder & HB_VCODEC_X264_MASK) != 0;
}
- (BOOL)isPresetSystemSupported:(int)encoder
{
    return hb_video_encoder_get_presets(encoder) != NULL;
}

- (BOOL)isSimpleOptionsPanelSupported:(int)encoder
{
    return (encoder & HB_VCODEC_FFMPEG_MASK) != 0;
}

- (void)qualityLimitsForEncoder:(int)encoder low:(float *)low high:(float *)high granularity:(float *)granularity direction:(int *)direction
{
    hb_video_quality_get_limits(encoder, low, high, granularity, direction);
}

@end
