/*  HBQueue.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBQueueItem.h"
#import "HBQueueJobItem.h"
#import "HBQueueActionItem.h"
#import "HBQueueWorker.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBQueueDidAddItemNotification;
extern NSString * const HBQueueDidRemoveItemNotification;
extern NSString * const HBQueueDidChangeItemNotification;
extern NSString * const HBQueueItemNotificationIndexesKey;           // NSIndexSet

extern NSString * const HBQueueDidMoveItemNotification;
extern NSString * const HBQueueItemNotificationSourceIndexesKey;     // NSArray<NSNumber *>
extern NSString * const HBQueueItemNotificationTargetIndexesKey;     // NSArray<NSNumber *>

extern NSString * const HBQueueLowSpaceAlertNotification;

extern NSString * const HBQueueDidStartNotification;
extern NSString * const HBQueueDidCompleteNotification;
extern NSString * const HBQueueDidChangeStateNotification;

extern NSString * const HBQueueDidStartItemNotification;
extern NSString * const HBQueueDidCompleteItemNotification;
extern NSString * const HBQueueItemNotificationItemKey;              // HBQueueJobItem

@interface HBQueue : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithURL:(NSURL *)fileURL;

@property (nonatomic, readonly) NSArray<id<HBQueueItem>> *items;

@property (nonatomic, readonly) NSUInteger pendingItemsCount;
@property (nonatomic, readonly) NSUInteger failedItemsCount;
@property (nonatomic, readonly) NSUInteger completedItemsCount;
@property (nonatomic, readonly) NSUInteger workingItemsCount;

@property (nonatomic, readonly) NSUInteger workersCount;

@property (nonatomic) NSUndoManager *undoManager;

- (void)addJob:(HBJob *)job;
- (void)addJobs:(NSArray<HBJob *> *)jobs;

- (void)addItems:(NSArray<id<HBQueueItem>> *)items atIndexes:(NSIndexSet *)indexes;
- (void)removeItemsAtIndexes:(NSIndexSet *)indexes;
- (void)moveItems:(NSArray<id<HBQueueItem>> *)items toIndex:(NSUInteger)index;

- (BOOL)itemExistAtURL:(NSURL *)url;

- (void)prepareItemForEditingAtIndex:(NSUInteger)index;

- (void)removeAllItems;
- (void)removeCompletedAndCancelledItems;
- (void)removeNotWorkingItems;
- (void)removeCompletedItems;

- (void)resetItemsAtIndexes:(NSIndexSet *)indexes;
- (void)resetAllItems;
- (void)resetFailedItems;

@property (nonatomic, readonly) BOOL canEncode;
@property (nonatomic, readonly) BOOL isEncoding;

- (void)start;
- (void)cancelCurrentAndContinue;
- (void)cancelCurrentAndStop;
- (void)finishCurrentAndStop;
- (void)cancelItemsAtIndexes:(NSIndexSet *)indexes;

@property (nonatomic, readonly) BOOL canPause;
- (void)pause;

@property (nonatomic, readonly) BOOL canResume;
- (void)resume;

- (nullable HBQueueWorker *)workerForItem:(HBQueueJobItem *)item;
- (void)invalidateWorkers;

@end

NS_ASSUME_NONNULL_END
