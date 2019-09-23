/*  HBAudioTrackPreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTrackPreset.h"
#import "HBCodingUtilities.h"
#include "handbrake/handbrake.h"

#define DEFAULT_SAMPLERATE 48000

@interface HBAudioTrackPreset ()

@property (nonatomic, readwrite) int container;
@property (nonatomic, readwrite) int selectedEncoder;

@end

@implementation HBAudioTrackPreset

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // defaults settings
        _encoder = HB_ACODEC_CA_AAC;
        _selectedEncoder = HB_ACODEC_INVALID;
        _container = HB_MUX_MKV;
        _sampleRate = 0;
        _bitRate = 160;
        _mixdown = HB_AMIXDOWN_DOLBYPLII;
    }
    return self;
}

- (instancetype)initWithContainer:(int)container
{
    self = [self init];
    if (self)
    {
        _container = container;
    }
    return self;
}

- (void)containerChanged:(int)container
{
    // Do things here
}

#pragma mark - Setters override

- (void)setEncoder:(int)encoder
{
    if (encoder != _encoder)
    {
        [[self.undo prepareWithInvocationTarget:self] setEncoder:_encoder];
    }
    _encoder = encoder;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateFallbackEncoder];
    }
}

- (void)setFallbackEncoder:(int)fallbackEncoder
{
    if (fallbackEncoder != _fallbackEncoder)
    {
        [[self.undo prepareWithInvocationTarget:self] setFallbackEncoder:_fallbackEncoder];
    }
    _fallbackEncoder = fallbackEncoder;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateFallbackEncoder];
    }
}

- (void)setSelectedEncoder:(int)fallbackEncoder
{
    if (fallbackEncoder != _selectedEncoder)
    {
        [[self.undo prepareWithInvocationTarget:self] setFallbackEncoder:_fallbackEncoder];
    }
    _selectedEncoder = fallbackEncoder;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateMixdown];
        [self validateSamplerate];
        [self validateBitrate];
    }
}

- (void)setMixdown:(int)mixdown
{
    if (mixdown == HB_AMIXDOWN_NONE)
    {
        mixdown = hb_mixdown_get_default(self.selectedEncoder, 0);
    }

    if (mixdown != _mixdown)
    {
        [[self.undo prepareWithInvocationTarget:self] setMixdown:_mixdown];
    }
    _mixdown = mixdown;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateBitrate];
    }
}

- (void)setSampleRate:(int)sampleRate
{
    if (sampleRate != _sampleRate)
    {
        [[self.undo prepareWithInvocationTarget:self] setSampleRate:_sampleRate];
    }
    _sampleRate = sampleRate;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateBitrate];
    }
}

- (void)setBitRate:(int)bitRate
{
    if (bitRate != _bitRate)
    {
        [[self.undo prepareWithInvocationTarget:self] setBitRate:_bitRate];
    }
    _bitRate = bitRate;
}

#pragma mark -
#pragma mark Validation

/**
 If the encoder is a passthru, return its fallback if available
 to make possible to set the fallback settings.
 */
- (void)validateFallbackEncoder
{
    if (_encoder & HB_ACODEC_PASS_FLAG)
    {
        int fallbackEncoder =  hb_audio_encoder_get_fallback_for_passthru(_encoder);
        self.selectedEncoder = (fallbackEncoder != HB_ACODEC_INVALID) ? fallbackEncoder : self.fallbackEncoder;
    }
    else
    {
        self.selectedEncoder = self.encoder;
    }
}

- (void)validateMixdown
{
    if (!hb_mixdown_has_codec_support(self.mixdown, self.selectedEncoder))
    {
        self.mixdown = hb_mixdown_get_default(self.selectedEncoder, 0);
    }
}

- (void)validateSamplerate
{
    if (self.selectedEncoder & HB_ACODEC_PASS_FLAG)
    {
        self.sampleRate = 0; // Auto (same as source)
    }
    else if (self.sampleRate)
    {
        self.sampleRate = hb_audio_samplerate_find_closest(self.sampleRate, self.selectedEncoder);
    }
}

