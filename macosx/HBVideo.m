/*  HBVideo.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideo.h"

@interface HBVideo ()

@property (nonatomic, readwrite) double qualityMinValue;
@property (nonatomic, readwrite) double qualityMaxValue;

@property (nonatomic, readwrite) NSUInteger mediumPresetIndex;

@end

@implementation HBVideo

- (instancetype)init
{
    self = [super init];
    if (self) {
        _encoder = HB_VCODEC_X264;
        _avgBitrate = 1000;
        _quality = 18.0;
        _qualityMaxValue = 51.0f;

        _widthForUnparse = 1;
        _heightForUnparse = 1;

        [self updateQualityBounds];
    }
    return self;
}

#pragma mark - Setters

/**
 *  Updates the min/max quality values
 */
- (void)updateQualityBounds
{
    // Get the current slider maxValue to check for a change in slider scale
    // later so that we can choose a new similar value on the new slider scale
    double previousMaxValue             = self.qualityMaxValue;
    double previousPercentOfSliderScale = (self.quality / (self.qualityMaxValue - self.qualityMinValue + 1));

    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits(self.encoder,
                                &minValue, &maxValue, &granularity, &direction);

    self.qualityMinValue = minValue;
    self.qualityMaxValue = maxValue;

    // check to see if we have changed slider scales
    if (previousMaxValue != maxValue)
    {
        // if so, convert the old setting to the new scale as close as possible
        // based on percentages
        self.quality = floor((maxValue - minValue + 1.) * (previousPercentOfSliderScale));
    }
}

- (void)setEncoder:(int)encoder
{
    _encoder = encoder;
    [self updateQualityBounds];
    [self validatePresetsSettings];
    [self validateAdvancedOptions];
}

- (void)setContainer:(int)container
{
    _container = container;

    BOOL encoderSupported = NO;

    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & self.container)
        {
            if (video_encoder->codec == self.encoder)
            {
                encoderSupported = YES;
            }
        }
    }

    if (!encoderSupported)
    {
        self.encoder = HB_VCODEC_X264;
    }
}

- (void)setTune:(NSString *)tune
{
    [_tune autorelease];

    if (![tune isEqualToString:@"none"])
    {
        _tune = [tune copy];
    }
    else
    {
        _tune = @"";
    }
}

- (void)validatePresetsSettings
{
    NSArray *presets = self.presets;
    if (presets.count && ![presets containsObject:self.preset]) {
        self.preset = presets[self.mediumPresetIndex];
    }

    NSArray *tunes = self.tunes;
    if (tunes.count && ![tunes containsObject:self.tune]) {
        self.tune = tunes.firstObject;
    }

    NSArray *profiles = self.profiles;
    if (profiles.count && ![profiles containsObject:self.profile]) {
        self.profile = profiles.firstObject;
    }

    NSArray *levels = self.levels;
    if (levels.count && ![levels containsObject:self.level]) {
        self.level = levels.firstObject;
    }
}

- (void)validateAdvancedOptions
{
    if (self.encoder != HB_VCODEC_H264_MASK)
    {
        self.advancedOptions = NO;
    }
}

#pragma mark - Possible values

- (NSArray *)encoders
{
    NSMutableArray *encoders = [NSMutableArray array];
    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & self.container)
        {
            [encoders addObject:@(video_encoder->name)];
        }
    }
    return [[encoders copy] autorelease];
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
    return [[framerates copy] autorelease];
}

- (NSArray *)presets
{
    NSMutableArray *temp = [NSMutableArray array];

    const char * const *presets = hb_video_encoder_get_presets(self.encoder);
    for (int i = 0; presets != NULL && presets[i] != NULL; i++)
    {
        [temp addObject:@(presets[i])];
        if (!strcasecmp(presets[i], "medium"))
        {
            self.mediumPresetIndex = i;
        }
    }

    return [[temp copy] autorelease];
}

- (NSArray *)tunes
{
    NSMutableArray *temp = [NSMutableArray array];

    [temp addObject:@"none"];

    const char * const *tunes = hb_video_encoder_get_tunes(self.encoder);

    for (int i = 0; tunes != NULL && tunes[i] != NULL; i++)
    {
        // we filter out "fastdecode" as we have a dedicated checkbox for it
        if (strcasecmp(tunes[i], "fastdecode") != 0)
        {
            [temp addObject:@(tunes[i])];
        }
    }

    return [[temp copy] autorelease];
}

