/*  HBQueueItemView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBJob;

NS_ASSUME_NONNULL_BEGIN

@protocol HBQueueItemViewDelegate

- (void)revealQueueItem:(HBJob *)job;
- (void)removeQueueItem:(HBJob *)job;
- (void)toggleQueueItemHeight:(HBJob *)job;

@end

@interface HBQueueItemView : NSTableCellView

@property (nonatomic, weak, nullable) HBJob *job;
@property (nonatomic, weak, nullable) id <HBQueueItemViewDelegate> delegate;
@property (nonatomic) BOOL expanded;

- (void)expand;
- (void)collapse;

@end

NS_ASSUME_NONNULL_END
