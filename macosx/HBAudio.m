//
//  HBAudio.m
//  HandBrake
//
//  Created on 2010-08-30.
//

#import "HBAudio.h"
#import "HBAudioController.h"
#import "hb.h"

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
        if (nil != (aValue = [dict objectForKey: aKey]) && [aValue isEqual: anObject])
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

@implementation HBAudio

#pragma mark -
#pragma mark Object Setup

+ (void) initialize

{
    if ([HBAudio class] == self)
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
                audioMustMatchTrack = [NSNumber
                                       numberWithInt:(audio_encoder->codec &
                                                      ~HB_ACODEC_PASS_FLAG)];
            }
            else
            {
                audioMustMatchTrack = [NSNumber numberWithBool:NO];
            }
            [masterCodecArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                         [NSString stringWithUTF8String:audio_encoder->name], keyAudioCodecName,
                                         [NSNumber numberWithInt:audio_encoder->codec],       keyAudioCodec,
                                         [NSNumber numberWithInt:audio_encoder->muxers],      keyAudioSupportedMuxers,
                                         audioMustMatchTrack,                                 keyAudioMustMatchTrack,
                                         nil]];
        }

        masterMixdownArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (const hb_mixdown_t *mixdown = hb_mixdown_get_next(NULL);
             mixdown != NULL;
             mixdown  = hb_mixdown_get_next(mixdown))
        {
            [masterMixdownArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                           [NSString stringWithUTF8String:mixdown->name], keyAudioMixdownName,
                                           [NSNumber numberWithInt:mixdown->amixdown],    keyAudioMixdown,
                                           nil]];
        }

        // Note that for the Auto value we use 0 for the sample rate because our controller will give back the track's
        // input sample rate when it finds this 0 value as the selected sample rate.  We do this because the input
        // sample rate depends on the track, which means it depends on the title, so cannot be nicely set up here.
        masterSampleRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        [masterSampleRateArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                          @"Auto", keyAudioSampleRateName,
                                          [NSNumber numberWithInt:0],          keyAudioSamplerate,
                                          nil]];
        for (const hb_rate_t *audio_samplerate = hb_audio_samplerate_get_next(NULL);
             audio_samplerate != NULL;
             audio_samplerate  = hb_audio_samplerate_get_next(audio_samplerate))
        {
            [masterSampleRateArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                              [NSString stringWithUTF8String:audio_samplerate->name], keyAudioSampleRateName,
                                              [NSNumber numberWithInt:audio_samplerate->rate],        keyAudioSamplerate,
                                              nil]];
        }

        masterBitRateArray = [[NSMutableArray alloc] init]; // knowingly leaked
        for (const hb_rate_t *audio_bitrate = hb_audio_bitrate_get_next(NULL);
             audio_bitrate != NULL;
             audio_bitrate  = hb_audio_bitrate_get_next(audio_bitrate))
        {
            [masterBitRateArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                                           [NSString stringWithUTF8String:audio_bitrate->name], keyAudioBitrateName,
                                           [NSNumber numberWithInt:audio_bitrate->rate],        keyAudioBitrate,
                                           nil]];
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
    if (nil != track && [self enabled])
    {
        BOOL goodToAdd;

        for (unsigned int i = 0; i < count; i++)
        {
            dict = [masterCodecArray objectAtIndex: i];

            // First make sure only codecs permitted by the container are here
            goodToAdd = !!([[dict objectForKey:keyAudioSupportedMuxers] intValue] &
                           [videoContainerTag                           intValue]);

            // Now we make sure if DTS or AC3 is not available in the track it is not put in the codec list, but in a general way
            if ([[dict objectForKey: keyAudioMustMatchTrack] boolValue])
            {
                if ([[dict objectForKey: keyAudioMustMatchTrack] intValue] != [[[self track] objectForKey: keyAudioInputCodec] intValue])
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
    if (![self codec] || ![permittedCodecs containsObject: [self codec]])
    {
        if (0 < [permittedCodecs count])
        {
            [self setCodec: [permittedCodecs objectAtIndex: 0]]; // This should be defaulting to Core Audio
        }
        else
        {
            [self setCodec: nil];
        }
    }
}

- (void) updateMixdowns: (BOOL) shouldSetDefault

{
    NSMutableArray *permittedMixdowns = [NSMutableArray array];
    NSDictionary *dict;
    int currentMixdown;

    unsigned long long channelLayout = [[track objectForKey: keyAudioInputChannelLayout] unsignedLongLongValue];
    NSUInteger count                 = [masterMixdownArray count];
    int codecCodec                   = [[codec objectForKey: keyAudioCodec] intValue];
    int theDefaultMixdown            = hb_mixdown_get_default(codecCodec, channelLayout);

    for (unsigned int i = 0; i < count; i++)
    {
        dict = [masterMixdownArray objectAtIndex: i];
        currentMixdown = [[dict objectForKey: keyAudioMixdown] intValue];

        if (hb_mixdown_is_supported(currentMixdown, codecCodec, channelLayout))
        {
            [permittedMixdowns addObject: dict];
        }
    }

    if (![self enabled])
    {
        permittedMixdowns = nil;
    }

    // Now make sure the permitted list and the actual ones matches
    [self setMixdowns: permittedMixdowns];

    // Select the proper one
    if (shouldSetDefault)
    {
        [self setMixdown: [permittedMixdowns dictionaryWithObject: [NSNumber numberWithInt: theDefaultMixdown]
                                                      matchingKey: keyAudioMixdown]];
    }

    if (![self mixdown] || ![permittedMixdowns containsObject: [self mixdown]])
    {
        [self setMixdown: [permittedMixdowns lastObject]];
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
    int trackInputBitRate = [[[self track] objectForKey: keyAudioInputBitrate] intValue];
    int theSampleRate = [[[self sampleRate] objectForKey: keyAudioSamplerate] intValue];

    if (0 == theSampleRate) // this means Auto
    {
        theSampleRate = [[[self track] objectForKey: keyAudioInputSampleRate] intValue];
    }

    int ourCodec          = [[codec objectForKey:keyAudioCodec] intValue];
    int ourMixdown        = [[[self mixdown] objectForKey:keyAudioMixdown] intValue];
    int theDefaultBitRate = hb_audio_bitrate_get_default(ourCodec, theSampleRate, ourMixdown);
    hb_audio_bitrate_get_limits(ourCodec, theSampleRate, ourMixdown, &minBitRate, &maxBitRate);

    BOOL codecIsPassthru = ([[codec objectForKey: keyAudioCodec] intValue] & HB_ACODEC_PASS_FLAG) ? YES : NO;
    BOOL codecIsLossless = (theDefaultBitRate == -1) ? YES : NO;

    if (codecIsPassthru)
    {
        NSDictionary *sourceBitRate = [masterBitRateArray dictionaryWithObject: [NSNumber numberWithInt: trackInputBitRate]
                                                                   matchingKey: keyAudioBitrate];
        if (!sourceBitRate)
        {
            // the source bitrate isn't in the master array - create it
            sourceBitRate = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSString stringWithFormat: @"%d", trackInputBitRate], keyAudioBitrateName,
                             [NSNumber numberWithInt: trackInputBitRate], keyAudioBitrate,
                             nil];
        }
        [permittedBitRates addObject: sourceBitRate];
    }
    else if (codecIsLossless)
    {
        NSDictionary *bitRateNotApplicable = [NSDictionary dictionaryWithObjectsAndKeys:
                                              @"N/A", keyAudioBitrateName,
                                              [NSNumber numberWithInt: -1], keyAudioBitrate,
                                              nil];
        [permittedBitRates addObject: bitRateNotApplicable];
    }
    else
    {
        for (unsigned int i = 0; i < count; i++)
        {
            dict = [masterBitRateArray objectAtIndex: i];
            currentBitRate = [[dict objectForKey: keyAudioBitrate] intValue];

            // First ensure the bitrate falls within range of the codec
            shouldAdd = (currentBitRate >= minBitRate && currentBitRate <= maxBitRate);

            if (shouldAdd)
            {
                [permittedBitRates addObject: dict];
            }
        }
    }

    if (![self enabled])
    {
        permittedBitRates = nil;
    }

    // Make sure we are updated with the permitted list
    [self setBitRates: permittedBitRates];

    // Select the proper one
    if (shouldSetDefault)
    {
        [self setBitRateFromName: [NSString stringWithFormat:@"%d", theDefaultBitRate]];
    }

    if (![self bitRate] || ![permittedBitRates containsObject: [self bitRate]])
    {
        [self setBitRate: [permittedBitRates lastObject]];
    }
}

- (id) init

{
    if (self = [super init])
    {
        [self addObserver: self forKeyPath: @"videoContainerTag" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"track" options: NSKeyValueObservingOptionOld context: NULL];
        [self addObserver: self forKeyPath: @"codec" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"mixdown" options: 0 context: NULL];
        [self addObserver: self forKeyPath: @"sampleRate" options: 0 context: NULL];
    }
    return self;
}

#pragma mark -
#pragma mark Accessors

@synthesize track;
@synthesize codec;
@synthesize mixdown;
@synthesize sampleRate;
@synthesize bitRate;
@synthesize drc;
@synthesize gain;
@synthesize videoContainerTag;
@synthesize controller;

@synthesize codecs;
@synthesize mixdowns;
@synthesize bitRates;

- (NSArray *) sampleRates
{
    return masterSampleRateArray;
}

- (void) dealloc

{
    [self removeObserver: self forKeyPath: @"videoContainerTag"];
    [self removeObserver: self forKeyPath: @"track"];
    [self removeObserver: self forKeyPath: @"codec"];
    [self removeObserver: self forKeyPath: @"mixdown"];
    [self removeObserver: self forKeyPath: @"sampleRate"];
    [self setTrack: nil];
    [self setCodec: nil];
    [self setMixdown: nil];
    [self setSampleRate: nil];
    [self setBitRate: nil];
    [self setDrc: nil];
    [self setGain: nil];
    [self setVideoContainerTag: nil];
    [self setCodecs: nil];
    [self setMixdowns: nil];
    [self setBitRates: nil];
    [super dealloc];
}

#pragma mark -
#pragma mark KVO

- (void) observeValueForKeyPath: (NSString *) keyPath ofObject: (id) object change: (NSDictionary *) change context: (void *) context

{
    if ([keyPath isEqualToString: @"videoContainerTag"])
    {
        [self updateCodecs];
    }
    else if ([keyPath isEqualToString: @"track"])
    {
        if (nil != [self track])
        {
            [self updateCodecs];
            [self updateMixdowns: YES];
            if ([self enabled])
            {
                [self setSampleRate: [[self sampleRates] objectAtIndex: 0]]; // default to Auto
            }
            if ([[controller noneTrack] isEqual: [change objectForKey: NSKeyValueChangeOldKey]])
            {
                [controller switchingTrackFromNone: self];
            }
            if ([[controller noneTrack] isEqual: [self track]])
            {
                [controller settingTrackToNone: self];
            }
        }
    }
    else if ([keyPath isEqualToString: @"codec"])
    {
        [self updateMixdowns: YES];
        [self updateBitRates: YES];
    }
    else if ([keyPath isEqualToString: @"mixdown"])
    {
        [self updateBitRates: YES];
        [[NSNotificationCenter defaultCenter] postNotificationName: HBMixdownChangedNotification object: self];
    }
    else if ([keyPath isEqualToString: @"sampleRate"])
    {
        [self updateBitRates: NO];
    }
}

#pragma mark -
#pragma mark Special Setters

- (void) setTrackFromIndex: (int) aValue

{
    [self setTrack: [self.controller.masterTrackArray dictionaryWithObject: [NSNumber numberWithInt: aValue]
                                                               matchingKey: keyAudioTrackIndex]];
}

// This returns whether it is able to set the actual codec desired.
- (BOOL) setCodecFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self codecs] dictionaryWithObject: aValue matchingKey: keyAudioCodecName];

    if (nil != dict)
    {
        [self setCodec: dict];
    }
    return (nil != dict);
}

