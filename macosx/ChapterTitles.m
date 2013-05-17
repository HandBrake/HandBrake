/*  ChapterTitles.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */
   
#include "ChapterTitles.h"
#include "hb.h"

@implementation ChapterTitles
- (id)init 
{
    self = [super init];
    if (self != nil)
    {
        fTitle              = NULL;
        fChapterTitlesArray = [[[NSMutableArray alloc] init] retain];
    }
    return self;
}

- (void)dealloc
{
    [fChapterTitlesArray release];
    [super               dealloc];
}

- (void)resetWithTitle:(hb_title_t *)title
{
    fTitle = title;
    [fChapterTitlesArray removeAllObjects];

    if (fTitle == NULL)
        return;

    for (int i = 0; i < hb_list_count(fTitle->job->list_chapter); i++)
    {
        hb_chapter_t *chapter = hb_list_item(fTitle->job->list_chapter, i);
        if (chapter != NULL)
        {
            if (chapter->title != NULL)
            {
                [fChapterTitlesArray addObject:[NSString
                                                stringWithUTF8String:chapter->title]];
            }
            else
            {
                [fChapterTitlesArray addObject:[NSString
                                                stringWithFormat:@"Chapter %d",
                                                i + 1]];
            }
        }
    }
}

- (NSArray*)chapterTitlesArray
{
    return [NSArray arrayWithArray:fChapterTitlesArray];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    if (fTitle == NULL)
    {
        return 0;
    }
    else
    {
        return [fChapterTitlesArray count];
    }
}

- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(NSInteger)rowIndex
{
    if (aTableColumn != nil && [[aTableColumn identifier] intValue] == 2 &&
        fTitle       != NULL)
    {
        [fChapterTitlesArray replaceObjectAtIndex:rowIndex
                                       withObject:[NSString
                                                   stringWithString:anObject]];
    }
}

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(NSInteger)rowIndex
{
    if ([[aTableColumn identifier] intValue] == 1)
    {
        return [NSString stringWithFormat:@"%d", rowIndex + 1];
    }
    else if (fTitle != NULL)
    {
        return [NSString stringWithString:[fChapterTitlesArray
                                           objectAtIndex:rowIndex]];
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
    textMovement = [[[notification userInfo] objectForKey:@"NSTextMovement"] integerValue];
    if( textMovement == NSReturnTextMovement && row < [chapterTable numberOfRows] )
    {
        NSArray *info = [NSArray arrayWithObjects:chapterTable,
            [NSNumber numberWithInteger:column], [NSNumber numberWithInteger:row], nil];
        /* The delay is unimportant; editNextRow: won't be called until the responder
        chain finishes because the event loop containing the timer is on this thread */
        [self performSelector:@selector(editNextRow:) withObject:info afterDelay:0.0];
    }
}

- (void)editNextRow: (id) objects
{
    NSTableView *chapterTable = [objects objectAtIndex:0];
    NSInteger column = [[objects objectAtIndex:1] integerValue];
    NSInteger row = [[objects objectAtIndex:2] integerValue];

    if( row >= 0 && row < [chapterTable numberOfRows] )
    {
        [chapterTable selectRow:row byExtendingSelection:NO];
        [chapterTable editColumn:column row:row withEvent:nil select:YES];
    }
}
@end
