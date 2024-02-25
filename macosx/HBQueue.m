/*  HBQueue.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueue.h"
#import "HBRemoteCore.h"
#import "HBQueueWorker.h"

#import "HBPreferencesKeys.h"
#import "NSArray+HBAdditions.h"

#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/ps/IOPowerSources.h>

static void *HBQueueContext = &HBQueueContext;

NSString * const HBQueueDidChangeStateNotification = @"HBQueueDidChangeStateNotification";

NSString * const HBQueueDidAddItemNotification = @"HBQueueDidAddItemNotification";
NSString * const HBQueueDidRemoveItemNotification = @"HBQueueDidRemoveItemNotification";
NSString * const HBQueueDidChangeItemNotification = @"HBQueueDidChangeItemNotification";

NSString * const HBQueueItemNotificationIndexesKey = @"HBQueueItemNotificationIndexesKey";

NSString * const HBQueueDidMoveItemNotification = @"HBQueueDidMoveItemNotification";
NSString * const HBQueueItemNotificationSourceIndexesKey = @"HBQueueItemNotificationSourceIndexesKey";
NSString * const HBQueueItemNotificationTargetIndexesKey = @"HBQueueItemNotificationTargetIndexesKey";

NSString * const HBQueueLowSpaceAlertNotification = @"HBQueueLowSpaceAlertNotification";

NSString * const HBQueueDidStartNotification = @"HBQueueDidStartNotification";
NSString * const HBQueueDidCompleteNotification = @"HBQueueDidCompleteNotification";

NSString * const HBQueueDidStartItemNotification = @"HBQueueDidStartItemNotification";
NSString * const HBQueueDidCompleteItemNotification = @"HBQueueDidCompleteItemNotification";
NSString * const HBQueueItemNotificationItemKey = @"HBQueueItemNotificationItemKey";

@interface HBQueue ()

@property (nonatomic, readonly) NSURL *fileURL;

@property (nonatomic, readonly) NSMutableArray<id<HBQueueItem>> *itemsInternal;
@property (nonatomic, readonly) NSArray<HBQueueWorker *> *workers;

@property (nonatomic) IOPMAssertionID assertionID;
@property (nonatomic) CFRunLoopSourceRef sourceRunLoop;
@property (nonatomic) BOOL pausedOnBatteryPower;

@property (nonatomic) NSUInteger pendingItemsCount;
@property (nonatomic) NSUInteger failedItemsCount;
@property (nonatomic) NSUInteger completedItemsCount;
@property (nonatomic) NSUInteger workingItemsCount;

@end

@implementation HBQueue

- (void)setUpWorkers
{
    NSArray<NSString *> *xpcServiceNames = @[@"fr.handbrake.HandBrakeXPCService", @"fr.handbrake.HandBrakeXPCService2",
                                             @"fr.handbrake.HandBrakeXPCService3", @"fr.handbrake.HandBrakeXPCService4"];

    NSMutableArray<HBQueueWorker *> *workers = [[NSMutableArray alloc] init];
    for (NSString *xpcServiceName in xpcServiceNames)
    {
        HBQueueWorker *worker = [[HBQueueWorker alloc] initWithXPCServiceName:xpcServiceName];
        [workers addObject:worker];
    }
    _workers = [workers copy];
}

- (void)setUpObservers
{
    for (HBQueueWorker *worker in _workers)
    {
        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueWorkerDidChangeStateNotification
                                                        object:worker
                                                         queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeStateNotification object:self];
        }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueWorkerDidStartItemNotification
                                                        object:worker
                                                         queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) {
            [self updateStats];
            HBQueueJobItem *item = note.userInfo[HBQueueWorkerItemNotificationItemKey];
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidStartItemNotification object:self userInfo:@{HBQueueItemNotificationItemKey: item}];
        }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueWorkerDidCompleteItemNotification
                                                        object:worker
                                                         queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) {
            [self updateStats];
            HBQueueJobItem *item = note.userInfo[HBQueueWorkerItemNotificationItemKey];
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteItemNotification object:self userInfo:@{HBQueueItemNotificationItemKey: item}];
            [self completedItem:item];
        }];
    }

    [NSUserDefaultsController.sharedUserDefaultsController addObserver:self forKeyPath:@"values.HBQueueWorkerCounts"
                                                                options:0 context:HBQueueContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBQueueContext)
    {
        if (self.isEncoding)
        {
            [self encodeNextQueueItem];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

static void powerSourceCallback(void *context)
{
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBQueuePauseOnBatteryPower])
    {
        CFTypeRef sourceInfo =  IOPSCopyPowerSourcesInfo();
        if (sourceInfo)
        {
            HBQueue *queue = (__bridge HBQueue *)context;
            CFStringRef powerSourceType = IOPSGetProvidingPowerSourceType(sourceInfo);
            if ((CFStringCompare(powerSourceType, CFSTR(kIOPMBatteryPowerKey), 0) == kCFCompareEqualTo ||
                 CFStringCompare(powerSourceType, CFSTR(kIOPMUPSPowerKey), 0) == kCFCompareEqualTo) &&
                queue.canPause)
            {
                [queue pause];
                queue.pausedOnBatteryPower = YES;
            }
            else if (CFStringCompare(powerSourceType, CFSTR(kIOPMACPowerKey), 0) == kCFCompareEqualTo &&
                     queue.pausedOnBatteryPower)
            {
                [queue resume];
            }
            CFRelease(sourceInfo);
        }
    }
}

- (void)setUpIOPSNotificationRunLoop
{
    self.sourceRunLoop = IOPSNotificationCreateRunLoopSource(powerSourceCallback, (__bridge void *)(self));
    if (self.sourceRunLoop)
    {
        CFRunLoopAddSource(CFRunLoopGetCurrent(), self.sourceRunLoop, kCFRunLoopDefaultMode);
    }
}

- (instancetype)initWithURL:(NSURL *)fileURL
{
    self = [super init];
    if (self)
    {
        _fileURL = fileURL;
        _itemsInternal = [self load];
        _undoManager = [[NSUndoManager alloc] init];
        _assertionID = -1;

        [self setEncodingJobsAsPending];

        [self setUpWorkers];
        [self setUpObservers];
        [self setUpIOPSNotificationRunLoop];
    }
    return self;
}

- (void)dealloc
{
    if (self.sourceRunLoop)
    {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self.sourceRunLoop, kCFRunLoopDefaultMode);
    }
}

#pragma mark - Load and save

- (NSMutableArray *)load
{
    NSError *error;

    NSData *queue = [NSData dataWithContentsOfURL:self.fileURL];
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:queue];
    unarchiver.requiresSecureCoding = YES;

    NSSet *objectClasses = [NSSet setWithObjects:[NSMutableArray class], [HBQueueJobItem class], [HBQueueActionStopItem class], nil];

    NSArray *loadedItems = [unarchiver decodeTopLevelObjectOfClasses:objectClasses
                                                              forKey:NSKeyedArchiveRootObjectKey
                                                               error:&error];

    if (error)
    {
        [HBUtilities writeErrorToActivityLog:error];
    }

    [unarchiver finishDecoding];

    return loadedItems ? [loadedItems mutableCopy] : [NSMutableArray array];
}

- (void)save
{
    if (![NSKeyedArchiver archiveRootObject:self.itemsInternal toFile:self.fileURL.path])
    {
        [HBUtilities writeToActivityLog:"Failed to write the queue to disk"];
    }
}

#pragma mark - Public methods

- (NSArray<id<HBQueueItem>> *)items
{
    return self.itemsInternal;
}

- (NSUInteger)workersCount
{
    NSUInteger count = [NSUserDefaults.standardUserDefaults integerForKey:HBQueueWorkerCounts];
    return count > 0 && count <= 4 ? count : 1;
}

- (void)addJob:(HBJob *)item
{
    NSParameterAssert(item);
    [self addJobs:@[item]];
}

- (void)addJobs:(NSArray<HBJob *> *)jobs
{
    NSParameterAssert(jobs);

    NSMutableArray<HBQueueJobItem *> *itemsToAdd = [NSMutableArray array];
    for (HBJob *job in jobs)
    {
        HBQueueJobItem *item = [[HBQueueJobItem alloc] initWithJob:job];
        [itemsToAdd addObject:item];
    }
    if (itemsToAdd.count)
    {
        NSIndexSet *indexes = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(self.itemsInternal.count, itemsToAdd.count)];
        [self addItems:itemsToAdd atIndexes:indexes];

        if (self.isEncoding && self.countOfEncodings < self.workersCount)
        {
            [self encodeNextQueueItem];
        }
    }
}

- (BOOL)itemExistAtURL:(NSURL *)url
{
    NSParameterAssert(url);

    for (HBQueueJobItem *item in self.itemsInternal)
    {
        if ([item isKindOfClass:[HBQueueJobItem class]] && 
            (item.state == HBQueueItemStateReady || item.state == HBQueueItemStateWorking)
            && [item.destinationURL isEqualTo:url])
        {
            return YES;
        }
    }
    return NO;
}

- (void)addItems:(NSArray<id<HBQueueItem>> *)items atIndexes:(NSIndexSet *)indexes
{
    NSParameterAssert(items);
    NSParameterAssert(indexes);

    // Forward
    NSUInteger currentIndex = indexes.firstIndex;
    NSUInteger currentObjectIndex = 0;
    while (currentIndex != NSNotFound)
    {
        [self.itemsInternal insertObject:items[currentObjectIndex] atIndex:currentIndex];
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
    [self save];
}

- (void)prepareItemForEditingAtIndex:(NSUInteger)index
{
    HBQueueJobItem *item = self.itemsInternal[index];
    NSIndexSet *indexes = [NSIndexSet indexSetWithIndex:index];

    if (item.state == HBQueueItemStateWorking)
    {
        [self cancelItemsAtIndexes:indexes];
    }

    item.state = HBQueueItemStateRescanning;
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueItemNotificationIndexesKey: [NSIndexSet indexSetWithIndex:index]}];
    [self updateStats];
}

- (void)removeItemsAtIndexes:(NSIndexSet *)indexes
{
    NSParameterAssert(indexes);

    if (indexes.count == 0)
    {
        return;
    }

    NSArray<id<HBQueueItem>> *removeItems = [self.itemsInternal objectsAtIndexes:indexes];

    if (self.itemsInternal.count > indexes.lastIndex)
    {
        [self.itemsInternal removeObjectsAtIndexes:indexes];
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
    [self save];
}

- (void)moveItems:(NSArray<id<HBQueueItem>> *)items toIndex:(NSUInteger)index
{
    NSMutableArray<NSNumber *> *source = [NSMutableArray array];
    NSMutableArray<NSNumber *> *dest = [NSMutableArray array];

    for (id object in items.reverseObjectEnumerator)
    {
        NSUInteger sourceIndex = [self.itemsInternal indexOfObject:object];
        [self.itemsInternal removeObjectAtIndex:sourceIndex];

        if (sourceIndex < index)
        {
            index--;
        }

        [self.itemsInternal insertObject:object atIndex:index];

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

    [self save];
}

- (void)moveQueueItemsAtIndexes:(NSArray *)source toIndexes:(NSArray *)dest
{
    NSMutableArray<NSNumber *> *newSource = [NSMutableArray array];
    NSMutableArray<NSNumber *> *newDest = [NSMutableArray array];

    for (NSInteger idx = source.count - 1; idx >= 0; idx--)
    {
        NSUInteger sourceIndex = [source[idx] integerValue];
        NSUInteger destIndex = [dest[idx] integerValue];

        [newSource addObject:@(destIndex)];
        [newDest addObject:@(sourceIndex)];

        id obj = [self.itemsInternal objectAtIndex:sourceIndex];
        [self.itemsInternal removeObjectAtIndex:sourceIndex];
        [self.itemsInternal insertObject:obj atIndex:destIndex];
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

    [self save];
}

/**
 * This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as canceled encodes
 */
