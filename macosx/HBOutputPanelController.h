/*  HBOutputPanelController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

/**
 * This class implements a panel that displays all text that is written
 * to stderr. User can easily copy the text to pasteboard from context menu.
 */
@interface HBOutputPanelController : NSWindowController

- (IBAction)clearOutput:(id)sender;
- (IBAction)copyAllOutputToPasteboard:(id)sender;
- (IBAction)openActivityLogFile:(id)sender;
- (IBAction)openEncodeLogDirectory:(id)sender;
- (IBAction)clearActivityLogFile:(id)sender;

@end
