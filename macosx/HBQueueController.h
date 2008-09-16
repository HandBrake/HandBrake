/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */


#import <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;


#define HB_QUEUE_DRAGGING 0             // <--- NOT COMPLETELY FUNCTIONAL YET
#define HB_OUTLINE_METRIC_CONTROLS 0    // for tweaking the outline cell spacings



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
{

BOOL                        fIsDragging;

}

- (BOOL) isDragging;

@end




@interface HBQueueController : NSWindowController
{
    hb_handle_t                  *fQueueEncodeLibhb;              // reference to libhb
    HBController                 *fHBController;        // reference to HBController
    NSMutableArray               *fJobGroups;           // mirror image of the queue array from controller.mm

    int                          fEncodingQueueItem;     // corresponds to the index of fJobGroups encoding item
    int                          fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    int                          fCompletedCount;
    int                          fCanceledCount;
    int                          fWorkingCount;
    BOOL                         fJobGroupCountsNeedUpdating;

    BOOL                         fCurrentJobPaneShown;  // NO when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    NSMutableIndexSet            *fSavedExpandedItems;  // used by save/restoreOutlineViewState to preserve which items are expanded
    NSMutableIndexSet            *fSavedSelectedItems;  // used by save/restoreOutlineViewState to preserve which items are selected
#if HB_QUEUE_DRAGGING
    NSArray                      *fDraggedNodes;
#endif
    NSTimer                      *fAnimationTimer;      // animates the icon of the current job in the queue outline view
    int                          fAnimationIndex;       // used to generate name of image used to animate the current job in the queue outline view

    //  +------------------window-------------------+
    //  |+-------------fCurrentJobPane-------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  |+---------------fQueuePane----------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  +-------------------------------------------+

    // fCurrentJobPane - visible only when processing a job
    IBOutlet NSView              *fCurrentJobPane;
    IBOutlet NSImageView         *fJobIconView;
    IBOutlet NSTextField         *fJobDescTextField;
    IBOutlet NSProgressIndicator *fProgressBar;
    IBOutlet NSTextField         *fProgressTextField;

    // fQueuePane - always visible; fills entire window when fCurrentJobPane is hidden
    IBOutlet NSView              *fQueuePane;
    IBOutlet HBQueueOutlineView  *fOutlineView;
    IBOutlet NSTextField         *fQueueCountField;
    NSArray                      *fDraggedNodes;
#if HB_OUTLINE_METRIC_CONTROLS
    IBOutlet NSSlider            *fIndentation; // debug
    IBOutlet NSSlider            *fSpacing;     // debug
#endif

}

- (void)setHandle: (hb_handle_t *)handle;
- (void)setHBController: (HBController *)controller;

- (void)setupToolbar;

- (void)setQueueArray: (NSMutableArray *)QueueFileArray;
- (id)outlineView:(NSOutlineView *)fOutlineView child:(NSInteger)index ofItem:(id)item;

- (BOOL)outlineView:(NSOutlineView *)fOutlineView isItemExpandable:(id)item;

- (BOOL)outlineView:(NSOutlineView *)fOutlineView shouldExpandItem:(id)item;

- (NSInteger)outlineView:(NSOutlineView *)fOutlineView numberOfChildrenOfItem:(id)item;

- (id)outlineView:(NSOutlineView *)fOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;

- (void)outlineView:(NSOutlineView *)fOutlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item;

- (void)moveObjectsInQueueArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(unsigned)insertIndex;

/* Animate the icon for the current encode */
- (void) animateWorkingEncodeIconInQueue;
- (void) startAnimatingCurrentWorkingEncodeInQueue;
- (void) stopAnimatingCurrentJobGroupInQueue;


- (IBAction)showQueueWindow: (id)sender;


/* control encodes in the window */
- (IBAction)removeSelectedQueueItem: (id)sender;
- (IBAction)revealSelectedQueueItem: (id)sender;

#if HB_OUTLINE_METRIC_CONTROLS
- (IBAction)imageSpacingChanged: (id)sender;
- (IBAction)indentChanged: (id)sender;
#endif





@end
