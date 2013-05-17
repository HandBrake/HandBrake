/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <dlfcn.h>
#import "Controller.h"
#import "HBOutputPanelController.h"
#import "HBPreferencesController.h"
#import "HBDVDDetector.h"
#import "HBPresets.h"
#import "HBPreviewController.h"
#import "DockTextField.h"

unsigned int maximumNumberOfAllowedAudioTracks = 24;
NSString *HBContainerChangedNotification       = @"HBContainerChangedNotification";
NSString *keyContainerTag                      = @"keyContainerTag";
NSString *HBTitleChangedNotification           = @"HBTitleChangedNotification";
NSString *keyTitleTag                          = @"keyTitleTag";

NSString *dragDropFiles                        = @"dragDropFiles";

#define DragDropSimplePboardType                 @"MyCustomOutlineViewPboardType"

NSString *dockTilePercentFormat                = @"%2.1f%%";
// DockTile update freqency in total percent increment
#define dockTileUpdateFrequency                  0.1f

/* We setup the toolbar values here ShowPreviewIdentifier */
static NSString *        ToggleDrawerIdentifier             = @"Toggle Drawer Item Identifier";
static NSString *        StartEncodingIdentifier            = @"Start Encoding Item Identifier";
static NSString *        PauseEncodingIdentifier            = @"Pause Encoding Item Identifier";
static NSString *        ShowQueueIdentifier                = @"Show Queue Item Identifier";
static NSString *        AddToQueueIdentifier               = @"Add to Queue Item Identifier";
static NSString *        ShowPictureIdentifier              = @"Show Picture Window Item Identifier";
static NSString *        ShowPreviewIdentifier              = @"Show Preview Window Item Identifier";
static NSString *        ShowActivityIdentifier             = @"Debug Output Item Identifier";
static NSString *        ChooseSourceIdentifier             = @"Choose Source Item Identifier";


/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

+ (unsigned int) maximumNumberOfAllowedAudioTracks	{	return maximumNumberOfAllowedAudioTracks;	}

- (id)init
{
    self = [super init];
    if( !self )
    {
        return nil;
    }

    fApplicationIcon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:@"HandBrake.icns"]];

    if( fApplicationIcon != nil )
        [NSApp setApplicationIconImage:fApplicationIcon];
    
    [HBPreferencesController registerUserDefaults];
    fHandle = NULL;
    fQueueEncodeLibhb = NULL;
    /* Check for check for the app support directory here as
     * outputPanel needs it right away, as may other future methods
     */
    NSString *libraryDir = [NSSearchPathForDirectoriesInDomains( NSLibraryDirectory,
                                                                NSUserDomainMask,
                                                                YES ) objectAtIndex:0];
    AppSupportDirectory = [[libraryDir stringByAppendingPathComponent:@"Application Support"]
                           stringByAppendingPathComponent:@"HandBrake"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:AppSupportDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:AppSupportDirectory
                                                   attributes:nil];
    }
    /* Check for and create the App Support Preview directory if necessary */
    NSString *PreviewDirectory = [AppSupportDirectory stringByAppendingPathComponent:@"Previews"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:PreviewDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:PreviewDirectory
                                                   attributes:nil];
    }                                                            
    outputPanel = [[HBOutputPanelController alloc] init];
    fPictureController = [[PictureController alloc] init];
    fQueueController = [[HBQueueController alloc] init];
    fAdvancedOptions = [[HBAdvancedController alloc] init];
    /* we init the HBPresets class which currently is only used
     * for updating built in presets, may move more functionality
     * there in the future
     */
    fPresetsBuiltin = [[HBPresets alloc] init];
    fPreferencesController = [[HBPreferencesController alloc] init];
    /* Lets report the HandBrake version number here to the activity log and text log file */
    NSString *versionStringFull = [[NSString stringWithFormat: @"Handbrake Version: %@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"]] stringByAppendingString: [NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
    [self writeToActivityLog: "%s", [versionStringFull UTF8String]];
    
    /* Load the dockTile and instiante initial text fields */
    dockTile = [[NSApplication sharedApplication] dockTile];
    NSImageView *iv = [[NSImageView alloc] init];
    [iv setImage:[[NSApplication sharedApplication] applicationIconImage]];
    [dockTile setContentView:iv];
    
    /* We can move the specific values out from here by subclassing NSDockTile and package everything in here */
    /* If colors are to be chosen once and for all, we can also remove the instantiation with numerical values */
    percentField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 32.0f, [dockTile size].width, 30.0f)];
    [percentField changeGradientColors:[NSColor colorWithDeviceRed:0.4f green:0.6f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.2f green:0.4f blue:0.2f alpha:1.0f]];
    [iv addSubview:percentField];
    
    timeField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, [dockTile size].width, 30.0f)];
    [timeField changeGradientColors:[NSColor colorWithDeviceRed:0.6f green:0.4f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.4f green:0.2f blue:0.2f alpha:1.0f]];
    [iv addSubview:timeField];
    
    [self updateDockIcon:-1.0 withETA:@""];
    
    /*
     * initialize fX264PresetsUnparsedUTF8String as early as possible
     * avoids an invalid free
     */
    fX264PresetsUnparsedUTF8String = NULL;
    
    return self;
}

// This method is triggered at launch (and every launch) whether or not
// files have been dragged on to the dockTile. As a consequence, [self openFiles]
// contains the logic to detect the case when no files has been drop on the dock
- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    [self openFiles:filenames];
    
    [NSApp replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (void)openFiles:(NSArray*)filenames
{
    if (filenames.count == 1 && [[filenames objectAtIndex:0] isEqual:@"YES"])
        return;
    
    NSMutableArray* filesList = [[NSMutableArray alloc] initWithArray:filenames];
    [filesList removeObject:@"YES"];
    
    // For now, we just want to accept one file at a time
    // If for any reason, more than one file is submitted, we will take the first one
    if (filesList.count > 1)
    {
        filesList = [NSMutableArray arrayWithObject:[filesList objectAtIndex:0]];
    }
    
    // The goal of this check is to know if the application was running before the drag & drop
    // if fSubtitlesDelegate is set, then applicationDidFinishLaunching was called
    if (fSubtitlesDelegate)
    {
        // Handbrake was already running when the user dropped the file(s)
        // So we get unstack the first one and launch the scan
        // The other ones remain in the UserDefaults, and will be handled in the updateUI method
        // when Handbrake is idle
        id firstItem = [filesList objectAtIndex:0];
        [filesList removeObjectAtIndex:0];
        
        // This variable has only one goal, let the updateUI knows that even if idling
        // maybe a scan is in preparation
        fWillScan = YES;
        
        if (filesList.count > 0)
        {
            [[NSUserDefaults standardUserDefaults] setObject:filesList forKey:dragDropFiles];
        }
        else
        {
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:dragDropFiles];
        }
        
        [browsedSourceDisplayName release];
        browsedSourceDisplayName = [[firstItem lastPathComponent] retain];
        [self performScan:firstItem scanTitleNum:0];
    }
    else
    {
        // Handbrake was not running before the user dropped the file(s)
        // So we save the file(s) list in the UserDefaults and we will read them
        // in the applicationDidFinishLaunching method
        [[NSUserDefaults standardUserDefaults] setObject:filesList forKey:dragDropFiles];
    }
}

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    /* Init libhb with check for updates libhb style set to "0" so its ignored and lets sparkle take care of it */
    int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
    fHandle = hb_init(loggingLevel, 0);
    /* Optional dvd nav UseDvdNav*/
    hb_dvd_set_dvdnav([[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue]);
    /* Init a separate instance of libhb for user scanning and setting up jobs */
    fQueueEncodeLibhb = hb_init(loggingLevel, 0);
    
	// Set the Growl Delegate
    [GrowlApplicationBridge setGrowlDelegate: self];
    /* Init others controllers */
    [fPictureController SetHandle: fHandle];
    [fPictureController   setHBController: self];
    
    [fQueueController   setHandle: fQueueEncodeLibhb];
    [fQueueController   setHBController: self];

    fChapterTitlesDelegate = [[ChapterTitles alloc] init];
    [fChapterTable setDataSource:fChapterTitlesDelegate];
    [fChapterTable setDelegate:fChapterTitlesDelegate];
    
    /* setup the subtitles delegate and connections to table */
    fSubtitlesDelegate = [[HBSubtitles alloc] init];
    [fSubtitlesTable setDataSource:fSubtitlesDelegate];
    [fSubtitlesTable setDelegate:fSubtitlesDelegate];
    [fSubtitlesTable setRowHeight:25.0];
    
	/* setup the audio controller */
	[fAudioDelegate setHBController: self];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(autoSetM4vExtension:) name: HBMixdownChangedNotification object: nil];

    [fPresetsOutlineView setAutosaveName:@"Presets View"];
    [fPresetsOutlineView setAutosaveExpandedItems:YES];
    
    dockIconProgress = 0;
    
    /* Init QueueFile .plist */
    [self loadQueueFile];
    /* Run hbInstances to get any info on other instances as well as set the
     * pid number for this instance in the case of multi-instance encoding. */ 
    hbInstanceNum = [self hbInstances];
    
    /* If we are a single instance it is safe to clean up the previews if there are any
     * left over. This is a bit of a kludge but will prevent a build up of old instance
     * live preview cruft. No danger of removing an active preview directory since they
     * are created later in HBPreviewController if they don't exist at the moment a live
     * preview encode is initiated. */
    if (hbInstanceNum == 1)
    {
        NSString *PreviewDirectory = [NSString stringWithFormat:@"~/Library/Application Support/HandBrake/Previews"];
        PreviewDirectory = [PreviewDirectory stringByExpandingTildeInPath];
        NSError *error;
        NSArray *files = [ [NSFileManager defaultManager]  contentsOfDirectoryAtPath: PreviewDirectory error: &error ];
        for( NSString *file in files ) 
        {
            if( file != @"." && file != @".." ) 
            {
                [ [NSFileManager defaultManager] removeItemAtPath: [ PreviewDirectory stringByAppendingPathComponent: file ] error: &error ];
                if( error ) 
                { 
                    //an error occurred
                    [self writeToActivityLog: "Could not remove existing preview at : %s",[file UTF8String] ];
                }
            }    
        }
        
    }
    
     
    
    /* Call UpdateUI every 1/2 sec */
    
    [[NSRunLoop currentRunLoop] addTimer:[NSTimer
                                          scheduledTimerWithTimeInterval:0.5 
                                          target:self
                                          selector:@selector(updateUI:) 
                                          userInfo:nil repeats:YES]
                                          forMode:NSDefaultRunLoopMode];
                             

    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showDebugOutputPanel:nil];

    // Open queue window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
        [self showQueueWindow:nil];

	[self openMainWindow:nil];
    
    /* We have to set the bool to tell hb what to do after a scan
     * Initially we set it to NO until we start processing the queue
     */
     applyQueueToScan = NO;
    
    // We try to get the list of filenames that may have been drag & drop before the application was running
    id dragDropFilesId = [[NSUserDefaults standardUserDefaults] objectForKey:dragDropFiles];
    
    /* Now we re-check the queue array to see if there are
     * any remaining encodes to be done in it and ask the
     * user if they want to reload the queue */
    if ([QueueFileArray count] > 0)
	{
        /* run  getQueueStats to see whats in the queue file */
        [self getQueueStats];
        /* this results in these values
         * fEncodingQueueItem = 0;
         * fPendingCount = 0;
         * fCompletedCount = 0;
         * fCanceledCount = 0;
         * fWorkingCount = 0;
         */
        
        /*On Screen Notification*/
        NSString * alertTitle;
        
        /* We check to see if there is already another instance of hb running.
         * Note: hbInstances == 1 means we are the only instance of HandBrake.app
         */
        if (hbInstanceNum > 1)
        {
            alertTitle = [NSString stringWithFormat:
                          NSLocalizedString(@"There is already an instance of HandBrake running.", @"")];
            NSBeginCriticalAlertSheet(
                                      alertTitle,
                                      NSLocalizedString(@"Reload Queue", nil),
                                      nil,
                                      nil,
                                      fWindow, self,
                                      nil, @selector(didDimissReloadQueue:returnCode:contextInfo:), nil,
                                      NSLocalizedString(@" HandBrake will now load up the existing queue.", nil));    
        }
        else
        {
            if (fWorkingCount > 0 || fPendingCount > 0)
            {
                if (fWorkingCount > 0)
                {
                    alertTitle = [NSString stringWithFormat:
                                  NSLocalizedString(@"HandBrake Has Detected %d Previously Encoding Item(s) and %d Pending Item(s) In Your Queue.", @""),
                                  fWorkingCount,fPendingCount];
                }
                else
                {
                    alertTitle = [NSString stringWithFormat:
                                  NSLocalizedString(@"HandBrake Has Detected %d Pending Item(s) In Your Queue.", @""),
                                  fPendingCount];
                }
                
                NSBeginCriticalAlertSheet(
                                          alertTitle,
                                          NSLocalizedString(@"Reload Queue", nil),
                                          nil,
                                          NSLocalizedString(@"Empty Queue", nil),
                                          fWindow, self,
                                          nil, @selector(didDimissReloadQueue:returnCode:contextInfo:), nil,
                                          NSLocalizedString(@" Do you want to reload them ?", nil));
                
                // After handling the previous queue (reload or empty), if there is files waiting for scanning
                // we will process them
                if (dragDropFilesId)
                {
                    NSArray *dragDropFiles = (NSArray *)dragDropFilesId;
                    [self openFiles:dragDropFiles];
                }
            }
            else
            {
                // We will open the source window only if there is no dropped files waiting to be scanned
                if (dragDropFilesId)
                {
                    NSArray *dragDropFiles = (NSArray *)dragDropFilesId;
                    [self openFiles:dragDropFiles];
                }
                else
                {
                    /* Since we addressed any pending or previously encoding items above, we go ahead and make sure
                     * the queue is empty of any finished items or cancelled items */
                    [self clearQueueAllItems];
                    /* We show whichever open source window specified in LaunchSourceBehavior preference key */
                    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
                    {
                        [self browseSources:nil];
                    }
                
                    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source (Title Specific)"])
                    {
                        [self browseSources:(id)fOpenSourceTitleMMenu];
                    }
                }
            }
            
        }
    }
    // We will open the source window only if there is no dropped files waiting to be scanned
    else if (dragDropFilesId)
    {
        NSArray *dragDropFiles = (NSArray *)dragDropFilesId;
        [self openFiles:dragDropFiles];
    }
    else
    {
        /* We show whichever open source window specified in LaunchSourceBehavior preference key */
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
        {
            [self browseSources:nil];
        }
        
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source (Title Specific)"])
        {
            [self browseSources:(id)fOpenSourceTitleMMenu];
        }
    }
    currentQueueEncodeNameString = @"";
}

#pragma mark -
#pragma mark Multiple Instances

/* hbInstances checks to see if other instances of HB are running and also sets the pid for this instance for multi-instance queue encoding */
 
 /* Note for now since we are in early phases of multi-instance I have put in quite a bit of logging. Can be removed as we see fit. */
- (int) hbInstances
{
    /* check to see if another instance of HandBrake.app is running */
    NSArray *runningAppDictionaries = [[NSWorkspace sharedWorkspace] launchedApplications];
    NSDictionary *runningAppsDictionary;
    int hbInstances = 0;
    NSString * thisInstanceAppPath = [[NSBundle mainBundle] bundlePath];
    NSString * runningInstanceAppPath;
    int runningInstancePidNum;
    [self writeToActivityLog: "hbInstances path to this instance: %s", [thisInstanceAppPath UTF8String]];
    for (runningAppsDictionary in runningAppDictionaries)
	{
        if ([[runningAppsDictionary valueForKey:@"NSApplicationName"] isEqualToString:@"HandBrake"])
		{
            /*Report the path to each active instances app path */
            runningInstancePidNum = [[runningAppsDictionary valueForKey:@"NSApplicationProcessIdentifier"] intValue];
            runningInstanceAppPath = [runningAppsDictionary valueForKey:@"NSApplicationPath"];
            [self writeToActivityLog: "hbInstance found instance pidnum:%d at path: %s", runningInstancePidNum, [runningInstanceAppPath UTF8String]];
            /* see if this is us by comparing the app path */
            if ([runningInstanceAppPath isEqualToString: thisInstanceAppPath])
            {
                /* If so this is our pidnum */
                [self writeToActivityLog: "hbInstance MATCH FOUND, our pidnum is:%d", runningInstancePidNum];
                /* Get the PID number for this hb instance, used in multi instance encoding */
                pidNum = runningInstancePidNum;
                /* Report this pid to the activity log */
                [self writeToActivityLog: "Pid for this instance:%d", pidNum];
                /* Tell fQueueController what our pidNum is */
                [fQueueController setPidNum:pidNum];
            }
            hbInstances++;
        }
    }
    return hbInstances;
}

- (int) getPidnum
{
    return pidNum;
}

#pragma mark -
#pragma mark Drag & drop handling

// This method is used by OSX to know what kind of files can be drag & drop on the NSWindow
// We only want filenames (and so folders too)
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard = [sender draggingPasteboard];
    
    if ([[pboard types] containsObject:NSFilenamesPboardType])
    {
        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        return paths.count == 1 ? NSDragOperationGeneric : NSDragOperationNone;
    }
    
    return NSDragOperationNone;
}

// This method is doing the job after the drag & drop operation has been validated by [self draggingEntered] and OSX
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard;
    
    pboard = [sender draggingPasteboard];
    
    if ([[pboard types] containsObject:NSFilenamesPboardType])
    {
        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        
        if (paths.count > 0)
        {
            // For now, we just want to accept one file at a time
            // If for any reason, more than one file is submitted, we will take the first one
            NSArray *reducedPaths = [NSArray arrayWithObject:[paths objectAtIndex:0]];
            paths = reducedPaths;
        }
        
        [self openFiles:paths];
    }
    
    return YES;
}

#pragma mark -

- (void) didDimissReloadQueue: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    
    [self writeToActivityLog: "didDimissReloadQueue number of hb instances:%d", hbInstanceNum];
    if (returnCode == NSAlertOtherReturn)
    {
        [self writeToActivityLog: "didDimissReloadQueue NSAlertOtherReturn Chosen"];
        [self clearQueueAllItems];
        
        /* We show whichever open source window specified in LaunchSourceBehavior preference key */
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
        {
            [self browseSources:nil];
        }
        
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source (Title Specific)"])
        {
            [self browseSources:(id)fOpenSourceTitleMMenu];
        }
    }
    else
    {
        [self writeToActivityLog: "didDimissReloadQueue First Button Chosen"];
        if (hbInstanceNum == 1)
        {
            
            [self setQueueEncodingItemsAsPending];
        }
        [self showQueueWindow:NULL];
    }
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) app
{

    
    hb_state_t s;
    hb_get_state( fQueueEncodeLibhb, &s );
    
    if ( s.state != HB_STATE_IDLE )
    {
        int result = NSRunCriticalAlertPanel(
                                             NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                                             NSLocalizedString(@"If you quit HandBrake your current encode will be reloaded into your queue at next launch. Do you want to quit anyway?", nil),
                                             NSLocalizedString(@"Quit", nil), NSLocalizedString(@"Don't Quit", nil), nil, @"A movie" );
        
        if (result == NSAlertDefaultReturn)
        {
            return NSTerminateNow;
        }
        else
            return NSTerminateCancel;
    }
    
    // Warn if items still in the queue
    else if ( fPendingCount > 0 )
    {
        int result = NSRunCriticalAlertPanel(
                                             NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                                             NSLocalizedString(@"There are pending encodes in your queue. Do you want to quit anyway?",nil),
                                             NSLocalizedString(@"Quit", nil), NSLocalizedString(@"Don't Quit", nil), nil);
        
        if ( result == NSAlertDefaultReturn )
            return NSTerminateNow;
        else
            return NSTerminateCancel;
    }
    
    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // When the application is closed and we still have some files in the dragDropFiles array
    // it's highly probable that the user throw a lot of files and just want to reset this
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:dragDropFiles];
    
    [currentQueueEncodeNameString release];
    [browsedSourceDisplayName release];
    [outputPanel release];
	[fQueueController release];
    [fPreviewController release];
    [fPictureController release];
    [fApplicationIcon release];

    hb_close(&fHandle);
    hb_close(&fQueueEncodeLibhb);
    hb_global_close();

}


- (void) awakeFromNib
{
    [fWindow center];
    [fWindow setExcludedFromWindowsMenu:NO];
    
    [fAdvancedOptions setView:fAdvancedView];
    
    /* lets setup our presets drawer for drag and drop here */
    [fPresetsOutlineView registerForDraggedTypes: [NSArray arrayWithObject:DragDropSimplePboardType] ];
    [fPresetsOutlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [fPresetsOutlineView setVerticalMotionCanBeginDrag: YES];
    
    /* Initialize currentScanCount so HB can use it to
     evaluate successive scans */
	currentScanCount = 0;
    
    
    /* Init UserPresets .plist */
	[self loadPresets];
	
    fRipIndicatorShown = NO;  // initially out of view in the nib
    
    /* For 64 bit builds, the threaded animation in the progress
     * indicators conflicts with the animation in the advanced tab
     * for reasons not completely clear. jbrjake found a note in the
     * 10.5 dev notes regarding this possiblility. It was also noted
     * that unless specified, setUsesThreadedAnimation defaults to true.
     * So, at least for now we set the indicator animation to NO for
     * both the scan and regular progress indicators for both 32 and 64 bit
     * as it test out fine on both and there is no reason our progress indicators
     * should require their own thread.
     */

    [fScanIndicator setUsesThreadedAnimation:NO];
    [fRipIndicator setUsesThreadedAnimation:NO];
  
    
    
	/* Show/Dont Show Presets drawer upon launch based
     on user preference DefaultPresetsDrawerShow*/
	if( [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPresetsDrawerShow"] > 0 )
	{
        [fPresetDrawer setDelegate:self];
        NSSize drawerSize = NSSizeFromString( [[NSUserDefaults standardUserDefaults] 
                                               stringForKey:@"Drawer Size"] );
        if( drawerSize.width )
            [fPresetDrawer setContentSize: drawerSize];
		[fPresetDrawer open];
	}
    
    /* Initially set the dvd angle widgets to hidden (dvdnav only) */
    [fSrcAngleLabel setHidden:YES];
    [fSrcAnglePopUp setHidden:YES];
    
    /* Setup the start / stop popup */
    [fEncodeStartStopPopUp removeAllItems];
    [fEncodeStartStopPopUp addItemWithTitle: @"Chapters"];
    [fEncodeStartStopPopUp addItemWithTitle: @"Seconds"];
    [fEncodeStartStopPopUp addItemWithTitle: @"Frames"];
    /* Align the start / stop widgets with the chapter popups */
    [fSrcTimeStartEncodingField setFrameOrigin:[fSrcChapterStartPopUp frame].origin];
    [fSrcTimeEndEncodingField setFrameOrigin:[fSrcChapterEndPopUp frame].origin];
    
    [fSrcFrameStartEncodingField setFrameOrigin:[fSrcChapterStartPopUp frame].origin];
    [fSrcFrameEndEncodingField setFrameOrigin:[fSrcChapterEndPopUp frame].origin];
    
    /* Destination box*/
    NSMenuItem *menuItem;
    [fDstFormatPopUp removeAllItems];
    // MP4 file
    menuItem = [[fDstFormatPopUp menu] addItemWithTitle:@"MP4 file" action: NULL keyEquivalent: @""];
    [menuItem setTag: HB_MUX_MP4];
	// MKV file
    menuItem = [[fDstFormatPopUp menu] addItemWithTitle:@"MKV file" action: NULL keyEquivalent: @""];
    [menuItem setTag: HB_MUX_MKV];
    
    [fDstFormatPopUp selectItemAtIndex: 0];
    
    [self formatPopUpChanged:nil];
    
	/* We enable the create chapters checkbox here since we are .mp4 */
	[fCreateChapterMarkers setEnabled: YES];
	if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultChapterMarkers"] > 0)
	{
		[fCreateChapterMarkers setState: NSOnState];
	}
    
    
    
    
    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                     @"%@/Desktop/Movie.mp4", NSHomeDirectory()]];
    
    /* Video encoder */
    [fVidEncoderPopUp removeAllItems];
    [fVidEncoderPopUp addItemWithTitle: @"FFmpeg"];
    
    /* setup our x264 presets widgets - this only needs to be done once */
    [self setupX264PresetsWidgets: nil];
    
    /* Video quality */
	[fVidBitrateField    setIntValue: 1000];
    [fVidQualityMatrix   selectCell: fVidBitrateCell];
    [self videoMatrixChanged:nil];
    
    /* Video framerate */
    [fVidRatePopUp removeAllItems];
	[fVidRatePopUp addItemWithTitle: NSLocalizedString( @"Same as source", @"" )];
    for( int i = 0; i < hb_video_rates_count; i++ )
    {
        if ([[NSString stringWithUTF8String: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%.3f",23.976]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithUTF8String: hb_video_rates[i].string], @" (NTSC Film)"]];
		}
		else if ([[NSString stringWithUTF8String: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%d",25]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithUTF8String: hb_video_rates[i].string], @" (PAL Film/Video)"]];
		}
		else if ([[NSString stringWithUTF8String: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%.2f",29.97]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithUTF8String: hb_video_rates[i].string], @" (NTSC Video)"]];
		}
		else
		{
			[fVidRatePopUp addItemWithTitle:
             [NSString stringWithUTF8String: hb_video_rates[i].string]];
		}
    }
    [fVidRatePopUp selectItemAtIndex: 0];
	
	/* Set Auto Crop to On at launch */
    [fPictureController setAutoCrop:YES];
		
    /* Bottom */
    [fStatusField setStringValue: @""];
    
    [self enableUI: NO];
	[self setupToolbar];
    
	/* We disable the Turbo 1st pass checkbox since we are not x264 */
	[fVidTurboPassCheck setEnabled: NO];
	[fVidTurboPassCheck setState: NSOffState];
    
    /* Auto Passthru advanced options box */
    [fAudioAutoPassthruBox setHidden:NO];
    
    
	/* lets get our default prefs here */
	[self getDefaultPresets:nil];
	/* lets initialize the current successful scancount here to 0 */
	currentSuccessfulScanCount = 0;
    
    /* Register HBController's Window as a receiver for files/folders drag & drop operations */
    [fWindow registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
}

- (void) enableUI: (bool) b
{
    NSControl * controls[] =
    {
        fSrcTitleField, fSrcTitlePopUp,
        fSrcChapterField, fSrcChapterStartPopUp, fSrcChapterToField,
        fSrcChapterEndPopUp, fSrcDuration1Field, fSrcDuration2Field,
        fDstFormatField, fDstFormatPopUp, fDstFile1Field, fDstFile2Field,
        fDstBrowseButton, fVidRateField, fVidRatePopUp, fVidEncoderField,
        fVidEncoderPopUp, fVidQualityField, fVidQualityMatrix,
        fPictureSettingsField, fPictureFiltersField,
        fSubField, fSubPopUp, fPresetsAdd, fPresetsDelete, fSrcAngleLabel,
        fSrcAnglePopUp, fCreateChapterMarkers, fVidTurboPassCheck,
        fDstMp4LargeFileCheck, fSubForcedCheck, fPresetsOutlineView,
        fDstMp4HttpOptFileCheck, fDstMp4iPodFileCheck, fVidQualityRFField,
        fVidQualityRFLabel, fEncodeStartStopPopUp, fSrcTimeStartEncodingField,
        fSrcTimeEndEncodingField, fSrcFrameStartEncodingField,
        fSrcFrameEndEncodingField, fLoadChaptersButton, fSaveChaptersButton,
        fFramerateMatrix,
        
    };
    for (unsigned i = 0; i < (sizeof(controls) / sizeof(NSControl*)); i++)
    {
        if ([[controls[i] className] isEqualToString: @"NSTextField"])
        {
            NSTextField *tf = (NSTextField*)controls[i];
            if (![tf isBezeled])
            {
                [tf setTextColor: (b ?
                                   [NSColor controlTextColor] :
                                   [NSColor disabledControlTextColor])];
                continue;
            }
        }
        [controls[i] setEnabled: b];
    }
   
    
	if (b) 
    {
        
        /* we also call calculatePictureSizing here to sense check if we already have vfr selected */
        [self calculatePictureSizing:nil];
        /* Also enable the preview window hud controls */
        [fPictureController enablePreviewHudControls];
    }
    else 
    {
        
		[fPresetsOutlineView setEnabled: NO];
        [fPictureController disablePreviewHudControls];
    }
    
    [self videoMatrixChanged:nil];
    [self enableX264Widgets:b];
}


/***********************************************************************
 * updateDockIcon
 ***********************************************************************
 * Updates two DockTextFields on the dockTile,
 * one with total percentage, the other one with the ETA.
 * The ETA string is formated by the callers
 **********************************************************************/
- (void) updateDockIcon: (double) progress withETA:(NSString*)etaStr
{
    if (progress < 0.0 || progress > 1.0)
    {
        [percentField setHidden:YES];
        [timeField setHidden:YES];
    }
    else
    {
        [percentField setTextToDisplay:[NSString stringWithFormat:dockTilePercentFormat,progress * 100]];
        [percentField setHidden:NO];
        [timeField setTextToDisplay:etaStr];
        [timeField setHidden:NO];
    }
    
    [dockTile display];
}

- (void) updateUI: (NSTimer *) timer
{
    
    /* Update UI for fHandle (user scanning instance of libhb ) */
    
    hb_list_t  * list;
    list = hb_get_titles( fHandle );
    /* check to see if there has been a new scan done
     this bypasses the constraints of HB_STATE_WORKING
     not allowing setting a newly scanned source */
	int checkScanCount = hb_get_scancount( fHandle );
	if( checkScanCount > currentScanCount )
	{
		currentScanCount = checkScanCount;
        [fScanIndicator setIndeterminate: NO];
        [fScanIndicator setDoubleValue: 0.0];
        [fScanIndicator setHidden: YES];
        [fScanHorizontalLine setHidden: NO];
        
		[self showNewScan:nil];
	}
    
    hb_state_t s;
    hb_get_state( fHandle, &s );
    
    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;
#define p s.param.scanning
        case HB_STATE_SCANNING:
		{
            if( p.preview_cur )
            {
                [fSrcDVD2Field setStringValue: [NSString stringWithFormat:
                                                NSLocalizedString( @"Scanning title %d of %d, preview %d…", @"" ),
                                                p.title_cur, p.title_count,
                                                p.preview_cur]];
            }
            else
            {
                [fSrcDVD2Field setStringValue: [NSString stringWithFormat:
                                                NSLocalizedString( @"Scanning title %d of %d…", @"" ),
                                                p.title_cur, p.title_count]];
            }
            [fScanIndicator setHidden: NO];
            [fScanHorizontalLine setHidden: YES];
            [fScanIndicator setDoubleValue: 100.0 * p.progress];
            break;
		}
#undef p
            
#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
            [fScanIndicator setIndeterminate: NO];
            [fScanIndicator setDoubleValue: 0.0];
            [fScanIndicator setHidden: YES];
            [fScanHorizontalLine setHidden: NO];
			[self writeToActivityLog:"ScanDone state received from fHandle"];
            [self showNewScan:nil];
            [[fWindow toolbar] validateVisibleItems];
            
			break;
        }
#undef p
            
#define p s.param.working
        case HB_STATE_WORKING:
        {
            
            break;
        }
#undef p
            
#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            
            break;
        }
#undef p
            
        case HB_STATE_PAUSED:
            break;
            
        case HB_STATE_WORKDONE:
        {
            break;
        }
    }
    
    
    /* Update UI for fQueueEncodeLibhb */
    // hb_list_t  * list;
    // list = hb_get_titles( fQueueEncodeLibhb ); //fQueueEncodeLibhb
    /* check to see if there has been a new scan done
     this bypasses the constraints of HB_STATE_WORKING
     not allowing setting a newly scanned source */
	
    checkScanCount = hb_get_scancount( fQueueEncodeLibhb );
	if( checkScanCount > currentScanCount )
	{
		currentScanCount = checkScanCount;
	}
    
    //hb_state_t s;
    hb_get_state( fQueueEncodeLibhb, &s );
    
    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;
#define p s.param.scanning
        case HB_STATE_SCANNING:
		{
            NSString *scan_status;
            if( p.preview_cur )
            {
                scan_status = [NSString stringWithFormat:
                               NSLocalizedString( @"Queue Scanning title %d of %d, preview %d…", @"" ),
                               p.title_cur, p.title_count, p.preview_cur];
            }
            else
            {
                scan_status = [NSString stringWithFormat:
                               NSLocalizedString( @"Queue Scanning title %d of %d…", @"" ),
                               p.title_cur, p.title_count];
            }
            [fStatusField setStringValue: scan_status];
            
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: scan_status];
            break;
		}
#undef p
            
#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
			[self writeToActivityLog:"ScanDone state received from fQueueEncodeLibhb"];
            [self processNewQueueEncode];
            [[fWindow toolbar] validateVisibleItems];
            
			break;
        }
#undef p
            
            
#define p s.param.working
            
        case HB_STATE_SEARCHING:
		{
            NSMutableString * string;
            NSString * pass_desc;
            
            /* Update text field */
            pass_desc = @"";
            //string = [NSMutableString stringWithFormat:
            //          NSLocalizedString( @"Searching for start point: pass %d %@ of %d, %.2f %%", @"" ),
            //          p.job_cur, pass_desc, p.job_count, 100.0 * p.progress];
            /* For now, do not announce "pass x of x for the search phase */
            string = [NSMutableString stringWithFormat:
                      NSLocalizedString( @"Searching for start point… :  %.2f %%", @"" ),
                      100.0 * p.progress];
            
            if( p.seconds > -1 )
            {
                [string appendFormat:
                 NSLocalizedString( @" (ETA %02dh%02dm%02ds)", @"" ),
                 p.hours, p.minutes, p.seconds];
            }
            
            [fStatusField setStringValue: string];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: string];
            /* Update slider */
            CGFloat progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue:100.0 * progress_total];
            
            // If progress bar hasn't been revealed at the bottom of the window, do
            // that now. This code used to be in doRip. I moved it to here to handle
            // the case where hb_start is called by HBQueueController and not from
            // HBController.
            if( !fRipIndicatorShown )
            {
                NSRect frame = [fWindow frame];
                if( frame.size.width <= 591 )
                    frame.size.width = 591;
                frame.size.height += 36;
                frame.origin.y -= 36;
                [fWindow setFrame:frame display:YES animate:YES];
                fRipIndicatorShown = YES;
                
            }
            
            /* Update dock icon */
            /* Note not done yet */
            break;  
        }
            
            
        case HB_STATE_WORKING:
        {
            NSMutableString * string;
            NSString * pass_desc;
			/* Update text field */
            if (p.job_cur == 1 && p.job_count > 1)
            {
                if ([[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SubtitleList"] && [[[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex]objectForKey:@"SubtitleList"] objectAtIndex:0] objectForKey:@"subtitleSourceTrackNum"] intValue] == 1)
                {
                    pass_desc = @"(subtitle scan)";   
                }
                else
                {
                    pass_desc = @"";
                }
            }
            else
            {
                pass_desc = @"";
            }
            
            
            if ([pass_desc length])
            {
                string = [NSMutableString stringWithFormat:
                          NSLocalizedString( @"Encoding: %@ \nPass %d %@ of %d, %.2f %%", @"" ),
                          currentQueueEncodeNameString,
                          p.job_cur, pass_desc, p.job_count, 100.0 * p.progress];
            }
            else
            {
                string = [NSMutableString stringWithFormat:
                          NSLocalizedString( @"Encoding: %@ \nPass %d of %d, %.2f %%", @"" ),
                          currentQueueEncodeNameString,
                          p.job_cur, p.job_count, 100.0 * p.progress];
            }
            
            if( p.seconds > -1 )
            {
                if ( p.rate_cur > 0.0 )
                {
                    [string appendFormat:
                     NSLocalizedString( @" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", @"" ),
                     p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
                }
                else
                {
                    [string appendFormat:
                     NSLocalizedString( @" (ETA %02dh%02dm%02ds)", @"" ),
                     p.hours, p.minutes, p.seconds];
                }
            }
            [fStatusField setStringValue: string];
            [fQueueController setQueueStatusString:string];
            
            /* Update slider */
            CGFloat progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue:100.0 * progress_total];
            
            // If progress bar hasn't been revealed at the bottom of the window, do
            // that now. This code used to be in doRip. I moved it to here to handle
            // the case where hb_start is called by HBQueueController and not from
            // HBController.
            if( !fRipIndicatorShown )
            {
                NSRect frame = [fWindow frame];
                if( frame.size.width <= 591 )
                    frame.size.width = 591;
                frame.size.height += 36;
                frame.origin.y -= 36;
                [fWindow setFrame:frame display:YES animate:YES];
                fRipIndicatorShown = YES;
                
            }
            
            /* Update dock icon */
            if( dockIconProgress < 100.0 * progress_total )
            {
                // ETA format is [XX]X:XX:XX when ETA is greater than one hour
                // [X]X:XX when ETA is greater than 0 (minutes or seconds)
                // When these conditions doesn't applied (eg. when ETA is undefined)
                // we show just a tilde (~)
                
                NSString *etaStr = @"";
                if (p.hours > 0)
                    etaStr = [NSString stringWithFormat:@"%d:%02d:%02d", p.hours, p.minutes, p.seconds];
                else if (p.minutes > 0 || p.seconds > 0)
                    etaStr = [NSString stringWithFormat:@"%d:%02d", p.minutes, p.seconds];
                else
                    etaStr = @"~";
                
                [self updateDockIcon:progress_total withETA:etaStr];

                dockIconProgress += dockTileUpdateFrequency;
            }
            
            break;
        }
#undef p
            
#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            /* Update text field */
            [fStatusField setStringValue: NSLocalizedString( @"Muxing…", @"" )];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: NSLocalizedString( @"Muxing…", @"" )];
            /* Update slider */
            [fRipIndicator setIndeterminate: YES];
            [fRipIndicator startAnimation: nil];
            
            /* Update dock icon */
            [self updateDockIcon:1.0 withETA:@""];
            
			break;
        }
#undef p
            
        case HB_STATE_PAUSED:
		    [fStatusField setStringValue: NSLocalizedString( @"Paused", @"" )];
            [fQueueController setQueueStatusString: NSLocalizedString( @"Paused", @"" )];
            
			break;
            
        case HB_STATE_WORKDONE:
        {
            // HB_STATE_WORKDONE happpens as a result of libhb finishing all its jobs
            // or someone calling hb_stop. In the latter case, hb_stop does not clear
            // out the remaining passes/jobs in the queue. We'll do that here.
            
            // Delete all remaining jobs of this encode.
            [fStatusField setStringValue: NSLocalizedString( @"Encode Finished.", @"" )];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: NSLocalizedString( @"Encode Finished.", @"" )];
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator stopAnimation: nil];
            [fRipIndicator setDoubleValue: 0.0];
            [[fWindow toolbar] validateVisibleItems];
            
            /* Restore dock icon */
            [self updateDockIcon:-1.0 withETA:@""];
            dockIconProgress = 0;
            
            if( fRipIndicatorShown )
            {
                NSRect frame = [fWindow frame];
                if( frame.size.width <= 591 )
				    frame.size.width = 591;
                frame.size.height += -36;
                frame.origin.y -= -36;
                [fWindow setFrame:frame display:YES animate:YES];
				fRipIndicatorShown = NO;
			}
            /* Since we are done with this encode, tell output to stop writing to the
             * individual encode log
             */
			[outputPanel endEncodeLog];
            /* Check to see if the encode state has not been cancelled
             to determine if we should check for encode done notifications */
			if( fEncodeState != 2 )
            {
                NSString *pathOfFinishedEncode;
                /* Get the output file name for the finished encode */
                pathOfFinishedEncode = [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"DestinationPath"];
                
                /* Both the Growl Alert and Sending to MetaX can be done as encodes roll off the queue */
                if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Growl Notification"] || 
                    [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
                {
                    /* If Play System Alert has been selected in Preferences */
                    if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertWhenDoneSound"] == YES )
                    {
                        NSBeep();
                    }
                    [self showGrowlDoneNotification:pathOfFinishedEncode];
                }
                
                /* Send to MetaX */
                [self sendToMetaX:pathOfFinishedEncode];
                
                /* since we have successfully completed an encode, we increment the queue counter */
                [self incrementQueueItemDone:currentQueueEncodeIndex]; 

            }
            
            break;
        }
    }
    
    /* Since we can use multiple instance off of the same queue file it is imperative that we keep the QueueFileArray updated off of the QueueFile.plist
     * so we go ahead and do it in this existing timer as opposed to using a new one */
    
    NSMutableArray * tempQueueArray = [[NSMutableArray alloc] initWithContentsOfFile:QueueFile];
    [QueueFileArray setArray:tempQueueArray];
    [tempQueueArray release]; 
    /* Send Fresh QueueFileArray to fQueueController to update queue window */
    [fQueueController setQueueArray: QueueFileArray];
    [self getQueueStats];
    
    /* Update the visibility of the Auto Passthru advanced options box */
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"ShowAdvancedOptsForAutoPassthru"] == YES)
    {
        [fAudioAutoPassthruBox setHidden:NO];
    }
    else
    {
        [fAudioAutoPassthruBox setHidden:YES];
    }
 
    
    // Finally after all UI updates, we look for a next dragDropItem to scan
    // fWillScan will signal that a scan will be launched, so we need to wait
    // the next idle cycle after the scan
    hb_get_state( fHandle, &s );
    if (s.state == HB_STATE_IDLE && !fWillScan)
    {
        // Continue to loop on the other drag & drop files if any
        if ([[NSUserDefaults standardUserDefaults] objectForKey:dragDropFiles])
        {
            NSMutableArray *filesList = [[NSMutableArray alloc] initWithArray:[[NSUserDefaults standardUserDefaults] objectForKey:dragDropFiles]];
        
            if (filesList.count > 0)
            {
                // We need to add the previous scan file into the queue (without doing the usual checks)
                // Before scanning the new one
                [self doAddToQueue];
                
                id nextItem = [filesList objectAtIndex:0];
                [filesList removeObjectAtIndex:0];
            
                [browsedSourceDisplayName release];
                browsedSourceDisplayName = [[((NSString*)nextItem) lastPathComponent] retain];
                [self performScan:nextItem scanTitleNum:0];
            
                if (filesList.count > 0)
                {
                    // Updating the list in the user defaults
                    [[NSUserDefaults standardUserDefaults] setObject:filesList forKey:dragDropFiles];
                }
                else
                {
                    // Cleaning if last one was treated
                    [[NSUserDefaults standardUserDefaults] removeObjectForKey:dragDropFiles];
                }
            }
            else
            {
                [[NSUserDefaults standardUserDefaults] removeObjectForKey:dragDropFiles];
            }
        }
    }
}

