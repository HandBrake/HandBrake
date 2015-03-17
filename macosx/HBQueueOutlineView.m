//
//  HBQueueOutlineView.m
//  HandBrake
//
//  Created by Damiano Galassi on 23/11/14.
//
//

#import "HBQueueOutlineView.h"

@implementation HBQueueOutlineView

- (void)viewDidEndLiveResize
{
    // Since we disabled calculating row heights during a live resize, force them to
    // recalculate now.
    [self noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [self numberOfRows])]];
    [super viewDidEndLiveResize];
}

/* This should be for dragging, we take this info from the presets right now */
- (NSImage *)dragImageForRowsWithIndexes:(NSIndexSet *)dragRows
                            tableColumns:(NSArray *)tableColumns
                                   event:(NSEvent *)dragEvent
                                  offset:(NSPointPointer)dragImageOffset
{
    _isDragging = YES;

    // By default, NSTableView only drags an image of the first column. Change this to
    // drag an image of the queue's icon and desc and action columns.
    NSArray *cols = @[[self tableColumnWithIdentifier:@"desc"],
                      [self tableColumnWithIdentifier:@"icon"],
                      [self tableColumnWithIdentifier:@"action"]];

    return [super dragImageForRowsWithIndexes:dragRows tableColumns:cols event:dragEvent offset:dragImageOffset];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [super mouseDown:theEvent];
    _isDragging = NO;
}

- (void)keyDown:(NSEvent *)event
{
    id delegate = [self delegate];

    unichar key = [[event charactersIgnoringModifiers] characterAtIndex:0];
    if ((key == NSDeleteCharacter || key == NSDeleteFunctionKey) &&
        [delegate respondsToSelector:@selector(HB_deleteSelectionFromTableView:)])
    {
        if ([self selectedRow] == -1)
        {
            NSBeep();
        }
        else
        {
            [delegate HB_deleteSelectionFromTableView:self];
        }
        return;
    }
    else
    {
        [super keyDown:event];
    }
}

/**
 *  An index set containing the indexes of the targeted rows.
 *  If the selected row indexes contain the clicked row index, it returns every selected row,
 *  otherwise it returns only the clicked row index.
 */
- (NSIndexSet *)targetedRowIndexes
{
    NSMutableIndexSet *rowIndexes = [NSMutableIndexSet indexSet];
    NSIndexSet *selectedRowIndexes = [self selectedRowIndexes];
    NSInteger clickedRow = [self clickedRow];

    if (clickedRow != -1)
    {
        [rowIndexes addIndex:clickedRow];

        // If we clicked on a selected row, then we want to consider all rows in the selection. Otherwise, we only consider the clicked on row.
        if ([selectedRowIndexes containsIndex:clickedRow])
        {
            [rowIndexes addIndexes:selectedRowIndexes];
        }
    }
    else
    {
        [rowIndexes addIndexes:selectedRowIndexes];
    }

    return [rowIndexes copy];
}

@end
