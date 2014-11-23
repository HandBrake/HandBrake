/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */


#import <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;

@interface HBQueueController : NSWindowController <NSToolbarDelegate, NSWindowDelegate>

- (void)setPidNum: (int)myPidnum;
- (void)setHandle: (hb_handle_t *)handle;
- (void)setHBController: (HBController *)controller;

- (void)setQueueArray: (NSMutableArray *)QueueFileArray;

/* Animate the icon for the current encode */
- (void) animateWorkingEncodeIconInQueue;
- (void) startAnimatingCurrentWorkingEncodeInQueue;
- (void) stopAnimatingCurrentJobGroupInQueue;
- (void)setQueueStatusString: (NSString *)statusString;

- (IBAction)showQueueWindow: (id)sender;

@end
