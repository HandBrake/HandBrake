/*  HBQueueTableViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueTableViewController.h"

#import "HBTableView.h"
#import "HBQueueItemView.h"

// Pasteboard type for or drag operations
#define HBQueueDragDropPboardType @"HBQueueCustomTableViewPboardType"

@interface HBQueueTableViewController () <NSTableViewDataSource, NSTableViewDelegate, HBQueueItemViewDelegate>

@property (nonatomic, weak, readonly) HBQueue *queue;
@property (nonatomic) NSArray<HBQueueItem *> *dragNodesArray;

@property (nonatomic, strong) id<HBQueueTableViewControllerDelegate> delegate;

@property (nonatomic, weak) IBOutlet HBTableView *tableView;

@end

@implementation HBQueueTableViewController

- (instancetype)initWithQueue:(HBQueue *)state delegate:(id<HBQueueTableViewControllerDelegate>)delegate
{
    self = [super init];
    if (self)
    {
        _queue = state;
        _delegate = delegate;
    }
    return self;
}

- (NSString *)nibName
{
    return @"HBQueueTableViewController";
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // lets setup our queue list table view for drag and drop here
    [self.tableView registerForDraggedTypes:@[HBQueueDragDropPboardType]];
    [self.tableView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [self.tableView setVerticalMotionCanBeginDrag:YES];

    // Reloads the queue, this is called
    // when another HandBrake instances modifies the queue
    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueReloadItemsNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        [self.tableView reloadData];
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidAddItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        NSIndexSet *indexes = note.userInfo[HBQueueItemNotificationIndexesKey];
        [self.tableView insertRowsAtIndexes:indexes withAnimation:NSTableViewAnimationSlideDown];
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidRemoveItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        NSIndexSet *indexes = note.userInfo[HBQueueItemNotificationIndexesKey];
        [self.tableView removeRowsAtIndexes:indexes withAnimation:NSTableViewAnimationSlideUp];
        [self.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:indexes.firstIndex] byExtendingSelection:NO];
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidMoveItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        NSArray<NSNumber *> *source = note.userInfo[HBQueueItemNotificationSourceIndexesKey];
        NSArray<NSNumber *> *target = note.userInfo[HBQueueItemNotificationTargetIndexesKey];

        [self.tableView beginUpdates];
        for (NSInteger idx = 0; idx < source.count; idx++)
        {
            [self.tableView moveRowAtIndex:source[idx].integerValue toIndex:target[idx].integerValue];
        }
        [self.tableView endUpdates];
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidChangeItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        NSIndexSet *indexes = note.userInfo[HBQueueItemNotificationIndexesKey];
        NSIndexSet *columnIndexes = [NSIndexSet indexSetWithIndex:0];
        [self.tableView reloadDataForRowIndexes:indexes columnIndexes:columnIndexes];
    }];

    typedef void (^HBUpdateHeight)(NSNotification *note);
    HBUpdateHeight updateHeight = ^void(NSNotification *note) {
        NSIndexSet *indexes = note.userInfo[HBQueueItemNotificationIndexesKey];
        NSIndexSet *columnIndexes = [NSIndexSet indexSetWithIndex:0];
        if (indexes.count)
        {
            [self.tableView reloadDataForRowIndexes:indexes columnIndexes:columnIndexes];
            [self.tableView noteHeightOfRowsWithIndexesChanged:indexes];
        }
    };

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidStartItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:updateHeight];
    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:updateHeight];
}

#pragma mark - UI Actions

/**
 * Delete encodes from the queue window and accompanying array
 * Also handling first cancelling the encode if in fact its currently encoding.
 */
- (IBAction)removeSelectedQueueItem:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    [self.delegate tableViewRemoveItemsAtIndexes:targetedRows];
}

/**
 * Show the finished encode in the finder
 */
- (IBAction)revealSelectedQueueItems:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        NSURL *url = [[self.queue.items objectAtIndex:currentIndex] completeOutputURL];
        [urls addObject:url];
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
}

- (IBAction)revealSelectedQueueItemsSources:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        NSURL *url = [[self.queue.items objectAtIndex:currentIndex] fileURL];
        [urls addObject:url];
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
}

- (IBAction)revealSelectedQueueItemsActivityLogs:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        NSURL *url = [[self.queue.items objectAtIndex:currentIndex] activityLogURL];
        if (url)
        {
            [urls addObject:url];
        }
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
}

