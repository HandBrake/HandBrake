//
//  HBSubtitles.m
//  HandBrake
//
//  Created by Damiano Galassi on 12/01/15.
//
//

#import "HBSubtitles.h"
#import "HBSubtitlesDefaults.h"

#import "HBTitle.h"
#import "NSCodingMacro.h"
#include "lang.h"

NSString *keySubTrackSelectionIndex = @"keySubTrackSelectionIndex";
NSString *keySubTrackName = @"keySubTrackName";
NSString *keySubTrackIndex = @"keySubTrackIndex";
NSString *keySubTrackLanguage = @"keySubTrackLanguage";
NSString *keySubTrackLanguageIsoCode = @"keySubTrackLanguageIsoCode";
NSString *keySubTrackType = @"keySubTrackType";

NSString *keySubTrackForced = @"keySubTrackForced";
NSString *keySubTrackBurned = @"keySubTrackBurned";
NSString *keySubTrackDefault = @"keySubTrackDefault";

NSString *keySubTrackSrtOffset = @"keySubTrackSrtOffset";
NSString *keySubTrackSrtFilePath = @"keySubTrackSrtFilePath";
NSString *keySubTrackSrtCharCode = @"keySubTrackSrtCharCode";
NSString *keySubTrackSrtCharCodeIndex = @"keySubTrackSrtCharCodeIndex";
NSString *keySubTrackLanguageIndex = @"keySubTrackLanguageIndex";

#define CHAR_CODE_DEFAULT_INDEX 11

@implementation HBSubtitles

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [super init];
    if (self)
    {
        _container = HB_MUX_MP4;

        _tracks = [[NSMutableArray alloc] init];
        _defaults = [[HBSubtitlesDefaults alloc] init];

        _masterTrackArray = [title.subtitlesTracks mutableCopy];

        NSMutableArray *forcedSourceNamesArray = [NSMutableArray array];
        for (NSDictionary *dict in _masterTrackArray)
        {
            enum subsource source = [dict[keySubTrackType] intValue];
            NSString *subSourceName = @(hb_subsource_name(source));
            // if the subtitle track can be forced, add its source name to the array
            if (hb_subtitle_can_force(source) && [forcedSourceNamesArray containsObject:subSourceName] == NO)
            {
                [forcedSourceNamesArray addObject:subSourceName];
            }
        }

        // now set the name of the Foreign Audio Search track
        if (forcedSourceNamesArray.count)
        {
            [forcedSourceNamesArray sortUsingComparator:^(id obj1, id obj2)
             {
                 return [((NSString *)obj1) compare:((NSString *)obj2)];
             }];

            NSString *tempList = @"";
            for (NSString *tempString in forcedSourceNamesArray)
            {
                if (tempList.length)
                {
                    tempList = [tempList stringByAppendingString:@", "];
                }
                tempList = [tempList stringByAppendingString:tempString];
            }
            self.foreignAudioSearchTrackName = [NSString stringWithFormat:@"Foreign Audio Search (Bitmap) (%@)", tempList];
        }
        else
        {
            self.foreignAudioSearchTrackName = @"Foreign Audio Search (Bitmap)";
        }
    }
    return self;
}