- (void)validateBitrate
{
    if (self.selectedEncoder & HB_ACODEC_PASS_FLAG)
    {
        self.bitRate = -1;
    }
    else if (self.bitRate == -1) // switching from passthru
    {
        self.bitRate = hb_audio_bitrate_get_default(self.selectedEncoder,
                                                    self.sampleRate ? self.sampleRate : DEFAULT_SAMPLERATE,
                                                    self.mixdown);
    }
    else
    {
        self.bitRate = hb_audio_bitrate_get_best(self.selectedEncoder, self.bitRate,
                                                 self.sampleRate ? self.sampleRate : DEFAULT_SAMPLERATE,
                                                 self.mixdown);
    }
}

- (BOOL)isAutoPassthruEnabledWithNoFallback
{
    return (self.encoder == HB_ACODEC_AUTO_PASS && self.fallbackEncoder == HB_ACODEC_NONE);
}

- (BOOL)mixdownEnabled
{
    BOOL retval = YES;

    if (self.mixdown == HB_AMIXDOWN_NONE || self.isAutoPassthruEnabledWithNoFallback)
    {
        // "None" mixdown (passthru)
        retval = NO;
    }

    return retval;
}

- (BOOL)bitrateEnabled
{
    BOOL retval = YES;

    int myCodecDefaultBitrate = hb_audio_bitrate_get_default(self.selectedEncoder, 0, 0);
    if (myCodecDefaultBitrate < 0 || self.isAutoPassthruEnabledWithNoFallback)
    {
        retval = NO;
    }
    return retval;
}

- (BOOL)passThruDisabled
{
    BOOL retval = YES;

    if (self.selectedEncoder & HB_ACODEC_PASS_FLAG || self.isAutoPassthruEnabledWithNoFallback)
    {
        retval = NO;
    }

    return retval;
}

- (void)setGain:(double)gain
{
    if (gain != _gain)
    {
        [[self.undo prepareWithInvocationTarget:self] setGain:_gain];
    }
    _gain = gain;
}

// Because we have indicated that the binding for the gain validates immediately we can implement the
// key value binding method to ensure the gain stays in our accepted range.
- (BOOL)validateGain:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        if ([*ioValue intValue] < -20)
        {
            *ioValue = @(-20);
        }
        else if ([*ioValue intValue] > 20)
        {
            *ioValue = @20;
        }
    }

    return retval;
}

- (void)setDrc:(double)drc
{
    if (drc != _drc)
    {
        [[self.undo prepareWithInvocationTarget:self] setDrc:_drc];
    }
    _drc = drc;
}

#pragma mark - Options

- (NSArray<NSString *> *)encoders
{
    NSMutableArray<NSString *> *encoders = [[NSMutableArray alloc] init];
    for (const hb_encoder_t *audio_encoder = hb_audio_encoder_get_next(NULL);
         audio_encoder != NULL;
         audio_encoder  = hb_audio_encoder_get_next(audio_encoder))
    {
        if (audio_encoder->codec != HB_ACODEC_NONE)
        {
            [encoders addObject:@(audio_encoder->name)];
        }
    }
    return encoders;
}

- (NSArray<NSString *> *)mixdowns
{
    NSMutableArray<NSString *> *mixdowns = [[NSMutableArray alloc] init];
    for (const hb_mixdown_t *mixdown = hb_mixdown_get_next(NULL);
         mixdown != NULL;
         mixdown  = hb_mixdown_get_next(mixdown))
    {
        if (hb_mixdown_has_codec_support(mixdown->amixdown, self.selectedEncoder))
        {
            [mixdowns addObject:@(mixdown->name)];
        }
    }
    return mixdowns;
}

