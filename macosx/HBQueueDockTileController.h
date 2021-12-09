/* HBQueueDockTileController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBQueue;

NS_ASSUME_NONNULL_BEGIN

@interface HBQueueDockTileController : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithQueue:(HBQueue *)queue dockTile:(NSDockTile *)dockTile image:(NSImage *)image NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