/* We use this to write messages to stderr from the macgui which show up in the activity window and log*/
- (void) writeToActivityLog:(const char *) format, ...
{
    va_list args;
    va_start(args, format);
    if (format != nil)
    {
        char str[1024];
        vsnprintf( str, 1024, format, args );

        time_t _now = time( NULL );
        struct tm * now  = localtime( &_now );
        fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, str );
    }
    va_end(args);
}

#pragma mark -
#pragma mark Toolbar
// ============================================================
// NSToolbar Related Methods
// ============================================================

- (void) setupToolbar {
    NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier: @"HandBrake Toolbar"] autorelease];

    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];

    [toolbar setDelegate: self];

    [fWindow setToolbar: toolbar];
}

- (NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier:
    (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted {
    NSToolbarItem * item = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdent] autorelease];

    if ([itemIdent isEqualToString: ToggleDrawerIdentifier])
    {
        [item setLabel: @"Toggle Presets"];
        [item setPaletteLabel: @"Toggler Presets"];
        [item setToolTip: @"Open/Close Preset Drawer"];
        [item setImage: [NSImage imageNamed: @"presets"]];
        [item setTarget: self];
        [item setAction: @selector(toggleDrawer:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: StartEncodingIdentifier])
    {
        [item setLabel: @"Start"];
        [item setPaletteLabel: @"Start Encoding"];
        [item setToolTip: @"Start Encoding"];
        [item setImage: [NSImage imageNamed: @"encode"]];
        [item setTarget: self];
        [item setAction: @selector(Rip:)];
    }
    else if ([itemIdent isEqualToString: ShowQueueIdentifier])
    {
        [item setLabel: @"Show Queue"];
        [item setPaletteLabel: @"Show Queue"];
        [item setToolTip: @"Show Queue"];
        [item setImage: [NSImage imageNamed: @"showqueue"]];
        [item setTarget: self];
        [item setAction: @selector(showQueueWindow:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: AddToQueueIdentifier])
    {
        [item setLabel: @"Add to Queue"];
        [item setPaletteLabel: @"Add to Queue"];
        [item setToolTip: @"Add to Queue"];
        [item setImage: [NSImage imageNamed: @"addqueue"]];
        [item setTarget: self];
        [item setAction: @selector(addToQueue:)];
    }
    else if ([itemIdent isEqualToString: PauseEncodingIdentifier])
    {
        [item setLabel: @"Pause"];
        [item setPaletteLabel: @"Pause Encoding"];
        [item setToolTip: @"Pause Encoding"];
        [item setImage: [NSImage imageNamed: @"pauseencode"]];
        [item setTarget: self];
        [item setAction: @selector(Pause:)];
    }
    else if ([itemIdent isEqualToString: ShowPictureIdentifier])
    {
        [item setLabel: @"Picture Settings"];
        [item setPaletteLabel: @"Show Picture Settings"];
        [item setToolTip: @"Show Picture Settings"];
        [item setImage: [NSImage imageNamed: @"picturesettings"]];
        [item setTarget: self];
        [item setAction: @selector(showPicturePanel:)];
    }
    else if ([itemIdent isEqualToString: ShowPreviewIdentifier])
    {
        [item setLabel: @"Preview Window"];
        [item setPaletteLabel: @"Show Preview"];
        [item setToolTip: @"Show Preview"];
        //[item setImage: [NSImage imageNamed: @"pref-picture"]];
        [item setImage: [NSImage imageNamed: @"preview"]];
        [item setTarget: self];
        [item setAction: @selector(showPreviewWindow:)];
    }
    else if ([itemIdent isEqualToString: ShowActivityIdentifier]) 
    {
        [item setLabel: @"Activity Window"];
        [item setPaletteLabel: @"Show Activity Window"];
        [item setToolTip: @"Show Activity Window"];
        [item setImage: [NSImage imageNamed: @"activity"]];
        [item setTarget: self];
        [item setAction: @selector(showDebugOutputPanel:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: ChooseSourceIdentifier])
    {
        [item setLabel: @"Source"];
        [item setPaletteLabel: @"Source"];
        [item setToolTip: @"Choose Video Source"];
        [item setImage: [NSImage imageNamed: @"source"]];
        [item setTarget: self];
        [item setAction: @selector(browseSources:)];
    }
    else
    {
        return nil;
    }

    return item;
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects: ChooseSourceIdentifier, NSToolbarSeparatorItemIdentifier, StartEncodingIdentifier,
        PauseEncodingIdentifier, AddToQueueIdentifier, ShowQueueIdentifier, NSToolbarFlexibleSpaceItemIdentifier, 
		NSToolbarSpaceItemIdentifier, ShowPictureIdentifier, ShowPreviewIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier, nil];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects:  StartEncodingIdentifier, PauseEncodingIdentifier, AddToQueueIdentifier,
        ChooseSourceIdentifier, ShowQueueIdentifier, ShowPictureIdentifier, ShowPreviewIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier,
        NSToolbarCustomizeToolbarItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier,
        NSToolbarSpaceItemIdentifier, NSToolbarSeparatorItemIdentifier, nil];
}

- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    NSString * ident = [toolbarItem itemIdentifier];
        
    if (fHandle)
    {
        hb_state_t s;
        
        hb_get_state( fHandle, &s );
        if (s.state == HB_STATE_SCANNING)
        {
            
            if ([ident isEqualToString: ChooseSourceIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
                [toolbarItem setLabel: @"Cancel Scan"];
                [toolbarItem setPaletteLabel: @"Cancel Scanning"];
                [toolbarItem setToolTip: @"Cancel Scanning Source"];
                return YES;
            }
            
            if ([ident isEqualToString: StartEncodingIdentifier] || [ident isEqualToString: AddToQueueIdentifier])
                return NO;
        }
        else
        {
            if ([ident isEqualToString: ChooseSourceIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"source"]];
                [toolbarItem setLabel: @"Source"];
                [toolbarItem setPaletteLabel: @"Source"];
                [toolbarItem setToolTip: @"Choose Video Source"];
                return YES;
            }
        }

        hb_get_state2( fQueueEncodeLibhb, &s );
        
        if (s.state == HB_STATE_WORKING || s.state == HB_STATE_SEARCHING || s.state == HB_STATE_MUXING)
        {
            if ([ident isEqualToString: StartEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
                [toolbarItem setLabel: @"Stop"];
                [toolbarItem setPaletteLabel: @"Stop"];
                [toolbarItem setToolTip: @"Stop Encoding"];
                return YES;
            }
            if ([ident isEqualToString: PauseEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"pauseencode"]];
                [toolbarItem setLabel: @"Pause"];
                [toolbarItem setPaletteLabel: @"Pause Encoding"];
                [toolbarItem setToolTip: @"Pause Encoding"];
                return YES;
            }
            if (SuccessfulScan)
            {
                if ([ident isEqualToString: AddToQueueIdentifier])
                    return YES;
                if ([ident isEqualToString: ShowPictureIdentifier])
                    return YES;
                if ([ident isEqualToString: ShowPreviewIdentifier])
                    return YES;
            }
        }
        else if (s.state == HB_STATE_PAUSED)
        {
            if ([ident isEqualToString: PauseEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
                [toolbarItem setLabel: @"Resume"];
                [toolbarItem setPaletteLabel: @"Resume Encoding"];
                [toolbarItem setToolTip: @"Resume Encoding"];
                return YES;
            }
            if ([ident isEqualToString: StartEncodingIdentifier])
                return YES;
            if ([ident isEqualToString: AddToQueueIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPictureIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPreviewIdentifier])
                return YES;
        }
        else if (s.state == HB_STATE_SCANNING)
            return NO;
        else if (s.state == HB_STATE_WORKDONE || s.state == HB_STATE_SCANDONE || SuccessfulScan)
        {
            if ([ident isEqualToString: StartEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
                if (hb_count(fHandle) > 0)
                    [toolbarItem setLabel: @"Start Queue"];
                else
                    [toolbarItem setLabel: @"Start"];
                [toolbarItem setPaletteLabel: @"Start Encoding"];
                [toolbarItem setToolTip: @"Start Encoding"];
                return YES;
            }
            if ([ident isEqualToString: AddToQueueIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPictureIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPreviewIdentifier])
                return YES;
        }

    }
    /* If there are any pending queue items, make sure the start/stop button is active */
    if ([ident isEqualToString: StartEncodingIdentifier] && fPendingCount > 0)
        return YES;
    if ([ident isEqualToString: ShowQueueIdentifier])
        return YES;
    if ([ident isEqualToString: ToggleDrawerIdentifier])
        return YES;
    if ([ident isEqualToString: ChooseSourceIdentifier])
        return YES;
    if ([ident isEqualToString: ShowActivityIdentifier])
        return YES;
    
    return NO;
}

- (BOOL) validateMenuItem: (NSMenuItem *) menuItem
{
    SEL action = [menuItem action];
    
    hb_state_t s;
    hb_get_state2( fHandle, &s );
    
    if (fHandle)
    {
        if (action == @selector(addToQueue:) || action == @selector(addAllTitlesToQueue:) || action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:))
            return SuccessfulScan && [fWindow attachedSheet] == nil;
        
        if (action == @selector(browseSources:))
        {
            if (s.state == HB_STATE_SCANNING)
                return NO;
            else
                return [fWindow attachedSheet] == nil;
        }
        if (action == @selector(selectDefaultPreset:))
            return [fPresetsOutlineView selectedRow] >= 0 && [fWindow attachedSheet] == nil;
        if (action == @selector(Pause:))
        {
            if (s.state == HB_STATE_WORKING)
            {
                if(![[menuItem title] isEqualToString:@"Pause Encoding"])
                    [menuItem setTitle:@"Pause Encoding"];
                return YES;
            }
            else if (s.state == HB_STATE_PAUSED)
            {
                if(![[menuItem title] isEqualToString:@"Resume Encoding"])
                    [menuItem setTitle:@"Resume Encoding"];
                return YES;
            }
            else
                return NO;
        }
        if (action == @selector(Rip:))
        {
            if (s.state == HB_STATE_WORKING || s.state == HB_STATE_MUXING || s.state == HB_STATE_PAUSED)
            {
                if(![[menuItem title] isEqualToString:@"Stop Encoding"])
                    [menuItem setTitle:@"Stop Encoding"];
                return YES;
            }
            else if (SuccessfulScan)
            {
                if(![[menuItem title] isEqualToString:@"Start Encoding"])
                    [menuItem setTitle:@"Start Encoding"];
                return [fWindow attachedSheet] == nil;
            }
            else
                return NO;
        }
    }
    if( action == @selector(setDefaultPreset:) )
    {
        return [fPresetsOutlineView selectedRow] != -1;
    }

    return YES;
}

#pragma mark -
#pragma mark Encode Done Actions
// register a test notification and make
// it enabled by default
#define SERVICE_NAME @"Encode Done"
- (NSDictionary *)registrationDictionaryForGrowl 
{ 
    NSDictionary *registrationDictionary = [NSDictionary dictionaryWithObjectsAndKeys: 
    [NSArray arrayWithObjects:SERVICE_NAME,nil], GROWL_NOTIFICATIONS_ALL, 
    [NSArray arrayWithObjects:SERVICE_NAME,nil], GROWL_NOTIFICATIONS_DEFAULT, 
    nil]; 

    return registrationDictionary; 
} 

-(void)showGrowlDoneNotification:(NSString *) filePath
{
    /* This end of encode action is called as each encode rolls off of the queue */
    /* Setup the Growl stuff */
    NSString * finishedEncode = filePath;
    /* strip off the path to just show the file name */
    finishedEncode = [finishedEncode lastPathComponent];
    NSString * growlMssg = [NSString stringWithFormat: @"your HandBrake encode %@ is done!",finishedEncode];
    [GrowlApplicationBridge 
     notifyWithTitle:@"Put down that cocktail…" 
     description:growlMssg 
     notificationName:SERVICE_NAME
     iconData:nil 
     priority:0 
     isSticky:1 
     clickContext:nil];
}
-(void)sendToMetaX:(NSString *) filePath
{
    /* This end of encode action is called as each encode rolls off of the queue */
    if([[NSUserDefaults standardUserDefaults] boolForKey: @"sendToMetaX"] == YES)
    {
        NSString *sendToApp = [[NSUserDefaults standardUserDefaults] objectForKey: @"SendCompletedEncodeToApp"];
        if (![sendToApp isEqualToString:@"None"])
        {
            [self writeToActivityLog: "trying to send encode to: %s", [sendToApp UTF8String]];
            NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@%@%@", @"tell application \"",sendToApp,@"\" to open (POSIX file \"", filePath, @"\")"]];
            [myScript executeAndReturnError: nil];
            [myScript release];
        }
        
    }
}

