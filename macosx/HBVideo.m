/*  HBVideo.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideo.h"
#import "HBJob.h"
#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"

#include "handbrake/handbrake.h"

NSString * const HBVideoChangedNotification = @"HBVideoChangedNotification";

@interface HBVideo ()

@property (nonatomic, readwrite, weak) HBJob *job;

@property (nonatomic, readwrite) double qualityMinValue;
@property (nonatomic, readwrite) double qualityMaxValue;

@property (nonatomic, readwrite) NSUInteger mediumPresetIndex;

@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@end

@implementation HBVideo

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self) {
        _encoder = HB_VCODEC_X264;
        _avgBitrate = 1000;
        _quality = 18.0;
        _qualityMaxValue = 51.0f;
        _job = job;

        _preset = @"medium";
        _tune = @"none";
        _profile = @"auto";
        _level = @"auto";

        _videoOptionExtra = @"";

        [self updateQualityBounds];

        _notificationsEnabled = YES;
    }
    return self;
}

- (void)postChangedNotification
{
    if (self.areNotificationsEnabled)
    {
        [[NSNotificationCenter defaultCenter] postNotification: [NSNotification notificationWithName:HBVideoChangedNotification
                                                                                              object:self
                                                                                            userInfo:nil]];
    }
}

#pragma mark - Setters

/**
 *  Updates the min/max quality values
 */
- (void)updateQualityBounds
{
    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits(self.encoder, &minValue, &maxValue, &granularity, &direction);

    self.qualityMinValue = minValue;
    self.qualityMaxValue = maxValue;

    // Ensure the quality value is not out of the new bounds
    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        if (self.quality > maxValue)
        {
            self.quality = maxValue;
        }
        else if (self.quality < minValue)
        {
            self.quality = minValue;
        }
    }
}

- (void)setEncoder:(int)encoder
{
    int previousEncoder = _encoder;

    if (encoder != _encoder)
    {
        [[self.undo prepareWithInvocationTarget:self] setEncoder:_encoder];
    }
    _encoder = encoder;
    [self updateQualityBounds];

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateQualityType];
        [self validatePresetsSettings];
        [self validateVideoOptionExtra:previousEncoder];
    }

    [self postChangedNotification];
}

- (void)setQualityType:(HBVideoQualityType)qualityType
{
    if (qualityType != _qualityType)
    {
        [[self.undo prepareWithInvocationTarget:self] setQualityType:_qualityType];
    }
    _qualityType = qualityType;
    [self postChangedNotification];
}

- (void)setAvgBitrate:(int)avgBitrate
{
    if (avgBitrate != _avgBitrate)
    {
        [[self.undo prepareWithInvocationTarget:self] setAvgBitrate:_avgBitrate];
    }
    _avgBitrate = avgBitrate;
    [self postChangedNotification];
}

- (void)setQuality:(double)quality
{
    if (quality != _quality)
    {
        [[self.undo prepareWithInvocationTarget:self] setQuality:_quality];
    }
    _quality = quality;
    [self postChangedNotification];
}

- (void)setFrameRate:(int)frameRate
{
    if (frameRate != _frameRate)
    {
        [[self.undo prepareWithInvocationTarget:self] setFrameRate:_frameRate];
    }
    _frameRate = frameRate;
    [self postChangedNotification];
}

- (void)setFrameRateMode:(HBVideoFrameRateMode)frameRateMode
{
    if (frameRateMode != _frameRateMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setFrameRateMode:_frameRateMode];
    }
    _frameRateMode = frameRateMode;
    [self postChangedNotification];
}

- (void)setTwoPass:(BOOL)twoPass
{
    if (twoPass != _twoPass)
    {
        [[self.undo prepareWithInvocationTarget:self] setTwoPass:_twoPass];
    }
    _twoPass = twoPass;
    [self postChangedNotification];
}

- (void)setTurboTwoPass:(BOOL)turboTwoPass
{
    if (turboTwoPass != _turboTwoPass)
    {
        [[self.undo prepareWithInvocationTarget:self] setTurboTwoPass:_turboTwoPass];
    }
    _turboTwoPass = turboTwoPass;
    [self postChangedNotification];
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
        self.encoder = hb_video_encoder_get_default(self.job.container);
    }
}

- (void)setPreset:(NSString *)preset
{
    if (![preset isEqualToString:_preset])
    {
        [[self.undo prepareWithInvocationTarget:self] setPreset:_preset];
    }
    _preset = [preset copy];
    [self postChangedNotification];
}

- (void)setTune:(NSString *)tune
{
    if (![tune isEqualToString:_tune])
    {
        [[self.undo prepareWithInvocationTarget:self] setTune:_tune];
    }

    if (tune == nil || [tune isEqualToString:@""])
    {
        _tune = @"none";
    }
    else
    {
        _tune = [tune copy];
    }

    [self postChangedNotification];
}

- (void)setProfile:(NSString *)profile
{
    if (![profile isEqualToString:_profile])
    {
        [[self.undo prepareWithInvocationTarget:self] setProfile:_profile];
    }
    _profile = [profile copy];
    [self postChangedNotification];
}

