/*  HBTableView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTableView.h"

@implementation HBTableView

/**
 *  An index set containing the indexes of the targeted rows.
 *  If the selected row indexes contain the clicked row index, it returns every selected row,
 *  otherwise it returns only the clicked row index.
 */
- (NSIndexSet *)targetedRowIndexes
{
    NSMutableIndexSet *rowIndexes = [NSMutableIndexSet indexSet];
    NSIndexSet *selectedRowIndexes = self.selectedRowIndexes;
    NSInteger clickedRow = self.clickedRow;

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

- (void)keyDown:(NSEvent *)event
{
    id delegate = self.delegate;

    NSString *characters = [event charactersIgnoringModifiers];
    if (characters.length)
    {
        unichar key = [characters characterAtIndex:0];
        if ((key == NSDeleteCharacter || key == NSDeleteFunctionKey) &&
            [delegate respondsToSelector:@selector(HB_deleteSelectionFromTableView:)])
        {
            if (self.selectedRow == -1)
            {
                NSBeep();
            }
            else
            {
                [delegate HB_deleteSelectionFromTableView:self];
            }
            return;
        }
        else if (key == NSLeftArrowFunctionKey &&
                 [delegate respondsToSelector:@selector(HB_collapseSelectionFromTableView:)])
        {
            [delegate HB_collapseSelectionFromTableView:self];
            return;
        }
        else if (key == NSRightArrowFunctionKey &&
                 [delegate respondsToSelector:@selector(HB_expandSelectionFromTableView:)])
        {
            [delegate HB_expandSelectionFromTableView:self];
            return;
        }
    }

    [super keyDown:event];
}

@end