- (void)addAllTracks
{
    [self.tracks removeAllObjects];

    // Add the foreign audio search pass
    [self addTrack:[self trackFromSourceTrackIndex:-1]];

    // Add the remainings tracks
    for (NSDictionary *track in self.masterTrackArray)
    {
        NSInteger sourceIndex = [track[keySubTrackIndex] integerValue];
        [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
    }

    [self.tracks addObject:[self createSubtitleTrack]];
    [self validatePassthru];
}

- (void)removeAll
{
    [self.tracks removeAllObjects];
    [self.tracks addObject:[self createSubtitleTrack]];
}

- (void)reloadDefaults
{
    [self addTracksFromDefaults];
}

// This gets called whenever the video container changes.
- (void)containerChanged:(int)container
{
    self.container = container;

    [self validatePassthru];
}

/**
 *  Convenience method to add a track to subtitlesArray.
 *  It calculates the keySubTrackSelectionIndex.
 *
 *  @param track the track to add.
 */
- (void)addTrack:(NSMutableDictionary *)newTrack
{
    newTrack[keySubTrackSelectionIndex] = @([newTrack[keySubTrackIndex] integerValue] + 1 + (self.tracks.count == 0));
    [self insertObject:newTrack inTracksAtIndex:[self countOfTracks]];
}

/**
 *  Creates a new subtitle track.
 */
- (NSMutableDictionary *)createSubtitleTrack
{
    NSMutableDictionary *newSubtitleTrack = [[NSMutableDictionary alloc] init];
    newSubtitleTrack[keySubTrackIndex] = @(-2);
    newSubtitleTrack[keySubTrackSelectionIndex] = @0;
    newSubtitleTrack[keySubTrackName] = @"None";
    newSubtitleTrack[keySubTrackForced] = @0;
    newSubtitleTrack[keySubTrackBurned] = @0;
    newSubtitleTrack[keySubTrackDefault] = @0;

    return newSubtitleTrack;
}

/**
 *  Creates a new track dictionary from a source track.
 *
 *  @param index the index of the source track in the subtitlesSourceArray,
 *               -1 means a Foreign Audio Search pass.
 *
 *  @return a new mutable track dictionary.
 */
- (NSMutableDictionary *)trackFromSourceTrackIndex:(NSInteger)index
{
    NSMutableDictionary *track = [self createSubtitleTrack];

    if (index == -1)
    {
        /*
         * we are foreign lang search, which is inherently bitmap
         *
         * since it can be either VOBSUB or PGS and the latter can't be
         * passed through to MP4, we need to know whether there are any
         * PGS tracks in the source - otherwise we can just set the
         * source track type to VOBSUB
         */
        int subtitleTrackType = VOBSUB;
        if ([self.foreignAudioSearchTrackName rangeOfString:@(hb_subsource_name(PGSSUB))].location != NSNotFound)
        {
            subtitleTrackType = PGSSUB;
        }
        // Use -1 to indicate the foreign lang search
        track[keySubTrackIndex] = @(-1);
        track[keySubTrackName] = self.foreignAudioSearchTrackName;
        track[keySubTrackType] = @(subtitleTrackType);
        // foreign lang search is most useful when combined w/Forced Only - make it default
        track[keySubTrackForced] = @1;
    }
    else
    {
        NSDictionary *sourceTrack = self.masterTrackArray[index];

        track[keySubTrackIndex] = @(index);
        track[keySubTrackName] = sourceTrack[keySubTrackName];

        /* check to see if we are an srt, in which case set our file path and source track type kvp's*/
        if ([self.masterTrackArray[index][keySubTrackType] intValue] == SRTSUB)
        {
            track[keySubTrackType] = @(SRTSUB);
            track[keySubTrackSrtFilePath] = sourceTrack[keySubTrackSrtFilePath];

            track[keySubTrackLanguageIndex] = @(self.languagesArrayDefIndex);
            track[keySubTrackLanguageIsoCode] = self.languagesArray[self.languagesArrayDefIndex][1];

            track[keySubTrackSrtCharCodeIndex] = @(CHAR_CODE_DEFAULT_INDEX);
            track[keySubTrackSrtCharCode] = self.charCodeArray[CHAR_CODE_DEFAULT_INDEX];
        }
        else
        {
            track[keySubTrackType] = sourceTrack[keySubTrackType];
        }
    }

    if (!hb_subtitle_can_burn([track[keySubTrackType] intValue]))
    {
        /* the source track cannot be burned in, so uncheck the widget */
        track[keySubTrackBurned] = @0;
    }

    if (!hb_subtitle_can_force([track[keySubTrackType] intValue]))
    {
        /* the source track does not support forced flags, so uncheck the widget */
        track[keySubTrackForced] = @0;
    }
    
    return track;
}

/**
 *  Remove all the subtitles tracks and
 *  add new ones based on the defaults settings
 */
- (IBAction)addTracksFromDefaults
{
    // Keeps a set of the indexes of the added track
    // so we don't add the same track twice.
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];

    [self.tracks removeAllObjects];

    // Add the foreign audio search pass
    if (self.defaults.addForeignAudioSearch)
    {
        [self addTrack:[self trackFromSourceTrackIndex:-1]];
    }

    // Add the tracks for the selected languages
    if (self.defaults.trackSelectionBehavior != HBSubtitleTrackSelectionBehaviorNone)
    {
        for (NSString *lang in self.defaults.trackSelectionLanguages)
        {
            for (NSDictionary *track in self.masterTrackArray)
            {
                if ([lang isEqualToString:@"und"] || [track[keySubTrackLanguageIsoCode] isEqualToString:lang])
                {
                    NSInteger sourceIndex = [track[keySubTrackIndex] intValue];

                    if (![tracksAdded containsIndex:sourceIndex])
                    {
                        [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
                    }
                    [tracksAdded addIndex:sourceIndex];

                    if (self.defaults.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                    {
                        break;
                    }
                }
            }
        }
    }

    // Add the closed captions track if there is one.
    if (self.defaults.addCC)
    {
        for (NSDictionary *track in self.masterTrackArray)
        {
            if ([track[keySubTrackType] intValue] == CC608SUB)
            {
                NSInteger sourceIndex = [track[keySubTrackIndex] intValue];
                if (![tracksAdded containsIndex:sourceIndex])
                {
                    [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
                }

                if (self.defaults.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                {
                    break;
                }
            }
        }
    }

    // Set the burn key for the appropriate track.
    if (self.defaults.burnInBehavior != HBSubtitleTrackBurnInBehaviorNone && self.tracks.count)
    {
        if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorFirst)
        {
            if ([self.tracks.firstObject[keySubTrackIndex] integerValue] != -1)
            {
                self.tracks.firstObject[keySubTrackBurned] = @YES;
            }
            else if (self.tracks.count > 1)
            {
                self.tracks[1][keySubTrackBurned] = @YES;
            }
        }
        else if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudio)
        {
            if ([self.tracks.firstObject[keySubTrackIndex] integerValue] == -1)
            {
                self.tracks.firstObject[keySubTrackBurned] = @YES;
            }
        }
        else if (self.defaults.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst)
        {
            self.tracks.firstObject[keySubTrackBurned] = @YES;
        }
    }

    // Burn-in the first dvd or bluray track and remove the others.
    if (self.defaults.burnInDVDSubtitles || self.defaults.burnInBluraySubtitles)
    {
        // Ugly settings for ugly players
        BOOL bitmapSubtitlesFound = NO;

        NSMutableArray *tracksToDelete = [[NSMutableArray alloc] init];
        for (NSMutableDictionary *track in self.tracks)
        {
            if ([track[keySubTrackIndex] integerValue] != -1)
            {
                if ((([track[keySubTrackType] intValue] == VOBSUB && self.defaults.burnInDVDSubtitles) ||
                     ([track[keySubTrackType] intValue] == PGSSUB && self.defaults.burnInBluraySubtitles)) &&
                    !bitmapSubtitlesFound)
                {
                    track[keySubTrackBurned] = @YES;
                    bitmapSubtitlesFound = YES;
                }
                else if ([track[keySubTrackType] intValue] == VOBSUB || [track[keySubTrackType] intValue] == PGSSUB)
                {
                    [tracksToDelete addObject:track];
                }
            }
        }
        [self.tracks removeObjectsInArray:tracksToDelete];
    }

    // Add an empty track
    [self insertObject:[self createSubtitleTrack] inTracksAtIndex:[self countOfTracks]];
    [self validatePassthru];
}

/**
 *  Checks whether any subtitles in the list cannot be passed through.
 *  Set the first of any such subtitles to burned-in, remove the others.
 */
- (void)validatePassthru
{
    BOOL convertToBurnInUsed = NO;
    NSMutableArray *tracksToDelete = [[NSMutableArray alloc] init];

    // convert any non-None incompatible tracks to burn-in or remove them
    for (NSMutableDictionary *track in self.tracks)
    {
        if (track[keySubTrackType] == nil)
        {
            continue;
        }

        int subtitleTrackType = [track[keySubTrackType] intValue];
        if (!hb_subtitle_can_pass(subtitleTrackType, self.container))
        {
            if (convertToBurnInUsed == NO)
            {
                //we haven't set any track to burned-in yet, so we can
                track[keySubTrackBurned] = @1;
                convertToBurnInUsed = YES; //remove any additional tracks
            }
            else
            {
                //we already have a burned-in track, we must remove others
                [tracksToDelete addObject:track];
            }
        }
    }
    //if we converted a track to burned-in, unset it for tracks that support passthru
    if (convertToBurnInUsed == YES)
    {
        for (NSMutableDictionary *track in self.tracks)
        {
            if (track[keySubTrackType] == nil)
            {
                continue;
            }

            int subtitleTrackType = [track[keySubTrackType] intValue];
            if (hb_subtitle_can_pass(subtitleTrackType, self.container))
            {
                track[keySubTrackBurned] = @0;
            }
        }
    }

    [self willChangeValueForKey:@"tracks"];
    if (tracksToDelete.count)
    {
        [self.tracks removeObjectsInArray:tracksToDelete];
    }
    [self didChangeValueForKey:@"tracks"];

}

#pragma mark - Languages

@synthesize languagesArray = _languagesArray;

- (NSArray *)languagesArray
{
    if (!_languagesArray)
    {
        _languagesArray = [self populateLanguageArray];
    }

    return _languagesArray;
}

- (NSArray *)populateLanguageArray
{
    NSMutableArray *languages = [[NSMutableArray alloc] init];

    for (const iso639_lang_t * lang = lang_get_next(NULL); lang != NULL; lang = lang_get_next(lang))
    {
        [languages addObject:@[@(lang->eng_name),
                               @(lang->iso639_2)]];
        if (!strcasecmp(lang->eng_name, "English"))
        {
            _languagesArrayDefIndex = [languages count] - 1;
        }
    }
    return [languages copy];
}

@synthesize charCodeArray = _charCodeArray;

- (NSArray *)charCodeArray
{
    if (!_charCodeArray)
    {
        // populate the charCodeArray.
        _charCodeArray = @[@"ANSI_X3.4-1968", @"ANSI_X3.4-1986", @"ANSI_X3.4", @"ANSI_X3.110-1983", @"ANSI_X3.110", @"ASCII",
                            @"ECMA-114", @"ECMA-118", @"ECMA-128", @"ECMA-CYRILLIC", @"IEC_P27-1", @"ISO-8859-1", @"ISO-8859-2",
                            @"ISO-8859-3", @"ISO-8859-4", @"ISO-8859-5", @"ISO-8859-6", @"ISO-8859-7", @"ISO-8859-8", @"ISO-8859-9",
                            @"ISO-8859-9E", @"ISO-8859-10", @"ISO-8859-11", @"ISO-8859-13", @"ISO-8859-14", @"ISO-8859-15", @"ISO-8859-16",
                            @"UTF-7", @"UTF-8", @"UTF-16", @"UTF-16LE", @"UTF-16BE", @"UTF-32", @"UTF-32LE", @"UTF-32BE"];
    }
    return _charCodeArray;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBSubtitles *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_container = _container;

        copy->_masterTrackArray = [_masterTrackArray mutableCopy];
        copy->_foreignAudioSearchTrackName = [_foreignAudioSearchTrackName copy];

        copy->_tracks = [[NSMutableArray alloc] init];
        [_tracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            if (idx < _tracks.count)
            {
                NSMutableDictionary *trackCopy = [obj copy];
                [copy->_tracks addObject:trackCopy];
            }
        }];

        copy->_defaults = [_defaults copy];
    }

    return copy;
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBAudioVersion"];

    encodeInt(_container);

    encodeObject(_masterTrackArray);
    encodeObject(_foreignAudioSearchTrackName);
    encodeObject(_tracks);

    encodeObject(_defaults);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_container);

    decodeObject(_masterTrackArray);
    decodeObject(_foreignAudioSearchTrackName);
    decodeObject(_tracks);

    decodeObject(_defaults);

    return self;
}

#pragma mark - Presets

- (void)writeToPreset:(NSMutableDictionary *)preset
{
    [self.defaults writeToPreset:preset];
}

- (void)applyPreset:(NSDictionary *)preset
{
    [self.defaults applyPreset:preset];
    [self addTracksFromDefaults];
}

#pragma mark -
#pragma mark KVC

- (NSUInteger) countOfTracks
{
    return self.tracks.count;
}

- (id)objectInTracksAtIndex:(NSUInteger)index
{
    return self.tracks[index];
}

- (void)insertObject:(id)track inTracksAtIndex:(NSUInteger)index;
{
    [self.tracks insertObject:track atIndex:index];
}

- (void)removeObjectFromTracksAtIndex:(NSUInteger)index
{
    [self.tracks removeObjectAtIndex:index];
}

@end