- (void) queueCompletedAlerts
{
    /* If Play System Alert has been selected in Preferences */
    if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertWhenDoneSound"] == YES )
    {
        NSBeep();
    }
    
    /* If Alert Window or Window and Growl has been selected */
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window"] ||
        [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"] )
    {
        /*On Screen Notification*/
        int status;
        status = NSRunAlertPanel(@"Put down that cocktail…",@"Your HandBrake queue is done!", @"OK", nil, nil);
        [NSApp requestUserAttention:NSCriticalRequest];
    }
    
    /* If sleep has been selected */
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"] )
    {
        /* Sleep */
        NSDictionary* errorDict;
        NSAppleEventDescriptor* returnDescriptor = nil;
        NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to sleep"];
        returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
    /* If Shutdown has been selected */
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"] )
    {
        /* Shut Down */
        NSDictionary* errorDict;
        NSAppleEventDescriptor* returnDescriptor = nil;
        NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to shut down"];
        returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
}

#pragma mark -
#pragma mark Get New Source

/*Opens the source browse window, called from Open Source widgets */
- (IBAction) browseSources: (id) sender
{
    
    hb_state_t s;
    hb_get_state( fHandle, &s );
    if (s.state == HB_STATE_SCANNING)
    {
        [self cancelScanning:nil];
        return;
    }
    
    
    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    [panel beginSheetForDirectory: sourceDirectory file: nil types: nil
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseSourcesDone:returnCode:contextInfo: )
                      contextInfo: sender]; 
}

- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    /* we convert the sender content of contextInfo back into a variable called sender
     * mostly just for consistency for evaluation later
     */
    id sender = (id)contextInfo;
    /* User selected a file to open */
	if( returnCode == NSOKButton )
    {
            /* Free display name allocated previously by this code */
        [browsedSourceDisplayName release];
       
        NSString *scanPath = [[sheet filenames] objectAtIndex: 0];
        /* we set the last searched source directory in the prefs here */
        NSString *sourceDirectory = [scanPath stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:sourceDirectory forKey:@"LastSourceDirectory"];
        /* we order out sheet, which is the browse window as we need to open
         * the title selection sheet right away
         */
        [sheet orderOut: self];
        
        if (sender == fOpenSourceTitleMMenu || [[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        {
            /* We put the chosen source path in the source display text field for the
             * source title selection sheet in which the user specifies the specific title to be
             * scanned  as well as the short source name in fSrcDsplyNameTitleScan just for display
             * purposes in the title panel
             */
            /* Full Path */
            [fScanSrcTitlePathField setStringValue:scanPath];
            NSString *displayTitlescanSourceName;

            if ([[scanPath lastPathComponent] isEqualToString: @"VIDEO_TS"])
            {
                /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
                 we have to use the title->path value so we get the proper name of the volume if a physical dvd is the source*/
                displayTitlescanSourceName = [[scanPath stringByDeletingLastPathComponent] lastPathComponent];
            }
            else
            {
                /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                displayTitlescanSourceName = [scanPath lastPathComponent];
            }
            /* we set the source display name in the title selection dialogue */
            [fSrcDsplyNameTitleScan setStringValue:displayTitlescanSourceName];
            /* we set the attempted scans display name for main window to displayTitlescanSourceName*/
            browsedSourceDisplayName = [displayTitlescanSourceName retain];
            /* We show the actual sheet where the user specifies the title to be scanned
             * as we are going to do a title specific scan
             */
            [self showSourceTitleScanPanel:nil];
        }
        else
        {
            /* We are just doing a standard full source scan, so we specify "0" to libhb */
            NSString *path = [[sheet filenames] objectAtIndex: 0];
            
            /* We check to see if the chosen file at path is a package */
            if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:path])
            {
                [self writeToActivityLog: "trying to open a package at: %s", [path UTF8String]];
                /* We check to see if this is an .eyetv package */
                if ([[path pathExtension] isEqualToString: @"eyetv"])
                {
                    [self writeToActivityLog:"trying to open eyetv package"];
                    /* We're looking at an EyeTV package - try to open its enclosed
                     .mpg media file */
                     browsedSourceDisplayName = [[[path stringByDeletingPathExtension] lastPathComponent] retain];
                    NSString *mpgname;
                    int n = [[path stringByAppendingString: @"/"]
                             completePathIntoString: &mpgname caseSensitive: YES
                             matchesIntoArray: nil
                             filterTypes: [NSArray arrayWithObject: @"mpg"]];
                    if (n > 0)
                    {
                        /* Found an mpeg inside the eyetv package, make it our scan path 
                        and call performScan on the enclosed mpeg */
                        path = mpgname;
                        [self writeToActivityLog:"found mpeg in eyetv package"];
                        [self performScan:path scanTitleNum:0];
                    }
                    else
                    {
                        /* We did not find an mpeg file in our package, so we do not call performScan */
                        [self writeToActivityLog:"no valid mpeg in eyetv package"];
                    }
                }
                /* We check to see if this is a .dvdmedia package */
                else if ([[path pathExtension] isEqualToString: @"dvdmedia"])
                {
                    /* path IS a package - but dvdmedia packages can be treaded like normal directories */
                    browsedSourceDisplayName = [[[path stringByDeletingPathExtension] lastPathComponent] retain];
                    [self writeToActivityLog:"trying to open dvdmedia package"];
                    [self performScan:path scanTitleNum:0];
                }
                else
                {
                    /* The package is not an eyetv package, so we do not call performScan */
                    [self writeToActivityLog:"unable to open package"];
                }
            }
            else // path is not a package, so we treat it as a dvd parent folder or VIDEO_TS folder
            {
                /* path is not a package, so we call perform scan directly on our file */
                if ([[path lastPathComponent] isEqualToString: @"VIDEO_TS"])
                {
                    [self writeToActivityLog:"trying to open video_ts folder (video_ts folder chosen)"];
                    /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name*/
                    browsedSourceDisplayName = [[[path stringByDeletingLastPathComponent] lastPathComponent] retain];
                }
                else
                {
                    [self writeToActivityLog:"trying to open video_ts folder (parent directory chosen)"];
                    /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                    /* make sure we remove any path extension as this can also be an '.mpg' file */
                    browsedSourceDisplayName = [[path lastPathComponent] retain];
                }
                applyQueueToScan = NO;
                [self performScan:path scanTitleNum:0];
            }

        }

    }
}

- (IBAction)showAboutPanel:(id)sender
{
    NSMutableDictionary* d = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
        fApplicationIcon, @"ApplicationIcon",
        nil ];
    [NSApp orderFrontStandardAboutPanelWithOptions:d];
    [d release];
}

/* Here we open the title selection sheet where we can specify an exact title to be scanned */
- (IBAction) showSourceTitleScanPanel: (id) sender
{
    /* We default the title number to be scanned to "0" which results in a full source scan, unless the
    * user changes it
    */
    [fScanSrcTitleNumField setStringValue: @"0"];
	/* Show the panel */
	[NSApp beginSheet:fScanSrcTitlePanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
}

- (IBAction) closeSourceTitleScanPanel: (id) sender
{
    [NSApp endSheet: fScanSrcTitlePanel];
    [fScanSrcTitlePanel orderOut: self];
    
    if(sender == fScanSrcTitleOpenButton)
    {
        /* We setup the scan status in the main window to indicate a source title scan */
        [fSrcDVD2Field setStringValue: @"Opening a new source title…"];
        [fScanIndicator setHidden: NO];
        [fScanHorizontalLine setHidden: YES];
        [fScanIndicator setIndeterminate: YES];
        [fScanIndicator startAnimation: nil];
		
        /* We use the performScan method to actually perform the specified scan passing the path and the title
         * to be scanned
         */
        applyQueueToScan = NO;
        [self performScan:[fScanSrcTitlePathField stringValue] scanTitleNum:[fScanSrcTitleNumField intValue]];
    }
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void) performScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum
{
    /* use a bool to determine whether or not we can decrypt using vlc */
    BOOL cancelScanDecrypt = 0;
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];
    
    // Notify ChapterTitles that there's no title
    [fChapterTitlesDelegate resetWithTitle:nil];
    [fChapterTable reloadData];
    
    // Notify Subtitles that there's no title
    [fSubtitlesDelegate resetWithTitle:nil];
    [fSubtitlesTable reloadData];
    
	//	Notify anyone interested (audio controller) that there's no title
	[[NSNotificationCenter defaultCenter] postNotification:
	 [NSNotification notificationWithName: HBTitleChangedNotification
								   object: self
								 userInfo: [NSDictionary dictionaryWithObjectsAndKeys:
											[NSData dataWithBytesNoCopy: &fTitle length: sizeof(fTitle) freeWhenDone: NO], keyTitleTag,
											nil]]];

    [self enableUI: NO];
    
    if( [detector isVideoDVD] )
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = [detector devicePath];
        [self writeToActivityLog: "trying to open a physical dvd at: %s", [scanPath UTF8String]];
        
        
        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        NSInteger suppressWarning = [prefs integerForKey:@"suppresslibdvdcss"];
        
        /* Notify the user that we don't support removal of copy proteciton. */
        void *dvdcss = dlopen("libdvdcss.2.dylib", RTLD_LAZY);
        if (dvdcss == NULL && suppressWarning != 1)
        {
            /* Only show the user this warning once. They may be using a solution we don't know about. Notifying them each time is annoying. */
            [prefs setInteger:1 forKey:@"suppresslibdvdcss"];
            
            /*compatible vlc not found, so we set the bool to cancel scanning to 1 */
            cancelScanDecrypt = 1;
            [self writeToActivityLog: "libdvdcss.2.dylib not found for decrypting physical dvd"];
            int status;
            status = NSRunAlertPanel(@"Please note that HandBrake does not support the removal of copy-protection from DVD Discs. You can if you wish install libdvdcss or any other 3rd party software for this function.",
                                     @"Videolan.org provides libdvdcss if you are not currently using another solution.", @"Get libdvdcss.pkg", @"Cancel Scan", @"Attempt Scan Anyway");
            [NSApp requestUserAttention:NSCriticalRequest];
            
            if (status == NSAlertDefaultReturn)
            {
                /* User chose to go download vlc (as they rightfully should) so we send them to the vlc site */
                [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://download.videolan.org/libdvdcss/1.2.12/macosx/"]];
            }
            else if (status == NSAlertAlternateReturn)
            {
                /* User chose to cancel the scan */
                [self writeToActivityLog: "Cannot open physical dvd, scan cancelled"];
            }
            else
            {
                /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
                cancelScanDecrypt = 0;
                [self writeToActivityLog: "User overrode copy-proteciton warning - trying to open physical dvd without decryption"];
            }
            
        }
        else
        {
            /* VLC was found in /Applications so all is well, we can carry on using vlc's libdvdcss.dylib for decrypting if needed */
            [self writeToActivityLog: "libdvdcss.2.dylib found for decrypting physical dvd"];
            dlclose(dvdcss);
        }
    }
    
    if (cancelScanDecrypt == 0)
    {
        /* We use our advanced pref to determine how many previews to scan */
        int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
        /* We use our advanced pref to determine the minimum title length to use in seconds*/
        int min_title_duration_seconds = [[[NSUserDefaults standardUserDefaults] objectForKey:@"MinTitleScanSeconds"] intValue];
        uint64_t min_title_duration_ticks = 90000LL * min_title_duration_seconds;
        /* set title to NULL */
        fTitle = NULL;
        /* We actually pass the scan off to libhb here.
         * If there is no title number passed to scan, we use 0
         * which causes the default behavior of a full source scan */
        if (scanTitleNum < 0)
        {
            scanTitleNum = 0;
        }
        if (scanTitleNum > 0)
        {
            [self writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        else
        {
            // minimum title duration doesn't apply to title-specific scan
            // it doesn't apply to batch scan either, but we can't tell it apart from DVD & BD folders here
            [self writeToActivityLog: "scanning titles with a duration of %d seconds or more", min_title_duration_seconds];
        }
        
        hb_system_sleep_prevent(fHandle);
        hb_scan(fHandle, [path UTF8String], scanTitleNum, hb_num_previews, 1 ,
                min_title_duration_ticks);
        
        [fSrcDVD2Field setStringValue:@"Scanning new source…"];

        // After the scan process, we signal to enableUI loop that this scan process is now finished
        // If remaining drag & drop files are in the UserDefaults array, they will then be processed
        fWillScan = NO;
    }
}

- (IBAction) cancelScanning:(id)sender
{
    hb_scan_stop(fHandle);
    hb_system_sleep_allow(fHandle);
}

- (IBAction) showNewScan:(id)sender
{
    hb_title_set_t * title_set;
	hb_title_t * title = NULL;
	int feature_title=0; // Used to store the main feature title

        title_set = hb_get_title_set( fHandle );
        
        if( !hb_list_count( title_set->list_title ) )
        {
            /* We display a message if a valid dvd source was not chosen */
            [fSrcDVD2Field setStringValue: @"No Valid Source Found"];
            SuccessfulScan = NO;
            
            // Notify ChapterTitles that there's no title
            [fSubtitlesDelegate resetWithTitle:nil];
            [fSubtitlesTable reloadData];
            
            // Notify Subtitles that there's no title
            [fChapterTitlesDelegate resetWithTitle:nil];
            [fChapterTable reloadData];

			//	Notify anyone interested (audio controller) that there's no title
			[[NSNotificationCenter defaultCenter] postNotification:
			 [NSNotification notificationWithName: HBTitleChangedNotification
										   object: self
										 userInfo: [NSDictionary dictionaryWithObjectsAndKeys:
													[NSData dataWithBytesNoCopy: &fTitle length: sizeof(fTitle) freeWhenDone: NO], keyTitleTag,
													nil]]];
        }
        else
        {
            if (applyQueueToScan == YES)
            {
                /* we are a rescan of an existing queue item and need to apply the queued settings to the scan */
                [self writeToActivityLog: "showNewScan: This is a queued item rescan"];
                
            }
            else if (applyQueueToScan == NO)
            {
                [self writeToActivityLog: "showNewScan: This is a new source item scan"];
            }
            else
            {
                [self writeToActivityLog: "showNewScan: cannot grok scan status"];
            }
            
              /* We increment the successful scancount here by one,
             which we use at the end of this function to tell the gui
             if this is the first successful scan since launch and whether
             or not we should set all settings to the defaults */
            currentSuccessfulScanCount++;
            
            [[fWindow toolbar] validateVisibleItems];
            
            [fSrcTitlePopUp removeAllItems];
            for( int i = 0; i < hb_list_count( title_set->list_title ); i++ )
            {
                title = (hb_title_t *) hb_list_item( title_set->list_title, i );
                
                currentSource = [NSString stringWithUTF8String: title->name];
                /*Set DVD Name at top of window with the browsedSourceDisplayName grokked right before -performScan */
                if (!browsedSourceDisplayName)
                {
                    browsedSourceDisplayName = @"NoNameDetected";
                }
                [fSrcDVD2Field setStringValue:browsedSourceDisplayName];
                
                // use the correct extension based on the container
                int format = [fDstFormatPopUp indexOfSelectedItem];
                char *ext = "mp4";
                if (format == 1)
                {
                    ext = "mkv";
                }
                
                /* If its a queue rescan for edit, get the queue item output path */
                /* if not, its a new source scan. */
                /* Check to see if the last destination has been set,use if so, if not, use Desktop */
                if (applyQueueToScan == YES)
                {
                    [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@", [[QueueFileArray objectAtIndex:fqueueEditRescanItemNum] objectForKey:@"DestinationPath"]]];
                }
                else if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"])
                {
                    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                                     @"%@/%@.%s", [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"],[browsedSourceDisplayName stringByDeletingPathExtension],ext]];
                }
                else
                {
                    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                                     @"%@/Desktop/%@.%s", NSHomeDirectory(),[browsedSourceDisplayName stringByDeletingPathExtension],ext]];
                }
                
                // set m4v extension if necessary - do not override user-specified .mp4 extension
                if (format == 0 && applyQueueToScan != YES)
                {
                    [self autoSetM4vExtension: sender];
                }
                
                /* See if this is the main feature according to libhb */
                if (title->index == title_set->feature)
                {
                    feature_title = i;
                }
                
                if( title->type == HB_BD_TYPE )
                {
                    [fSrcTitlePopUp addItemWithTitle: [NSString
                                                       stringWithFormat: @"%@ %d (%05d.MPLS) - %02dh%02dm%02ds",
                                                       currentSource, title->index, title->playlist,
                                                       title->hours, title->minutes, title->seconds]];
                }
                else
                {
                    [fSrcTitlePopUp addItemWithTitle: [NSString
                                                       stringWithFormat: @"%@ %d - %02dh%02dm%02ds",
                                                       currentSource, title->index,
                                                       title->hours, title->minutes, title->seconds]];
                }
            }
            
            /* if we are a stream, select the first title */
            if (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE)
            {
                [fSrcTitlePopUp selectItemAtIndex: 0];
            }
            else
            {
                /* if not then select the main feature title */
                [fSrcTitlePopUp selectItemAtIndex: feature_title];
            }
            [self titlePopUpChanged:nil];
            
            SuccessfulScan = YES;
            [self enableUI: YES];

            /* if its the initial successful scan after awakeFromNib */
            if (currentSuccessfulScanCount == 1)
            {
                [self encodeStartStopPopUpChanged:nil];
                
                [self selectDefaultPreset:nil];
                
                // Open preview window now if it was visible when HB was closed
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PreviewWindowIsOpen"])
                    [self showPreviewWindow:nil];
                
                // Open picture sizing window now if it was visible when HB was closed
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PictureSizeWindowIsOpen"])
                    [self showPicturePanel:nil];
                
            }
            if (applyQueueToScan == YES)
            {
                /* we are a rescan of an existing queue item and need to apply the queued settings to the scan */
                [self writeToActivityLog: "showNewScan: calling applyQueueSettingsToMainWindow"];
                [self applyQueueSettingsToMainWindow:nil];
                
            }
        }
    
    /* Done scanning, allow system sleep for the scan handle */
    hb_system_sleep_allow(fHandle);
}


#pragma mark -
#pragma mark New Output Destination

- (IBAction) browseFile: (id) sender
{
    /* Open a panel to let the user choose and update the text field */
    NSSavePanel * panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
	[panel beginSheetForDirectory: [[fDstFile2Field stringValue] stringByDeletingLastPathComponent] file: [[fDstFile2Field stringValue] lastPathComponent]
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( browseFileDone:returnCode:contextInfo: )
					  contextInfo: NULL];
}

- (void) browseFileDone: (NSSavePanel *) sheet
             returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fDstFile2Field setStringValue: [sheet filename]];
        /* Save this path to the prefs so that on next browse destination window it opens there */
        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];   
    }
}


#pragma mark -
#pragma mark Main Window Control

- (IBAction) openMainWindow: (id) sender
{
    [fWindow  makeKeyAndOrderFront:nil];
}

- (BOOL) windowShouldClose: (id) sender
{
    return YES;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
    if( !flag ) {
        [fWindow makeKeyAndOrderFront:nil];
    }
    return YES;
}

- (NSSize) drawerWillResizeContents:(NSDrawer *) drawer toSize:(NSSize) contentSize {
	[[NSUserDefaults standardUserDefaults] setObject:NSStringFromSize( contentSize ) forKey:@"Drawer Size"];
	return contentSize;
}

#pragma mark -
#pragma mark Queue File

- (void) loadQueueFile {
	/* We declare the default NSFileManager into fileManager */
	NSFileManager * fileManager = [NSFileManager defaultManager];
	/* We define the location of the user presets file */
    QueueFile = @"~/Library/Application Support/HandBrake/Queue.plist";
	QueueFile = [[QueueFile stringByExpandingTildeInPath]retain];
    /* We check for the Queue.plist */
	if ([fileManager fileExistsAtPath:QueueFile] == 0)
	{
		[fileManager createFileAtPath:QueueFile contents:nil attributes:nil];
	}
    
	QueueFileArray = [[NSMutableArray alloc] initWithContentsOfFile:QueueFile];
	/* lets check to see if there is anything in the queue file .plist */
    if (nil == QueueFileArray)
	{
        /* if not, then lets initialize an empty array */
		QueueFileArray = [[NSMutableArray alloc] init];
    }
    else
    {
        /* ONLY clear out encoded items if we are single instance */
        if (hbInstanceNum == 1)
        {
            [self clearQueueEncodedItems];
        }
    }
}

- (void)addQueueFileItem
{
    [QueueFileArray addObject:[self createQueueFileItem]];
    [self saveQueueFileItem];
    
}

- (void) removeQueueFileItem:(int) queueItemToRemove
{
    [QueueFileArray removeObjectAtIndex:queueItemToRemove];
    [self saveQueueFileItem];

}

- (void)saveQueueFileItem
{
    [QueueFileArray writeToFile:QueueFile atomically:YES];
    [fQueueController setQueueArray: QueueFileArray];
    [self getQueueStats];
}

- (void)getQueueStats
{
/* lets get the stats on the status of the queue array */

fEncodingQueueItem = 0;
fPendingCount = 0;
fCompletedCount = 0;
fCanceledCount = 0;
fWorkingCount = 0;

    /* We use a number system to set the encode status of the queue item
     * in controller.mm
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */

	int i = 0;
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 0) // Completed
		{
			fCompletedCount++;	
		}
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 1) // being encoded
		{
			fWorkingCount++;
            fEncodingQueueItem = i;
            /* check to see if we are the instance doing this encoding */
            if ([thisQueueDict objectForKey:@"EncodingPID"] && [[thisQueueDict objectForKey:@"EncodingPID"] intValue] == pidNum)
            {
                currentQueueEncodeIndex = i;
            }
            	
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 2) // pending		
        {
			fPendingCount++;
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 3) // cancelled		
        {
			fCanceledCount++;
		}
		i++;
	}

    /* Set the queue status field in the main window */
    NSMutableString * string;
    if (fPendingCount == 0)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"No encode pending", @"" )];
    }
    else if (fPendingCount == 1)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode pending", @"" ), fPendingCount];
    }
    else
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encodes pending", @"" ), fPendingCount];
    }
    [fQueueStatus setStringValue:string];
}

/* Used to get the next pending queue item index and return it if found */
- (int)getNextPendingQueueIndex
{
    /* initialize nextPendingIndex to -1, this value tells incrementQueueItemDone that there are no pending items in the queue */
    int nextPendingIndex = -1;
	BOOL nextPendingFound = NO;
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    int i = 0;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisQueueDict = tempObject;
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 2 && nextPendingFound == NO) // pending		
        {
			nextPendingFound = YES;
            nextPendingIndex = [QueueFileArray indexOfObject: tempObject];
            [self writeToActivityLog: "getNextPendingQueueIndex next pending encode index is:%d", nextPendingIndex];
		}
        i++;
	}
    return nextPendingIndex;
}


/* This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void) setQueueEncodingItemsAsPending
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        /* We want to keep any queue item that is pending or was previously being encoded */
        if ([[tempObject objectForKey:@"Status"] intValue] == 1 || [[tempObject objectForKey:@"Status"] intValue] == 2)
        {
            /* If the queue item is marked as "encoding" (1)
             * then change its status back to pending (2) which effectively
             * puts it back into the queue to be encoded
             */
            if ([[tempObject objectForKey:@"Status"] intValue] == 1)
            {
                [tempObject setObject:[NSNumber numberWithInt: 2] forKey:@"Status"];
            }
            [tempArray addObject:tempObject];
        }
    }
    
    [QueueFileArray setArray:tempArray];
    [self saveQueueFileItem];
}


/* This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as cancelled encodes */
- (void) clearQueueEncodedItems
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        /* If the queue item is either completed (0) or cancelled (3) from the
         * last session, then we put it in tempArray to be deleted from QueueFileArray.
         * NOTE: this means we retain pending (2) and also an item that is marked as
         * still encoding (1). If the queue has an item that is still marked as encoding
         * from a previous session, we can conlude that HB was either shutdown, or crashed
         * during the encodes so we keep it and tell the user in the "Load Queue Alert"
         */
        if ([[tempObject objectForKey:@"Status"] intValue] == 0 || [[tempObject objectForKey:@"Status"] intValue] == 3)
        {
            [tempArray addObject:tempObject];
        }
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/* This method will clear the queue of all encodes. effectively creating an empty queue */
- (void) clearQueueAllItems
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        [tempArray addObject:tempObject];
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/* This method will duplicate prepareJob however into the
 * queue .plist instead of into the job structure so it can
 * be recalled later */
- (NSDictionary *)createQueueFileItem
{
    NSMutableDictionary *queueFileJob = [[NSMutableDictionary alloc] init];
    
       hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    
    
    
    /* We use a number system to set the encode status of the queue item
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */
    [queueFileJob setObject:[NSNumber numberWithInt:2] forKey:@"Status"];
    /* Source and Destination Information */
    
    [queueFileJob setObject:[NSString stringWithUTF8String: title->path] forKey:@"SourcePath"];
    [queueFileJob setObject:[fSrcDVD2Field stringValue] forKey:@"SourceName"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->index] forKey:@"TitleNumber"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcAnglePopUp indexOfSelectedItem] + 1] forKey:@"TitleAngle"];
    
    /* Determine and set a variable to tell hb what start and stop times to use (chapters, seconds or frames) */
    if( [fEncodeStartStopPopUp indexOfSelectedItem] == 0 )
    {
        [queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"fEncodeStartStop"];    
    }
    else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 1)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:1] forKey:@"fEncodeStartStop"];   
    }
    else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 2)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:2] forKey:@"fEncodeStartStop"];
    }
    /* Chapter encode info */
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"ChapterStart"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"ChapterEnd"];
    /* Time (pts) encode info */
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcTimeStartEncodingField intValue]] forKey:@"StartSeconds"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcTimeEndEncodingField intValue] - [fSrcTimeStartEncodingField intValue]] forKey:@"StopSeconds"];
    /* Frame number encode info */
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcFrameStartEncodingField intValue]] forKey:@"StartFrame"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcFrameEndEncodingField intValue] - [fSrcFrameStartEncodingField intValue]] forKey:@"StopFrame"];
    
    
    /* The number of seek points equals the number of seconds announced in the title as that is our current granularity */
    int title_duration_seconds = (title->hours * 3600) + (title->minutes * 60) + (title->seconds);
    [queueFileJob setObject:[NSNumber numberWithInt:title_duration_seconds] forKey:@"SourceTotalSeconds"];
    
    [queueFileJob setObject:[fDstFile2Field stringValue] forKey:@"DestinationPath"];
    
    /* Lets get the preset info if there is any */
    [queueFileJob setObject:[fPresetSelectedDisplay stringValue] forKey:@"PresetName"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fPresetsOutlineView selectedRow]] forKey:@"PresetIndexNum"];
    
    [queueFileJob setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
    /* Chapter Markers*/
    /* If we are encoding by chapters and we have only one chapter or a title without chapters, set chapter markers to off.
       Leave them on if we're doing point to point encoding, as libhb supports chapters when doing p2p. */
    if ([fEncodeStartStopPopUp indexOfSelectedItem] == 0 &&
        [fSrcChapterStartPopUp indexOfSelectedItem] == [fSrcChapterEndPopUp indexOfSelectedItem])
    {
        [queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
    }
    else
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fCreateChapterMarkers state]] forKey:@"ChapterMarkers"];
    }
	
    /* We need to get the list of chapter names to put into an array and store 
     * in our queue, so they can be reapplied in prepareJob when this queue
     * item comes up if Chapter Markers is set to on.
     */
    [queueFileJob setObject:[fChapterTitlesDelegate chapterTitlesArray] forKey:@"ChapterNames"];
    
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
	[queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
    /* Mux mp4 with http optimization */
    [queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
    /* Add iPod uuid atom */
    [queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
    
    /* Codecs */
	/* Video encoder */
	[queueFileJob setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
	
    /* x264 advanced options */
    if ([fX264UseAdvancedOptionsCheck state])
    {
        // we are using the advanced panel
        [queueFileJob setObject:[NSNumber numberWithInt:1]       forKey: @"x264UseAdvancedOptions"];
        [queueFileJob setObject:[fAdvancedOptions optionsString] forKey:@"x264Option"];
    }
    else
    {
        // we are using the x264 preset system
        [queueFileJob setObject:[NSNumber numberWithInt:0] forKey: @"x264UseAdvancedOptions"];
        [queueFileJob setObject:[self x264Preset]          forKey: @"x264Preset"];
        [queueFileJob setObject:[self x264Tune]            forKey: @"x264Tune"];
        [queueFileJob setObject:[self x264OptionExtra]     forKey: @"x264OptionExtra"];
        [queueFileJob setObject:[self h264Profile]         forKey: @"h264Profile"];
        [queueFileJob setObject:[self h264Level]           forKey: @"h264Level"];
    }
    
    /* FFmpeg (lavc) Option String */
    [queueFileJob setObject:[fAdvancedOptions optionsStringLavc] forKey:@"lavcOption"];

	[queueFileJob setObject:[NSNumber numberWithInt:[[fVidQualityMatrix selectedCell] tag] + 1] forKey:@"VideoQualityType"];
	[queueFileJob setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
	[queueFileJob setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];
    /* Framerate */
    [queueFileJob setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
    /* Frame Rate Mode */
    if ([fFramerateMatrix selectedRow] == 1) // if selected we are cfr regardless of the frame rate popup
    {
        [queueFileJob setObject:@"cfr" forKey:@"VideoFramerateMode"];
    }
    else
    {
        if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source frame rate
        {
            [queueFileJob setObject:@"vfr" forKey:@"VideoFramerateMode"];
        }
        else
        {
            [queueFileJob setObject:@"pfr" forKey:@"VideoFramerateMode"];
        }
        
    }
    
    
	/* 2 Pass Encoding */
	[queueFileJob setObject:[NSNumber numberWithInt:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
	/* Turbo 2 pass Encoding fVidTurboPassCheck*/
	[queueFileJob setObject:[NSNumber numberWithInt:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];
    
	/* Picture Sizing */
	/* Use Max Picture settings for whatever the dvd is.*/
	[queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->modulus] forKey:@"PictureModulus"];
    /* if we are custom anamorphic, store the exact storage, par and display dims */
    if (fTitle->job->anamorphic.mode == 3)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->modulus] forKey:@"PicturePARModulus"];
        
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PicturePARStorageWidth"];
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PicturePARStorageHeight"];
        
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_width] forKey:@"PicturePARPixelWidth"];
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_height] forKey:@"PicturePARPixelHeight"];
        
        [queueFileJob setObject:[NSNumber numberWithFloat:fTitle->job->anamorphic.dar_width] forKey:@"PicturePARDisplayWidth"];
        [queueFileJob setObject:[NSNumber numberWithFloat:fTitle->job->anamorphic.dar_height] forKey:@"PicturePARDisplayHeight"];

    }

    /* Text summaries of various settings */
    [queueFileJob setObject:[NSString stringWithString:[self pictureSettingsSummary]]
                     forKey:@"PictureSettingsSummary"];
    [queueFileJob setObject:[NSString stringWithString:[self pictureFiltersSummary]]
                     forKey:@"PictureFiltersSummary"];
    [queueFileJob setObject:[NSString stringWithString:[self muxerOptionsSummary]]
                     forKey:@"MuxerOptionsSummary"];
    
    /* Set crop settings here */
	[queueFileJob setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    /* Picture Filters */
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
    [queueFileJob setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController decomb]] forKey:@"PictureDecomb"];
    [queueFileJob setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
    [queueFileJob setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController denoise]] forKey:@"PictureDenoise"];
    [queueFileJob setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
    
    [queueFileJob setObject:[NSString stringWithFormat:@"%d",[fPictureController deblock]] forKey:@"PictureDeblock"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController grayscale]] forKey:@"VideoGrayScale"];
    
    /* Auto Passthru */
    [queueFileJob setObject:[NSNumber numberWithInt:[fAudioAllowAACPassCheck state]] forKey: @"AudioAllowAACPass"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fAudioAllowAC3PassCheck state]] forKey: @"AudioAllowAC3Pass"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fAudioAllowDTSHDPassCheck state]] forKey: @"AudioAllowDTSHDPass"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fAudioAllowDTSPassCheck state]] forKey: @"AudioAllowDTSPass"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fAudioAllowMP3PassCheck state]] forKey: @"AudioAllowMP3Pass"];
    // just in case we need it for display purposes
    [queueFileJob setObject:[fAudioFallbackPopUp titleOfSelectedItem] forKey: @"AudioEncoderFallback"];
    // actual fallback encoder
    [queueFileJob setObject:[NSNumber numberWithInt:[[fAudioFallbackPopUp selectedItem] tag]] forKey: @"JobAudioEncoderFallback"];
    
    /* Audio */
    [fAudioDelegate prepareAudioForQueueFileJob: queueFileJob];
    
	/* Subtitles */
    NSMutableArray *subtitlesArray = [[NSMutableArray alloc] initWithArray:[fSubtitlesDelegate getSubtitleArray] copyItems:YES];
    [queueFileJob setObject:[NSArray arrayWithArray: subtitlesArray] forKey:@"SubtitleList"];
    [subtitlesArray autorelease];

    /* Now we go ahead and set the "job->values in the plist for passing right to fQueueEncodeLibhb */
     
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterStart"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterEnd"];
    
    
    [queueFileJob setObject:[NSNumber numberWithInt:[[fDstFormatPopUp selectedItem] tag]] forKey:@"JobFileFormatMux"];
    
    /* Codecs */
	/* Video encoder */
	[queueFileJob setObject:[NSNumber numberWithInt:[[fVidEncoderPopUp selectedItem] tag]] forKey:@"JobVideoEncoderVcodec"];
	
    /* Framerate */
    [queueFileJob setObject:[NSNumber numberWithInt:[fVidRatePopUp indexOfSelectedItem]] forKey:@"JobIndexVideoFramerate"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate] forKey:@"JobVrate"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate_base] forKey:@"JobVrateBase"];
	
    /* Picture Sizing */
	/* Use Max Picture settings for whatever the dvd is.*/
	[queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    
    /* Set crop settings here */
	[queueFileJob setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    

    /* we need to auto relase the queueFileJob and return it */
    [queueFileJob autorelease];
    return queueFileJob;

}

/* this is actually called from the queue controller to modify the queue array and return it back to the queue controller */
- (void)moveObjectsInQueueArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;
    
    
    NSUInteger removeIndex;
        
    if (index >= insertIndex)
    {
        removeIndex = index + aboveInsertIndexCount;
        aboveInsertIndexCount++;
    }
    else
    {
        removeIndex = index;
        insertIndex--;
    }

    id object = [[QueueFileArray objectAtIndex:removeIndex] retain];
    [QueueFileArray removeObjectAtIndex:removeIndex];
    [QueueFileArray insertObject:object atIndex:insertIndex];
    [object release];
        
    index = [indexSet indexLessThanIndex:index];

   /* We save all of the Queue data here 
    * and it also gets sent back to the queue controller*/
    [self saveQueueFileItem]; 
    
}


#pragma mark -
#pragma mark Queue Job Processing

