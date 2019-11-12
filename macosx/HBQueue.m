/*  HBQueue.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueue.h"
#import "HBRemoteCore.h"

#import "HBPreferencesKeys.h"
#import "NSArray+HBAdditions.h"
#import "HBJobOutputFileWriter.h"

static void *HBQueueContext = &HBQueueContext;
static void *HBQueueLogLevelContext = &HBQueueLogLevelContext;

NSString * const HBQueueDidChangeStateNotification = @"HBQueueDidChangeStateNotification";

NSString * const HBQueueDidAddItemNotification = @"HBQueueDidAddItemNotification";
NSString * const HBQueueDidRemoveItemNotification = @"HBQueueDidRemoveItemNotification";
NSString * const HBQueueDidChangeItemNotification = @"HBQueueDidChangeItemNotification";

NSString * const HBQueueItemNotificationIndexesKey = @"HBQueueReloadItemsNotification";

NSString * const HBQueueDidMoveItemNotification = @"HBQueueDidMoveItemNotification";
NSString * const HBQueueItemNotificationSourceIndexesKey = @"HBQueueItemNotificationSourceIndexesKey";
NSString * const HBQueueItemNotificationTargetIndexesKey = @"HBQueueItemNotificationTargetIndexesKey";

NSString * const HBQueueReloadItemsNotification = @"HBQueueReloadItemsNotification";

NSString * const HBQueueLowSpaceAlertNotification = @"HBQueueLowSpaceAlertNotification";

NSString * const HBQueueProgressNotification = @"HBQueueProgressNotification";
NSString * const HBQueueProgressNotificationPercentKey = @"HBQueueProgressNotificationPercentKey";
NSString * const HBQueueProgressNotificationHoursKey = @"HBQueueProgressNotificationHoursKey";
NSString * const HBQueueProgressNotificationMinutesKey = @"HBQueueProgressNotificationMinutesKey";
NSString * const HBQueueProgressNotificationSecondsKey = @"HBQueueProgressNotificationSecondsKey";
NSString * const HBQueueProgressNotificationInfoKey = @"HBQueueProgressNotificationInfoKey";

NSString * const HBQueueDidStartNotification = @"HBQueueDidStartNotification";
NSString * const HBQueueDidCompleteNotification = @"HBQueueDidCompleteNotification";

NSString * const HBQueueDidStartItemNotification = @"HBQueueDidStartItemNotification";
NSString * const HBQueueDidCompleteItemNotification = @"HBQueueDidCompleteItemNotification";
NSString * const HBQueueItemNotificationItemKey = @"HBQueueItemNotificationItemKey";

@interface HBQueue ()

@property (nonatomic, readonly) HBRemoteCore *core;
@property (nonatomic) BOOL stop;

@property (nonatomic, nullable) HBJobOutputFileWriter *currentLog;

@end

@implementation HBQueue

- (instancetype)initWithURL:(NSURL *)queueURL
{
    self = [super init];
    if (self)
    {
        NSInteger loggingLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];

        // Init a separate instance of libhb for the queue
        _core = [[HBRemoteCore alloc] initWithLogLevel:loggingLevel name:@"QueueCore"];
        _core.automaticallyPreventSleep = NO;

        _items = [[HBDistributedArray alloc] initWithURL:queueURL class:[HBQueueItem class]];
        _undoManager = [[NSUndoManager alloc] init];

        // Set up the observers
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(reloadQueue) name:HBDistributedArrayChanged object:_items];

        [self updateStats];

        // Set up observers
        [self.core addObserver:self forKeyPath:@"state"
                       options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                       context:HBQueueContext];

        [NSUserDefaultsController.sharedUserDefaultsController addObserver:self forKeyPath:@"values.LoggingLevel"
                                                                   options:0 context:HBQueueLogLevelContext];
    }
    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBQueueContext)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeStateNotification object:self];
    }
    else if (context == HBQueueLogLevelContext)
    {
        self.core.logLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc
{
    [self.core removeObserver:self forKeyPath:@"state"];
    [self.core invalidate];
}

#pragma mark - Public methods

- (void)addJob:(HBJob *)item
{
    NSParameterAssert(item);
    [self addJobs:@[item]];
}

- (void)addJobs:(NSArray<HBJob *> *)jobs
{
    NSParameterAssert(jobs);

    NSMutableArray<HBQueueItem *> *itemsToAdd = [NSMutableArray array];
    for (HBJob *job in jobs)
    {
        HBQueueItem *item = [[HBQueueItem alloc] initWithJob:job];
        [itemsToAdd addObject:item];
    }
    if (itemsToAdd.count)
    {
        NSIndexSet *indexes = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(self.items.count, itemsToAdd.count)];
        [self addItems:itemsToAdd atIndexes:indexes];
    }
}

- (BOOL)itemExistAtURL:(NSURL *)url
{
    NSParameterAssert(url);

    for (HBQueueItem *item in self.items)
    {
        if ((item.state == HBQueueItemStateReady || item.state == HBQueueItemStateWorking)
            && [item.completeOutputURL isEqualTo:url])
        {
            return YES;
        }
    }
    return NO;
}

- (void)addItems:(NSArray<HBQueueItem *> *)items atIndexes:(NSIndexSet *)indexes
{
    NSParameterAssert(items);
    NSParameterAssert(indexes);
    [self.items beginTransaction];

    // Forward
    NSUInteger currentIndex = indexes.firstIndex;
    NSUInteger currentObjectIndex = 0;
    while (currentIndex != NSNotFound)
    {
        [self.items insertObject:items[currentObjectIndex] atIndex:currentIndex];
        currentIndex = [indexes indexGreaterThanIndex:currentIndex];
        currentObjectIndex++;
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidAddItemNotification object:self userInfo:@{HBQueueItemNotificationIndexesKey: indexes}];

    NSUndoManager *undo = self.undoManager;
    [[undo prepareWithInvocationTarget:self] removeItemsAtIndexes:indexes];

    if (!undo.isUndoing)
    {
        if (items.count == 1)
        {
            [undo setActionName:NSLocalizedString(@"Add Job To Queue", @"Queue undo action name")];
        }
        else
        {
            [undo setActionName:NSLocalizedString(@"Add Jobs To Queue", @"Queue undo action name")];
        }
    }

    [self updateStats];
    [self.items commit];
}

- (void)removeItemAtIndex:(NSUInteger)index
{
    [self removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
}

- (void)removeItemsAtIndexes:(NSIndexSet *)indexes
{
    NSParameterAssert(indexes);

    if (indexes.count == 0)
    {
        return;
    }

    [self.items beginTransaction];

    NSArray<HBQueueItem *> *removeItems = [self.items objectsAtIndexes:indexes];

    if (self.items.count > indexes.lastIndex)
    {
        [self.items removeObjectsAtIndexes:indexes];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidRemoveItemNotification object:self userInfo:@{HBQueueItemNotificationIndexesKey: indexes}];

    NSUndoManager *undo = self.undoManager;
    [[undo prepareWithInvocationTarget:self] addItems:removeItems atIndexes:indexes];

    if (!undo.isUndoing)
    {
        if (indexes.count == 1)
        {
            [undo setActionName:NSLocalizedString(@"Remove Job From Queue", @"Queue undo action name")];
        }
        else
        {
            [undo setActionName:NSLocalizedString(@"Remove Jobs From Queue", @"Queue undo action name")];
        }
    }

    [self updateStats];
    [self.items commit];
}

- (void)moveItems:(NSArray<HBQueueItem *> *)items toIndex:(NSUInteger)index
{
    [self.items beginTransaction];

    NSMutableArray<NSNumber *> *source = [NSMutableArray array];
    NSMutableArray<NSNumber *> *dest = [NSMutableArray array];

    for (id object in items.reverseObjectEnumerator)
    {
        NSUInteger sourceIndex = [self.items indexOfObject:object];
        [self.items removeObjectAtIndex:sourceIndex];

        if (sourceIndex < index)
        {
            index--;
        }

        [self.items insertObject:object atIndex:index];

        [source addObject:@(index)];
        [dest addObject:@(sourceIndex)];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidMoveItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueItemNotificationSourceIndexesKey: dest,
                                                               HBQueueItemNotificationTargetIndexesKey: source}];

    NSUndoManager *undo = self.undoManager;
    [[undo prepareWithInvocationTarget:self] moveQueueItemsAtIndexes:source toIndexes:dest];

    if (!undo.isUndoing)
    {
        if (items.count == 1)
        {
            [undo setActionName:NSLocalizedString(@"Move Job in Queue", @"Queue undo action name")];
        }
        else
        {
            [undo setActionName:NSLocalizedString(@"Move Jobs in Queue", @"Queue undo action name")];
        }
    }

    [self.items commit];
}

- (void)moveQueueItemsAtIndexes:(NSArray *)source toIndexes:(NSArray *)dest
{
    [self.items beginTransaction];

    NSMutableArray<NSNumber *> *newSource = [NSMutableArray array];
    NSMutableArray<NSNumber *> *newDest = [NSMutableArray array];

    for (NSInteger idx = source.count - 1; idx >= 0; idx--)
    {
        NSUInteger sourceIndex = [source[idx] integerValue];
        NSUInteger destIndex = [dest[idx] integerValue];

        [newSource addObject:@(destIndex)];
        [newDest addObject:@(sourceIndex)];

        id obj = [self.items objectAtIndex:sourceIndex];
        [self.items removeObjectAtIndex:sourceIndex];
        [self.items insertObject:obj atIndex:destIndex];
    }

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidMoveItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueItemNotificationSourceIndexesKey: newDest,
                                                               HBQueueItemNotificationTargetIndexesKey: newSource}];

    NSUndoManager *undo = self.undoManager;
    [[undo prepareWithInvocationTarget:self] moveQueueItemsAtIndexes:newSource toIndexes:newDest];

    if (!undo.isUndoing)
    {
        if (source.count == 1)
        {
            [undo setActionName:NSLocalizedString(@"Move Job in Queue", @"Queue undo action name")];
        }
        else
        {
            [undo setActionName:NSLocalizedString(@"Move Jobs in Queue", @"Queue undo action name")];
        }
    }

    [self.items commit];
}

/**
 * This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as canceled encodes
 */
