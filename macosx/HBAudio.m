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
NSString *keyAudioMP4 = @"keyAudioMP4";
NSString *keyAudioMKV = @"keyAudioMKV";
NSString *keyAudioSampleRateName = @"keyAudioSampleRateName";
NSString *keyAudioBitrateName = @"keyAudioBitrateName";
NSString *keyAudioMustMatchTrack = @"keyAudioMustMatchTrack";
NSString *keyAudioMixdownName = @"keyAudioMixdownName";
NSString *keyAudioMixdownLimitsToTrackBitRate = @"keyAudioMixdownLimitsToTrackBitRate";
NSString *keyAudioMixdownCanBeDefault = @"keyAudioMixdownCanBeDefault";

NSString *keyAudioCodec = @"codec";
NSString *keyAudioMixdown = @"mixdown";
NSString *keyAudioSamplerate = @"samplerate";
NSString *keyAudioBitrate = @"bitrate";

static NSMutableArray *masterCodecArray = nil;
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
	
	while (nil != (dict = [enumerator nextObject]) && nil == retval) {
		if (nil != (aValue = [dict objectForKey: aKey]) && YES == [aValue isEqual: anObject]) {
			retval = dict;
		}
	}
	return retval;
}
- (NSDictionary *) dictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{	return [self dictionaryWithObject: anObject matchingKey: aKey reverse: NO];	}
- (NSDictionary *) lastDictionaryWithObject: (id) anObject matchingKey: (NSString *) aKey
{	return [self dictionaryWithObject: anObject matchingKey: aKey reverse: YES];	}

@end

@implementation HBAudio

#pragma mark -
#pragma mark Object Setup

+ (void) load

{
	if ([HBAudio class] == self) {
		int i;
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSDictionary *dict;
		
		masterCodecArray = [[NSMutableArray alloc] init];	//	knowingly leaked
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"AAC (CoreAudio)", @"AAC (CoreAudio)"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_CA_AAC], keyAudioCodec,
									  [NSNumber numberWithBool: YES], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"AAC (faac)", @"AAC (faac)"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_FAAC], keyAudioCodec,
									  [NSNumber numberWithBool: YES], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"MP3 (lame)", @"MP3 (lame)"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_LAME], keyAudioCodec,
									  [NSNumber numberWithBool: YES], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"AC3 Passthru", @"AC3 Passthru"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_AC3_PASS], keyAudioCodec,
									  [NSNumber numberWithBool: YES], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithInt: HB_ACODEC_AC3], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"AC3", @"AC3"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_AC3], keyAudioCodec,
									  [NSNumber numberWithBool: YES], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"DTS Passthru", @"DTS Passthru"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_DCA_PASS], keyAudioCodec,
									  [NSNumber numberWithBool: NO], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithInt: HB_ACODEC_DCA], keyAudioMustMatchTrack,
									  nil]];
		[masterCodecArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									  NSLocalizedString(@"Vorbis (vorbis)", @"Vorbis (vorbis)"), keyAudioCodecName,
									  [NSNumber numberWithInt: HB_ACODEC_VORBIS], keyAudioCodec,
									  [NSNumber numberWithBool: NO], keyAudioMP4,
									  [NSNumber numberWithBool: YES], keyAudioMKV,
									  [NSNumber numberWithBool: NO], keyAudioMustMatchTrack,
									  nil]];
		
		//	Note that for the Auto value we use 0 for the sample rate because our controller will give back the track's
		//	input sample rate when it finds this 0 value as the selected sample rate.  We do this because the input
		//	sample rate depends on the track, which means it depends on the title, so cannot be nicely set up here.
		masterSampleRateArray = [[NSMutableArray alloc] init];	//	knowingly leaked
		[masterSampleRateArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
										   NSLocalizedString(@"Auto", @"Auto"), keyAudioSampleRateName,
										   [NSNumber numberWithInt: 0], keyAudioSamplerate,
										   nil]];
		for (i = 0; i < hb_audio_rates_count; i++) {
			[masterSampleRateArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
											   [NSString stringWithUTF8String: hb_audio_rates[i].string], keyAudioSampleRateName,
											   [NSNumber numberWithInt: hb_audio_rates[i].rate], keyAudioSamplerate,
											   nil]];
		}
		
		masterBitRateArray = [[NSMutableArray alloc] init];	// knowingly leaked
		for (i = 0; i < hb_audio_bitrates_count; i++) {
			int rate = hb_audio_bitrates[i].rate;
			dict = [NSDictionary dictionaryWithObjectsAndKeys:
					[NSString stringWithUTF8String: hb_audio_bitrates[i].string], keyAudioBitrateName,
					[NSNumber numberWithInt: rate], keyAudioBitrate,
					nil];
			[masterBitRateArray addObject: dict];
		}
		
		[pool release];
	}
	return;
}

