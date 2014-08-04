//
//  HBAudioController.m
//  HandBrake
//
//  Created on 2010-08-24.
//

#import "HBAudioController.h"
#import "Controller.h"
#import "HBAudio.h"
#import "HBAudioSettings.h"
#import "HBAudioDefaultsController.h"
#import "HBAudioTrackPreset.h"
#import "hb.h"
#include "lang.h"

NSString *keyAudioTrackIndex = @"keyAudioTrackIndex";
NSString *keyAudioTrackName = @"keyAudioTrackName";
NSString *keyAudioInputBitrate = @"keyAudioInputBitrate";
NSString *keyAudioInputSampleRate = @"keyAudioInputSampleRate";
NSString *keyAudioInputCodec = @"keyAudioInputCodec";
NSString *keyAudioInputCodecParam = @"keyAudioInputCodecParam";
NSString *keyAudioInputChannelLayout = @"keyAudioInputChannelLayout";
NSString *keyAudioTrackLanguageIsoCode = @"keyAudioTrackLanguageIsoCode";

NSString *HBMixdownChangedNotification = @"HBMixdownChangedNotification";


@interface HBAudioController () {
    IBOutlet NSTableView         * fTableView;

    NSMutableArray               * audioArray;        // the configured audio information
    NSArray                      * masterTrackArray;  // the master list of audio tracks from the title
    NSDictionary                 * noneTrack;         // this represents no audio track selection
}

@property (assign) IBOutlet NSPopUpButton *trackPopup;
@property (assign) IBOutlet NSButton *configureDefaults;
@property (assign) IBOutlet NSButton *reloadDefaults;
@property (nonatomic, readwrite) BOOL enabled;

@property (nonatomic, readwrite, retain) NSArray *masterTrackArray;
@property (nonatomic, retain) NSNumber *videoContainerTag; // initially is the default HB_MUX_MP4

// Defaults
@property (nonatomic, readwrite, retain) HBAudioDefaultsController *defaultsController;
@property (nonatomic, readwrite, retain) HBAudioSettings *settings;

@end

@implementation HBAudioController

#pragma mark -
#pragma mark Accessors

@synthesize masterTrackArray;
@synthesize noneTrack;
@synthesize videoContainerTag;

- (instancetype)init
{
    self = [super initWithNibName:@"Audio" bundle:nil];
    if (self)
    {
        [self setVideoContainerTag: [NSNumber numberWithInt: HB_MUX_MP4]];
        audioArray = [[NSMutableArray alloc] init];

        /* register that we are interested in changes made to the video container */
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserver: self selector: @selector(containerChanged:) name: HBContainerChangedNotification object: nil];
        [center addObserver: self selector: @selector(titleChanged:) name: HBTitleChangedNotification object: nil];
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
}

- (IBAction)addAllAudioTracks:(id)sender
{
    [self addTracksFromDefaults:YES];
}

- (IBAction)removeAll:(id)sender
{
    [self _clearAudioArray];
    [self switchingTrackFromNone:nil];
}

- (void)enableUI:(BOOL)b
{
    self.enabled = b;
    [fTableView setEnabled:b];
    [self.trackPopup setEnabled:b];
    [self.configureDefaults setEnabled:b];
    [self.reloadDefaults setEnabled:b];
}

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    return self.enabled;
}

#pragma mark -
#pragma mark HBController Support

- (NSArray *)audioTracks