- (void) incrementQueueItemDone:(int) queueItemDoneIndexNum
{
    /* Mark the encode just finished as done (status 0)*/
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:0] forKey:@"Status"];
	
    /* We save all of the Queue data here */
    [self saveQueueFileItem];

    /* Since we have now marked a queue item as done
     * we can go ahead and increment currentQueueEncodeIndex 
     * so that if there is anything left in the queue we can
     * go ahead and move to the next item if we want to */
    int queueItems = [QueueFileArray count];
    /* Check to see if there are any more pending items in the queue */
    int newQueueItemIndex = [self getNextPendingQueueIndex];
    /* If we still have more pending items in our queue, lets go to the next one */
    if (newQueueItemIndex >= 0 && newQueueItemIndex < queueItems)
    {
        /*Set our currentQueueEncodeIndex now to the newly found Pending encode as we own it */
        currentQueueEncodeIndex = newQueueItemIndex;
        /* now we mark the queue item as Status = 1 ( being encoded ) so another instance can not come along and try to scan it while we are scanning */
        [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
        [self writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];
        /* now we can go ahead and scan the new pending queue item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];

    }
    else
    {
        [self writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
        /* Done encoding, allow system sleep for the encode handle */
        hb_system_sleep_allow(fQueueEncodeLibhb);
        /*
         * Since there are no more items to encode, go to queueCompletedAlerts
         * for user specified alerts after queue completed
         */
        [self queueCompletedAlerts];
    }
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum
{
    /* Tell HB to output a new activity log file for this encode */
    [outputPanel startEncodeLog:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"DestinationPath"]];
    
    /* We now flag the queue item as being owned by this instance of HB using the PID */
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:pidNum] forKey:@"EncodingPID"];
    /* Get the currentQueueEncodeNameString from the queue item to display in the status field */
    currentQueueEncodeNameString = [[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"DestinationPath"] lastPathComponent]retain];
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    
    /* use a bool to determine whether or not we can decrypt using vlc */
    BOOL cancelScanDecrypt = 0;
    /* set the bool so that showNewScan knows to apply the appropriate queue
     * settings as this is a queue rescan
     */
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];

    if (cancelScanDecrypt == 0)
    {
        /* We actually pass the scan off to libhb here.
         * If there is no title number passed to scan, we use "0"
         * which causes the default behavior of a full source scan */
        if (scanTitleNum < 0)
        {
            scanTitleNum = 0;
        }
        if (scanTitleNum > 0)
        {
            [self writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        /*
         * Only scan 10 previews before an encode - additional previews are
         * only useful for autocrop and static previews, which are already taken
         * care of at this point
         */
        hb_system_sleep_prevent(fQueueEncodeLibhb);
        hb_scan(fQueueEncodeLibhb, [path UTF8String], scanTitleNum, 10, 0, 0);
    }
}

/* This assumes that we have re-scanned and loaded up a new queue item to send to libhb as fQueueEncodeLibhb */
- (void) processNewQueueEncode
{
    hb_list_t  * list  = hb_get_titles( fQueueEncodeLibhb );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,0 ); // is always zero since now its a single title scan
    hb_job_t * job = title->job;
    
    if( !hb_list_count( list ) )
    {
        [self writeToActivityLog: "processNewQueueEncode WARNING nothing found in the title list"];
    }
    
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    [self writeToActivityLog: "Preset: %s", [[queueToApply objectForKey:@"PresetName"] UTF8String]];
    [self writeToActivityLog: "processNewQueueEncode number of passes expected is: %d", ([[queueToApply objectForKey:@"VideoTwoPass"] intValue] + 1)];
    hb_job_set_file(job, [[queueToApply objectForKey:@"DestinationPath"] UTF8String]);
    [self prepareJob];
    
    /*
     * If scanning we need to do some extra setup of the job.
     */
    if (job->indepth_scan == 1)
    {
        char *x264_preset_tmp   = job->x264_preset   != NULL ? strdup(job->x264_preset)   : NULL;
        char *x264_tune_tmp     = job->x264_tune     != NULL ? strdup(job->x264_tune)     : NULL;
        char *advanced_opts_tmp = job->advanced_opts != NULL ? strdup(job->advanced_opts) : NULL;
        char *h264_profile_tmp  = job->h264_profile  != NULL ? strdup(job->h264_profile)  : NULL;
        char *h264_level_tmp    = job->h264_level    != NULL ? strdup(job->h264_level)    : NULL;
        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        hb_job_set_x264_preset  (job, NULL);
        hb_job_set_x264_tune    (job, NULL);
        hb_job_set_advanced_opts(job, NULL);
        hb_job_set_h264_profile (job, NULL);
        hb_job_set_h264_level   (job, NULL);
        job->pass = -1;
        hb_add(fQueueEncodeLibhb, job);
        /*
         * reset the advanced settings
         */
        hb_job_set_x264_preset  (job, x264_preset_tmp);
        hb_job_set_x264_tune    (job, x264_tune_tmp);
        hb_job_set_advanced_opts(job, advanced_opts_tmp);
        hb_job_set_h264_profile (job, h264_profile_tmp);
        hb_job_set_h264_level   (job, h264_level_tmp);
        free(x264_preset_tmp);
        free(x264_tune_tmp);
        free(advanced_opts_tmp);
        free(h264_profile_tmp);
        free(h264_level_tmp);
    }

    
    if ([[queueToApply objectForKey:@"VideoTwoPass"] intValue] == 1)
    {
        job->indepth_scan = 0;
        job->pass = 1;
        hb_add(fQueueEncodeLibhb, job);
        job->pass = 2;
        hb_add(fQueueEncodeLibhb, job);
        
    }
    else
    {
        job->indepth_scan = 0;
        job->pass = 0;
        hb_add(fQueueEncodeLibhb, job);
    }

    NSString *destinationDirectory = [[queueToApply objectForKey:@"DestinationPath"] stringByDeletingLastPathComponent];
	[[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
	/* Lets mark our new encode as 1 or "Encoding" */
    [queueToApply setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
    [self saveQueueFileItem];
    
    /* libhb makes a copy of the job.  So we need to free any resource
     * that were allocated in construction of the job. This empties
     * the audio, subtitle, and filter lists */
    hb_job_reset(job);
    
    /* We should be all setup so let 'er rip */   
    [self doRip];
}

#pragma mark -
#pragma mark Queue Item Editing

/* Rescans the chosen queue item back into the main window */
- (void)rescanQueueItemToMainWindow:(NSString *) scanPath scanTitleNum: (int) scanTitleNum selectedQueueItem: (int) selectedQueueItem
{
    fqueueEditRescanItemNum = selectedQueueItem;
    [self writeToActivityLog: "rescanQueueItemToMainWindow: Re-scanning queue item at index:%d",fqueueEditRescanItemNum];
    applyQueueToScan = YES;
    /* Make sure we release the display name before reassigning it */
    [browsedSourceDisplayName release];
    /* Set the browsedSourceDisplayName for showNewScan */
    browsedSourceDisplayName = [[[QueueFileArray objectAtIndex:fqueueEditRescanItemNum] objectForKey:@"SourceName"] retain];
    [self performScan:scanPath scanTitleNum:scanTitleNum];
}


/* We use this method after a queue item rescan for edit.
 * it largely mirrors -selectPreset in terms of structure.
 * Assumes that a queue item has been reloaded into the main window.
 */
- (IBAction)applyQueueSettingsToMainWindow:(id)sender
{
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:fqueueEditRescanItemNum];
    hb_job_t * job = fTitle->job;
    if (queueToApply)
    {
        [self writeToActivityLog: "applyQueueSettingsToMainWindow: queue item found"];
    }
    /* Set title number and chapters */
    /* since the queue only scans a single title, its already been selected in showNewScan
       so do not try to reset it here. However if we do decide to do full source scans on
       a queue edit rescan, we would need it. So leaving in for now but commenting out. */
    //[fSrcTitlePopUp selectItemAtIndex: [[queueToApply objectForKey:@"TitleNumber"] intValue] - 1];
    
    [fSrcChapterStartPopUp selectItemAtIndex: [[queueToApply objectForKey:@"ChapterStart"] intValue] - 1];
    [fSrcChapterEndPopUp selectItemAtIndex: [[queueToApply objectForKey:@"ChapterEnd"] intValue] - 1];
    
    /* File Format */
    [fDstFormatPopUp selectItemWithTitle:[queueToApply objectForKey:@"FileFormat"]];
    [self formatPopUpChanged:nil];
    
    /* Chapter Markers*/
    [fCreateChapterMarkers setState:[[queueToApply objectForKey:@"ChapterMarkers"] intValue]];
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
    [fDstMp4LargeFileCheck setState:[[queueToApply objectForKey:@"Mp4LargeFile"] intValue]];
    /* Mux mp4 with http optimization */
    [fDstMp4HttpOptFileCheck setState:[[queueToApply objectForKey:@"Mp4HttpOptimize"] intValue]];
    
    /* video encoder */
    [fVidEncoderPopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoEncoder"]];
    [fAdvancedOptions setLavcOptions:     [queueToApply objectForKey:@"lavcOption"]];
    /* advanced x264 options */
    if ([[queueToApply objectForKey:@"x264UseAdvancedOptions"] intValue])
    {
        // we are using the advanced panel
        [fAdvancedOptions setOptions:[queueToApply objectForKey:@"x264Option"]];
        // preset does not use the x264 preset system, reset the widgets
        [self setX264Preset:     nil];
        [self setX264Tune:       nil];
        [self setX264OptionExtra:[queueToApply objectForKey:@"x264Option"]];
        [self setH264Profile:    nil];
        [self setH264Level:      nil];
        // enable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOnState];
        [self updateX264Widgets:nil];
    }
    else
    {
        // we are using the x264 preset system
        [self setX264Preset:     [queueToApply objectForKey:@"x264Preset"]];
        [self setX264Tune:       [queueToApply objectForKey:@"x264Tune"]];
        [self setX264OptionExtra:[queueToApply objectForKey:@"x264OptionExtra"]];
        [self setH264Profile:    [queueToApply objectForKey:@"h264Profile"]];
        [self setH264Level:      [queueToApply objectForKey:@"h264Level"]];
        // preset does not use the advanced panel, reset it
        [fAdvancedOptions setOptions:@""];
        // disable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOffState];
        [self updateX264Widgets:nil];
    }
    
    /* Lets run through the following functions to get variables set there */
    [self videoEncoderPopUpChanged:nil];
    /* Set the state of ipod compatible with Mp4iPodCompatible. Only for x264*/
    [fDstMp4iPodFileCheck setState:[[queueToApply objectForKey:@"Mp4iPodCompatible"] intValue]];
    [self calculateBitrate:nil];
    
    /* Video quality */
    [fVidQualityMatrix selectCellAtRow:[[queueToApply objectForKey:@"VideoQualityType"] intValue] column:0];
    
    [fVidBitrateField setStringValue:[queueToApply objectForKey:@"VideoAvgBitrate"]];
    
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
    {
        /* Since theora's qp value goes up from left to right, we can just set the slider float value */
        [fVidQualitySlider setFloatValue:[[queueToApply objectForKey:@"VideoQualitySlider"] floatValue]];
    }
    else
    {
        /* Since ffmpeg and x264 use an "inverted" slider (lower qp/rf values indicate a higher quality) we invert the value on the slider */
        [fVidQualitySlider setFloatValue:([fVidQualitySlider maxValue] + [fVidQualitySlider minValue]) - [[queueToApply objectForKey:@"VideoQualitySlider"] floatValue]];
    }
    
    [self videoMatrixChanged:nil];
        
    /* Video framerate */
    if ([[queueToApply objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"vfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want vfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    else
    {
        /* Now set the Video Frame Rate Mode to either pfr or cfr according to the preset */
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"pfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    [fVidRatePopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoFramerate"]];
    [self videoFrameRateChanged:nil];
    
    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[[queueToApply objectForKey:@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];
    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[[queueToApply objectForKey:@"VideoTurboTwoPass"] intValue]];
    
    /* Auto Passthru */
    [fAudioAllowAACPassCheck setState:[[queueToApply objectForKey:@"AudioAllowAACPass"] intValue]];
    [fAudioAllowAC3PassCheck setState:[[queueToApply objectForKey:@"AudioAllowAC3Pass"] intValue]];
    [fAudioAllowDTSHDPassCheck setState:[[queueToApply objectForKey:@"AudioAllowDTSHDPass"] intValue]];
    [fAudioAllowDTSPassCheck setState:[[queueToApply objectForKey:@"AudioAllowDTSPass"] intValue]];
    [fAudioAllowMP3PassCheck setState:[[queueToApply objectForKey:@"AudioAllowMP3Pass"] intValue]];
    [fAudioFallbackPopUp selectItemWithTitle:[queueToApply objectForKey:@"AudioEncoderFallback"]];
    
    /* Audio */
    /* Now lets add our new tracks to the audio list here */
    [fAudioDelegate addTracksFromQueue: queueToApply];
    
    /*Subtitles*/
    /* Crashy crashy right now, working on it */
    [fSubtitlesDelegate setNewSubtitles:[queueToApply objectForKey:@"SubtitleList"]];
    [fSubtitlesTable reloadData];  
    /* Picture Settings */
    
    /* If Cropping is set to custom, then recall all four crop values from
     when the preset was created and apply them */
    if ([[queueToApply objectForKey:@"PictureAutoCrop"]  intValue] == 0)
    {
        [fPictureController setAutoCrop:NO];
        
        /* Here we use the custom crop values saved at the time the preset was saved */
        job->crop[0] = [[queueToApply objectForKey:@"PictureTopCrop"]  intValue];
        job->crop[1] = [[queueToApply objectForKey:@"PictureBottomCrop"]  intValue];
        job->crop[2] = [[queueToApply objectForKey:@"PictureLeftCrop"]  intValue];
        job->crop[3] = [[queueToApply objectForKey:@"PictureRightCrop"]  intValue];
        
    }
    else /* if auto crop has been saved in preset, set to auto and use post scan auto crop */
    {
        [fPictureController setAutoCrop:YES];
        /* Here we use the auto crop values determined right after scan */
        job->crop[0] = AutoCropTop;
        job->crop[1] = AutoCropBottom;
        job->crop[2] = AutoCropLeft;
        job->crop[3] = AutoCropRight;
        
    }
    
    job->modulus = [[queueToApply objectForKey:@"PictureModulus"]  intValue];
    
    /*
     * if the preset specifies neither max. width nor height
     * (both are 0), use the max. picture size
     *
     * if the specified non-zero dimensions exceed those of the
     * source, also use the max. picture size (no upscaling)
     */
    if (([[queueToApply objectForKey:@"PictureWidth"]  intValue] <= 0 &&
         [[queueToApply objectForKey:@"PictureHeight"] intValue] <= 0)              ||
        ([[queueToApply objectForKey:@"PictureWidth"]  intValue] >  fTitle->width &&
         [[queueToApply objectForKey:@"PictureHeight"] intValue] >  fTitle->height) ||
        ([[queueToApply objectForKey:@"PictureHeight"] intValue] <= 0 &&
         [[queueToApply objectForKey:@"PictureWidth"]  intValue] >  fTitle->width)  ||
        ([[queueToApply objectForKey:@"PictureWidth"]  intValue] <= 0 &&
         [[queueToApply objectForKey:@"PictureHeight"] intValue] >  fTitle->height))
    {
        /* use the source's width/height to avoid upscaling */
        [self revertPictureSizeToMax:nil];
    }
    else // source width/height is >= preset width/height
    {
        /* use the preset values for width/height */
        job->width  = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
        job->height = [[queueToApply objectForKey:@"PictureHeight"] intValue];
    }
    job->keep_ratio = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    if (job->keep_ratio == 1)
    {
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        if( job->height > fTitle->height )
        {
            job->height = fTitle->height;
            hb_fix_aspect( job, HB_KEEP_HEIGHT );
        }
    }
    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    job->modulus = [[queueToApply objectForKey:@"PictureModulus"]  intValue];
    
    /* Filters */
    
    /* We only allow *either* Decomb or Deinterlace. So check for the PictureDecombDeinterlace key. */
    [fPictureController setUseDecomb:1];
    [fPictureController setDecomb:0];
    [fPictureController setDeinterlace:0];
    if ([[queueToApply objectForKey:@"PictureDecombDeinterlace"] intValue] == 1)
    {
        /* we are using decomb */
        /* Decomb */
        if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] > 0)
        {
            [fPictureController setDecomb:[[queueToApply objectForKey:@"PictureDecomb"] intValue]];
            
            /* if we are using "Custom" in the decomb setting, also set the custom string*/
            if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 1)
            {
                [fPictureController setDecombCustomString:[queueToApply objectForKey:@"PictureDecombCustom"]];    
            }
        }
    }
    else
    {
        /* We are using Deinterlace */
        /* Deinterlace */
        if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] > 0)
        {
            [fPictureController setUseDecomb:0];
            [fPictureController setDeinterlace:[[queueToApply objectForKey:@"PictureDeinterlace"] intValue]];
            /* if we are using "Custom" in the deinterlace setting, also set the custom string*/
            if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 1)
            {
                [fPictureController setDeinterlaceCustomString:[queueToApply objectForKey:@"PictureDeinterlaceCustom"]];    
            }
        }
    }
    
    
    /* Detelecine */
    if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] > 0)
    {
        [fPictureController setDetelecine:[[queueToApply objectForKey:@"PictureDetelecine"] intValue]];
        /* if we are using "Custom" in the detelecine setting, also set the custom string*/
        if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 1)
        {
            [fPictureController setDetelecineCustomString:[queueToApply objectForKey:@"PictureDetelecineCustom"]];    
        }
    }
    else
    {
        [fPictureController setDetelecine:0];
    }
    
    /* Denoise */
    if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] > 0)
    {
        [fPictureController setDenoise:[[queueToApply objectForKey:@"PictureDenoise"] intValue]];
        /* if we are using "Custom" in the denoise setting, also set the custom string*/
        if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 1)
        {
            [fPictureController setDenoiseCustomString:[queueToApply objectForKey:@"PictureDenoiseCustom"]];    
        }
    }
    else
    {
        [fPictureController setDenoise:0];
    }   
    
    /* Deblock */
    if ([[queueToApply objectForKey:@"PictureDeblock"] intValue] == 1)
    {
        /* if its a one, then its the old on/off deblock, set on to 5*/
        [fPictureController setDeblock:5];
    }
    else
    {
        /* use the settings intValue */
        [fPictureController setDeblock:[[queueToApply objectForKey:@"PictureDeblock"] intValue]];
    }
    
    if ([[queueToApply objectForKey:@"VideoGrayScale"] intValue] == 1)
    {
        [fPictureController setGrayscale:1];
    }
    else
    {
        [fPictureController setGrayscale:0];
    }
    
    /* we call SetTitle: in fPictureController so we get an instant update in the Picture Settings window */
    [fPictureController SetTitle:fTitle];
    [self calculatePictureSizing:nil];
    
    /* somehow we need to figure out a way to tie the queue item to a preset if it used one */
    //[queueFileJob setObject:[fPresetSelectedDisplay stringValue] forKey:@"PresetName"];
    //[queueFileJob setObject:[NSNumber numberWithInt:[fPresetsOutlineView selectedRow]] forKey:@"PresetIndexNum"];
    if ([queueToApply objectForKey:@"PresetIndexNum"]) // This item used a preset so insert that info
	{
		/* Deselect the currently selected Preset if there is one*/
        //[fPresetsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:[[queueToApply objectForKey:@"PresetIndexNum"] intValue]] byExtendingSelection:NO];
        //[self selectPreset:nil];
		
        //[fPresetsOutlineView selectRow:[[queueToApply objectForKey:@"PresetIndexNum"] intValue]];
		/* Change UI to show "Custom" settings are being used */
		//[fPresetSelectedDisplay setStringValue: [[queueToApply objectForKey:@"PresetName"] stringValue]];
	}
    else
    {
        /* Deselect the currently selected Preset if there is one*/
		[fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
    }
    
    /* We need to set this bool back to NO, in case the user wants to do a scan */
    //applyQueueToScan = NO;
    
    /* Not that source is loaded and settings applied, delete the queue item from the queue */
    [self writeToActivityLog: "applyQueueSettingsToMainWindow: deleting queue item:%d",fqueueEditRescanItemNum];
    [self removeQueueFileItem:fqueueEditRescanItemNum];
}



#pragma mark -
#pragma mark Live Preview
/* Note,this is much like prepareJob, but directly sets the job vars so Picture Preview
 * can encode to its temp preview directory and playback. This is *not* used for any actual user
 * encodes
 */
- (void) prepareJobForPreview
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    hb_filter_object_t * filter;
    /* set job->angle for libdvdnav */
    job->angle = [fSrcAnglePopUp indexOfSelectedItem] + 1;
    /* Chapter selection */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end   = [fSrcChapterEndPopUp   indexOfSelectedItem] + 1;
	
    /* Format (Muxer) and Video Encoder */
    job->mux = [[fDstFormatPopUp selectedItem] tag];
    job->vcodec = [[fVidEncoderPopUp selectedItem] tag];
    job->fastfirstpass = 0;

    job->chapter_markers = 0;
    
	if (job->vcodec == HB_VCODEC_X264)
    {
        /* advanced x264 options */
        NSString   *tmpString;
        // translate zero-length strings to NULL for libhb
        const char *x264_preset   = NULL;
        const char *x264_tune     = NULL;
        const char *advanced_opts = NULL;
        const char *h264_profile  = NULL;
        const char *h264_level    = NULL;
        if ([fX264UseAdvancedOptionsCheck state])
        {
            // we are using the advanced panel
            if ([(tmpString = [fAdvancedOptions optionsString]) length])
            {
                advanced_opts = [tmpString UTF8String];
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = [self x264Tune]) length])
            {
                x264_tune = [tmpString UTF8String];
            }
            if ([(tmpString = [self x264OptionExtra]) length])
            {
                advanced_opts = [tmpString UTF8String];
            }
            if ([(tmpString = [self h264Profile]) length])
            {
                h264_profile = [tmpString UTF8String];
            }
            if ([(tmpString = [self h264Level]) length])
            {
                h264_level = [tmpString UTF8String];
            }
            x264_preset = [[self x264Preset] UTF8String];
        }
        hb_job_set_x264_preset  (job, x264_preset);
        hb_job_set_x264_tune    (job, x264_tune);
        hb_job_set_advanced_opts(job, advanced_opts);
        hb_job_set_h264_profile (job, h264_profile);
        hb_job_set_h264_level   (job, h264_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_advanced_opts(job,
                                 [[fAdvancedOptions optionsStringLavc]
                                  UTF8String]);
    }

    /* Video settings */
    
    if( [fVidRatePopUp indexOfSelectedItem] > 0 )
    {
        /* a specific framerate has been chosen */
        job->vrate      = 27000000;
        job->vrate_base = hb_video_rates[[fVidRatePopUp indexOfSelectedItem]-1].rate;
        if ([fFramerateMatrix selectedRow] == 1)
        {
            // CFR
            job->cfr = 1;
        }
        else
        {
            // PFR
            job->cfr = 2;
        }
    }
    else
    {
        /* same as source */
        job->vrate      = title->rate;
        job->vrate_base = title->rate_base;
        if ([fFramerateMatrix selectedRow] == 1)
        {
            // CFR
            job->cfr = 1;
        }
        else
        {
            // VFR
            job->cfr = 0;
        }
    }

    switch( [[fVidQualityMatrix selectedCell] tag] )
    {
        case 0:
            /* ABR */
            job->vquality = -1.0;
            job->vbitrate = [fVidBitrateField intValue];
            break;
        case 1:
            /* Constant Quality */
            job->vquality = [fVidQualityRFField floatValue];
            job->vbitrate = 0;
            break;
    }

    /* Subtitle settings */
    NSMutableArray *subtitlesArray = [[NSMutableArray alloc] initWithArray:[fSubtitlesDelegate getSubtitleArray] copyItems:YES];
    
    
int subtitle;
int force;
int burned;
int def;
bool one_burned = FALSE;

    int i = 0;
    NSEnumerator *enumerator = [subtitlesArray objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        
        subtitle = [[tempObject objectForKey:@"subtitleSourceTrackNum"] intValue];
        force = [[tempObject objectForKey:@"subtitleTrackForced"] intValue];
        burned = [[tempObject objectForKey:@"subtitleTrackBurned"] intValue];
        def = [[tempObject objectForKey:@"subtitleTrackDefault"] intValue];
        
        /* since the subtitleSourceTrackNum 0 is "None" in our array of the subtitle popups,
         * we want to ignore it for display as well as encoding.
         */
        if (subtitle > 0)
        {
            /* if i is 0, then we are in the first item of the subtitles which we need to 
             * check for the "Foreign Audio Search" which would be subtitleSourceTrackNum of 1
             * bearing in mind that for all tracks subtitleSourceTrackNum of 0 is None.
             */
            
            /* if we are on the first track and using "Foreign Audio Search" */ 
            if (i == 0 && subtitle == 1)
            {
                /* NOTE: Currently foreign language search is borked for preview.
                 * Commented out but left in for initial commit. */
                
                
                [self writeToActivityLog: "Foreign Language Search: %d", 1];
                
                job->indepth_scan = 1;
                
                if (burned != 1)
                {
                    job->select_subtitle_config.dest = PASSTHRUSUB;
                }
                else
                {
                    job->select_subtitle_config.dest = RENDERSUB;
                }
                
                job->select_subtitle_config.force = force;
                job->select_subtitle_config.default_track = def;
            }
            else
            {
                /* if we are getting the subtitles from an external srt file */
                if ([[tempObject objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
                {
                    hb_subtitle_config_t sub_config;
                    
                    sub_config.offset = [[tempObject objectForKey:@"subtitleTrackSrtOffset"] intValue];
                    
                    /* we need to srncpy file path and char code */
                    strncpy(sub_config.src_filename, [[tempObject objectForKey:@"subtitleSourceSrtFilePath"] UTF8String], 255);
                    sub_config.src_filename[255] = 0;
                    strncpy(sub_config.src_codeset, [[tempObject objectForKey:@"subtitleTrackSrtCharCode"] UTF8String], 39);
                    sub_config.src_codeset[39] = 0;
                    
                    sub_config.force = 0;
                    sub_config.dest = PASSTHRUSUB;
                    sub_config.default_track = def;
                    
                    hb_srt_add( job, &sub_config, [[tempObject objectForKey:@"subtitleTrackSrtLanguageIso3"] UTF8String]);
                    continue;
                }
                
                /* for the actual source tracks, we must subtract the non source entries so 
                 * that the menu index matches the source subtitle_list index for convenience */
                if( i == 0 )
                {
                    /* for the first track, the source tracks start at menu index 2 ( None is 0,
                     * Foreign Language Search is 1) so subtract 2 */
                    subtitle = subtitle - 2;
                }
                else
                {
                    /* for all other tracks, the source tracks start at menu index 1 (None is 0)
                     * so subtract 1. */
                    subtitle = subtitle - 1;
                }
                
                /* We are setting a source subtitle so access the source subtitle info */  
                hb_subtitle_t * subt = (hb_subtitle_t *) hb_list_item( title->list_subtitle, subtitle );
                
                if( subt != NULL )
                {
                    hb_subtitle_config_t sub_config = subt->config;
                    
                    if( !burned && hb_subtitle_can_pass( subt->source, job->mux ) )
                    {
                        sub_config.dest = PASSTHRUSUB;
                    }
                    else if( hb_subtitle_can_burn( subt->source ) )
                    {
                        // Only allow one subtitle to be burned into the video
                        if( one_burned )
                            continue;
                        one_burned = TRUE;
                        sub_config.dest = RENDERSUB;
                    }
                    
                    sub_config.force = force;
                    sub_config.default_track = def;
                    hb_subtitle_add( job, &sub_config, subtitle );
                }
            }
        }
        i++;
    }
    if( one_burned )
    {
        filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d",
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );
    }
   
    
    
[subtitlesArray autorelease];
    
    
    /* Auto Passthru */
    job->acodec_copy_mask = 0;
    if ([fAudioAllowAACPassCheck state] == NSOnState)
    {
        job->acodec_copy_mask |= HB_ACODEC_FFAAC;
    }
    if ([fAudioAllowAC3PassCheck state] == NSOnState)
    {
        job->acodec_copy_mask |= HB_ACODEC_AC3;
    }
    if ([fAudioAllowDTSHDPassCheck state] == NSOnState)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_HD;
    }
    if ([fAudioAllowDTSPassCheck state] == NSOnState)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA;
    }
    if ([fAudioAllowMP3PassCheck state] == NSOnState)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3;
    }
    job->acodec_fallback = [[fAudioFallbackPopUp selectedItem] tag];
    
    /* Audio tracks and mixdowns */
	[fAudioDelegate prepareAudioForJob: job];

    
    
    /* Filters */
    
    /* Though Grayscale is not really a filter, per se
     * we put it here since its in the filters panel
     */
     
    if ([fPictureController grayscale])
    {
        job->grayscale = 1;
    }
    else
    {
        job->grayscale = 0;
    }
    
    /* Now lets call the filters if applicable.
    * The order of the filters is critical
    */
    

	/* Detelecine */
    filter = hb_filter_init( HB_FILTER_DETELECINE );
    if ([fPictureController detelecine] == 1)
    {
        /* use a custom detelecine string */
        hb_add_filter( job, filter, [[fPictureController detelecineCustomString] UTF8String] );
    }
    else if ([fPictureController detelecine] == 2)
    {
        /* Default */
        hb_add_filter( job, filter, NULL );
    }
    
    
    
    if ([fPictureController useDecomb] == 1)
    {
        /* Decomb */
        filter = hb_filter_init( HB_FILTER_DECOMB );
        if ([fPictureController decomb] == 1)
        {
            /* use a custom decomb string */
            hb_add_filter( job, filter, [[fPictureController decombCustomString] UTF8String] );
        }
        else if ([fPictureController decomb] == 2)
        {
            /* use libhb defaults */
            hb_add_filter( job, filter, NULL );
        }
        else if ([fPictureController decomb] == 3)
        {
            /* use old defaults (decomb fast) */
            hb_add_filter( job, filter, "7:2:6:9:1:80" );
        }
        else if ([fPictureController decomb] == 4)
        {
            /* decomb 3 with bobbing enabled */
            hb_add_filter( job, filter, "455" );
        }
    }
    else
    {
        /* Deinterlace */
        filter = hb_filter_init( HB_FILTER_DEINTERLACE );
        if ([fPictureController deinterlace] == 1)
        {
            /* we add the custom string if present */
            hb_add_filter( job, filter, [[fPictureController deinterlaceCustomString] UTF8String] );            
        }
        else if ([fPictureController deinterlace] == 2)
        {
            /* Run old deinterlacer fd by default */
            hb_add_filter( job, filter, "0" );
        }
        else if ([fPictureController deinterlace] == 3)
        {
            /* Yadif mode 0 (without spatial deinterlacing) */
            hb_add_filter( job, filter, "1" );            
        }
        else if ([fPictureController deinterlace] == 4)
        {
            /* Yadif (with spatial deinterlacing) */
            hb_add_filter( job, filter, "3" );
        }
        else if ([fPictureController deinterlace] == 5)
        {
            /* Yadif (with spatial deinterlacing and bobbing) */
            hb_add_filter( job, filter, "15" );
        }
	}
    
    /* Denoise */
    filter = hb_filter_init( HB_FILTER_DENOISE );
	if ([fPictureController denoise] == 1) // custom in popup
	{
		/* we add the custom string if present */
        hb_add_filter( job, filter, [[fPictureController denoiseCustomString] UTF8String] );
	}
    else if ([fPictureController denoise] == 2) // Weak in popup
	{
        hb_add_filter( job, filter, "2:1:2:3" );
	}
	else if ([fPictureController denoise] == 3) // Medium in popup
	{
        hb_add_filter( job, filter, "3:2:2:3" );
	}
	else if ([fPictureController denoise] == 4) // Strong in popup
	{
        hb_add_filter( job, filter, "7:7:5:5" );
	}
    
    
    /* Deblock  (uses pp7 default) */
    /* NOTE: even though there is a valid deblock setting of 0 for the filter, for 
     * the macgui's purposes a value of 0 actually means to not even use the filter
     * current hb_filter_deblock.settings valid ranges are from 5 - 15 
     */
    filter = hb_filter_init( HB_FILTER_DEBLOCK );
    if ([fPictureController deblock] != 0)
    {
        NSString *deblockStringValue = [NSString stringWithFormat: @"%d",[fPictureController deblock]];
        hb_add_filter( job, filter, [deblockStringValue UTF8String] );
    }

    /* Add Crop/Scale filter */
    filter = hb_filter_init( HB_FILTER_CROP_SCALE );
    hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d:%d:%d",
                                  job->width,job->height,
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );

    /* Add framerate shaping filter */
    filter = hb_filter_init( HB_FILTER_VFR );
    hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                  job->cfr, job->vrate, job->vrate_base] UTF8String] );
}


#pragma mark -
#pragma mark Job Handling


- (void) prepareJob
{
    
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    hb_list_t  * list  = hb_get_titles( fQueueEncodeLibhb );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,0 ); // is always zero since now its a single title scan
    hb_job_t * job = title->job;
    hb_audio_config_t * audio;
    hb_filter_object_t * filter;
    /* Title Angle for dvdnav */
    job->angle = [[queueToApply objectForKey:@"TitleAngle"] intValue];
    
    if([[queueToApply objectForKey:@"fEncodeStartStop"] intValue] == 0)
    {
        /* Chapter selection */
        [self writeToActivityLog: "Start / Stop set to chapters"];
        job->chapter_start = [[queueToApply objectForKey:@"JobChapterStart"] intValue];
        job->chapter_end   = [[queueToApply objectForKey:@"JobChapterEnd"] intValue];
    }
    else if ([[queueToApply objectForKey:@"fEncodeStartStop"] intValue] == 1)
    {
        /* we are pts based start / stop */
        [self writeToActivityLog: "Start / Stop set to seconds…"];
        
        /* Point A to Point B. Time to time in seconds.*/
        /* get the start seconds from the start seconds field */
        int start_seconds = [[queueToApply objectForKey:@"StartSeconds"] intValue];
        job->pts_to_start = start_seconds * 90000LL;
        /* Stop seconds is actually the duration of encode, so subtract the end seconds from the start seconds */
        int stop_seconds = [[queueToApply objectForKey:@"StopSeconds"] intValue];
        job->pts_to_stop = stop_seconds * 90000LL;
        
    }
    else if ([[queueToApply objectForKey:@"fEncodeStartStop"] intValue] == 2)
    {
        /* we are frame based start / stop */
        [self writeToActivityLog: "Start / Stop set to frames…"];
        
        /* Point A to Point B. Frame to frame */
        /* get the start frame from the start frame field */
        int start_frame = [[queueToApply objectForKey:@"StartFrame"] intValue];
        job->frame_to_start = start_frame;
        /* get the frame to stop on from the end frame field */
        int stop_frame = [[queueToApply objectForKey:@"StopFrame"] intValue];
        job->frame_to_stop = stop_frame;
        
    }

	
        
    
    /* Format (Muxer) and Video Encoder */
    job->mux = [[queueToApply objectForKey:@"JobFileFormatMux"] intValue];
    job->vcodec = [[queueToApply objectForKey:@"JobVideoEncoderVcodec"] intValue];
    
    
    /* If mpeg-4, then set mpeg-4 specific options like chapters and > 4gb file sizes */
    if( [[queueToApply objectForKey:@"Mp4LargeFile"] intValue] == 1)
    {
        job->largeFileSize = 1;
    }
    else
    {
        job->largeFileSize = 0;
    }
    /* We set http optimized mp4 here */
    if( [[queueToApply objectForKey:@"Mp4HttpOptimize"] intValue] == 1 )
    {
        job->mp4_optimize = 1;
    }
    else
    {
        job->mp4_optimize = 0;
    }

	
    /* We set the chapter marker extraction here based on the format being
     mpeg4 or mkv and the checkbox being checked */
    if ([[queueToApply objectForKey:@"ChapterMarkers"] intValue] == 1)
    {
        job->chapter_markers = 1;
        
        /* now lets get our saved chapter names out the array in the queue file
         * and insert them back into the title chapter list. We have it here,
         * because unless we are inserting chapter markers there is no need to
         * spend the overhead of iterating through the chapter names array imo
         * Also, note that if for some reason we don't apply chapter names, the
         * chapters just come out 001, 002, etc. etc.
         */
         
        NSMutableArray *ChapterNamesArray = [queueToApply objectForKey:@"ChapterNames"];
        int i = 0;
        NSEnumerator *enumerator = [ChapterNamesArray objectEnumerator];
        id tempObject;
        while (tempObject = [enumerator nextObject])
        {
            hb_chapter_t *chapter = (hb_chapter_t *) hb_list_item( job->list_chapter, i );
            if( chapter != NULL )
            {
                hb_chapter_set_title( chapter, [tempObject UTF8String] );
            }
            i++;
        }
    }
    else
    {
        job->chapter_markers = 0;
    }
    
    if (job->vcodec == HB_VCODEC_X264)
    {
        /* iPod 5G atom */
        job->ipod_atom = ([[queueToApply objectForKey:@"Mp4iPodCompatible"]
                           intValue] == 1);
        
        /* set fastfirstpass if 2-pass and Turbo are enabled */
        if ([[queueToApply objectForKey:@"VideoTwoPass"] intValue] == 1)
        {
            job->fastfirstpass = ([[queueToApply objectForKey:@"VideoTurboTwoPass"]
                                   intValue] == 1);
        }
        
        /* advanced x264 options */
        NSString   *tmpString;
        // translate zero-length strings to NULL for libhb
        const char *x264_preset   = NULL;
        const char *x264_tune     = NULL;
        const char *advanced_opts = NULL;
        const char *h264_profile  = NULL;
        const char *h264_level    = NULL;
        if ([[queueToApply objectForKey:@"x264UseAdvancedOptions"] intValue])
        {
            // we are using the advanced panel
            if ([(tmpString = [queueToApply objectForKey:@"x264Option"]) length])
            {
                advanced_opts = [tmpString UTF8String];
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = [queueToApply objectForKey:@"x264Tune"]) length])
            {
                x264_tune = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"x264OptionExtra"]) length])
            {
                advanced_opts = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"h264Profile"]) length])
            {
                h264_profile = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"h264Level"]) length])
            {
                h264_level = [tmpString UTF8String];
            }
            x264_preset = [[queueToApply objectForKey:@"x264Preset"] UTF8String];
        }
        hb_job_set_x264_preset  (job, x264_preset);
        hb_job_set_x264_tune    (job, x264_tune);
        hb_job_set_advanced_opts(job, advanced_opts);
        hb_job_set_h264_profile (job, h264_profile);
        hb_job_set_h264_level   (job, h264_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_advanced_opts(job,
                                 [[queueToApply objectForKey:@"lavcOption"]
                                  UTF8String]);
    }
    
    
    /* Picture Size Settings */
    job->width = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
    job->height = [[queueToApply objectForKey:@"PictureHeight"]  intValue];
    
    job->keep_ratio = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    job->modulus = [[queueToApply objectForKey:@"PictureModulus"] intValue];
    if ([[queueToApply objectForKey:@"PicturePAR"]  intValue] == 3)
    {
        /* insert our custom values here for capuj */
        job->width = [[queueToApply objectForKey:@"PicturePARStorageWidth"]  intValue];
        job->height = [[queueToApply objectForKey:@"PicturePARStorageHeight"]  intValue];
        
        job->modulus = [[queueToApply objectForKey:@"PicturePARModulus"] intValue];
        
        job->anamorphic.par_width = [[queueToApply objectForKey:@"PicturePARPixelWidth"]  intValue];
        job->anamorphic.par_height = [[queueToApply objectForKey:@"PicturePARPixelHeight"]  intValue];
        
        job->anamorphic.dar_width = [[queueToApply objectForKey:@"PicturePARDisplayWidth"]  floatValue];
        job->anamorphic.dar_height = [[queueToApply objectForKey:@"PicturePARDisplayHeight"]  floatValue];
    }
    
    /* Here we use the crop values saved at the time the preset was saved */
    job->crop[0] = [[queueToApply objectForKey:@"PictureTopCrop"]  intValue];
    job->crop[1] = [[queueToApply objectForKey:@"PictureBottomCrop"]  intValue];
    job->crop[2] = [[queueToApply objectForKey:@"PictureLeftCrop"]  intValue];
    job->crop[3] = [[queueToApply objectForKey:@"PictureRightCrop"]  intValue];
    
    /* Video settings */
    /* Framerate */
    
    if( [[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue] > 0 )
    {
        /* a specific framerate has been chosen */
        job->vrate      = 27000000;
        job->vrate_base = hb_video_rates[[[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue]-1].rate;
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"cfr"])
        {
            // CFR
            job->cfr = 1;
        }
        else
        {
            // PFR
            job->cfr = 2;
        }
    }
    else
    {
        /* same as source */
        job->vrate      = [[queueToApply objectForKey:@"JobVrate"] intValue];
        job->vrate_base = [[queueToApply objectForKey:@"JobVrateBase"] intValue];
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"cfr"])
        {
            // CFR
            job->cfr = 1;
        }
        else
        {
            // VFR
            job->cfr = 0;
        }
    }
    
    if ( [[queueToApply objectForKey:@"VideoQualityType"] intValue] != 2 )
    {
        job->vquality = -1.0;
        job->vbitrate = [[queueToApply objectForKey:@"VideoAvgBitrate"] intValue];
    }
    if ( [[queueToApply objectForKey:@"VideoQualityType"] intValue] == 2 )
    {
        job->vquality = [[queueToApply objectForKey:@"VideoQualitySlider"] floatValue];
        job->vbitrate = 0;
        
    }
    
    job->grayscale = [[queueToApply objectForKey:@"VideoGrayScale"] intValue];
    