- (void)removeCompletedAndCancelledItems
{
    [self.items beginTransaction];
    NSIndexSet *indexes = [self.items indexesOfObjectsUsingBlock:^BOOL(HBQueueItem *item) {
        return (item.state == HBQueueItemStateCompleted || item.state == HBQueueItemStateCanceled);
    }];
    [self removeItemsAtIndexes:indexes];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidRemoveItemNotification object:self userInfo:@{@"indexes": indexes}];
    [self.items commit];
}

/**
 * This method will clear the queue of all encodes. effectively creating an empty queue
 */
- (void)removeAllItems
{
    [self.items beginTransaction];
    [self removeItemsAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.items.count)]];
    [self.items commit];
}

- (void)removeNotWorkingItems
{
    [self.items beginTransaction];
    NSIndexSet *indexes = [self.items indexesOfObjectsUsingBlock:^BOOL(HBQueueItem *item) {
        return (item.state != HBQueueItemStateWorking);
    }];
    [self removeItemsAtIndexes:indexes];
    [self.items commit];
}

- (void)removeCompletedItems
{
    [self.items beginTransaction];
    NSIndexSet *indexes = [self.items indexesOfObjectsUsingBlock:^BOOL(HBQueueItem *item) {
        return (item.state == HBQueueItemStateCompleted);
    }];
    [self removeItemsAtIndexes:indexes];
    [self.items commit];
}

