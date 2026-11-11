/*  HBQueueImportWindowController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueueItem.h"

NS_ASSUME_NONNULL_BEGIN

/// A sheet to choose which items of a queue file to import.
@interface HBQueueImportWindowController : NSWindowController

- (instancetype)initWithItems:(NSArray<id<HBQueueItem>> *)items;

@property (nonatomic, readonly) NSArray<id<HBQueueItem>> *selectedItems;

@end

NS_ASSUME_NONNULL_END
