/*  HBQueueJobItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueueItem.h"

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@interface HBQueueJobItem : NSObject<HBQueueItem>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithJob:(HBJob *)job;

@property (nonatomic, readonly) HBJob *job;

/// Current state of the job.
@property (nonatomic) HBQueueItemState state;

/// The file URL of the source.
@property (nonatomic, readonly) NSURL *fileURL;

/// The directory URL at which the new file will be created.
@property (nonatomic, readonly, copy) NSURL *destinationFolderURL;

/// The name of the new file that will be created.
@property (nonatomic, readonly, copy) NSString *destinationFileName;

/// The file URL at which the new file will be created.
@property (nonatomic, readonly, copy) NSURL *destinationURL;

/// The file URL at which the new file will be created.
@property (nonatomic, copy, nullable) NSURL *activityLogURL;

@property (nonatomic, readonly) NSTimeInterval encodeDuration;
@property (nonatomic, readonly) NSTimeInterval pauseDuration;

@property (nonatomic, nullable) NSDate *startedDate;
@property (nonatomic, nullable) NSDate *endedDate;

- (void)pausedAtDate:(NSDate *)date;
- (void)resumedAtDate:(NSDate *)date;

@property (nonatomic, readonly) double avgFps;
@property (nonatomic, readonly) NSUInteger fileSize;
@property (nonatomic, readonly) NSAttributedString *attributedDescription;
@property (nonatomic, readonly, nullable) NSAttributedString *attributedStatistics;

- (void)setDoneWithResult:(HBCoreResult)result;

@end

NS_ASSUME_NONNULL_END