- (NSArray *)profiles
{
    NSMutableArray *temp = [NSMutableArray array];

    const char * const *profiles = hb_video_encoder_get_profiles(self.encoder);
    for (int i = 0; profiles != NULL && profiles[i] != NULL; i++)
    {
        [temp addObject:@(profiles[i])];
    }

    return [[temp copy] autorelease];
}

- (NSArray *)levels
{
    NSMutableArray *temp = [NSMutableArray array];

    const char * const *levels = hb_video_encoder_get_levels(self.encoder);
    for (int i = 0; levels != NULL && levels[i] != NULL; i++)
    {
        [temp addObject:@(levels[i])];
    }
    if (!temp.count)
    {
        [temp addObject:@"auto"];
    }

    return [[temp copy] autorelease];
}

- (BOOL)fastDecodeSupported
{
    return (self.encoder == HB_VCODEC_X264);
}

/**
 *  This is called everytime a x264 widget in the video tab is changed to
 *  display the expanded options in a text field via outlet fDisplayX264PresetsUnparseTextField
 */
- (NSString *)unparseOptions
{
    if (self.encoder != HB_VCODEC_X264)
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

    tmpString = self.tune;
    if (self.fastDecode)
    {
        if (self.tune.length)
        {
            tmpString = [tmpString stringByAppendingString: @","];
        }
        tmpString = [tmpString stringByAppendingString: @"fastdecode"];
    }

    // prepare the tune, advanced options, profile and level
    if ([tmpString length])
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
    char *fX264PresetsUnparsedUTF8String = hb_x264_param_unparse(x264_preset,
                                                           x264_tune,
                                                           advanced_opts,
                                                           h264_profile,
                                                           h264_level,
                                                           _widthForUnparse, _heightForUnparse);
    // update the text field
    if (fX264PresetsUnparsedUTF8String != NULL)
    {
        tmpString = [NSString stringWithUTF8String:fX264PresetsUnparsedUTF8String];
        free(fX264PresetsUnparsedUTF8String);
    }
    else
    {
        tmpString = @"";
    }

    return tmpString;
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    // Tell KVO to reload the presets settings
    // after a change to the encoder.
    if ([key isEqualToString:@"presets"] ||
        [key isEqualToString:@"tunes"] ||
        [key isEqualToString:@"profiles"] ||
        [key isEqualToString:@"levels"])
    {
        retval = [NSSet setWithObjects:@"encoder", nil];
    }

    // Tell KVO to reload the x264 unparse string
    // after values changes.
    if ([key isEqualToString:@"unparseOptions"])
    {
        retval = [NSSet setWithObjects:@"encoder", @"preset", @"tune", @"profile", @"level",
                  @"videoOptionExtra", @"fastDecode", @"widthForUnparse", @"heightForUnparse", nil];
    }

    if ([key isEqualToString:@"encoders"])
    {
        retval = [NSSet setWithObjects:@"container", nil];
    }

    if ([key isEqualToString:@"fastDecodeSupported"])
    {
        retval = [NSSet setWithObjects:@"encoder", nil];
    }

    return retval;
}

