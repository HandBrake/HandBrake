/* $Id: HBSubtitles.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesController.h"
#import "HBSubtitlesDefaultsController.h"
#import "HBSubtitlesDefaults.h"

#import "Controller.h"
#include "hb.h"
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

@interface HBSubtitlesController () <NSTableViewDataSource, NSTableViewDelegate>

// IBOutles
@property (assign) IBOutlet NSTableView *fTableView;

@property (assign) IBOutlet NSPopUpButton *trackPopUp;
@property (assign) IBOutlet NSButton *configureDefaults;
@property (assign) IBOutlet NSButton *reloadDefaults;

@property (nonatomic, readwrite) BOOL enabled;

// Subtitles arrays
@property (nonatomic, readonly) NSMutableArray *subtitleArray;
@property (nonatomic, readonly) NSMutableArray *subtitleSourceArray;

@property (nonatomic, readwrite, retain) NSString *foreignAudioSearchTrackName;
@property (nonatomic, readwrite) int container;

// Defaults
@property (nonatomic, readwrite, retain) HBSubtitlesDefaultsController *defaultsController;
@property (nonatomic, readwrite, retain) HBSubtitlesDefaults *settings;

// Table view cells models
@property (nonatomic, readonly) NSArray *charCodeArray;
@property (nonatomic, readwrite) BOOL foreignAudioSearchSelected;

@property (nonatomic, readonly) NSArray *languagesArray;
@property (nonatomic, readonly) NSInteger languagesArrayDefIndex;

// Cached table view's cells
@property (nonatomic, readonly) NSPopUpButtonCell *languagesCell;
@property (nonatomic, readonly) NSPopUpButtonCell *encodingsCell;

@end

@implementation HBSubtitlesController

- (instancetype)init
{
    self = [super initWithNibName:@"Subtitles" bundle:nil];
    if (self)
    {
        _subtitleSourceArray = [[NSMutableArray alloc] init];
        _subtitleArray = [[NSMutableArray alloc] init];
        _languagesArray = [[self populateLanguageArray] retain];

        // populate the charCodeArray.
        _charCodeArray = [@[@"ANSI_X3.4-1968", @"ANSI_X3.4-1986", @"ANSI_X3.4", @"ANSI_X3.110-1983", @"ANSI_X3.110", @"ASCII",
                          @"ECMA-114", @"ECMA-118", @"ECMA-128", @"ECMA-CYRILLIC", @"IEC_P27-1", @"ISO-8859-1", @"ISO-8859-2",
                          @"ISO-8859-3", @"ISO-8859-4", @"ISO-8859-5", @"ISO-8859-6", @"ISO-8859-7", @"ISO-8859-8", @"ISO-8859-9",
                          @"ISO-8859-9E", @"ISO-8859-10", @"ISO-8859-11", @"ISO-8859-13", @"ISO-8859-14", @"ISO-8859-15", @"ISO-8859-16",
                          @"UTF-7", @"UTF-8", @"UTF-16", @"UTF-16LE", @"UTF-16BE", @"UTF-32", @"UTF-32LE", @"UTF-32BE"] retain];

        // Register as observer for the HBController notifications.
        [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(containerChanged:) name: HBContainerChangedNotification object: nil];
        [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(titleChanged:) name: HBTitleChangedNotification object: nil];
    }

    return self;
}

- (void)setUIEnabled:(BOOL)flag
{
    [self.trackPopUp setEnabled:flag];
    [self.configureDefaults setEnabled:flag];
    [self.reloadDefaults setEnabled:flag];
    [self.fTableView setEnabled:flag];
    self.enabled = flag;
}

- (void)titleChanged:(NSNotification *)aNotification
{
    NSDictionary *notDict = [aNotification userInfo];
    NSData *theData = notDict[keyTitleTag];
    hb_title_t *title = NULL;
    [theData getBytes: &title length: sizeof(title)];

    /* reset the subtitles arrays */
    [self.subtitleArray removeAllObjects];
    [self.subtitleSourceArray removeAllObjects];

    if (title)
    {
        /* now populate the array with the source subtitle track info */
        NSMutableArray *forcedSourceNamesArray = [[NSMutableArray alloc] init];
        for (int i = 0; i < hb_list_count(title->list_subtitle); i++)
        {
            hb_subtitle_t *subtitle = (hb_subtitle_t *)hb_list_item(title->list_subtitle, i);

            /* Human-readable representation of subtitle->source */
            NSString *bitmapOrText  = subtitle->format == PICTURESUB ? @"Bitmap" : @"Text";
            NSString *subSourceName = @(hb_subsource_name(subtitle->source));

            /* if the subtitle track can be forced, add its source name to the array */
            if (hb_subtitle_can_force(subtitle->source) && [forcedSourceNamesArray containsObject:subSourceName] == NO)
            {
                [forcedSourceNamesArray addObject:subSourceName];
            }

            // Use the native language name if available
            iso639_lang_t *language = lang_for_code2(subtitle->iso639_2);
            NSString *nativeLanguage = strlen(language->native_name) ? @(language->native_name) : @(language->eng_name);

            /* create a dictionary of source subtitle information to store in our array */
            [self.subtitleSourceArray addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", i, nativeLanguage, bitmapOrText, subSourceName],
                                                  keySubTrackIndex: @(i),
                                                  keySubTrackType: @(subtitle->source),
                                                  keySubTrackLanguage: nativeLanguage,
                                                  keySubTrackLanguageIsoCode: @(subtitle->iso639_2)}];
        }

        /* now set the name of the Foreign Audio Search track */
        if ([forcedSourceNamesArray count])
        {
            [forcedSourceNamesArray sortUsingComparator:^(id obj1, id obj2)
            {
                return [((NSString *)obj1) compare:((NSString *)obj2)];
            }];

            NSString *tempList = @"";
            for (NSString *tempString in forcedSourceNamesArray)
            {
                if ([tempList length])
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
        [forcedSourceNamesArray release];

        // Append an empty track at the end
        // to display a "None" row in the table view
        [self.subtitleArray addObject:[self createSubtitleTrack]];
    }

    [self.fTableView reloadData];    
}

