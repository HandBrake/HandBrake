/*  HBAudioTrackPreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTrackPreset.h"
#import "HBCodingUtilities.h"
#include "hb.h"

#define DEFAULT_SAMPLERATE 48000

static void *HBAudioEncoderContex = &HBAudioEncoderContex;

@interface HBAudioTrackPreset ()

@property (nonatomic, readwrite) int container;

@end

@implementation HBAudioTrackPreset

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // defaults settings
        _encoder = HB_ACODEC_CA_AAC;
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
    _encoder = encoder;
    [self validateMixdown];
    [self validateBitrate];
}

- (void)setMixdown:(int)mixdown
{
    _mixdown = mixdown;
    [self validateBitrate];
}

- (void)setSampleRate:(int)sampleRate
{
    _sampleRate = sampleRate;
    [self validateBitrate];
}

#pragma mark -
#pragma mark Validation

/**
 *  Validates the mixdown property.
 */
- (void)validateMixdown
{
    if (!hb_mixdown_has_codec_support(self.mixdown, self.encoder))
    {
        self.mixdown = hb_mixdown_get_default(self.encoder, 0);
    }
}

- (void)validateBitrate
{
    self.bitRate = hb_audio_bitrate_get_best(self.encoder, self.bitRate, self.sampleRate, self.mixdown);
}

- (BOOL)mixdownEnabled
{
    BOOL retval = YES;

    if (self.mixdown == HB_AMIXDOWN_NONE)
    {
        // "None" mixdown (passthru)
        retval = NO;
    }

    return retval;
}

- (BOOL)bitrateEnabled
{
    BOOL retval = YES;

    int myCodecDefaultBitrate = hb_audio_bitrate_get_default(self.encoder, 0, 0);
    if (myCodecDefaultBitrate < 0)
    {
        retval = NO;
    }
    return retval;
}

- (BOOL)passThruDisabled
{
    BOOL retval = YES;

    if (self.encoder & HB_ACODEC_PASS_FLAG)
    {
        retval = NO;
    }

    return retval;
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

#pragma mark - Options

- (NSArray *)encoders
{
    NSMutableArray *encoders = [[NSMutableArray alloc] init];
    for (const hb_encoder_t *audio_encoder = hb_audio_encoder_get_next(NULL);
         audio_encoder != NULL;
         audio_encoder  = hb_audio_encoder_get_next(audio_encoder))
    {
        [encoders addObject:@(audio_encoder->name)];
    }
    return encoders;
}

- (NSArray *)mixdowns
{
    NSMutableArray *mixdowns = [[NSMutableArray alloc] init];
    for (const hb_mixdown_t *mixdown = hb_mixdown_get_next(NULL);
         mixdown != NULL;
         mixdown  = hb_mixdown_get_next(mixdown))
    {
        if (hb_mixdown_has_codec_support(mixdown->amixdown, self.encoder))
        {
            [mixdowns addObject:@(mixdown->name)];
        }
    }
    return mixdowns;
}

- (NSArray *)samplerates
{
    NSMutableArray *samplerates = [[NSMutableArray alloc] init];
    for (const hb_rate_t *audio_samplerate = hb_audio_samplerate_get_next(NULL);
         audio_samplerate != NULL;
         audio_samplerate  = hb_audio_samplerate_get_next(audio_samplerate))
    {
        [samplerates addObject:@(audio_samplerate->name)];
    }
    return samplerates;
}

- (NSArray *)bitrates
{
    int minBitRate = 0;
    int maxBitRate = 0;

    hb_audio_bitrate_get_limits(self.encoder, self.sampleRate, self.mixdown, &minBitRate, &maxBitRate);

    NSMutableArray *bitrates = [[NSMutableArray alloc] init];
    for (const hb_rate_t *audio_bitrate = hb_audio_bitrate_get_next(NULL);
         audio_bitrate != NULL;
         audio_bitrate  = hb_audio_bitrate_get_next(audio_bitrate))
    {
        if (audio_bitrate->rate >= minBitRate && audio_bitrate->rate <= maxBitRate)
        {
            [bitrates addObject:@(audio_bitrate->name)];
        }
    }
    return bitrates;
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    // Tell KVO to reaload the *enabled keyPaths
    // after a change to encoder.
    if ([key isEqualToString:@"bitrateEnabled"] ||
        [key isEqualToString:@"passThruDisabled"] ||
        [key isEqualToString:@"mixdownEnabled"])
    {
        retval = [NSSet setWithObjects:@"encoder", nil];
    }

    return retval;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudioTrackPreset *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_encoder = _encoder;
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

    decodeInt(_encoder);
    decodeInt(_mixdown);
    decodeInt(_sampleRate);
    decodeInt(_bitRate);

    decodeDouble(_gain);
    decodeDouble(_drc);

    decodeInt(_container);

    return self;
}

@end

#pragma mark - Value Trasformers

@implementation HBEncoderTrasformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_audio_encoder_get_name([value intValue]);
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
    return @(hb_audio_encoder_get_from_name([value UTF8String]));
}

@end

@implementation HBMixdownTrasformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_mixdown_get_name([value intValue]);
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
    return @(hb_mixdown_get_from_name([value UTF8String]));
}

@end

@implementation HBSampleRateTrasformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    const char *name = hb_audio_samplerate_get_name([value intValue]);
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
    int sampleRate = hb_audio_samplerate_get_from_name([value UTF8String]);
    if (sampleRate < 0)
    {
        sampleRate = 0;
    }
    return @(sampleRate);
}

@end

@implementation HBIntegerTrasformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    return [value stringValue];
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    return @([value intValue]);
}

@end
