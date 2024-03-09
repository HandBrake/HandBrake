/* HBQueueController.h

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HBAppDelegate;
@class HBController;
@class HBQueue;

@interface HBQueueController : NSWindowController <NSToolbarDelegate, NSWindowDelegate>

- (instancetype)initWithQueue:(HBQueue *)queue;

@property (nonatomic, weak, readonly) HBQueue *queue;

@property (nonatomic, weak, nullable) HBAppDelegate *delegate;

- (IBAction)toggleStartCancel:(id)sender;
- (IBAction)togglePauseResume:(id)sender;
- (IBAction)toggleDetails:(id)sender;
- (IBAction)toggleQuickLook:(id)sender;

- (IBAction)resetAll:(id)sender;
- (IBAction)resetFailed:(id)sender;
- (IBAction)removeAll:(id)sender;
- (IBAction)removeCompleted:(id)sender;

@end

NS_ASSUME_NONNULL_END