{
    NSMutableArray *tracksArray = [NSMutableArray array];

    NSUInteger audioArrayCount = [self countOfAudioArray];
    for (NSUInteger counter = 0; counter < audioArrayCount; counter++)
    {
        HBAudio *anAudio = [self objectInAudioArrayAtIndex: counter];
        if ([anAudio enabled])
        {
            NSMutableDictionary *dict = [NSMutableDictionary dictionary];
            NSNumber *sampleRateToUse = ([anAudio.sampleRate[keyAudioSamplerate] intValue] == 0 ?
                                         anAudio.track[keyAudioInputSampleRate] :
                                         anAudio.sampleRate[keyAudioSamplerate]);

            dict[@"Track"]            = @([anAudio.track[keyAudioTrackIndex] intValue] -1);
            dict[@"TrackDescription"] = anAudio.track[keyAudioTrackName];
            dict[@"Encoder"]          = anAudio.codec[keyAudioCodecName];
            dict[@"Mixdown"]          = anAudio.mixdown[keyAudioMixdownName];
            dict[@"Samplerate"]       = anAudio.sampleRate[keyAudioSampleRateName];
            dict[@"Bitrate"]          = anAudio.bitRate[keyAudioBitrateName];

            // output is not passthru so apply gain
            if (!([[anAudio codec][keyAudioCodec] intValue] & HB_ACODEC_PASS_FLAG))
            {
                dict[@"TrackGainSlider"] = anAudio.gain;
            }
            else
            {
                // output is passthru - the Gain dial is disabled so don't apply its value
                dict[@"TrackGainSlider"] = @0;
            }

            if (hb_audio_can_apply_drc([anAudio.track[keyAudioInputCodec] intValue],
                                       [anAudio.track[keyAudioInputCodecParam] intValue],
                                       [anAudio.codec[keyAudioCodec] intValue]))
            {
                dict[@"TrackDRCSlider"] = anAudio.drc;
            }
            else
            {
                // source isn't AC3 or output is passthru - the DRC dial is disabled so don't apply its value
                dict[@"TrackDRCSlider"] = @0;
            }

            dict[@"JobEncoder"]    = anAudio.codec[keyAudioCodec];
            dict[@"JobMixdown"]    = anAudio.mixdown[keyAudioMixdown];
            dict[@"JobSamplerate"] = sampleRateToUse;
            dict[@"JobBitrate"]    = anAudio.bitRate[keyAudioBitrate];

            [tracksArray addObject:dict];
        }
    }

    return tracksArray;
}

- (void) addTracksFromQueue: (NSMutableDictionary *) aQueue

{
    // Reinitialize the configured list of audio tracks
    [self _clearAudioArray];

    // The following is the pattern to follow, but with Audio%dTrack being the key to seek...
    // Can we assume that there will be no skip in the data?
    for (NSDictionary *audioDict in [aQueue objectForKey:@"AudioList"])
    {
        HBAudio *newAudio = [[HBAudio alloc] init];
        [newAudio setController: self];
        [self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
        [newAudio setVideoContainerTag: [self videoContainerTag]];
        [newAudio setTrackFromIndex: audioDict[@"Track"]];
        [newAudio setCodecFromName: audioDict[@"Encoder"]];
        [newAudio setMixdownFromName: audioDict[@"Mixdown"]];
        [newAudio setSampleRateFromName: audioDict[@"Samplerate"]];
        [newAudio setBitRateFromName: audioDict[@"Bitrate"]];
        [newAudio setDrc: audioDict[@"TrackDRCSlider"]];
        [newAudio setGain: audioDict[@"TrackGainSlider"]];
        [newAudio release];
    }

    [self switchingTrackFromNone: nil]; // see if we need to add one to the list
}

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    self.settings = [[[HBAudioSettings alloc] init] autorelease];
    [self.settings applySettingsFromPreset:preset];
    [self.settings validateEncoderFallbackForVideoContainer:[self.videoContainerTag intValue]];

    [self addTracksFromDefaults:NO];
}

- (void) _clearAudioArray
{
    while (0 < [self countOfAudioArray])
    {
        [self removeObjectFromAudioArrayAtIndex: 0];
    }
}

/**
 *  Uses the templateAudioArray from the preset to create the audios for the specified trackIndex.
 *
 *  @param templateAudioArray the track template.
 *  @param trackIndex         the index of the source track.
 *  @param firstOnly          use only the first track of the template or all.
 */