- (void)resetItemsAtIndexes:(NSIndexSet *)indexes
{
    if ([self.items beginTransaction] == HBDistributedArrayContentReload)
    {
        // Do not execute the action if the array changed.
        [self.items commit];
        return;
    }

    NSMutableIndexSet *updatedIndexes = [NSMutableIndexSet indexSet];

    NSUInteger currentIndex =  indexes.firstIndex;
    while (currentIndex != NSNotFound) {
        HBQueueItem *item = self.items[currentIndex];

        if (item.state == HBQueueItemStateCanceled || item.state == HBQueueItemStateCompleted || item.state == HBQueueItemStateFailed)
        {
            item.state = HBQueueItemStateReady;
            [updatedIndexes addIndex:currentIndex];
        }
        currentIndex = [indexes indexGreaterThanIndex:currentIndex];
    }

    [self updateStats];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeItemNotification object:self userInfo:@{HBQueueItemNotificationIndexesKey: indexes}];
    [self.items commit];
}

- (void)resetAllItems
{
    [self.items beginTransaction];
    NSIndexSet *indexes = [self.items indexesOfObjectsUsingBlock:^BOOL(HBQueueItem *item) {
        return (item.state != HBQueueItemStateWorking);
    }];
    [self resetItemsAtIndexes:indexes];
    [self.items commit];
}