- (void)removeCompletedAndCancelledItems
{
    NSIndexSet *indexes = [self.itemsInternal HB_indexesOfObjectsUsingBlock:^BOOL(id<HBQueueItem> item) {
        return (item.state == HBQueueItemStateCompleted || item.state == HBQueueItemStateCanceled);
    }];
    [self removeItemsAtIndexes:indexes];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidRemoveItemNotification object:self userInfo:@{@"indexes": indexes}];
    [self save];
}

/**
 * This method will clear the queue of all encodes. effectively creating an empty queue
 */
- (void)removeAllItems
{
    [self removeItemsAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.itemsInternal.count)]];
    [self save];
}

- (void)removeNotWorkingItems
{
    NSIndexSet *indexes = [self.itemsInternal HB_indexesOfObjectsUsingBlock:^BOOL(id<HBQueueItem> item) {
        return (item.state != HBQueueItemStateWorking && item.state != HBQueueItemStateRescanning);
    }];
    [self removeItemsAtIndexes:indexes];
}

- (void)removeCompletedItems
{
    NSIndexSet *indexes = [self.itemsInternal HB_indexesOfObjectsUsingBlock:^BOOL(id<HBQueueItem> item) {
        return (item.state == HBQueueItemStateCompleted);
    }];
    [self removeItemsAtIndexes:indexes];
}

