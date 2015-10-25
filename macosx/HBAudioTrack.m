/*  HBAudioTrack.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTrack.h"
#import "HBAudioController.h"
#import "HBJob.h"
#import "HBCodingUtilities.h"
#import "hb.h"

NSString *keyAudioTrackIndex = @"keyAudioTrackIndex";
NSString *keyAudioTrackName = @"keyAudioTrackName";
NSString *keyAudioInputBitrate = @"keyAudioInputBitrate";
NSString *keyAudioInputSampleRate = @"keyAudioInputSampleRate";
NSString *keyAudioInputCodec = @"keyAudioInputCodec";
NSString *keyAudioInputCodecParam = @"keyAudioInputCodecParam";
NSString *keyAudioInputChannelLayout = @"keyAudioInputChannelLayout";
NSString *keyAudioTrackLanguageIsoCode = @"keyAudioTrackLanguageIsoCode";

NSString *keyAudioCodecName = @"keyAudioCodecName";
NSString *keyAudioSupportedMuxers = @"keyAudioSupportedMuxers";
NSString *keyAudioSampleRateName = @"keyAudioSampleRateName";
NSString *keyAudioBitrateName = @"keyAudioBitrateName";
NSString *keyAudioMustMatchTrack = @"keyAudioMustMatchTrack";
NSString *keyAudioMixdownName = @"keyAudioMixdownName";

NSString *keyAudioCodec = @"codec";
NSString *keyAudioMixdown = @"mixdown";
NSString *keyAudioSamplerate = @"samplerate";
NSString *keyAudioBitrate = @"bitrate";

static NSMutableArray *masterCodecArray = nil;
static NSMutableArray *masterMixdownArray = nil;
static NSMutableArray *masterBitRateArray = nil;

@interface NSArray (HBAudioSupport)
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey;
- (NSDictionary *) lastDictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey;
@end
@implementation NSArray (HBAudioSupport)
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey reverse: (BOOL) reverse
{
    NSDictionary *retval = nil;
    NSEnumerator *enumerator = reverse ? [self reverseObjectEnumerator] : [self objectEnumerator];
    NSDictionary *dict;
    id aValue;

    while (nil != (dict = [enumerator nextObject]) && !retval)
    {
        if (nil != (aValue = dict[aKey]) && [aValue isEqual: anObject])
        {
            retval = dict;
        }
    }
    return retval;
}
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{
    return [self dictionaryWithObject: anObject matchingKey: aKey reverse: NO];
}
- (NSDictionary *) lastDictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{
    return [self dictionaryWithObject: anObject matchingKey: aKey reverse: YES];
}

@end

@interface HBAudioTrack ()

@property (nonatomic, readwrite) NSArray *codecs;
@property (nonatomic, readwrite) NSArray *mixdowns;
@property (nonatomic, readwrite) NSArray *bitRates;

@end

@implementation HBAudioTrack

#pragma mark -
#pragma mark Object Setup

+ (void)initialize
{
    if ([HBAudioTrack class] == self)
    {
        masterCodecArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (const hb_encoder_t *audio_encoder = hb_audio_encoder_get_next(NULL);
             audio_encoder != NULL;
             audio_encoder  = hb_audio_encoder_get_next(audio_encoder))
        {
            id audioMustMatchTrack;
            if ((audio_encoder->codec &  HB_ACODEC_PASS_FLAG) &&
                (audio_encoder->codec != HB_ACODEC_AUTO_PASS))
            {
                audioMustMatchTrack = @(audio_encoder->codec &
                                                      ~HB_ACODEC_PASS_FLAG);
            }
            else
            {
                audioMustMatchTrack = @NO;
            }
            [masterCodecArray addObject:@{keyAudioCodecName:       @(audio_encoder->name),
                                          keyAudioCodec:           @(audio_encoder->codec),
                                          keyAudioSupportedMuxers: @(audio_encoder->muxers),
                                          keyAudioMustMatchTrack:    audioMustMatchTrack}];
        }

        masterMixdownArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (const hb_mixdown_t *mixdown = hb_mixdown_get_next(NULL);
             mixdown != NULL;
             mixdown  = hb_mixdown_get_next(mixdown))
        {
            [masterMixdownArray addObject:@{keyAudioMixdownName: @(mixdown->name),
                                            keyAudioMixdown:     @(mixdown->amixdown)}];
        }

        masterBitRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (const hb_rate_t *audio_bitrate = hb_audio_bitrate_get_next(NULL);
             audio_bitrate != NULL;
             audio_bitrate  = hb_audio_bitrate_get_next(audio_bitrate))
        {
            [masterBitRateArray addObject:@{keyAudioBitrateName: @(audio_bitrate->name),
                                            keyAudioBitrate:     @(audio_bitrate->rate)}];
        }
    }
}

// Ensure the list of codecs is accurate
// Update the current value of codec based on the revised list
- (void) updateCodecs
{
    NSMutableArray *permittedCodecs = [NSMutableArray array];
    NSUInteger count = [masterCodecArray count];
    NSDictionary *dict;

    // First get a list of the permitted codecs based on the internal rules
    if (nil != self.track && self.enabled)
    {
        BOOL goodToAdd;

        for (unsigned int i = 0; i < count; i++)
        {
            dict = masterCodecArray[i];

            // First make sure only codecs permitted by the container are here
            goodToAdd = !!([dict[keyAudioSupportedMuxers] intValue] & self.container);

            // Now we make sure if DTS or AC3 is not available in the track it is not put in the codec list, but in a general way
            if ([dict[keyAudioMustMatchTrack] boolValue])
            {
                if ([dict[keyAudioMustMatchTrack] intValue] != [self.track[keyAudioInputCodec] intValue])
                {
                    goodToAdd = NO;
                }
            }

            if (goodToAdd)
            {
                [permittedCodecs addObject: dict];
            }
        }
    }

    // Now make sure the permitted list and the actual ones matches
    [self setCodecs: permittedCodecs];

    // Ensure our codec is on the list of permitted codecs
    if (!self.codec || ![permittedCodecs containsObject: self.codec])
    {
        if (0 < [permittedCodecs count])
        {
            self.codec = permittedCodecs[0]; // This should be defaulting to Core Audio
        }
        else
        {
            self.codec = nil;
        }
    }
}

- (void)updateMixdowns:(BOOL)shouldSetDefault
{
    NSMutableArray *permittedMixdowns = [NSMutableArray array];
    NSDictionary *dict;
    int currentMixdown;

    unsigned long long channelLayout = [self.track[keyAudioInputChannelLayout] unsignedLongLongValue];
    NSUInteger count                 = [masterMixdownArray count];
    int codecCodec                   = [self.codec[keyAudioCodec] intValue];
    int theDefaultMixdown            = hb_mixdown_get_default(codecCodec, channelLayout);

    for (unsigned int i = 0; i < count; i++)
    {
        dict = masterMixdownArray[i];
        currentMixdown = [dict[keyAudioMixdown] intValue];

        if (hb_mixdown_is_supported(currentMixdown, codecCodec, channelLayout))
        {
            [permittedMixdowns addObject: dict];
        }
    }

    if (!self.enabled)
    {
        permittedMixdowns = nil;
    }

    // Now make sure the permitted list and the actual ones matches
    self.mixdowns = permittedMixdowns;

    // Select the proper one
    if (shouldSetDefault)
    {
        self.mixdown = [permittedMixdowns dictionaryWithObject: @(theDefaultMixdown)
                                                      matchingKey: keyAudioMixdown];
    }

    if (!self.mixdown || ![permittedMixdowns containsObject: self.mixdown])
    {
        self.mixdown = [permittedMixdowns lastObject];
    }
}

- (void)validateSamplerate
{
    int codec      = [self.codec[keyAudioCodec] intValue];
    int samplerate = [self.sampleRate[keyAudioSamplerate] intValue];

    if (codec & HB_ACODEC_PASS_FLAG)
    {
        [self setSampleRateFromName:@"Auto"];
    }
    else if (samplerate)
    {
        samplerate = hb_audio_samplerate_get_best(codec, samplerate, NULL);
        [self setSampleRateFromName:@(hb_audio_samplerate_get_name(samplerate))];
    }
}

- (void)updateBitRates:(BOOL)shouldSetDefault
{
    NSMutableArray *permittedBitRates = [NSMutableArray array];
    NSDictionary *dict;
    int minBitRate;
    int maxBitRate;
    int currentBitRate;
    BOOL shouldAdd;

    NSUInteger count = [masterBitRateArray count];
    int trackInputBitRate = [self.track[keyAudioInputBitrate] intValue];
    int theSampleRate = [self.sampleRate[keyAudioSamplerate] intValue];

    if (0 == theSampleRate) // this means Auto
    {
        theSampleRate = [self.track[keyAudioInputSampleRate] intValue];
    }

    int ourCodec          = [self.codec[keyAudioCodec] intValue];
    int ourMixdown        = [self.mixdown[keyAudioMixdown] intValue];
    int theDefaultBitRate = hb_audio_bitrate_get_default(ourCodec, theSampleRate, ourMixdown);
    hb_audio_bitrate_get_limits(ourCodec, theSampleRate, ourMixdown, &minBitRate, &maxBitRate);

    BOOL codecIsPassthru = ([self.codec[keyAudioCodec] intValue] & HB_ACODEC_PASS_FLAG) ? YES : NO;
    BOOL codecIsLossless = (theDefaultBitRate == -1) ? YES : NO;

    if (codecIsPassthru)
    {
        NSDictionary *sourceBitRate = [masterBitRateArray dictionaryWithObject: @(trackInputBitRate)
                                                                   matchingKey: keyAudioBitrate];
        if (!sourceBitRate)
        {
            // the source bitrate isn't in the master array - create it
            sourceBitRate = @{keyAudioBitrateName: [NSString stringWithFormat: @"%d", trackInputBitRate],
                              keyAudioBitrate: @(trackInputBitRate)};
        }
        [permittedBitRates addObject: sourceBitRate];
    }
    else if (codecIsLossless)
    {
        NSDictionary *bitRateNotApplicable = @{keyAudioBitrateName: @"N/A",
                                               keyAudioBitrate: @-1};
        [permittedBitRates addObject: bitRateNotApplicable];
    }
    else
    {
        for (unsigned int i = 0; i < count; i++)
        {
            dict = masterBitRateArray[i];
            currentBitRate = [dict[keyAudioBitrate] intValue];

            // First ensure the bitrate falls within range of the codec
            shouldAdd = (currentBitRate >= minBitRate && currentBitRate <= maxBitRate);

            if (shouldAdd)
            {
                [permittedBitRates addObject: dict];
            }
        }
    }

    if (!self.enabled)
    {
        permittedBitRates = nil;
    }

    // Make sure we are updated with the permitted list
    self.bitRates = permittedBitRates;

    // Select the proper one
    if (shouldSetDefault)
    {
        [self setBitRateFromName: [NSString stringWithFormat:@"%d", theDefaultBitRate]];
    }

    if (!self.bitRate || ![permittedBitRates containsObject: self.bitRate])
    {
        self.bitRate = [permittedBitRates lastObject];
    }
}

#pragma mark -
#pragma mark Accessors

- (NSArray *)sampleRates
{
    NSMutableArray *samplerates = [[NSMutableArray alloc] init];

    /*
     * Note that for the Auto value we use 0 for the sample rate because our controller will give back the track's
     * input sample rate when it finds this 0 value as the selected sample rate.  We do this because the input
     * sample rate depends on the track, which means it depends on the title, so cannot be nicely set up here.
     */
    [samplerates addObject:@{keyAudioSampleRateName: @"Auto",
                             keyAudioSamplerate:     @0}];

    int codec = [self.codec[keyAudioCodec] intValue];
    for (const hb_rate_t *audio_samplerate = hb_audio_samplerate_get_next(NULL);
         audio_samplerate != NULL;
         audio_samplerate  = hb_audio_samplerate_get_next(audio_samplerate))
    {
        int rate = audio_samplerate->rate;
        if (rate == hb_audio_samplerate_get_best(codec, rate, NULL))
        {
            [samplerates addObject:@{keyAudioSampleRateName: @(audio_samplerate->name),
                                     keyAudioSamplerate:     @(rate)}];
        }
    }
    return samplerates;
}

