/*  HBQueueTableViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#import "HBQueue.h"
#import "HBQueueJobItem.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBQueueTableViewControllerDelegate

- (void)tableViewDidSelectItemsAtIndexes:(NSIndexSet *)indexes;
- (void)tableViewEditItem:(HBQueueJobItem *)item;
- (void)tableViewResetItemsAtIndexes:(NSIndexSet *)indexes;
- (void)tableViewRemoveItemsAtIndexes:(NSIndexSet *)indexes;
- (void)tableViewAddAction:(id<HBQueueActionItem>)action atIndex:(NSInteger)index;
@end

@interface HBQueueTableViewController : NSViewController

- (instancetype)initWithQueue:(HBQueue *)queue delegate:(id<HBQueueTableViewControllerDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END
