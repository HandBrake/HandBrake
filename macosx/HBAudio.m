/*  HBAudio.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudio.h"

#import "HBTitle.h"
#import "HBAudioTrack.h"
#import "HBAudioTrackPreset.h"
#import "HBAudioDefaults.h"

#import "HBCodingUtilities.h"

#include "hb.h"

NSString *HBAudioChangedNotification = @"HBAudioChangedNotification";

@interface HBAudio () <HBAudioTrackDataSource, HBAudioTrackDelegate>

@property (nonatomic, readonly, strong) NSDictionary *noneTrack;
@property (nonatomic, readonly, strong) NSArray *masterTrackArray;  // the master list of audio tracks from the title

@end

@implementation HBAudio

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [super init];
    if (self)
    {
        _container = HB_MUX_MP4;

        _tracks = [[NSMutableArray alloc] init];
        _defaults = [[HBAudioDefaults alloc] init];

        _noneTrack = @{keyAudioTrackIndex: @0,
                        keyAudioTrackName: NSLocalizedString(@"None", @"None"),
                        keyAudioInputCodec: @0};

        NSMutableArray *sourceTracks = [NSMutableArray array];
        [sourceTracks addObject:_noneTrack];
        [sourceTracks addObjectsFromArray:title.audioTracks];
        _masterTrackArray = [sourceTracks copy];

        [self switchingTrackFromNone: nil]; // this ensures there is a None track at the end of the list
    }
    return self;
}

- (void)addAllTracks
{
    [self addTracksFromDefaults:YES];
}

- (void)removeAll
{
    [self _clearAudioArray];
    [self switchingTrackFromNone:nil];
}

- (void)reloadDefaults
{
    [self addTracksFromDefaults:NO];
}

- (void)_clearAudioArray
{
    while (0 < [self countOfTracks])
    {
        [self removeObjectFromTracksAtIndex: 0];
    }
}

/**
 *  Uses the templateAudioArray from the preset to create the audios for the specified trackIndex.
 *
 *  @param templateAudioArray the track template.
 *  @param trackIndex         the index of the source track.
 *  @param firstOnly          use only the first track of the template or all.
 */
- (void)_processPresetAudioArray:(NSArray *)templateAudioArray forTrack:(NSUInteger)trackIndex firstOnly:(BOOL)firstOnly
{
    for (HBAudioTrackPreset *preset in templateAudioArray)
    {
        BOOL fallenBack = NO;
        HBAudioTrack *newAudio = [[HBAudioTrack alloc] init];
        [newAudio setDataSource:self];
        [newAudio setDelegate:self];
        [self insertObject: newAudio inTracksAtIndex: [self countOfTracks]];
        [newAudio setContainer:self.container];
        [newAudio setTrackFromIndex: (int)trackIndex];
        [newAudio setUndo:self.undo];

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
                    name = hb_audio_encoder_get_name(self.defaults.encoderFallback);
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
            [newAudio setDrc:preset.drc];
            [newAudio setGain:preset.gain];
        }
        else
        {
            [self removeObjectFromTracksAtIndex: [self countOfTracks] - 1];
        }

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
- (NSIndexSet *)_tracksWithISOCode:(NSString *)isoCode selectOnlyFirst:(BOOL)selectOnlyFirst
{
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];

    // We search for the prefix noting that our titles have the format %d: %s where the %s is the prefix
    [self.masterTrackArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
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

- (void)_processPresetAudioArray:(NSArray *)templateAudioArray forTracks:(NSIndexSet *)trackIndexes firstOnly:(BOOL)firstOnly
{
    __block BOOL firsTrack = firstOnly;
    [trackIndexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        // Add the track
        [self _processPresetAudioArray: self.defaults.tracksArray forTrack:idx firstOnly:firsTrack];
        firsTrack = self.defaults.secondaryEncoderMode ? YES : NO;
    }];
}

- (void)addTracksFromDefaults:(BOOL)allTracks
{
    BOOL firstTrack = NO;
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];
    NSMutableIndexSet *tracksToAdd = [NSMutableIndexSet indexSet];

    // Reinitialize the configured list of audio tracks
    [self _clearAudioArray];

    if (self.defaults.trackSelectionBehavior != HBAudioTrackSelectionBehaviorNone)
    {
        // Add tracks of Default and Alternate Language by name
        for (NSString *languageISOCode in self.defaults.trackSelectionLanguages)
        {
            NSMutableIndexSet *tracksIndexes = [[self _tracksWithISOCode: languageISOCode
                                                         selectOnlyFirst: self.defaults.trackSelectionBehavior == HBAudioTrackSelectionBehaviorFirst] mutableCopy];
            [tracksIndexes removeIndexes:tracksAdded];
            if (tracksIndexes.count)
            {
                [self _processPresetAudioArray:self.defaults.tracksArray forTracks:tracksIndexes firstOnly:firstTrack];
                firstTrack = self.defaults.secondaryEncoderMode ? YES : NO;
                [tracksAdded addIndexes:tracksIndexes];
            }
        }

        // If no preferred Language was found, add standard track 1
        if (tracksAdded.count == 0 && self.masterTrackArray.count > 1)
        {
            [tracksToAdd addIndex:1];
        }
    }

    // If all tracks should be added, add all track numbers that are not yet processed
    if (allTracks)
    {
        [tracksToAdd addIndexesInRange:NSMakeRange(1, self.masterTrackArray.count - 1)];
        [tracksToAdd removeIndexes:tracksAdded];
    }

    if (tracksToAdd.count)
    {
        [self _processPresetAudioArray:self.defaults.tracksArray forTracks:tracksToAdd firstOnly:firstTrack];
    }

    // Add an None item
    [self switchingTrackFromNone: nil];
}

