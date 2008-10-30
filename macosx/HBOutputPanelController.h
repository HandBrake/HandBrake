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
{
    /// Textview that displays debug output.
    IBOutlet NSTextView *textView;

    /// Text storage for the debug output.
    NSTextStorage *outputTextStorage;

    /// Path to log text file.
    NSString *outputLogFile;
    /// Path to individual log text file.
    NSString *outputLogFileForEncode;
    BOOL encodeLogOn;
}

- (IBAction)showOutputPanel:(id)sender;
- (IBAction)clearOutput:(id)sender;
- (IBAction)copyAllOutputToPasteboard:(id)sender;
- (IBAction)openActivityLogFile:(id)sender;
- (IBAction)openEncodeLogDirectory:(id)sender;
- (IBAction)clearActivityLogFile:(id)sender;
- (void) startEncodeLog:(NSString *) logPath;
- (void) endEncodeLog;

@end
