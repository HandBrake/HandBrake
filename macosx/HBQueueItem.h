/*  HBQueueItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBDistributedArray.h"

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

/**
 *  A flag to indicate the item's state
 */
typedef NS_ENUM(NSUInteger, HBQueueItemState){
    HBQueueItemStateReady,
    HBQueueItemStateWorking,
    HBQueueItemStateCompleted,
    HBQueueItemStateCanceled,
    HBQueueItemStateFailed
};

@interface HBQueueItem : NSObject<NSSecureCoding, HBUniqueObject>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithJob:(HBJob *)job;

/// Current state of the job.
@property (nonatomic, readwrite) HBQueueItemState state;

/// The file URL of the source.
@property (nonatomic, readonly) NSURL *fileURL;

/// The file URL at which the new file will be created.
@property (nonatomic, readonly, copy) NSURL *outputURL;

/// The name of the new file that will be created.
@property (nonatomic, readonly, copy) NSString *outputFileName;

/// The file URL at which the new file will be created.
@property (nonatomic, readonly, copy) NSURL *completeOutputURL;

@property (nonatomic, readonly) NSAttributedString *attributedTitleDescription;
@property (nonatomic, readonly) NSAttributedString *attributedDescription;

@property (nonatomic, readwrite) BOOL expanded;

@property (nonatomic, readonly) HBJob *job;

@end

NS_ASSUME_NONNULL_END