#pragma mark -
#pragma mark Process Subtitles to libhb

/* Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
 * which means that we need to account for the offset of non source language settings in from
 * the NSPopUpCell menu. For all of the objects in the SubtitleList array this means 0 is "None"
 * from the popup menu, additionally the first track has "Foreign Audio Search" at 1. So we use
 * an int to offset the index number for the objectForKey:@"subtitleSourceTrackNum" to map that
 * to the source tracks position in title->list_subtitle.
 */

int subtitle;
int force;
int burned;
int def;
bool one_burned = FALSE;

    int i = 0;
    NSEnumerator *enumerator = [[queueToApply objectForKey:@"SubtitleList"] objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        
        subtitle = [[tempObject objectForKey:@"subtitleSourceTrackNum"] intValue];
        force = [[tempObject objectForKey:@"subtitleTrackForced"] intValue];
        burned = [[tempObject objectForKey:@"subtitleTrackBurned"] intValue];
        def = [[tempObject objectForKey:@"subtitleTrackDefault"] intValue];
        
        /* since the subtitleSourceTrackNum 0 is "None" in our array of the subtitle popups,
         * we want to ignore it for display as well as encoding.
         */
        if (subtitle > 0)
        {
            /* if i is 0, then we are in the first item of the subtitles which we need to 
             * check for the "Foreign Audio Search" which would be subtitleSourceTrackNum of 1
             * bearing in mind that for all tracks subtitleSourceTrackNum of 0 is None.
             */
            
            /* if we are on the first track and using "Foreign Audio Search" */ 
            if (i == 0 && subtitle == 1)
            {
                [self writeToActivityLog: "Foreign Language Search: %d", 1];
                
                job->indepth_scan = 1;
                
                if (burned != 1)
                {
                    job->select_subtitle_config.dest = PASSTHRUSUB;
                }
                else
                {
                    job->select_subtitle_config.dest = RENDERSUB;
                }
                
                job->select_subtitle_config.force = force;
                job->select_subtitle_config.default_track = def;
            }
            else
            {
                /* if we are getting the subtitles from an external srt file */
                if ([[tempObject objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
                {
                    hb_subtitle_config_t sub_config;
                    
                    sub_config.offset = [[tempObject objectForKey:@"subtitleTrackSrtOffset"] intValue];
                    
                    /* we need to srncpy file name and codeset */
                    strncpy(sub_config.src_filename, [[tempObject objectForKey:@"subtitleSourceSrtFilePath"] UTF8String], 255);
                    sub_config.src_filename[255] = 0;
                    strncpy(sub_config.src_codeset, [[tempObject objectForKey:@"subtitleTrackSrtCharCode"] UTF8String], 39);
                    sub_config.src_codeset[39] = 0;
                    
                    sub_config.force = 0;
                    sub_config.dest = PASSTHRUSUB;
                    sub_config.default_track = def;
                    
                    hb_srt_add( job, &sub_config, [[tempObject objectForKey:@"subtitleTrackSrtLanguageIso3"] UTF8String]);
                    continue;
                }
                
                /* for the actual source tracks, we must subtract the non source entries so 
                 * that the menu index matches the source subtitle_list index for convenience */
                if( i == 0 )
                {
                    /* for the first track, the source tracks start at menu index 2 ( None is 0,
                     * Foreign Language Search is 1) so subtract 2 */
                    subtitle = subtitle - 2;
                }
                else
                {
                    /* for all other tracks, the source tracks start at menu index 1 (None is 0)
                     * so subtract 1. */
                    subtitle = subtitle - 1;
                }
                
                /* We are setting a source subtitle so access the source subtitle info */  
                hb_subtitle_t * subt = (hb_subtitle_t *) hb_list_item( title->list_subtitle, subtitle );
                
                if( subt != NULL )
                {
                    hb_subtitle_config_t sub_config = subt->config;
                    
                    if( !burned && hb_subtitle_can_pass( subt->source, job->mux ) )
                    {
                        sub_config.dest = PASSTHRUSUB;
                    }
                    else if( hb_subtitle_can_burn( subt->source ) )
                    {
                        // Only allow one subtitle to be burned into the video
                        if( one_burned )
                            continue;
                        one_burned = TRUE;
                        sub_config.dest = RENDERSUB;
                    }
                    
                    sub_config.force = force;
                    sub_config.default_track = def;
                    hb_subtitle_add( job, &sub_config, subtitle );
                }
            }
        }
        i++;
    }
    if( one_burned )
    {
        filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d",
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );
    }

#pragma mark -

    /* Auto Passthru */
    job->acodec_copy_mask = 0;
    if( [[queueToApply objectForKey: @"AudioAllowAACPass"] intValue] == 1 )
    {
        job->acodec_copy_mask |= HB_ACODEC_FFAAC;
    }
    if( [[queueToApply objectForKey: @"AudioAllowAC3Pass"] intValue] == 1 )
    {
        job->acodec_copy_mask |= HB_ACODEC_AC3;
    }
    if( [[queueToApply objectForKey: @"AudioAllowDTSHDPass"] intValue] == 1 )
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_HD;
    }
    if( [[queueToApply objectForKey: @"AudioAllowDTSPass"] intValue] == 1 )
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA;
    }
    if( [[queueToApply objectForKey: @"AudioAllowMP3Pass"] intValue] == 1 )
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3;
    }
    job->acodec_fallback = [[queueToApply objectForKey: @"JobAudioEncoderFallback"] intValue];
    
    /* Audio tracks and mixdowns */
    /* Now lets add our new tracks to the audio list here */
    for (unsigned int counter = 0; counter < maximumNumberOfAllowedAudioTracks; counter++)
    {
        NSString *prefix    = [NSString stringWithFormat:@"Audio%d",    counter + 1];
        NSString *jobPrefix = [NSString stringWithFormat:@"JobAudio%d", counter + 1];
        if ([[queueToApply objectForKey:[prefix stringByAppendingString:@"Track"]] intValue] > 0)
        {
            audio           = (hb_audio_config_t*)calloc(1, sizeof(*audio));
            hb_audio_config_init(audio);
            audio->in.track = [[queueToApply objectForKey:[prefix stringByAppendingString:@"Track"]] intValue] - 1;
            /* We go ahead and assign values to our audio->out.<properties> */
            audio->out.track                     = audio->in.track;
            audio->out.codec                     = [[queueToApply objectForKey:[jobPrefix stringByAppendingString:@"Encoder"]]           intValue];
            audio->out.compression_level         = hb_get_default_audio_compression(audio->out.codec);
            audio->out.mixdown                   = [[queueToApply objectForKey:[jobPrefix stringByAppendingString:@"Mixdown"]]           intValue];
            audio->out.normalize_mix_level       = 0;
            audio->out.bitrate                   = [[queueToApply objectForKey:[jobPrefix stringByAppendingString:@"Bitrate"]]           intValue];
            audio->out.samplerate                = [[queueToApply objectForKey:[jobPrefix stringByAppendingString:@"Samplerate"]]        intValue];
            audio->out.dynamic_range_compression = [[queueToApply objectForKey:[prefix    stringByAppendingString:@"TrackDRCSlider"]]  floatValue];
            audio->out.gain                      = [[queueToApply objectForKey:[prefix    stringByAppendingString:@"TrackGainSlider"]] floatValue];
            audio->out.dither_method             = hb_audio_dither_get_default();
            
            hb_audio_add(job, audio);
            free(audio);
        }
    }
    
    /* Now lets call the filters if applicable.
     * The order of the filters is critical
     */
    /* Detelecine */
    filter = hb_filter_init( HB_FILTER_DETELECINE );
    if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 1)
    {
        /* use a custom detelecine string */
        hb_add_filter( job, filter, [[queueToApply objectForKey:@"PictureDetelecineCustom"] UTF8String] );
    }
    else if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 2)
    {
        /* Use libhb's default values */
        hb_add_filter( job, filter, NULL );
    }
    
    if ([[queueToApply objectForKey:@"PictureDecombDeinterlace"] intValue] == 1)
    {
        /* Decomb */
        filter = hb_filter_init( HB_FILTER_DECOMB );
        if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 1)
        {
            /* use a custom decomb string */
            hb_add_filter( job, filter, [[queueToApply objectForKey:@"PictureDecombCustom"] UTF8String] );
        }
        else if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 2)
        {
            /* use libhb defaults */
            hb_add_filter( job, filter, NULL );
        }
        else if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 3)
        {
            /* use old defaults (decomb fast) */
            hb_add_filter( job, filter, "7:2:6:9:1:80" );
        }
        else if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 4)
        {
            /* decomb 3 with bobbing enabled */
            hb_add_filter( job, filter, "455" );
        }
    }
    else
    {
        /* Deinterlace */
        filter = hb_filter_init( HB_FILTER_DEINTERLACE );
        if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 1)
        {
            /* we add the custom string if present */
            hb_add_filter( job, filter, [[queueToApply objectForKey:@"PictureDeinterlaceCustom"] UTF8String] );            
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 2)
        {
            /* Run old deinterlacer fd by default */
            hb_add_filter( job, filter, "0" );
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 3)
        {
            /* Yadif mode 0 (without spatial deinterlacing) */
            hb_add_filter( job, filter, "1" );            
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 4)
        {
            /* Yadif (with spatial deinterlacing) */
            hb_add_filter( job, filter, "3" );            
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 5)
        {
            /* Yadif (with spatial deinterlacing and bobbing) */
            hb_add_filter( job, filter, "15" );            
        }
    }
    /* Denoise */
    filter = hb_filter_init( HB_FILTER_DENOISE );
	if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 1) // Custom in popup
	{
		/* we add the custom string if present */
        hb_add_filter( job, filter, [[queueToApply objectForKey:@"PictureDenoiseCustom"] UTF8String] );	
	}
    else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 2) // Weak in popup
	{
        hb_add_filter( job, filter, "2:1:2:3" );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 3) // Medium in popup
	{
        hb_add_filter( job, filter, "3:2:2:3" );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 4) // Strong in popup
	{
        hb_add_filter( job, filter, "7:7:5:5" );	
	}
    
    
    /* Deblock  (uses pp7 default) */
    /* NOTE: even though there is a valid deblock setting of 0 for the filter, for 
     * the macgui's purposes a value of 0 actually means to not even use the filter
     * current hb_filter_deblock.settings valid ranges are from 5 - 15 
     */
    filter = hb_filter_init( HB_FILTER_DEBLOCK );
    if ([[queueToApply objectForKey:@"PictureDeblock"] intValue] != 0)
    {
        hb_add_filter( job, filter, [[queueToApply objectForKey:@"PictureDeblock"] UTF8String] );
    }

    /* Add Crop/Scale filter */
    filter = hb_filter_init( HB_FILTER_CROP_SCALE );
    hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d:%d:%d",
                                  job->width,job->height,
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );

    /* Add framerate shaping filter */
    filter = hb_filter_init( HB_FILTER_VFR );
    hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                  job->cfr, job->vrate, job->vrate_base] UTF8String] );

[self writeToActivityLog: "prepareJob exiting"];    
}



/* addToQueue: puts up an alert before ultimately calling doAddToQueue
 */
- (IBAction) addToQueue: (id) sender
{
	/* We get the destination directory from the destination field here */
	NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
	/* We check for a valid destination here */
	if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
	{
		NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
        return;
	}
    
    BOOL fileExists;
    fileExists = NO;
    
    BOOL fileExistsInQueue;
    fileExistsInQueue = NO;
    
    /* We check for and existing file here */
    if([[NSFileManager defaultManager] fileExistsAtPath: [fDstFile2Field stringValue]])
    {
        fileExists = YES;
    }
    
    /* We now run through the queue and make sure we are not overwriting an exisiting queue item */
    int i = 0;
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"DestinationPath"] isEqualToString: [fDstFile2Field stringValue]])
		{
			fileExistsInQueue = YES;	
		}
        i++;
	}
    
    
	if(fileExists == YES)
    {
        NSBeginCriticalAlertSheet( NSLocalizedString( @"File already exists.", @"" ),
                                  NSLocalizedString( @"Cancel", @"" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
    }
    else if (fileExistsInQueue == YES)
    {
        NSBeginCriticalAlertSheet( NSLocalizedString( @"There is already a queue item for this destination.", @"" ),
                                  NSLocalizedString( @"Cancel", @"" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
    }
    else
    {
        [self doAddToQueue];
    }
}

/* overwriteAddToQueueAlertDone: called from the alert posted by addToQueue that asks
   the user if they want to overwrite an exiting movie file.
*/
- (void) overwriteAddToQueueAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
        [self doAddToQueue];
}

- (void) doAddToQueue
{
    [self addQueueFileItem ];
}



/* Rip: puts up an alert before ultimately calling doRip
*/
- (IBAction) Rip: (id) sender
{
    [self writeToActivityLog: "Rip: Pending queue count is %d", fPendingCount];
    /* Rip or Cancel ? */
    hb_state_t s;
    hb_get_state2( fQueueEncodeLibhb, &s );
    
    if(s.state == HB_STATE_WORKING || s.state == HB_STATE_PAUSED)
	{
        [self Cancel: sender];
        return;
    }
    
    /* We check to see if we need to warn the user that the computer will go to sleep
                 or shut down when encoding is finished */
                [self remindUserOfSleepOrShutdown];
    
    // If there are pending jobs in the queue, then this is a rip the queue
    if (fPendingCount > 0)
    {
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        /* here lets start the queue with the first pending item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 
        
        return;
    }
    
    // Before adding jobs to the queue, check for a valid destination.
    
    NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
    if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
    {
        NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
        return;
    }
    
    /* We check for duplicate name here */
    if( [[NSFileManager defaultManager] fileExistsAtPath:[fDstFile2Field stringValue]] )
    {
        NSBeginCriticalAlertSheet( NSLocalizedString( @"File already exists", @"" ),
                                  NSLocalizedString( @"Cancel", "" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overWriteAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
        
        // overWriteAlertDone: will be called when the alert is dismissed. It will call doRip.
    }
    else
    {
        /* if there are no pending jobs in the queue, then add this one to the queue and rip
         otherwise, just rip the queue */
        if(fPendingCount == 0)
        {
            [self doAddToQueue];
        }
        
        /* go right to processing the new queue encode */
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 
        
    }
}

/* overWriteAlertDone: called from the alert posted by Rip: that asks the user if they
   want to overwrite an exiting movie file.
*/
- (void) overWriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        /* if there are no jobs in the queue, then add this one to the queue and rip 
        otherwise, just rip the queue */
        if( fPendingCount == 0 )
        {
            [self doAddToQueue];
        }

        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 

    }
}

- (void) remindUserOfSleepOrShutdown
{
       if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"])
       {
               /*Warn that computer will sleep after encoding*/
               int reminduser;
               NSBeep();
               reminduser = NSRunAlertPanel(@"The computer will sleep after encoding is done.",@"You have selected to sleep the computer after encoding. To turn off sleeping, go to the HandBrake preferences.", @"OK", @"Preferences…", nil);
               [NSApp requestUserAttention:NSCriticalRequest];
               if ( reminduser == NSAlertAlternateReturn )
               {
                       [self showPreferencesWindow:nil];
               }
       }
       else if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"])
       {
               /*Warn that computer will shut down after encoding*/
               int reminduser;
               NSBeep();
               reminduser = NSRunAlertPanel(@"The computer will shut down after encoding is done.",@"You have selected to shut down the computer after encoding. To turn off shut down, go to the HandBrake preferences.", @"OK", @"Preferences…", nil);
               [NSApp requestUserAttention:NSCriticalRequest];
               if ( reminduser == NSAlertAlternateReturn )
               {
                       [self showPreferencesWindow:nil];
               }
       }

}


- (void) doRip
{
    /* Let libhb do the job */
    hb_start( fQueueEncodeLibhb );
    /* set the fEncodeState State */
    fEncodeState = 1;
}


//------------------------------------------------------------------------------------
// Displays an alert asking user if the want to cancel encoding of current job.
// Cancel: returns immediately after posting the alert. Later, when the user
// acknowledges the alert, doCancelCurrentJob is called.
//------------------------------------------------------------------------------------
- (IBAction)Cancel: (id)sender
{
    if (!fQueueController) return;
    
    /*
     * No need to allow system sleep here as we'll either call Cancel:
     * (which will take care of it) or resume right away
     */
    hb_pause(fQueueEncodeLibhb);
    
    NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"You are currently encoding. What would you like to do ?", nil)];
   
    // Which window to attach the sheet to?
    NSWindow * docWindow;
    if ([sender respondsToSelector: @selector(window)])
        docWindow = [sender window];
    else
        docWindow = fWindow;
        
    NSBeginCriticalAlertSheet(
            alertTitle,
            NSLocalizedString(@"Continue Encoding", nil),
            NSLocalizedString(@"Cancel Current and Stop", nil),
            NSLocalizedString(@"Cancel Current and Continue", nil),
            docWindow, self,
            nil, @selector(didDimissCancel:returnCode:contextInfo:), nil,
            NSLocalizedString(@"Your encode will be cancelled if you don't continue encoding.", nil));
    
    // didDimissCancelCurrentJob:returnCode:contextInfo: will be called when the dialog is dismissed
}

- (void) didDimissCancel: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    /* No need to prevent system sleep here as we didn't allow it in Cancel: */
    hb_resume(fQueueEncodeLibhb);
    
    if (returnCode == NSAlertOtherReturn)
    {
        [self doCancelCurrentJob];  // <- this also stops libhb
    }
    else if (returnCode == NSAlertAlternateReturn)
    {
        [self doCancelCurrentJobAndStop];
    }
}

//------------------------------------------------------------------------------------
// Cancels and deletes the current job and stops libhb from processing the remaining
// encodes.
//------------------------------------------------------------------------------------
- (void) doCancelCurrentJob
{
    // Stop the current job. hb_stop will only cancel the current pass and then set
    // its state to HB_STATE_WORKDONE. It also does this asynchronously. So when we
    // see the state has changed to HB_STATE_WORKDONE (in updateUI), we'll delete the
    // remaining passes of the job and then start the queue back up if there are any
    // remaining jobs.
     
    
    hb_stop(fQueueEncodeLibhb);
    hb_system_sleep_allow(fQueueEncodeLibhb);
    
    // Delete all remaining jobs since libhb doesn't do this on its own.
            hb_job_t * job;
            while( ( job = hb_job(fQueueEncodeLibhb, 0) ) )
                hb_rem( fQueueEncodeLibhb, job );
                
    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:3] forKey:@"Status"];
    // and as always, save it in Queue.plist
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    
    // and see if there are more items left in our queue
    int queueItems = [QueueFileArray count];
    /* If we still have more items in our queue, lets go to the next one */
    /* Check to see if there are any more pending items in the queue */
    int newQueueItemIndex = [self getNextPendingQueueIndex];
    /* If we still have more pending items in our queue, lets go to the next one */
    if (newQueueItemIndex >= 0 && newQueueItemIndex < queueItems)
    {
        /*Set our currentQueueEncodeIndex now to the newly found Pending encode as we own it */
        currentQueueEncodeIndex = newQueueItemIndex;
        /* now we mark the queue item as Status = 1 ( being encoded ) so another instance can not come along and try to scan it while we are scanning */
        [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
        [self writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];
        /* now we can go ahead and scan the new pending queue item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];

    }
    else
    {
        [self writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
    }
}

- (void) doCancelCurrentJobAndStop
{
    hb_stop(fQueueEncodeLibhb);
    hb_system_sleep_allow(fQueueEncodeLibhb);
    
    // Delete all remaining jobs since libhb doesn't do this on its own.
            hb_job_t * job;
            while( ( job = hb_job(fQueueEncodeLibhb, 0) ) )
                hb_rem( fQueueEncodeLibhb, job );
                
                
    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:3] forKey:@"Status"];
    // and as always, save it in Queue.plist
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    // so now lets move to 
    currentQueueEncodeIndex++ ;
    [self writeToActivityLog: "cancelling current job and stopping the queue"];
}
- (IBAction) Pause: (id) sender
{
    hb_state_t s;
    hb_get_state2(fQueueEncodeLibhb, &s);

    if (s.state == HB_STATE_PAUSED)
    {
        hb_system_sleep_prevent(fQueueEncodeLibhb);
        hb_resume(fQueueEncodeLibhb);
    }
    else
    {
        hb_pause(fQueueEncodeLibhb);
        hb_system_sleep_allow(fQueueEncodeLibhb);
    }
}

#pragma mark -
#pragma mark Batch Queue Titles Methods
- (IBAction) addAllTitlesToQueue: (id) sender
{
    NSBeginCriticalAlertSheet( NSLocalizedString( @"You are about to add ALL titles to the queue!", @"" ), 
                              NSLocalizedString( @"Cancel", @"" ), NSLocalizedString( @"Yes, I want to add all titles to the queue.", @"" ), nil, fWindow, self,
                              @selector( addAllTitlesToQueueAlertDone:returnCode:contextInfo: ),
                              NULL, NULL, [NSString stringWithFormat:
                                           NSLocalizedString( @"Current settings will be applied to all %d titles. Are you sure you want to do this?", @"" ),[fSrcTitlePopUp numberOfItems]] );
}

- (void) addAllTitlesToQueueAlertDone: (NSWindow *) sheet
                           returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
        [self doAddAllTitlesToQueue];
}

- (void) doAddAllTitlesToQueue
{
    
    /* first get the currently selected index so we can choose it again after cycling through the available titles. */
    int currentlySelectedTitle = [fSrcTitlePopUp indexOfSelectedItem];
    
    /* For each title in the fSrcTitlePopUp, select it */
    for( int i = 0; i < [fSrcTitlePopUp numberOfItems]; i++ )
    {
        [fSrcTitlePopUp selectItemAtIndex:i];
        /* Now call titlePopUpChanged to load it up */
        [self titlePopUpChanged:nil];
        /* now add the title to the queue */
        [self addToQueue:nil];   
    }
    /* Now that we are done, reselect the previously selected title.*/
    [fSrcTitlePopUp selectItemAtIndex: currentlySelectedTitle];
    /* Now call titlePopUpChanged to load it up */
    [self titlePopUpChanged:nil]; 
}

#pragma mark -
#pragma mark GUI Controls Changed Methods

- (IBAction) titlePopUpChanged: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

    /* If we are a stream type and a batch scan, grok the output file name from title->name upon title change */
    if ((title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE) &&
        hb_list_count( list ) > 1 )
    {
        /* we set the default name according to the new title->name */
        [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                         @"%@/%@.%@", [[fDstFile2Field stringValue] stringByDeletingLastPathComponent],
                                         [NSString stringWithUTF8String: title->name],
                                         [[fDstFile2Field stringValue] pathExtension]]];
        
        /* Change the source to read out the parent folder also */
        [fSrcDVD2Field setStringValue:[NSString stringWithFormat:@"%@/%@", browsedSourceDisplayName,[NSString stringWithUTF8String: title->name]]];
    }
    
    /* For point a to point b pts encoding, set the start and end fields to 0 and the title duration in seconds respectively */
    int duration = (title->hours * 3600) + (title->minutes * 60) + (title->seconds);
    [fSrcTimeStartEncodingField setStringValue: [NSString stringWithFormat: @"%d", 0]];
    [fSrcTimeEndEncodingField setStringValue: [NSString stringWithFormat: @"%d", duration]];
    /* For point a to point b frame encoding, set the start and end fields to 0 and the title duration * announced fps in seconds respectively */
    [fSrcFrameStartEncodingField setStringValue: [NSString stringWithFormat: @"%d", 1]];
    //[fSrcFrameEndEncodingField setStringValue: [NSString stringWithFormat: @"%d", ((title->hours * 3600) + (title->minutes * 60) + (title->seconds)) * 24]];
    [fSrcFrameEndEncodingField setStringValue: [NSString stringWithFormat: @"%d", duration * (title->rate / title->rate_base)]];    
    
    /* If Auto Naming is on. We create an output filename of dvd name - title number */
    if( [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"] > 0 && ( hb_list_count( list ) > 1 ) )
	{
		[fDstFile2Field setStringValue: [NSString stringWithFormat:
			@"%@/%@-%d.%@", [[fDstFile2Field stringValue] stringByDeletingLastPathComponent],
			[browsedSourceDisplayName stringByDeletingPathExtension],
            title->index,
			[[fDstFile2Field stringValue] pathExtension]]];	
	}
    /* Update encode start / stop variables */
     
    
    
    /* Update chapter popups */
    [fSrcChapterStartPopUp removeAllItems];
    [fSrcChapterEndPopUp   removeAllItems];
    for( int i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        [fSrcChapterStartPopUp addItemWithTitle: [NSString
            stringWithFormat: @"%d", i + 1]];
        [fSrcChapterEndPopUp addItemWithTitle: [NSString
            stringWithFormat: @"%d", i + 1]];
    }

    [fSrcChapterStartPopUp selectItemAtIndex: 0];
    [fSrcChapterEndPopUp   selectItemAtIndex:
        hb_list_count( title->list_chapter ) - 1];
    [self chapterPopUpChanged:nil];
    
    /* if using dvd nav, show the angle widget */
    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue])
    {
        [fSrcAngleLabel setHidden:NO];
        [fSrcAnglePopUp setHidden:NO];
        
        [fSrcAnglePopUp removeAllItems];
        for( int i = 0; i < title->angle_count; i++ )
        {
            [fSrcAnglePopUp addItemWithTitle: [NSString stringWithFormat: @"%d", i + 1]];
        }
        [fSrcAnglePopUp selectItemAtIndex: 0];
    }
    else
    {
        [fSrcAngleLabel setHidden:YES];
        [fSrcAnglePopUp setHidden:YES];
    }
    
    /* Start Get and set the initial pic size for display */
	fTitle = title;
    
    /* Set Auto Crop to on upon selecting a new title  */
    [fPictureController setAutoCrop:YES];
    
	/* We get the originial output picture width and height and put them
	in variables for use with some presets later on */
	PicOrigOutputWidth = title->width;
	PicOrigOutputHeight = title->height;
	AutoCropTop = title->crop[0];
	AutoCropBottom = title->crop[1];
	AutoCropLeft = title->crop[2];
	AutoCropRight = title->crop[3];

	/* Reset the new title in fPictureController &&  fPreviewController*/
    [fPictureController SetTitle:title];

        
    /* Update Subtitle Table */
    [fSubtitlesDelegate resetWithTitle:title];
    [fSubtitlesTable reloadData];
    

    /* Update chapter table */
    [fChapterTitlesDelegate resetWithTitle:title];
    [fChapterTable reloadData];

	/* Update audio table */
	[[NSNotificationCenter defaultCenter] postNotification:
	 [NSNotification notificationWithName: HBTitleChangedNotification
								   object: self
								 userInfo: [NSDictionary dictionaryWithObjectsAndKeys:
											[NSData dataWithBytesNoCopy: &fTitle length: sizeof(fTitle) freeWhenDone: NO], keyTitleTag,
											nil]]];
    [fVidRatePopUp selectItemAtIndex: 0];

    /* we run the picture size values through calculatePictureSizing to get all picture setting	information*/
	[self calculatePictureSizing:nil];

   /* lets call tableViewSelected to make sure that any preset we have selected is enforced after a title change */
    [self selectPreset:nil];
}

- (IBAction) encodeStartStopPopUpChanged: (id) sender;
{
    if( [fEncodeStartStopPopUp isEnabled] )
    {
        /* We are chapters */
        if( [fEncodeStartStopPopUp indexOfSelectedItem] == 0 )
        {
            [fSrcChapterStartPopUp  setHidden: NO];
            [fSrcChapterEndPopUp  setHidden: NO];
            
            [fSrcTimeStartEncodingField  setHidden: YES];
            [fSrcTimeEndEncodingField  setHidden: YES];
            
            [fSrcFrameStartEncodingField  setHidden: YES];
            [fSrcFrameEndEncodingField  setHidden: YES];
            
               [self chapterPopUpChanged:nil];   
        }
        /* We are time based (seconds) */
        else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 1)
        {
            [fSrcChapterStartPopUp  setHidden: YES];
            [fSrcChapterEndPopUp  setHidden: YES];
            
            [fSrcTimeStartEncodingField  setHidden: NO];
            [fSrcTimeEndEncodingField  setHidden: NO];
            
            [fSrcFrameStartEncodingField  setHidden: YES];
            [fSrcFrameEndEncodingField  setHidden: YES];
            
            [self startEndSecValueChanged:nil];
        }
        /* We are frame based */
        else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 2)
        {
            [fSrcChapterStartPopUp  setHidden: YES];
            [fSrcChapterEndPopUp  setHidden: YES];
            
            [fSrcTimeStartEncodingField  setHidden: YES];
            [fSrcTimeEndEncodingField  setHidden: YES];
            
            [fSrcFrameStartEncodingField  setHidden: NO];
            [fSrcFrameEndEncodingField  setHidden: NO];
            
            [self startEndFrameValueChanged:nil];
        }
    }
}

- (IBAction) chapterPopUpChanged: (id) sender
{

	/* If start chapter popup is greater than end chapter popup,
	we set the end chapter popup to the same as start chapter popup */
	if ([fSrcChapterStartPopUp indexOfSelectedItem] > [fSrcChapterEndPopUp indexOfSelectedItem])
	{
		[fSrcChapterEndPopUp selectItemAtIndex: [fSrcChapterStartPopUp indexOfSelectedItem]];
    }

		
	hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

    hb_chapter_t * chapter;
    int64_t        duration = 0;
    for( int i = [fSrcChapterStartPopUp indexOfSelectedItem];
         i <= [fSrcChapterEndPopUp indexOfSelectedItem]; i++ )
    {
        chapter = (hb_chapter_t *) hb_list_item( title->list_chapter, i );
        duration += chapter->duration;
    }
    
    duration /= 90000; /* pts -> seconds */
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
        @"%02lld:%02lld:%02lld", duration / 3600, ( duration / 60 ) % 60,
        duration % 60]];
    
    //[self calculateBitrate: sender];
    
    /* We're changing the chapter range - we may need to flip the m4v/mp4 extension */
    if ([fDstFormatPopUp indexOfSelectedItem] == 0)
        [self autoSetM4vExtension: sender];
}

