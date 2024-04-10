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

@property (nonatomic, unsafe_unretained) IBOutlet NSTextView *textView;

@property (nonatomic, readonly) NSMutableString *textBuffer;
@property (nonatomic, readonly) NSDictionary *textAttributes;

@property (nonatomic, copy, readonly) HBOutputFileWriter *outputWriter;

@end

@implementation HBOutputPanelController

/**
 * Initializes the object, creates outputTextStorage and starts redirection of stderr.
 */
- (instancetype)init
{
    if( (self = [super initWithWindowNibName:@"OutputPanel"]) )
    {
        _textAttributes = @{NSForegroundColorAttributeName: NSColor.textColor};
        _textBuffer = [[NSMutableString alloc] init];

        // Add ourself as stderr/stdout listener
        [HBOutputRedirect.stderrRedirect addListener:self queue:dispatch_get_main_queue()];
        [HBOutputRedirect.stdoutRedirect addListener:self queue:dispatch_get_main_queue()];

        // Redirect the output to a file on the disk.
        NSURL *outputLogURL = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"HandBrake-activitylog.txt"];
        if (outputLogURL)
        {
            _outputWriter = [[HBOutputFileWriter alloc] initWithFileURL:outputLogURL];

            if (_outputWriter)
            {
                [HBOutputRedirect.stderrRedirect addListener:_outputWriter queue:dispatch_get_main_queue()];
                [HBOutputRedirect.stdoutRedirect addListener:_outputWriter queue:dispatch_get_main_queue()];
            }
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
    self.window.tabbingMode = NSWindowTabbingModeDisallowed;
}

- (IBAction)showWindow:(id)sender
{
    [super showWindow:sender];
    [self displayBuffer];
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

- (void)appendToTextView:(NSString *)text
{
    NSTextStorage *textStorage = self.textView.textStorage;
    NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:text attributes:_textAttributes];

    [textStorage appendAttributedString:attributedString];

    // Remove text from outputTextStorage as defined by TextStorageUpperSizeLimit and TextStorageLowerSizeLimit
    if (textStorage.length > TextStorageUpperSizeLimit)
    {
        [textStorage deleteCharactersInRange:NSMakeRange(0, textStorage.length - TextStorageLowerSizeLimit)];
    }

    [_textView scrollToEndOfDocument:self];
}

- (void)appendToBuffer:(NSString *)text
{
    [_textBuffer appendString:text];
    if (_textBuffer.length > TextStorageUpperSizeLimit)
    {
        [_textBuffer deleteCharactersInRange:NSMakeRange(0, _textBuffer.length - TextStorageLowerSizeLimit)];
    }
}

- (void)displayBuffer
{
    [self appendToTextView:self.textBuffer];
    [self.textBuffer deleteCharactersInRange:NSMakeRange(0, self.textBuffer.length)];
}

/**
 * Displays text received from HBOutputRedirect in the text view
 */
- (void)redirect:(NSString *)text type:(HBRedirectType)type
{
    if (self.windowLoaded && self.window.isVisible)
    {
        [self appendToTextView:text];
    }
    else
    {
        [self appendToBuffer:text];
    }
}

/**
 * Clears the output window.
 */
- (IBAction)clearOutput:(id)sender
{
	[self.textView.textStorage deleteCharactersInRange:NSMakeRange(0, self.textView.textStorage.length)];
    [self writeHeader];
}

/**
 * Copies all text in the output window to pasteboard.
 */
- (IBAction)copyAllOutputToPasteboard:(id)sender
{
	NSPasteboard *pboard = NSPasteboard.generalPasteboard;
    [pboard declareTypes:@[NSPasteboardTypeString] owner:nil];
    [pboard setString:self.textView.textStorage.string forType:NSPasteboardTypeString];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openActivityLogFile:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:self.outputWriter.url];
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
    [self.outputWriter clear];
}

@end