- (void)resetFailedItems
{
    [self.items beginTransaction];
    NSIndexSet *indexes = [self.items indexesOfObjectsUsingBlock:^BOOL(HBQueueItem *item) {
        return (item.state == HBQueueItemStateFailed);
    }];
    [self resetItemsAtIndexes:indexes];
    [self.items commit];
}

/**
 * This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void)setEncodingJobsAsPending
{
    [self.items beginTransaction];

    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
    NSUInteger idx = 0;
    for (HBQueueItem *item in self.items)
    {
        // We want to keep any queue item that is pending or was previously being encoded
        if (item.state == HBQueueItemStateWorking)
        {
            item.state = HBQueueItemStateReady;
            [indexes addIndex:idx];
        }
        idx++;
    }

    [self updateStats];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeItemNotification object:self userInfo:@{HBQueueItemNotificationIndexesKey: indexes}];
    [self.items commit];
}

- (BOOL)canEncode
{
    return self.pendingItemsCount > 0 && ![self isEncoding];
}

- (BOOL)isEncoding
{
    HBState s = self.core.state;
    return self.currentItem || (s == HBStateScanning) || (s == HBStatePaused) || (s == HBStateWorking) || (s == HBStateMuxing) || (s == HBStateSearching);
}

- (BOOL)canPause
{
    HBState s = self.core.state;
    return (s == HBStateWorking || s == HBStateMuxing);
}

- (void)pause
{
    [self.currentItem pausedAtDate:[NSDate date]];
    [self.core pause];
    [self.core allowSleep];
}

- (BOOL)canResume
{
    return self.core.state == HBStatePaused;
}

- (void)resume
{
    [self.currentItem resumedAtDate:[NSDate date]];
    [self.core resume];
    [self.core preventSleep];
}

#pragma mark - Private queue editing methods

/**
 *  Reloads the queue, this is called
 *  when another HandBrake instance modifies the queue
 */
- (void)reloadQueue
{
    [self updateStats];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueReloadItemsNotification object:self];
}

- (void)updateStats
{
    NSUInteger pendingCount = 0, failedCount = 0, completedCount = 0;

    for (HBQueueItem *item in self.items)
    {
        if (item.state == HBQueueItemStateReady)
        {
            pendingCount++;
        }
        else if (item.state == HBQueueItemStateCompleted)
        {
            completedCount++;
        }
        else if (item.state == HBQueueItemStateFailed)
        {
            failedCount++;
        }
    }

    self.pendingItemsCount = pendingCount;
    self.failedItemsCount = failedCount;
    self.completedItemsCount = completedCount;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeStateNotification object:self];
}

