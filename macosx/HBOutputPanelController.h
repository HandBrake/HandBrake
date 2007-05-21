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
@interface HBOutputPanelController : NSObject
{
	/// Panel that displays debug output.
	IBOutlet NSPanel *outputPanel;
	
	/// Textview that displays debug output.
	IBOutlet NSTextView *textView;
	
	/// Text storage for the debug output.
	NSTextStorage *outputTextStorage;
}

- (IBAction)showOutputPanel:(id)sender;
- (IBAction)clearOutput:(id)sender;
- (IBAction)copyAllOutputToPasteboard:(id)sender;

@end
