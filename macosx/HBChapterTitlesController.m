/*  ChapterTitles.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */
   
#import "HBChapterTitlesController.h"
#import "HBJob.h"

@interface HBChapterTitlesController () <NSTableViewDataSource, NSTableViewDelegate>
{
    IBOutlet NSTableView         * fChapterTable;
	IBOutlet NSTableColumn       * fChapterTableNameColumn;
}

@property (nonatomic, readwrite, strong) NSMutableArray *chapterTitles;

@end

@implementation HBChapterTitlesController

- (instancetype)init
{
    self = [super initWithNibName:@"ChaptersTitles" bundle:nil];
    if (self)
    {
        _chapterTitles = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)setJob:(HBJob *)job
{
    _job = job;
    self.chapterTitles = job.chapterTitles;
    [fChapterTable reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return self.chapterTitles.count;
}

- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(NSInteger)rowIndex
{
    if (aTableColumn != nil && [[aTableColumn identifier] intValue] == 2)
    {
        (self.chapterTitles)[rowIndex] = [NSString
                                                   stringWithString:anObject];
    }
}

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(NSInteger)rowIndex
{
    if ([[aTableColumn identifier] intValue] == 1)
    {
        return [NSString stringWithFormat:@"%ld", rowIndex + 1];
    }
    else
    {
        return [NSString stringWithString:(self.chapterTitles)[rowIndex]];
    }
    return @"__DATA ERROR__";
}

/* Method to edit the next chapter when the user presses Return. We have to use
a timer to avoid interfering with the chain of events that handles the edit. */
- (void)controlTextDidEndEditing: (NSNotification *) notification
{
    NSTableView *chapterTable = [notification object];
    NSInteger column = [chapterTable editedColumn];
    NSInteger row = [chapterTable editedRow];
    NSInteger textMovement;

    // Edit the cell in the next row, same column
    row++;
    textMovement = [[notification userInfo][@"NSTextMovement"] integerValue];
    if( textMovement == NSReturnTextMovement && row < [chapterTable numberOfRows] )
    {
        NSArray *info = @[chapterTable,
            @(column), @(row)];
        /* The delay is unimportant; editNextRow: won't be called until the responder
        chain finishes because the event loop containing the timer is on this thread */
        [self performSelector:@selector(editNextRow:) withObject:info afterDelay:0.0];
    }
}

- (void)editNextRow: (id) objects
{
    NSTableView *chapterTable = objects[0];
    NSInteger column = [objects[1] integerValue];
    NSInteger row = [objects[2] integerValue];

    if( row >= 0 && row < [chapterTable numberOfRows] )
    {
        [chapterTable selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
        [chapterTable editColumn:column row:row withEvent:nil select:YES];
    }
}

#pragma mark -
#pragma mark Chapter Files Import / Export

- (IBAction) browseForChapterFile: (id) sender
{
    /* We get the current file name and path from the destination field here */
    NSURL *sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectory"];

	/* Open a panel to let the user choose the file */
	NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setDirectoryURL:sourceDirectory];
    [panel setAllowedFileTypes:@[@"csv"]];

    [panel beginSheetModalForWindow:[[self view] window] completionHandler:^(NSInteger result) {
        NSArray *chaptersArray = nil; /* temp array for chapters */
        NSMutableArray *chaptersMutableArray = nil; /* temp array for chapters */
        NSString *chapterName = nil; 	/* temp string from file */
        NSInteger chapters, i;

        if (result == NSOKButton)  /* if they click OK */
        {
            chapterName = [[NSString alloc] initWithContentsOfURL:[panel URL] encoding:NSUTF8StringEncoding error:NULL];
            chaptersArray = [chapterName componentsSeparatedByString:@"\n"];
            chaptersMutableArray = [chaptersArray mutableCopy];
            chapters = [self numberOfRowsInTableView:fChapterTable];
            if ([chaptersMutableArray count] > 0)
            {
                /* if last item is empty remove it */
                if ([chaptersMutableArray[[chaptersArray count]-1] length] == 0)
                {
                    [chaptersMutableArray removeLastObject];
                }
            }
            /* if chapters in table is not equal to array count */
            if ((unsigned int) chapters != [chaptersMutableArray count])
            {
                [panel close];
                [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", @"Unable to load chapter file")
                                 defaultButton:NSLocalizedString(@"OK", @"OK")
                               alternateButton:NULL
                                   otherButton:NULL
                     informativeTextWithFormat:NSLocalizedString(@"%d chapters expected, %d chapters found in %@", @"%d chapters expected, %d chapters found in %@"),
                  chapters, [chaptersMutableArray count], [[panel URL] lastPathComponent]] runModal];
                return;
            }
            /* otherwise, go ahead and populate table with array */
            for (i=0; i<chapters; i++)
            {

                if([chaptersMutableArray[i] length] > 5)
                {
                    /* avoid a segfault */
                    /* Get the Range.location of the first comma in the line and then put everything after that into chapterTitle */
                    NSRange firstCommaRange = [chaptersMutableArray[i] rangeOfString:@","];
                    NSString *chapterTitle = [chaptersMutableArray[i] substringFromIndex:firstCommaRange.location + 1];
                    /* Since we store our chapterTitle commas as "\," for the cli, we now need to remove the escaping "\" from the title */
                    chapterTitle = [chapterTitle stringByReplacingOccurrencesOfString:@"\\," withString:@","];
                    [self tableView:fChapterTable
                     setObjectValue:chapterTitle
                     forTableColumn:fChapterTableNameColumn
                                row:i];
                }
                else
                {
                    [panel close];
                    [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", @"Unable to load chapter file")
                                     defaultButton:NSLocalizedString(@"OK", @"OK")
                                   alternateButton:NULL
                                       otherButton:NULL
                         informativeTextWithFormat:NSLocalizedString(@"%@ was not formatted as expected.", @"%@ was not formatted as expected."), [[panel URL] lastPathComponent]] runModal];
                    [fChapterTable reloadData];
                    return;
                }
            }
            [fChapterTable reloadData];
        }
    }];
}