- (void) _processPresetAudioArray: (NSArray *) templateAudioArray forTrack: (NSUInteger) trackIndex firstOnly: (BOOL) firstOnly

{
    for (HBAudioTrackPreset *preset in templateAudioArray)
    {
        BOOL fallenBack = NO;
        HBAudio *newAudio = [[HBAudio alloc] init];
        [newAudio setController: self];
        [self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
        [newAudio setVideoContainerTag: [self videoContainerTag]];
        [newAudio setTrackFromIndex: (int)trackIndex];
        
        const char *name = hb_audio_encoder_get_name(preset.encoder);
        NSString *audioEncoder = nil;
        
        // Check if we need to use a fallback
        if (name)
        {
            audioEncoder = @(name);
            if (preset.encoder & HB_ACODEC_PASS_FLAG &&
                ![newAudio setCodecFromName:audioEncoder])
            {
                int passthru, fallback;
                fallenBack = YES;
                passthru   = hb_audio_encoder_get_from_name([audioEncoder UTF8String]);
                fallback   = hb_audio_encoder_get_fallback_for_passthru(passthru);
                name       = hb_audio_encoder_get_name(fallback);
                
                // If we couldn't find an encoder for the passthru format
                // fall back to the selected encoder fallback
                if (name == NULL)
                {
                    name = hb_audio_encoder_get_name(self.settings.encoderFallback);
                }
            }
            else
            {
                name = hb_audio_encoder_sanitize_name([audioEncoder UTF8String]);
            }
            audioEncoder = @(name);
        }
        
        // If our preset wants us to support a codec that the track does not support, instead
        // of changing the codec we remove the audio instead.
        if ([newAudio setCodecFromName:audioEncoder])
        {
            const char *mixdown = hb_mixdown_get_name(preset.mixdown);
            if (mixdown)
            {
                [newAudio setMixdownFromName: @(mixdown)];
            }
            
            const char *sampleRateName = hb_audio_samplerate_get_name(preset.sampleRate);
            if (!sampleRateName)
            {
                [newAudio setSampleRateFromName: @"Auto"];
            }
            else
            {
                [newAudio setSampleRateFromName: @(sampleRateName)];
            }
            if (!fallenBack)
            {
                [newAudio setBitRateFromName: [NSString stringWithFormat:@"%d", preset.bitRate]];
            }
            [newAudio setDrc: @(preset.drc)];
            [newAudio setGain: @(preset.gain)];
        }
        else
        {
            [self removeObjectFromAudioArrayAtIndex: [self countOfAudioArray] - 1];
        }
        [newAudio release];
        
        if (firstOnly)
        {
            break;
        }
    }
}

/**
 *  Matches the source audio tracks with the specific language iso code.
 *
 *  @param isoCode         the iso code to match.
 *  @param selectOnlyFirst whether to match only the first track for the iso code.
 *
 *  @return a NSIndexSet with the index of the matched tracks.
 */
- (NSIndexSet *) _tracksWithISOCode: (NSString *) isoCode selectOnlyFirst: (BOOL) selectOnlyFirst
{
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
    
    // We search for the prefix noting that our titles have the format %d: %s where the %s is the prefix
    [masterTrackArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        if (idx) // Note that we skip the "None" track
        {
            if ([isoCode isEqualToString:@"und"] ||  [obj[keyAudioTrackLanguageIsoCode] isEqualToString:isoCode])
            {
                [indexes addIndex:idx];
                
                if (selectOnlyFirst)
                {
                    *stop = YES;
                }
            }
        }
    }];

    return indexes;
}

- (void) _processPresetAudioArray: (NSArray *) templateAudioArray forTracks: (NSIndexSet *) trackIndexes firstOnly: (BOOL) firstOnly
{
    __block BOOL firsTrack = firstOnly;
    [trackIndexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        // Add the track
        [self _processPresetAudioArray: self.settings.tracksArray forTrack:idx firstOnly:firsTrack];
        firsTrack = self.settings.secondaryEncoderMode ? YES : NO;
    }];
}

