/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */


#import <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;

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

@interface HBQueueOutlineView : NSOutlineView

@property (nonatomic, readonly) BOOL isDragging;

@end

@interface HBQueueController : NSWindowController <NSToolbarDelegate, NSWindowDelegate>

- (void)setPidNum: (int)myPidnum;
- (void)setHandle: (hb_handle_t *)handle;
- (void)setHBController: (HBController *)controller;

- (void)setQueueArray: (NSMutableArray *)QueueFileArray;

/* Animate the icon for the current encode */
- (void) animateWorkingEncodeIconInQueue;
- (void) startAnimatingCurrentWorkingEncodeInQueue;
- (void) stopAnimatingCurrentJobGroupInQueue;
- (void)setQueueStatusString: (NSString *)statusString;

- (IBAction)showQueueWindow: (id)sender;

/* control encodes in the window */
- (IBAction)removeSelectedQueueItem: (id)sender;
- (IBAction)revealSelectedQueueItem: (id)sender;
- (IBAction)editSelectedQueueItem: (id)sender;

@end
