/*  HBSubtitles.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitles.h"
#import "HBSubtitlesDefaults.h"

#import "HBSubtitlesTrack.h"

#import "HBJob.h"
#import "HBJob+HBJobConversion.h"
#import "HBTitle.h"
#import "HBCodingUtilities.h"
#import "HBLocalizationUtilities.h"
#import "HBUtilities.h"
#import "HBJob+Private.h"
#import "HBSecurityAccessToken.h"

#include "common.h"

extern NSString *keySubTrackName;
extern NSString *keySubTrackLanguageIsoCode;
extern NSString *keySubTrackType;

extern NSString *keySubTrackExternalFileURL;
extern NSString *keySubTrackExternalFileURLBookmark;

#define NONE_TRACK_INDEX        0
#define FOREIGN_TRACK_INDEX     1

@interface HBSubtitles () <HBTrackDataSource, HBTrackDelegate>

@property (nonatomic, readwrite) NSArray<NSDictionary *> *sourceTracks;

@property (nonatomic, readonly) NSMutableArray<HBSecurityAccessToken *> *tokens;
@property (nonatomic, readwrite) NSInteger *accessCount;

@property (nonatomic, readwrite, weak) HBJob *job;
@property (nonatomic, readwrite) int container;

/// Used to avoid circular dependency validation.
@property (nonatomic, readwrite) BOOL validating;

@end

@implementation HBSubtitles

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _job = job;
        _container = HB_MUX_MP4;

        _tracks = [[NSMutableArray alloc] init];
        _defaults = [[HBSubtitlesDefaults alloc] init];
        _tokens = [NSMutableArray array];

        NSMutableArray *sourceTracks = [job.title.subtitlesTracks mutableCopy];

        int foreignAudioType = VOBSUB;

        // now set the name of the Foreign Audio Search track
        NSMutableString *foreignAudioSearchTrackName = [HBKitLocalizedString(@"Foreign Audio Search", "HBSubtitles -> search pass name") mutableCopy];

        // Add the none and foreign track to the source array
        NSDictionary *none = @{ keySubTrackName: HBKitLocalizedString(@"None", @"HBSubtitles -> none track name")};
        [sourceTracks insertObject:none atIndex:0];

        NSDictionary *foreign = @{ keySubTrackName: [foreignAudioSearchTrackName copy],
                                   keySubTrackType: @(foreignAudioType) };
        [sourceTracks insertObject:foreign atIndex:1];

        _sourceTracks = [sourceTracks copy];

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
    }

    // Else add a new None track
    if (oldSourceIdx == NONE_TRACK_INDEX)
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
    [self removeTracksAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.tracks.count)]];

    // Add the remainings tracks
    for (NSUInteger idx = 1; idx < self.sourceTracksArray.count; idx++) {
        [self addTrack:[self trackFromSourceTrackIndex:idx]];
    }

    [self addNoneTrack];
    [self validatePassthru];
}

- (void)removeAll
{
    [self removeTracksAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.tracks.count)]];
    [self addNoneTrack];
}

- (void)reloadDefaults
{
    [self addDefaultTracksFromJobSettings:self.job.jobDict];
}

- (void)addExternalTrackFromURL:(NSURL *)fileURL
{
    int type = [fileURL.pathExtension.lowercaseString isEqualToString:@"srt"] ? IMPORTSRT : IMPORTSSA;

#ifdef __SANDBOX_ENABLED__
    // Create the security scoped bookmark
    NSData *bookmark = [HBUtilities bookmarkFromURL:fileURL
                                    options:NSURLBookmarkCreationWithSecurityScope |
                                            NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess];
#endif

    // Create a new entry for the subtitle source array so it shows up in our subtitle source list
    NSMutableArray *sourceTrack = [self.sourceTracks mutableCopy];
#ifdef __SANDBOX_ENABLED__
    [sourceTrack addObject:@{keySubTrackName: fileURL.lastPathComponent,
                             keySubTrackType: @(type),
                             keySubTrackExternalFileURL: fileURL,
                             keySubTrackExternalFileURLBookmark: bookmark}];
#else
    [sourceTrack addObject:@{keySubTrackName: fileURL.lastPathComponent,
                             keySubTrackType: @(type),
                             keySubTrackExternalFileURL: fileURL}];
#endif
    self.sourceTracks = [sourceTrack copy];
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

- (void)addDefaultTracksFromJobSettings:(NSDictionary *)settings
{
    NSMutableArray<HBSubtitlesTrack *> *tracks = [NSMutableArray array];
    NSArray<NSDictionary<NSString *, id> *> *settingsTracks = settings[@"Subtitle"][@"SubtitleList"];
    NSDictionary<NSString *, id> *search = settings[@"Subtitle"][@"Search"];

    // Reinitialize the configured list of audio tracks
    [self removeTracksAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.tracks.count)]];

    // Add the foreign audio search pass
    if ([search[@"Enable"] boolValue])
    {
        HBSubtitlesTrack *track = [self trackFromSourceTrackIndex:FOREIGN_TRACK_INDEX];

        track.burnedIn = [search[@"Burn"] boolValue];
        track.forcedOnly = [search[@"Forced"] boolValue];
        track.def = [search[@"Default"] boolValue];

        [tracks addObject:track];
    }

    // Add the tracks
    for (NSDictionary *trackDict in settingsTracks)
    {
        HBSubtitlesTrack *track = [self trackFromSourceTrackIndex:[trackDict[@"Track"] unsignedIntegerValue] + 2];

        track.burnedIn = [trackDict[@"Burn"] boolValue];
        track.forcedOnly = [trackDict[@"Forced"] boolValue];

        [tracks addObject:track];
    }

    [self insertTracks:tracks atIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, tracks.count)]];

    // Add an None item
    [self addNoneTrack];
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

- (BOOL)startAccessingSecurityScopedResource
{
#ifdef __SANDBOX_ENABLED__
    if (self.accessCount == 0)
    {
        for (NSDictionary *sourceTrack in self.sourceTracks)
        {
            if (sourceTrack[keySubTrackExternalFileURLBookmark])
            {
                [self.tokens addObject:[HBSecurityAccessToken tokenWithObject:sourceTrack[keySubTrackExternalFileURL]]];
            }
        }
    }
    self.accessCount += 1;
    return YES;
#else
    return NO;
#endif
}

- (void)stopAccessingSecurityScopedResource
{
#ifdef __SANDBOX_ENABLED__
    self.accessCount -= 1;
    NSAssert(self.accessCount >= 0, @"[HBSubtitles stopAccessingSecurityScopedResource:] unbalanced call");
    if (self.accessCount == 0)
    {
        [self.tokens removeAllObjects];
    }
#endif
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBSubtitles *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_container = _container;
        copy->_sourceTracks = [_sourceTracks copy];

        copy->_tracks = [[NSMutableArray alloc] init];

        for (HBSubtitlesTrack *track in _tracks)
        {
            HBSubtitlesTrack *trackCopy = [track copy];
            [copy->_tracks addObject:trackCopy];

            trackCopy.dataSource = copy;
            trackCopy.delegate = copy;
        }

        copy->_defaults = [_defaults copy];
        copy->_tokens = [NSMutableArray array];
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

#ifdef __SANDBOX_ENABLED__
    NSMutableArray *sourceTracks = [NSMutableArray array];
#endif

    _tokens = [NSMutableArray array];

    decodeInt(_container); if (_container != HB_MUX_MP4 && _container != HB_MUX_MKV && _container != HB_MUX_WEBM) { goto fail; }
    decodeCollectionOfObjects3(_sourceTracks, NSArray, NSDictionary, NSURL, NSData);

#ifdef __SANDBOX_ENABLED__
    for (NSDictionary *sourceTrack in _sourceTracks)
    {
        if (sourceTrack[keySubTrackExternalFileURLBookmark])
        {
            NSMutableDictionary<NSString *, id> *copy =  [sourceTrack mutableCopy];
            NSURL *srtURL = [HBUtilities URLFromBookmark:sourceTrack[keySubTrackExternalFileURLBookmark]];
            if (srtURL)
            {
                copy[keySubTrackExternalFileURL] = srtURL;
            }
            [sourceTracks addObject:copy];
        }
        else
        {
            [sourceTracks addObject:sourceTrack];
        }
    }
    _sourceTracks = [sourceTracks copy];
#endif

    decodeCollectionOfObjects(_tracks, NSMutableArray, HBSubtitlesTrack);

    for (HBSubtitlesTrack *track in _tracks)
    {
        track.dataSource = self;
        track.delegate = self;
    }

    decodeObjectOrFail(_defaults, HBSubtitlesDefaults);

    return self;

fail:
    return nil;
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

- (HBSubtitlesTrack *)objectInTracksAtIndex:(NSUInteger)index
{
    return self.tracks[index];
}

- (void)insertObject:(HBSubtitlesTrack *)track inTracksAtIndex:(NSUInteger)index;
{
    [[self.undo prepareWithInvocationTarget:self] removeObjectFromTracksAtIndex:index];
    [self.tracks insertObject:track atIndex:index];
}

- (void)insertTracks:(NSArray<HBSubtitlesTrack *> *)array atIndexes:(NSIndexSet *)indexes
{
    [[self.undo prepareWithInvocationTarget:self] removeTracksAtIndexes:indexes];
    [self.tracks insertObjects:array atIndexes:indexes];
}

- (void)removeObjectFromTracksAtIndex:(NSUInteger)index
{
    HBSubtitlesTrack *track = self.tracks[index];
    [[self.undo prepareWithInvocationTarget:self] insertObject:track inTracksAtIndex:index];
    [self.tracks removeObjectAtIndex:index];
}

- (void)removeTracksAtIndexes:(NSIndexSet *)indexes
{
    NSArray<HBSubtitlesTrack *> *tracks = [self.tracks objectsAtIndexes:indexes];
    [[self.undo prepareWithInvocationTarget:self] insertTracks:tracks atIndexes:indexes];
    [self.tracks removeObjectsAtIndexes:indexes];
}


@end