- (void) addTracksFromDefaults: (BOOL) allTracks

{
    BOOL firstTrack = NO;
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];
    NSMutableIndexSet *tracksToAdd = [NSMutableIndexSet indexSet];

    // Reinitialize the configured list of audio tracks
    [self _clearAudioArray];

    if (self.settings.trackSelectionBehavior != HBAudioTrackSelectionBehaviorNone)
    {
        // Add tracks of Default and Alternate Language by name
        for (NSString *languageISOCode in self.settings.trackSelectionLanguages)
        {
            NSMutableIndexSet *tracksIndexes = [[self _tracksWithISOCode: languageISOCode
                                                 selectOnlyFirst: self.settings.trackSelectionBehavior == HBAudioTrackSelectionBehaviorFirst] mutableCopy];
            [tracksIndexes removeIndexes:tracksAdded];
            if (tracksIndexes.count)
            {
                [self _processPresetAudioArray:self.settings.tracksArray forTracks:tracksIndexes firstOnly:firstTrack];
                firstTrack = self.settings.secondaryEncoderMode ? YES : NO;
                [tracksAdded addIndexes:tracksIndexes];
            }
            [tracksIndexes release];
        }

        // If no preferred Language was found, add standard track 1
        if (tracksAdded.count == 0 && masterTrackArray.count > 1)
        {
            [tracksToAdd addIndex:1];
        }
    }

    // If all tracks should be added, add all track numbers that are not yet processed
    if (allTracks)
    {
        [tracksToAdd addIndexesInRange:NSMakeRange(1, masterTrackArray.count - 1)];
        [tracksToAdd removeIndexes:tracksAdded];
    }

    if (tracksToAdd.count)
    {
        [self _processPresetAudioArray:self.settings.tracksArray forTracks:tracksToAdd firstOnly:firstTrack];
    }

    // Add an None item
    [self switchingTrackFromNone: nil];
}

- (BOOL) anyCodecMatches: (int) aCodecValue

