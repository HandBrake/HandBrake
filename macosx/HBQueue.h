/*  HBQueue.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBDistributedArray.h"
#import "HBQueueItem.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBQueueDidChangeStateNotification;

extern NSString * const HBQueueDidAddItemNotification;
extern NSString * const HBQueueDidRemoveItemNotification;
extern NSString * const HBQueueDidChangeItemNotification;
extern NSString * const HBQueueItemNotificationIndexesKey;           // NSIndexSet

extern NSString * const HBQueueDidMoveItemNotification;
extern NSString * const HBQueueItemNotificationSourceIndexesKey;     // NSArray<NSNumber *>
extern NSString * const HBQueueItemNotificationTargetIndexesKey;     // NSArray<NSNumber *>

extern NSString * const HBQueueReloadItemsNotification;

extern NSString * const HBQueueLowSpaceAlertNotification;

extern NSString * const HBQueueProgressNotification;
extern NSString * const HBQueueProgressNotificationPercentKey;       // NSNumber - double
extern NSString * const HBQueueProgressNotificationHoursKey;         // NSNumber - double
extern NSString * const HBQueueProgressNotificationMinutesKey;       // NSNumber - double
extern NSString * const HBQueueProgressNotificationSecondsKey;       // NSNumber - double
extern NSString * const HBQueueProgressNotificationInfoKey;          // NSString

extern NSString * const HBQueueDidStartNotification;
extern NSString * const HBQueueDidCompleteNotification;

extern NSString * const HBQueueDidStartItemNotification;
extern NSString * const HBQueueDidCompleteItemNotification;
extern NSString * const HBQueueItemNotificationItemKey;              // HBQueueItem

@interface HBQueue : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithURL:(NSURL *)queueURL;

@property (nonatomic, readonly) HBDistributedArray<HBQueueItem *> *items;

@property (nonatomic, nullable) HBQueueItem *currentItem;

@property (nonatomic) NSUInteger pendingItemsCount;
@property (nonatomic) NSUInteger failedItemsCount;
@property (nonatomic) NSUInteger completedItemsCount;

@property (nonatomic) NSUndoManager *undoManager;

- (void)addJob:(HBJob *)job;
- (void)addJobs:(NSArray<HBJob *> *)jobs;

- (void)addItems:(NSArray<HBQueueItem *> *)items atIndexes:(NSIndexSet *)indexes;
- (void)removeItemAtIndex:(NSUInteger)index;
- (void)removeItemsAtIndexes:(NSIndexSet *)indexes;
- (void)moveItems:(NSArray<HBQueueItem *> *)items toIndex:(NSUInteger)index;

- (BOOL)itemExistAtURL:(NSURL *)url;

- (void)removeAllItems;
- (void)removeCompletedAndCancelledItems;
- (void)removeNotWorkingItems;
- (void)removeCompletedItems;

- (void)resetItemsAtIndexes:(NSIndexSet *)indexes;
- (void)resetAllItems;
- (void)resetFailedItems;

- (void)setEncodingJobsAsPending;

@property (nonatomic, readonly) BOOL canEncode;
@property (nonatomic, readonly) BOOL isEncoding;

- (void)start;
- (void)cancelCurrentItemAndContinue;
- (void)finishCurrentAndStop;
- (void)cancelCurrentItemAndStop;

@property (nonatomic, readonly) BOOL canPause;
- (void)pause;

@property (nonatomic, readonly) BOOL canResume;
- (void)resume;

@end

NS_ASSUME_NONNULL_END
