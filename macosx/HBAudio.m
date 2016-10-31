/*  HBAudio.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudio.h"

#import "HBJob.h"
#import "HBJob+HBJobConversion.m"
#import "HBTitle.h"
#import "HBAudioTrack.h"
#import "HBAudioTrackPreset.h"
#import "HBAudioDefaults.h"

#import "HBCodingUtilities.h"
#import "HBJob+Private.h"

#include "hb.h"

#define NONE_TRACK_INDEX 0

NSString *HBAudioChangedNotification = @"HBAudioChangedNotification";

@interface HBAudio () <HBAudioTrackDataSource, HBAudioTrackDelegate>

@property (nonatomic, readwrite, weak) HBJob *job;
@property (nonatomic, readwrite) int container;

@end

@implementation HBAudio

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _job = job;
        _container = HB_MUX_MP4;

        _sourceTracks = [job.title.audioTracks mutableCopy];
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
    [self addDefaultTracksFromJobSettings:self.job.jobDict];
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

- (void)addDefaultTracksFromJobSettings:(NSDictionary *)settings
{
    NSArray<NSDictionary<NSString *, id> *> *tracks = settings[@"Audio"][@"AudioList"];

    // Reinitialize the configured list of audio tracks
    while (self.countOfTracks)
    {
        [self removeObjectFromTracksAtIndex:0];
    }

    // Add the tracks
    for (NSDictionary *trackDict in tracks)
    {
        HBAudioTrack *track = [self trackFromSourceTrackIndex:[trackDict[@"Track"] unsignedIntegerValue] + 1];

        track.drc = [trackDict[@"DRC"] doubleValue];
        track.gain = [trackDict[@"Gain"] doubleValue];
        track.mixdown = hb_mixdown_get_from_name([trackDict[@"Mixdown"] UTF8String]);
        if ([trackDict[@"Samplerate"] isKindOfClass:[NSString class]])
        {
            int sampleRate = hb_audio_samplerate_get_from_name([trackDict[@"Samplerate"] UTF8String]);
            track.sampleRate = sampleRate == -1 ? 0 : sampleRate;
        }
        else
        {
            track.sampleRate = [trackDict[@"Samplerate"] intValue];
        }
        track.bitRate = [trackDict[@"Bitrate"] intValue];
        track.encoder = hb_audio_encoder_get_from_name([trackDict[@"Encoder"] UTF8String]);

        [self insertObject:track inTracksAtIndex:[self countOfTracks]];
    }

    // Add an None item
    [self addNoneTrack];
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

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    [self.defaults applyPreset:preset jobSettings:settings];
    [self addDefaultTracksFromJobSettings:settings];
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
