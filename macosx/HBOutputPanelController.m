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

@interface HBOutputPanelController () <HBOutputRedirectListening, NSWindowDelegate>

@property (nonatomic, unsafe_unretained) IBOutlet NSTextView *textView;

@property (nonatomic, readonly) NSMutableString *textBuffer;
@property (nonatomic, readonly) NSDictionary *textAttributes;

@property (nonatomic, readonly) HBOutputFileWriter *outputWriter;
@property (nonatomic) NSTimer *timer;

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
        NSURL *outputLogURL = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"HandBrake-activitylog.txt" isDirectory:NO];
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
    [self stopTimer];
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

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
    if (self.window.occlusionState & NSWindowOcclusionStateVisible)
    {
        [self startTimer];
    }
    else
    {
        [self stopTimer];
    }
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
    NSUInteger length = self.textBuffer.length;
    if (length)
    {
        [self appendToTextView:self.textBuffer];
        [self.textBuffer deleteCharactersInRange:NSMakeRange(0, length)];
    }
}

- (void)redirect:(NSString *)text type:(HBRedirectType)type
{
    [self appendToBuffer:text];
}

- (void)startTimer
{
    if (self.timer == nil)
    {
        self.timer = [NSTimer scheduledTimerWithTimeInterval:0.1
                                                      target:self
                                                    selector:@selector(timerFired:)
                                                    userInfo:nil
                                                     repeats:YES];
        self.timer.tolerance = 0.1;
    }
}

- (void)stopTimer
{
    [self.timer invalidate];
    self.timer = nil;
}

- (void)timerFired:(NSTimer *)timer
{
    [self displayBuffer];
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
    NSURL *encodeLogDirectory = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"EncodeLogs" isDirectory:YES];
    if (encodeLogDirectory)
    {
        if ([NSFileManager.defaultManager createDirectoryAtURL:encodeLogDirectory withIntermediateDirectories:YES attributes:nil error:NULL])
        {
            [NSWorkspace.sharedWorkspace openURL:encodeLogDirectory];
        }
    }
}

- (IBAction)clearActivityLogFile:(id)sender
{
    [self.outputWriter clear];
}

@end