- (void)resetItemsAtIndexes:(NSIndexSet *)indexes
{
    NSMutableIndexSet *updatedIndexes = [NSMutableIndexSet indexSet];

    NSUInteger currentIndex =  indexes.firstIndex;
    while (currentIndex != NSNotFound) {
        id<HBQueueItem> item = self.itemsInternal[currentIndex];

        if (item.state == HBQueueItemStateCanceled || item.state == HBQueueItemStateCompleted ||
            item.state == HBQueueItemStateFailed || item.state == HBQueueItemStateRescanning)
        {
            item.state = HBQueueItemStateReady;
            [updatedIndexes addIndex:currentIndex];
        }
        currentIndex = [indexes indexGreaterThanIndex:currentIndex];
    }

    [self updateStats];
    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidChangeItemNotification object:self userInfo:@{HBQueueItemNotificationIndexesKey: indexes}];
    [self save];
}

- (void)resetAllItems
{
    NSIndexSet *indexes = [self.itemsInternal HB_indexesOfObjectsUsingBlock:^BOOL(id<HBQueueItem> item) {
        return (item.state != HBQueueItemStateWorking && item.state != HBQueueItemStateRescanning);
    }];
    [self resetItemsAtIndexes:indexes];
}

- (void)resetFailedItems
{
    NSIndexSet *indexes = [self.itemsInternal HB_indexesOfObjectsUsingBlock:^BOOL(id<HBQueueItem> item) {
        return (item.state == HBQueueItemStateFailed);
    }];
    [self resetItemsAtIndexes:indexes];
}

