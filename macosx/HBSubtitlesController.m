/* $Id: HBSubtitles.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesController.h"
#import "HBSubtitlesDefaultsController.h"

#import "HBSubtitles.h"
#import "HBSubtitlesDefaults.h"

#include "hb.h"
#include "lang.h"

static void *HBSubtitlesControllerContext = &HBSubtitlesControllerContext;

@interface HBSubtitlesController () <NSTableViewDataSource, NSTableViewDelegate>

// IBOutles
@property (assign) IBOutlet NSTableView *fTableView;

// Defaults
@property (nonatomic, readwrite, retain) HBSubtitlesDefaultsController *defaultsController;

// Cached table view's cells
@property (nonatomic, readonly) NSPopUpButtonCell *languagesCell;
@property (nonatomic, readonly) NSPopUpButtonCell *encodingsCell;

@end

@implementation HBSubtitlesController

- (instancetype)init
{
    self = [super initWithNibName:@"Subtitles" bundle:nil];

    [self addObserver:self forKeyPath:@"self.subtitles.tracks" options:NSKeyValueObservingOptionInitial context:HBSubtitlesControllerContext];

    return self;
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSubtitlesControllerContext)
    {
        // We use KVO to update the table manually
        // because this table isn't using bindings
        if ([keyPath isEqualToString:@"self.subtitles.tracks"])
        {
            [self.fTableView reloadData];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)setSubtitles:(HBSubtitles *)subtitles
{
    _subtitles = subtitles;

    [self.fTableView reloadData];    
}

#pragma mark - Actions

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    return (self.subtitles != nil);
}

/**
 *  Add every subtitles track that still isn't in the subtitles array.
 */
- (IBAction)addAll:(id)sender
{
    [self.subtitles addAllTracks];
    [self.fTableView reloadData];
}

/**
 *  Remove all the subtitles tracks.
 */
- (IBAction)removeAll:(id)sender
{
    [self.subtitles removeAll];
    [self.fTableView reloadData];
}

/**
 *  Remove all the subtitles tracks and
 *  add new ones based on the defaults settings
 */
- (IBAction)addTracksFromDefaults:(id)sender
{
    [self.subtitles reloadDefaults];
    [self.fTableView reloadData];
}