/**
 *  Resets the item state to ready.
 */
- (IBAction)resetJobState:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    if (targetedRows.count)
    {
        [self.delegate tableViewResetItemsAtIndexes:targetedRows];
    }
}

/**
 * Send the selected queue item back to the main window for rescan and possible edit.
 */
- (IBAction)editSelectedQueueItem:(id)sender
{
    NSInteger row = self.tableView.clickedRow;
    HBQueueItem *item = [self.queue.items objectAtIndex:row];
    if (item)
    {
        [self.delegate tableViewEditItem:item];
    }
}

- (IBAction)removeAll:(id)sender
{
    [self.queue removeNotWorkingItems];
}

- (IBAction)removeCompleted:(id)sender
{
    [self.queue removeCompletedItems];
}

#pragma mark - UI Validation

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(editSelectedQueueItem:) ||
        action == @selector(removeSelectedQueueItem:) ||
        action == @selector(revealSelectedQueueItems:) ||
        action == @selector(revealSelectedQueueItemsSources:))
    {
        return (self.tableView.selectedRow != -1 || self.tableView.clickedRow != -1);
    }

    if (action == @selector(revealSelectedQueueItemsActivityLogs:))
    {
        return (self.tableView.selectedRow != -1 || self.tableView.clickedRow != -1);
    }

    if (action == @selector(resetJobState:))
    {
        return self.tableView.targetedRowIndexes.count > 0;
    }

    if (action == @selector(removeAll:))
    {
        return self.queue.items.count > 0;
    }

    if (action == @selector(removeCompleted:))
    {
        return self.queue.completedItemsCount > 0;
    }

    return YES;
}

#pragma mark - NSTableView data source

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row {

    HBQueueItem *item = self.queue.items[row];

    HBQueueItemView *view = item.state == HBQueueItemStateWorking && item == self.queue.currentItem ?
                            [tableView makeViewWithIdentifier:@"MainWorkingCell" owner:self] :
                            [tableView makeViewWithIdentifier:@"MainCell" owner:self];

    view.delegate = self;
    view.item = item;

    return view;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.queue.items.count;
}

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
    HBQueueItem *item = self.queue.items[row];
    return item.state == HBQueueItemStateWorking && item == self.queue.currentItem ? 58 : 22;
}

#pragma mark NSQueueItemView delegate

- (void)removeQueueItem:(nonnull HBQueueItem *)item
{
    NSUInteger index = [self.queue.items indexOfObject:item];
    [self.delegate tableViewRemoveItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
}

- (void)revealQueueItem:(nonnull HBQueueItem *)item
{
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[item.completeOutputURL]];
}

#pragma mark NSTableView delegate

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    NSIndexSet *indexes = self.tableView.selectedRowIndexes;
    [self.delegate tableViewDidSelectItemsAtIndexes:indexes];
}

- (void)HB_deleteSelectionFromTableView:(NSTableView *)tableView
{
    [self removeSelectedQueueItem:tableView];
}

- (BOOL)tableView:(NSTableView *)tableView writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard *)pboard
{
    NSArray<HBQueueItem *> *items = [self.queue.items objectsAtIndexes:rowIndexes];
    // Dragging is only allowed of the pending items.
    if (items[0].state != HBQueueItemStateReady)
    {
        return NO;
    }

    self.dragNodesArray = items;

    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:@[HBQueueDragDropPboardType] owner:self];

    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:HBQueueDragDropPboardType];

    return YES;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
    // Don't allow dropping ONTO an item since they can't really contain any children.
    BOOL isOnDropTypeProposal = dropOperation == NSTableViewDropOn;
    if (isOnDropTypeProposal)
    {
        return NSDragOperationNone;
    }

    // We do not let the user drop a pending item before or *above*
    // already finished or currently encoding items.
    NSInteger encodingRow = self.queue.currentItem ? [self.queue.items indexOfObject:self.queue.currentItem] : NSNotFound;
    if (encodingRow != NSNotFound && row <= encodingRow)
    {
        return NSDragOperationNone;
        row = MAX(row, encodingRow);
    }

    return NSDragOperationMove;
}

- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation
{
    [self.queue moveItems:self.dragNodesArray toIndex:row];
    return YES;
}

@end