#pragma mark -
#pragma mark Setters

- (void)setContainer:(int)container
{
    if (container != _container)
    {
        [[self.undo prepareWithInvocationTarget:self] setContainer:_container];
    }
    _container = container;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self updateCodecs];
    }
}

- (void)setTrack:(NSDictionary *)track
{
    if (track != _track)
    {
        [[self.undo prepareWithInvocationTarget:self] setTrack:_track];
    }
    NSDictionary *oldValue = _track;
    _track = track;
    if (nil != _track && !(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self updateCodecs];
        [self updateMixdowns: YES];
        if (self.enabled)
        {
            self.sampleRate = self.sampleRates[0]; // default to Auto
        }
        if ([self.dataSource.noneTrack isEqual: oldValue])
        {
            [self.delegate switchingTrackFromNone: self];
        }
        if ([self.dataSource.noneTrack isEqual: self.track])
        {
            [self.delegate settingTrackToNone: self];
        }
    }
}

- (void)setCodec:(NSDictionary *)codec
{
    if (codec != _codec)
    {
        [[self.undo prepareWithInvocationTarget:self] setCodec:_codec];
    }
    _codec = codec;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateSamplerate];
        [self updateMixdowns:YES];
        [self updateBitRates:YES];
    }
}

- (void)setMixdown:(NSDictionary *)mixdown
{
    if (mixdown != _mixdown)
    {
        [[self.undo prepareWithInvocationTarget:self] setMixdown:_mixdown];
    }
    _mixdown = mixdown;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self updateBitRates:YES];
        [self.delegate mixdownChanged];
    }
}

