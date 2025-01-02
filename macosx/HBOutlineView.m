/*  HBOutlineView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutlineView.h"

@implementation HBOutlineView

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

        // If we clicked on a selected row, then we want to consider all rows in the selection.
        // Otherwise, we only consider the clicked on row.
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
