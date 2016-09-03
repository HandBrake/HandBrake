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

#define NONE_TRACK_INDEX 0

NSString *HBAudioChangedNotification = @"HBAudioChangedNotification";

@interface HBAudio () <HBAudioTrackDataSource, HBAudioTrackDelegate>
@end

@implementation HBAudio

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [super init];
    if (self)
    {
        _container = HB_MUX_MP4;

        _sourceTracks = [title.audioTracks mutableCopy];
        _tracks = [[NSMutableArray alloc] init];
        _defaults = [[HBAudioDefaults alloc] init];

        // Add the none and foreign track to the source array
        NSDictionary *none = @{keyAudioTrackName: NSLocalizedString(@"None", nil)};
        [_sourceTracks insertObject:none atIndex:0];
    }
    return self;
}

#pragma mark - Data Source

- (NSDictionary<NSString *, id> *)sourceTrackAtIndex:(NSUInteger)idx;
{
    return self.sourceTracks[idx];
}

- (NSArray<NSString *> *)sourceTracksArray
{
    NSMutableArray *sourceNames = [NSMutableArray array];

    for (NSDictionary *track in self.sourceTracks)
    {
        [sourceNames addObject:track[keyAudioTrackName]];
    }

    return sourceNames;
}

#pragma mark - Delegate

- (void)track:(HBAudioTrack *)track didChangeSourceFrom:(NSUInteger)oldSourceIdx;
{
    // If the source was changed to None, remove the track
    if (track.sourceTrackIdx == NONE_TRACK_INDEX)
    {
        NSUInteger idx = [self.tracks indexOfObject:track];
        [self removeObjectFromTracksAtIndex:idx];
    }
    // Else add a new None track
    else if (oldSourceIdx == NONE_TRACK_INDEX)
    {
        [self addNoneTrack];
    }
}

- (void)addNoneTrack
{
    HBAudioTrack *track = [self trackFromSourceTrackIndex:NONE_TRACK_INDEX];
    [self addTrack:track];
}

#pragma mark - Public methods

- (void)addAllTracks
{
    while (self.countOfTracks)
    {
        [self removeObjectFromTracksAtIndex:0];
    }

    // Add the remainings tracks
    for (NSUInteger idx = 1; idx < self.sourceTracksArray.count; idx++) {
        [self addTrack:[self trackFromSourceTrackIndex:idx]];
    }

    [self addNoneTrack];
}

- (void)removeAll
{
    while (self.countOfTracks)
    {
        [self removeObjectFromTracksAtIndex:0];
    }
    [self addNoneTrack];
}

- (void)reloadDefaults
{
    [self addTracksFromDefaults];
}

