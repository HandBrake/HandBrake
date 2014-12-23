/*  HBVideo.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideo.h"
#import "HBJob.h"
#import "NSCodingMacro.h"
#include "hb.h"

@interface HBVideo ()

@property (nonatomic, readwrite) double qualityMinValue;
@property (nonatomic, readwrite) double qualityMaxValue;

@property (nonatomic, readwrite) NSUInteger mediumPresetIndex;

@end

@implementation HBVideo

- (instancetype)initWithJob:(HBJob *)job;
{
    self = [super init];
    if (self) {
        _encoder = HB_VCODEC_X264;
        _avgBitrate = 1000;
        _quality = 18.0;
        _qualityMaxValue = 51.0f;
        _job = job;

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

- (void)containerChanged
{
    BOOL encoderSupported = NO;

    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & self.job.container)
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
                  @"videoOptionExtra", @"fastDecode", @"job.picture.width", @"job.picture.height", nil];
    }

    if ([key isEqualToString:@"encoders"])
    {
        retval = [NSSet setWithObjects:@"job.container", nil];
    }

    if ([key isEqualToString:@"fastDecodeSupported"])
    {
        retval = [NSSet setWithObjects:@"encoder", nil];
    }

    return retval;
}

#pragma mark -

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

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBVideoVersion"];

    encodeInt(_encoder);

    encodeInt(_qualityType);
    encodeInt(_avgBitrate);
    encodeDouble(_quality);

    encodeInt(_frameRate);
    encodeInt(_frameRateMode);

    encodeBool(_twoPass);
    encodeBool(_turboTwoPass);

    encodeInt(_advancedOptions);
    encodeObject(_preset);
    encodeObject(_tune);
    encodeObject(_profile);
    encodeObject(_level);

    encodeObject(_videoOptionExtra);

    encodeBool(_fastDecode);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_encoder);

    decodeInt(_qualityType);
    decodeInt(_avgBitrate);
    decodeDouble(_quality);

    decodeInt(_frameRate);
    decodeInt(_frameRateMode);

    decodeBool(_twoPass);
    decodeBool(_turboTwoPass);

    decodeInt(_advancedOptions);
    decodeObject(_preset);
    decodeObject(_tune);
    decodeObject(_profile);
    decodeObject(_level);

    decodeObject(_videoOptionExtra);

    decodeBool(_fastDecode);

    return self;
}

#pragma mark - Various conversion methods from dict/preset/queue/etc

/**
 *  Returns a string minus the fastdecode string.
 */
- (NSString *)stripFastDecodeFromString:(NSString *)tune
{
    // filter out fastdecode
    tune = [tune stringByReplacingOccurrencesOfString:@"," withString:@""];
    tune = [tune stringByReplacingOccurrencesOfString:@"fastdecode" withString:@""];

    return tune;
}

/**
 *  Retuns the tune string plus the fastdecode option (if enabled)
 */
- (NSString *)completeTune
{
    NSMutableString *string = [[NSMutableString alloc] init];

    if (self.tune)
    {
        [string appendString:self.tune];
    }

    if (self.fastDecode)
    {
        if (string.length)
        {
            [string appendString:@","];
        }

        [string appendString:@"fastdecode"];
    }

    return [string autorelease];
}

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
            self.fastDecode = NO;

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

            if ([self.tune rangeOfString:@"fastdecode"].location != NSNotFound)
            {
                self.fastDecode = YES;;
            }
            else
            {
                self.fastDecode = NO;
            }

            self.tune = [self stripFastDecodeFromString:self.tune];
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
        preset[@"VideoTune"]        = [self completeTune];
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
         queueFileJob[@"VideoTune"] = [self completeTune];
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

@end
