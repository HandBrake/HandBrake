/**
 * @file
 * @date 18.5.2007
 *
 * Implementation of class HBOutputPanelController.
 */

#import "HBOutputPanelController.h"
#import "HBOutputRedirect.h"

/// Maximum amount of characters that can be shown in the view.
// Original value used by cleaner
//#define TextStorageUpperSizeLimit 20000
// lets use this higher value for now for better gui debugging
#define TextStorageUpperSizeLimit 40000

/// When old output is removed, this is the amount of characters that will be
/// left in outputTextStorage.
// Original value used by cleaner
//#define TextStorageLowerSizeLimit 15000
// lets use this higher value for now for better gui debugging
#define TextStorageLowerSizeLimit 35000

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

        /* We initialize the outputTextStorage object for the activity window */
        outputTextStorage = [[NSTextStorage alloc] init];

        /* We declare the default NSFileManager into fileManager */
        NSFileManager * fileManager = [NSFileManager defaultManager];
        /* Establish the log file location to write to */
        /* We are initially using a .txt file as opposed to a .log file since it will open by
         * default with the users text editor instead of the .log default Console.app, should
         * create less confusion for less experienced users when we ask them to paste the log for support
         */
        outputLogFile = @"~/Library/Application Support/HandBrake/HandBrake-activitylog.txt";
        outputLogFile = [[outputLogFile stringByExpandingTildeInPath]retain];

        /* We check for an existing output log file here */
        if( [fileManager fileExistsAtPath:outputLogFile] == 0 )
        {
            /* if not, then we create a new blank one */
            [fileManager createFileAtPath:outputLogFile contents:nil attributes:nil];
        }
        /* We overwrite the existing output log with the date for starters the output log to start fresh with the new session */
        /* Use the current date and time for the new output log header */
        NSString *startOutputLogString = [NSString stringWithFormat: @"HandBrake Activity Log for Session (Cleared): %@\n\n", [[NSDate  date] descriptionWithCalendarFormat:nil timeZone:nil locale:nil]];

        [startOutputLogString writeToFile:outputLogFile atomically:YES encoding:NSUTF8StringEncoding error:NULL];

        [[HBOutputRedirect stderrRedirect] addListener:self];
        [[HBOutputRedirect stdoutRedirect] addListener:self];

        [self setWindowFrameAutosaveName:@"OutputPanelFrame"];
        [[textView layoutManager] replaceTextStorage:outputTextStorage];
        [[textView enclosingScrollView] setLineScroll:10];
        [[textView enclosingScrollView] setPageScroll:20];
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
- (IBAction)showOutputPanel:(id)sender
{
    [textView scrollRangeToVisible:NSMakeRange([outputTextStorage length], 0)];
    [self showWindow:sender];

    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"OutputPanelIsOpen"];
}

/**
 * Displays text received from HBOutputRedirect in the text view.
 */
- (void)stderrRedirect:(NSString *)text
{
	
    NSAttributedString *attributedString = [[NSAttributedString alloc] initWithString:text];
	/* Actually write the libhb output to the text view (outputTextStorage) */
    [outputTextStorage appendAttributedString:attributedString];
    [attributedString release];
    
	/* remove text from outputTextStorage as defined by TextStorageUpperSizeLimit and TextStorageLowerSizeLimit */
    if ([outputTextStorage length] > TextStorageUpperSizeLimit)
		[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length] - TextStorageLowerSizeLimit)];
    
    [textView scrollRangeToVisible:NSMakeRange([outputTextStorage length], 0)];
    
    /* We use a c function to write to the log file without reading it into memory 
        * as it should be faster and easier on memory than using cocoa's writeToFile
        * thanks ritsuka !!*/
    FILE *f = fopen([outputLogFile UTF8String], "a");
    fprintf(f, "%s", [text UTF8String]);
    fclose(f);
    
    
    /* Below uses Objective-C to write to the file, though it is slow and uses
        * more memory than the c function above. For now, leaving this in here
        * just in case and commented out.
    */
    /* Put the new incoming string from libhb into an nsstring for appending to our log file */
    //NSString *newOutputString = [[NSString alloc] initWithString:text];
    /*get the current log file and put it into an NSString */
    /* HACK ALERT: must be a way to do it without reading the whole log into memory 
        Performance note: could batch write to the log, but want to get each line as it comes out of
        libhb in case of a crash or freeze so we see exactly what the last thing was before crash*/
    //NSString *currentOutputLogString = [[NSString alloc]initWithContentsOfFile:outputLogFile encoding:NSUTF8StringEncoding error:NULL];
    
    /* Append the new libhb output string to the existing log file string */
    //currentOutputLogString = [currentOutputLogString stringByAppendingString:newOutputString];
    /* Save the new modified log file string back to disk */
    //[currentOutputLogString writeToFile:outputLogFile atomically:YES encoding:NSUTF8StringEncoding error:NULL];
    /* Release the new libhb output string */
    //[newOutputString release];
}
- (void)stdoutRedirect:(NSString *)text { [self stderrRedirect:text]; }

/**
 * Clears the output window.
 */
- (IBAction)clearOutput:(id)sender
{
	[outputTextStorage deleteCharactersInRange:NSMakeRange(0, [outputTextStorage length])];
    /* We want to rewrite the app version info to the top of the activity window so it is always present */
    NSString *versionStringFull = [[NSString stringWithFormat: @"Handbrake Version: %@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleGetInfoString"]] stringByAppendingString: [NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
    time_t _now = time( NULL );
    struct tm * now  = localtime( &_now );
    fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, [versionStringFull UTF8String]);
    
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
    NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@", @"tell application \"Finder\" to open (POSIX file \"", outputLogFile, @"\")"]];
    [myScript executeAndReturnError: nil];
    [myScript release];
}

- (IBAction)clearActivityLogFile:(id)sender
{
    /* We overwrite the existing output log with the new date and time header */
        /* Use the current date and time for the new output log header */
        NSString *startOutputLogString = [NSString stringWithFormat: @"HandBrake Activity Log for Session Starting: %@\n\n", [[NSDate  date] descriptionWithCalendarFormat:nil timeZone:nil locale:nil]];
        [startOutputLogString writeToFile:outputLogFile atomically:NO encoding:NSUTF8StringEncoding error:NULL];
        
        /* We want to rewrite the app version info to the top of the activity window so it is always present */
        NSString *versionStringFull = [[NSString stringWithFormat: @"macgui: Handbrake Version: %@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleGetInfoString"]] stringByAppendingString: [NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
        [versionStringFull writeToFile:outputLogFile atomically:NO encoding:NSUTF8StringEncoding error:NULL];
        
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"OutputPanelIsOpen"];
}


@end
