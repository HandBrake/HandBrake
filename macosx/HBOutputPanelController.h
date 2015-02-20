/**
 * @file
 * @date 18.5.2007
 *
 * Interface of class HBOutputPanelController.
 */

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
