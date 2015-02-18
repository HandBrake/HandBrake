/**
 * @file
 * @date 18.5.2007
 *
 * Implementation of class HBOutputPanelController.
 */

#import "HBOutputPanelController.h"
#import "HBOutputRedirect.h"
#import "HBUtilities.h"

/// Maximum amount of characters that can be shown in the view.
// Original value used by cleaner
//#define TextStorageUpperSizeLimit 20000
// lets use this higher value for now for better gui debugging
#define TextStorageUpperSizeLimit 125000

/// When old output is removed, this is the amount of characters that will be
/// left in outputTextStorage.
// Original value used by cleaner
//#define TextStorageLowerSizeLimit 15000
// lets use this higher value for now for better gui debugging
#define TextStorageLowerSizeLimit 120000

@interface HBOutputPanelController () <HBOutputRedirectListening>
{
    /// Textview that displays debug output.
    IBOutlet NSTextView *textView;

    /// Text storage for the debug output.
    NSTextStorage *outputTextStorage;
}

/// Path to log text file.
@property (nonatomic, copy) NSString *outputLogFile;

/// Path to individual log text file.
@property (nonatomic, copy) NSString *outputLogFileForEncode;

/// Whether we are writing an addition log file for the current encode or not.
@property (nonatomic) BOOL encodeLogOn;

@end

@implementation HBOutputPanelController

/**
 * Initializes the object, creates outputTextStorage and starts redirection of stderr.
 */
- (id)init
{
    if( (self = [super initWithWindowNibName:@"OutputPanel"]) )
    {
        /* NSWindowController likes to lazily load its window nib. Since this
         * controller tries to touch the outlets before accessing the window, we
         * need to force it to load immadiately by invoking its accessor.
         *
         * If/when we switch to using bindings, this can probably go away.
         */
        [self window];

        // Use the inline search bar if available.
        if ([textView respondsToSelector:@selector(setUsesFindBar:)])
        {
            [textView setUsesFindBar:YES];
        }

        /* We initialize the outputTextStorage object for the activity window */
        outputTextStorage = [[NSTextStorage alloc] init];

        /* We declare the default NSFileManager into fileManager */
        NSFileManager * fileManager = [NSFileManager defaultManager];
        /* Establish the log file location to write to */
        /* We are initially using a .txt file as opposed to a .log file since it will open by
         * default with the users text editor instead of the .log default Console.app, should
         * create less confusion for less experienced users when we ask them to paste the log for support
         */
        _outputLogFile = [[[HBUtilities appSupportPath] stringByAppendingPathComponent:@"HandBrake-activitylog.txt"] retain];

        /* We check for an existing output log file here */
        if ([fileManager fileExistsAtPath:_outputLogFile] == NO)
        {
            /* if not, then we create a new blank one */
            [fileManager createFileAtPath:_outputLogFile contents:nil attributes:nil];
        }
        /* We overwrite the existing output log with the date for starters the output log to start fresh with the new session */
        /* Use the current date and time for the new output log header */
        NSString *startOutputLogString = [self logHeaderForReason:@"Session (Cleared)"];
        [startOutputLogString writeToFile:_outputLogFile atomically:YES encoding:NSUTF8StringEncoding error:NULL];

        [[HBOutputRedirect stderrRedirect] addListener:self];
        [[HBOutputRedirect stdoutRedirect] addListener:self];

        [[textView layoutManager] replaceTextStorage:outputTextStorage];
        [[textView enclosingScrollView] setLineScroll:10];
        [[textView enclosingScrollView] setPageScroll:20];
        
        _encodeLogOn = NO;
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
    [outputTextStorage release];
    [super dealloc];
}

/**
 * Loads output panel from OutputPanel.nib and shows it.
 */
- (IBAction)showWindow:(id)sender
{
    if ([[self window] isVisible])
    {
        [[self window] close];
    }
    else
    {
        [textView scrollToEndOfDocument:self];
        [super showWindow:sender];

        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"OutputPanelIsOpen"];
    }
}

- (NSString *)logHeaderForReason:(NSString *)reason
{
    return [NSString stringWithFormat:@"HandBrake Activity Log for %@: %@\n%@",
            reason,
            [[NSDate date] descriptionWithCalendarFormat:nil timeZone:nil locale:nil],
            [self handBrakeVersion]];
}

- (NSString *)handBrakeVersion
{
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    return [NSString stringWithFormat:@"Handbrake Version: %@ (%@)",
            infoDictionary[@"CFBundleShortVersionString"],
            infoDictionary[@"CFBundleVersion"]];
}