- (IBAction) startEndSecValueChanged: (id) sender
{

	int duration = [fSrcTimeEndEncodingField intValue] - [fSrcTimeStartEncodingField intValue];
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
        @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60,
        duration % 60]];
    
    //[self calculateBitrate: sender];
    
}

- (IBAction) startEndFrameValueChanged: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
    hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );
    
    int duration = ([fSrcFrameEndEncodingField intValue] - [fSrcFrameStartEncodingField intValue]) / (title->rate / title->rate_base);
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
                                         @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60,
                                         duration % 60]];
    
    //[self calculateBitrate: sender];
}


- (IBAction) formatPopUpChanged: (id) sender
{
    NSString * string = [fDstFile2Field stringValue];
    int format = [fDstFormatPopUp indexOfSelectedItem];
    char * ext = NULL;
    NSMenuItem *menuItem;
    int i;
	/* Initially set the large file (64 bit formatting) output checkbox to hidden */
    [fDstMp4LargeFileCheck setHidden: YES];
    [fDstMp4HttpOptFileCheck setHidden: YES];
    [fDstMp4iPodFileCheck setHidden: YES];
    
    /* Update the Video Codec Popup */
    /* lets get the tag of the currently selected item first so we might reset it later */
    int selectedVidEncoderTag;
    selectedVidEncoderTag = [[fVidEncoderPopUp selectedItem] tag];
    
    /* Note: we now store the video encoder int values from common.c in the tags of each popup for easy retrieval later */
    [fVidEncoderPopUp removeAllItems];
    for( i = 0; i < hb_video_encoders_count; i++ )
    {
        if( ( ( format == 0 ) && ( hb_video_encoders[i].muxers & HB_MUX_MP4 ) ) ||
            ( ( format == 1 ) && ( hb_video_encoders[i].muxers & HB_MUX_MKV ) ) )
        {
            menuItem = [[fVidEncoderPopUp menu] addItemWithTitle: [NSString stringWithUTF8String: hb_video_encoders[i].human_readable_name]
                                                          action: NULL keyEquivalent: @""];
            [menuItem setTag: hb_video_encoders[i].encoder];
        }
    }
    
    /*
     * item 0 will be selected by default
     * deselect it so that we can detect whether the video encoder has changed
     */
    [fVidEncoderPopUp selectItem:nil];
    if (selectedVidEncoderTag)
    {
        // if we have a tag for previously selected encoder, try to select it
        // if this fails, [fVidEncoderPopUp selectedItem] will be nil
        // we'll handle that scenario further down
        [fVidEncoderPopUp selectItemWithTag:selectedVidEncoderTag];
    }
    
    /* Update the Auto Passtgru Fallback Codec Popup */
    /* lets get the tag of the currently selected item first so we might reset it later */
    int selectedAutoPassthruFallbackEncoderTag;
    selectedAutoPassthruFallbackEncoderTag = [[fAudioFallbackPopUp selectedItem] tag];
    
    [fAudioFallbackPopUp removeAllItems];
    for( i = 0; i < hb_audio_encoders_count; i++ )
    {
        if( !( hb_audio_encoders[i].encoder & HB_ACODEC_PASS_FLAG ) &&
             ( ( ( format == 0 ) && ( hb_audio_encoders[i].muxers & HB_MUX_MP4 ) ) ||
               ( ( format == 1 ) && ( hb_audio_encoders[i].muxers & HB_MUX_MKV ) ) ) )
        {
            menuItem = [[fAudioFallbackPopUp menu] addItemWithTitle: [NSString stringWithUTF8String: hb_audio_encoders[i].human_readable_name]
                                                             action: NULL keyEquivalent: @""];
            [menuItem setTag: hb_audio_encoders[i].encoder];
        }
    }
    
    /* if we have a previously selected auto passthru fallback encoder tag, then try to select it */
    if (selectedAutoPassthruFallbackEncoderTag)
    {
        selectedAutoPassthruFallbackEncoderTag = [fAudioFallbackPopUp selectItemWithTag: selectedAutoPassthruFallbackEncoderTag];
    }
    /* if we had no previous fallback selected OR if selection failed
     * select the default fallback encoder (AC3) */
    if (!selectedAutoPassthruFallbackEncoderTag)
    {
        [fAudioFallbackPopUp selectItemWithTag: HB_ACODEC_AC3];
    }
    
    switch( format )
    {
        case 0:
			[self autoSetM4vExtension: nil];
            /* We show the mp4 option checkboxes here since we are mp4 */
            [fCreateChapterMarkers setEnabled: YES];
			[fDstMp4LargeFileCheck setHidden: NO];
			[fDstMp4HttpOptFileCheck setHidden: NO];
            [fDstMp4iPodFileCheck setHidden: NO];
            break;
            
        case 1:
            ext = "mkv";
            /* We enable the create chapters checkbox here */
			[fCreateChapterMarkers setEnabled: YES];
			break;
            

    }
    /* tell fSubtitlesDelegate we have a new video container */
    
    [fSubtitlesDelegate containerChanged:[[fDstFormatPopUp selectedItem] tag]];
    [fSubtitlesTable reloadData];
	
	/* post a notification for any interested observers to indicate that our video container has changed */
	[[NSNotificationCenter defaultCenter] postNotification:
	 [NSNotification notificationWithName: HBContainerChangedNotification
								   object: self
								 userInfo: [NSDictionary dictionaryWithObjectsAndKeys:
											[NSNumber numberWithInt: [[fDstFormatPopUp selectedItem] tag]], keyContainerTag,
											nil]]];

    if( format == 0 )
        [self autoSetM4vExtension: sender];
    else
        [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@.%s", [string stringByDeletingPathExtension], ext]];

    if (SuccessfulScan)
    {
        if ([fVidEncoderPopUp selectedItem] == nil)
        {
            /* this means the above call to selectItemWithTag failed */
            [fVidEncoderPopUp selectItemAtIndex:0];
            [self videoEncoderPopUpChanged:nil];
        }
    }
	[self customSettingUsed:sender];
}

- (IBAction) autoSetM4vExtension: (id) sender
{
    if ( [fDstFormatPopUp indexOfSelectedItem] )
        return;
    
    NSString * extension = @"mp4";
    
    BOOL anyCodecAC3 = [fAudioDelegate anyCodecMatches: HB_ACODEC_AC3] || [fAudioDelegate anyCodecMatches: HB_ACODEC_AC3_PASS];
    /* Chapter markers are enabled if the checkbox is ticked and we are doing p2p or we have > 1 chapter */
    BOOL chapterMarkers = ([fCreateChapterMarkers state] == NSOnState) &&
                          ([fEncodeStartStopPopUp indexOfSelectedItem] != 0 ||
                           [fSrcChapterStartPopUp indexOfSelectedItem] < [fSrcChapterEndPopUp indexOfSelectedItem]);
	
    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultMpegExtension"] isEqualToString: @".m4v"] || 
        ((YES == anyCodecAC3 || YES == chapterMarkers) &&
         [[[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultMpegExtension"] isEqualToString: @"Auto"] ))
    {
        extension = @"m4v";
    }
    
    if( [extension isEqualTo: [[fDstFile2Field stringValue] pathExtension]] )
        return;
    else
        [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@.%@",
                                         [[fDstFile2Field stringValue] stringByDeletingPathExtension], extension]];
}

/* Method to determine if we should change the UI
To reflect whether or not a Preset is being used or if
the user is using "Custom" settings by determining the sender*/
- (IBAction) customSettingUsed: (id) sender
{
	if ([sender stringValue])
	{
		/* Deselect the currently selected Preset if there is one*/
		[fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
	}
[self calculateBitrate:nil];
}


#pragma mark -
#pragma mark - Video

- (IBAction) videoEncoderPopUpChanged: (id) sender
{
    int videoEncoder = [[fVidEncoderPopUp selectedItem] tag];
    
    [fAdvancedOptions setHidden:YES];
    /* If we are using x264 then show the x264 advanced panel and the x264 presets box */
    if (videoEncoder == HB_VCODEC_X264)
    {
        [fAdvancedOptions setHidden:NO];
        
        // show the x264 presets box
        [fX264PresetsBox setHidden:NO];
                
        [self autoSetM4vExtension: sender];
    }
    else // we are FFmpeg (lavc) or Theora
    {
        [fAdvancedOptions setHidden:YES];
        [fX264PresetsBox setHidden:YES];
        
        // We Are Lavc
        if ([[fVidEncoderPopUp selectedItem] tag] & HB_VCODEC_FFMPEG_MASK)
        {
            [fAdvancedOptions setLavcOptsEnabled:YES];
        }
        else /// We are Theora
        {
            [fAdvancedOptions setLavcOptsEnabled:NO];  
        }
    }


    if (videoEncoder != HB_VCODEC_X264)
    {
        /* We set the iPod atom checkbox to disabled and uncheck it as its only for x264 in the mp4
         * container. Format is taken care of in formatPopUpChanged method by hiding and unchecking
         * anything other than MP4. */ 
        [fDstMp4iPodFileCheck setEnabled: NO];
        [fDstMp4iPodFileCheck setState: NSOffState];
    }
    else
    {
        [fDstMp4iPodFileCheck setEnabled: YES];
    }
    [self setupQualitySlider];
	[self calculatePictureSizing: sender];
	[self twoPassCheckboxChanged: sender];
}


- (IBAction) twoPassCheckboxChanged: (id) sender
{
	/* check to see if x264 is chosen */
	if([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264)
    {
		if( [fVidTwoPassCheck state] == NSOnState)
		{
			[fVidTurboPassCheck setHidden: NO];
		}
		else
		{
			[fVidTurboPassCheck setHidden: YES];
			[fVidTurboPassCheck setState: NSOffState];
		}
		/* Make sure Two Pass is checked if Turbo is checked */
		if( [fVidTurboPassCheck state] == NSOnState)
		{
			[fVidTwoPassCheck setState: NSOnState];
		}
	}
	else
	{
		[fVidTurboPassCheck setHidden: YES];
		[fVidTurboPassCheck setState: NSOffState];
	}
	
	/* We call method method to change UI to reflect whether a preset is used or not*/
	[self customSettingUsed: sender];
}

- (IBAction ) videoFrameRateChanged: (id) sender
{
    /* Hide and set the PFR Checkbox to OFF if we are set to Same as Source */
    /* Depending on whether or not Same as source is selected modify the title for
     * fFramerateVfrPfrCell*/
    if ([fVidRatePopUp indexOfSelectedItem] == 0) // We are Same as Source
    {
        [fFramerateVfrPfrCell setTitle:@"Variable Framerate"];
    }
    else
    {
        [fFramerateVfrPfrCell setTitle:@"Peak Framerate (VFR)"];


    }
    
    /* We call method method to calculatePictureSizing to error check detelecine*/
    [self calculatePictureSizing: sender];

    /* We call method method to change UI to reflect whether a preset is used or not*/
	[self customSettingUsed: sender];
}

- (IBAction) videoMatrixChanged: (id) sender;
{
    /* We use the selectedCell: tag of the fVidQualityMatrix instead of selectedRow
     * so that the order of the video controls can be switched around.
     * Constant quality is 1 and Average bitrate is 0 for reference. */
    bool bitrate, quality;
    bitrate = quality = false;
    if( [fVidQualityMatrix isEnabled] )
    {
        switch( [[fVidQualityMatrix selectedCell] tag] )
        {
            case 0:
                bitrate = true;
                break;
            case 1:
                quality = true;
                break;
        }
    }

    [fVidBitrateField     setEnabled: bitrate];
    [fVidQualitySlider    setEnabled: quality];
    [fVidQualityRFField   setEnabled: quality];
    [fVidQualityRFLabel    setEnabled: quality];
    [fVidTwoPassCheck     setEnabled: !quality &&
     [fVidQualityMatrix isEnabled]];
    if( quality )
    {
        [fVidTwoPassCheck setState: NSOffState];
		[fVidTurboPassCheck setHidden: YES];
		[fVidTurboPassCheck setState: NSOffState];
    }

    [self qualitySliderChanged: sender];
    //[self calculateBitrate: sender];
	[self customSettingUsed: sender];
}

/* Use this method to setup the quality slider for cq/rf values depending on
 * the video encoder selected.
 */
- (void) setupQualitySlider
{
    /* Get the current slider maxValue to check for a change in slider scale later
     * so that we can choose a new similar value on the new slider scale */
    float previousMaxValue = [fVidQualitySlider maxValue];
    float previousPercentOfSliderScale = [fVidQualitySlider floatValue] / ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue] + 1);
    NSString * qpRFLabelString = @"QP:";
    /* x264 0-51 */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264)
    {
        [fVidQualitySlider setMinValue:0.0];
        [fVidQualitySlider setMaxValue:51.0];
        /* As x264 allows for qp/rf values that are fractional, we get the value from the preferences */
        int fractionalGranularity = 1 / [[NSUserDefaults standardUserDefaults] floatForKey:@"x264CqSliderFractional"];
        [fVidQualitySlider setNumberOfTickMarks:(([fVidQualitySlider maxValue] - [fVidQualitySlider minValue]) * fractionalGranularity) + 1];
        qpRFLabelString = @"RF:";
    }
    /* FFmpeg MPEG-2/4 1-31 */
    if ([[fVidEncoderPopUp selectedItem] tag] & HB_VCODEC_FFMPEG_MASK )
    {
        [fVidQualitySlider setMinValue:1.0];
        [fVidQualitySlider setMaxValue:31.0];
        [fVidQualitySlider setNumberOfTickMarks:31];
    }
    /* Theora 0-63 */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
    {
        [fVidQualitySlider setMinValue:0.0];
        [fVidQualitySlider setMaxValue:63.0];
        [fVidQualitySlider setNumberOfTickMarks:64];
    }
    [fVidQualityRFLabel setStringValue:qpRFLabelString];
    
    /* check to see if we have changed slider scales */
    if (previousMaxValue != [fVidQualitySlider maxValue])
    {
        /* if so, convert the old setting to the new scale as close as possible based on percentages */
        float rf =  ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue] + 1) * previousPercentOfSliderScale;
        [fVidQualitySlider setFloatValue:rf];
    }
    
    [self qualitySliderChanged:nil];
}

- (IBAction) qualitySliderChanged: (id) sender
{
    
    /* Our constant quality slider is in a range based
     * on each encoders qp/rf values. The range depends
     * on the encoder. Also, the range is inverse of quality
     * for all of the encoders *except* for theora
     * (ie. as the "quality" goes up, the cq or rf value
     * actually goes down). Since the IB sliders always set
     * their max value at the right end of the slider, we
     * will calculate the inverse, so as the slider floatValue
     * goes up, we will show the inverse in the rf field
     * so, the floatValue at the right for x264 would be 51
     * and our rf field needs to show 0 and vice versa.
     */
    
    float sliderRfInverse = ([fVidQualitySlider maxValue] - [fVidQualitySlider floatValue]) + [fVidQualitySlider minValue];
    /* If the encoder is theora, use the float, otherwise use the inverse float*/
    //float sliderRfToPercent;
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
    {
        [fVidQualityRFField setStringValue: [NSString stringWithFormat: @"%.2f", [fVidQualitySlider floatValue]]];   
    }
    else
    {
        [fVidQualityRFField setStringValue: [NSString stringWithFormat: @"%.2f", sliderRfInverse]];
    }
    /* Show a warning if x264 and rf 0 which is lossless */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264 && sliderRfInverse == 0.0)
    {
        [fVidQualityRFField setStringValue: [NSString stringWithFormat: @"%.2f (Warning: Lossless)", sliderRfInverse]];
    }
    
    [self customSettingUsed: sender];
}


- (void) controlTextDidChange: (NSNotification *) notification
{
    [self calculateBitrate:nil];
}

- (IBAction) calculateBitrate: (id) sender
{
    if( !fHandle || ![fVidQualityMatrix selectedRow] || !SuccessfulScan )
    {
        return;
    }

    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    /* For  hb_calc_bitrate in addition to the Target Size in MB out of the
     * Target Size Field, we also need the job info for the Muxer, the Chapters
     * as well as all of the audio track info.
     * This used to be accomplished by simply calling prepareJob here, however
     * since the resilient queue sets the queue array values instead of the job
     * values directly, we duplicate the old prepareJob code here for the variables
     * needed
     */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end = [fSrcChapterEndPopUp indexOfSelectedItem] + 1; 
    job->mux = [[fDstFormatPopUp selectedItem] tag];
    
    /* Audio goes here */
	[fAudioDelegate prepareAudioForJob: job];
       
}

#pragma mark -
#pragma mark - Video x264 Presets

- (IBAction) setupX264PresetsWidgets: (id) sender
{
    NSUInteger i;
    /*
     * now we populate the x264 preset system widgets via hb_x264_presets(),
     * hb_x264_tunes(), hb_h264_profiles(), hb_h264_levels()
     */
    // store x264 preset names
    const char * const * x264_presets = hb_x264_presets();
    NSMutableArray *tmp_array = [[NSMutableArray alloc] init];
    for (i = 0; x264_presets[i] != NULL; i++)
    {
        [tmp_array addObject:[NSString stringWithUTF8String:x264_presets[i]]];
        if (!strcasecmp(x264_presets[i], "medium"))
        {
            fX264MediumPresetIndex = i;
        }
    }
    fX264PresetNames = [[NSArray alloc] initWithArray:tmp_array];
    [tmp_array release];
    // setup the x264 preset slider
    [fX264PresetsSlider setMinValue:0];
    [fX264PresetsSlider setMaxValue:[fX264PresetNames count]-1];
    [fX264PresetsSlider setNumberOfTickMarks:[fX264PresetNames count]];
    [fX264PresetsSlider setIntegerValue:fX264MediumPresetIndex];
    [fX264PresetsSlider setTickMarkPosition:NSTickMarkAbove];
    [fX264PresetsSlider setAllowsTickMarkValuesOnly:YES];
    [self x264PresetsSliderChanged: sender];
    // setup the x264 tune popup
    [fX264TunePopUp removeAllItems];
    [fX264TunePopUp addItemWithTitle: @"none"];
    const char * const * x264_tunes = hb_x264_tunes();
    for (int i = 0; x264_tunes[i] != NULL; i++)
    {
        // we filter out "fastdecode" as we have a dedicated checkbox for it
        if (strcasecmp(x264_tunes[i], "fastdecode") != 0)
        {
            [fX264TunePopUp addItemWithTitle: [NSString stringWithUTF8String:x264_tunes[i]]];
        }
    }
    // the fastdecode checkbox is off by default
    [fX264FastDecodeCheck setState: NSOffState];
    // setup the h264 profile popup
    [fX264ProfilePopUp removeAllItems];
    const char * const * h264_profiles = hb_h264_profiles();
    for (int i = 0; h264_profiles[i] != NULL; i++)
    {
        [fX264ProfilePopUp addItemWithTitle: [NSString stringWithUTF8String:h264_profiles[i]]];
    }
    // setup the h264 level popup
    [fX264LevelPopUp removeAllItems];
    const char * const * h264_levels = hb_h264_levels();
    for (int i = 0; h264_levels[i] != NULL; i++)
    {
        [fX264LevelPopUp addItemWithTitle: [NSString stringWithUTF8String:h264_levels[i]]];
    }
    // clear the additional x264 options
    [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:@""];
}

- (void) enableX264Widgets: (bool) enable
{
    NSControl *controls[] =
    {
        fX264PresetsSlider, fX264PresetSliderLabel, fX264PresetSelectedTextField,
        fX264TunePopUp, fX264TunePopUpLabel, fX264FastDecodeCheck,
        fDisplayX264PresetsAdditonalOptionsTextField, fDisplayX264PresetsAdditonalOptionsLabel,
        fX264ProfilePopUp, fX264ProfilePopUpLabel,
        fX264LevelPopUp, fX264LevelPopUpLabel,
        fDisplayX264PresetsUnparseTextField,
    };
    
    // check whether the x264 preset system and the advanced panel should be enabled
    BOOL enable_x264_controls  = (enable && [fX264UseAdvancedOptionsCheck state] == NSOffState);
    BOOL enable_advanced_panel = (enable && [fX264UseAdvancedOptionsCheck state] == NSOnState);
    
    // enable/disable the checkbox and advanced panel
    [fX264UseAdvancedOptionsCheck setEnabled:enable];
    [fAdvancedOptions enableUI:enable_advanced_panel];
    
    // enable/disable the x264 preset system controls
    for (unsigned i = 0; i < (sizeof(controls) / sizeof(NSControl*)); i++)
    {
        if ([[controls[i] className] isEqualToString: @"NSTextField"])
        {
            NSTextField *tf = (NSTextField*)controls[i];
            if (![tf isBezeled])
            {
                [tf setTextColor:(enable_x264_controls       ?
                                  [NSColor controlTextColor] :
                                  [NSColor disabledControlTextColor])];
                continue;
            }
        }
        [controls[i] setEnabled:enable_x264_controls];
    }
}

- (IBAction) updateX264Widgets: (id) sender
{
    if ([fX264UseAdvancedOptionsCheck state] == NSOnState)
    {
        /*
         * we are using or switching to the advanced panel
         *
         * if triggered by selectPreset or applyQueueSettingToMainWindow,
         * the options string will have been specified explicitly - leave it.
         *
         * if triggered by the advanced panel on/off checkbox, set the options
         * string to the value of the unparsed x264 preset system string.
         */
        if (sender == fX264UseAdvancedOptionsCheck)
        {
            if (fX264PresetsUnparsedUTF8String != NULL)
            {
                [fAdvancedOptions setOptions:
                 [NSString stringWithUTF8String:fX264PresetsUnparsedUTF8String]];
            }
            else
            {
                [fAdvancedOptions setOptions:@""];
            }
        }
    }
    // enable/disable, populate and update the various widgets
    [self             enableX264Widgets:       YES];
    [self             x264PresetsSliderChanged:nil];
    [fAdvancedOptions X264AdvancedOptionsSet:  nil];
}

#pragma mark -
#pragma mark x264 preset system

- (NSString*) x264Preset
{
    return (NSString*)[fX264PresetNames objectAtIndex:[fX264PresetsSlider intValue]];
}

- (NSString*) x264Tune
{
    NSString *x264Tune = @"";
    if ([fX264TunePopUp indexOfSelectedItem])
    {
        x264Tune = [x264Tune stringByAppendingString:
                    [fX264TunePopUp titleOfSelectedItem]];
    }
    if ([fX264FastDecodeCheck state])
    {
        if ([x264Tune length])
        {
            x264Tune = [x264Tune stringByAppendingString: @","];
        }
        x264Tune = [x264Tune stringByAppendingString: @"fastdecode"];
    }
    return x264Tune;
}

- (NSString*) x264OptionExtra
{
    return [fDisplayX264PresetsAdditonalOptionsTextField stringValue];
}

- (NSString*) h264Profile
{
    if ([fX264ProfilePopUp indexOfSelectedItem])
    {
        return [fX264ProfilePopUp titleOfSelectedItem];
    }
    return @"";
}

- (NSString*) h264Level
{
    if ([fX264LevelPopUp indexOfSelectedItem])
    {
        return [fX264LevelPopUp titleOfSelectedItem];
    }
    return @"";
}

- (void) setX264Preset: (NSString*)x264Preset
{
    if (x264Preset)
    {
        NSString *name;
        NSEnumerator *enumerator = [fX264PresetNames objectEnumerator];
        while ((name = (NSString *)[enumerator nextObject]))
        {
            if ([name isEqualToString:x264Preset])
            {
                [fX264PresetsSlider setIntegerValue:
                 [fX264PresetNames indexOfObject:name]];
                return;
            }
        }
    }
    [fX264PresetsSlider setIntegerValue:fX264MediumPresetIndex];
}

- (void) setX264Tune: (NSString*)x264Tune
{
    if (!x264Tune)
    {
        [fX264TunePopUp selectItemAtIndex:0];
        [fX264FastDecodeCheck setState:NSOffState];
        return;
    }
    // handle fastdecode
    if ([x264Tune rangeOfString:@"fastdecode"].location != NSNotFound)
    {
        [fX264FastDecodeCheck setState:NSOnState];
    }
    else
    {
        [fX264FastDecodeCheck setState:NSOffState];
    }
    // filter out fastdecode
    x264Tune = [x264Tune stringByReplacingOccurrencesOfString:@","
                                                   withString:@""];
    x264Tune = [x264Tune stringByReplacingOccurrencesOfString:@"fastdecode"
                                                   withString:@""];
    // set the tune
    [fX264TunePopUp selectItemWithTitle:x264Tune];
    // fallback
    if ([fX264TunePopUp indexOfSelectedItem] == -1)
    {
        [fX264TunePopUp selectItemAtIndex:0];
    }
}

- (void) setX264OptionExtra: (NSString*)x264OptionExtra
{
    if (!x264OptionExtra)
    {
        [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:@""];
        return;
    }
    [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:x264OptionExtra];
}

- (void) setH264Profile: (NSString*)h264Profile
{
    if (!h264Profile)
    {
        [fX264ProfilePopUp selectItemAtIndex:0];
        return;
    }
    // set the profile
    [fX264ProfilePopUp selectItemWithTitle:h264Profile];
    // fallback
    if ([fX264ProfilePopUp indexOfSelectedItem] == -1)
    {
        [fX264ProfilePopUp selectItemAtIndex:0];
    }
}

- (void) setH264Level: (NSString*)h264Level
{
    if (!h264Level)
    {
        [fX264LevelPopUp selectItemAtIndex:0];
        return;
    }
    // set the level
    [fX264LevelPopUp selectItemWithTitle:h264Level];
    // fallback
    if ([fX264LevelPopUp indexOfSelectedItem] == -1)
    {
        [fX264LevelPopUp selectItemAtIndex:0];
    }
}


- (IBAction) x264PresetsSliderChanged: (id) sender
{ 
    // we assume the preset names and slider were setup properly
    [fX264PresetSelectedTextField setStringValue: [self x264Preset]];
    [self x264PresetsChangedDisplayExpandedOptions:nil];
    
}

/* This is called everytime a x264 widget in the video tab is changed to 
   display the expanded options in a text field via outlet fDisplayX264PresetsUnparseTextField
 */
- (IBAction) x264PresetsChangedDisplayExpandedOptions: (id) sender

{
   /* API reference:
    *
    * char * hb_x264_param_unparse(const char *x264_preset,
    *                              const char *x264_tune,
    *                              const char *x264_encopts,
    *                              const char *h264_profile,
    *                              const char *h264_level,
    *                              int width, int height);
    */
    NSString   *tmpString;
    const char *x264_preset   = [[self x264Preset] UTF8String];
    const char *x264_tune     = NULL;
    const char *advanced_opts = NULL;
    const char *h264_profile  = NULL;
    const char *h264_level    = NULL;
    int         width         = 1;
    int         height        = 1;
    // prepare the tune, advanced options, profile and level
    if ([(tmpString = [self x264Tune]) length])
    {
        x264_tune = [tmpString UTF8String];
    }
    if ([(tmpString = [self x264OptionExtra]) length])
    {
        advanced_opts = [tmpString UTF8String];
    }
    if ([(tmpString = [self h264Profile]) length])
    {
        h264_profile = [tmpString UTF8String];
    }
    if ([(tmpString = [self h264Level]) length])
    {
        h264_level = [tmpString UTF8String];
    }
    // width and height must be non-zero
    if (fX264PresetsWidthForUnparse && fX264PresetsHeightForUnparse)
    {
        width  = fX264PresetsWidthForUnparse;
        height = fX264PresetsHeightForUnparse;
    }
    // free the previous unparsed string
    free(fX264PresetsUnparsedUTF8String);
    // now, unparse
    fX264PresetsUnparsedUTF8String = hb_x264_param_unparse(x264_preset,
                                                           x264_tune,
                                                           advanced_opts,
                                                           h264_profile,
                                                           h264_level,
                                                           width, height);
    // update the text field
    if (fX264PresetsUnparsedUTF8String != NULL)
    {
        [fDisplayX264PresetsUnparseTextField setStringValue:
         [NSString stringWithFormat:@"x264 Unparse: %s",
          fX264PresetsUnparsedUTF8String]];
    }
    else
    {
        [fDisplayX264PresetsUnparseTextField setStringValue:@"x264 Unparse:"];
    }
}

#pragma mark -
#pragma mark - Picture

/* lets set the picture size back to the max from right after title scan
   Lets use an IBAction here as down the road we could always use a checkbox
   in the gui to easily take the user back to max. Remember, the compiler
   resolves IBActions down to -(void) during compile anyway */
- (IBAction) revertPictureSizeToMax: (id) sender
{
	hb_job_t * job = fTitle->job;
	/* Here we apply the title source and height */
    job->width = fTitle->width;
    job->height = fTitle->height;
    
    [self calculatePictureSizing: sender];
    /* We call method to change UI to reflect whether a preset is used or not*/    
    [self customSettingUsed: sender];
}

/**
 * Registers changes made in the Picture Settings Window.
 */

- (void)pictureSettingsDidChange 
{
	[self calculatePictureSizing:nil];
}

/* Get and Display Current Pic Settings in main window */
- (IBAction) calculatePictureSizing: (id) sender
{
    if (fTitle->job->anamorphic.mode > 0)
    {
        fTitle->job->keep_ratio = 0;
    }
    
    // align picture settings and video filters in the UI using tabs
    [fPictureSettingsField setStringValue:[NSString stringWithFormat:@"Picture Settings:  \t %@",
                                           [self pictureSettingsSummary]]];
    [fPictureFiltersField  setStringValue:[NSString stringWithFormat:@"Picture Filters: \t\t %@",
                                           [self pictureFiltersSummary]]];
    
    /* Store storage resolution for unparse */
    fX264PresetsWidthForUnparse  = fTitle->job->width;
    fX264PresetsHeightForUnparse = fTitle->job->height;
    // width or height may have changed, unparse
    [self x264PresetsChangedDisplayExpandedOptions:nil];
    
    // reload still previews
    // note: fTitle->job->deinterlace is set by fPictureController now
    [fPictureController decombDeinterlacePreviewImage];
}

#pragma mark -
#pragma mark - Text Summaries

- (NSString*) pictureSettingsSummary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    if (fPictureController && fTitle && fTitle->job)
    {
        [summary appendString:[fPictureController getPictureSizeInfoString]];
        if (fTitle->job->anamorphic.mode != 1)
        {
            // anamorphic is not Strict, show the modulus
            [summary appendFormat:@", Modulus: %d", fTitle->job->modulus];
        }
        [summary appendFormat:@", Crop: %s %d/%d/%d/%d",
         [fPictureController autoCrop] ? "Auto" : "Custom",
         fTitle->job->crop[0], fTitle->job->crop[1],
         fTitle->job->crop[2], fTitle->job->crop[3]];
    }
    return [NSString stringWithString:summary];
}

