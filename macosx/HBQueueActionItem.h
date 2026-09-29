/*  HBQueueActionItem.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueueItem.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBQueueActionItem <HBQueueItem>
@end

@interface HBQueueActionStopItem : NSObject<HBQueueActionItem>

/// Current state of the item.
@property (nonatomic) HBQueueItemState state;

/// The title of the item.
@property (nonatomic, readonly) NSString *title;

@end

NS_ASSUME_NONNULL_END