{
    BOOL retval = NO;
    NSUInteger audioArrayCount = [self countOfAudioArray];
    for (NSUInteger i = 0; i < audioArrayCount && !retval; i++)
    {
        HBAudio *anAudio = [self objectInAudioArrayAtIndex: i];
        if ([anAudio enabled] && aCodecValue == [[[anAudio codec] objectForKey: keyAudioCodec] intValue])
        {
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
    [newAudio setGain: [NSNumber numberWithFloat: 0.0]];
    [newAudio release];
}

#pragma mark -
#pragma mark Notification Handling

- (void) settingTrackToNone: (HBAudio *) newNoneTrack

{
    // If this is not the last track in the array we need to remove it.  We then need to see if a new
    // one needs to be added (in the case when we were at maximum count and this switching makes it
    // so we are no longer at maximum.
    NSUInteger index = [audioArray indexOfObject: newNoneTrack];

    if (NSNotFound != index && index < [self countOfAudioArray] - 1)
    {
        [self removeObjectFromAudioArrayAtIndex: index];
    }
    [self switchingTrackFromNone: nil]; // see if we need to add one to the list
}

- (void) switchingTrackFromNone: (HBAudio *) noLongerNoneTrack

{
    NSUInteger count = [self countOfAudioArray];
    BOOL needToAdd = NO;

    // If there is no last track that is None we add one.
    if (0 < count)
    {
        HBAudio *lastAudio = [self objectInAudioArrayAtIndex: count - 1];
        if ([lastAudio enabled])
        {
            needToAdd = YES;
        }
    }
    else
    {
        needToAdd = YES;
    }

    if (needToAdd)
    {
        [self addNewAudioTrack];
    }
}

// This gets called whenever the video container changes.
- (void) containerChanged: (NSNotification *) aNotification

{
    NSDictionary *notDict = [aNotification userInfo];

    [self setVideoContainerTag: [notDict objectForKey: keyContainerTag]];

    // Update each of the instances because this value influences possible settings.
    for (HBAudio *audioObject in audioArray)
    {
        [audioObject setVideoContainerTag: [self videoContainerTag]];
    }

    // Update the Auto Passthru Fallback Codec Popup
    // lets get the tag of the currently selected item first so we might reset it later
    [self.settings validateEncoderFallbackForVideoContainer:[self.videoContainerTag intValue]];
}

- (void) titleChanged: (NSNotification *) aNotification

{
    NSDictionary *notDict = [aNotification userInfo];
    NSData *theData = [notDict objectForKey: keyTitleTag];
    hb_title_t *title = NULL;

    // Reinitialize the configured list of audio tracks
    [self _clearAudioArray];
    
    [theData getBytes: &title length: sizeof(title)];
    if (title)
    {
        hb_audio_config_t *audio;
        hb_list_t *list = title->list_audio;
        int i, count = hb_list_count(list);

        // Reinitialize the master list of available audio tracks from this title
        NSMutableArray *newTrackArray = [NSMutableArray array];
        [noneTrack release];
        noneTrack = [[NSDictionary dictionaryWithObjectsAndKeys:
                      [NSNumber numberWithInt: 0], keyAudioTrackIndex,
                      NSLocalizedString(@"None", @"None"), keyAudioTrackName,
                      [NSNumber numberWithInt: 0], keyAudioInputCodec,
                      nil] retain];
        [newTrackArray addObject: noneTrack];
        for (i = 0; i < count; i++)
        {
            audio = (hb_audio_config_t *) hb_list_audio_config_item(list, i);
            [newTrackArray addObject: [NSDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithInt: i + 1], keyAudioTrackIndex,
                                       [NSString stringWithFormat: @"%d: %s", i, audio->lang.description], keyAudioTrackName,
                                       [NSNumber numberWithInt: audio->in.bitrate / 1000], keyAudioInputBitrate,
                                       [NSNumber numberWithInt: audio->in.samplerate], keyAudioInputSampleRate,
                                       [NSNumber numberWithInt: audio->in.codec], keyAudioInputCodec,
                                       [NSNumber numberWithInt: audio->in.codec_param], keyAudioInputCodecParam,
                                       [NSNumber numberWithUnsignedLongLong: audio->in.channel_layout], keyAudioInputChannelLayout,
                                       @(audio->lang.iso639_2), keyAudioTrackLanguageIsoCode,
                                       nil]];
        }
        self.masterTrackArray = newTrackArray;
        [self switchingTrackFromNone: nil]; // this ensures there is a None track at the end of the list
    }
    else
    {
        self.masterTrackArray = nil;
    }

}

#pragma mark - Defaults

- (IBAction)showSettingsSheet:(id)sender
{
    self.defaultsController = [[[HBAudioDefaultsController alloc] initWithSettings:self.settings] autorelease];
    self.defaultsController.delegate = self;

	[NSApp beginSheet:[self.defaultsController window]
       modalForWindow:[[self view] window]
        modalDelegate:self
       didEndSelector:NULL
          contextInfo:NULL];
}

- (void)sheetDidEnd
{
    self.defaultsController = nil;
}

- (IBAction)reloadDefaults:(id)sender
{
    [self addTracksFromDefaults:NO];
}

#pragma mark -
#pragma mark KVC

- (NSUInteger) countOfAudioArray

{
    return [audioArray count];
}

- (HBAudio *) objectInAudioArrayAtIndex: (NSUInteger) index

{
    return [audioArray objectAtIndex: index];
}

- (void) insertObject: (HBAudio *) audioObject inAudioArrayAtIndex: (NSUInteger) index;

{
    [audioArray insertObject: audioObject atIndex: index];
}

- (void) removeObjectFromAudioArrayAtIndex: (NSUInteger) index

{
    [audioArray removeObjectAtIndex: index];
}

@end

