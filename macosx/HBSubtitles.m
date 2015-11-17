/*  HBSubtitles.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitles.h"
#import "HBSubtitlesDefaults.h"

#import "HBSubtitlesTrack.h"

#import "HBTitle.h"
#import "HBCodingUtilities.h"

#include "common.h"

extern NSString *keySubTrackName;
extern NSString *keySubTrackLanguageIsoCode;
extern NSString *keySubTrackType;

extern NSString *keySubTrackSrtFileURL;

#define NONE_TRACK_INDEX        0
#define FOREIGN_TRACK_INDEX     1

@interface HBSubtitles () <HBTrackDataSource, HBTrackDelegate>

/// Used to aovid circular dependecy validation.
@property (nonatomic, readwrite) BOOL validating;

@end

@implementation HBSubtitles

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [super init];
    if (self)
    {
        _container = HB_MUX_MP4;

        _sourceTracks = [title.subtitlesTracks mutableCopy];
        _tracks = [[NSMutableArray alloc] init];
        _defaults = [[HBSubtitlesDefaults alloc] init];

        NSMutableSet<NSString *> *forcedSourceNamesArray = [NSMutableSet set];
        int foreignAudioType = VOBSUB;
        for (NSDictionary *dict in _sourceTracks)
        {
            enum subsource source = [dict[keySubTrackType] intValue];
            NSString *name = @(hb_subsource_name(source));
            // if the subtitle track can be forced, add its source name to the array
            if (hb_subtitle_can_force(source) && name.length)
            {
                [forcedSourceNamesArray addObject:name];
            }
        }

        // now set the name of the Foreign Audio Search track
        NSMutableString *foreignAudioSearchTrackName = [@"Foreign Audio Search (Bitmap)" mutableCopy];
        if (forcedSourceNamesArray.count)
        {
            [foreignAudioSearchTrackName appendFormat:@" ("];
            for (NSString *name in forcedSourceNamesArray)
            {
                [foreignAudioSearchTrackName appendFormat:@"%@, ", name];
            }
            [foreignAudioSearchTrackName deleteCharactersInRange:NSMakeRange(foreignAudioSearchTrackName.length - 2, 2)];
            [foreignAudioSearchTrackName appendFormat:@")"];
        }

        // Add the none and foreign track to the source array
        NSDictionary *none = @{  keySubTrackName: NSLocalizedString(@"None", nil)};
        [_sourceTracks insertObject:none atIndex:0];

        NSDictionary *foreign = @{ keySubTrackName: foreignAudioSearchTrackName,
                                   keySubTrackType: @(foreignAudioType) };
        [_sourceTracks insertObject:foreign atIndex:1];

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
        [sourceNames addObject:track[keySubTrackName]];
    }

    return sourceNames;
}

#pragma mark - Delegate

- (void)track:(HBSubtitlesTrack *)track didChangeSourceFrom:(NSUInteger)oldSourceIdx;
{
    // If the source was changed to None, remove the track
    if (track.sourceTrackIdx == NONE_TRACK_INDEX)
    {
        NSUInteger idx = [self.tracks indexOfObject:track];
        [self removeObjectFromTracksAtIndex:idx];
    }
    // If the source was changed to Foreign Audio Track,
    // insert it at top if it wasn't already there
    else if (track.sourceTrackIdx == FOREIGN_TRACK_INDEX)
    {
        NSUInteger idx = [self.tracks indexOfObject:track];
        if (idx != 0)
        {
            [self removeObjectFromTracksAtIndex:idx];
            if (self.tracks[0].sourceTrackIdx != FOREIGN_TRACK_INDEX)
            {
                [self insertObject:track inTracksAtIndex:0];
            }
        }
        [self addNoneTrack];
    }
    // Else add a new None track
    else if (oldSourceIdx == NONE_TRACK_INDEX)
    {
        [self addNoneTrack];
    }
    [self validatePassthru];
}

- (BOOL)canSetBurnedInOption:(HBSubtitlesTrack *)track
{
    BOOL result = YES;
    for (HBSubtitlesTrack *subTrack in self.tracks)
    {
        if (subTrack != track && subTrack.isEnabled
            && subTrack.sourceTrackIdx > FOREIGN_TRACK_INDEX && !subTrack.canPassthru)
        {
            result = NO;
        }
    }
    return result;
}

- (void)didSetBurnedInOption:(HBSubtitlesTrack *)track
{
    if (self.validating == NO && track.sourceTrackIdx != FOREIGN_TRACK_INDEX)
    {
        self.validating = YES;
        NSUInteger idx = [self.tracks indexOfObject:track];
        [self validateBurned:idx];
        self.validating = NO;
    }
}

- (void)didSetDefaultOption:(HBSubtitlesTrack *)track
{
    if (self.validating == NO && track.sourceTrackIdx != FOREIGN_TRACK_INDEX)
    {
        self.validating = YES;
        NSUInteger idx = [self.tracks indexOfObject:track];
        [self validateDefault:idx];
        self.validating = NO;
    }
}

- (void)addNoneTrack
{
    HBSubtitlesTrack *track = [self trackFromSourceTrackIndex:NONE_TRACK_INDEX];
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
    [self validatePassthru];
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

- (void)addSrtTrackFromURL:(NSURL *)srtURL
{
    // Create a new entry for the subtitle source array so it shows up in our subtitle source list
    [self.sourceTracks addObject:@{keySubTrackName: srtURL.lastPathComponent,
                                   keySubTrackType: @(SRTSUB),
                                   keySubTrackSrtFileURL: srtURL}];
    HBSubtitlesTrack *track = [self trackFromSourceTrackIndex:self.sourceTracksArray.count - 1];
    [self insertObject:track inTracksAtIndex:[self countOfTracks] - 1];
}

- (void)setContainer:(int)container
{
    _container = container;
    for (HBSubtitlesTrack *track in self.tracks)
    {
        track.container = container;
    }
    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validatePassthru];
    }
}

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    for (HBSubtitlesTrack *track in self.tracks)
    {
        track.undo = undo;
    }
    self.defaults.undo = undo;
}

- (void)setDefaults:(HBSubtitlesDefaults *)defaults
{
    if (defaults != _defaults)
    {
        [[self.undo prepareWithInvocationTarget:self] setDefaults:_defaults];
    }
    _defaults = defaults;
    _defaults.undo = self.undo;
}

/**
 *  Convenience method to add a track to subtitlesArray.
 *
 *  @param track the track to add.
 */