//	Ensure the list of codecs is accurate
//	Update the current value of codec based on the revised list
- (void) updateCodecs

{
	NSMutableArray *permittedCodecs = [NSMutableArray array];
	unsigned int count = [masterCodecArray count];
	NSDictionary *dict;
	NSString *keyThatAllows = nil;

	//	Determine which key we use to see which codecs are permitted
	switch ([videoContainerTag intValue]) {
		case HB_MUX_MP4:
			keyThatAllows = keyAudioMP4;
			break;
		case HB_MUX_MKV:
			keyThatAllows = keyAudioMKV;
			break;
		default:
			keyThatAllows = @"error condition";
			break;
	}

	//	First get a list of the permitted codecs based on the internal rules
	if (nil != track && YES == [self enabled]) {
		BOOL goodToAdd;

		for (unsigned int i = 0; i < count; i++) {
			dict = [masterCodecArray objectAtIndex: i];
			
			//	First make sure only codecs permitted by the container are here
			goodToAdd = [[dict objectForKey: keyThatAllows] boolValue];
			
			//	Now we make sure if DTS or AC3 is not available in the track it is not put in the codec list, but in a general way
			if (YES == [[dict objectForKey: keyAudioMustMatchTrack] boolValue]) {
				if ([[dict objectForKey: keyAudioMustMatchTrack] intValue] != [[[self track] objectForKey: keyAudioInputCodec] intValue]) {
					goodToAdd = NO;
				}
			}
			
			if (YES == goodToAdd) {
				[permittedCodecs addObject: dict];
			}
		}
	}
	
	//	Now make sure the permitted list and the actual ones matches
	[self setCodecs: permittedCodecs];

	//	Ensure our codec is on the list of permitted codecs
	if (nil == [self codec] || NO == [permittedCodecs containsObject: [self codec]]) {
		if (0 < [permittedCodecs count]) {
			[self setCodec: [permittedCodecs objectAtIndex: 0]];	//	This should be defaulting to Core Audio
		}
		else {
			[self setCodec: nil];
		}
	}

	return;
}

//	The rules here are provided as-is from the original -[Controller audioTrackPopUpChanged:mixdownToUse:] routine
//	with the comments taken from there as well.
- (void) updateMixdowns