- (BOOL)isDiskSpaceLowAtURL:(NSURL *)url
{
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBQueuePauseIfLowSpace])
    {
        NSURL *volumeURL = nil;
        NSDictionary<NSURLResourceKey, id> *attrs = [url resourceValuesForKeys:@[NSURLIsVolumeKey, NSURLVolumeURLKey] error:NULL];
        long long minCapacity = [[NSUserDefaults.standardUserDefaults stringForKey:HBQueueMinFreeSpace] longLongValue] * 1000000000;

        volumeURL = [attrs[NSURLIsVolumeKey] boolValue] ? url : attrs[NSURLVolumeURLKey];

        if (volumeURL)
        {
            [volumeURL removeCachedResourceValueForKey:NSURLVolumeAvailableCapacityKey];
            attrs = [volumeURL resourceValuesForKeys:@[NSURLVolumeAvailableCapacityKey] error:NULL];

            if (attrs[NSURLVolumeAvailableCapacityKey])
            {
                if ([attrs[NSURLVolumeAvailableCapacityKey] longLongValue] < minCapacity)
                {
                    return YES;
                }
            }
        }
    }

    return NO;
}

/**
 * Used to get the next pending queue item and return it if found
 */
- (HBQueueItem *)getNextPendingQueueItem
{
    for (HBQueueItem *item in self.items)
    {
        if (item.state == HBQueueItemStateReady)
        {
            return item;
        }
    }
    return nil;
}

- (void)start
{
    if (self.canEncode)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidStartNotification object:self];
        [self.core preventSleep];
        [self encodeNextQueueItem];
    }
}

/**
 *  Starts the queue
 */
- (void)encodeNextQueueItem
{
    [self.items beginTransaction];

    // since we have completed an encode, we go to the next
    if (self.stop)
    {
        [HBUtilities writeToActivityLog:"Queue manually stopped"];

        self.stop = NO;
        [self.core allowSleep];

        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
    }
    else
    {
        // Check to see if there are any more pending items in the queue
        HBQueueItem *nextItem = [self getNextPendingQueueItem];

        if (nextItem && [self isDiskSpaceLowAtURL:nextItem.outputURL])
        {
            // Disk space is low, show an alert
            [HBUtilities writeToActivityLog:"Queue Stopped, low space on destination disk"];
            [self.core allowSleep];

            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueLowSpaceAlertNotification object:self];
        }
        // If we still have more pending items in our queue, lets go to the next one
        else if (nextItem)
        {
            // now we mark the queue item as working so another instance can not come along and try to scan it while we are scanning
            nextItem.startedDate = [NSDate date];
            nextItem.state = HBQueueItemStateWorking;

            // Tell HB to output a new activity log file for this encode
            self.currentLog = [[HBJobOutputFileWriter alloc] initWithJob:nextItem.job];
            if (self.currentLog)
            {
                nextItem.activityLogURL = self.currentLog.url;

                dispatch_queue_t mainQueue = dispatch_get_main_queue();
                [self.core.stderrRedirect addListener:self.currentLog queue:mainQueue];
                [self.core.stdoutRedirect addListener:self.currentLog queue:mainQueue];
            }

            self.currentItem = nextItem;
            NSIndexSet *indexes = [NSIndexSet indexSetWithIndex:[self.items indexOfObject:nextItem]];

            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidStartItemNotification object:self userInfo:@{HBQueueItemNotificationItemKey: nextItem,
                                                                                                                            HBQueueItemNotificationIndexesKey: indexes}];

            // now we can go ahead and scan the new pending queue item
            [self encodeItem:nextItem];

            // erase undo manager history
            [self.undoManager removeAllActions];
        }
        else
        {
            [HBUtilities writeToActivityLog:"Queue Done, there are no more pending encodes"];
            [self.core allowSleep];

            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
        }
    }

    [self updateStats];

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeStateNotification object:self];

    [self.items commit];
}

