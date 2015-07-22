//
//  HBQueueOutlineView.h
//  HandBrake
//
//  Created by Damiano Galassi on 23/11/14.
//
//

#import <Cocoa/Cocoa.h>

//------------------------------------------------------------------------------------
// As usual, we need to subclass NSOutlineView to handle a few special cases:
//
// (1) variable row heights during live resizes
// HBQueueOutlineView exists solely to get around a bug in variable row height outline
// views in which row heights get messed up during live resizes. See this discussion:
// http://lists.apple.com/archives/cocoa-dev/2005/Oct/msg00871.html
// However, the recommeneded fix (override drawRect:) does not work. Instead, this
// subclass implements viewDidEndLiveResize in order to recalculate all row heights.
//
// (2) prevent expanding of items during drags
// During dragging operations, we don't want outline items to expand, since a queue
// doesn't really have children items.
//
// (3) generate a drag image that incorporates more than just the first column
// By default, NSTableView only drags an image of the first column. Change this to
// drag an image of the queue's icon and desc columns.

@protocol HBQueueOutlineViewDelegate <NSOutlineViewDelegate>

@optional
- (void)HB_deleteSelectionFromTableView:(NSTableView *)tableView;
@end

@interface HBQueueOutlineView : NSOutlineView

@property (nonatomic, readonly) BOOL isDragging;

/**
 *  An index set containing the indexes of the targeted rows.
 *  If the selected row indexes contain the clicked row index, it returns every selected row,
 *  otherwise it returns only the clicked row index.
 */
@property (nonatomic, readonly, copy) NSIndexSet *targetedRowIndexes;

@end