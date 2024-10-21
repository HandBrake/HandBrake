/*  HBAppDelegate.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBJob;

@interface HBAppDelegate : NSObject <NSApplicationDelegate>

- (IBAction)showQueueWindow:(id)sender;
- (IBAction)showPreferencesWindow:(id)sender;
- (IBAction)showOutputPanel:(id)sender;

- (IBAction)toggleStartCancel:(id)sender;
- (IBAction)togglePauseResume:(id)sender;

- (IBAction)reloadPreset:(id)sender;

- (void)openJob:(HBJob *)job completionHandler:(void (^)(BOOL result))handler;

@end