- (void)containerChanged:(NSNotification *)aNotification
{
    NSDictionary *notDict = [aNotification userInfo];
    self.container = [notDict[keyContainerTag] intValue];

    [self validatePassthru];

    [self.fTableView reloadData];
}

- (NSArray *)subtitles
{
    NSMutableArray *ret = [self.subtitleArray mutableCopy];
    [ret removeLastObject];
    return [ret autorelease];
}

- (void)addTracksFromQueue:(NSMutableArray *)newSubtitleArray
{
    /* Note: we need to look for external subtitles so it can be added to the source array track.
     * Remember the source container subs are already loaded with resetTitle which is already called
     * so any external sub sources need to be added to our source subs here
     */
    for (id tempObject in newSubtitleArray)
    {
        /* We have an srt track */
        if ([tempObject[keySubTrackType] intValue] == SRTSUB)
        {
            NSString *filePath = tempObject[keySubTrackSrtFilePath];
            /* create a dictionary of source subtitle information to store in our array */
            [self.subtitleSourceArray addObject:@{keySubTrackIndex: @(self.subtitleSourceArray.count + 1),
                                                  keySubTrackName: [filePath lastPathComponent],
                                                  keySubTrackType: @(SRTSUB),
                                                  keySubTrackSrtFilePath: filePath}];
        }
    }

    [newSubtitleArray addObject:[self createSubtitleTrack]];

    // Set the subtitleArray to the newSubtitleArray
    [self.subtitleArray setArray:newSubtitleArray];
    [self.fTableView reloadData];
}

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    self.settings = [[[HBSubtitlesDefaults alloc] init] autorelease];
    [self.settings applySettingsFromPreset:preset];

    [self addTracksFromDefaults:self];
}

#pragma mark - Actions

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    return self.enabled;
}

/**
 *  Add every subtitles track that still isn't in the subtitles array.
 */