- (void)setSampleRate:(NSDictionary *)sampleRate
{
    if (sampleRate != _sampleRate)
    {
        [[self.undo prepareWithInvocationTarget:self] setSampleRate:_sampleRate];
    }
    _sampleRate = sampleRate;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self updateBitRates: NO];
    }
}

- (void)setBitRate:(NSDictionary *)bitRate
{
    if (bitRate != _bitRate)
    {
        [[self.undo prepareWithInvocationTarget:self] setBitRate:_bitRate];
    }
    _bitRate = bitRate;
}

- (void)setDrc:(double)drc
{
    if (drc != _drc)
    {
        [[self.undo prepareWithInvocationTarget:self] setDrc:_drc];
    }
    _drc = drc;
}

- (void)setGain:(double)gain
{
    if (gain != _gain)
    {
        [[self.undo prepareWithInvocationTarget:self] setGain:_gain];
    }
    _gain = gain;
}

#pragma mark -
#pragma mark Special Setters

- (void)setTrackFromIndex:(int)aValue
{
    self.track = [self.dataSource.masterTrackArray dictionaryWithObject: @(aValue)
                                                            matchingKey: keyAudioTrackIndex];
}

// This returns whether it is able to set the actual codec desired.
- (BOOL)setCodecFromName:(NSString *)aValue
{
    NSDictionary *dict = [self.codecs dictionaryWithObject: aValue matchingKey: keyAudioCodecName];
    if (nil != dict)
    {
        self.codec = dict;
    }
    return (nil != dict);
}