- (void)setLevel:(NSString *)level
{
    if (![level isEqualToString:_level])
    {
        [(HBVideo *)[self.undo prepareWithInvocationTarget:self] setLevel:_level];
    }
    _level = [level copy];
    [self postChangedNotification];
}

- (void)setVideoOptionExtra:(NSString *)videoOptionExtra
{
    if (![videoOptionExtra isEqualToString:_videoOptionExtra])
    {
        [[self.undo prepareWithInvocationTarget:self] setVideoOptionExtra:_videoOptionExtra];
    }
    if (videoOptionExtra != nil)
    {
        _videoOptionExtra = [videoOptionExtra copy];
    }
    else
    {
        _videoOptionExtra = @"";
    }
    [self postChangedNotification];
}

- (void)setFastDecode:(BOOL)fastDecode
{
    if (fastDecode != _fastDecode)
    {
        [[self.undo prepareWithInvocationTarget:self] setFastDecode:_fastDecode];
    }
    _fastDecode = fastDecode;
    [self postChangedNotification];
}

- (void)validateQualityType
{
    if (self.qualityType != 0 && !hb_video_quality_is_supported(self.encoder))
    {
        self.qualityType = 0;
    }

    if (!hb_video_twopass_is_supported(self.encoder))
    {
        self.twoPass = NO;
    }
}

