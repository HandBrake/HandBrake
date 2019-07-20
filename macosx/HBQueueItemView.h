/*  HBQueueItemView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBQueueItem;

NS_ASSUME_NONNULL_BEGIN

@protocol HBQueueItemViewDelegate

- (void)revealQueueItem:(HBQueueItem *)job;
- (void)removeQueueItem:(HBQueueItem *)job;

@end

@interface HBQueueItemView : NSTableCellView

@property (nonatomic, weak, nullable) HBQueueItem *item;
@property (nonatomic, weak, nullable) id <HBQueueItemViewDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