- (void)startEncodeLog:(NSURL *)logURL
{
    self.encodeLogOn = YES;
    NSString *outputFileForEncode = logURL.path ;
    /* Since the destination path matches the extension of the output file, replace the
     * output movie extension and replace it with ".txt"
     */
    NSFileManager * fileManager = [NSFileManager defaultManager];
    /* Establish the log file location to write to */
    /* We are initially using a .txt file as opposed to a .log file since it will open by
     * default with the users text editor instead of the .log default Console.app, should
     * create less confusion for less experienced users when we ask them to paste the log for support
     */
    /* We need to get the current time in YY-MM-DD HH-MM-SS format to put at the beginning of the name of the log file */
    time_t _now = time( NULL );
    struct tm * now  = localtime( &_now );
    NSString *dateForLogTitle = [NSString stringWithFormat:@"%02d-%02d-%02d %02d-%02d-%02d",now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,now->tm_hour, now->tm_min, now->tm_sec]; 
    
    /* Assemble the new log file name as YY-MM-DD HH-MM-SS mymoviename.txt */
    NSString *outputDateFileName = [NSString stringWithFormat:@"%@ %@.txt",[[outputFileForEncode lastPathComponent] stringByDeletingPathExtension],dateForLogTitle];
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"EncodeLogLocation"]) // if we are putting it in the same directory with the movie
    {
        self.outputLogFileForEncode = [NSString stringWithFormat:@"%@/%@",[outputFileForEncode stringByDeletingLastPathComponent], outputDateFileName];
    }
    else // if we are putting it in the default ~/Libraries/Application Support/HandBrake/EncodeLogs logs directory
    {
        NSString *encodeLogDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"EncodeLogs"];
        if( ![[NSFileManager defaultManager] fileExistsAtPath:encodeLogDirectory] )
        {
            [[NSFileManager defaultManager] createDirectoryAtPath:encodeLogDirectory
                                            withIntermediateDirectories:NO
                                            attributes:nil
                                            error:nil];
        }
        self.outputLogFileForEncode = [NSString stringWithFormat:@"%@/%@",encodeLogDirectory,outputDateFileName];
    }
    [fileManager createFileAtPath:self.outputLogFileForEncode contents:nil attributes:nil];
    
    /* Similar to the regular activity log, we print a header containing the date and time of the encode as well as what directory it was encoded to */
    NSString *startOutputLogString = [self logHeaderForReason:outputFileForEncode];
    [startOutputLogString writeToFile:self.outputLogFileForEncode atomically:YES encoding:NSUTF8StringEncoding error:NULL];
}

- (void) endEncodeLog
{
    self.encodeLogOn = NO;
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
    [attributedString release];
    
	/* remove text from outputTextStorage as defined by TextStorageUpperSizeLimit and TextStorageLowerSizeLimit */
    if (outputTextStorage.length > TextStorageUpperSizeLimit)
		[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length] - TextStorageLowerSizeLimit)];

    if (self.window.isVisible)
    {
        [textView scrollToEndOfDocument:self];
    }

    FILE *f = fopen(_outputLogFile.fileSystemRepresentation, "a");
    fprintf(f, "%s", text.UTF8String);
    fclose(f);

    if (_encodeLogOn == YES && _outputLogFileForEncode != nil)
    {
        FILE *e = fopen(_outputLogFileForEncode.fileSystemRepresentation, "a");
        fprintf(e, "%s", text.UTF8String);
        fclose(e);
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
    fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, [[self handBrakeVersion] UTF8String]);
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

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openActivityLogFile:(id)sender
{
    /* Opens the activity window log file in the users default text editor */
    NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@", @"tell application \"Finder\" to open (POSIX file \"", _outputLogFile, @"\")"]];
    [myScript executeAndReturnError: nil];
    [myScript release];
}

/**
 * Opens the activity log txt file in users default editor.
 */
- (IBAction)openEncodeLogDirectory:(id)sender
{
    /* Opens the activity window log file in the users default text editor */
    NSString *encodeLogDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"EncodeLogs"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:encodeLogDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:encodeLogDirectory
                                            withIntermediateDirectories:NO
                                            attributes:nil
                                            error:nil];
    }
    
    NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@", @"tell application \"Finder\" to open (POSIX file \"", encodeLogDirectory, @"\")"]];
    [myScript executeAndReturnError: nil];
    [myScript release];
}

- (IBAction)clearActivityLogFile:(id)sender
{
    /* We overwrite the existing output log with the new date and time header */
    /* Use the current date and time for the new output log header */
    NSString *startOutputLogString = [self logHeaderForReason:@"Session Starting"];
    [startOutputLogString writeToFile:_outputLogFile atomically:NO encoding:NSUTF8StringEncoding error:NULL];

    /* We want to rewrite the app version info to the top of the activity window so it is always present */
    NSString *versionStringFull = [self handBrakeVersion];
    [versionStringFull writeToFile:_outputLogFile atomically:NO encoding:NSUTF8StringEncoding error:NULL];
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"OutputPanelIsOpen"];
}


@end