- (BOOL)anyCodecMatches:(int)codec
{
    BOOL retval = NO;
    NSUInteger audioArrayCount = [self countOfTracks];
    for (NSUInteger i = 0; i < audioArrayCount && !retval; i++)
    {
        HBAudioTrack *anAudio = [self objectInTracksAtIndex: i];
        if ([anAudio enabled] && codec == [[anAudio codec][keyAudioCodec] intValue])
        {
            retval = YES;
        }
    }
    return retval;
}

- (void)addNewAudioTrack
{
    HBAudioTrack *newAudio = [[HBAudioTrack alloc] init];
    [newAudio setDataSource:self];
    [newAudio setDelegate:self];
    [self insertObject:newAudio inTracksAtIndex:[self countOfTracks]];
    [newAudio setContainer:self.container];
    [newAudio setTrack: self.noneTrack];
    [newAudio setUndo:self.undo];
}

#pragma mark -
#pragma mark Notification Handling

- (void)settingTrackToNone:(HBAudioTrack *)newNoneTrack
{
    // If this is not the last track in the array we need to remove it.  We then need to see if a new
    // one needs to be added (in the case when we were at maximum count and this switching makes it
    // so we are no longer at maximum.
    NSUInteger index = [self.tracks indexOfObject: newNoneTrack];

    if (NSNotFound != index && index < [self countOfTracks] - 1)
    {
        [self removeObjectFromTracksAtIndex: index];
    }
    [self switchingTrackFromNone: nil]; // see if we need to add one to the list
}

- (void)switchingTrackFromNone:(HBAudioTrack *)noLongerNoneTrack
{
    NSUInteger count = [self countOfTracks];
    BOOL needToAdd = NO;

    // If there is no last track that is None we add one.
    if (0 < count)
    {
        HBAudioTrack *lastAudio = [self objectInTracksAtIndex: count - 1];
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
- (void)setContainer:(int)container
{
    _container = container;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        // Update each of the instances because this value influences possible settings.
        for (HBAudioTrack *audioObject in self.tracks)
        {
            audioObject.container = container;
        }

        // Update the Auto Passthru Fallback Codec Popup
        // lets get the tag of the currently selected item first so we might reset it later
        [self.defaults validateEncoderFallbackForVideoContainer:container];
    }
}

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    for (HBAudioTrack *track in self.tracks)
    {
        track.undo = undo;
    }
    self.defaults.undo = undo;
}

- (void)setDefaults:(HBAudioDefaults *)defaults
{
    if (defaults != _defaults)
    {
        [[self.undo prepareWithInvocationTarget:self] setDefaults:_defaults];
    }
    _defaults = defaults;
    _defaults.undo = self.undo;
}

- (void)mixdownChanged
{
    [[NSNotificationCenter defaultCenter] postNotificationName: HBAudioChangedNotification object: self];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudio *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_container = _container;

        copy->_noneTrack = [_noneTrack copy];
        copy->_masterTrackArray = [_masterTrackArray copy];

        copy->_tracks = [[NSMutableArray alloc] init];
        [_tracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            if (idx < _tracks.count)
            {
                HBAudioTrack *trackCopy = [obj copy];
                trackCopy.dataSource = copy;
                trackCopy.delegate = copy;
                [copy->_tracks addObject:trackCopy];
            }
        }];

        copy->_defaults = [_defaults copy];
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
    [coder encodeInt:1 forKey:@"HBAudioVersion"];

    encodeInt(_container);

    encodeObject(_noneTrack);
    encodeObject(_masterTrackArray);
    encodeObject(_tracks);

    encodeObject(_defaults);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_container);

    decodeObject(_noneTrack, NSDictionary);
    decodeObject(_masterTrackArray, NSArray);
    decodeObject(_tracks, NSMutableArray);

    for (HBAudioTrack *track in _tracks)
    {
        track.dataSource = self;
        track.delegate = self;
    }

    decodeObject(_defaults, HBAudioDefaults);

    return self;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset
{
    [self.defaults writeToPreset:preset];
}

- (void)applyPreset:(HBPreset *)preset
{
    [self.defaults applyPreset:preset];
    [self addTracksFromDefaults:NO];
}

#pragma mark -
#pragma mark KVC

- (NSUInteger) countOfTracks
{
    return self.tracks.count;
}

- (HBAudioTrack *)objectInTracksAtIndex:(NSUInteger)index
{
    return self.tracks[index];
}

- (void)insertObject:(HBAudioTrack *)track inTracksAtIndex:(NSUInteger)index;
{
    [[self.undo prepareWithInvocationTarget:self] removeObjectFromTracksAtIndex:index];
    [self.tracks insertObject:track atIndex:index];
}

- (void)removeObjectFromTracksAtIndex:(NSUInteger)index
{
    HBAudioTrack *track = self.tracks[index];
    [[self.undo prepareWithInvocationTarget:self] insertObject:track inTracksAtIndex:index];
    [self.tracks removeObjectAtIndex:index];
}

@end
