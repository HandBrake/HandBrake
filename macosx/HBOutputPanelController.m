/**
 * @file
 * @date 18.5.2007
 *
 * Implementation of class HBOutputPanelController.
 */

#import "HBOutputPanelController.h"
#import "HBOutputRedirect.h"
#import "HBOutputFileWriter.h"
#import "HBUtilities.h"

/// Maximum amount of characters that can be shown in the view.
#define TextStorageUpperSizeLimit 125000

/// When old output is removed, this is the amount of characters that will be
/// left in outputTextStorage.
#define TextStorageLowerSizeLimit 120000

@interface HBOutputPanelController () <HBOutputRedirectListening>
{
    /// Textview that displays debug output.
    IBOutlet NSTextView *textView;

    /// Text storage for the debug output.
    NSTextStorage *outputTextStorage;
}

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
        /* NSWindowController likes to lazily load its window nib. Since this
         * controller tries to touch the outlets before accessing the window, we
         * need to force it to load immadiately by invoking its accessor.
         *
         * If/when we switch to using bindings, this can probably go away.
         */
        (void)[self window];

        // Additionally, redirect the output to a file on the disk.
        NSURL *outputLogFile = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"HandBrake-activitylog.txt"];

        _outputFile = [[HBOutputFileWriter alloc] initWithFileURL:outputLogFile];
        [[HBOutputRedirect stderrRedirect] addListener:_outputFile];
        [[HBOutputRedirect stdoutRedirect] addListener:_outputFile];

        // We initialize the outputTextStorage object for the activity window
        outputTextStorage = [[NSTextStorage alloc] init];
        [[textView layoutManager] replaceTextStorage:outputTextStorage];
        [[textView enclosingScrollView] setLineScroll:10];
        [[textView enclosingScrollView] setPageScroll:20];

        // Add ourself as stderr/stdout listener
        [[HBOutputRedirect stderrRedirect] addListener:self];
        [[HBOutputRedirect stdoutRedirect] addListener:self];

        // Lets report the HandBrake version number here to the activity log and text log file
        NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
        NSString *versionStringFull = [NSString stringWithFormat:@"Handbrake Version: %@  (%@)", infoDict[@"CFBundleShortVersionString"], infoDict[@"CFBundleVersion"]];
        [HBUtilities writeToActivityLog: "%s", versionStringFull.UTF8String];
    }
    return self;
}

/**
 * Stops redirection of stderr and releases resources.
 */
- (void)dealloc
{
    [[HBOutputRedirect stderrRedirect] removeListener:self];
    [[HBOutputRedirect stdoutRedirect] removeListener:self];
}

/**
 * Loads output panel from OutputPanel.nib and shows it.
 */
- (IBAction)showWindow:(id)sender
{
    [textView scrollToEndOfDocument:self];
    [super showWindow:sender];
}

/**
 * Displays text received from HBOutputRedirect in the text view
 * and write it to the log files.
 */
- (void)stderrRedirect:(NSString *)text
{
    NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:text];
	/* Actually write the libhb output to the text view (outputTextStorage) */
    [outputTextStorage appendAttributedString:attributedString];
    
	/* remove text from outputTextStorage as defined by TextStorageUpperSizeLimit and TextStorageLowerSizeLimit */
    if (outputTextStorage.length > TextStorageUpperSizeLimit)
    {
		[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length] - TextStorageLowerSizeLimit)];
    }

    if (self.window.isVisible)
    {
        [textView scrollToEndOfDocument:self];
    }
}
- (void)stdoutRedirect:(NSString *)text { [self stderrRedirect:text]; }

/**
 * Clears the output window.
 */
- (IBAction)clearOutput:(id)sender
{
	[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length])];
    /* We want to rewrite the app version info to the top of the activity window so it is always present */
    time_t _now = time( NULL );
    struct tm * now  = localtime( &_now );
    fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, [[HBUtilities handBrakeVersion] UTF8String]);
}

/**
 * Copies all text in the output window to pasteboard.
 */
- (IBAction)copyAllOutputToPasteboard:(id)sender
{
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	[pboard declareTypes:@[NSStringPboardType] owner:nil];
	[pboard setString:[outputTextStorage string] forType:NSStringPboardType];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openActivityLogFile:(id)sender
{
    /* Opens the activity window log file in the users default text editor */
    [[NSWorkspace sharedWorkspace] openURL:self.outputFile.url];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openEncodeLogDirectory:(id)sender
{
    /* Opens the activity window log file in the users default text editor */
    NSURL *encodeLogDirectory = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"EncodeLogs"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:encodeLogDirectory.path] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:encodeLogDirectory.path
                                            withIntermediateDirectories:NO
                                            attributes:nil
                                            error:nil];
    }
    [[NSWorkspace sharedWorkspace] openURL:encodeLogDirectory];
}

- (IBAction)clearActivityLogFile:(id)sender
{
    [self.outputFile clear];
}

@end
