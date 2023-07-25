/*  HBChapterTitlesController.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBChapterTitlesController.h"
#import "HBPreferencesKeys.h"
@import HandBrakeKit;

@interface NSArray (HBCSVAdditions)

+ (nullable NSArray<NSArray<NSString *> *> *)HB_arrayWithContentsOfCSVURL:(NSURL *)url;

@end

@implementation NSArray (HBCSVAdditions)

// CSV parsing examples
// CSV Record:
//     one,two,three
// Fields:
//     <one>
//     <two>
//     <three>
// CSV Record:
//     one, two, three
// Fields:
//     <one>
//     < two>
//     < three>
// CSV Record:
//     one,"2,345",three
// Fields:
//     <one>
//     <2,345>
//     <three>
// CSV record:
//     one,"John said, ""Hello there.""",three
// Explanation: inside a quoted field, two double quotes in a row count
// as an escaped double quote in the field data.
// Fields:
//     <one>
//     <John said, "Hello there.">
//     <three>
+ (nullable NSArray<NSArray<NSString *> *> *)HB_arrayWithContentsOfCSVURL:(NSURL *)url
{
    NSString *str = [[NSString alloc] initWithContentsOfURL:url encoding:NSUTF8StringEncoding error:NULL];

    if (str == nil)
    {
        return nil;
    }

    NSMutableString *csvString = [str mutableCopy];
    [csvString replaceOccurrencesOfString:@"\r\n" withString:@"\n" options:NSLiteralSearch range:NSMakeRange(0, csvString.length)];
    [csvString replaceOccurrencesOfString:@"\r" withString:@"\n" options:NSLiteralSearch range:NSMakeRange(0, csvString.length)];

    if (!csvString)
    {
        return 0;
    }

    if ([csvString characterAtIndex:0] == 0xFEFF)
    {
        [csvString deleteCharactersInRange:NSMakeRange(0,1)];
    }
    if ([csvString characterAtIndex:[csvString length]-1] != '\n')
    {
        [csvString appendFormat:@"%c",'\n'];
    }

    NSScanner *sc = [NSScanner scannerWithString:csvString];
    sc.charactersToBeSkipped =  nil;
    NSMutableArray *csvArray = [NSMutableArray array];
    [csvArray addObject:[NSMutableArray array]];
    NSCharacterSet *commaNewlineCS = [NSCharacterSet characterSetWithCharactersInString:@",\n"];

    while (sc.scanLocation < csvString.length)
    {
        if ([sc scanString:@"\"" intoString:NULL])
        {
            // Quoted field
            NSMutableString *field = [NSMutableString string];
            BOOL done = NO;
            NSString *quotedString;
            // Scan until we get to the end double quote or the EOF.
            while (!done && sc.scanLocation < csvString.length)
            {
                if ([sc scanUpToString:@"\"" intoString:&quotedString])
                {
                    [field appendString:quotedString];
                }
                if ([sc scanString:@"\"\"" intoString:NULL])
                {
                    // Escaped double quote inside the quoted string.
                    [field appendString:@"\""];
                }
                else
                {
                    done = YES;
                }
            }
            if (sc.scanLocation < csvString.length)
            {
                ++sc.scanLocation;
                BOOL nextIsNewline = [sc scanString:@"\n" intoString:NULL];
                BOOL nextIsComma = NO;
                if (!nextIsNewline)
                {
                    nextIsComma = [sc scanString:@"," intoString:NULL];
                }
                if (nextIsNewline || nextIsComma)
                {
                    [[csvArray lastObject] addObject:field];
                    if (nextIsNewline && sc.scanLocation < csvString.length)
                    {
                        [csvArray addObject:[NSMutableArray array]];
                    }
                }
                else
                {
                    // Quoted fields must be immediately followed by a comma or newline.
                    return nil;
                }
            }
            else
            {
                // No close quote found before EOF, so file is invalid CSV.
                return nil;
            }
        }
        else
        {
            NSString *field;
            [sc scanUpToCharactersFromSet:commaNewlineCS intoString:&field];
            BOOL nextIsNewline = [sc scanString:@"\n" intoString:NULL];
            BOOL nextIsComma = NO;
            if (!nextIsNewline)
            {
                nextIsComma = [sc scanString:@"," intoString:NULL];
            }
            if (nextIsNewline || nextIsComma)
            {
                [[csvArray lastObject] addObject:field];
                if (nextIsNewline && sc.scanLocation < csvString.length)
                {
                    [csvArray addObject:[NSMutableArray array]];
                }
            }
        }
    }
    return csvArray;
}

@end

@interface HBChapterTitlesController () <NSTableViewDataSource, NSTableViewDelegate>

@property (nonatomic, weak) IBOutlet NSTableView *table;
@property (nonatomic, readwrite, strong) NSArray<HBChapter *> *chapterTitles;

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

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.table.doubleAction = @selector(doubleClickAction:);
}

/**
 * Method to edit the next chapter when the user presses Return.
 * We queue the action on the runloop to avoid interfering
 * with the chain of events that handles the edit.
 */