/**
 * This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void)setEncodingJobsAsPending
{
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
    NSUInteger idx = 0;
    for (id<HBQueueItem> item in self.itemsInternal)
    {
        // We want to keep any queue item that is pending or was previously being encoded
        if (item.state == HBQueueItemStateWorking || item.state == HBQueueItemStateRescanning)
        {
            item.state = HBQueueItemStateReady;
            [indexes addIndex:idx];
        }
        idx++;
    }

    [self updateStats];
    [self save];
}

- (BOOL)canEncode
{
    return self.pendingItemsCount > 0 && ![self isEncoding];
}

- (BOOL)isEncoding
{
    BOOL isEncoding = NO;
    for (HBQueueWorker *worker in self.workers)
    {
        isEncoding |= worker.isEncoding;
    }

    return isEncoding;
}

- (NSUInteger)countOfEncodings
{
    NSUInteger count = 0;
    for (HBQueueWorker *worker in self.workers)
    {
        count += worker.isEncoding ? 1 : 0;
    }

    return count;
}

- (BOOL)canPause
{
    BOOL canPause = NO;
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.isEncoding)
        {
            canPause |= worker.canPause;
        }
    }

    return canPause;
}

- (void)pause
{
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.canPause)
        {
            [worker pause];
        }
    }
    [self allowSleep];
}

- (BOOL)canResume
{
    BOOL canResume = NO;
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.isEncoding)
        {
            canResume |= worker.canResume;
        }
    }

    return canResume;
}

- (void)resume
{
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.canResume)
        {
            [worker resume];
        }
    }

    self.pausedOnBatteryPower = NO;
    [self preventSleep];
}

#pragma mark - Sleep

- (void)preventSleep
{
    if (_assertionID != -1)
    {
        // nothing to do
        return;
    }

    CFStringRef reasonForActivity= CFSTR("HandBrake is currently scanning and/or encoding");

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleSystemSleep,
                                                   kIOPMAssertionLevelOn, reasonForActivity, &_assertionID);

    if (success != kIOReturnSuccess)
    {
        [HBUtilities writeToActivityLog:"HBRemoteCore: failed to prevent system sleep"];
    }
}

- (void)allowSleep
{
    if (_assertionID == -1)
    {
        // nothing to do
        return;
    }

    IOReturn success = IOPMAssertionRelease(_assertionID);

    if (success == kIOReturnSuccess)
    {
        _assertionID = -1;
    }
}

#pragma mark - Private queue editing methods

- (void)updateStats
{
    NSUInteger pendingCount = 0, failedCount = 0, completedCount = 0, workingCount = 0;

    for (HBQueueJobItem *item in self.itemsInternal)
    {
        if ([item isKindOfClass:[HBQueueJobItem class]])
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
            else if (item.state == HBQueueItemStateWorking)
            {
                workingCount++;
            }
        }
    }

    self.pendingItemsCount = pendingCount;
    self.failedItemsCount = failedCount;
    self.completedItemsCount = completedCount;
    self.workingItemsCount = workingCount;

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
- (id<HBQueueItem>)nextPendingQueueItem
{
    for (id<HBQueueItem> item in self.itemsInternal)
    {
        if (item.state == HBQueueItemStateReady)
        {
            return item;
        }
    }
    return nil;
}

- (HBQueueWorker *)firstAvailableWorker
{
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.isEncoding == NO)
        {
            return worker;
        }
    }
    return nil;
}

- (void)addStopAction
{

    id<HBQueueItem> nextItem = self.nextPendingQueueItem;
    NSUInteger index = nextItem ? [self.itemsInternal indexOfObject:nextItem] : self.itemsInternal.count;

    if ([nextItem isKindOfClass:[HBQueueActionStopItem class]] == NO)
    {
        HBQueueActionStopItem *stopItem = [[HBQueueActionStopItem alloc] init];
        [self addItems:@[stopItem] atIndexes:[NSIndexSet indexSetWithIndex:index]];
    }
}

- (void)start
{
    if (self.canEncode)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidStartNotification object:self];
        self.pausedOnBatteryPower = NO;
        [self preventSleep];
        [self encodeNextQueueItem];
    }
}

/**
 *  Starts the queue
 */
