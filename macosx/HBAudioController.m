//
//  HBAudioController.m
//  HandBrake
//
//  Created on 2010-08-24.
//

#import "HBAudioController.h"
#import "HBAudioTrack.h"
#import "HBAudioDefaultsController.h"

#import "HBJob.h"

#import "hb.h"
#include "lang.h"

@interface HBAudioController () {
    IBOutlet NSTableView         * fTableView;

    NSMutableArray               * audioArray;        // the configured audio information
    NSArray                      * masterTrackArray;  // the master list of audio tracks from the title
    NSDictionary                 * noneTrack;         // this represents no audio track selection
}

@property (assign) IBOutlet NSPopUpButton *trackPopup;
@property (assign) IBOutlet NSButton *configureDefaults;
@property (assign) IBOutlet NSButton *reloadDefaults;

@property (nonatomic, readwrite, retain) NSArray *masterTrackArray;
@property (nonatomic, retain) NSNumber *videoContainerTag; // initially is the default HB_MUX_MP4

// Defaults
@property (nonatomic, readwrite, retain) HBAudioDefaultsController *defaultsController;
@property (nonatomic, readwrite, retain) HBAudioDefaults *settings;

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
        [self setVideoContainerTag: @HB_MUX_MP4];
        audioArray = [[NSMutableArray alloc] init];

        /* register that we are interested in changes made to the video container */
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserver: self selector: @selector(containerChanged:) name: HBContainerChangedNotification object: nil];
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

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    return (self.job != nil);
}

#pragma mark -
#pragma mark HBController Support

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
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
        HBAudioTrack *newAudio = [[HBAudioTrack alloc] init];
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
        HBAudioTrack *anAudio = [self objectInAudioArrayAtIndex: i];
        if ([anAudio enabled] && aCodecValue == [[anAudio codec][keyAudioCodec] intValue])
        {
            retval = YES;
        }
    }
    return retval;
}

- (void) addNewAudioTrack

{
    HBAudioTrack *newAudio = [[HBAudioTrack alloc] init];
    [newAudio setController: self];
    [self insertObject: newAudio inAudioArrayAtIndex: [self countOfAudioArray]];
    [newAudio setVideoContainerTag: [self videoContainerTag]];
    [newAudio setTrack: noneTrack];
    [newAudio setDrc: @0.0f];
    [newAudio setGain: @0.0f];
    [newAudio release];
}

#pragma mark -
#pragma mark Notification Handling

- (void) settingTrackToNone: (HBAudioTrack *) newNoneTrack

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

- (void) switchingTrackFromNone: (HBAudioTrack *) noLongerNoneTrack

{
    NSUInteger count = [self countOfAudioArray];
    BOOL needToAdd = NO;

    // If there is no last track that is None we add one.
    if (0 < count)
    {
        HBAudioTrack *lastAudio = [self objectInAudioArrayAtIndex: count - 1];
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

    [self setVideoContainerTag: notDict[keyContainerTag]];

    // Update each of the instances because this value influences possible settings.
    for (HBAudioTrack *audioObject in audioArray)
    {
        [audioObject setVideoContainerTag: [self videoContainerTag]];
    }

    // Update the Auto Passthru Fallback Codec Popup
    // lets get the tag of the currently selected item first so we might reset it later
    [self.settings validateEncoderFallbackForVideoContainer:[self.videoContainerTag intValue]];
}

- (void)setJob:(HBJob *)job

{
    // Reinitialize the configured list of audio tracks
    [self _clearAudioArray];
    
    if (job)
    {
        _job = job;
        [audioArray release];
        audioArray = [job.audioTracks retain];
        self.settings = job.audioDefaults;

        // Reinitialize the master list of available audio tracks from this title
        NSMutableArray *newTrackArray = [NSMutableArray array];
        [noneTrack release];
        noneTrack = [@{keyAudioTrackIndex: @0,
                       keyAudioTrackName: NSLocalizedString(@"None", @"None"),
                       keyAudioInputCodec: @0} retain];
        [newTrackArray addObject: noneTrack];
        [newTrackArray addObjectsFromArray:job.title.audioTracks];
        self.masterTrackArray = newTrackArray;

        // Readd the controller reference to the audio tracks.
        for (HBAudioTrack *audioTrack in audioArray)
        {
            audioTrack.controller = self;
        }

        [self switchingTrackFromNone: nil]; // this ensures there is a None track at the end of the list
    }
    else
    {
        _job = nil;
        [audioArray release];
        audioArray = nil;
        self.settings = nil;
        self.masterTrackArray = nil;
    }

}

#pragma mark - Defaults

- (IBAction)showSettingsSheet:(id)sender
{
    self.defaultsController = [[[HBAudioDefaultsController alloc] initWithSettings:self.settings] autorelease];

	[NSApp beginSheet:[self.defaultsController window]
       modalForWindow:[[self view] window]
        modalDelegate:self
       didEndSelector:@selector(sheetDidEnd)
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

- (HBAudioTrack *) objectInAudioArrayAtIndex: (NSUInteger) index

{
    return audioArray[index];
}

- (void) insertObject: (HBAudioTrack *) audioObject inAudioArrayAtIndex: (NSUInteger) index;

{
    [audioArray insertObject: audioObject atIndex: index];
}

- (void) removeObjectFromAudioArrayAtIndex: (NSUInteger) index

{
    [audioArray removeObjectAtIndex: index];
}

@end

