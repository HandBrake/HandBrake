//
//  HBAudioController.m
//  HandBrake
//
//  Created on 2010-08-24.
//

#import "HBAudioController.h"
#import "Controller.h"
#import "HBAudio.h"
#import "hb.h"

NSString *keyAudioTrackIndex = @"keyAudioTrackIndex";
NSString *keyAudioTrackName = @"keyAudioTrackName";
NSString *keyAudioInputBitrate = @"keyAudioInputBitrate";
NSString *keyAudioInputSampleRate = @"keyAudioInputSampleRate";
NSString *keyAudioInputCodec = @"keyAudioInputCodec";
NSString *keyAudioInputChannelLayout = @"keyAudioInputChannelLayout";
NSString *HBMixdownChangedNotification = @"HBMixdownChangedNotification";

@implementation HBAudioController

#pragma mark -
#pragma mark Accessors

@synthesize masterTrackArray;
@synthesize noneTrack;
@synthesize videoContainerTag;

- (id) init

{
	if (self = [super init]) {
		[self setVideoContainerTag: [NSNumber numberWithInt: HB_MUX_MP4]];
		audioArray = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void) dealloc

{
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	[masterTrackArray release];
	[noneTrack release];
	[audioArray release];
	[self setVideoContainerTag: nil];
	[super dealloc];
	return;
}

- (void) setHBController: (id) aController

{
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	myController = aController;

	/* register that we are interested in changes made to the video container */
	[center addObserver: self selector: @selector(containerChanged:) name: HBContainerChangedNotification object: aController];
	[center addObserver: self selector: @selector(titleChanged:) name: HBTitleChangedNotification object: aController];
	return;
}

- (void) _clearAudioArray

{
	while (0 < [self countOfAudioArray]) {
		[self removeObjectFromAudioArrayAtIndex: 0];
	}
	return;
}

#pragma mark -
#pragma mark HBController Support

- (void) prepareAudioForQueueFileJob: (NSMutableDictionary *) aDict

{
	unsigned int audioArrayCount = [self countOfAudioArray];
	for (unsigned int counter = 0; counter < audioArrayCount; counter++) {
		HBAudio *anAudio = [self objectInAudioArrayAtIndex: counter];
		if (YES == [anAudio enabled]) {
			NSString *prefix = [NSString stringWithFormat: @"Audio%d", counter + 1];
			NSNumber *sampleRateToUse = (0 == [[[anAudio sampleRate] objectForKey: keyAudioSamplerate] intValue]) ?
								[[anAudio track] objectForKey: keyAudioInputSampleRate] :
								[[anAudio sampleRate] objectForKey: keyAudioSamplerate];
		
			[aDict setObject: [[anAudio track] objectForKey: keyAudioTrackIndex] forKey: [prefix stringByAppendingString: @"Track"]];
			[aDict setObject: [[anAudio track] objectForKey: keyAudioTrackName] forKey: [prefix stringByAppendingString: @"TrackDescription"]];
			[aDict setObject: [[anAudio codec] objectForKey: keyAudioCodecName] forKey: [prefix stringByAppendingString: @"Encoder"]];
			[aDict setObject: [[anAudio mixdown] objectForKey: keyAudioMixdownName] forKey: [prefix stringByAppendingString: @"Mixdown"]];
			[aDict setObject: [[anAudio sampleRate] objectForKey: keyAudioSampleRateName] forKey: [prefix stringByAppendingString: @"Samplerate"]];
			[aDict setObject: [[anAudio bitRate] objectForKey: keyAudioBitrateName] forKey: [prefix stringByAppendingString: @"Bitrate"]];
			[aDict setObject: [anAudio drc] forKey: [prefix stringByAppendingString: @"TrackDRCSlider"]];
		
			prefix = [NSString stringWithFormat: @"JobAudio%d", counter + 1];
			[aDict setObject: [[anAudio codec] objectForKey: keyAudioCodec] forKey: [prefix stringByAppendingString: @"Encoder"]];
			[aDict setObject: [[anAudio mixdown] objectForKey: keyAudioMixdown] forKey: [prefix stringByAppendingString: @"Mixdown"]];
			[aDict setObject: sampleRateToUse forKey: [prefix stringByAppendingString: @"Samplerate"]];
			[aDict setObject: [[anAudio bitRate] objectForKey: keyAudioBitrate] forKey: [prefix stringByAppendingString: @"Bitrate"]];
		}
	}
	return;
}

- (void) prepareAudioForJob: (hb_job_t *) aJob

{
	unsigned int i;
	
	//	First clear out any audio tracks in the job currently
    int audiotrack_count = hb_list_count(aJob->list_audio);
    for(i = 0; i < audiotrack_count; i++)
    {
        hb_audio_t *temp_audio = (hb_audio_t *) hb_list_item(aJob->list_audio, 0);
        hb_list_rem(aJob->list_audio, temp_audio);
    }

	//	Now add audio tracks based on the current settings
	unsigned int audioArrayCount = [self countOfAudioArray];
	for (i = 0; i < audioArrayCount; i++) {
		HBAudio *anAudio = [self objectInAudioArrayAtIndex: i];
		if (YES == [anAudio enabled]) {
			NSNumber *sampleRateToUse = (0 == [[[anAudio sampleRate] objectForKey: keyAudioSamplerate] intValue]) ?
										[[anAudio track] objectForKey: keyAudioInputSampleRate] :
										[[anAudio sampleRate] objectForKey: keyAudioSamplerate];
			
			hb_audio_config_t *audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
			hb_audio_config_init(audio);
			audio->in.track = [[[anAudio track] objectForKey: keyAudioTrackIndex] intValue] - 1;
			/* We go ahead and assign values to our audio->out.<properties> */
			audio->out.track = audio->in.track;
			audio->out.codec = [[[anAudio codec] objectForKey: keyAudioCodec] intValue];
			audio->out.mixdown = [[[anAudio mixdown] objectForKey: keyAudioMixdown] intValue];
			audio->out.bitrate = [[[anAudio bitRate] objectForKey: keyAudioBitrate] intValue];
			audio->out.samplerate = [sampleRateToUse intValue];
			audio->out.dynamic_range_compression = [[anAudio drc] floatValue];
        
			hb_audio_add(aJob, audio);
			free(audio);
		}
	}
	return;
}

- (void) prepareAudioForPreset: (NSMutableArray *) anArray

{
	unsigned int audioArrayCount = [self countOfAudioArray];
	unsigned int i;

	for (i = 0; i < audioArrayCount; i++) {
		HBAudio *anAudio = [self objectInAudioArrayAtIndex: i];
		if (YES == [anAudio enabled]) {
			NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithCapacity: 7];
			[dict setObject: [[anAudio track] objectForKey: keyAudioTrackIndex] forKey: @"AudioTrack"];
			[dict setObject: [[anAudio track] objectForKey: keyAudioTrackName] forKey: @"AudioTrackDescription"];
			[dict setObject: [[anAudio codec] objectForKey: keyAudioCodecName] forKey: @"AudioEncoder"];
			[dict setObject: [[anAudio mixdown] objectForKey: keyAudioMixdownName] forKey: @"AudioMixdown"];
			[dict setObject: [[anAudio sampleRate] objectForKey: keyAudioSampleRateName] forKey: @"AudioSamplerate"];
			[dict setObject: [[anAudio bitRate] objectForKey: keyAudioBitrateName] forKey: @"AudioBitrate"];
			[dict setObject: [anAudio drc] forKey: @"AudioTrackDRCSlider"];
			[anArray addObject: dict];
			[dict release];
		}
	}
	return;
}