- (void)setMixdownFromName:(NSString *)aValue
{
    NSDictionary *dict = [self.mixdowns dictionaryWithObject: aValue matchingKey: keyAudioMixdownName];
    if (nil != dict)
    {
        self.mixdown = dict;
    }
}

- (void)setSampleRateFromName:(NSString *)aValue
{
    NSDictionary *dict = [self.sampleRates dictionaryWithObject: aValue matchingKey: keyAudioSampleRateName];
    if (nil != dict)
    {
        self.sampleRate = dict;
    }
}

- (void)setBitRateFromName:(NSString *)aValue
{
    NSDictionary *dict = [self.bitRates dictionaryWithObject: aValue matchingKey: keyAudioBitrateName];
    if (nil != dict)
    {
        self.bitRate = dict;
    }
}

- (void)setCodecs:(NSArray *)codecs
{
    if (codecs != _codecs)
    {
        [[self.undo prepareWithInvocationTarget:self] setCodecs:_codecs];
    }
    _codecs = codecs;
}

- (void)setMixdowns:(NSArray *)mixdowns
{
    if (mixdowns != _mixdowns)
    {
        [[self.undo prepareWithInvocationTarget:self] setMixdowns:_mixdowns];
    }
    _mixdowns = mixdowns;
}

- (void)setBitRates:(NSArray *)bitRates
{
    if (bitRates != _bitRates)
    {
        [[self.undo prepareWithInvocationTarget:self] setBitRates:_bitRates];
    }
    _bitRates = bitRates;
}