{
	NSMutableArray *retval = [NSMutableArray array];
	int trackCodec = [[track objectForKey: keyAudioInputCodec] intValue];
	int codecCodec = [[codec objectForKey: keyAudioCodec] intValue];

	if (HB_ACODEC_AC3 == trackCodec && HB_ACODEC_AC3_PASS == codecCodec) {
		[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
							NSLocalizedString(@"AC3 Passthru", @"AC3 Passthru"), keyAudioMixdownName,
							[NSNumber numberWithInt: HB_ACODEC_AC3_PASS], keyAudioMixdown,
							[NSNumber numberWithBool: YES], keyAudioMixdownLimitsToTrackBitRate,
							[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
							nil]];
	}
	else if (HB_ACODEC_DCA == trackCodec && HB_ACODEC_DCA_PASS == codecCodec) {
		[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
							NSLocalizedString(@"DTS Passthru", @"DTS Passthru"), keyAudioMixdownName,
							[NSNumber numberWithInt: HB_ACODEC_DCA_PASS], keyAudioMixdown,
							[NSNumber numberWithBool: YES], keyAudioMixdownLimitsToTrackBitRate,
							[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
							nil]];
	}
	else {
		int audioCodecsSupport6Ch = (trackCodec && HB_ACODEC_LAME != codecCodec);
		int channelLayout = [[track objectForKey: keyAudioInputChannelLayout] intValue];
		int layout = channelLayout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;
		
		/* add a mono option? */
		[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
							[NSString stringWithUTF8String: hb_audio_mixdowns[0].human_readable_name], keyAudioMixdownName,
							[NSNumber numberWithInt: hb_audio_mixdowns[0].amixdown], keyAudioMixdown,
							[NSNumber numberWithBool: NO], keyAudioMixdownLimitsToTrackBitRate,
							[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
							nil]];
		
		/* offer stereo if we have a stereo-or-better source */
		if (layout >= HB_INPUT_CH_LAYOUT_STEREO) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[1].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: hb_audio_mixdowns[1].amixdown], keyAudioMixdown,
								[NSNumber numberWithBool: NO], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
								nil]];
		}
	
		/* do we want to add a dolby surround (DPL1) option? */
		if (HB_INPUT_CH_LAYOUT_3F1R == layout || HB_INPUT_CH_LAYOUT_3F2R == layout || HB_INPUT_CH_LAYOUT_DOLBY == layout) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[2].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: hb_audio_mixdowns[2].amixdown], keyAudioMixdown,
								[NSNumber numberWithBool: NO], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
								nil]];
		}
		
		/* do we want to add a dolby pro logic 2 (DPL2) option? */
		if (HB_INPUT_CH_LAYOUT_3F2R == layout) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[3].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: hb_audio_mixdowns[3].amixdown], keyAudioMixdown,
								[NSNumber numberWithBool: NO], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
								nil]];
		}
		
		/* do we want to add a 6-channel discrete option? */
		if (1 == audioCodecsSupport6Ch && HB_INPUT_CH_LAYOUT_3F2R == layout && (channelLayout & HB_INPUT_CH_LAYOUT_HAS_LFE)) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[4].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: hb_audio_mixdowns[4].amixdown], keyAudioMixdown,
								[NSNumber numberWithBool: NO], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: (HB_ACODEC_AC3 == codecCodec) ? YES : NO], keyAudioMixdownCanBeDefault,
								nil]];
		}
		
		//	based on the fact that we are in an else section where the ifs before hand would have detected the following two
		//	situations, the following code will never add anything to the returned array.  I am leaving this in place for
		//	historical reasons.
		/* do we want to add an AC-3 passthrough option? */
		if (HB_ACODEC_AC3 == trackCodec && HB_ACODEC_AC3_PASS == codecCodec) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[5].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: HB_ACODEC_AC3_PASS], keyAudioMixdown,
								[NSNumber numberWithBool: YES], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
								nil]];
		}
			
		/* do we want to add a DTS Passthru option ? HB_ACODEC_DCA*/
		if (HB_ACODEC_DCA == trackCodec && HB_ACODEC_DCA_PASS == codecCodec) {
			[retval addObject: [NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithUTF8String: hb_audio_mixdowns[5].human_readable_name], keyAudioMixdownName,
								[NSNumber numberWithInt: HB_ACODEC_DCA_PASS], keyAudioMixdown,
								[NSNumber numberWithBool: YES], keyAudioMixdownLimitsToTrackBitRate,
								[NSNumber numberWithBool: YES], keyAudioMixdownCanBeDefault,
								nil]];
		}
	}
	
	//	Now make sure the permitted list and the actual ones matches
	[self setMixdowns: retval];
	
	//	Ensure our mixdown is on the list of permitted ones
	if (YES == [[NSUserDefaults standardUserDefaults] boolForKey: @"CodecDefaultsMixdown"] ||
		nil == [self mixdown] || NO == [retval containsObject: [self mixdown]]) {
		[self setMixdown: [retval lastDictionaryWithObject: [NSNumber numberWithBool: YES] matchingKey: keyAudioMixdownCanBeDefault]];
	}
	
	return;
}

- (void) updateBitRates

