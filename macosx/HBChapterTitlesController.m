/*  HBChapterTitlesController.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */
   
#import "HBChapterTitlesController.h"
#import "HBChapter.h"
#import "HBJob.h"

@interface HBChapterTitlesController () <NSTableViewDataSource, NSTableViewDelegate>

@property (weak) IBOutlet NSTableView *table;
@property (nonatomic, readwrite, strong) NSArray *chapterTitles;

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
}

/**
 * Method to edit the next chapter when the user presses Return.
 * We queue the action on the runloop to avoid interfering
 * with the chain of events that handles the edit.
 */
- (void)controlTextDidEndEditing:(NSNotification *)notification
{
    NSTableView *chapterTable = self.table;
    NSInteger column = 2;
    NSInteger row = [self.table rowForView:[notification object]];
    NSInteger textMovement;

    // Edit the cell in the next row, same column
    row++;
    textMovement = [[notification userInfo][@"NSTextMovement"] integerValue];
    if (textMovement == NSReturnTextMovement && row < chapterTable.numberOfRows)
    {
        NSArray *info = @[chapterTable, @(column), @(row)];
        // The delay is unimportant; editNextRow: won't be called until the responder
        // chain finishes because the event loop containing the timer is on this thread
        [self performSelector:@selector(editNextRow:) withObject:info afterDelay:0.0];
    }
}

- (void)editNextRow:(id)objects
{
    NSTableView *chapterTable = objects[0];
    NSInteger column = [objects[1] integerValue];
    NSInteger row = [objects[2] integerValue];

    if (row >= 0 && row < chapterTable.numberOfRows)
    {
        [chapterTable selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
        [chapterTable editColumn:column row:row withEvent:nil select:YES];
    }
}

#pragma mark - Chapter Files Import / Export

- (IBAction)browseForChapterFile:(id)sender
{
    // We get the current file name and path from the destination field here
    NSURL *sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectory"];

	// Open a panel to let the user choose the file
	NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.allowedFileTypes = @[@"csv"];
    panel.directoryURL = sourceDirectory;

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSFileHandlingPanelOKButton)
        {
            NSString *csv = [[NSString alloc] initWithContentsOfURL:panel.URL encoding:NSUTF8StringEncoding error:NULL];
            NSMutableArray *csvArray = [[csv componentsSeparatedByString:@"\n"] mutableCopy];
            NSUInteger count = self.chapterTitles.count;

            if (csvArray.count > 0)
            {
                // if last item is empty remove it
                if ([csvArray.lastObject length] == 0)
                {
                    [csvArray removeLastObject];
                }
            }
            // if chapters in table is not equal to array count
            if (count != csvArray.count)
            {
                [panel close];
                [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", nil)
                                 defaultButton:NSLocalizedString(@"OK", nil)
                               alternateButton:NULL
                                   otherButton:NULL
                     informativeTextWithFormat:NSLocalizedString(@"%d chapters expected, %d chapters found in %@", nil),
                  count, csvArray.count, panel.URL.lastPathComponent] runModal];
            }
            else
            {
                // otherwise, go ahead and populate table with array
                NSUInteger idx = 0;
                for (NSString *csvLine in csvArray)
                {
                    if (csvLine.length > 4)
                    {
                        // Get the Range.location of the first comma in the line and then put everything after that into chapterTitle
                        NSRange firstCommaRange = [csvLine rangeOfString:@","];
                        NSString *chapterTitle = [csvLine substringFromIndex:firstCommaRange.location + 1];
                        // Since we store our chapterTitle commas as "\," for the cli, we now need to remove the escaping "\" from the title
                        chapterTitle = [chapterTitle stringByReplacingOccurrencesOfString:@"\\," withString:@","];

                        [self.chapterTitles[idx] setTitle:chapterTitle];
                        idx++;
                    }
                    else
                    {
                        [panel close];
                        [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", nil)
                                         defaultButton:NSLocalizedString(@"OK", nil)
                                       alternateButton:NULL
                                           otherButton:NULL
                             informativeTextWithFormat:NSLocalizedString(@"%@ was not formatted as expected.", nil), panel.URL.lastPathComponent] runModal];
                        break;

                    }
                }
            }
        }
    }];
}

- (IBAction)browseForChapterFileSave:(id)sender
{
    NSURL *destinationDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectory"];

    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.allowedFileTypes = @[@"csv"];
    panel.directoryURL = destinationDirectory;
    panel.nameFieldStringValue = self.job.destURL.lastPathComponent.stringByDeletingPathExtension;

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSFileHandlingPanelOKButton)
        {
            NSError *saveError;
            NSMutableString *csv = [NSMutableString string];

            NSInteger idx = 0;
            for (HBChapter *chapter in self.chapterTitles)
            {
                // put each chapter title from the table into the array
                [csv appendFormat:@"%03ld,",idx + 1];
                idx++;

                // Escape any commas in the chapter name with "\,"
                NSString *sanatizedTitle = [chapter.title stringByReplacingOccurrencesOfString:@"," withString:@"\\,"];
                [csv appendString:sanatizedTitle];
                [csv appendString:@"\n"];
            }

            [csv deleteCharactersInRange:NSMakeRange(csv.length - 1, 1)];

            // try to write it to where the user wanted
            if (![csv writeToURL:panel.URL
                      atomically:YES
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