- (void)completedItem:(HBQueueItem *)item result:(HBCoreResult)result
{
    NSParameterAssert(item);
    [self.items beginTransaction];

    item.endedDate = [NSDate date];

    // Since we are done with this encode, tell output to stop writing to the
    // individual encode log.
    [self.core.stderrRedirect removeListener:self.currentLog];
    [self.core.stdoutRedirect removeListener:self.currentLog];

    self.currentLog = nil;

    // Mark the encode just finished
    switch (result) {
        case HBCoreResultDone:
            item.state = HBQueueItemStateCompleted;
            break;
        case HBCoreResultCanceled:
            item.state = HBQueueItemStateCanceled;
            break;
        default:
            item.state = HBQueueItemStateFailed;
            break;
    }

    // Update UI
    NSString *info = nil;
    switch (result) {
        case HBCoreResultDone:
            info = NSLocalizedString(@"Encode Finished.", @"Queue status");
            break;
        case HBCoreResultCanceled:
            info = NSLocalizedString(@"Encode Canceled.", @"Queue status");
            break;
        default:
            info = NSLocalizedString(@"Encode Failed.", @"Queue status");
            break;
    }

    self.currentItem = nil;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueProgressNotification object:self userInfo:@{HBQueueProgressNotificationPercentKey: @1.0,
                                                                                                                HBQueueProgressNotificationInfoKey: info}];

    NSUInteger index = [self.items indexOfObject:item];
    NSIndexSet *indexes = index != NSNotFound ? [NSIndexSet indexSetWithIndex:index] : [NSIndexSet indexSet];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteItemNotification object:self userInfo:@{HBQueueItemNotificationItemKey: item,
                                                                                                                       HBQueueItemNotificationIndexesKey: indexes}];

    [self.items commit];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)encodeItem:(HBQueueItem *)item
{
    NSParameterAssert(item);

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueProgressNotification object:self userInfo:@{HBQueueProgressNotificationPercentKey: @0,
                                                                                                                    HBQueueProgressNotificationInfoKey: info}];
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        if (result == HBCoreResultDone)
        {
            [self realEncodeItem:item];
        }
        else
        {
            [self completedItem:item result:result];
            [self encodeNextQueueItem];
        }
    };

    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    [self.core scanURL:item.fileURL
            titleIndex:item.job.titleIdx
              previews:10
           minDuration:0
          keepPreviews:NO
       progressHandler:progressHandler
     completionHandler:completionHandler];
}

/**
 * This assumes that we have re-scanned and loaded up a new queue item to send to libhb
 */
- (void)realEncodeItem:(HBQueueItem *)item
{
    NSParameterAssert(item);

    HBJob *job = item.job;

    NSParameterAssert(job);

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        if (state == HBStateMuxing)
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueProgressNotificationPercentKey: @1,
                                                                       HBQueueProgressNotificationInfoKey: info}];
        }
        else
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueProgressNotificationPercentKey: @(progress.percent),
                                                                       HBQueueProgressNotificationHoursKey: @(progress.hours),
                                                                       HBQueueProgressNotificationMinutesKey: @(progress.minutes),
                                                                       HBQueueProgressNotificationSecondsKey: @(progress.seconds),
                                                                       HBQueueProgressNotificationInfoKey: info}];
        }
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        [self completedItem:item result:result];

        if ([NSUserDefaults.standardUserDefaults boolForKey:HBQueueAutoClearCompletedItems])
        {
            [self removeCompletedItems];
        }

        [self encodeNextQueueItem];
    };

    // We should be all setup so let 'er rip
    [self.core encodeJob:job progressHandler:progressHandler completionHandler:completionHandler];
}

/**
 * Cancels the current job
 */
- (void)doCancelCurrentItem
{
    if (self.core.state == HBStateScanning)
    {
        [self.core cancelScan];
    }
    else
    {
        [self.core cancelEncode];
    }
}

/**
 * Cancels the current job and starts processing the next in queue.
 */
- (void)cancelCurrentItemAndContinue
{
    [self doCancelCurrentItem];
}

/**
 * Cancels the current job and stops libhb from processing the remaining encodes.
 */
- (void)cancelCurrentItemAndStop
{
    if (self.core.state != HBStateIdle)
    {
        self.stop = YES;
        [self doCancelCurrentItem];
    }
}

/**
 * Finishes the current job and stops libhb from processing the remaining encodes.
 */
- (void)finishCurrentAndStop
{
    if (self.core.state != HBStateIdle)
    {
        self.stop = YES;
    }
}

@end