- (void)encodeNextQueueItem
{
    id<HBQueueItem> nextItem = self.nextPendingQueueItem;

    // since we have completed an encode, we go to the next
    if ([nextItem isKindOfClass:[HBQueueActionStopItem class]])
    {
        if (self.isEncoding == NO)
        {
            [HBUtilities writeToActivityLog:"Queue manually stopped"];
            NSUInteger index = [self.itemsInternal indexOfObject:nextItem];
            [self removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
            [self allowSleep];
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
        }
    }
    else
    {
        // Check to see if there are any more pending items in the queue
        HBQueueJobItem *nextJobItem = self.nextPendingQueueItem;
        HBQueueWorker *worker = self.firstAvailableWorker;

        if (nextJobItem && [self isDiskSpaceLowAtURL:nextJobItem.destinationFolderURL])
        {
            [HBUtilities writeToActivityLog:"Queue Stopped, low space on destination disk"];
            [self allowSleep];

            if (self.isEncoding == NO)
            {
                [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
            }
            else
            {
                [self pause];
            }

            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueLowSpaceAlertNotification object:self];
        }
        // If we still have more pending items in our queue, lets go to the next one
        else if (nextJobItem && worker && self.countOfEncodings < self.workersCount)
        {
            [worker encodeItem:nextJobItem];

            // Erase undo manager history
            [self.undoManager removeAllActions];

            if (self.firstAvailableWorker)
            {
                [self encodeNextQueueItem];
            }
        }
        else if (self.isEncoding == NO)
        {
            [HBUtilities writeToActivityLog:"Queue Done, there are no more pending encodes"];
            [self allowSleep];

            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueDidCompleteNotification object:self];
        }
    }

    [self save];
}

- (void)completedItem:(id<HBQueueItem>)item
{
    NSParameterAssert(item);
    [self save];
    [self encodeNextQueueItem];
}

/**
 * Cancels the current job
 */
- (void)doCancelAll
{
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.isEncoding)
        {
            [worker cancel];
        }
    }
}

/**
 * Cancels the current job and starts processing the next in queue.
 */
- (void)cancelCurrentAndContinue
{
    [self doCancelAll];
}

/**
 * Cancels the current job and stops libhb from processing the remaining encodes.
 */
- (void)cancelCurrentAndStop
{
    if (self.isEncoding)
    {
        [self addStopAction];
        [self doCancelAll];
    }
}

/**
 * Finishes the current job and stops libhb from processing the remaining encodes.
 */
- (void)finishCurrentAndStop
{
    if (self.isEncoding)
    {
        [self addStopAction];
    }
}

- (void)cancelItemsAtIndexes:(NSIndexSet *)indexes
{
    NSArray<id<HBQueueItem>> *items = [self.items objectsAtIndexes:indexes];

    for (HBQueueJobItem *item in items) {
        for (HBQueueWorker *worker in self.workers) {
            if (worker.item == item) {
                [worker cancel];
            }
        }
    }
}

- (nullable HBQueueWorker *)workerForItem:(HBQueueJobItem *)item
{
    for (HBQueueWorker *worker in self.workers)
    {
        if (worker.item == item)
        {
            return worker;
        }
    }
    return nil;
}

@end
