/* HBControllerToolbarDelegate.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueue.h"

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

#define TOOLBAR_OPEN     @"TOOLBAR_OPEN"
#define TOOLBAR_ADD      @"TOOLBAR_ADD"
#define TOOLBAR_START    @"TOOLBAR_START"
#define TOOLBAR_PAUSE    @"TOOLBAR_PAUSE"
#define TOOLBAR_PRESET   @"TOOLBAR_PRESET"
#define TOOLBAR_PREVIEW  @"TOOLBAR_PREVIEW"
#define TOOLBAR_QUEUE    @"TOOLBAR_QUEUE"
#define TOOLBAR_ACTIVITY @"TOOLBAR_ACTIVITY"

@interface HBControllerToolbarDelegate : NSObject<NSToolbarDelegate>

- (instancetype)initWithTarget:(id)target;

- (void)updateToolbarButtonsStateForScanCore:(HBState)state toolbar:(NSToolbar *)toolbar;
- (void)updateToolbarButtonsState:(HBQueue *)queue toolbar:(NSToolbar *)toolbar;
- (void)updateToolbarQueueBadge:(NSString *)value toolbar:(NSToolbar *)toolbar;

@end

NS_ASSUME_NONNULL_END