- (void)validatePresetsSettings
{
    NSArray *presets = self.presets;
    if (presets.count && ![presets containsObject:self.preset])
    {
        if (self.encoder & HB_VCODEC_VT_MASK)
        {
            self.preset = self.presets.lastObject;
        }
        else if (presets.count > self.mediumPresetIndex)
        {
            self.preset = presets[self.mediumPresetIndex];
        }
        else
        {
            self.preset = presets.firstObject;
        }
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

    if (!(self.encoder & (HB_VCODEC_X264_MASK | HB_VCODEC_SVT_AV1_MASK)))
    {
        self.fastDecode = NO;
    }
}

- (void)validateVideoOptionExtra:(int)previousEncoder
{
    if (!((previousEncoder & HB_VCODEC_X264_MASK &&
        self.encoder & HB_VCODEC_X264_MASK) ||
        (previousEncoder & HB_VCODEC_X265_MASK &&
         self.encoder & HB_VCODEC_X265_MASK)))
    {
        self.videoOptionExtra = @"";
    }
}

- (void)setNilValueForKey:(NSString *)key
{
    [self setValue:@0 forKey:key];
}

#pragma mark -

+ (NSSet<NSString *> *)keyPathsForValuesAffectingPresets
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (NSArray<NSString *> *)presets
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

    return [temp copy];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingTunes
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (NSArray<NSString *> *)tunes
{
    NSMutableArray<NSString *> *temp = [NSMutableArray array];

    [temp addObject:@"none"];

    const char * const *tunes = hb_video_encoder_get_tunes(self.encoder);

    for (int i = 0; tunes != NULL && tunes[i] != NULL; i++)
    {
        // we filter out "fastdecode" as we have a dedicated checkbox for it
        if (!(self.encoder & (HB_VCODEC_X264_MASK | HB_VCODEC_SVT_AV1_MASK))
            || strcasecmp(tunes[i], "fastdecode") != 0)
        {
            [temp addObject:@(tunes[i])];
        }
    }

    return [temp copy];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingProfiles
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (NSArray<NSString *> *)profiles
{
    NSMutableArray<NSString *> *temp = [NSMutableArray array];

    const char * const *profiles = hb_video_encoder_get_profiles(self.encoder);
    for (int i = 0; profiles != NULL && profiles[i] != NULL; i++)
    {
        [temp addObject:@(profiles[i])];
    }
    if (!temp.count)
    {
        [temp addObject:@"auto"];
    }

    return [temp copy];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingLevels
{
    return [NSSet setWithObjects:@"encoder", nil];
}

- (NSArray<NSString *> *)levels
{
    NSMutableArray<NSString *> *temp = [NSMutableArray array];

    const char * const *levels = hb_video_encoder_get_levels(self.encoder);
    for (int i = 0; levels != NULL && levels[i] != NULL; i++)
    {
        [temp addObject:@(levels[i])];
    }
    if (!temp.count)
    {
        [temp addObject:@"auto"];
    }

    return [temp copy];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBVideo *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_encoder = _encoder;

        copy->_qualityType = _qualityType;
        copy->_avgBitrate = _avgBitrate;
        copy->_quality = _quality;

        copy->_qualityMinValue = _qualityMinValue;
        copy->_qualityMaxValue = _qualityMaxValue;

        copy->_frameRate = _frameRate;
        copy->_frameRateMode = _frameRateMode;

        copy->_twoPass = _twoPass;
        copy->_turboTwoPass = _turboTwoPass;

        copy->_preset = [_preset copy];
        copy->_tune = [_tune copy];
        copy->_profile = [_profile copy];
        copy->_level = [_level copy];
        copy->_videoOptionExtra = [_videoOptionExtra copy];
        copy->_fastDecode = _fastDecode;

        copy->_notificationsEnabled = _notificationsEnabled;
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
    [coder encodeInt:2 forKey:@"HBVideoVersion"];

    encodeInt(_encoder);

    encodeInteger(_qualityType);
    encodeInt(_avgBitrate);
    encodeDouble(_quality);

    encodeDouble(_qualityMinValue);
    encodeDouble(_qualityMaxValue);

    encodeInt(_frameRate);
    encodeInteger(_frameRateMode);

    encodeBool(_twoPass);
    encodeBool(_turboTwoPass);

    encodeObject(_preset);
    encodeObject(_tune);
    encodeObject(_profile);
    encodeObject(_level);

    encodeObject(_videoOptionExtra);

    encodeBool(_fastDecode);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_encoder);

    decodeInteger(_qualityType); if (_qualityType < HBVideoQualityTypeAvgBitrate || _qualityType > HBVideoQualityTypeConstantQuality) { goto fail; }
    decodeInt(_avgBitrate); if (_avgBitrate < 0) { goto fail; }
    decodeDouble(_quality);

    decodeDouble(_qualityMinValue);
    decodeDouble(_qualityMaxValue);

    decodeInt(_frameRate); if (_frameRate < 0) { goto fail; }
    decodeInteger(_frameRateMode); if (_frameRateMode < HBVideoFrameRateModeVFR_PFR || _frameRateMode > HBVideoFrameRateModeCFR) { goto fail; }
    decodeBool(_twoPass);
    decodeBool(_turboTwoPass);

    decodeObjectOrFail(_preset, NSString);
    decodeObjectOrFail(_tune, NSString);
    decodeObjectOrFail(_profile, NSString);
    decodeObjectOrFail(_level, NSString);

    decodeObjectOrFail(_videoOptionExtra, NSString);

    decodeBool(_fastDecode);

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
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
 *  Returns the tune string plus the fastdecode option (if enabled)
 */
- (NSString *)completeTune
{
    NSMutableString *string = [[NSMutableString alloc] init];

    if (self.tune && ![self.tune isEqualToString:@"none"])
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

    return [string copy];
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    self.notificationsEnabled = NO;

    // map legacy encoder names via libhb.
    self.encoder = hb_video_encoder_get_from_name([preset[@"VideoEncoder"] UTF8String]);

    if (hb_video_encoder_get_presets(self.encoder) != NULL)
    {
        if ((self.encoder & HB_VCODEC_X264_MASK) && [preset[@"x264UseAdvancedOptions"] boolValue])
        {
            // preset does not use the x264 preset system, reset the widgets.
            self.preset = @"medium";
            self.tune = @"";
            self.profile = @"auto";
            self.level = @"auto";
            self.fastDecode = NO;

            self.videoOptionExtra = preset[@"VideoOptionExtra"];
        }
        else
        {
            // x264UseAdvancedOptions is set to 0 (disabled),
            // so we use the new preset system and
            // disable the advanced panel

            self.preset = preset[@"VideoPreset"];
            self.tune   = preset[@"VideoTune"];
            self.videoOptionExtra = preset[@"VideoOptionExtra"];
            self.profile = preset[@"VideoProfile"];
            self.level   = preset[@"VideoLevel"];

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
        self.videoOptionExtra = preset[@"VideoOptionExtra"];
    }

    int qualityType = [preset[@"VideoQualityType"] intValue] - 1;
    /* Note since the removal of Target Size encoding, the possible values for VideoQuality type are 0 - 1.
     * Therefore any preset that uses the old 2 for Constant Quality would now use 1 since there is one less index
     * for the fVidQualityMatrix. It should also be noted that any preset that used the deprecated Target Size
     * setting of 0 would set us to 0 or ABR since ABR is now tagged 0. Fortunately this does not affect any built-in
     * presets since they all use Constant Quality or Average Bitrate.*/
    if (qualityType == -1 || hb_video_quality_is_supported(self.encoder) == 0)
    {
        qualityType = 0;
    }
    self.qualityType = qualityType;

    self.avgBitrate = [preset[@"VideoAvgBitrate"] intValue];
    self.quality = [preset[@"VideoQualitySlider"] floatValue];

    // Video framerate
    if ([preset[@"VideoFramerate"] isEqualToString:@"auto"])
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
    self.twoPass = [preset[@"VideoTwoPass"] boolValue];

    // Turbo 1st pass for 2 Pass Encoding.
    self.turboTwoPass = [preset[@"VideoTurboTwoPass"] boolValue];

    self.notificationsEnabled = YES;
}

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset[@"VideoEncoder"] = @(hb_video_encoder_get_short_name(self.encoder));

    if (hb_video_encoder_get_presets(self.encoder) != NULL)
    {
        preset[@"VideoPreset"]      = self.preset;
        preset[@"VideoTune"]        = [self completeTune];
        preset[@"VideoOptionExtra"] = self.videoOptionExtra;
        preset[@"VideoProfile"]     = self.profile;
        preset[@"VideoLevel"]       = self.level;
        preset[@"x264UseAdvancedOptions"] = @NO;
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
        preset[@"VideoFramerate"] = @"auto";

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

@end
