/*  HBOutputPanelController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutputPanelController.h"
#import "HBOutputRedirect.h"
#import "HBOutputFileWriter.h"

@import HandBrakeKit.HBUtilities;

/// Maximum amount of characters that can be shown in the view.
#define TextStorageUpperSizeLimit 125000

/// When old output is removed, this is the amount of characters that will be
/// left in outputTextStorage.
#define TextStorageLowerSizeLimit 120000

@interface HBOutputPanelController () <HBOutputRedirectListening>

/// Textview that displays debug output.
@property (nonatomic, unsafe_unretained) IBOutlet NSTextView *textView;

/// Text storage for the debug output.
@property (nonatomic, readonly) NSTextStorage *outputTextStorage;
@property (nonatomic, readonly) NSDictionary *textAttributes;

/// Path to log text file.
@property (nonatomic, copy, readonly) HBOutputFileWriter *outputFile;

@end

@implementation HBOutputPanelController

/**
 * Initializes the object, creates outputTextStorage and starts redirection of stderr.
 */
- (instancetype)init
{
    if( (self = [super initWithWindowNibName:@"OutputPanel"]) )
    {
        // We initialize the outputTextStorage object for the activity window
        _outputTextStorage = [[NSTextStorage alloc] init];

        // Text attributes
        _textAttributes = @{NSForegroundColorAttributeName: NSColor.textColor};

        // Add ourself as stderr/stdout listener
        [HBOutputRedirect.stderrRedirect addListener:self queue:dispatch_get_main_queue()];
        [HBOutputRedirect.stdoutRedirect addListener:self queue:dispatch_get_main_queue()];

        // Redirect the output to a file on the disk.
        NSURL *outputLogFile = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"HandBrake-activitylog.txt"];

        _outputFile = [[HBOutputFileWriter alloc] initWithFileURL:outputLogFile];
        if (_outputFile)
        {
            [HBOutputRedirect.stderrRedirect addListener:_outputFile queue:dispatch_get_main_queue()];
            [HBOutputRedirect.stdoutRedirect addListener:_outputFile queue:dispatch_get_main_queue()];
        }

        [self writeHeader];
    }
    return self;
}

- (void)dealloc
{
    [HBOutputRedirect.stderrRedirect removeListener:self];
    [HBOutputRedirect.stdoutRedirect removeListener:self];
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    self.window.tabbingMode = NSWindowTabbingModeDisallowed;

    [_textView.layoutManager replaceTextStorage:_outputTextStorage];
    [_textView.enclosingScrollView setLineScroll:10];
    [_textView.enclosingScrollView setPageScroll:20];

    [_textView scrollToEndOfDocument:self];
}

- (IBAction)showWindow:(id)sender
{
    [_textView scrollToEndOfDocument:self];
    [super showWindow:sender];
}

/**
 Write the HandBrake version number to the log
 */
- (void)writeHeader
{
    NSDictionary *infoDict = NSBundle.mainBundle.infoDictionary;
    NSString *versionStringFull = [NSString stringWithFormat:@"Handbrake Version: %@ (%@)", infoDict[@"CFBundleShortVersionString"], infoDict[@"CFBundleVersion"]];
    [HBUtilities writeToActivityLog:"%s", versionStringFull.UTF8String];
}

/**
 * Displays text received from HBOutputRedirect in the text view
 */
- (void)redirect:(NSString *)text type:(HBRedirectType)type
{
    NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:text attributes:_textAttributes];
	// Actually write the libhb output to the text view (outputTextStorage)
    [_outputTextStorage appendAttributedString:attributedString];

	// remove text from outputTextStorage as defined by TextStorageUpperSizeLimit and TextStorageLowerSizeLimit */
    if (_outputTextStorage.length > TextStorageUpperSizeLimit)
    {
		[_outputTextStorage deleteCharactersInRange:NSMakeRange(0, _outputTextStorage.length - TextStorageLowerSizeLimit)];
    }

    if (self.windowLoaded && self.window.isVisible)
    {
        [_textView scrollToEndOfDocument:self];
    }
}

/**
 * Clears the output window.
 */
- (IBAction)clearOutput:(id)sender
{
	[_outputTextStorage deleteCharactersInRange:NSMakeRange(0, _outputTextStorage.length)];
    [self writeHeader];
}

/**
 * Copies all text in the output window to pasteboard.
 */
- (IBAction)copyAllOutputToPasteboard:(id)sender
{
	NSPasteboard *pboard = NSPasteboard.generalPasteboard;
    [pboard declareTypes:@[NSPasteboardTypeString] owner:nil];
    [pboard setString:_outputTextStorage.string forType:NSPasteboardTypeString];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openActivityLogFile:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:self.outputFile.url];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openEncodeLogDirectory:(id)sender
{
    // Opens the activity window log file in the users default text editor
    NSURL *encodeLogDirectory = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"EncodeLogs"];
    if (![NSFileManager.defaultManager fileExistsAtPath:encodeLogDirectory.path])
    {
        [NSFileManager.defaultManager createDirectoryAtPath:encodeLogDirectory.path
                                withIntermediateDirectories:NO
                                                 attributes:nil
                                                      error:nil];
    }
    [NSWorkspace.sharedWorkspace openURL:encodeLogDirectory];
}

- (IBAction)clearActivityLogFile:(id)sender
{
    [self.outputFile clear];
}

@end