- (void) setMixdownFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self mixdowns] dictionaryWithObject: aValue matchingKey: keyAudioMixdownName];

    if (nil != dict)
    {
        [self setMixdown: dict];
    }
}

- (void) setSampleRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self sampleRates] dictionaryWithObject: aValue matchingKey: keyAudioSampleRateName];

    if (nil != dict)
    {
        [self setSampleRate: dict];
    }
}

- (void) setBitRateFromName: (NSString *) aValue

{
    NSDictionary *dict = [[self bitRates] dictionaryWithObject: aValue matchingKey: keyAudioBitrateName];

    if (nil != dict)
    {
        [self setBitRate: dict];
    }
}


#pragma mark -
#pragma mark Validation

// Because we have indicated that the binding for the drc validates immediately we can implement the
// key value binding method to ensure the drc stays in our accepted range.
- (BOOL) validateDrc: (id *) ioValue error: (NSError *) outError

{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        if (0.0 < [*ioValue floatValue] && 1.0 > [*ioValue floatValue])
        {
            *ioValue = [NSNumber numberWithFloat: 1.0];
        }
    }

    return retval;
}

// Because we have indicated that the binding for the gain validates immediately we can implement the
// key value binding method to ensure the gain stays in our accepted range.

- (BOOL) validateGain: (id *) ioValue error: (NSError *) outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        if (0.0 < [*ioValue floatValue] && 1.0 > [*ioValue floatValue])
        {
            *ioValue = [NSNumber numberWithFloat: 0.0];
        }
    }

    return retval;
}

#pragma mark -
#pragma mark Bindings Support

- (BOOL) enabled

{
    return (nil != track) ? (![track isEqual: [controller noneTrack]]) : NO;
}

- (BOOL) mixdownEnabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myMixdown = [[[self mixdown] objectForKey: keyAudioMixdown] intValue];
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
    BOOL retval = [self enabled];

    if (retval)
    {
        int myCodecCodec          = [[[self codec] objectForKey:keyAudioCodec] intValue];
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
    BOOL retval = [self enabled];

    if (retval)
    {
        int myTrackParam = [[[self track] objectForKey: keyAudioInputCodecParam] intValue];
        int myTrackCodec = [[[self track] objectForKey: keyAudioInputCodec] intValue];
        int myCodecCodec = [[[self codec] objectForKey: keyAudioCodec] intValue];
        if (!hb_audio_can_apply_drc(myTrackCodec, myTrackParam, myCodecCodec))
        {
            retval = NO;
        }
    }
    return retval;
}

- (BOOL) PassThruDisabled

{
    BOOL retval = [self enabled];

    if (retval)
    {
        int myCodecCodec = [[[self codec] objectForKey: keyAudioCodec] intValue];
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

@end

