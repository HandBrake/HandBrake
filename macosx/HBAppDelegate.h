/*  HBAppDelegate.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface HBAppDelegate : NSObject <NSApplicationDelegate>

- (IBAction)showPreferencesWindow:(id)sender;
- (IBAction)showOutputPanel:(id)sender;

@end
