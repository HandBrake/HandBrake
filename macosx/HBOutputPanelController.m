/**
 * @file
 * @date 18.5.2007
 *
 * Implementation of class HBOutputPanelController.
 */

#import "HBOutputPanelController.h"
#import "HBOutputRedirect.h"

/// Maximum amount of characters that can be shown in the view.
#define TextStorageUpperSizeLimit 20000

/// When old output is removed, this is the amount of characters that will be
/// left in outputTextStorage.
#define TextStorageLowerSizeLimit 15000

@implementation HBOutputPanelController

/**
 * Initializes the object, creates outputTextStorage and starts redirection of stderr.
 */
- (id)init
{
	if (self = [super init])
	{
		outputTextStorage = [[NSTextStorage alloc] init];
		[[HBOutputRedirect stderrRedirect] addListener:self];
	}
	return self;
}

/**
 * Stops redirection of stderr and releases resources.
 */
- (void)dealloc
{
	[[HBOutputRedirect stderrRedirect] removeListener:self];	
	[outputTextStorage release];
	[outputPanel release];
	[super dealloc];
}

/**
 * Loads output panel from OutputPanel.nib and shwos it.
 */
- (IBAction)showOutputPanel:(id)sender
{
	if (!outputPanel)
	{
		BOOL loadSucceeded = [NSBundle loadNibNamed:@"OutputPanel" owner:self] && outputPanel;
		NSAssert(loadSucceeded, @"Could not open nib file");
		
		[outputPanel setFrameAutosaveName:@"OutputPanelFrame"];
		[[textView layoutManager] replaceTextStorage:outputTextStorage];
	}
		
    [textView scrollRangeToVisible:NSMakeRange([outputTextStorage length], 0)];
	[outputPanel orderFront:nil];
}

/**
 * Displays text received from HBOutputRedirect in the text view.
 */
- (void)stderrRedirect:(NSString *)text
{
	NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:text];
	[outputTextStorage appendAttributedString:attributedString];
	[attributedString release];

	if ([outputTextStorage length] > TextStorageUpperSizeLimit)
		[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length] - TextStorageLowerSizeLimit)];

    [textView scrollRangeToVisible:NSMakeRange([outputTextStorage length], 0)];
}

/**
 * Clears the output window.
 */
- (IBAction)clearOutput:(id)sender
{
	[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length])];
}

/**
 * Copies all text in the output window to pasteboard.
 */
- (IBAction)copyAllOutputToPasteboard:(id)sender
{
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	[pboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	[pboard setString:[outputTextStorage string] forType:NSStringPboardType];
}

@end