- (NSString*) pictureFiltersSummary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    if (fPictureController)
    {
        /* Detelecine */
        switch ([fPictureController detelecine])
        {
            case 1:
                [summary appendFormat:@" - Detelecine (%@)",
                 [fPictureController detelecineCustomString]];
                break;
                
            case 2:
                [summary appendString:@" - Detelecine (Default)"];
                break;
                
            default:
                break;
        }
        
        if ([fPictureController useDecomb] == 1)
        {
            /* Decomb */
            switch ([fPictureController decomb])
            {
                case 1:
                    [summary appendFormat:@" - Decomb (%@)",
                     [fPictureController decombCustomString]];
                    break;
                    
                case 2:
                    [summary appendString:@" - Decomb (Default)"];
                    break;
                    
                case 3:
                    [summary appendString:@" - Decomb (Fast)"];
                    break;
                    
                case 4:
                    [summary appendString:@" - Decomb (Bob)"];
                    break;
                    
                default:
                    break;
            }
        }
        else
        {
            /* Deinterlace */
            switch ([fPictureController deinterlace])
            {
                case 1:
                    [summary appendFormat:@" - Deinterlace (%@)",
                     [fPictureController deinterlaceCustomString]];
                    break;
                    
                case 2:
                    [summary appendString:@" - Deinterlace (Fast)"];
                    break;
                    
                case 3:
                    [summary appendString:@" - Deinterlace (Slow)"];
                    break;
                    
                case 4:
                    [summary appendString:@" - Deinterlace (Slower)"];
                    break;
                    
                case 5:
                    [summary appendString:@" - Deinterlace (Bob)"];
                    break;
                    
                default:
                    break;
            }
        }
        
        /* Deblock */
        if ([fPictureController deblock] > 0)
        {
            [summary appendFormat:@" - Deblock (%d)",
             [fPictureController deblock]];
        }
        
        /* Denoise */
        switch ([fPictureController denoise])
        {
            case 1:
                [summary appendFormat:@" - Denoise (%@)",
                 [fPictureController denoiseCustomString]];
                break;
                
            case 2:
                [summary appendString:@" - Denoise (Weak)"];
                break;
                
            case 3:
                [summary appendString:@" - Denoise (Medium)"];
                break;
                
            case 4:
                [summary appendString:@" - Denoise (Strong)"];
                break;
                
            default:
                break;
        }
        
        /* Grayscale */
        if ([fPictureController grayscale]) 
        {
            [summary appendString:@" - Grayscale"];
        }
    }
    if ([summary hasPrefix:@" - "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 3)];
    }
    return [NSString stringWithString:summary];
}

- (NSString*) muxerOptionsSummary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    if (([fDstFormatPopUp selectedItem]) &&
        [[fDstFormatPopUp selectedItem] tag] == HB_MUX_MP4)
    {
        if ([fDstMp4LargeFileCheck state])
        {
            [summary appendString:@" - Large file size"];
        }
        if ([fDstMp4HttpOptFileCheck state])
        {
            [summary appendString:@" - Web optimized"];
        }
        if ([fDstMp4iPodFileCheck state])
        {
            [summary appendString:@" - iPod 5G support"];
        }
    }
    if ([summary hasPrefix:@" - "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 3)];
    }
    return [NSString stringWithString:summary];
}

#pragma mark -
#pragma mark - Audio and Subtitles


//	This causes all audio tracks from the title to be used based on the current preset
- (IBAction) addAllAudioTracks: (id) sender

{
    [fAudioDelegate	addAllTracksFromPreset:[self selectedPreset]];
    return;
}

- (IBAction) browseImportSrtFile: (id) sender
{

    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: NO ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSrtImportDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSrtImportDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse srt sheet here and call for browseImportSrtFileDone after the sheet is closed */
    NSArray *fileTypes = [NSArray arrayWithObjects:@"plist", @"srt", nil];
    [panel beginSheetForDirectory: sourceDirectory file: nil types: fileTypes
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseImportSrtFileDone:returnCode:contextInfo: )
                      contextInfo: sender];
}

- (void) browseImportSrtFileDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *importSrtDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *importSrtFilePath = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:importSrtDirectory forKey:@"LastSrtImportDirectory"];
        
        /* now pass the string off to fSubtitlesDelegate to add the srt file to the dropdown */
        [fSubtitlesDelegate createSubtitleSrtTrack:importSrtFilePath];
        
        [fSubtitlesTable reloadData];
        
    }
}                                           

#pragma mark -
#pragma mark Open New Windows

- (IBAction) openHomepage: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.fr/"]];
}

- (IBAction) openForums: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://forum.handbrake.fr/"]];
}
- (IBAction) openUserGuide: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://trac.handbrake.fr/wiki/HandBrakeGuide"]];
}

/**
 * Shows debug output window.
 */
- (IBAction)showDebugOutputPanel:(id)sender
{
    [outputPanel showOutputPanel:sender];
}

/**
 * Shows preferences window.
 */
- (IBAction) showPreferencesWindow: (id) sender
{
    NSWindow * window = [fPreferencesController window];
    if (![window isVisible])
        [window center];

    [window makeKeyAndOrderFront: nil];
}

/**
 * Shows queue window.
 */
- (IBAction) showQueueWindow:(id)sender
{
    [fQueueController showQueueWindow:sender];
}


- (IBAction) toggleDrawer:(id)sender
{
    [fPresetDrawer toggle:self];
}

/**
 * Shows Picture Settings Window.
 */

- (IBAction) showPicturePanel: (id) sender
{
	[fPictureController showPictureWindow:sender];
}

- (void) picturePanelWindowed
{
	[fPictureController setToWindowedMode];
}

- (IBAction) showPreviewWindow: (id) sender
{
	[fPictureController showPreviewWindow:sender];
}

#pragma mark -
#pragma mark Preset Outline View Methods
#pragma mark - Required
/* These are required by the NSOutlineView Datasource Delegate */


/* used to specify the number of levels to show for each item */
- (int)outlineView:(NSOutlineView *)fPresetsOutlineView numberOfChildrenOfItem:(id)item
{
    /* currently use no levels to test outline view viability */
    if (item == nil) // for an outline view the root level of the hierarchy is always nil
    {
        return [UserPresets count];
    }
    else
    {
        /* we need to return the count of the array in ChildrenArray for this folder */
        NSArray *children = nil;
        children = [item objectForKey:@"ChildrenArray"];
        if ([children count] > 0)
        {
            return [children count];
        }
        else
        {
            return 0;
        }
    }
}

/* We use this to deterimine children of an item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView child:(NSInteger)index ofItem:(id)item
{
    
    /* we need to return the count of the array in ChildrenArray for this folder */
    NSArray *children = nil;
    if (item == nil)
    {
        children = UserPresets;
    }
    else
    {
        if ([item objectForKey:@"ChildrenArray"])
        {
            children = [item objectForKey:@"ChildrenArray"];
        }
    }   
    if ((children == nil) || ( [children count] <= (NSUInteger) index))
    {
        return nil;
    }
    else
    {
        return [children objectAtIndex:index];
    }
    
    
    // We are only one level deep, so we can't be asked about children
    //NSAssert (NO, @"Presets View outlineView:child:ofItem: currently can't handle nested items.");
    //return nil;
}

/* We use this to determine if an item should be expandable */
- (BOOL)outlineView:(NSOutlineView *)fPresetsOutlineView isItemExpandable:(id)item
{
    
    /* we need to return the count of the array in ChildrenArray for this folder */
    NSArray *children= nil;
    if (item == nil)
    {
        children = UserPresets;
    }
    else
    {
        if ([item objectForKey:@"ChildrenArray"])
        {
            children = [item objectForKey:@"ChildrenArray"];
        }
    }   
    
    /* To deterimine if an item should show a disclosure triangle
     * we could do it by the children count as so:
     * if ([children count] < 1)
     * However, lets leave the triangle show even if there are no
     * children to help indicate a folder, just like folder in the
     * finder can show a disclosure triangle even when empty
     */
    
    /* We need to determine if the item is a folder */
   if ([[item objectForKey:@"Folder"] intValue] == 1)
   {
        return YES;
    }
    else
    {
        return NO;
    }
    
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item
{
    // Our outline view has no levels, but we can still expand every item. Doing so
    // just makes the row taller. See heightOfRowByItem below.
//return ![(HBQueueOutlineView*)outlineView isDragging];

return YES;
}


/* Used to tell the outline view which information is to be displayed per item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	/* We have two columns right now, icon and PresetName */
	
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        return [item objectForKey:@"PresetName"];
    }
    else
    {
        //return @"";
        return nil;
    }
}

- (id)outlineView:(NSOutlineView *)outlineView itemForPersistentObject:(id)object
{
    return [NSKeyedUnarchiver unarchiveObjectWithData:object];
}
- (id)outlineView:(NSOutlineView *)outlineView persistentObjectForItem:(id)item
{
    return [NSKeyedArchiver archivedDataWithRootObject:item];
}

#pragma mark - Added Functionality (optional)
/* Use to customize the font and display characteristics of the title cell */
- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        NSFont *txtFont;
        NSColor *fontColor;
        NSColor *shadowColor;
        txtFont = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
        /*check to see if its a selected row */
        if ([fPresetsOutlineView selectedRow] == [fPresetsOutlineView rowForItem:item])
        {
            
            fontColor = [NSColor blackColor];
            shadowColor = [NSColor colorWithDeviceRed:(127.0/255.0) green:(140.0/255.0) blue:(160.0/255.0) alpha:1.0];
        }
        else
        {
            if ([[item objectForKey:@"Type"] intValue] == 0)
            {
                fontColor = [NSColor blueColor];
            }
            else // User created preset, use a black font
            {
                fontColor = [NSColor blackColor];
            }
            /* check to see if its a folder */
            //if ([[item objectForKey:@"Folder"] intValue] == 1)
            //{
            //fontColor = [NSColor greenColor];
            //}
            
            
        }
        /* We use bold text for the default preset */
        if (presetUserDefault == nil &&                    // no User default found
            [[item objectForKey:@"Default"] intValue] == 1)// 1 is HB default
        {
            txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
        }
        if ([[item objectForKey:@"Default"] intValue] == 2)// 2 is User default
        {
            txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
        }
        
        
        [cell setTextColor:fontColor];
        [cell setFont:txtFont];
        
    }
}

/* We use this to edit the name field in the outline view */
- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        id theRecord;
        
        theRecord = item;
        [theRecord setObject:object forKey:@"PresetName"];
        
        [self sortPresets];
        
        [fPresetsOutlineView reloadData];
        /* We save all of the preset data here */
        [self savePreset];
    }
}
/* We use this to provide tooltips for the items in the presets outline view */
- (NSString *)outlineView:(NSOutlineView *)fPresetsOutlineView toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tc item:(id)item mouseLocation:(NSPoint)mouseLocation
{
    //if ([[tc identifier] isEqualToString:@"PresetName"])
    //{
        /* initialize the tooltip contents variable */
        NSString *loc_tip;
        /* if there is a description for the preset, we show it in the tooltip */
        if ([item objectForKey:@"PresetDescription"])
        {
            loc_tip = [item objectForKey:@"PresetDescription"];
            return (loc_tip);
        }
        else
        {
            loc_tip = @"No description available";
        }
        return (loc_tip);
    //}
}

- (void) outlineViewSelectionDidChange: (NSNotification *) ignored

{
	[self willChangeValueForKey: @"hasValidPresetSelected"];
	[self didChangeValueForKey: @"hasValidPresetSelected"];
	return;
}

#pragma mark -
#pragma mark Preset Outline View Methods (dragging related)


- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
	// Dragging is only allowed for custom presets.
    //[[[self selectedPreset] objectForKey:@"Default"] intValue] != 1
    if ([[[self selectedPreset] objectForKey:@"Type"] intValue] == 0) // 0 is built in preset
    {
        return NO;
    }
    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    fDraggedNodes = items;
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:[NSArray arrayWithObjects: DragDropSimplePboardType, nil] owner:self];
    
    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:DragDropSimplePboardType]; 
    
    return YES;
}

- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index
{
	
	// Don't allow dropping ONTO an item since they can't really contain any children.
    
    BOOL isOnDropTypeProposal = index == NSOutlineViewDropOnItemIndex;
    if (isOnDropTypeProposal)
        return NSDragOperationNone;
    
    // Don't allow dropping INTO an item since they can't really contain any children as of yet.
	if (item != nil)
	{
		index = [fPresetsOutlineView rowForItem: item] + 1;
		item = nil;
	}
    
    // Don't allow dropping into the Built In Presets.
    if (index < presetCurrentBuiltInCount)
    {
        return NSDragOperationNone;
        index = MAX (index, presetCurrentBuiltInCount);
	}    
	
    [outlineView setDropItem:item dropChildIndex:index];
    return NSDragOperationGeneric;
}



- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index
{
    /* first, lets see if we are dropping into a folder */
    if ([[fPresetsOutlineView itemAtRow:index] objectForKey:@"Folder"] && [[[fPresetsOutlineView itemAtRow:index] objectForKey:@"Folder"] intValue] == 1) // if its a folder
	{
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    childrenArray = [[fPresetsOutlineView itemAtRow:index] objectForKey:@"ChildrenArray"];
    [childrenArray addObject:item];
    [[fPresetsOutlineView itemAtRow:index] setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
    [childrenArray autorelease];
    }
    else // We are not, so we just move the preset into the existing array 
    {
        NSMutableIndexSet *moveItems = [NSMutableIndexSet indexSet];
        id obj;
        NSEnumerator *enumerator = [fDraggedNodes objectEnumerator];
        while (obj = [enumerator nextObject])
        {
            [moveItems addIndex:[UserPresets indexOfObject:obj]];
        }
        // Successful drop, lets rearrange the view and save it all
        [self moveObjectsInPresetsArray:UserPresets fromIndexes:moveItems toIndex: index];
    }
    [fPresetsOutlineView reloadData];
    [self savePreset];
    return YES;
}

- (void)moveObjectsInPresetsArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;
    
    NSUInteger removeIndex;

    if (index >= insertIndex)
    {
        removeIndex = index + aboveInsertIndexCount;
        aboveInsertIndexCount++;
    }
    else
    {
        removeIndex = index;
        insertIndex--;
    }

    id object = [[array objectAtIndex:removeIndex] retain];
    [array removeObjectAtIndex:removeIndex];
    [array insertObject:object atIndex:insertIndex];
    [object release];

    index = [indexSet indexLessThanIndex:index];
}



#pragma mark - Functional Preset NSOutlineView Methods

- (IBAction)selectPreset:(id)sender
{

    if (YES == [self hasValidPresetSelected])
    {
        chosenPreset = [self selectedPreset];
        [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];

        if ([[chosenPreset objectForKey:@"Default"] intValue] == 1)
        {
            [fPresetSelectedDisplay setStringValue:[NSString stringWithFormat:@"%@ (Default)", [chosenPreset objectForKey:@"PresetName"]]];
        }
        else
        {
            [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];
        }
        
        /* File Format */
        [fDstFormatPopUp selectItemWithTitle:[chosenPreset objectForKey:@"FileFormat"]];
        [self formatPopUpChanged:nil];
        
        /* Chapter Markers*/
        [fCreateChapterMarkers setState:[[chosenPreset objectForKey:@"ChapterMarkers"] intValue]];
        /* check to see if we have only one chapter */
        [self chapterPopUpChanged:nil];
        
        /* Allow Mpeg4 64 bit formatting +4GB file sizes */
        [fDstMp4LargeFileCheck setState:[[chosenPreset objectForKey:@"Mp4LargeFile"] intValue]];
        /* Mux mp4 with http optimization */
        [fDstMp4HttpOptFileCheck setState:[[chosenPreset objectForKey:@"Mp4HttpOptimize"] intValue]];
        
        /* Video encoder */
        [fVidEncoderPopUp selectItemWithTitle:[chosenPreset objectForKey:@"VideoEncoder"]];
        [self videoEncoderPopUpChanged:nil];
        
        if ([[chosenPreset objectForKey:@"VideoEncoder"] isEqualToString:@"H.264 (x264)"])
        {
            if (![chosenPreset objectForKey:@"x264UseAdvancedOptions"] ||
                [[chosenPreset objectForKey:@"x264UseAdvancedOptions"] intValue])
            {
                /*
                 * x264UseAdvancedOptions is not set (legacy preset)
                 * or set to 1 (enabled), so we use the old advanced panel
                 */
                if ([chosenPreset objectForKey:@"x264Option"])
                {
                    /* we set the advanced options string here if applicable */
                    [fAdvancedOptions setOptions:        [chosenPreset objectForKey:@"x264Option"]];
                    [self             setX264OptionExtra:[chosenPreset objectForKey:@"x264Option"]];
                }
                else
                {
                    [fAdvancedOptions setOptions:        @""];
                    [self             setX264OptionExtra:nil];
                }
                /* preset does not use the x264 preset system, reset the widgets */
                [self setX264Preset: nil];
                [self setX264Tune:   nil];
                [self setH264Profile:nil];
                [self setH264Level:  nil];
                /* we enable the advanced panel and update the widgets */
                [fX264UseAdvancedOptionsCheck setState:NSOnState];
                [self updateX264Widgets:nil];
            }
            else
            {
                /*
                 * x264UseAdvancedOptions is set to 0 (disabled),
                 * so we use the x264 preset system
                 */
                [self setX264Preset:     [chosenPreset objectForKey:@"x264Preset"]];
                [self setX264Tune:       [chosenPreset objectForKey:@"x264Tune"]];
                [self setX264OptionExtra:[chosenPreset objectForKey:@"x264OptionExtra"]];
                [self setH264Profile:    [chosenPreset objectForKey:@"h264Profile"]];
                [self setH264Level:      [chosenPreset objectForKey:@"h264Level"]];
                /* preset does not use the advanced panel, reset it */
                [fAdvancedOptions setOptions:@""];
                /* we disable the advanced panel and update the widgets */
                [fX264UseAdvancedOptionsCheck setState:NSOffState];
                [self updateX264Widgets:nil];
            }
        }
        
        if ([chosenPreset objectForKey:@"lavcOption"])
        {
            [fAdvancedOptions setLavcOptions:[chosenPreset objectForKey:@"lavcOption"]];
        }
        else
        {
            [fAdvancedOptions setLavcOptions:@""];   
        }
        
        /* Lets run through the following functions to get variables set there */
        //[self videoEncoderPopUpChanged:nil];
        /* Set the state of ipod compatible with Mp4iPodCompatible. Only for x264*/
        [fDstMp4iPodFileCheck setState:[[chosenPreset objectForKey:@"Mp4iPodCompatible"] intValue]];
        [self calculateBitrate:nil];
        
        /* Video quality */
        
        int qualityType = [[chosenPreset objectForKey:@"VideoQualityType"] intValue] - 1;
        /* Note since the removal of Target Size encoding, the possible values for VideoQuality type are 0 - 1.
         * Therefore any preset that uses the old 2 for Constant Quality would now use 1 since there is one less index
         * for the fVidQualityMatrix. It should also be noted that any preset that used the deprecated Target Size
         * setting of 0 would set us to 0 or ABR since ABR is now tagged 0. Fortunately this does not affect any built-in
         * presets since they all use Constant Quality or Average Bitrate.*/
        if (qualityType == -1)
        {
            qualityType = 0;
        }
        [fVidQualityMatrix selectCellWithTag:qualityType];

        [fVidBitrateField setStringValue:[chosenPreset objectForKey:@"VideoAvgBitrate"]];
        
        if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
        {
            /* Since theora's qp value goes up from left to right, we can just set the slider float value */
            [fVidQualitySlider setFloatValue:[[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue]];
        }
        else
        {
            /* Since ffmpeg and x264 use an "inverted" slider (lower qp/rf values indicate a higher quality) we invert the value on the slider */
            [fVidQualitySlider setFloatValue:([fVidQualitySlider maxValue] + [fVidQualitySlider minValue]) - [[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue]];
        }
        
        [self videoMatrixChanged:nil];
        
        /* Video framerate */
        if ([[chosenPreset objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
        {
            /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
            if (![chosenPreset objectForKey:@"VideoFramerateMode"] ||
                [[chosenPreset objectForKey:@"VideoFramerateMode"] isEqualToString:@"vfr"])
            {
                [fFramerateMatrix selectCellAtRow:0 column:0]; // we want vfr
            }
            else
            {
                [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
            }
        }
        else
        {
            /* Now set the Video Frame Rate Mode to either pfr or cfr according to the preset */
            if ([[chosenPreset objectForKey:@"VideoFramerateMode"] isEqualToString:@"pfr"] ||
                [[chosenPreset objectForKey:@"VideoFrameratePFR"]  intValue] == 1)
            {
                [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
            }
            else
            {
                [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
            }
        }
        [fVidRatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"VideoFramerate"]];
        [self videoFrameRateChanged:nil];
        
        /* 2 Pass Encoding */
        [fVidTwoPassCheck setState:[[chosenPreset objectForKey:@"VideoTwoPass"] intValue]];
        [self twoPassCheckboxChanged:nil];
        
        /* Turbo 1st pass for 2 Pass Encoding */
        [fVidTurboPassCheck setState:[[chosenPreset objectForKey:@"VideoTurboTwoPass"] intValue]];
        
        /* Auto Passthru: if the preset has Auto Passthru fields, use them.
         * Otherwise assume every passthru is allowed and the fallback is AC3 */
        id tempObject;
        if ((tempObject = [chosenPreset objectForKey:@"AudioAllowAACPass"]) != nil)
        {
            [fAudioAllowAACPassCheck setState:[tempObject intValue]];
        }
        else
        {
            [fAudioAllowAACPassCheck setState:NSOnState];
        }
        if ((tempObject = [chosenPreset objectForKey:@"AudioAllowAC3Pass"]) != nil)
        {
            [fAudioAllowAC3PassCheck setState:[tempObject intValue]];
        }
        else
        {
            [fAudioAllowAC3PassCheck setState:NSOnState];
        }
        if ((tempObject = [chosenPreset objectForKey:@"AudioAllowDTSHDPass"]) != nil)
        {
            [fAudioAllowDTSHDPassCheck setState:[tempObject intValue]];
        }
        else
        {
            [fAudioAllowDTSHDPassCheck setState:NSOnState];
        }
        if ((tempObject = [chosenPreset objectForKey:@"AudioAllowDTSPass"]) != nil)
        {
            [fAudioAllowDTSPassCheck setState:[tempObject intValue]];
        }
        else
        {
            [fAudioAllowDTSPassCheck setState:NSOnState];
        }
        if ((tempObject = [chosenPreset objectForKey:@"AudioAllowMP3Pass"]) != nil)
        {
            [fAudioAllowMP3PassCheck setState:[tempObject intValue]];
        }
        else
        {
            [fAudioAllowMP3PassCheck setState:NSOnState];
        }
        if ((tempObject = [chosenPreset objectForKey:@"AudioEncoderFallback"]) != nil)
        {
            [fAudioFallbackPopUp selectItemWithTitle:tempObject];
        }
        else
        {
            [fAudioFallbackPopUp selectItemWithTitle:@"AC3 (ffmpeg)"];
        }
        
        /* Audio */
        [fAudioDelegate addTracksFromPreset: chosenPreset];
        
        /*Subtitles*/
        [fSubPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Subtitles"]];
        /* Forced Subtitles */
        [fSubForcedCheck setState:[[chosenPreset objectForKey:@"SubtitlesForced"] intValue]];
        
        /* Picture Settings */
        /* Note: objectForKey:@"UsesPictureSettings" refers to picture size, which encompasses:
         * height, width, keep ar, anamorphic and crop settings.
         * picture filters are handled separately below.
         */
        /* Check to see if the objectForKey:@"UsesPictureSettings is greater than 0, as 0 means use picture sizing "None" 
         * ( 2 is use max for source and 1 is use exact size when the preset was created ) and the 
         * preset completely ignores any picture sizing values in the preset.
         */
        if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] > 0)
        {
            hb_job_t * job = fTitle->job;
            
            /* If Cropping is set to custom, then recall all four crop values from
             when the preset was created and apply them */
            if ([[chosenPreset objectForKey:@"PictureAutoCrop"]  intValue] == 0)
            {
                [fPictureController setAutoCrop:NO];
                
                /* Here we use the custom crop values saved at the time the preset was saved */
                job->crop[0] = [[chosenPreset objectForKey:@"PictureTopCrop"]  intValue];
                job->crop[1] = [[chosenPreset objectForKey:@"PictureBottomCrop"]  intValue];
                job->crop[2] = [[chosenPreset objectForKey:@"PictureLeftCrop"]  intValue];
                job->crop[3] = [[chosenPreset objectForKey:@"PictureRightCrop"]  intValue];
                
            }
            else /* if auto crop has been saved in preset, set to auto and use post scan auto crop */
            {
                [fPictureController setAutoCrop:YES];
                /* Here we use the auto crop values determined right after scan */
                job->crop[0] = AutoCropTop;
                job->crop[1] = AutoCropBottom;
                job->crop[2] = AutoCropLeft;
                job->crop[3] = AutoCropRight;
                
            }
            
            /* Set modulus */
            if ([chosenPreset objectForKey:@"PictureModulus"])
            {
                job->modulus = [[chosenPreset objectForKey:@"PictureModulus"]  intValue];
            }
            else
            {
                job->modulus = 16;
            }
             
            /* Check to see if the objectForKey:@"UsesPictureSettings" is 2,
             * which means "Use max. picture size for the source" */
            if ([[chosenPreset objectForKey:@"UsesPictureSettings"]    intValue] == 2 ||
                [[chosenPreset objectForKey:@"UsesMaxPictureSettings"] intValue] == 1)
            {
                /* Use Max Picture settings for whatever the dvd is.*/
                [self revertPictureSizeToMax:nil];
                job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
                if (job->keep_ratio == 1)
                {
                    hb_fix_aspect( job, HB_KEEP_WIDTH );
                    if( job->height > fTitle->height )
                    {
                        job->height = fTitle->height;
                        hb_fix_aspect( job, HB_KEEP_HEIGHT );
                    }
                }
                job->anamorphic.mode = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
            }
            /* If not 0 or 2 we assume objectForKey:@"UsesPictureSettings" is 1,
             * which means "Use the picture size specified in the preset" */
            else
            {
                /*
                 * if the preset specifies neither max. width nor height
                 * (both are 0), use the max. picture size
                 *
                 * if the specified non-zero dimensions exceed those of the
                 * source, also use the max. picture size (no upscaling)
                 */
                if (([[chosenPreset objectForKey:@"PictureWidth"]  intValue] <= 0 &&
                     [[chosenPreset objectForKey:@"PictureHeight"] intValue] <= 0)              ||
                    ([[chosenPreset objectForKey:@"PictureWidth"]  intValue] >  fTitle->width &&
                     [[chosenPreset objectForKey:@"PictureHeight"] intValue] >  fTitle->height) ||
                    ([[chosenPreset objectForKey:@"PictureHeight"] intValue] <= 0 &&
                     [[chosenPreset objectForKey:@"PictureWidth"]  intValue] >  fTitle->width)  ||
                    ([[chosenPreset objectForKey:@"PictureWidth"]  intValue] <= 0 &&
                     [[chosenPreset objectForKey:@"PictureHeight"] intValue] >  fTitle->height))
                {
                    /* use the source's width/height to avoid upscaling */
                    [self revertPictureSizeToMax:nil];
                }
                else // source width/height is >= preset width/height
                {
                    /* use the preset values for width/height */
                    job->width  = [[chosenPreset objectForKey:@"PictureWidth"]  intValue];
                    job->height = [[chosenPreset objectForKey:@"PictureHeight"] intValue];
                }
                job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
                if (job->keep_ratio == 1)
                {
                    int height = fTitle->height;

                    if ( job->height && job->height < fTitle->height )
                        height = job->height;

                    hb_fix_aspect( job, HB_KEEP_WIDTH );
                    // Make sure the resulting height is less than
                    // the title height and less than the height
                    // requested in the preset.
                    if( job->height > height )
                    {
                        job->height = height;
                        hb_fix_aspect( job, HB_KEEP_HEIGHT );
                    }
                }
                job->anamorphic.mode = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
                if ( job->anamorphic.mode > 0 )
                {
                    int w, h, par_w, par_h;

                    job->anamorphic.par_width = fTitle->pixel_aspect_width;
                    job->anamorphic.par_height = fTitle->pixel_aspect_height;
                    job->maxWidth = job->width;
                    job->maxHeight = job->height;
                    hb_set_anamorphic_size( job, &w, &h, &par_w, &par_h );
                    job->maxWidth = 0;
                    job->maxHeight = 0;
                    job->width = w;
                    job->height = h;
                }
                
            }
            
            
        }
        /* If the preset has an objectForKey:@"UsesPictureFilters", and handle the filters here */
        if ([chosenPreset objectForKey:@"UsesPictureFilters"] && [[chosenPreset objectForKey:@"UsesPictureFilters"]  intValue] > 0)
        {
            /* Filters */
            
            /* We only allow *either* Decomb or Deinterlace. So check for the PictureDecombDeinterlace key. */
            [fPictureController setUseDecomb:1];
            [fPictureController setDecomb:0];
            [fPictureController setDeinterlace:0];
            if ([[chosenPreset objectForKey:@"PictureDecombDeinterlace"] intValue] == 1)
            {
                /* we are using decomb */
                /* Decomb */
                if ([[chosenPreset objectForKey:@"PictureDecomb"] intValue] > 0)
                {
                    [fPictureController setDecomb:[[chosenPreset objectForKey:@"PictureDecomb"] intValue]];
                    
                    /* if we are using "Custom" in the decomb setting, also set the custom string*/
                    if ([[chosenPreset objectForKey:@"PictureDecomb"] intValue] == 1)
                    {
                        [fPictureController setDecombCustomString:[chosenPreset objectForKey:@"PictureDecombCustom"]];    
                    }
                }
             }
            else
            {
                /* We are using Deinterlace */
                /* Deinterlace */
                if ([[chosenPreset objectForKey:@"PictureDeinterlace"] intValue] > 0)
                {
                    [fPictureController setUseDecomb:0];
                    [fPictureController setDeinterlace:[[chosenPreset objectForKey:@"PictureDeinterlace"] intValue]];
                    /* if we are using "Custom" in the deinterlace setting, also set the custom string*/
                    if ([[chosenPreset objectForKey:@"PictureDeinterlace"] intValue] == 1)
                    {
                        [fPictureController setDeinterlaceCustomString:[chosenPreset objectForKey:@"PictureDeinterlaceCustom"]];    
                    }
                }
            }
            
            
            /* Detelecine */
            if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] > 0)
            {
                [fPictureController setDetelecine:[[chosenPreset objectForKey:@"PictureDetelecine"] intValue]];
                /* if we are using "Custom" in the detelecine setting, also set the custom string*/
                if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] == 1)
                {
                    [fPictureController setDetelecineCustomString:[chosenPreset objectForKey:@"PictureDetelecineCustom"]];    
                }
            }
            else
            {
                [fPictureController setDetelecine:0];
            }
            
            /* Denoise */
            if ([[chosenPreset objectForKey:@"PictureDenoise"] intValue] > 0)
            {
                [fPictureController setDenoise:[[chosenPreset objectForKey:@"PictureDenoise"] intValue]];
                /* if we are using "Custom" in the denoise setting, also set the custom string*/
                if ([[chosenPreset objectForKey:@"PictureDenoise"] intValue] == 1)
                {
                    [fPictureController setDenoiseCustomString:[chosenPreset objectForKey:@"PictureDenoiseCustom"]];    
                }
            }
            else
            {
                [fPictureController setDenoise:0];
            }   
            
            /* Deblock */
            if ([[chosenPreset objectForKey:@"PictureDeblock"] intValue] == 1)
            {
                /* if its a one, then its the old on/off deblock, set on to 5*/
                [fPictureController setDeblock:5];
            }
            else
            {
                /* use the settings intValue */
                [fPictureController setDeblock:[[chosenPreset objectForKey:@"PictureDeblock"] intValue]];
            }
            
            if ([[chosenPreset objectForKey:@"VideoGrayScale"] intValue] == 1)
            {
                [fPictureController setGrayscale:1];
            }
            else
            {
                [fPictureController setGrayscale:0];
            }
        }
        /* we call SetTitle: in fPictureController so we get an instant update in the Picture Settings window */
        [fPictureController SetTitle:fTitle];
        [fPictureController SetTitle:fTitle];
        [self calculatePictureSizing:nil];
    }
}


- (BOOL)hasValidPresetSelected
{
    return ([fPresetsOutlineView selectedRow] >= 0 && [[[self selectedPreset] objectForKey:@"Folder"] intValue] != 1);
}


- (id)selectedPreset
{
    return [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]];
}


#pragma mark -
#pragma mark Manage Presets

- (void) loadPresets {
    /* We declare the default NSFileManager into fileManager */
    NSFileManager * fileManager = [NSFileManager defaultManager];
    /* We define the location of the user presets file */
    UserPresetsFile = @"~/Library/Application Support/HandBrake/UserPresets.plist";
    UserPresetsFile = [[UserPresetsFile stringByExpandingTildeInPath]retain];
    /* We check for the presets.plist */
    if ([fileManager fileExistsAtPath:UserPresetsFile] == 0)
    {
        [fileManager createFileAtPath:UserPresetsFile contents:nil attributes:nil];
    }

    UserPresets = [[NSMutableArray alloc] initWithContentsOfFile:UserPresetsFile];
    if (nil == UserPresets)
    {
        UserPresets = [[NSMutableArray alloc] init];
        [self addFactoryPresets:nil];
    }
    [fPresetsOutlineView reloadData];
    
    [self checkBuiltInsForUpdates];
}

- (void) checkBuiltInsForUpdates {
    
    BOOL updateBuiltInPresets = NO;
    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        /* iterate through the built in presets to see if any have an old build number */
        NSMutableDictionary *thisPresetDict = tempObject;
        /*Key Type == 0 is built in, and key PresetBuildNumber is the build number it was created with */
        if ([[thisPresetDict objectForKey:@"Type"] intValue] == 0)		
        {
			if (![thisPresetDict objectForKey:@"PresetBuildNumber"] || [[thisPresetDict objectForKey:@"PresetBuildNumber"] intValue] < [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue])
            {
                updateBuiltInPresets = YES;
            }	
		}
        i++;
    }
    /* if we have built in presets to update, then do so AlertBuiltInPresetUpdate*/
    if ( updateBuiltInPresets == YES)
    {
        if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertBuiltInPresetUpdate"] == YES)
        {
            /* Show an alert window that built in presets will be updated */
            /*On Screen Notification*/
            int status;
            NSBeep();
            status = NSRunAlertPanel(@"HandBrake has determined your built in presets are out of date…",@"HandBrake will now update your built-in presets.", @"OK", nil, nil);
            [NSApp requestUserAttention:NSCriticalRequest];
        }
        /* when alert is dismissed, go ahead and update the built in presets */
        [self addFactoryPresets:nil];
    }
    
}