#pragma mark - Various conversion methods from dict/preset/queue/etc

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    // map legacy encoder names via libhb.
    const char *strValue = hb_video_encoder_sanitize_name([preset[@"VideoEncoder"] UTF8String]);
    self.encoder = hb_video_encoder_get_from_name(strValue);

    if (self.encoder == HB_VCODEC_X264 || self.encoder == HB_VCODEC_X265)
    {
        if (self.encoder == HB_VCODEC_X264 &&
            (!preset[@"x264UseAdvancedOptions"] ||
             [preset[@"x264UseAdvancedOptions"] intValue]))
        {
            // preset does not use the x264 preset system, reset the widgets.
            self.preset = @"medium";
            self.tune = nil;
            self.profile = nil;
            self.level = nil;

            // x264UseAdvancedOptions is not set (legacy preset)
            // or set to 1 (enabled), so we use the old advanced panel.
            if (preset[@"x264Option"])
            {
                // we set the advanced options string here if applicable.
                self.videoOptionExtra = preset[@"x264Option"];
                self.advancedOptions = YES;
            }
            else
            {
                self.videoOptionExtra = nil;
            }
        }
        else
        {
            // x264UseAdvancedOptions is set to 0 (disabled),
            // so we use the new preset system and
            // disable the advanced panel
            self.advancedOptions = NO;

            if (preset[@"x264Preset"])
            {
                // Read the old x264 preset keys
                self.preset = preset[@"x264Preset"];
                self.tune   = preset[@"x264Tune"];
                self.videoOptionExtra = preset[@"x264OptionExtra"];
                self.profile = preset[@"h264Profile"];
                self.level   = preset[@"h264Level"];
            }
            else
            {
                // Read the new preset keys (0.10)
                self.preset = preset[@"VideoPreset"];
                self.tune   = preset[@"VideoTune"];
                self.videoOptionExtra = preset[@"VideoOptionExtra"];
                self.profile = preset[@"VideoProfile"];
                self.level   = preset[@"VideoLevel"];
            }
        }
    }
    else
    {
        if (preset[@"lavcOption"])
        {
            // Load the old format
            self.videoOptionExtra = preset[@"lavcOption"];
        }
        else
        {
            self.videoOptionExtra = preset[@"VideoOptionExtra"];
        }
    }

    int qualityType = [preset[@"VideoQualityType"] intValue] - 1;
    /* Note since the removal of Target Size encoding, the possible values for VideoQuality type are 0 - 1.
     * Therefore any preset that uses the old 2 for Constant Quality would now use 1 since there is one less index
     * for the fVidQualityMatrix. It should also be noted that any preset that used the deprecated Target Size
     * setting of 0 would set us to 0 or ABR since ABR is now tagged 0. Fortunately this does not affect any built-in
     * presets since they all use Constant Quality or Average Bitrate.*/
    if (qualityType == -1)
    {
        qualityType = 0;
    }
    self.qualityType = qualityType;

    self.avgBitrate = [preset[@"VideoAvgBitrate"] intValue];
    self.quality = [preset[@"VideoQualitySlider"] floatValue];

    // Video framerate
    if ([preset[@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        // Now set the Video Frame Rate Mode to either vfr or cfr according to the preset.
        if (!preset[@"VideoFramerateMode"] ||
            [preset[@"VideoFramerateMode"] isEqualToString:@"vfr"])
        {
            self.frameRateMode = 0; // we want vfr
        }
        else
        {
            self.frameRateMode = 1; // we want cfr
        }
    }
    else
    {
        // Now set the Video Frame Rate Mode to either pfr or cfr according to the preset.
        if ([preset[@"VideoFramerateMode"] isEqualToString:@"pfr"] ||
            [preset[@"VideoFrameratePFR"]  intValue] == 1)
        {
            self.frameRateMode = 0; // we want pfr
        }
        else
        {
            self.frameRateMode = 1; // we want cfr
        }
    }
    // map legacy names via libhb.
    int intValue = hb_video_framerate_get_from_name([preset[@"VideoFramerate"] UTF8String]);
    if (intValue == -1)
    {
        intValue = 0;
    }
    self.frameRate = intValue;

    // 2 Pass Encoding.
    self.twoPass = [preset[@"VideoTwoPass"] intValue];

    // Turbo 1st pass for 2 Pass Encoding.
    self.turboTwoPass = [preset[@"VideoTurboTwoPass"] intValue];
}

- (void)prepareVideoForPreset:(NSMutableDictionary *)preset
{
    preset[@"VideoEncoder"] = @(hb_video_encoder_get_name(self.encoder));

    // x264 Options, this will either be advanced panel or the video tabs x264 presets panel with modded option string
    if (self.advancedOptions)
    {
        // use the old advanced panel.
        preset[@"x264UseAdvancedOptions"] = @1;
        preset[@"x264Option"] = self.videoOptionExtra;
    }
    else if (self.encoder == HB_VCODEC_X264 || self.encoder == HB_VCODEC_X265)
    {
        // use the new preset system.
        preset[@"x264UseAdvancedOptions"] = @0;
        preset[@"VideoPreset"]      = self.preset;
        preset[@"VideoTune"]        = self.tune;
        preset[@"VideoOptionExtra"] = self.videoOptionExtra;
        preset[@"VideoProfile"]     = self.profile;
        preset[@"VideoLevel"]       = self.level;
    }
    else
    {
        // FFmpeg (lavc) Option String
        preset[@"VideoOptionExtra"] = self.videoOptionExtra;
    }

    /* though there are actually only 0 - 1 types available in the ui we need to map to the old 0 - 2
     * set of indexes from when we had 0 == Target , 1 == Abr and 2 == Constant Quality for presets
     * to take care of any legacy presets. */
    preset[@"VideoQualityType"] = @(self.qualityType + 1);
    preset[@"VideoAvgBitrate"] = @(self.avgBitrate);
    preset[@"VideoQualitySlider"] = @(self.quality);

    /* Video framerate and framerate mode */
    if (self.frameRateMode == 1)
    {
        preset[@"VideoFramerateMode"] = @"cfr";
    }
    if (self.frameRate == 0) // Same as source is selected
    {
        preset[@"VideoFramerate"] = @"Same as source";

        if (self.frameRateMode == 0)
        {
            preset[@"VideoFramerateMode"] = @"vfr";
        }
    }
    else // translate the rate (selected item's tag) to the official libhb name
    {
        preset[@"VideoFramerate"] = [NSString stringWithFormat:@"%s",
                                     hb_video_framerate_get_name((int)self.frameRate)];

        if (self.frameRateMode == 0)
        {
            preset[@"VideoFramerateMode"] = @"pfr";
        }
    }

    preset[@"VideoTwoPass"] = @(self.twoPass);
    preset[@"VideoTurboTwoPass"] = @(self.turboTwoPass);
}

- (void)applyVideoSettingsFromQueue:(NSDictionary *)queueToApply
{
    // Video encoder
    self.encoder = [queueToApply[@"JobVideoEncoderVcodec"] intValue];

    // Advanced x264 options
    if ([queueToApply[@"x264UseAdvancedOptions"] intValue])
    {
        // we are using the advanced panel
        self.preset = @"medium";
        self.tune = nil;
        self.profile = nil;
        self.level = nil;
        self.videoOptionExtra = queueToApply[@"x264Option"];
        self.advancedOptions = YES;
    }
    else if (self.encoder == HB_VCODEC_X264 || self.encoder == HB_VCODEC_X265)
    {
        // we are using the x264 preset system
        self.preset = queueToApply[@"VideoPreset"];
        self.tune = queueToApply[@"VideoTune"];
        self.videoOptionExtra = queueToApply[@"VideoOptionExtra"];
        self.profile = queueToApply[@"VideoProfile"];
        self.level = queueToApply[@"VideoLevel"];
    }
    else
    {
        self.videoOptionExtra = queueToApply[@"VideoOptionExtra"];
    }

    self.qualityType = [queueToApply[@"VideoQualityType"] intValue] - 1;

    self.avgBitrate = [queueToApply[@"VideoAvgBitrate"] intValue];
    self.quality = [queueToApply[@"VideoQualitySlider"] doubleValue];

    // Video framerate
    if ([queueToApply[@"JobIndexVideoFramerate"] intValue] == 0)
    {
        // Now set the Video Frame Rate Mode to either vfr or cfr according to the preset
        if ([queueToApply[@"VideoFramerateMode"] isEqualToString:@"vfr"])
        {
            self.frameRateMode = 0; // we want vfr
        }
        else
        {
            self.frameRateMode = 1; // we want cfr
        }
    }
    else
    {
        // Now set the Video Frame Rate Mode to either pfr or cfr according to the preset
        if ([queueToApply[@"VideoFramerateMode"] isEqualToString:@"pfr"])
        {
            self.frameRateMode = 0; // we want pfr
        }
        else
        {
            self.frameRateMode = 1; // we want cfr
        }
    }

    self.frameRate = hb_video_framerate_get_from_name([queueToApply[@"VideoFramerate"] UTF8String]);

    self.twoPass = [queueToApply[@"VideoTwoPass"] intValue];
    self.turboTwoPass = [queueToApply[@"VideoTurboTwoPass"] intValue];
}

- (void)prepareVideoForQueueFileJob:(NSMutableDictionary *)queueFileJob
{
    // Video encoder.
    queueFileJob[@"VideoEncoder"] = @(hb_video_encoder_get_name(self.encoder));
    queueFileJob[@"JobVideoEncoderVcodec"] = @(self.encoder);

    // x264 advanced options.
    if (self.advancedOptions)
    {
        // we are using the advanced panel
        queueFileJob[@"x264UseAdvancedOptions"] = @1;
        queueFileJob[@"x264Option"] = self.videoOptionExtra;
     }
     else if (self.encoder == HB_VCODEC_X264 || self.encoder == HB_VCODEC_X265)
     {
         // we are using the x264/x265 preset system.
         queueFileJob[@"x264UseAdvancedOptions"] = @0;
         queueFileJob[@"VideoPreset"] = self.preset;
         queueFileJob[@"VideoTune"] = self.tune;
         queueFileJob[@"VideoOptionExtra"] = self.videoOptionExtra;
         queueFileJob[@"VideoProfile"] = self.profile;
         queueFileJob[@"VideoLevel"] = self.level;
     }
     else
     {
         // FFmpeg (lavc) Option String.
         queueFileJob[@"VideoOptionExtra"] = self.videoOptionExtra;
     }

    queueFileJob[@"VideoQualityType"] = @(self.qualityType + 1);
    queueFileJob[@"VideoAvgBitrate"] = @(self.avgBitrate);
    queueFileJob[@"VideoQualitySlider"] = @(self.quality);

    // Framerate
    if (self.frameRate)
    {
        queueFileJob[@"VideoFramerate"] = @(hb_video_framerate_get_name(self.frameRate));
    }
    else
    {
        queueFileJob[@"VideoFramerate"] = @"Same as source";
    }
    queueFileJob[@"JobIndexVideoFramerate"] = @(self.frameRate);

    // Frame Rate Mode
    if (self.frameRateMode == 1) // if selected we are cfr regardless of the frame rate popup
    {
        queueFileJob[@"VideoFramerateMode"] = @"cfr";
    }
    else
    {
        if (self.frameRate == 0) // Same as source frame rate
        {
            queueFileJob[@"VideoFramerateMode"] = @"vfr";
        }
        else
        {
            queueFileJob[@"VideoFramerateMode"] = @"pfr";
        }
    }

    // 2 Pass Encoding
    queueFileJob[@"VideoTwoPass"] = @(self.twoPass);
    queueFileJob[@"VideoTurboTwoPass"] = @(self.turboTwoPass);
}

- (void)prepareVideoForJobPreview:(hb_job_t *)job andTitle:(hb_title_t *)title
{
    job->vcodec = self.encoder;
    job->fastfirstpass = 0;

    job->chapter_markers = 0;

    if (job->vcodec == HB_VCODEC_X264)
    {
        /* advanced x264 options */
        NSString   *tmpString;
        // translate zero-length strings to NULL for libhb
        const char *encoder_preset  = NULL;
        const char *encoder_tune    = NULL;
        const char *encoder_options = NULL;
        const char *encoder_profile = NULL;
        const char *encoder_level   = NULL;
        if (self.advancedOptions)
        {
            // we are using the advanced panel
            if ([(tmpString = self.videoOptionExtra) length])
            {
                encoder_options = [tmpString UTF8String];
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = self.tune) length])
            {
                encoder_tune = [tmpString UTF8String];
            }
            if ([(tmpString = self.videoOptionExtra) length])
            {
                encoder_options = [tmpString UTF8String];
            }
            if ([(tmpString = self.profile) length])
            {
                encoder_profile = [tmpString UTF8String];
            }
            if ([(tmpString = self.level) length])
            {
                encoder_level = [tmpString UTF8String];
            }
            encoder_preset = [self.preset UTF8String];
        }
        hb_job_set_encoder_preset (job, encoder_preset);
        hb_job_set_encoder_tune   (job, encoder_tune);
        hb_job_set_encoder_options(job, encoder_options);
        hb_job_set_encoder_profile(job, encoder_profile);
        hb_job_set_encoder_level  (job, encoder_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_encoder_options(job, [self.videoOptionExtra UTF8String]);
    }

    /* Video settings */
    int fps_mode, fps_num, fps_den;
    if (self.frameRate > 0 )
    {
        /* a specific framerate has been chosen */
        fps_num = 27000000;
        fps_den = (int)self.frameRate;
        if (self.frameRateMode == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // PFR
            fps_mode = 2;
        }
    }
    else
    {
        /* same as source */
        fps_num = title->vrate.num;
        fps_den = title->vrate.den;
        if (self.frameRateMode == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // VFR
            fps_mode = 0;
        }
    }

    switch (self.qualityType)
    {
        case 0:
            /* ABR */
            job->vquality = -1.0;
            job->vbitrate = self.avgBitrate;
            break;
        case 1:
            /* Constant Quality */
            job->vquality = self.quality;
            job->vbitrate = 0;
            break;
    }

    /* Add framerate shaping filter */
    hb_filter_object_t *filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);
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