- (NSArray<NSString *> *)sampleRates
{
    NSMutableArray<NSString *> *sampleRates = [[NSMutableArray alloc] init];
    [sampleRates addObject:@"Auto"];

    for (const hb_rate_t *audio_samplerate = hb_audio_samplerate_get_next(NULL);
         audio_samplerate != NULL;
         audio_samplerate  = hb_audio_samplerate_get_next(audio_samplerate))
    {
        int rate = audio_samplerate->rate;
        if (rate == hb_audio_samplerate_find_closest(rate, self.selectedEncoder))
        {
            [sampleRates addObject:@(audio_samplerate->name)];
        }
    }
    return sampleRates;
}

- (NSArray<NSString *> *)bitRates
{
    int minBitRate = 0;
    int maxBitRate = 0;

    hb_audio_bitrate_get_limits(self.selectedEncoder, self.sampleRate, self.mixdown, &minBitRate, &maxBitRate);

    NSMutableArray<NSString *> *bitRates = [[NSMutableArray alloc] init];
    for (const hb_rate_t *audio_bitrate = hb_audio_bitrate_get_next(NULL);
         audio_bitrate != NULL;
         audio_bitrate  = hb_audio_bitrate_get_next(audio_bitrate))
    {
        if (audio_bitrate->rate >= minBitRate && audio_bitrate->rate <= maxBitRate)
        {
            [bitRates addObject:@(audio_bitrate->name)];
        }
    }
    return bitRates;
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    // Tell KVO to reload the *enabled keyPaths
    // after a change to encoder.
    if ([key isEqualToString:@"bitrateEnabled"] ||
        [key isEqualToString:@"passThruDisabled"] ||
        [key isEqualToString:@"mixdownEnabled"])
    {
        retval = [NSSet setWithObjects:@"selectedEncoder", @"encoder", @"fallbackEncoder", @"mixdown", @"sampleRate", nil];
    }
    else if ([key isEqualToString:@"mixdowns"])
    {
        retval = [NSSet setWithObjects:@"selectedEncoder", @"encoder", @"fallbackEncoder", nil];
    }
    else if ([key isEqualToString:@"sampleRates"])
    {
        retval = [NSSet setWithObjects:@"selectedEncoder", @"encoder", @"fallbackEncoder", @"mixdown", nil];
    }
    else if ([key isEqualToString:@"bitRates"])
    {
        retval = [NSSet setWithObjects:@"selectedEncoder", @"encoder", @"fallbackEncoder", @"mixdown", @"sampleRate", nil];
    }
    else
    {
        retval = [NSSet set];
    }

    return retval;
}

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"drc"] || [key isEqualToString:@"gain"])
    {
        [self setValue:@0 forKey:key];
    }
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudioTrackPreset *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_encoder = _encoder;
        copy->_fallbackEncoder = _fallbackEncoder;
        copy->_selectedEncoder = _selectedEncoder;
        copy->_mixdown = _mixdown;
        copy->_sampleRate = _sampleRate;
        copy->_bitRate = _bitRate;

        copy->_gain = _gain;
        copy->_drc = _drc;

        copy->_container = _container;
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
    [coder encodeInt:1 forKey:@"HBAudioTrackPresetVersion"];

    encodeInt(_encoder);
    encodeInt(_fallbackEncoder);
    encodeInt(_mixdown);
    encodeInt(_sampleRate);
    encodeInt(_bitRate);

    encodeDouble(_gain);
    encodeDouble(_drc);

    encodeInt(_container);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_encoder); if (_encoder < 0) { goto fail; }
    decodeInt(_fallbackEncoder); if (_fallbackEncoder < 0) { goto fail; }
    decodeInt(_mixdown); if (_mixdown < 0) { goto fail; }
    decodeInt(_sampleRate); if (_sampleRate < 0) { goto fail; }
    decodeInt(_bitRate); if (_bitRate < -1) { goto fail; }

    decodeDouble(_gain);
    decodeDouble(_drc);

    decodeInt(_container); if (_container != HB_MUX_MP4 && _container != HB_MUX_MKV && _container != HB_MUX_WEBM) { goto fail; }

    [self validateFallbackEncoder];

    return self;

fail:
    return nil;
}

@end