- (IBAction) addPresetPicDropdownChanged: (id) sender
{
    if ([[fPresetNewPicSettingsPopUp selectedItem] tag] == 1)
    {
        [fPresetNewPicWidthHeightBox setHidden:NO];
    }
    else
    {
        [fPresetNewPicWidthHeightBox setHidden:YES];
    }
}

- (IBAction) showAddPresetPanel: (id) sender
{
    /* Deselect the currently selected Preset if there is one*/
    [fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];

    /*
     * Populate the preset picture settings popup.
     *
     * Custom is not applicable when the anamorphic mode is Strict.
     *
     * Use [NSMenuItem tag] to store preset values for each option.
     */
    [fPresetNewPicSettingsPopUp removeAllItems];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"None"];
    [[fPresetNewPicSettingsPopUp lastItem] setTag: 0];
    if (fTitle->job->anamorphic.mode != 1)
    {
        // not Strict, Custom is applicable
        [fPresetNewPicSettingsPopUp addItemWithTitle:@"Custom"];
        [[fPresetNewPicSettingsPopUp lastItem] setTag: 1];
    }
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"Source Maximum (post source scan)"];
    [[fPresetNewPicSettingsPopUp lastItem] setTag: 2];
    /*
     * Default to Source Maximum for anamorphic Strict
     * Default to Custom for all other anamorphic modes
     */
    [fPresetNewPicSettingsPopUp selectItemWithTag: (1 + (fTitle->job->anamorphic.mode == 1))];
    /* Save the current filters in the preset by default */
    [fPresetNewPicFiltersCheck setState:NSOnState];
    // fPresetNewFolderCheck
    [fPresetNewFolderCheck setState:NSOffState];
    /* Erase info from the input fields*/
	[fPresetNewName setStringValue: @""];
	[fPresetNewDesc setStringValue: @""];
    
    /* Initialize custom height and width settings to current values */
    
	[fPresetNewPicWidth setStringValue: [NSString stringWithFormat:@"%d",fTitle->job->width]];
	[fPresetNewPicHeight setStringValue: [NSString stringWithFormat:@"%d",fTitle->job->height]];
    [self addPresetPicDropdownChanged:nil];
	/* Show the panel */
	[NSApp beginSheet:fAddPresetPanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
}

- (IBAction) closeAddPresetPanel: (id) sender
{
    [NSApp endSheet: fAddPresetPanel];
    [fAddPresetPanel orderOut: self];
}

- (IBAction)addUserPreset:(id)sender
{
    if (![[fPresetNewName stringValue] length])
            NSRunAlertPanel(@"Warning!", @"You need to insert a name for the preset.", @"OK", nil , nil);
    else
    {
        /* Here we create a custom user preset */
        [UserPresets addObject:[self createPreset]];
        [self addPreset];

        [self closeAddPresetPanel:nil];
    }
}
- (void)addPreset
{

	
	/* We Reload the New Table data for presets */
    [fPresetsOutlineView reloadData];
   /* We save all of the preset data here */
    [self savePreset];
}

- (void)sortPresets
{

	
	/* We Sort the Presets By Factory or Custom */
	NSSortDescriptor * presetTypeDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"Type" 
                                                    ascending:YES] autorelease];
	/* We Sort the Presets Alphabetically by name  We do not use this now as we have drag and drop*/
	/*
    NSSortDescriptor * presetNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	//NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,presetNameDescriptor,nil];
    
    */
    /* Since we can drag and drop our custom presets, lets just sort by type and not name */
    NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,nil];
	NSArray *sortedArray=[UserPresets sortedArrayUsingDescriptors:sortDescriptors];
	[UserPresets setArray:sortedArray];
	

}

- (IBAction)insertPreset:(id)sender
{
    int index = [fPresetsOutlineView selectedRow];
    [UserPresets insertObject:[self createPreset] atIndex:index];
    [fPresetsOutlineView reloadData];
    [self savePreset];
}

- (NSDictionary *)createPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    /* Preset build number */
    [preset setObject:[NSString stringWithFormat: @"%d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]] forKey:@"PresetBuildNumber"];
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
    /* Set whether or not this is to be a folder fPresetNewFolderCheck*/
    [preset setObject:[NSNumber numberWithBool:[fPresetNewFolderCheck state]] forKey:@"Folder"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    if ([fPresetNewFolderCheck state] == YES)
    {
        /* initialize and set an empty array for children here since we are a new folder */
        NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
        [preset setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
        [childrenArray autorelease];
    }
    else // we are not creating a preset folder, so we go ahead with the rest of the preset info
    {
        /*Get the whether or not to apply pic Size and Cropping (includes Anamorphic)*/
        [preset setObject:[NSNumber numberWithInteger:[[fPresetNewPicSettingsPopUp selectedItem] tag]] forKey:@"UsesPictureSettings"];
        /* Get whether or not to use the current Picture Filter settings for the preset */
        [preset setObject:[NSNumber numberWithInt:[fPresetNewPicFiltersCheck state]] forKey:@"UsesPictureFilters"];

        /* Get New Preset Description from the field in the AddPresetPanel*/
        [preset setObject:[fPresetNewDesc stringValue] forKey:@"PresetDescription"];
        /* File Format */
        [preset setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
        /* Chapter Markers fCreateChapterMarkers*/
        [preset setObject:[NSNumber numberWithInt:[fCreateChapterMarkers state]] forKey:@"ChapterMarkers"];
        /* Allow Mpeg4 64 bit formatting +4GB file sizes */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
        /* Mux mp4 with http optimization */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
        /* Add iPod uuid atom */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
        
        /* Codecs */
        /* Video encoder */
        [preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
        /* x264 Options, this will either be advanced panel or the video tabs x264 presets panel with modded option string */
        
        if ([fX264UseAdvancedOptionsCheck state] == NSOnState)
        {
            /* use the old advanced panel */
            [preset setObject:[NSNumber numberWithInt:1]       forKey:@"x264UseAdvancedOptions"];
            [preset setObject:[fAdvancedOptions optionsString] forKey:@"x264Option"];
        }
        else
        {
            /* use the x264 preset system */
            [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
            [preset setObject:[self x264Preset]          forKey:@"x264Preset"];
            [preset setObject:[self x264Tune]            forKey:@"x264Tune"];
            [preset setObject:[self x264OptionExtra]     forKey:@"x264OptionExtra"];
            [preset setObject:[self h264Profile]         forKey:@"h264Profile"];
            [preset setObject:[self h264Level]           forKey:@"h264Level"];
            /*
             * bonus: set the unparsed options to make the preset compatible
             * with old HB versions
             */
            if (fX264PresetsUnparsedUTF8String != NULL)
            {
                [preset setObject:[NSString stringWithUTF8String:fX264PresetsUnparsedUTF8String]
                           forKey:@"x264Option"];
            }
            else
            {
                [preset setObject:@"" forKey:@"x264Option"];
            }
        }

        /* FFmpeg (lavc) Option String */
        [preset setObject:[fAdvancedOptions optionsStringLavc] forKey:@"lavcOption"];
        
        /* though there are actually only 0 - 1 types available in the ui we need to map to the old 0 - 2
         * set of indexes from when we had 0 == Target , 1 == Abr and 2 == Constant Quality for presets
         * to take care of any legacy presets. */
        [preset setObject:[NSNumber numberWithInt:[[fVidQualityMatrix selectedCell] tag] +1 ] forKey:@"VideoQualityType"];
        [preset setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
        [preset setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];
        
        /* Video framerate */
        /* Set the Video Frame Rate Mode */
        if ([fFramerateMatrix selectedRow] == 1)
        {
            [preset setObject:@"cfr" forKey:@"VideoFramerateMode"];
        }
        /* Set the actual framerate from popup overriding the cfr setting as needed */
        if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source is selected
        {
            [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
            
            if ([fFramerateMatrix selectedRow] == 0)
            {
                [preset setObject:@"vfr" forKey:@"VideoFramerateMode"];
            }
        }
        else // we can record the actual titleOfSelectedItem
        {
            [preset setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
            
            if ([fFramerateMatrix selectedRow] == 0)
            {
                [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
            }
        }
        

        
        /* 2 Pass Encoding */
        [preset setObject:[NSNumber numberWithInt:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
        /* Turbo 2 pass Encoding fVidTurboPassCheck*/
        [preset setObject:[NSNumber numberWithInt:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];
        /*Picture Settings*/
        hb_job_t * job = fTitle->job;
        
        /* Picture Sizing */
        [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
        [preset setObject:[NSNumber numberWithInt:[fPresetNewPicWidth intValue]] forKey:@"PictureWidth"];
        [preset setObject:[NSNumber numberWithInt:[fPresetNewPicHeight intValue]] forKey:@"PictureHeight"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->modulus] forKey:@"PictureModulus"];
        
        /* Set crop settings here */
        [preset setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
        
        /* Picture Filters */
        [preset setObject:[NSNumber numberWithInt:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
        [preset setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
        [preset setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController denoise]] forKey:@"PictureDenoise"];
        [preset setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController deblock]] forKey:@"PictureDeblock"]; 
        [preset setObject:[NSNumber numberWithInt:[fPictureController decomb]] forKey:@"PictureDecomb"];
        [preset setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController grayscale]] forKey:@"VideoGrayScale"];
        
        /* Auto Pasthru */
        [preset setObject:[NSNumber numberWithInt:[fAudioAllowAACPassCheck state]] forKey: @"AudioAllowAACPass"];
        [preset setObject:[NSNumber numberWithInt:[fAudioAllowAC3PassCheck state]] forKey: @"AudioAllowAC3Pass"];
        [preset setObject:[NSNumber numberWithInt:[fAudioAllowDTSHDPassCheck state]] forKey: @"AudioAllowDTSHDPass"];
        [preset setObject:[NSNumber numberWithInt:[fAudioAllowDTSPassCheck state]] forKey: @"AudioAllowDTSPass"];
        [preset setObject:[NSNumber numberWithInt:[fAudioAllowMP3PassCheck state]] forKey: @"AudioAllowMP3Pass"];
        [preset setObject:[fAudioFallbackPopUp titleOfSelectedItem] forKey: @"AudioEncoderFallback"];
        
        /* Audio */
        NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
		[fAudioDelegate prepareAudioForPreset: audioListArray];
        
        
        [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

        
        /* Temporarily remove subtitles from creating a new preset as it has to be converted over to use the new
         * subititle array code. */
        /* Subtitles*/
        //[preset setObject:[fSubPopUp titleOfSelectedItem] forKey:@"Subtitles"];
        /* Forced Subtitles */
        //[preset setObject:[NSNumber numberWithInt:[fSubForcedCheck state]] forKey:@"SubtitlesForced"];
    }
    [preset autorelease];
    return preset;
    
}

- (void)savePreset
{
    [UserPresets writeToFile:UserPresetsFile atomically:YES];
	/* We get the default preset in case it changed */
	[self getDefaultPresets:nil];

}

- (IBAction)deletePreset:(id)sender
{
    
    
    if ( [fPresetsOutlineView numberOfSelectedRows] == 0 )
    {
        return;
    }
    /* Alert user before deleting preset */
    int status;
    status = NSRunAlertPanel(@"Warning!", @"Are you sure that you want to delete the selected preset?", @"OK", @"Cancel", nil);

    if ( status == NSAlertDefaultReturn )
    {
        int presetToModLevel = [fPresetsOutlineView levelForItem: [self selectedPreset]];
        NSDictionary *presetToMod = [self selectedPreset];
        NSDictionary *presetToModParent = [fPresetsOutlineView parentForItem: presetToMod];

        NSEnumerator *enumerator;
        NSMutableArray *presetsArrayToMod;
        NSMutableArray *tempArray;
        id tempObject;
        /* If we are a root level preset, we are modding the UserPresets array */
        if (presetToModLevel == 0)
        {
            presetsArrayToMod = UserPresets;
        }
        else // We have a parent preset, so we modify the chidren array object for key
        {
            presetsArrayToMod = [presetToModParent objectForKey:@"ChildrenArray"]; 
        }
        
        enumerator = [presetsArrayToMod objectEnumerator];
        tempArray = [NSMutableArray array];
        
        while (tempObject = [enumerator nextObject]) 
        {
            NSDictionary *thisPresetDict = tempObject;
            if (thisPresetDict == presetToMod)
            {
                [tempArray addObject:tempObject];
            }
        }
        
        [presetsArrayToMod removeObjectsInArray:tempArray];
        [fPresetsOutlineView reloadData];
        [self savePreset];   
    }
}


#pragma mark -
#pragma mark Import Export Preset(s)

- (IBAction) browseExportPresetFile: (id) sender
{
    /* Open a panel to let the user choose where and how to save the export file */
    NSSavePanel * panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
    NSString *defaultExportDirectory = [NSString stringWithFormat: @"%@/Desktop/", NSHomeDirectory()];

	[panel beginSheetForDirectory: defaultExportDirectory file: @"HB_Export.plist"
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( browseExportPresetFileDone:returnCode:contextInfo: )
					  contextInfo: NULL];
}

- (void) browseExportPresetFileDone: (NSSavePanel *) sheet
                   returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *presetExportDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *exportPresetsFile = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:presetExportDirectory forKey:@"LastPresetExportDirectory"];
        /* We check for the presets.plist */
        if ([[NSFileManager defaultManager] fileExistsAtPath:exportPresetsFile] == 0)
        {
            [[NSFileManager defaultManager] createFileAtPath:exportPresetsFile contents:nil attributes:nil];
        }
        NSMutableArray * presetsToExport = [[NSMutableArray alloc] initWithContentsOfFile:exportPresetsFile];
        if (nil == presetsToExport)
        {
            presetsToExport = [[NSMutableArray alloc] init];

            /* now get and add selected presets to export */

        }
        if (YES == [self hasValidPresetSelected])
        {
            [presetsToExport addObject:[self selectedPreset]];
            [presetsToExport writeToFile:exportPresetsFile atomically:YES];

        }

    }
}


- (IBAction) browseImportPresetFile: (id) sender
{

    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: NO ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastPresetImportDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastPresetImportDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    /* set this for allowed file types, not sure if we should allow xml or not */
    NSArray *fileTypes = [NSArray arrayWithObjects:@"plist", @"xml", nil];
    [panel beginSheetForDirectory: sourceDirectory file: nil types: fileTypes
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseImportPresetDone:returnCode:contextInfo: )
                      contextInfo: sender];
}

- (void) browseImportPresetDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *importPresetsDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *importPresetsFile = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:importPresetsDirectory forKey:@"LastPresetImportDirectory"];
        /* NOTE: here we need to do some sanity checking to verify we do not hose up our presets file   */
        NSMutableArray * presetsToImport = [[NSMutableArray alloc] initWithContentsOfFile:importPresetsFile];
        /* iterate though the new array of presets to import and add them to our presets array */
        int i = 0;
        NSEnumerator *enumerator = [presetsToImport objectEnumerator];
        id tempObject;
        while (tempObject = [enumerator nextObject])
        {
            /* make any changes to the incoming preset we see fit */
            /* make sure the incoming preset is not tagged as default */
            [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
            /* prepend "(imported) to the name of the incoming preset for clarification since it can be changed */
            NSString * prependedName = [@"(import) " stringByAppendingString:[tempObject objectForKey:@"PresetName"]] ;
            [tempObject setObject:prependedName forKey:@"PresetName"];
            
            /* actually add the new preset to our presets array */
            [UserPresets addObject:tempObject];
            i++;
        }
        [presetsToImport autorelease];
        [self sortPresets];
        [self addPreset];
        
    }
}

#pragma mark -
#pragma mark Manage Default Preset

- (IBAction)getDefaultPresets:(id)sender
{
	presetHbDefault = nil;
    presetUserDefault = nil;
    presetUserDefaultParent = nil;
    presetUserDefaultParentParent = nil;
    NSMutableDictionary *presetHbDefaultParent = nil;
    NSMutableDictionary *presetHbDefaultParentParent = nil;
    
    int i = 0;
    BOOL userDefaultFound = NO;
    presetCurrentBuiltInCount = 0;
    /* First we iterate through the root UserPresets array to check for defaults */
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSMutableDictionary *thisPresetDict = tempObject;
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
		{
			presetHbDefault = thisPresetDict;	
		}
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
		{
			presetUserDefault = thisPresetDict;
            userDefaultFound = YES;
        }
        if ([[thisPresetDict objectForKey:@"Type"] intValue] == 0) // Type 0 is a built in preset		
        {
			presetCurrentBuiltInCount++; // <--increment the current number of built in presets	
		}
		i++;
        
        /* if we run into a folder, go to level 1 and iterate through the children arrays for the default */
        if ([thisPresetDict objectForKey:@"ChildrenArray"])
        {
            NSMutableDictionary *thisPresetDictParent = thisPresetDict;
            NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
            id tempObject;
            while (tempObject = [enumerator nextObject])
            {
                NSMutableDictionary *thisPresetDict = tempObject;
                if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
                {
                    presetHbDefault = thisPresetDict;
                    presetHbDefaultParent = thisPresetDictParent;
                }
                if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
                {
                    presetUserDefault = thisPresetDict;
                    presetUserDefaultParent = thisPresetDictParent;
                    userDefaultFound = YES;
                }
                
                /* if we run into a folder, go to level 2 and iterate through the children arrays for the default */
                if ([thisPresetDict objectForKey:@"ChildrenArray"])
                {
                    NSMutableDictionary *thisPresetDictParentParent = thisPresetDict;
                    NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
                    id tempObject;
                    while (tempObject = [enumerator nextObject])
                    {
                        NSMutableDictionary *thisPresetDict = tempObject;
                        if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
                        {
                            presetHbDefault = thisPresetDict;
                            presetHbDefaultParent = thisPresetDictParent;
                            presetHbDefaultParentParent = thisPresetDictParentParent;	
                        }
                        if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
                        {
                            presetUserDefault = thisPresetDict;
                            presetUserDefaultParent = thisPresetDictParent;
                            presetUserDefaultParentParent = thisPresetDictParentParent;
                            userDefaultFound = YES;	
                        }
                        
                    }
                }
            }
        }
        
	}
    /* check to see if a user specified preset was found, if not then assign the parents for
     * the presetHbDefault so that we can open the parents for the nested presets
     */
    if (userDefaultFound == NO)
    {
        presetUserDefaultParent = presetHbDefaultParent;
        presetUserDefaultParentParent = presetHbDefaultParentParent;
    }
}

- (IBAction)setDefaultPreset:(id)sender
{
/* We need to determine if the item is a folder */
   if ([[[self selectedPreset] objectForKey:@"Folder"] intValue] == 1)
   {
   return;
   }

    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
    id tempObject;
    /* First make sure the old user specified default preset is removed */
    while (tempObject = [enumerator nextObject])
    {
        NSMutableDictionary *thisPresetDict = tempObject;
        if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
        {
            [[UserPresets objectAtIndex:i] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
        }
		
        /* if we run into a folder, go to level 1 and iterate through the children arrays for the default */
        if ([thisPresetDict objectForKey:@"ChildrenArray"])
        {
            NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
            id tempObject;
            int ii = 0;
            while (tempObject = [enumerator nextObject])
            {
                NSMutableDictionary *thisPresetDict1 = tempObject;
                if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
                {
                    [[[thisPresetDict objectForKey:@"ChildrenArray"] objectAtIndex:ii] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
                }
                /* if we run into a folder, go to level 2 and iterate through the children arrays for the default */
                if ([thisPresetDict1 objectForKey:@"ChildrenArray"])
                {
                    NSEnumerator *enumerator = [[thisPresetDict1 objectForKey:@"ChildrenArray"] objectEnumerator];
                    id tempObject;
                    int iii = 0;
                    while (tempObject = [enumerator nextObject])
                    {
                        if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
                        {
                            [[[thisPresetDict1 objectForKey:@"ChildrenArray"] objectAtIndex:iii] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
                        }
                        iii++;
                    }
                }
                ii++;
            }

        }
        i++;
	}


    int presetToModLevel = [fPresetsOutlineView levelForItem: [self selectedPreset]];
    NSDictionary *presetToMod = [self selectedPreset];
    NSDictionary *presetToModParent = [fPresetsOutlineView parentForItem: presetToMod];


    NSMutableArray *presetsArrayToMod;
    NSMutableArray *tempArray;

    /* If we are a root level preset, we are modding the UserPresets array */
    if (presetToModLevel == 0)
    {
        presetsArrayToMod = UserPresets;
    }
    else // We have a parent preset, so we modify the chidren array object for key
    {
        presetsArrayToMod = [presetToModParent objectForKey:@"ChildrenArray"]; 
    }
    
    enumerator = [presetsArrayToMod objectEnumerator];
    tempArray = [NSMutableArray array];
    int iiii = 0;
    while (tempObject = [enumerator nextObject]) 
    {
        NSDictionary *thisPresetDict = tempObject;
        if (thisPresetDict == presetToMod)
        {
            if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 2
            {
                [[presetsArrayToMod objectAtIndex:iiii] setObject:[NSNumber numberWithInt:2] forKey:@"Default"];	
            }
        }
     iiii++;
     }
    
    
    /* We save all of the preset data here */
    [self savePreset];
    /* We Reload the New Table data for presets */
    [fPresetsOutlineView reloadData];
}

- (IBAction)selectDefaultPreset:(id)sender
{
	NSMutableDictionary *presetToMod;
    /* if there is a user specified default, we use it */
	if (presetUserDefault)
	{
        presetToMod = presetUserDefault;
    }
	else if (presetHbDefault) //else we use the built in default presetHbDefault
	{
        presetToMod = presetHbDefault;
	}
    else
    {
    return;
    }
    
    if (presetUserDefaultParent != nil)
    {
        [fPresetsOutlineView expandItem:presetUserDefaultParent];
        
    }
    if (presetUserDefaultParentParent != nil)
    {
        [fPresetsOutlineView expandItem:presetUserDefaultParentParent];
        
    }
    
    [fPresetsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:[fPresetsOutlineView rowForItem: presetToMod]] byExtendingSelection:NO];
	[self selectPreset:nil];
}


#pragma mark -
#pragma mark Manage Built In Presets


- (IBAction)deleteFactoryPresets:(id)sender
{
    //int status;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
    
	//NSNumber *index;
    NSMutableArray *tempArray;


        tempArray = [NSMutableArray array];
        /* we look here to see if the preset is we move on to the next one */
        while ( tempObject = [enumerator nextObject] )  
		{
			/* if the preset is "Factory" then we put it in the array of
			presets to delete */
			if ([[tempObject objectForKey:@"Type"] intValue] == 0)
			{
				[tempArray addObject:tempObject];
			}
        }
        
        [UserPresets removeObjectsInArray:tempArray];
        [fPresetsOutlineView reloadData];
        [self savePreset];   

}

   /* We use this method to recreate new, updated factory presets */
- (IBAction)addFactoryPresets:(id)sender
{
    
    /* First, we delete any existing built in presets */
    [self deleteFactoryPresets: sender];
    /* Then we generate new built in presets programmatically with fPresetsBuiltin
     * which is all setup in HBPresets.h and  HBPresets.m*/
    [fPresetsBuiltin generateBuiltinPresets:UserPresets];
    /* update build number for built in presets */
    /* iterate though the new array of presets to import and add them to our presets array */
    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        /* Record the apps current build number in the PresetBuildNumber key */
        if ([[tempObject objectForKey:@"Type"] intValue] == 0) // Type 0 is a built in preset		
        {
            /* Preset build number */
            [[UserPresets objectAtIndex:i] setObject:[NSNumber numberWithInt:[[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]] forKey:@"PresetBuildNumber"];
        }
        i++;
    }
    /* report the built in preset updating to the activity log */
    [self writeToActivityLog: "built in presets updated to build number: %d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]];
    
    [self sortPresets];
    [self addPreset];
    
}

#pragma mark -
#pragma mark Chapter Files Import / Export

- (IBAction) browseForChapterFile: (id) sender
{
	/* Open a panel to let the user choose the file */
	NSOpenPanel * panel = [NSOpenPanel openPanel];
	/* We get the current file name and path from the destination field here */
	[panel beginSheetForDirectory: [NSString stringWithFormat:@"%@/",
                                    [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"]]
                             file: NULL
                            types: [NSArray arrayWithObjects:@"csv",nil]
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseForChapterFileDone:returnCode:contextInfo: )
                      contextInfo: NULL];
}

- (void) browseForChapterFileDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    NSArray *chaptersArray; /* temp array for chapters */
	NSMutableArray *chaptersMutableArray; /* temp array for chapters */
    NSString *chapterName; 	/* temp string from file */
    int chapters, i;
    
    if( returnCode == NSOKButton )  /* if they click OK */
    {	
        chapterName = [[NSString alloc] initWithContentsOfFile:[sheet filename] encoding:NSUTF8StringEncoding error:NULL];
        chaptersArray = [chapterName componentsSeparatedByString:@"\n"];
        chaptersMutableArray= [chaptersArray mutableCopy];
		chapters = [fChapterTitlesDelegate numberOfRowsInTableView:fChapterTable];
        if ([chaptersMutableArray count] > 0)
        { 
        /* if last item is empty remove it */
            if ([[chaptersMutableArray objectAtIndex:[chaptersArray count]-1] length] == 0)
            {
                [chaptersMutableArray removeLastObject];
            }
        }
        /* if chapters in table is not equal to array count */
        if ((unsigned int) chapters != [chaptersMutableArray count])
        {
            [sheet close];
            [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", @"Unable to load chapter file")
                             defaultButton:NSLocalizedString(@"OK", @"OK")
                           alternateButton:NULL 
                               otherButton:NULL
                 informativeTextWithFormat:NSLocalizedString(@"%d chapters expected, %d chapters found in %@", @"%d chapters expected, %d chapters found in %@"), 
              chapters, [chaptersMutableArray count], [[sheet filename] lastPathComponent]] runModal];
            return;
        }
		/* otherwise, go ahead and populate table with array */
		for (i=0; i<chapters; i++)
        {
         
            if([[chaptersMutableArray objectAtIndex:i] length] > 5)
            { 
                /* avoid a segfault */
                /* Get the Range.location of the first comma in the line and then put everything after that into chapterTitle */
                NSRange firstCommaRange = [[chaptersMutableArray objectAtIndex:i] rangeOfString:@","];
                NSString *chapterTitle = [[chaptersMutableArray objectAtIndex:i] substringFromIndex:firstCommaRange.location + 1];
                /* Since we store our chapterTitle commas as "\," for the cli, we now need to remove the escaping "\" from the title */
                chapterTitle = [chapterTitle stringByReplacingOccurrencesOfString:@"\\," withString:@","];
                [fChapterTitlesDelegate tableView:fChapterTable 
                                   setObjectValue:chapterTitle
                                   forTableColumn:fChapterTableNameColumn
                                              row:i];
            }
            else 
            {
                [sheet close];
                [[NSAlert alertWithMessageText:NSLocalizedString(@"Unable to load chapter file", @"Unable to load chapter file")
                                 defaultButton:NSLocalizedString(@"OK", @"OK")
                               alternateButton:NULL 
                                   otherButton:NULL
                     informativeTextWithFormat:NSLocalizedString(@"%@ was not formatted as expected.", @"%@ was not formatted as expected."), [[sheet filename] lastPathComponent]] runModal];   
                [fChapterTable reloadData];
                return;
            }
        }
        [fChapterTable reloadData];
    }
}

- (IBAction) browseForChapterFileSave: (id) sender
{
    NSSavePanel *panel = [NSSavePanel savePanel];
    /* Open a panel to let the user save to a file */
    [panel setAllowedFileTypes:[NSArray arrayWithObjects:@"csv",nil]];
    [panel beginSheetForDirectory: [[fDstFile2Field stringValue] stringByDeletingLastPathComponent] 
                             file: [[[[fDstFile2Field stringValue] lastPathComponent] stringByDeletingPathExtension] 
                                     stringByAppendingString:@"-chapters.csv"]
                   modalForWindow: fWindow 
                    modalDelegate: self
                   didEndSelector: @selector( browseForChapterFileSaveDone:returnCode:contextInfo: )
                      contextInfo: NULL];
}

- (void) browseForChapterFileSaveDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    NSString *chapterName;      /* pointer for string for later file-writing */
    NSString *chapterTitle;
    NSError *saveError = [[NSError alloc] init];
    int chapters, i;    /* ints for the number of chapters in the table and the loop */
    
    if( returnCode == NSOKButton )   /* if they clicked OK */
    {	
        chapters = [fChapterTitlesDelegate numberOfRowsInTableView:fChapterTable];
        chapterName = [NSString string];
        for (i=0; i<chapters; i++)
        {
            /* put each chapter title from the table into the array */
            if (i<9)
            { /* if i is from 0 to 8 (chapters 1 to 9) add two leading zeros */
                chapterName = [chapterName stringByAppendingFormat:@"00%d,",i+1];
            }
            else if (i<99)
            { /* if i is from 9 to 98 (chapters 10 to 99) add one leading zero */
                chapterName = [chapterName stringByAppendingFormat:@"0%d,",i+1];
            }
            else if (i<999)
            { /* in case i is from 99 to 998 (chapters 100 to 999) no leading zeros */
                chapterName = [chapterName stringByAppendingFormat:@"%d,",i+1];
            }
            
            chapterTitle = [fChapterTitlesDelegate tableView:fChapterTable objectValueForTableColumn:fChapterTableNameColumn row:i];
            /* escape any commas in the chapter name with "\," */
            chapterTitle = [chapterTitle stringByReplacingOccurrencesOfString:@"," withString:@"\\,"];
            chapterName = [chapterName stringByAppendingString:chapterTitle];
            if (i+1 != chapters)
            { /* if not the last chapter */
                chapterName = [chapterName stringByAppendingString:@ "\n"];
            }

            
        }
        /* try to write it to where the user wanted */
        if (![chapterName writeToFile:[sheet filename] 
                           atomically:NO 
                             encoding:NSUTF8StringEncoding 
                                error:&saveError])
        {
            [sheet close];
            [[NSAlert alertWithError:saveError] runModal];
        }
    }
}

@end

/*******************************
 * Subclass of the HBPresetsOutlineView *
 *******************************/

@implementation HBPresetsOutlineView
- (NSImage *)dragImageForRowsWithIndexes:(NSIndexSet *)dragRows tableColumns:(NSArray *)tableColumns event:(NSEvent*)dragEvent offset:(NSPointPointer)dragImageOffset
{
    fIsDragging = YES;

    // By default, NSTableView only drags an image of the first column. Change this to
    // drag an image of the queue's icon and PresetName columns.
    NSArray * cols = [NSArray arrayWithObjects: [self tableColumnWithIdentifier:@"PresetName"], nil];
    return [super dragImageForRowsWithIndexes:dragRows tableColumns:cols event:dragEvent offset:dragImageOffset];
}



- (void) mouseDown:(NSEvent *)theEvent
{
    [super mouseDown:theEvent];
	fIsDragging = NO;
}



- (BOOL) isDragging;
{
    return fIsDragging;
}
@end



