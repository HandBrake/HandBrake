/*  HBAudioTrack.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioTrack.h"
#import "HBAudioController.h"
#import "HBJob.h"
#import "NSCodingMacro.h"
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
static NSMutableArray *masterSampleRateArray = nil;
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

@implementation HBAudioTrack

#pragma mark -
#pragma mark Object Setup

+ (void) initialize

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

        // Note that for the Auto value we use 0 for the sample rate because our controller will give back the track's
        // input sample rate when it finds this 0 value as the selected sample rate.  We do this because the input
        // sample rate depends on the track, which means it depends on the title, so cannot be nicely set up here.
        masterSampleRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        [masterSampleRateArray addObject:@{keyAudioSampleRateName: @"Auto",
                                           keyAudioSamplerate:     @0}];
        for (const hb_rate_t *audio_samplerate = hb_audio_samplerate_get_next(NULL);
             audio_samplerate != NULL;
             audio_samplerate  = hb_audio_samplerate_get_next(audio_samplerate))
        {
            [masterSampleRateArray addObject:@{keyAudioSampleRateName: @(audio_samplerate->name),
                                               keyAudioSamplerate:     @(audio_samplerate->rate)}];
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
            goodToAdd = !!([dict[keyAudioSupportedMuxers] intValue] &
                           [self.videoContainerTag                           intValue]);

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

- (void) updateMixdowns: (BOOL) shouldSetDefault

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

- (void) updateBitRates: (BOOL) shouldSetDefault

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

- (NSArray *) sampleRates
{
    return masterSampleRateArray;
}

- (void) dealloc
{
    [_track release];
    [_codec release];
    [_mixdown release];
    [_sampleRate release];
    [_bitRate release];
    [_drc release];
    [_gain release];
    [_videoContainerTag release];
    [_codecs release];
    [_mixdowns release];
    [_bitRates release];

    [super dealloc];
}

#pragma mark -
#pragma mark Setters

- (void)setVideoContainerTag:(NSNumber *)videoContainerTag
{
    [_videoContainerTag autorelease];
    _videoContainerTag = [videoContainerTag retain];
    [self updateCodecs];
}

- (void)setTrack:(NSDictionary *)track
{
    NSDictionary *oldValue = _track;
    _track = [track retain];
    if (nil != _track)
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
    [oldValue release];
}

- (void)setCodec:(NSDictionary *)codec
{
    [_codec autorelease];
    _codec = [codec retain];
    [self updateMixdowns: YES];
    [self updateBitRates: YES];
}

- (void)setMixdown:(NSDictionary *)mixdown
{
    [_mixdown autorelease];
    _mixdown = [mixdown retain];
    [self updateBitRates: YES];
    [self.delegate mixdownChanged];
}

- (void)setSampleRate:(NSDictionary *)sampleRate
{
    [_sampleRate autorelease];
    _sampleRate = [sampleRate retain];
    [self updateBitRates: NO];
}

#pragma mark -
#pragma mark Special Setters

- (void) setTrackFromIndex: (int) aValue

{
    self.track = [self.dataSource.masterTrackArray dictionaryWithObject: @(aValue)
                                                            matchingKey: keyAudioTrackIndex];
}

// This returns whether it is able to set the actual codec desired.
- (BOOL) setCodecFromName: (NSString *) aValue

{
    NSDictionary *dict = [self.codecs dictionaryWithObject: aValue matchingKey: keyAudioCodecName];

    if (nil != dict)
    {
        self.codec = dict;
    }
    return (nil != dict);
}

- (void) setMixdownFromName: (NSString *) aValue

{
    NSDictionary *dict = [self.mixdowns dictionaryWithObject: aValue matchingKey: keyAudioMixdownName];

    if (nil != dict)
    {
        self.mixdown = dict;
    }
}

- (void) setSampleRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [self.sampleRates dictionaryWithObject: aValue matchingKey: keyAudioSampleRateName];

    if (nil != dict)
    {
        self.sampleRate = dict;
    }
}

- (void) setBitRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [self.bitRates dictionaryWithObject: aValue matchingKey: keyAudioBitrateName];

    if (nil != dict)
    {
        self.bitRate = dict;
    }
}


#pragma mark -
#pragma mark Validation

// Because we have indicated that the binding for the gain validates immediately we can implement the
// key value binding method to ensure the gain stays in our accepted range.
- (BOOL)validateGain:(id *)ioValue error:(NSError *)outError
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

#pragma mark -
#pragma mark Bindings Support

- (BOOL) enabled

{
    return (nil != self.track) ? (![self.track isEqual: self.dataSource.noneTrack]) : NO;
}

- (BOOL) mixdownEnabled

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

- (BOOL) bitrateEnabled

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

- (BOOL) DRCEnabled

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

- (BOOL) PassThruDisabled

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

+ (NSSet *) keyPathsForValuesAffectingValueForKey: (NSString *) key

{
    NSSet *retval = nil;

    if ([key isEqualToString: @"enabled"])
    {
        retval = [NSSet setWithObjects: @"track", nil];
    }
    else if ([key isEqualToString: @"PassThruDisabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"DRCEnabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"bitrateEnabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"codec", nil];
    }
    else if ([key isEqualToString: @"mixdownEnabled"])
    {
        retval = [NSSet setWithObjects: @"track", @"mixdown", nil];
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
        copy->_drc = [_drc copy];
        copy->_gain = [_gain copy];
        copy->_videoContainerTag = [_videoContainerTag copy];

        copy->_codecs = [_codecs copy];
        copy->_mixdowns = [_mixdowns copy];
        copy->_bitRates = [_bitRates copy];
    }

    return copy;
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBAudioTrackVersion"];

    encodeObject(_track);
    encodeObject(_codec);
    encodeObject(_mixdown);
    encodeObject(_sampleRate);
    encodeObject(_bitRate);
    encodeObject(_drc);
    encodeObject(_gain);
    encodeObject(_videoContainerTag);

    encodeObject(_codecs);
    encodeObject(_mixdowns);
    encodeObject(_bitRates);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeObject(_track);
    decodeObject(_codec);
    decodeObject(_mixdown);
    decodeObject(_sampleRate);
    decodeObject(_bitRate);
    decodeObject(_drc);
    decodeObject(_gain);
    decodeObject(_videoContainerTag);

    decodeObject(_codecs);
    decodeObject(_mixdowns);
    decodeObject(_bitRates);

    return self;
}

@end

