/* HBQueueToolbarDelegate.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBQueue.h"

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

#define TOOLBAR_START       @"TOOLBAR_START"
#define TOOLBAR_PAUSE       @"TOOLBAR_PAUSE"
#define TOOLBAR_WHEN_DONE   @"TOOLBAR_WHEN_DONE"
#define TOOLBAR_ACTION      @"TOOLBAR_ACTION"
#define TOOLBAR_DETAILS     @"TOOLBAR_DETAILS"
#define TOOLBAR_QUICKLOOK   @"TOOLBAR_QUICKLOOK"

@interface HBQueueToolbarDelegate : NSObject<NSToolbarDelegate>

- (void)updateToolbarButtonsState:(HBQueue *)queue toolbar:(NSToolbar *)toolbar;

@end

NS_ASSUME_NONNULL_END