- (void) addTracksFromQueue: (NSMutableDictionary *) aQueue

{
	NSString *base;
	int value;
	int maximumNumberOfAllowedAudioTracks = [HBController maximumNumberOfAllowedAudioTracks];

	//	Reinitialize the configured list of audio tracks
	[self _clearAudioArray];
	
	//	The following is the pattern to follow, but with Audio%dTrack being the key to seek...
	//	Can we assume that there will be no skip in the data?
	for (unsigned int i = 1; i <= maximumNumberOfAllowedAudioTracks; i++) {
		base = [NSString stringWithFormat: @"Audio%d", i];
		value = [[aQueue objectForKey: [base stringByAppendingString: @"Track"]] intValue];
		if (0 < value) {
			HBAudio *newAudio = [[HBAudio alloc] init];
			[newAudio setController: self];
			[self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
			[newAudio setVideoContainerTag: [self videoContainerTag]];
			[newAudio setTrackFromIndex: value];
			[newAudio setCodecFromName: [aQueue objectForKey: [base stringByAppendingString: @"Encoder"]]];
			[newAudio setMixdownFromName: [aQueue objectForKey: [base stringByAppendingString: @"Mixdown"]]];
			[newAudio setSampleRateFromName: [aQueue objectForKey: [base stringByAppendingString: @"Samplerate"]]];
			[newAudio setBitRateFromName: [aQueue objectForKey: [base stringByAppendingString: @"Bitrate"]]];
			[newAudio setDrc: [aQueue objectForKey: [base stringByAppendingString: @"TrackDRCSlider"]]];
			[newAudio release];
		}
	}

	[self switchingTrackFromNone: nil];	// see if we need to add one to the list
	
	return;
}

//	This routine takes the preset and will return the value for the key AudioList
//	if it exists, otherwise it creates an array from the data in the present.
- (NSArray *) _presetAudioArrayFromPreset: (NSMutableDictionary *) aPreset

{
	NSArray *retval = [aPreset objectForKey: @"AudioList"];

	if (nil == retval) {
		int maximumNumberOfAllowedAudioTracks = [HBController maximumNumberOfAllowedAudioTracks];
		NSString *base;
		NSMutableArray *whatToUse = [NSMutableArray array];
		for (unsigned int i = 1; i <= maximumNumberOfAllowedAudioTracks; i++) {
			base = [NSString stringWithFormat: @"Audio%d", i];
			if (nil != [aPreset objectForKey: [base stringByAppendingString: @"Track"]]) {
				[whatToUse addObject: [NSDictionary dictionaryWithObjectsAndKeys:
									   [aPreset objectForKey: [base stringByAppendingString: @"Encoder"]], @"AudioEncoder",
									   [aPreset objectForKey: [base stringByAppendingString: @"Mixdown"]], @"AudioMixdown",
									   [aPreset objectForKey: [base stringByAppendingString: @"Samplerate"]], @"AudioSamplerate",
									   [aPreset objectForKey: [base stringByAppendingString: @"Bitrate"]], @"AudioBitrate",
									   [aPreset objectForKey: [base stringByAppendingString: @"TrackDRCSlider"]], @"AudioTrackDRCSlider",
									   nil]];
			}
		}
		retval = whatToUse;
	}
	return retval;
}

//	This uses the templateAudioArray from the preset to create the audios for the specified trackIndex
- (void) _processPresetAudioArray: (NSArray *) templateAudioArray forTrack: (unsigned int) trackIndex andType: (int) aType

{
	NSEnumerator *enumerator = [templateAudioArray objectEnumerator];
	NSDictionary *dict;
	NSString *key;
	int maximumNumberOfAllowedAudioTracks = [HBController maximumNumberOfAllowedAudioTracks];
	
	while (nil != (dict = [enumerator nextObject])) {
		if ([self countOfAudioArray] < maximumNumberOfAllowedAudioTracks) {
			HBAudio *newAudio = [[HBAudio alloc] init];
			[newAudio setController: self];
			[self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
			[newAudio setVideoContainerTag: [self videoContainerTag]];
			[newAudio setTrackFromIndex: trackIndex];
			key = [dict objectForKey: @"AudioEncoder"];
			if (0 == aType &&
				YES == [[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] &&
				YES == [key isEqualToString: @"AAC (faac)"]
				) {
				key = @"AAC (CoreAudio)";
			}
			if (YES == [[NSUserDefaults standardUserDefaults] boolForKey: @"AC3PassthruDefaultsToAC3"] &&
				YES == [key isEqualToString: @"AC3 Passthru"]) {
				if (NO == [newAudio setCodecFromName: key]) {
					key = @"AC3";
				}
			}
			//	If our preset wants us to support a codec that the track does not support, instead
			//	of changing the codec we remove the audio instead.
			if (YES == [newAudio setCodecFromName: key]) {
				[newAudio setMixdownFromName: [dict objectForKey: @"AudioMixdown"]];
				[newAudio setSampleRateFromName: [dict objectForKey: @"AudioSamplerate"]];
				[newAudio setBitRateFromName: [dict objectForKey: @"AudioBitrate"]];
				[newAudio setDrc: [dict objectForKey: @"AudioTrackDRCSlider"]];
			}
			else {
				[self removeObjectFromAudioArrayAtIndex: [self countOfAudioArray] - 1];
			}
			[newAudio release];
		}
	}
	return;
}

//	This matches the FIRST track with the specified prefix, otherwise it uses the defaultIfNotFound value
- (unsigned int) _trackWithTitlePrefix: (NSString *) prefix defaultIfNotFound: (unsigned int) defaultIfNotFound

{
	unsigned int retval = defaultIfNotFound;
	int count = [masterTrackArray count];
	NSString *languageTitle;
	BOOL found = NO;
	
	//	We search for the prefix noting that our titles have the format %d: %s where the %s is the prefix
	for (unsigned int i = 1; i < count && NO == found; i++) {	//	Note that we skip the "None" track
		languageTitle = [[masterTrackArray objectAtIndex: i] objectForKey: keyAudioTrackName];
		if (YES == [[languageTitle substringFromIndex: [languageTitle rangeOfString: @" "].location + 1] hasPrefix: prefix]) {
			retval = i;
			found = YES;
		}
	}
	return retval;
}

//	When we add a track and we do not have a preset to use for the track we use
//	this bogus preset to do the dirty work.
- (NSMutableDictionary *) _defaultPreset

{
	static NSMutableDictionary *retval = nil;
	
	if (nil == retval) {
		retval = [[NSMutableDictionary dictionaryWithObjectsAndKeys:
		[NSArray arrayWithObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt: 1],     @"AudioTrack",
		@"AAC (faac)",                   @"AudioEncoder",
		@"Dolby Pro Logic II",           @"AudioMixdown",
		@"Auto",                         @"AudioSamplerate",
		@"160",                          @"AudioBitrate",
		[NSNumber numberWithFloat: 0.0], @"AudioTrackDRCSlider",
		nil]], @"AudioList", nil] retain];
	}
	return retval;
}

- (void) addTracksFromPreset: (NSMutableDictionary *) aPreset allTracks: (BOOL) allTracks

{
	id whatToUse = [self _presetAudioArrayFromPreset: aPreset];
	NSString *preferredLanguageName = [[NSUserDefaults standardUserDefaults] stringForKey: @"DefaultLanguage"];
	int preferredLanguage = [self _trackWithTitlePrefix: preferredLanguageName defaultIfNotFound: 1];

	//	Reinitialize the configured list of audio tracks
	[self _clearAudioArray];
	
	[self _processPresetAudioArray: whatToUse forTrack: preferredLanguage andType: [[aPreset objectForKey: @"Type"] intValue]];
	if (YES == allTracks) {
		unsigned int count = [masterTrackArray count];
		for (unsigned int i = 1; i < count; i++) {
			if (i != preferredLanguage) {
				[self _processPresetAudioArray: whatToUse forTrack: i andType: [[aPreset objectForKey: @"Type"] intValue]];
			}
		}
	}

	return;
}

- (void) _ensureAtLeastOneNonEmptyTrackExists

{
	int count = [self countOfAudioArray];
	if (0 == count || NO == [[self objectInAudioArrayAtIndex: 0] enabled]) {
		[self addTracksFromPreset: [self _defaultPreset] allTracks: NO];
		}
	[self switchingTrackFromNone: nil];	//	this ensures there is a None track at the end of the list
	return;
}

- (void) addTracksFromPreset: (NSMutableDictionary *) aPreset

{
	[self addTracksFromPreset: aPreset allTracks: NO];
	[self _ensureAtLeastOneNonEmptyTrackExists];
	return;
}

- (void) addAllTracksFromPreset: (NSMutableDictionary *) aPreset

{
	[self addTracksFromPreset: aPreset allTracks: YES];
	[self _ensureAtLeastOneNonEmptyTrackExists];
	return;
}

- (BOOL) anyCodecMatches: (int) aCodecValue

{
	BOOL retval = NO;
	unsigned int audioArrayCount = [self countOfAudioArray];
	for (unsigned int i = 0; i < audioArrayCount && NO == retval; i++) {
		HBAudio *anAudio = [self objectInAudioArrayAtIndex: i];
        if (YES == [anAudio enabled] && aCodecValue == [[[anAudio codec] objectForKey: keyAudioCodec] intValue]) {
			retval = YES;
		}
	}
	return retval;
}

- (void) addNewAudioTrack

{
	HBAudio *newAudio = [[HBAudio alloc] init];
	[newAudio setController: self];
	[self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
	[newAudio setVideoContainerTag: [self videoContainerTag]];
	[newAudio setTrack: noneTrack];
	[newAudio setDrc: [NSNumber numberWithFloat: 0.0]];
	[newAudio release];	
	return;
}

#pragma mark -
#pragma mark Notification Handling

- (void) settingTrackToNone: (HBAudio *) newNoneTrack

{
	//	If this is not the last track in the array we need to remove it.  We then need to see if a new
	//	one needs to be added (in the case when we were at maximum count and this switching makes it
	//	so we are no longer at maximum.
	unsigned int index = [audioArray indexOfObject: newNoneTrack];

	if (NSNotFound != index && index < [self countOfAudioArray] - 1) {
		[self removeObjectFromAudioArrayAtIndex: index];
	}
	[self switchingTrackFromNone: nil];	// see if we need to add one to the list
	return;
}

- (void) switchingTrackFromNone: (HBAudio *) noLongerNoneTrack

{
	int count = [self countOfAudioArray];
	BOOL needToAdd = NO;
	int maximumNumberOfAllowedAudioTracks = [HBController maximumNumberOfAllowedAudioTracks];

	//	If there is no last track that is None and we are less than our maximum number of permitted tracks, we add one.
	if (count < maximumNumberOfAllowedAudioTracks) {
		if (0 < count) {
			HBAudio *lastAudio = [self objectInAudioArrayAtIndex: count - 1];
			if (YES == [lastAudio enabled]) {
				needToAdd = YES;
			}
		}
		else {
			needToAdd = YES;
		}
	}

	if (YES == needToAdd) {
		[self addNewAudioTrack];
	}
	return;
}

//	This gets called whenever the video container changes.
- (void) containerChanged: (NSNotification *) aNotification

{
	NSDictionary *notDict = [aNotification userInfo];

	[self setVideoContainerTag: [notDict objectForKey: keyContainerTag]];

	//	Update each of the instances because this value influences possible settings.
	NSEnumerator *enumerator = [audioArray objectEnumerator];
	HBAudio *audioObject;

	while (nil != (audioObject = [enumerator nextObject])) {
		[audioObject setVideoContainerTag: [self videoContainerTag]];
	}
	return;
}

- (void) titleChanged: (NSNotification *) aNotification

{
	NSDictionary *notDict = [aNotification userInfo];
	NSData *theData = [notDict objectForKey: keyTitleTag];
	hb_title_t *title = NULL;

	[theData getBytes: &title length: sizeof(title)];
	if (title) {
		hb_audio_config_t *audio;
		hb_list_t *list = title->list_audio;
		int i, count = hb_list_count(list);

		//	Reinitialize the master list of available audio tracks from this title
		[masterTrackArray release];
		masterTrackArray = [[NSMutableArray alloc] init];
		[noneTrack release];
		noneTrack = [[NSDictionary dictionaryWithObjectsAndKeys:
					 [NSNumber numberWithInt: 0], keyAudioTrackIndex,
					 NSLocalizedString(@"None", @"None"), keyAudioTrackName,
					 [NSNumber numberWithInt: 0], keyAudioInputCodec,
							 nil] retain];
		[masterTrackArray addObject: noneTrack];
		for (i = 0; i < count; i++) {
			audio = (hb_audio_config_t *) hb_list_audio_config_item(list, i);
			[masterTrackArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
										  [NSNumber numberWithInt: i + 1], keyAudioTrackIndex,
										  [NSString stringWithFormat: @"%d: %s", i, audio->lang.description], keyAudioTrackName,
										  [NSNumber numberWithInt: audio->in.bitrate / 1000], keyAudioInputBitrate,
										  [NSNumber numberWithInt: audio->in.samplerate], keyAudioInputSampleRate,
										  [NSNumber numberWithInt: audio->in.codec], keyAudioInputCodec,
										  [NSNumber numberWithInt: audio->in.channel_layout], keyAudioInputChannelLayout,
										  nil]];
		}
	}

	//	Reinitialize the configured list of audio tracks
	[self _clearAudioArray];

	if (NO == [myController hasValidPresetSelected]) {
		[self _ensureAtLeastOneNonEmptyTrackExists];
	}
	return;
}

#pragma mark -
#pragma mark KVC

- (unsigned int) countOfAudioArray

{
	return [audioArray count];
}

- (HBAudio *) objectInAudioArrayAtIndex: (unsigned int) index

{
	return [audioArray objectAtIndex: index];
}

- (void) insertObject: (HBAudio *) audioObject inAudioArrayAtIndex: (unsigned int) index;

{
	[audioArray insertObject: audioObject atIndex: index];
	return;
}

- (void) removeObjectFromAudioArrayAtIndex: (unsigned int) index

{
	[audioArray removeObjectAtIndex: index];
	return;
}

@end