- (IBAction)addAll:(id)sender
{
    [self.subtitleArray removeAllObjects];

    // Add the foreign audio search pass
    [self addTrack:[self trackFromSourceTrackIndex:-1]];

    // Add the remainings tracks
    for (NSDictionary *track in self.subtitleSourceArray)
    {
        NSInteger sourceIndex = [track[keySubTrackIndex] integerValue];
        [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
    }

    [self.subtitleArray addObject:[self createSubtitleTrack]];
    [self validatePassthru];
    [self.fTableView reloadData];
}

/**
 *  Remove all the subtitles tracks.
 */
- (IBAction)removeAll:(id)sender
{
    [self.subtitleArray removeAllObjects];
    [self.subtitleArray addObject:[self createSubtitleTrack]];
    [self.fTableView reloadData];
}

/**
 *  Remove all the subtitles tracks and
 *  add new ones based on the defaults settings
 */
- (IBAction)addTracksFromDefaults:(id)sender
{
    // Keeps a set of the indexes of the added track
    // so we don't add the same track twice.
    NSMutableIndexSet *tracksAdded = [NSMutableIndexSet indexSet];

    [self.subtitleArray removeAllObjects];

    // Add the foreign audio search pass
    if (self.settings.addForeignAudioSearch)
    {
        [self addTrack:[self trackFromSourceTrackIndex:-1]];
    }

    // Add the tracks for the selected languages
    if (self.settings.trackSelectionBehavior != HBSubtitleTrackSelectionBehaviorNone)
    {
        for (NSString *lang in self.settings.trackSelectionLanguages)
        {
            for (NSDictionary *track in self.subtitleSourceArray)
            {
                if ([lang isEqualToString:@"und"] || [track[keySubTrackLanguageIsoCode] isEqualToString:lang])
                {
                    NSInteger sourceIndex = [track[keySubTrackIndex] intValue];

                    if (![tracksAdded containsIndex:sourceIndex])
                    {
                        [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
                    }
                    [tracksAdded addIndex:sourceIndex];

                    if (self.settings.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                    {
                        break;
                    }
                }
            }
        }
    }

    // Add the closed captions track if there is one.
    if (self.settings.addCC)
    {
        for (NSDictionary *track in self.subtitleSourceArray)
        {
            if ([track[keySubTrackType] intValue] == CC608SUB)
            {
                NSInteger sourceIndex = [track[keySubTrackIndex] intValue];
                if (![tracksAdded containsIndex:sourceIndex])
                {
                    [self addTrack:[self trackFromSourceTrackIndex:sourceIndex]];
                }

                if (self.settings.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
                {
                    break;
                }
            }
        }
    }

    // Add an empty track
    [self.subtitleArray addObject:[self createSubtitleTrack]];

    [self validatePassthru];
    [self.fTableView reloadData];
}

- (IBAction)showSettingsSheet:(id)sender
{
    self.defaultsController = [[[HBSubtitlesDefaultsController alloc] initWithSettings:self.settings] autorelease];
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

#pragma mark - Subtitles tracks creation and validation

/**
 *  Convenience method to add a track to subtitlesArray.
 *  It calculates the keySubTrackSelectionIndex.
 *
 *  @param track the track to add.
 */
- (void)addTrack:(NSMutableDictionary *)newTrack
{
    newTrack[keySubTrackSelectionIndex] = @([newTrack[keySubTrackIndex] integerValue] + 1 + (self.subtitleArray.count == 0));
    [self.subtitleArray addObject:newTrack];
}

/**
 *  Creates a new subtitle track.
 */
- (NSMutableDictionary *)createSubtitleTrack
{
    NSMutableDictionary *newSubtitleTrack = [[NSMutableDictionary alloc] init];
    newSubtitleTrack[keySubTrackIndex] = @0;
    newSubtitleTrack[keySubTrackSelectionIndex] = @0;
    newSubtitleTrack[keySubTrackName] = @"None";
    newSubtitleTrack[keySubTrackForced] = @0;
    newSubtitleTrack[keySubTrackBurned] = @0;
    newSubtitleTrack[keySubTrackDefault] = @0;

    return [newSubtitleTrack autorelease];
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
        NSDictionary *sourceTrack = self.subtitleSourceArray[index];

        track[keySubTrackIndex] = @(index);
        track[keySubTrackName] = sourceTrack[keySubTrackName];

        /* check to see if we are an srt, in which case set our file path and source track type kvp's*/
        if ([self.subtitleSourceArray[index][keySubTrackType] intValue] == SRTSUB)
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
 *  Checks whether any subtitles in the list cannot be passed through.
 *  Set the first of any such subtitles to burned-in, remove the others.
 */
- (void)validatePassthru
{
    int subtitleTrackType;
    BOOL convertToBurnInUsed = NO;
    NSMutableArray *tracksToDelete = [[NSMutableArray alloc] init];

    // convert any non-None incompatible tracks to burn-in or remove them
    for (id tempObject in self.subtitleArray)
    {
        if (tempObject[keySubTrackType] == nil)
        {
            continue;
        }

        subtitleTrackType = [tempObject[keySubTrackType] intValue];
        if (!hb_subtitle_can_pass(subtitleTrackType, self.container))
        {
            if (convertToBurnInUsed == NO)
            {
                //we haven't set any track to burned-in yet, so we can
                tempObject[keySubTrackBurned] = @1;
                convertToBurnInUsed = YES; //remove any additional tracks
            }
            else
            {
                //we already have a burned-in track, we must remove others
                [tracksToDelete addObject:tempObject];
            }
        }
    }
    //if we converted a track to burned-in, unset it for tracks that support passthru
    if (convertToBurnInUsed == YES)
    {
        for (id tempObject in self.subtitleArray)
        {
            if (tempObject[keySubTrackType] == nil)
            {
                continue;
            }

            subtitleTrackType = [tempObject[keySubTrackType] intValue];
            if (hb_subtitle_can_pass(subtitleTrackType, self.container))
            {
                tempObject[keySubTrackBurned] = @0;
            }
        }
    }

    if (tracksToDelete.count)
    {
        [self.subtitleArray removeObjectsInArray:tracksToDelete];
        [self.fTableView reloadData];
    }

    [tracksToDelete release];
}

- (void)validateBurned:(NSInteger)index
{
    [self.subtitleArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
     {
         if (idx != index)
         {
             obj[keySubTrackBurned] = @0;
         }
     }];
    [self validatePassthru];
}

- (void)validateDefault:(NSInteger)index
{
    [self.subtitleArray enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
    {
        if (idx != index)
        {
            obj[keySubTrackDefault] = @0;
        }
    }];
}

#pragma mark -
#pragma mark Subtitle Table Data Source Methods

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return self.subtitleArray.count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    NSDictionary *track = self.subtitleArray[rowIndex];

    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        NSNumber *index = track[keySubTrackSelectionIndex];
        if (index)
            return index;
        else
            return @0;
    }
    else if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
        return track[keySubTrackForced];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
        return track[keySubTrackBurned];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
        return track[keySubTrackDefault];
    }
    /* These next three columns only apply to srt's. they are disabled for source subs */
    else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
    {
        if ([track[keySubTrackType] intValue] == SRTSUB)
        {
            return track[keySubTrackLanguageIndex];
        }
        else
        {
            return @0;
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        if ([track[keySubTrackType] intValue] == SRTSUB)
        {
            return track[keySubTrackSrtCharCodeIndex];
        }
        else
        {
            return @0;
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        if (track[keySubTrackSrtOffset])
        {
            return [track[keySubTrackSrtOffset] stringValue];
        }
        else
        {
            return @"0";
        }
    }

    return nil;
}

/**
 *  Called whenever a widget in the table is edited or changed, we use it to record the change in the controlling array
 *  including removing and adding new tracks via the "None" ("track" index of 0)
 */
- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        /* Set the array to track if we are vobsub (picture sub) */
        if ([anObject intValue] > 0)
        {
            NSMutableDictionary *newTrack = [self trackFromSourceTrackIndex:[anObject integerValue] - 1 - (rowIndex == 0)];
            // Selection index calculation
            newTrack[keySubTrackSelectionIndex] = @([anObject integerValue]);
            self.subtitleArray[rowIndex] = newTrack;
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
        self.subtitleArray[rowIndex][keySubTrackForced] = @([anObject intValue]);
    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
        self.subtitleArray[rowIndex][keySubTrackBurned] = @([anObject intValue]);
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            self.subtitleArray[rowIndex][keySubTrackDefault] = @0;
        }
        /* now we need to make sure no other tracks are set to burned if we have set burned */
        if ([anObject intValue] == 1)
        {
            [self validateBurned:rowIndex];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
        self.subtitleArray[rowIndex][keySubTrackDefault] = @([anObject intValue]);
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            self.subtitleArray[rowIndex][keySubTrackBurned] = @0;
        }
        /* now we need to make sure no other tracks are set to default */
        if ([anObject intValue] == 1)
        {
            [self validateDefault:rowIndex];
        }
    }
    /* These next three columns only apply to srt's. they are disabled for source subs */
    else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
    {
        self.subtitleArray[rowIndex][keySubTrackLanguageIndex] = @([anObject intValue]);
        self.subtitleArray[rowIndex][keySubTrackLanguageIsoCode] = self.languagesArray[[anObject intValue]][1];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        /* charCodeArray */
        self.subtitleArray[rowIndex][keySubTrackSrtCharCodeIndex] = @([anObject intValue]);
        self.subtitleArray[rowIndex][keySubTrackSrtCharCode] = self.charCodeArray[[anObject intValue]];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        self.subtitleArray[rowIndex][keySubTrackSrtOffset] = @([anObject integerValue]);
    }

    /* now lets do a bit of logic to add / remove tracks as necessary via the "None" track (index 0) */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        /* Since currently no quicktime based playback devices support soft vobsubs in mp4, we make sure "burned in" is specified
         * by default to avoid massive confusion and anarchy. However we also want to guard against multiple burned in subtitle tracks
         * as libhb would ignore all but the first one anyway. Plus it would probably be stupid.
         */
        if ((self.container & HB_MUX_MASK_MP4) && ([anObject intValue] != 0))
        {
            if ([self.subtitleArray[rowIndex][keySubTrackType] intValue] == VOBSUB)
            {
                /* lets see if there are currently any burned in subs specified */
                BOOL subtrackBurnedInFound = NO;
                for (id tempObject in self.subtitleArray)
                {
                    if ([tempObject[keySubTrackBurned] intValue] == 1)
                    {
                        subtrackBurnedInFound = YES;
                    }
                }
                /* if we have no current vobsub set to burn it in ... burn it in by default */
                if (!subtrackBurnedInFound)
                {
                    self.subtitleArray[rowIndex][keySubTrackBurned] = @1;
                    /* Burned In and Default are mutually exclusive */
                    self.subtitleArray[rowIndex][keySubTrackDefault] = @0;
                }
            }
        }

        /* We use the track popup index number (presumes index 0 is "None" which is ignored and only used to remove tracks if need be)
         * to determine whether to 1 modify an existing track, 2. add a new empty "None" track or 3. remove an existing track.
         */

        if ([anObject intValue] != 0 && rowIndex == [self.subtitleArray count] - 1) // if we have a last track which != "None"
        {
            /* add a new empty None track */
            [self.subtitleArray addObject:[self createSubtitleTrack]];
        }
        else if ([anObject intValue] == 0 && rowIndex != ([self.subtitleArray count] -1))// if this track is set to "None" and not the last track displayed
        {
            /* we know the user chose to remove this track by setting it to None, so remove it from the array */
            /* However,if this is the first track we have to reset the selected index of the next track by + 1, since it will now become
             * the first track, which has to account for the extra "Foreign Language Search" index. */
            if (rowIndex == 0 && [self.subtitleArray[1][keySubTrackSelectionIndex] intValue] != 0)
            {
                /* get the index of the selection in row one (which is track two) */
                int trackOneSelectedIndex = [self.subtitleArray[1][keySubTrackSelectionIndex] intValue];
                /* increment the index of the subtitle menu item by one, to account for Foreign Language Search which is unique to the first track */
                self.subtitleArray[1][keySubTrackSelectionIndex] = @(trackOneSelectedIndex + 1);
            }
            /* now that we have made the adjustment for track one (index 0) go ahead and delete the track */
            [self.subtitleArray removeObjectAtIndex: rowIndex];
        }

        // Validate the current passthru tracks.
        [self validatePassthru];
    }

    [aTableView reloadData];
}

#pragma mark -
#pragma mark Subtitle Table Delegate Methods

- (NSCell *)tableView:(NSTableView *)tableView dataCellForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)rowIndex
{
    if ([[tableColumn identifier] isEqualToString:@"track"])
    {
        // 'track' is a popup of all available source subtitle tracks for the given title
        NSPopUpButtonCell *cellTrackPopup = [[NSPopUpButtonCell alloc] init];
        [cellTrackPopup setControlSize:NSSmallControlSize];
        [cellTrackPopup setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];

        // Add our initial "None" track which we use to add source tracks or remove tracks.
        // "None" is always index 0.
        [[cellTrackPopup menu] addItemWithTitle:@"None" action:NULL keyEquivalent:@""];

        // Foreign Audio Search (index 1 in the popup) is only available for the first track
        if (rowIndex == 0)
        {
            [[cellTrackPopup menu] addItemWithTitle:self.foreignAudioSearchTrackName action:NULL keyEquivalent:@""];
        }

        for (NSDictionary *track in self.subtitleSourceArray)
        {
            [[cellTrackPopup menu] addItemWithTitle:track[keySubTrackName] action:NULL keyEquivalent:@""];
        }

        return [cellTrackPopup autorelease];
    }
    else if ([[tableColumn identifier] isEqualToString:@"srt_lang"])
    {
        return self.languagesCell;
    }
    else if ([[tableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        return self.encodingsCell;
    }

    return nil;
}

/**
 *  Enables/Disables the table view cells.
 */
- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        return;
    }

    // If the Track is None, we disable the other cells as None is an empty track
    if ([self.subtitleArray[rowIndex][keySubTrackSelectionIndex] intValue] == 0)
    {
        [aCell setEnabled:NO];
    }
    else
    {
        // Since we have a valid track, we go ahead and enable the rest of the widgets and set them according to the controlling array */
        [aCell setEnabled:YES];
    }

    if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
        // Disable the "Forced Only" checkbox if a) the track is "None" or b) the subtitle track doesn't support forced flags
        if (![self.subtitleArray[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_force([self.subtitleArray[rowIndex][keySubTrackType] intValue]))
        {
            [aCell setEnabled:NO];
        }
        else
        {
            [aCell setEnabled:YES];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
        /*
         * Disable the "Burned In" checkbox if:
         * a) the track is "None" OR
         * b) the subtitle track can't be burned in OR
         * c) the subtitle track can't be passed through (e.g. PGS w/MP4)
         */
        int subtitleTrackType = [self.subtitleArray[rowIndex][keySubTrackType] intValue];
        if (![self.subtitleArray[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_burn(subtitleTrackType) || !hb_subtitle_can_pass(subtitleTrackType, self.container))
        {
            [aCell setEnabled:NO];
        }
        else
        {
            [aCell setEnabled:YES];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
        /*
         * Disable the "Default" checkbox if:
         * a) the track is "None" OR
         * b) the subtitle track can't be passed through (e.g. PGS w/MP4)
         */
        if (![self.subtitleArray[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_pass([self.subtitleArray[rowIndex][keySubTrackType] intValue], self.container))
        {
            [aCell setEnabled:NO];
        }
        else
        {
            [aCell setEnabled:YES];
        }
    }
    /* These next three columns only apply to srt's. they are disabled for source subs */
    else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
    {
        /* We have an srt file so set the track type (Source or SRT, and the srt file path ) kvp's*/
        if ([self.subtitleArray[rowIndex][keySubTrackType] intValue] == SRTSUB)
        {
            [aCell setEnabled:YES];
        }
        else
        {
            [aCell setEnabled:NO];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        /* We have an srt file so set the track type (Source or SRT, and the srt file path ) kvp's*/
        if ([self.subtitleArray[rowIndex][keySubTrackType] intValue] == SRTSUB)
        {
            [aCell setEnabled:YES];
        }
        else
        {
            [aCell setEnabled:NO];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        if ([self.subtitleArray[rowIndex][keySubTrackType] intValue] == SRTSUB)
        {
            [aCell setEnabled:YES];
        }
        else
        {
            [aCell setEnabled:NO];
        }
    }
}

#pragma mark - Srt import

/**
 *  Imports a srt file.
 *
 *  @param sender
 */
- (IBAction)browseImportSrtFile:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];

    NSURL *sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastSrtImportDirectoryURL"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastSrtImportDirectoryURL"];
	}
	else
	{
		sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
	}

    /* we open up the browse srt sheet here and call for browseImportSrtFileDone after the sheet is closed */
    NSArray *fileTypes = @[@"plist", @"srt"];
    [panel setDirectoryURL:sourceDirectory];
    [panel setAllowedFileTypes:fileTypes];
    [panel beginSheetModalForWindow:[[self view] window] completionHandler:^(NSInteger result) {
        if (result == NSOKButton)
        {
            NSURL *importSrtFileURL = [panel URL];
            NSURL *importSrtDirectory = [importSrtFileURL URLByDeletingLastPathComponent];
            [[NSUserDefaults standardUserDefaults] setURL:importSrtDirectory forKey:@"LastSrtImportDirectoryURL"];

            /* Create a new entry for the subtitle source array so it shows up in our subtitle source list */
            NSString *displayname = [importSrtFileURL lastPathComponent];// grok an appropriate display name from the srt subtitle */

            /* create a dictionary of source subtitle information to store in our array */
            [self.subtitleSourceArray addObject:@{keySubTrackIndex: @(self.subtitleSourceArray.count),
                                                  keySubTrackName: displayname,
                                                  keySubTrackType: @(SRTSUB),
                                                  keySubTrackSrtFilePath: importSrtFileURL.path}];

            // Now create a new srt subtitle dictionary assuming the user wants to add it to their list
            NSMutableDictionary *newSubtitleSrtTrack = [self trackFromSourceTrackIndex:self.subtitleSourceArray.count - 1];
            // Calculate the pop up selection index
            newSubtitleSrtTrack[keySubTrackSelectionIndex] = @(self.subtitleSourceArray.count + (self.subtitleArray.count == 1));
            [self.subtitleArray insertObject:newSubtitleSrtTrack atIndex:self.subtitleArray.count - 1];

            [self.fTableView reloadData];
        }
    }];
}

#pragma mark - UI cells

- (NSArray *)populateLanguageArray
{
    NSMutableArray *languages = [[[NSMutableArray alloc] init] autorelease];

    for (const iso639_lang_t * lang = lang_get_next(NULL); lang != NULL; lang = lang_get_next(lang))
    {
        [languages addObject:@[@(lang->eng_name),
                               @(lang->iso639_2)]];
        if (!strcasecmp(lang->eng_name, "English"))
        {
            _languagesArrayDefIndex = [languages count] - 1;
        }
    }
    return [[languages copy] autorelease];
}

@synthesize languagesCell = _languagesCell;

- (NSPopUpButtonCell *)languagesCell
{
    if (!_languagesCell)
    {
        // 'srt_lang' is a popup of commonly used languages to be matched to the source srt file
        _languagesCell = [[NSPopUpButtonCell alloc] init];
        // Set the Popups properties
        [_languagesCell setControlSize:NSSmallControlSize];
        [_languagesCell setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];

        // list our languages as per the languagesArray
        for (NSArray *lang in self.languagesArray)
        {
            [[_languagesCell menu] addItemWithTitle:lang[0] action:NULL keyEquivalent:@""];
        }
    }
    return _languagesCell;
}

@synthesize encodingsCell = _encodingsCell;

- (NSPopUpButtonCell *)encodingsCell
{
    if (!_encodingsCell) {
        // 'srt_charcode' is a popup of commonly used character codes to be matched to the source srt file
        _encodingsCell = [[NSPopUpButtonCell alloc] init];
        // Set the Popups properties
        [_encodingsCell setControlSize:NSSmallControlSize];
        [_encodingsCell setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];

        // list our character codes, as per charCodeArray
        for (NSString *charCode in self.charCodeArray)
        {
            [[_encodingsCell menu] addItemWithTitle:charCode action: NULL keyEquivalent: @""];
        }
    }
    return _encodingsCell;
}

@end
