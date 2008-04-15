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
@end