{
	NSMutableArray *permittedBitRates = [NSMutableArray array];
	int count;
	NSDictionary *dict;
	
	count = [masterBitRateArray count];
	int minBitRate;
	int maxBitRate;
	NSString *defaultBitRate;
	int currentBitRate;
	int trackInputBitRate = [[[self track] objectForKey: keyAudioInputBitrate] intValue];
	BOOL limitsToTrackInputBitRate = [[[self mixdown] objectForKey: keyAudioMixdownLimitsToTrackBitRate] boolValue];
	BOOL shouldAdd;
	int theSampleRate = [[[self sampleRate] objectForKey: keyAudioSamplerate] intValue];
		
	if (0 == theSampleRate) {	//	this means Auto
		theSampleRate = [[[self track] objectForKey: keyAudioInputSampleRate] intValue];
		}

	int ourCodec = [[codec objectForKey: keyAudioCodec] intValue];
	int ourMixdown = [[[self mixdown] objectForKey: keyAudioMixdown] intValue];
	hb_get_audio_bitrate_limits(ourCodec, theSampleRate, ourMixdown, &minBitRate, &maxBitRate);
	int theDefaultBitRate = hb_get_default_audio_bitrate(ourCodec, theSampleRate, ourMixdown);
	defaultBitRate = [NSString stringWithFormat: @"%d", theDefaultBitRate];

	for (unsigned int i = 0; i < count; i++) {
		dict = [masterBitRateArray objectAtIndex: i];
		currentBitRate = [[dict objectForKey: keyAudioBitrate] intValue];
		
		//	First ensure the bitrate falls within range of the codec
		shouldAdd = (currentBitRate >= minBitRate && currentBitRate <= maxBitRate);
		
		//	Now make sure the mixdown is not limiting us to the track input bitrate
		if (YES == shouldAdd && YES == limitsToTrackInputBitRate) {
			if (currentBitRate != trackInputBitRate) {
				shouldAdd = NO;
			}
		}
				
		if (YES == shouldAdd) {
			[permittedBitRates addObject: dict];
		}
	}
	
	//	There is a situation where we have a mixdown requirement to match the track input bit rate,
	//	but it does not fall into the range the codec supports.  Therefore, we force it here.
	if (YES == limitsToTrackInputBitRate && 0 == [permittedBitRates count]) {
		NSDictionary *missingBitRate = [masterBitRateArray dictionaryWithObject: [NSNumber numberWithInt: trackInputBitRate] matchingKey: keyAudioBitrate];
		if (nil == missingBitRate) {
			//	We are in an even worse situation where the requested bit rate does not even exist in the underlying
			//	library of supported bitrates.  Of course since this value is ignored we can freely make a bogus one
			//	for the UI just to make the user a little more aware.
			missingBitRate = [NSDictionary dictionaryWithObjectsAndKeys:
							  [NSString stringWithFormat: @"%d", trackInputBitRate], keyAudioBitrateName,
							  [NSNumber numberWithInt: trackInputBitRate], keyAudioBitrate,
							  nil];
			}
		[permittedBitRates addObject: missingBitRate];
	}
	
	//	Make sure we are updated with the permitted list
	[self setBitRates: permittedBitRates];

	//	Select the proper one
	[self setBitRateFromName: defaultBitRate];

	if (nil == [self bitRate] || NO == [permittedBitRates containsObject: [self bitRate]]) {
		[self setBitRate: [permittedBitRates lastObject]];
	}
	
	return;
}

#pragma mark -
#pragma mark Accessors

@synthesize track;
@synthesize codec;
@synthesize mixdown;
@synthesize sampleRate;
@synthesize bitRate;
@synthesize drc;
@synthesize videoContainerTag;
@synthesize controller;

@synthesize codecs;
@synthesize mixdowns;
@synthesize bitRates;

- (void) setVideoContainerTag: (NSNumber *) aValue

{
	if ((nil != aValue || nil != videoContainerTag) && NO == [aValue isEqual: videoContainerTag]) {
		[aValue retain];
		[videoContainerTag release];
		videoContainerTag = aValue;
		[self updateCodecs];
	}
	return;
}

//	We do some detection of the None track to do special things.
- (void) setTrack: (NSDictionary *) aValue

{
	if ((nil != aValue || nil != track) && NO == [aValue isEqual: track]) {
		BOOL switchingFromNone = [track isEqual: [controller noneTrack]];
		BOOL switchingToNone = [aValue isEqual: [controller noneTrack]];

		[aValue retain];
		[track release];
		track = aValue;

		if (nil != aValue) {
			[self updateCodecs];
			if (YES == [self enabled]) {
				[self setSampleRate: [[self sampleRates] objectAtIndex: 0]];	//	default to Auto
			}
			if (YES == switchingFromNone) {
				[controller switchingTrackFromNone: self];
			}
			if (YES == switchingToNone) {
				[controller settingTrackToNone: self];
			}
		}
	}
	return;
}

- (void) setCodec: (NSDictionary *) aValue

{
	if ((nil != aValue || nil != codec) && NO == [aValue isEqual: codec]) {
		[aValue retain];
		[codec release];
		codec = aValue;
		[self updateMixdowns];
		[self updateBitRates];
	}
	return;
}