- (void)controlTextDidEndEditing:(NSNotification *)notification
{
    NSTableView *chapterTable = self.table;
    NSInteger column = [self.table columnForView:[notification object]];
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

- (IBAction)doubleClickAction:(NSTableView *)sender
{
    if (sender.clickedRow > -1) {
        NSTableColumn *column = sender.tableColumns[sender.clickedColumn];
        if ([column.identifier isEqualToString:@"title"]) {
            // edit the cell
            [sender editColumn:sender.clickedColumn
                           row:sender.clickedRow
                     withEvent:nil
                        select:YES];
        }
    }
}

#pragma mark - Chapter Files Import / Export

- (BOOL)importChaptersFromURL:(NSURL *)URL error:(NSError **)outError
{
    NSArray<NSArray<NSString *> *> *csvData = [NSArray HB_arrayWithContentsOfCSVURL:URL];
    if (csvData.count == self.chapterTitles.count)
    {
        NSUInteger i = 0;
        for (NSArray<NSString *> *lineFields in csvData)
        {
            if (lineFields.count < 2 || [lineFields[0] integerValue] != i + 1)
            {
                if (NULL != outError)
                {
                    *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Invalid chapters CSV file", @"Chapters import -> invalid CSV description"),
                                                                                      NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(@"The CSV file is not a valid chapters CSV file.", @"Chapters import -> invalid CSV recovery suggestion")}];
                }
                return NO;
            }
            i++;
        }

        NSUInteger j = 0;
        for (NSArray<NSString *> *lineFields in csvData)
        {
            [self.chapterTitles[j] setTitle:lineFields[1]];
            j++;
        }
        return YES;
    }

    if (NULL != outError)
    {
        *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Incorrect line count", @"Chapters import -> invalid CSV line count description"),
                                                                          NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(@"The line count in the chapters CSV file does not match the number of chapters in the movie.", @"Chapters import -> invalid CSV line count recovery suggestion")}];
    }

    return NO;
}

- (IBAction)browseForChapterFile:(id)sender
{
    // We get the current file name and path from the destination field here
    NSURL *sourceDirectory = [NSUserDefaults.standardUserDefaults URLForKey:HBLastDestinationDirectoryURL];

	// Open a panel to let the user choose the file
	NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.allowedFileTypes = @[@"csv", @"txt"];
    panel.directoryURL = sourceDirectory;

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSModalResponseOK)
        {
            NSError *error;
            if ([self importChaptersFromURL:panel.URL error:&error] == NO)
            {
                [self presentError:error];
            }
            [panel.URL stopAccessingSecurityScopedResource];
        }
    }];
}

- (IBAction)browseForChapterFileSave:(id)sender
{
    NSURL *destinationDirectory = [NSUserDefaults.standardUserDefaults URLForKey:HBLastDestinationDirectoryURL];

    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.allowedFileTypes = @[@"csv"];
    panel.directoryURL = destinationDirectory;
    panel.nameFieldStringValue = self.job.destinationFileName.stringByDeletingPathExtension;

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSModalResponseOK)
        {
            NSError *saveError;
            NSMutableString *csv = [NSMutableString string];

            NSInteger idx = 0;
            for (HBChapter *chapter in self.chapterTitles)
            {
                // put each chapter title from the table into the array
                [csv appendFormat:@"%ld,",idx + 1];
                idx++;

                NSString *sanitizedTitle = [chapter.title stringByReplacingOccurrencesOfString:@"\"" withString:@"\"\""];

                // If the title contains any commas or quotes, add quotes
                if ([sanitizedTitle containsString:@","] || [sanitizedTitle containsString:@"\""])
                {
                    [csv appendString:@"\""];
                    [csv appendString:sanitizedTitle];
                    [csv appendString:@"\""];
                }
                else
                {
                    [csv appendString:sanitizedTitle];
                }
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

            [panel.URL stopAccessingSecurityScopedResource];
        }
    }];
}

@end
