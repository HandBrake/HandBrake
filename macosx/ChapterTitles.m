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
    if( self != nil )
    {
        fTitle = NULL;
    }
    
    return self;
}

- (void)resetWithTitle:(hb_title_t *)title
{
    int i;
    NSString *chapterString;
    
    fTitle = title;

    if (!title)
        return;

    int count = hb_list_count( title->list_chapter );

    for( i = 0; i < count; i++ )
    {
        hb_chapter_t *chapter = hb_list_item( title->list_chapter, i );
        
        if( chapter != NULL && chapter->title[0] == '\0' )
        {
            chapterString = [NSString stringWithFormat:@"Chapter %2d",(i+1)];
    
            strncpy( chapter->title, [chapterString UTF8String], 1023);
            chapter->title[1023] = '\0';
        }
    }
    
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    if( fTitle == NULL )
    {
        return 0;
    }
    else
    {
        return hb_list_count( fTitle->list_chapter );
    }
}

- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(int)rowIndex
{
    if(aTableColumn != nil && [[aTableColumn identifier] intValue] == 2)
    {
        hb_chapter_t *chapter = hb_list_item( fTitle->list_chapter, rowIndex );
        
        if( chapter != NULL )
        {
            strncpy( chapter->title, [anObject UTF8String], 1023);
            chapter->title[1023] = '\0';
        }
    }
}

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(int)rowIndex
{
    NSString *cellEntry;

    if([[aTableColumn identifier] intValue] == 1)
    {
        cellEntry = [NSString stringWithFormat:@"%d",rowIndex+1];
    }
    else
    {
        hb_chapter_t *chapter = hb_list_item( fTitle->list_chapter, rowIndex );
        
        if( chapter != NULL )
        {
            cellEntry = [NSString stringWithUTF8String:chapter->title];
        }
        else
        {
            cellEntry = @"__DATA ERROR__";
        }
    }
    
    return cellEntry;
}

/* Method to edit the next chapter when the user presses Return. We have to use
a timer to avoid interfering with the chain of events that handles the edit. */
- (void)controlTextDidEndEditing: (NSNotification *) notification
{
    NSTableView *chapterTable = [notification object];
    NSInteger column = [chapterTable editedColumn];
    NSInteger row = [chapterTable editedRow];
    int textMovement;

    // Edit the cell in the next row, same column
    row++;
    textMovement = [[[notification userInfo] objectForKey:@"NSTextMovement"] intValue];
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
