/*  HBQueueItemView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueueItem.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBQueueItemViewDelegate

- (void)revealQueueItem:(id<HBQueueItem>)item;
- (void)removeQueueItem:(id<HBQueueItem>)item;

@end

@interface HBQueueItemView : NSTableCellView

@property (nonatomic, weak, nullable) id<HBQueueItem> item;
@property (nonatomic, weak, nullable) id<HBQueueItemViewDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
