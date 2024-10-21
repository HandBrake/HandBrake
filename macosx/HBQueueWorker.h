/*  HBQueueWorker.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBQueueJobItem.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBQueueWorkerDidChangeStateNotification;

extern NSString * const HBQueueWorkerProgressNotification;
extern NSString * const HBQueueWorkerProgressNotificationPercentKey;       // NSNumber - double
extern NSString * const HBQueueWorkerProgressNotificationHoursKey;         // NSNumber - double
extern NSString * const HBQueueWorkerProgressNotificationMinutesKey;       // NSNumber - double
extern NSString * const HBQueueWorkerProgressNotificationSecondsKey;       // NSNumber - double
extern NSString * const HBQueueWorkerProgressNotificationInfoKey;          // NSString

extern NSString * const HBQueueWorkerDidStartItemNotification;
extern NSString * const HBQueueWorkerDidCompleteItemNotification;
extern NSString * const HBQueueWorkerItemNotificationItemKey;              // HBQueueItem

@interface HBQueueWorker : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithXPCServiceName:(NSString *)serviceName;

@property (nonatomic, nullable, readonly) HBQueueJobItem *item;

- (void)encodeItem:(HBQueueJobItem *)item;
- (void)cancel;

@property (nonatomic, readonly) BOOL canEncode;
@property (nonatomic, readonly) BOOL isEncoding;

@property (nonatomic, readonly) BOOL canPause;
- (void)pause;

@property (nonatomic, readonly) BOOL canResume;
- (void)resume;

- (void)invalidate;

@end

NS_ASSUME_NONNULL_END