- (void)setContainer:(int)container
{
    _container = container;
    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        for (HBAudioTrack *track in self.tracks)
        {
            track.container = container;
        }

        // Update the Auto Passthru Fallback Codec Popup
        // lets get the tag of the currently selected item first so we might reset it later
        [self.defaults validateEncoderFallbackForVideoContainer:container];

        //[self validatePassthru];
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

/**
 *  Convenience method to add a track to tracks array.
 *
 *  @param track the track to add.
 */
- (void)addTrack:(HBAudioTrack *)newTrack
{
    [self insertObject:newTrack inTracksAtIndex:[self countOfTracks]];
}

/**
 *  Creates a new track dictionary from a source track.
 *
 *  @param index the index of the source track in the sourceTracks array
 */
- (HBAudioTrack *)trackFromSourceTrackIndex:(NSInteger)index
{
    HBAudioTrack *track = [[HBAudioTrack alloc] initWithTrackIdx:index container:self.container dataSource:self delegate:self];
    track.undo = self.undo;
    return track;
}

#pragma mark - Defaults

- (void)addTracksFromDefaults
{
    BOOL firstTrack = NO;
    BOOL allTracks = NO;
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];
    NSMutableIndexSet *tracksToAdd = [NSMutableIndexSet indexSet];

    // Reinitialize the configured list of audio tracks
    while (self.countOfTracks)
    {
        [self removeObjectFromTracksAtIndex:0];
    }

    if (self.defaults.trackSelectionBehavior != HBAudioTrackSelectionBehaviorNone)
    {
        // Add tracks of Default and Alternate Language by name
        for (NSString *languageISOCode in self.defaults.trackSelectionLanguages)
        {
            NSMutableIndexSet *tracksIndexes = [[self _tracksWithISOCode:languageISOCode
                                                         selectOnlyFirst:self.defaults.trackSelectionBehavior == HBAudioTrackSelectionBehaviorFirst] mutableCopy];
            [tracksIndexes removeIndexes:tracksAdded];
            if (tracksIndexes.count)
            {
                [self _processPresetAudioArray:self.defaults.tracksArray forTracks:tracksIndexes firstOnly:firstTrack];
                firstTrack = self.defaults.secondaryEncoderMode ? YES : NO;
                [tracksAdded addIndexes:tracksIndexes];
            }
        }

        // If no preferred Language was found, add standard track 1
        if (tracksAdded.count == 0 && self.sourceTracks.count > 1)
        {
            [tracksToAdd addIndex:1];
        }
    }

    // If all tracks should be added, add all track numbers that are not yet processed
    if (allTracks)
    {
        [tracksToAdd addIndexesInRange:NSMakeRange(1, self.sourceTracks.count - 1)];
        [tracksToAdd removeIndexes:tracksAdded];
    }

    if (tracksToAdd.count)
    {
        [self _processPresetAudioArray:self.defaults.tracksArray forTracks:tracksToAdd firstOnly:firstTrack];
    }

    // Add an None item
    [self addNoneTrack];
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
        HBAudioTrack *newAudio = [[HBAudioTrack alloc] initWithTrackIdx:trackIndex
                                                              container:self.container
                                                             dataSource:self
                                                               delegate:self];
        [newAudio setUndo:self.undo];

        [self insertObject:newAudio inTracksAtIndex:[self countOfTracks]];

        int inputCodec = [self.sourceTracks[trackIndex][keyAudioInputCodec] intValue];
        int outputCodec = preset.encoder;

        // Check if we need to use a fallback
         if (preset.encoder & HB_ACODEC_PASS_FLAG && !(preset.encoder & inputCodec & HB_ACODEC_PASS_MASK))
         {
             outputCodec = hb_audio_encoder_get_fallback_for_passthru(outputCodec);

             // If we couldn't find an encoder for the passthru format
             // fall back to the selected encoder fallback
             if (outputCodec == HB_ACODEC_INVALID)
             {
                 outputCodec = self.defaults.encoderFallback;
             }
         }

        int supportedMuxers = hb_audio_encoder_get_from_codec(outputCodec)->muxers;

        // If our preset wants us to support a codec that the track does not support,
        // instead of changing the codec we remove the audio instead.
        if (supportedMuxers & self.container)
        {
            newAudio.drc = preset.drc;
            newAudio.gain = preset.gain;
            newAudio.mixdown = preset.mixdown;
            newAudio.sampleRate = preset.sampleRate;
            newAudio.bitRate = preset.bitRate;
            newAudio.encoder = outputCodec;
        }
        else
        {
            [self removeObjectFromTracksAtIndex:[self countOfTracks] - 1];
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

    [self.sourceTracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
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

- (void)_processPresetAudioArray:(NSArray<HBAudioTrackPreset *> *)templateAudioArray forTracks:(NSIndexSet *)trackIndexes firstOnly:(BOOL)firstOnly
{
    __block BOOL firstTrack = firstOnly;
    [trackIndexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        // Add the track
        [self _processPresetAudioArray: self.defaults.tracksArray forTrack:idx firstOnly:firstTrack];
        firstTrack = self.defaults.secondaryEncoderMode ? YES : NO;
    }];
}

- (BOOL)anyCodecMatches:(int)codec
{
    BOOL retval = NO;
    NSUInteger audioArrayCount = [self countOfTracks];
    for (NSUInteger i = 0; i < audioArrayCount && !retval; i++)
    {
        HBAudioTrack *anAudio = [self objectInTracksAtIndex: i];
        if (anAudio.isEnabled && codec == anAudio.encoder)
        {
            retval = YES;
        }
    }
    return retval;
}

#pragma mark -
#pragma mark Notification Handling

- (void)encoderChanged
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBAudioChangedNotification object:self];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudio *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_container = _container;
        copy->_sourceTracks = [_sourceTracks mutableCopy];

        copy->_tracks = [[NSMutableArray alloc] init];
        [_tracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            if (idx < _tracks.count)
            {
                id trackCopy = [obj copy];
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
    [coder encodeInt:2 forKey:@"HBAudioVersion"];

    encodeInt(_container);
    encodeObject(_sourceTracks);
    encodeObject(_tracks);
    encodeObject(_defaults);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_container);
    decodeObject(_sourceTracks, NSMutableArray);
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
    [self addTracksFromDefaults];
}

#pragma mark -
#pragma mark KVC

- (NSUInteger)countOfTracks
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