- (void)addTrack:(HBSubtitlesTrack *)newTrack
{
    [self insertObject:newTrack inTracksAtIndex:[self countOfTracks]];
}

/**
 *  Creates a new track dictionary from a source track.
 *
 *  @param index the index of the source track in the subtitlesSourceArray
 */
- (HBSubtitlesTrack *)trackFromSourceTrackIndex:(NSInteger)index
{
    HBSubtitlesTrack *track = [[HBSubtitlesTrack alloc] initWithTrackIdx:index container:self.container
                                                              dataSource:self delegate:self];
    track.undo = self.undo;
    return track;
}

#pragma mark - Defaults

/**
 *  Remove all the subtitles tracks and
 *  add new ones based on the defaults settings
 */
- (IBAction)addTracksFromDefaults
{
    // Keeps a set of the indexes of the added track
    // so we don't add the same track twice.
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];

    while (self.countOfTracks)
    {
        [self removeObjectFromTracksAtIndex:0];
    }

    // Add the foreign audio search pass
    if (self.defaults.addForeignAudioSearch)
    {
        [self addTrack:[self trackFromSourceTrackIndex:FOREIGN_TRACK_INDEX]];
    }

    // Add the tracks for the selected languages
    if (self.defaults.trackSelectionBehavior != HBSubtitleTrackSelectionBehaviorNone)
    {
        for (NSString *lang in self.defaults.trackSelectionLanguages)
        {
            NSUInteger idx = 0;
            for (NSDictionary *track in self.sourceTracks)
            {
                if (idx > FOREIGN_TRACK_INDEX &&
                    ([lang isEqualToString:@"und"] || [track[keySubTrackLanguageIsoCode] isEqualToString:lang]))
                {
                    if (![tracksAdded containsIndex:idx])
                    {
                        [self addTrack:[self trackFromSourceTrackIndex:idx]];
                    }
                    [tracksAdded addIndex:idx];

                    if (self.defaults.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                    {
                        break;
                    }
                }
                idx++;
            }
        }
    }

    // Add the closed captions track if there is one.
    if (self.defaults.addCC)
    {
        NSUInteger idx = 0;
        for (NSDictionary *track in self.sourceTracks)
        {
            if ([track[keySubTrackType] intValue] == CC608SUB)
            {
                if (![tracksAdded containsIndex:idx])
                {
                    [self addTrack:[self trackFromSourceTrackIndex:idx]];
                }

                if (self.defaults.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                {
                    break;
                }
            }
            idx++;
        }
    }

    // Set the burn key for the appropriate track.
    if (self.defaults.burnInBehavior != HBSubtitleTrackBurnInBehaviorNone && self.tracks.count)
    {
        if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorFirst)
        {
            if (self.tracks.firstObject.sourceTrackIdx != FOREIGN_TRACK_INDEX)
            {
                self.tracks.firstObject.burnedIn = YES;
            }
            else if (self.tracks.count > 1)
            {
                self.tracks[0].burnedIn = NO;
                self.tracks[1].burnedIn = YES;
            }
        }
        else if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudio)
        {
            if (self.tracks.firstObject.sourceTrackIdx == FOREIGN_TRACK_INDEX)
            {
                self.tracks.firstObject.burnedIn = YES;
            }
        }
        else if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst)
        {
            self.tracks.firstObject.burnedIn = YES;
        }
    }

    // Burn-in the first dvd or bluray track and remove the others.
    if (self.defaults.burnInDVDSubtitles || self.defaults.burnInBluraySubtitles)
    {
        // Ugly settings for ugly players
        BOOL bitmapSubtitlesFound = NO;

        NSMutableArray *tracksToDelete = [[NSMutableArray alloc] init];
        for (HBSubtitlesTrack *track in self.tracks)
        {
            if (track.sourceTrackIdx != 1)
            {
                if (((track.type == VOBSUB && self.defaults.burnInDVDSubtitles) ||
                     (track.type == PGSSUB && self.defaults.burnInBluraySubtitles)) &&
                    !bitmapSubtitlesFound)
                {
                    track.burnedIn = YES;
                    bitmapSubtitlesFound = YES;
                }
                else if (track.type == VOBSUB || track.type == PGSSUB)
                {
                    [tracksToDelete addObject:track];
                }
            }
        }
        [self.tracks removeObjectsInArray:tracksToDelete];
    }

    // Add an empty track
    [self addNoneTrack];
    [self validatePassthru];
}