- (void) setMixdown: (NSDictionary *) aValue

{
	if ((nil != aValue || nil != mixdown) && NO == [aValue isEqual: mixdown]) {
		[aValue retain];
		[mixdown release];
		mixdown = aValue;
		[self updateBitRates];
		[[NSNotificationCenter defaultCenter] postNotificationName: HBMixdownChangedNotification object: self];
	}
	return;
}

- (NSArray *) tracks	{	return [controller masterTrackArray];	}

- (NSArray *) sampleRates	{	return masterSampleRateArray;	}

- (void) dealloc

{
	[self setTrack: nil];
	[self setCodec: nil];
	[self setMixdown: nil];
	[self setSampleRate: nil];
	[self setBitRate: nil];
	[self setDrc: nil];
	[self setVideoContainerTag: nil];
	[self setCodecs: nil];
	[self setMixdowns: nil];
	[self setBitRates: nil];
	[super dealloc];
	return;
}

#pragma mark -
#pragma mark Special Setters

- (void) setTrackFromIndex: (int) aValue

{
	[self setTrack: [[self tracks] dictionaryWithObject: [NSNumber numberWithInt: aValue] matchingKey: keyAudioTrackIndex]];
	return;
}

//	This returns whether it is able to set the actual codec desired.
- (BOOL) setCodecFromName: (NSString *) aValue

{
	NSDictionary *dict = [[self codecs] dictionaryWithObject: aValue matchingKey: keyAudioCodecName];
	
	if (nil != dict) {
		[self setCodec: dict];
	}
	return (nil != dict);
}

- (void) setMixdownFromName: (NSString *) aValue

{
	NSDictionary *dict = [[self mixdowns] dictionaryWithObject: aValue matchingKey: keyAudioMixdownName];

	if (nil != dict) {
		[self setMixdown: dict];
	}
	return;
}

- (void) setSampleRateFromName: (NSString *) aValue

{
	NSDictionary *dict = [[self sampleRates] dictionaryWithObject: aValue matchingKey: keyAudioSampleRateName];
	
	if (nil != dict) {
		[self setSampleRate: dict];
	}
	return;
}

- (void) setBitRateFromName: (NSString *) aValue

{
	NSDictionary *dict = [[self bitRates] dictionaryWithObject: aValue matchingKey: keyAudioBitrateName];
	
	if (nil != dict) {
		[self setBitRate: dict];
	}
	return;
}


#pragma mark -
#pragma mark Validation

//	Because we have indicated that the binding for the drc validates immediately we can implement the
//	key value binding method to ensure the drc stays in our accepted range.
- (BOOL) validateDrc: (id *) ioValue error: (NSError *) outError

{
	BOOL retval = YES;
	
	if (nil != *ioValue) {
		if (0.0 < [*ioValue floatValue] && 1.0 > [*ioValue floatValue]) {
			*ioValue = [NSNumber numberWithFloat: 1.0];
		}
	}
	
	return retval;
}

#pragma mark -
#pragma mark Bindings Support

- (BOOL) enabled

{
	return (nil != track) ? (NO == [track isEqual: [controller noneTrack]]) : NO;
}

- (BOOL) mixdownEnabled

{
	BOOL retval = [self enabled];

	if (YES == retval) {
		int myMixdown = [[[self mixdown] objectForKey: keyAudioMixdown] intValue];
		if (HB_ACODEC_AC3_PASS == myMixdown || HB_ACODEC_DCA_PASS == myMixdown) {
			retval = NO;
		}
	}
	return retval;
}

- (BOOL) AC3Enabled

{
	BOOL retval = [self enabled];
	
	if (YES == retval) {
		int myTrackCodec = [[[self track] objectForKey: keyAudioInputCodec] intValue];
		if (HB_ACODEC_AC3 != myTrackCodec) {
			retval = NO;
		}
	}
	return retval;
}

+ (NSSet *) keyPathsForValuesAffectingEnabled

{
	return [NSSet setWithObjects: @"track", nil];
}

+ (NSSet *) keyPathsForValuesAffectingMixdownEnabled

{
	return [NSSet setWithObjects: @"track", @"mixdown", nil];
}

+ (NSSet *) keyPathsForValuesAffectingAC3Enabled

{
	return [NSSet setWithObjects: @"track", nil];
}

@end
