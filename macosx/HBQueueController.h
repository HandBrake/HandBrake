/* HBQueueController.h

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBController;
@class HBCore;

@interface HBQueueController : NSWindowController <NSToolbarDelegate, NSWindowDelegate>

- (void)setPidNum: (int)myPidnum;
- (void)setCore: (HBCore *)core;
- (void)setHBController: (HBController *)controller;

- (void)setQueueArray: (NSMutableArray *)QueueFileArray;
- (void)setQueueStatusString: (NSString *)statusString;

- (IBAction)showQueueWindow: (id)sender;

@end
