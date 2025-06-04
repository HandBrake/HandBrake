/*  HBQueueTableViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueTableViewController.h"

#import "HBQueue.h"
#import "HBTableView.h"
#import "HBQueueItemView.h"
#import "HBQueueItemWorkingView.h"
#import "NSArray+HBAdditions.h"
#import "HBPasteboardItem.h"

@import QuickLookUI;

@interface HBQueueTableViewController () <NSMenuItemValidation, NSTableViewDataSource, NSTableViewDelegate, HBQueueItemViewDelegate, QLPreviewPanelDataSource, QLPreviewPanelDelegate>

@property (nonatomic, weak, readonly) HBQueue *queue;
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
    [self.tableView registerForDraggedTypes:@[tableViewIndex]];
    [self.tableView setVerticalMotionCanBeginDrag:YES];

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
        HBQueueJobItem *item = note.userInfo[HBQueueItemNotificationItemKey];
        NSUInteger index = [self.queue.items indexOfObject:item];
        if (index != NSNotFound)
        {
            NSIndexSet *indexes = [NSIndexSet indexSetWithIndex:index];
            NSIndexSet *columnIndexes = [NSIndexSet indexSetWithIndex:0];
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

- (IBAction)moveSelectedQueueItemsToTop:(id)sender
{
    NSIndexSet *indexes = self.tableView.targetedRowIndexes;
    NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];

    NSUInteger index = 0;
    for (id<HBQueueItem> item in self.queue.items)
    {
        if (item.state == HBQueueItemStateReady)
        {
            break;
        }
        index += 1;
    }

    [self.queue moveItems:items toIndex:index];
}

- (IBAction)moveSelectedQueueItemsToBottom:(id)sender
{
    NSIndexSet *indexes = self.tableView.targetedRowIndexes;
    NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];
    [self.queue moveItems:items toIndex:self.queue.items.count];
}

/**
 * Show the finished encode in the finder
 */
- (IBAction)revealDestinationItemsInFinder:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        id<HBQueueItem> item = [self.queue.items objectAtIndex:currentIndex];
        if ([item isKindOfClass:[HBQueueJobItem class]])
        {
            NSURL *url = item.destinationURL;
            [urls addObject:url];
        }
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    if (urls.count)
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
    }
}

- (IBAction)revealSourceItemsInFinder:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        id<HBQueueItem> item = [self.queue.items objectAtIndex:currentIndex];
        if ([item isKindOfClass:[HBQueueJobItem class]])
        {
            NSURL *url = [(HBQueueJobItem *)item fileURL];
            [urls addObject:url];
        }
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    if (urls.count)
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
    }
}

- (IBAction)revealSelectedQueueItemsActivityLogs:(id)sender
{
    NSIndexSet *targetedRows = self.tableView.targetedRowIndexes;
    NSMutableArray<NSURL *> *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = targetedRows.firstIndex;
    while (currentIndex != NSNotFound) {
        id<HBQueueItem> item = [self.queue.items objectAtIndex:currentIndex];
        if ([item isKindOfClass:[HBQueueJobItem class]])
        {
            NSURL *url = [(HBQueueJobItem *)item activityLogURL];
            if (url)
            {
                [urls addObject:url];
            }
        }
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    if (urls.count)
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
    }
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
    HBQueueJobItem *item = [self.queue.items objectAtIndex:row];
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

    if (action == @selector(editSelectedQueueItem:)) {
        NSIndexSet *indexes = self.tableView.targetedRowIndexes;
        return indexes.count == 1 && self.queue.items[indexes.firstIndex].hasFileRepresentation;
    }

    if (action == @selector(removeSelectedQueueItem:) ||
        action == @selector(moveSelectedQueueItemsToBottom:) ||
        action == @selector(moveSelectedQueueItemsToTop:))
    {
        return self.tableView.targetedRowIndexes.count > 0;
    }

    if (action == @selector(revealSourceItemsInFinder:))
    {
        NSIndexSet *indexes = self.tableView.targetedRowIndexes;
        if (indexes.count == 0) { return NO; }
        NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];

        return [items HB_containsWhere:^BOOL(id<HBQueueItem> _Nonnull object) {
            return object.hasFileRepresentation;
        }];
    }

    if (action == @selector(revealSelectedQueueItemsActivityLogs:) ||
        action == @selector(revealDestinationItemsInFinder:))
    {
        NSIndexSet *indexes = self.tableView.targetedRowIndexes;
        if (indexes.count == 0) { return NO; }
        NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];

        return [items HB_containsWhere:^BOOL(id<HBQueueItem> _Nonnull object) {
            return object.hasFileRepresentation &&
                (object.state == HBQueueItemStateWorking || object.state == HBQueueItemStateFailed ||
                 object.state == HBQueueItemStateCanceled || object.state == HBQueueItemStateCompleted);
        }];
    }

    if (action == @selector(resetJobState:))
    {
        NSIndexSet *indexes = self.tableView.targetedRowIndexes;
        if (indexes.count == 0) { return NO; }
        NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];

        return [items HB_containsWhere:^BOOL(id<HBQueueItem> _Nonnull object) {
            return object.hasFileRepresentation &&
                (object.state == HBQueueItemStateFailed || object.state == HBQueueItemStateCanceled ||
                 object.state == HBQueueItemStateCompleted);
        }];
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
                  row:(NSInteger)row
{

    id<HBQueueItem> item = self.queue.items[row];
    HBQueueItemView *view = nil;

    if (item.state == HBQueueItemStateWorking)
    {
        HBQueueItemWorkingView *workingView = [tableView makeViewWithIdentifier:@"MainWorkingCell" owner:self];
        HBQueueWorker *worker = [self.queue workerForItem:item];
        workingView.item = item;
        workingView.worker = worker;
        view = workingView;
    }
    else
    {
        view = [tableView makeViewWithIdentifier:@"MainCell" owner:self];
        view.item = item;
    }

    view.delegate = self;

    return view;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.queue.items.count;
}

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
    id<HBQueueItem> item = self.queue.items[row];
    return item.state == HBQueueItemStateWorking ? 58 : 22;
}