#pragma mark -
#pragma mark Validation

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

#pragma mark - Bindings Support

- (NSArray *)masterTrackArray
{
    return self.dataSource.masterTrackArray;
}

- (BOOL)enabled
{
    return (nil != self.track) ? (![self.track isEqual: self.dataSource.noneTrack]) : NO;
}

- (BOOL)mixdownEnabled
{
    BOOL retval = self.enabled;

    if (retval)
    {
        int myMixdown = [self.mixdown[keyAudioMixdown] intValue];
        if (myMixdown == HB_AMIXDOWN_NONE)
        {
            // "None" mixdown (passthru)
            retval = NO;
        }
    }
    return retval;
}

- (BOOL)bitrateEnabled
{
    BOOL retval = self.enabled;

    if (retval)
    {
        int myCodecCodec          = [self.codec[keyAudioCodec] intValue];
        int myCodecDefaultBitrate = hb_audio_bitrate_get_default(myCodecCodec, 0, 0);
        if (myCodecDefaultBitrate < 0)
        {
            retval = NO;
        }
    }
    return retval;
}

- (BOOL)DRCEnabled
{
    BOOL retval = self.enabled;

    if (retval)
    {
        int myTrackParam = [self.track[keyAudioInputCodecParam] intValue];
        int myTrackCodec = [self.track[keyAudioInputCodec] intValue];
        int myCodecCodec = [self.codec[keyAudioCodec] intValue];
        if (!hb_audio_can_apply_drc(myTrackCodec, myTrackParam, myCodecCodec))
        {
            retval = NO;
        }
    }
    return retval;
}

- (BOOL)PassThruDisabled
{
    BOOL retval = self.enabled;

    if (retval)
    {
        int myCodecCodec = [self.codec[keyAudioCodec] intValue];
        if (myCodecCodec & HB_ACODEC_PASS_FLAG)
        {
            retval = NO;
        }
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

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"enabled"])
    {
        retval = [NSSet setWithObjects:@"track", nil];
    }
    else if ([key isEqualToString:@"PassThruDisabled"])
    {
        retval = [NSSet setWithObjects:@"track", @"codec", nil];
    }
    else if ([key isEqualToString:@"DRCEnabled"])
    {
        retval = [NSSet setWithObjects:@"track", @"codec", nil];
    }
    else if ([key isEqualToString:@"bitrateEnabled"])
    {
        retval = [NSSet setWithObjects:@"track", @"codec", nil];
    }
    else if ([key isEqualToString:@"mixdownEnabled"])
    {
        retval = [NSSet setWithObjects:@"track", @"mixdown", nil];
    }
    return retval;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudioTrack *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_track = [_track copy];
        copy->_codec = [_codec copy];
        copy->_mixdown = [_mixdown copy];
        copy->_sampleRate = [_sampleRate copy];
        copy->_bitRate = [_bitRate copy];
        copy->_drc = _drc;
        copy->_gain = _gain;
        copy->_container = _container;

        copy->_codecs = [_codecs copy];
        copy->_mixdowns = [_mixdowns copy];
        copy->_bitRates = [_bitRates copy];
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
    [coder encodeInt:2 forKey:@"HBAudioTrackVersion"];

    encodeObject(_track);
    encodeObject(_codec);
    encodeObject(_mixdown);
    encodeObject(_sampleRate);
    encodeObject(_bitRate);
    encodeDouble(_drc);
    encodeDouble(_gain);
    encodeInt(_container);

    encodeObject(_codecs);
    encodeObject(_mixdowns);
    encodeObject(_bitRates);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeObject(_track, NSDictionary);
    decodeObject(_codec, NSDictionary);
    decodeObject(_mixdown, NSDictionary);
    decodeObject(_sampleRate, NSDictionary);
    decodeObject(_bitRate, NSDictionary);
    decodeDouble(_drc);
    decodeDouble(_gain);
    decodeInt(_container);

    decodeObject(_codecs, NSMutableArray);
    decodeObject(_mixdowns, NSMutableArray);
    decodeObject(_bitRates, NSArray);

    return self;
}

@end