#pragma mark - Validation

/**
 *  Checks whether any subtitles in the list cannot be passed through.
 *  Set the first of any such subtitles to burned-in, remove the others.
 */
- (void)validatePassthru
{
    BOOL convertToBurnInUsed = NO;
    NSMutableIndexSet *tracksToDelete = [[NSMutableIndexSet alloc] init];

    // convert any non-None incompatible tracks to burn-in or remove them
    NSUInteger idx = 0;
    for (HBSubtitlesTrack *track in self.tracks)
    {
        if (track.isEnabled && track.sourceTrackIdx > FOREIGN_TRACK_INDEX && !track.canPassthru)
        {
            if (convertToBurnInUsed == NO)
            {
                //we haven't set any track to burned-in yet, so we can
                track.burnedIn = YES;
                convertToBurnInUsed = YES; //remove any additional tracks
            }
            else
            {
                //we already have a burned-in track, we must remove others
                [tracksToDelete addIndex:idx];
            }
        }
        idx++;
    }

    //if we converted a track to burned-in, unset it for tracks that support passthru
    if (convertToBurnInUsed == YES)
    {
        for (HBSubtitlesTrack *track in self.tracks)
        {
            if (track.isEnabled && track.sourceTrackIdx > FOREIGN_TRACK_INDEX && track.canPassthru)
            {
                track.burnedIn = NO;
            }
        }
    }

    // Delete the tracks
    NSUInteger currentIndex = [tracksToDelete lastIndex];
    while (currentIndex != NSNotFound)
    {
        [self removeObjectFromTracksAtIndex:currentIndex];
        currentIndex = [tracksToDelete indexLessThanIndex:currentIndex];
    }
}

- (void)validateBurned:(NSUInteger)index
{
    [self.tracks enumerateObjectsUsingBlock:^(HBSubtitlesTrack *track, NSUInteger idx, BOOL *stop)
    {
        if (idx != index && track.sourceTrackIdx != FOREIGN_TRACK_INDEX)
        {
            track.burnedIn = NO;
        }
    }];
}

- (void)validateDefault:(NSUInteger)index
{
    [self.tracks enumerateObjectsUsingBlock:^(HBSubtitlesTrack *obj, NSUInteger idx, BOOL *stop)
    {
        if (idx != index && obj.sourceTrackIdx != FOREIGN_TRACK_INDEX)
        {
            obj.def = NO;
        }
    }];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBSubtitles *copy = [[[self class] alloc] init];

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
    [coder encodeInt:1 forKey:@"HBSubtitlesVersion"];

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

    for (HBSubtitlesTrack *track in _tracks)
    {
        track.dataSource = self;
        track.delegate = self;
    }

    decodeObject(_defaults, HBSubtitlesDefaults);

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

- (HBSubtitlesTrack *)objectInTracksAtIndex:(NSUInteger)index
{
    return self.tracks[index];
}

- (void)insertObject:(HBSubtitlesTrack *)track inTracksAtIndex:(NSUInteger)index;
{
    [[self.undo prepareWithInvocationTarget:self] removeObjectFromTracksAtIndex:index];
    [self.tracks insertObject:track atIndex:index];
}

- (void)removeObjectFromTracksAtIndex:(NSUInteger)index
{
    HBSubtitlesTrack *track = self.tracks[index];
    [[self.undo prepareWithInvocationTarget:self] insertObject:track inTracksAtIndex:index];
    [self.tracks removeObjectAtIndex:index];
}

@end