#pragma mark NSQueueItemView delegate

- (void)removeQueueItem:(nonnull id<HBQueueItem>)item
{
    NSUInteger index = [self.queue.items indexOfObject:item];
    [self.delegate tableViewRemoveItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
}

- (void)revealQueueItem:(nonnull id<HBQueueItem>)item
{
    if ([item isKindOfClass:[HBQueueJobItem class]])
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[[(HBQueueJobItem *)item destinationURL]]];
    }
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

- (nullable id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    return [[HBPasteboardItem alloc] initWithIndex:row];
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
    NSDragOperation dragOp = NSDragOperationNone;

    // if drag source is our own table view, it's a move or a copy
    if (info.draggingSource == tableView)
    {
        // At a minimum, allow move
        dragOp = NSDragOperationMove;
    }

    [tableView setDropRow:row dropOperation:NSTableViewDropAbove];

    return dragOp;
}

- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation
{
    NSMutableIndexSet *indexes = [[NSMutableIndexSet alloc] init];
    for (NSPasteboardItem *item in info.draggingPasteboard.pasteboardItems)
    {
        NSNumber *index = [item propertyListForType:tableViewIndex];
        [indexes addIndex:index.integerValue];
    }

    NSArray *items = [self.queue.items objectsAtIndexes:indexes];
    [self.queue moveItems:items toIndex:row];

    return YES;
}

#pragma mark - QuickLook

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel *)panel
{
    return YES;
}

- (void)beginPreviewPanelControl:(QLPreviewPanel *)panel
{
    QLPreviewPanel.sharedPreviewPanel.delegate = self;
    QLPreviewPanel.sharedPreviewPanel.dataSource = self;
}

- (void)endPreviewPanelControl:(QLPreviewPanel *)panel
{
    QLPreviewPanel.sharedPreviewPanel.delegate = nil;
    QLPreviewPanel.sharedPreviewPanel.dataSource = nil;
}

- (NSInteger)numberOfPreviewItemsInPreviewPanel:(QLPreviewPanel *)panel
{
    return self.tableView.selectedRowIndexes.count;
}

- (id<QLPreviewItem>)previewPanel:(QLPreviewPanel *)panel previewItemAtIndex:(NSInteger)index
{
    NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:self.tableView.selectedRowIndexes];
    if (items.count > index)
    {
        return items[index].destinationURL;
    }
    else
    {
        return nil;
    }
}

- (BOOL)previewPanel:(QLPreviewPanel *)panel handleEvent:(NSEvent *)event
{
    if (event.type == NSEventTypeKeyDown)
    {
        [self.tableView keyDown:event];
        [QLPreviewPanel.sharedPreviewPanel reloadData];
        return YES;
    }
    return NO;
}

- (void)keyDown:(NSEvent *)event
{
    NSString *characters = event.charactersIgnoringModifiers;
    if (characters.length)
    {
        unichar key = [characters characterAtIndex:0];
        if (key == ' ')
        {
            if (QLPreviewPanel.sharedPreviewPanel.isVisible)
            {
                [QLPreviewPanel.sharedPreviewPanel orderOut:self];
            }
            else
            {
                [QLPreviewPanel.sharedPreviewPanel makeKeyAndOrderFront:self];
            }
            return;
        }
    }

    [super keyDown:event];
}

@end
