/*  HBVideo+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideo+UIAdditions.h"
#import "HBJob.h"
#include "hb.h"

@implementation HBVideo (UIAdditions)

@dynamic qualityMaxValue;
@dynamic qualityMinValue;
@dynamic levels;
@dynamic tunes;
@dynamic presets;
@dynamic profiles;

#pragma mark - Possible values

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

    [framerates addObject:NSLocalizedString(@"Same as source", @"")];

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

- (BOOL)fastDecodeSupported
{
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

- (BOOL)turboTwoPassSupported
{
    return ((self.encoder & HB_VCODEC_X264_MASK) ||
            (self.encoder & HB_VCODEC_X265_MASK));
}

/**
 *  This is called everytime a x264 widget in the video tab is changed to
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


#pragma mark - Value Trasformers

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
        return NSLocalizedString(@"Same as source", @"");
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value isEqualTo:NSLocalizedString(@"Same as source", @"")])
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
}

- (instancetype)init
{
    return [self initWithReversedDirection:NO min:0 max:50];
}

- (instancetype)initWithReversedDirection:(BOOL)reverse min:(double)min max:(double)max
{
    self = [super init];
    if (self)
    {
        _reverse = reverse;
        _min = min;
        _max = max;
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
        return @(inverseValue);
    }
    else
    {
        return value;
    }
}

@end