- (IBAction)showSettingsSheet:(id)sender
{
    self.defaultsController = [[[HBSubtitlesDefaultsController alloc] initWithSettings:self.subtitles.defaults] autorelease];

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

#pragma mark - Subtitles tracks creation and validation

/**
 *  Checks whether any subtitles in the list cannot be passed through.
 *  Set the first of any such subtitles to burned-in, remove the others.
 */
- (void)validatePassthru
{
    [self.subtitles validatePassthru];
    [self.fTableView reloadData];
}

- (void)validateBurned:(NSInteger)index
{
    [self.subtitles.tracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
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
    [self.subtitles.tracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
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
    return self.subtitles.tracks.count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    NSDictionary *track = self.subtitles.tracks[rowIndex];

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
            NSMutableDictionary *newTrack = [self.subtitles trackFromSourceTrackIndex:[anObject integerValue] - 1 - (rowIndex == 0)];
            // Selection index calculation
            newTrack[keySubTrackSelectionIndex] = @([anObject integerValue]);
            self.subtitles.tracks[rowIndex] = newTrack;
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
        self.subtitles.tracks[rowIndex][keySubTrackForced] = @([anObject intValue]);
    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
        self.subtitles.tracks[rowIndex][keySubTrackBurned] = @([anObject intValue]);
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            self.subtitles.tracks[rowIndex][keySubTrackDefault] = @0;
        }
        /* now we need to make sure no other tracks are set to burned if we have set burned */
        if ([anObject intValue] == 1)
        {
            [self validateBurned:rowIndex];
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
        self.subtitles.tracks[rowIndex][keySubTrackDefault] = @([anObject intValue]);
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            self.subtitles.tracks[rowIndex][keySubTrackBurned] = @0;
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
        self.subtitles.tracks[rowIndex][keySubTrackLanguageIndex] = @([anObject intValue]);
        self.subtitles.tracks[rowIndex][keySubTrackLanguageIsoCode] = self.subtitles.languagesArray[[anObject intValue]][1];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        /* charCodeArray */
        self.subtitles.tracks[rowIndex][keySubTrackSrtCharCodeIndex] = @([anObject intValue]);
        self.subtitles.tracks[rowIndex][keySubTrackSrtCharCode] = self.subtitles.charCodeArray[[anObject intValue]];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        self.subtitles.tracks[rowIndex][keySubTrackSrtOffset] = @([anObject integerValue]);
    }

    /* now lets do a bit of logic to add / remove tracks as necessary via the "None" track (index 0) */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        /* Since currently no quicktime based playback devices support soft vobsubs in mp4, we make sure "burned in" is specified
         * by default to avoid massive confusion and anarchy. However we also want to guard against multiple burned in subtitle tracks
         * as libhb would ignore all but the first one anyway. Plus it would probably be stupid.
         */
        if ((self.subtitles.container & HB_MUX_MASK_MP4) && ([anObject intValue] != 0))
        {
            if ([self.subtitles.tracks[rowIndex][keySubTrackType] intValue] == VOBSUB)
            {
                /* lets see if there are currently any burned in subs specified */
                BOOL subtrackBurnedInFound = NO;
                for (id tempObject in self.subtitles.tracks)
                {
                    if ([tempObject[keySubTrackBurned] intValue] == 1)
                    {
                        subtrackBurnedInFound = YES;
                    }
                }
                /* if we have no current vobsub set to burn it in ... burn it in by default */
                if (!subtrackBurnedInFound)
                {
                    self.subtitles.tracks[rowIndex][keySubTrackBurned] = @1;
                    /* Burned In and Default are mutually exclusive */
                    self.subtitles.tracks[rowIndex][keySubTrackDefault] = @0;
                }
            }
        }

        /* We use the track popup index number (presumes index 0 is "None" which is ignored and only used to remove tracks if need be)
         * to determine whether to 1 modify an existing track, 2. add a new empty "None" track or 3. remove an existing track.
         */

        if ([anObject intValue] != 0 && rowIndex == [self.subtitles.tracks count] - 1) // if we have a last track which != "None"
        {
            /* add a new empty None track */
            [self.subtitles.tracks addObject:[self.subtitles createSubtitleTrack]];
        }
        else if ([anObject intValue] == 0 && rowIndex != ([self.subtitles.tracks count] -1))// if this track is set to "None" and not the last track displayed
        {
            /* we know the user chose to remove this track by setting it to None, so remove it from the array */
            /* However,if this is the first track we have to reset the selected index of the next track by + 1, since it will now become
             * the first track, which has to account for the extra "Foreign Language Search" index. */
            if (rowIndex == 0 && [self.subtitles.tracks[1][keySubTrackSelectionIndex] intValue] != 0)
            {
                /* get the index of the selection in row one (which is track two) */
                int trackOneSelectedIndex = [self.subtitles.tracks[1][keySubTrackSelectionIndex] intValue];
                /* increment the index of the subtitle menu item by one, to account for Foreign Language Search which is unique to the first track */
                self.subtitles.tracks[1][keySubTrackSelectionIndex] = @(trackOneSelectedIndex + 1);
            }
            /* now that we have made the adjustment for track one (index 0) go ahead and delete the track */
            [self.subtitles.tracks removeObjectAtIndex: rowIndex];
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
            [[cellTrackPopup menu] addItemWithTitle:self.subtitles.foreignAudioSearchTrackName action:NULL keyEquivalent:@""];
        }

        for (NSDictionary *track in self.subtitles.masterTrackArray)
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
    if ([self.subtitles.tracks[rowIndex][keySubTrackSelectionIndex] intValue] == 0)
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
        if (![self.subtitles.tracks[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_force([self.subtitles.tracks[rowIndex][keySubTrackType] intValue]))
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
        int subtitleTrackType = [self.subtitles.tracks[rowIndex][keySubTrackType] intValue];
        if (![self.subtitles.tracks[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_burn(subtitleTrackType) || !hb_subtitle_can_pass(subtitleTrackType, self.subtitles.container))
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
        if (![self.subtitles.tracks[rowIndex][keySubTrackSelectionIndex] intValue] ||
            !hb_subtitle_can_pass([self.subtitles.tracks[rowIndex][keySubTrackType] intValue], self.subtitles.container))
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
        if ([self.subtitles.tracks[rowIndex][keySubTrackType] intValue] == SRTSUB)
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
        if ([self.subtitles.tracks[rowIndex][keySubTrackType] intValue] == SRTSUB)
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
        if ([self.subtitles.tracks[rowIndex][keySubTrackType] intValue] == SRTSUB)
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
            [self.subtitles.masterTrackArray addObject:@{keySubTrackIndex: @(self.subtitles.masterTrackArray.count),
                                                  keySubTrackName: displayname,
                                                  keySubTrackType: @(SRTSUB),
                                                  keySubTrackSrtFilePath: importSrtFileURL.path}];

            // Now create a new srt subtitle dictionary assuming the user wants to add it to their list
            NSMutableDictionary *newSubtitleSrtTrack = [self.subtitles trackFromSourceTrackIndex:self.subtitles.masterTrackArray.count - 1];
            // Calculate the pop up selection index
            newSubtitleSrtTrack[keySubTrackSelectionIndex] = @(self.subtitles.masterTrackArray.count + (self.subtitles.tracks.count == 1));
            [self.subtitles.tracks insertObject:newSubtitleSrtTrack atIndex:self.subtitles.tracks.count - 1];

            [self.fTableView reloadData];
        }
    }];
}

#pragma mark - UI cells

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
        for (NSArray *lang in self.subtitles.languagesArray)
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
        for (NSString *charCode in self.subtitles.charCodeArray)
        {
            [[_encodingsCell menu] addItemWithTitle:charCode action: NULL keyEquivalent: @""];
        }
    }
    return _encodingsCell;
}

@end