- (IBAction) browseForChapterFileSave: (id) sender
{
    NSURL *destinationDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectory"];

    /* Open a panel to let the user save to a file */
    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setAllowedFileTypes:@[@"csv"]];
    [panel setDirectoryURL:destinationDirectory];
    [panel setNameFieldStringValue:self.job.destURL.lastPathComponent.stringByDeletingPathExtension];

    [panel beginSheetModalForWindow:[[self view] window] completionHandler:^(NSInteger result) {
        NSString *chapterName;      /* pointer for string for later file-writing */
        NSString *chapterTitle;
        NSError *saveError = nil;
        NSInteger chapters, i;    /* ints for the number of chapters in the table and the loop */

        if( result == NSOKButton )   /* if they clicked OK */
        {
            chapters = [self numberOfRowsInTableView:fChapterTable];
            chapterName = [NSString string];
            for (i=0; i<chapters; i++)
            {
                /* put each chapter title from the table into the array */
                if (i<9)
                { /* if i is from 0 to 8 (chapters 1 to 9) add two leading zeros */
                    chapterName = [chapterName stringByAppendingFormat:@"00%ld,",i+1];
                }
                else if (i<99)
                { /* if i is from 9 to 98 (chapters 10 to 99) add one leading zero */
                    chapterName = [chapterName stringByAppendingFormat:@"0%ld,",i+1];
                }
                else if (i<999)
                { /* in case i is from 99 to 998 (chapters 100 to 999) no leading zeros */
                    chapterName = [chapterName stringByAppendingFormat:@"%ld,",i+1];
                }

                chapterTitle = [self tableView:fChapterTable objectValueForTableColumn:fChapterTableNameColumn row:i];
                /* escape any commas in the chapter name with "\," */
                chapterTitle = [chapterTitle stringByReplacingOccurrencesOfString:@"," withString:@"\\,"];
                chapterName = [chapterName stringByAppendingString:chapterTitle];
                if (i+1 != chapters)
                { /* if not the last chapter */
                    chapterName = [chapterName stringByAppendingString:@ "\n"];
                }


            }
            /* try to write it to where the user wanted */
            if (![chapterName writeToURL:[panel URL]
                              atomically:NO
                                encoding:NSUTF8StringEncoding
                                   error:&saveError])
            {
                [panel close];
                [[NSAlert alertWithError:saveError] runModal];
            }
        }
    }];
}

@end
