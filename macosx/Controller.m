/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <dlfcn.h>
#import "Controller.h"
#import "HBOutputPanelController.h"
#import "HBPreferencesController.h"
#import "HBDVDDetector.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"
#import "HBPreviewController.h"
#import "DockTextField.h"
#import "HBUtilities.h"

#import "HBPresetsViewController.h"

#import "HBAudioSettings.h"

NSString *HBContainerChangedNotification       = @"HBContainerChangedNotification";
NSString *keyContainerTag                      = @"keyContainerTag";
NSString *HBTitleChangedNotification           = @"HBTitleChangedNotification";
NSString *keyTitleTag                          = @"keyTitleTag";

NSString *dragDropFiles                        = @"dragDropFiles";

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

@interface HBController () <HBPresetsViewControllerDelegate>
@end

/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- (id)init
{
    self = [super init];
    if( !self )
    {
        return nil;
    }

    [HBPreferencesController registerUserDefaults];
    fHandle = NULL;
    fQueueEncodeLibhb = NULL;

    /* Check for and create the App Support Preview directory if necessary */
    NSString *PreviewDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Previews"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:PreviewDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:PreviewDirectory
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:NULL];
    }                                                            
    outputPanel = [[HBOutputPanelController alloc] init];
    fPictureController = [[HBPictureController alloc] init];
    fQueueController = [[HBQueueController alloc] init];

    /* we init the HBPresetsManager class */
    NSURL *presetsURL = [NSURL fileURLWithPath:[[HBUtilities appSupportPath] stringByAppendingPathComponent:@"UserPresets.plist"]];
    presetManager = [[HBPresetsManager alloc] initWithURL:presetsURL];

    fPreferencesController = [[HBPreferencesController alloc] init];
    /* Lets report the HandBrake version number here to the activity log and text log file */
    NSString *versionStringFull = [[NSString stringWithFormat: @"Handbrake Version: %@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"]] stringByAppendingString: [NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
    [HBUtilities writeToActivityLog: "%s", [versionStringFull UTF8String]];
    
    /* Load the dockTile and instiante initial text fields */
    dockTile = [[NSApplication sharedApplication] dockTile];
    NSImageView *iv = [[NSImageView alloc] init];
    [iv setImage:[[NSApplication sharedApplication] applicationIconImage]];
    [dockTile setContentView:iv];
    [iv release];
    
    /* We can move the specific values out from here by subclassing NSDockTile and package everything in here */
    /* If colors are to be chosen once and for all, we can also remove the instantiation with numerical values */
    percentField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 32.0f, [dockTile size].width, 30.0f)];
    [percentField changeGradientColors:[NSColor colorWithDeviceRed:0.4f green:0.6f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.2f green:0.4f blue:0.2f alpha:1.0f]];
    [iv addSubview:percentField];
    
    timeField = [[DockTextField alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, [dockTile size].width, 30.0f)];
    [timeField changeGradientColors:[NSColor colorWithDeviceRed:0.6f green:0.4f blue:0.4f alpha:1.0f] endColor:[NSColor colorWithDeviceRed:0.4f green:0.2f blue:0.2f alpha:1.0f]];
    [iv addSubview:timeField];
    
    [self updateDockIcon:-1.0 withETA:@""];
    
    /* Init libhb with check for updates libhb style set to "0" so its ignored and lets sparkle take care of it */
    int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
    fHandle = hb_init(loggingLevel, 0);
    /* Optional dvd nav UseDvdNav*/
    hb_dvd_set_dvdnav([[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue]);
    /* Init a separate instance of libhb for user scanning and setting up jobs */
    fQueueEncodeLibhb = hb_init(loggingLevel, 0);

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
    
    NSMutableArray *filesList = [[[NSMutableArray alloc] initWithArray:filenames] autorelease];
    [filesList removeObject:@"YES"];
    
    // For now, we just want to accept one file at a time
    // If for any reason, more than one file is submitted, we will take the first one
    if (filesList.count > 1)
    {
        filesList = [NSMutableArray arrayWithObject:[filesList objectAtIndex:0]];
    }
    
    // The goal of this check is to know if the application was running before the drag & drop
    // if fSubtitlesDelegate is set, then applicationDidFinishLaunching was called
    if (fSubtitlesViewController)
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
	// Set the Growl Delegate
    [GrowlApplicationBridge setGrowlDelegate: self];
    /* Init others controllers */
    [fPictureController setDelegate: self];
    [fPictureController setHandle: fHandle];

    [fQueueController   setHandle: fQueueEncodeLibhb];
    [fQueueController   setHBController: self];

	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(autoSetM4vExtension:) name: HBMixdownChangedNotification object: nil];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(updateMp4Checkboxes:) name: HBVideoEncoderChangedNotification object: nil];
    
    dockIconProgress = 0;
    
    /* Init QueueFile .plist */
    [self loadQueueFile];
    [self initQueueFSEvent];
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
        NSString *PreviewDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Previews"];
        NSError *error;
        NSArray *files = [ [NSFileManager defaultManager]  contentsOfDirectoryAtPath: PreviewDirectory error: &error ];
        for( NSString *file in files ) 
        {
            if( ![file  isEqual: @"."] && ![file  isEqual: @".."] )
            {
                [ [NSFileManager defaultManager] removeItemAtPath: [ PreviewDirectory stringByAppendingPathComponent: file ] error: &error ];
                if( error ) 
                { 
                    //an error occurred
                    [HBUtilities writeToActivityLog: "Could not remove existing preview at : %s",[file UTF8String] ];
                }
            }    
        }
        
    }

    [self enableUI: NO];
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
        
        /* On Screen Notification
         * We check to see if there is already another instance of hb running.
         * Note: hbInstances == 1 means we are the only instance of HandBrake.app
         */
        if (hbInstanceNum > 1)
        {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:NSLocalizedString(@"There is already an instance of HandBrake running.", @"")];
            [alert setInformativeText:NSLocalizedString(@"HandBrake will now load up the existing queue.", nil)];
            [alert addButtonWithTitle:NSLocalizedString(@"Reload Queue", nil)];
            [alert beginSheetModalForWindow:fWindow
                              modalDelegate:self
                             didEndSelector:@selector(didDimissReloadQueue:returnCode:contextInfo:)
                                contextInfo:nil];
            [alert release];
        }
        else
        {
            if (fWorkingCount > 0 || fPendingCount > 0)
            {
                NSString *alertTitle;

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

                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:alertTitle];
                [alert setInformativeText:NSLocalizedString(@"Do you want to reload them ?", nil)];
                [alert addButtonWithTitle:NSLocalizedString(@"Reload Queue", nil)];
                [alert addButtonWithTitle:NSLocalizedString(@"Empty Queue", nil)];
                [alert setAlertStyle:NSCriticalAlertStyle];
                [alert beginSheetModalForWindow:fWindow
                                  modalDelegate:self
                                 didEndSelector:@selector(didDimissReloadQueue:returnCode:contextInfo:)
                                    contextInfo:nil];
                [alert release];

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
    NSArray *runningInstances = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
    NSRunningApplication *runningInstance;
    
    NSRunningApplication *thisInstance = [NSRunningApplication currentApplication];
    NSString *thisInstanceAppPath = [[NSBundle mainBundle] bundlePath];
    [HBUtilities writeToActivityLog: "hbInstances path to this instance: %s", [thisInstanceAppPath UTF8String]];
    
    int hbInstances = 0;
    NSString *runningInstanceAppPath;
    pid_t runningInstancePidNum;
    
    for (runningInstance in runningInstances)
	{
        /*Report the path to each active instances app path */
        runningInstancePidNum =  [runningInstance processIdentifier];
        runningInstanceAppPath = [[runningInstance bundleURL] path];
        [HBUtilities writeToActivityLog: "hbInstance found instance pidnum: %d at path: %s", runningInstancePidNum, [runningInstanceAppPath UTF8String]];
        /* see if this is us*/
        if ([runningInstance isEqual: thisInstance])
        {
            /* If so this is our pidnum */
            [HBUtilities writeToActivityLog: "hbInstance MATCH FOUND, our pidnum is: %d", runningInstancePidNum];
            /* Get the PID number for this hb instance, used in multi instance encoding */
            pidNum = runningInstancePidNum;
            /* Report this pid to the activity log */
            [HBUtilities writeToActivityLog: "Pid for this instance: %d", pidNum];
            /* Tell fQueueController what our pidNum is */
            [fQueueController setPidNum:pidNum];
        }
        hbInstances++;
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
    
    [HBUtilities writeToActivityLog: "didDimissReloadQueue number of hb instances:%d", hbInstanceNum];
    if (returnCode == NSAlertSecondButtonReturn)
    {
        [HBUtilities writeToActivityLog: "didDimissReloadQueue NSAlertSecondButtonReturn Chosen"];
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
        [HBUtilities writeToActivityLog: "didDimissReloadQueue NSAlertFirstButtonReturn Chosen"];
        if (hbInstanceNum == 1)
        {
            
            [self setQueueEncodingItemsAsPending];
        }
        [self reloadQueue];
        [self showQueueWindow:NULL];
    }
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) app
{
    hb_state_t s;
    hb_get_state2(fQueueEncodeLibhb, &s);

    if (s.state != HB_STATE_IDLE)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil)];
        [alert setInformativeText:NSLocalizedString(@"If you quit HandBrake your current encode will be reloaded into your queue at next launch. Do you want to quit anyway?", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Quit", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Don't Quit", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        NSInteger result = [alert runModal];
        [alert release];

        if (result == NSAlertFirstButtonReturn)
        {
            return NSTerminateNow;
        }
        else
        {
            return NSTerminateCancel;
        }
    }

    // Warn if items still in the queue
    else if (fPendingCount > 0)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil)];
        [alert setInformativeText:NSLocalizedString(@"There are pending encodes in your queue. Do you want to quit anyway?",nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Quit", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Don't Quit", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];
        NSInteger result = [alert runModal];
        [alert release];
        if (result == NSAlertFirstButtonReturn)
        {
            return NSTerminateNow;
        }
        else
        {
            return NSTerminateCancel;
        }
    }

    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // When the application is closed and we still have some files in the dragDropFiles array
    // it's highly probable that the user throw a lot of files and just want to reset this
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:dragDropFiles];

    [presetManager savePresets];

    [self closeQueueFSEvent];
    [currentQueueEncodeNameString release];
    [browsedSourceDisplayName release];
    [outputPanel release];
	[fQueueController release];
    [fPreviewController release];
    [fPictureController release];

    hb_close(&fHandle);
    hb_close(&fQueueEncodeLibhb);
    hb_global_close();

}


- (void) awakeFromNib
{
    [fWindow center];
    [fWindow setExcludedFromWindowsMenu:NO];

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

    [fPresetDrawer setDelegate:self];
    NSSize drawerSize = NSSizeFromString([[NSUserDefaults standardUserDefaults]
                                           stringForKey:@"Drawer Size"]);
    if (drawerSize.width)
        [fPresetDrawer setContentSize: drawerSize];

	/* Show/Dont Show Presets drawer upon launch based
     on user preference DefaultPresetsDrawerShow*/
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPresetsDrawerShow"])
	{
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
    for (const hb_container_t *container = hb_container_get_next(NULL);
         container != NULL;
         container  = hb_container_get_next(container))
    {
        menuItem = [[fDstFormatPopUp menu] addItemWithTitle:[NSString stringWithUTF8String:container->name]
                                                     action:nil
                                              keyEquivalent:@""];
        [menuItem setTag:container->format];
    }
    // select the first container
    [fDstFormatPopUp selectItemAtIndex:0];
    [self formatPopUpChanged:nil];

    [fDstFile2Field setStringValue:[NSString
                                    stringWithFormat:@"%@/Desktop/Movie.mp4",
                                    NSHomeDirectory()]];

	/* Set Auto Crop to On at launch */
    [fPictureController setAutoCrop:YES];
		
    /* Bottom */
    [fStatusField setStringValue: @""];
    
	[self setupToolbar];

    /* Register HBController's Window as a receiver for files/folders drag & drop operations */
    [fWindow registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

    // Set up the preset drawer
    fPresetsView = [[HBPresetsViewController alloc] initWithPresetManager:presetManager];
    [fPresetDrawer setContentView:[fPresetsView view]];
    fPresetsView.delegate = self;

    [[fPresetDrawer contentView] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    // Set up the chapters title view
    fChapterTitlesController = [[HBChapterTitlesController alloc] init];
    [fChaptersTitlesView addSubview: [fChapterTitlesController view]];

    // make sure we automatically resize the controller's view to the current window size
	[[fChapterTitlesController view] setFrame: [fChaptersTitlesView bounds]];
    [[fChapterTitlesController view] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    // setup the subtitles view
    fSubtitlesViewController = [[HBSubtitlesController alloc] init];
	[fSubtitlesView addSubview: [fSubtitlesViewController view]];

    // make sure we automatically resize the controller's view to the current window size
	[[fSubtitlesViewController view] setFrame: [fSubtitlesView bounds]];
    [[fSubtitlesViewController view] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

	// setup the audio controller
    fAudioController = [[HBAudioController alloc] init];
	[fAudioView addSubview: [fAudioController view]];

    // make sure we automatically resize the controller's view to the current window size
	[[fAudioController view] setFrame: [fAudioView bounds]];
    [[fAudioController view] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    // setup the advanced view controller
    fAdvancedOptions = [[HBAdvancedController alloc] init];
	[fAdvancedView addSubview: [fAdvancedOptions view]];

    // make sure we automatically resize the controller's view to the current window size
	[[fAudioController view] setFrame: [fAudioView bounds]];
    [[fAudioController view] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    // setup the video view controller
    fVideoController = [[HBVideoController alloc] init];
    fVideoController.fAdvancedOptions = fAdvancedOptions;
    fVideoController.fHBController = self;
	[fVideoView addSubview: [fVideoController view]];

    // make sure we automatically resize the controller's view to the current window size
	[[fVideoController view] setFrame: [fVideoView bounds]];
    [[fVideoController view] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    [fWindow recalculateKeyViewLoop];

    // Presets initialization
    [self checkBuiltInsForUpdates];
    [self buildPresetsMenu];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(buildPresetsMenu) name:HBPresetsChangedNotification object:nil];
}

- (void) enableUI: (BOOL) b
{
    NSControl * controls[] =
    {
        fSrcTitleField, fSrcTitlePopUp,
        fSrcChapterStartPopUp, fSrcChapterToField,
        fSrcChapterEndPopUp, fSrcDuration1Field, fSrcDuration2Field,
        fDstFormatField, fDstFormatPopUp, fDstFile1Field, fDstFile2Field,
        fDstBrowseButton, fSrcAngleLabel,
        fSrcAnglePopUp, fDstMp4LargeFileCheck,
        fDstMp4HttpOptFileCheck, fDstMp4iPodFileCheck,
        fEncodeStartStopPopUp, fSrcTimeStartEncodingField,
        fSrcTimeEndEncodingField, fSrcFrameStartEncodingField,
        fSrcFrameEndEncodingField,
        
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

    [fPresetsView setUIEnabled:b];
    [fVideoController setUIEnabled:b];
    [fAudioController setUIEnabled:b];
    [fSubtitlesViewController setUIEnabled:b];
    [fChapterTitlesController setUIEnabled:b];
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
			[HBUtilities writeToActivityLog:"ScanDone state received from fHandle"];
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
			[HBUtilities writeToActivityLog:"ScanDone state received from fQueueEncodeLibhb"];
            [self processNewQueueEncode];
            [[fWindow toolbar] validateVisibleItems];
            
			break;
        }
#undef p
            
            
#define p s.param.working
            
        case HB_STATE_SEARCHING:
		{
            NSMutableString *string;

            /* Update text field */
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

    [self getQueueStats];

    // Finally after all UI updates, we look for a next dragDropItem to scan
    // fWillScan will signal that a scan will be launched, so we need to wait
    // the next idle cycle after the scan
    hb_get_state2( fHandle, &s );
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

            [filesList release];
        }
    }
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
        
        hb_get_state2( fHandle, &s );
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
    hb_get_state2( fQueueEncodeLibhb, &s );

    if (fQueueEncodeLibhb)
    {
        if (action == @selector(addToQueue:) || action == @selector(addAllTitlesToQueue:) || action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:))
            return SuccessfulScan && [fWindow attachedSheet] == nil;
        
        if (action == @selector(selectDefaultPreset:))
            return [fWindow attachedSheet] == nil;
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
        if (action == @selector(browseSources:))
        {
            hb_get_state2( fHandle, &s );

            if (s.state == HB_STATE_SCANNING)
                return NO;
            else
                return [fWindow attachedSheet] == nil;
        }
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
            [HBUtilities writeToActivityLog: "trying to send encode to: %s", [sendToApp UTF8String]];
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
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Put down that cocktail…"];
        [alert setInformativeText:@"Your HandBrake queue is done!"];
        [NSApp requestUserAttention:NSCriticalRequest];
        [alert runModal];
        [alert release];
    }

    /* If sleep has been selected */
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"] )
    {
        /* Sleep */
        NSDictionary *errorDict;
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to sleep"];
        [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
    /* If Shutdown has been selected */
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"] )
    {
        /* Shut Down */
        NSDictionary* errorDict;
        NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to shut down"];
        [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
}

#pragma mark -
#pragma mark Get New Source

/*Opens the source browse window, called from Open Source widgets */
- (IBAction) browseSources: (id) sender
{
    
    hb_state_t s;
    hb_get_state2( fHandle, &s );
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
    NSURL *sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastSourceDirectoryURL"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastSourceDirectoryURL"];
	}
	else
	{
		sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    [panel setDirectoryURL:sourceDirectory];
    [panel beginSheetModalForWindow:fWindow completionHandler:
     ^(NSInteger result) {
         [self browseSourcesDone:panel returnCode:(int)result contextInfo:sender];
     }];
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
        // We started a new scan, so set SuccessfulScan to no for now.
        SuccessfulScan = NO;

            /* Free display name allocated previously by this code */
        [browsedSourceDisplayName release];
       
        NSURL *scanURL = [[sheet URLs] objectAtIndex: 0];
        /* we set the last searched source directory in the prefs here */
        NSURL *sourceDirectory = [scanURL URLByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setURL:sourceDirectory forKey:@"LastSourceDirectoryURL"];
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
            [fScanSrcTitlePathField setStringValue:[scanURL path]];
            NSString *displayTitlescanSourceName;

            if ([[scanURL lastPathComponent] isEqualToString: @"VIDEO_TS"])
            {
                /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
                 we have to use the title->path value so we get the proper name of the volume if a physical dvd is the source*/
                displayTitlescanSourceName = [[scanURL URLByDeletingLastPathComponent] lastPathComponent];
            }
            else
            {
                /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                displayTitlescanSourceName = [scanURL lastPathComponent];
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
            NSURL *url = [[sheet URLs] objectAtIndex: 0];
            
            /* We check to see if the chosen file at path is a package */
            if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:[url path]])
            {
                [HBUtilities writeToActivityLog: "trying to open a package at: %s", [[url path] UTF8String]];
                /* We check to see if this is an .eyetv package */
                if ([[url pathExtension] isEqualToString: @"eyetv"])
                {
                    [HBUtilities writeToActivityLog:"trying to open eyetv package"];
                    /* We're looking at an EyeTV package - try to open its enclosed
                     .mpg media file */
                     browsedSourceDisplayName = [[[url URLByDeletingPathExtension] lastPathComponent] retain];
                    NSString *mpgname;
                    NSUInteger n = [[[url path] stringByAppendingString: @"/"]
                             completePathIntoString: &mpgname caseSensitive: YES
                             matchesIntoArray: nil
                             filterTypes: [NSArray arrayWithObject: @"mpg"]];
                    if (n > 0)
                    {
                        /* Found an mpeg inside the eyetv package, make it our scan path 
                        and call performScan on the enclosed mpeg */
                        [HBUtilities writeToActivityLog:"found mpeg in eyetv package"];
                        [self performScan:mpgname scanTitleNum:0];
                    }
                    else
                    {
                        /* We did not find an mpeg file in our package, so we do not call performScan */
                        [HBUtilities writeToActivityLog:"no valid mpeg in eyetv package"];
                    }
                }
                /* We check to see if this is a .dvdmedia package */
                else if ([[url pathExtension] isEqualToString: @"dvdmedia"])
                {
                    /* path IS a package - but dvdmedia packages can be treaded like normal directories */
                    browsedSourceDisplayName = [[[url URLByDeletingPathExtension] lastPathComponent] retain];
                    [HBUtilities writeToActivityLog:"trying to open dvdmedia package"];
                    [self performScan:[url path] scanTitleNum:0];
                }
                else
                {
                    /* The package is not an eyetv package, try to open it anyway */
                    browsedSourceDisplayName = [[url lastPathComponent] retain];
                    [HBUtilities writeToActivityLog:"not a known to package"];
                    [self performScan:[url path] scanTitleNum:0];
                }
            }
            else // path is not a package, so we treat it as a dvd parent folder or VIDEO_TS folder
            {
                /* path is not a package, so we call perform scan directly on our file */
                if ([[url lastPathComponent] isEqualToString: @"VIDEO_TS"])
                {
                    [HBUtilities writeToActivityLog:"trying to open video_ts folder (video_ts folder chosen)"];
                    /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name*/
                    browsedSourceDisplayName = [[[url URLByDeletingLastPathComponent] lastPathComponent] retain];
                }
                else
                {
                    [HBUtilities writeToActivityLog:"trying to open video_ts folder (parent directory chosen)"];
                    /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                    /* make sure we remove any path extension as this can also be an '.mpg' file */
                    browsedSourceDisplayName = [[url lastPathComponent] retain];
                }
                applyQueueToScan = NO;
                [self performScan:[url path] scanTitleNum:0];
            }

        }

    }
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
- (void) performScan:(NSString *) scanPath scanTitleNum: (NSInteger) scanTitleNum
{
    /* use a bool to determine whether or not we can decrypt using vlc */
    BOOL cancelScanDecrypt = 0;
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];

    [fPictureController setTitle:NULL];

	//	Notify anyone interested (audio/subtitles/chapters controller) that there's no title
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
        [HBUtilities writeToActivityLog: "trying to open a physical dvd at: %s", [scanPath UTF8String]];
        
        
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
            [HBUtilities writeToActivityLog: "libdvdcss.2.dylib not found for decrypting physical dvd"];
            NSInteger status;
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"Please note that HandBrake does not support the removal of copy-protection from DVD Discs. You can if you wish install libdvdcss or any other 3rd party software for this function."];
            [alert setInformativeText:@"Videolan.org provides libdvdcss if you are not currently using another solution."];
            [alert addButtonWithTitle:@"Get libdvdcss.pkg"];
            [alert addButtonWithTitle:@"Cancel Scan"];
            [alert addButtonWithTitle:@"Attempt Scan Anyway"];
            [NSApp requestUserAttention:NSCriticalRequest];
            status = [alert runModal];
            [alert release];

            if (status == NSAlertFirstButtonReturn)
            {
                /* User chose to go download vlc (as they rightfully should) so we send them to the vlc site */
                [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://download.videolan.org/libdvdcss/1.2.12/macosx/"]];
            }
            else if (status == NSAlertSecondButtonReturn)
            {
                /* User chose to cancel the scan */
                [HBUtilities writeToActivityLog: "Cannot open physical dvd, scan cancelled"];
            }
            else
            {
                /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
                cancelScanDecrypt = 0;
                [HBUtilities writeToActivityLog:"User overrode copy-protection warning - trying to open physical dvd without decryption"];
            }
            
        }
        else if (dvdcss != NULL)
        {
            /* VLC was found in /Applications so all is well, we can carry on using vlc's libdvdcss.dylib for decrypting if needed */
            [HBUtilities writeToActivityLog: "libdvdcss.2.dylib found for decrypting physical dvd"];
            dlclose(dvdcss);
        }
        else
        {
            /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
            cancelScanDecrypt = 0;
            [HBUtilities writeToActivityLog:"Copy-protection warning disabled in preferences - trying to open physical dvd without decryption"];
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
            [HBUtilities writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        else
        {
            // minimum title duration doesn't apply to title-specific scan
            // it doesn't apply to batch scan either, but we can't tell it apart from DVD & BD folders here
            [HBUtilities writeToActivityLog: "scanning titles with a duration of %d seconds or more", min_title_duration_seconds];
        }
        
        hb_system_sleep_prevent(fHandle);
        hb_scan(fHandle, [path UTF8String], (int)scanTitleNum, hb_num_previews, 1 ,
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

            // Notify PictureController that there's no title
            [fPictureController setTitle:NULL];

			//	Notify anyone interested (video/audio/subtitles/chapters controller) that there's no title
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
                [HBUtilities writeToActivityLog: "showNewScan: This is a queued item rescan"];
                
            }
            else if (applyQueueToScan == NO)
            {
                [HBUtilities writeToActivityLog: "showNewScan: This is a new source item scan"];
            }
            else
            {
                [HBUtilities writeToActivityLog: "showNewScan: cannot grok scan status"];
            }

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
                int videoContainer = (int)[[fDstFormatPopUp selectedItem] tag];
                const char *ext    = hb_container_get_default_extension(videoContainer);
                
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
                if ((videoContainer & HB_MUX_MASK_MP4) && (applyQueueToScan != YES))
                {
                    [self autoSetM4vExtension:sender];
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
            if (title && (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE))
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

            [self encodeStartStopPopUpChanged:nil];

            // Open preview window now if it was visible when HB was closed
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PreviewWindowIsOpen"])
                [self showPreviewWindow:nil];

            // Open picture sizing window now if it was visible when HB was closed
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PictureSizeWindowIsOpen"])
                [self showPicturePanel:nil];
                

            if (applyQueueToScan == YES)
            {
                /* we are a rescan of an existing queue item and need to apply the queued settings to the scan */
                [HBUtilities writeToActivityLog: "showNewScan: calling applyQueueSettingsToMainWindow"];
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
    NSString* destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
    [panel setDirectoryURL:[NSURL fileURLWithPath:destinationDirectory]];
    [panel setNameFieldStringValue:[[fDstFile2Field stringValue] lastPathComponent]];
    [panel beginSheetModalForWindow:fWindow completionHandler:^(NSInteger result) {
        [self browseFileDone:panel returnCode:(int)result contextInfo:sender];
    }];
}

- (void) browseFileDone: (NSSavePanel *) sheet
             returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fDstFile2Field setStringValue: [[sheet URL] path]];
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

static void queueFSEventStreamCallback(
                                ConstFSEventStreamRef streamRef,
                                void *clientCallBackInfo,
                                size_t numEvents,
                                void *eventPaths,
                                const FSEventStreamEventFlags eventFlags[],
                                const FSEventStreamEventId eventIds[])
{
    HBController *hb = (HBController *)clientCallBackInfo;
    [hb reloadQueue];
}

- (void)initQueueFSEvent
{
    /* Define variables and create a CFArray object containing
     CFString objects containing paths to watch.
    */
    CFStringRef mypath = (CFStringRef) [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Queue"];
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mypath, 1, NULL);

    FSEventStreamContext callbackCtx;
    callbackCtx.version			= 0;
    callbackCtx.info			= self;
    callbackCtx.retain			= NULL;
    callbackCtx.release			= NULL;
    callbackCtx.copyDescription	= NULL;

    CFAbsoluteTime latency = 0.5; /* Latency in seconds */

    /* Create the stream, passing in a callback */
    QueueStream = FSEventStreamCreate(NULL,
                                 &queueFSEventStreamCallback,
                                 &callbackCtx,
                                 pathsToWatch,
                                 kFSEventStreamEventIdSinceNow,
                                 latency,
                                 kFSEventStreamCreateFlagIgnoreSelf
                                 );

    CFRelease(pathsToWatch);

    /* Create the stream before calling this. */
    FSEventStreamScheduleWithRunLoop(QueueStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(QueueStream);
}

- (void)closeQueueFSEvent
{
    FSEventStreamStop(QueueStream);
    FSEventStreamInvalidate(QueueStream);
    FSEventStreamRelease(QueueStream);
}

- (void)loadQueueFile
{
	/* We declare the default NSFileManager into fileManager */
	NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *appSupportPath = [HBUtilities appSupportPath];

	/* We define the location of the user presets file */
    QueueFile = [[appSupportPath stringByAppendingPathComponent:@"Queue/Queue.plist"] retain];

    /* We check for the Queue.plist */
	if( ![fileManager fileExistsAtPath:QueueFile] )
	{
        if( ![fileManager fileExistsAtPath:[appSupportPath stringByAppendingPathComponent:@"Queue"]] )
        {
            [fileManager createDirectoryAtPath:[appSupportPath stringByAppendingPathComponent:@"Queue"] withIntermediateDirectories:YES attributes:nil error:NULL];
        }

		[fileManager createFileAtPath:QueueFile contents:nil attributes:nil];
	}

	QueueFileArray = [[NSMutableArray alloc] initWithContentsOfFile:QueueFile];
	/* lets check to see if there is anything in the queue file .plist */
    if( QueueFileArray == nil )
	{
        /* if not, then lets initialize an empty array */
		QueueFileArray = [[NSMutableArray alloc] init];
    }
    else
    {
        /* ONLY clear out encoded items if we are single instance */
        if( hbInstanceNum == 1 )
        {
            [self clearQueueEncodedItems];
        }
    }
}

- (void)reloadQueue
{
    [HBUtilities writeToActivityLog:"Queue reloaded"];

    NSMutableArray * tempQueueArray = [[NSMutableArray alloc] initWithContentsOfFile:QueueFile];
    [QueueFileArray setArray:tempQueueArray];
    [tempQueueArray release];
    /* Send Fresh QueueFileArray to fQueueController to update queue window */
    [fQueueController setQueueArray: QueueFileArray];
}

- (void)addQueueFileItem
{
    [QueueFileArray addObject:[self createQueueFileItem]];
    [self saveQueueFileItem];
    
}

- (void) removeQueueFileItem:(NSUInteger) queueItemToRemove
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
	for (id tempObject in QueueFileArray)
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
- (NSInteger)getNextPendingQueueIndex
{
    /* initialize nextPendingIndex to -1, this value tells incrementQueueItemDone that there are no pending items in the queue */
    NSInteger nextPendingIndex = -1;
	BOOL nextPendingFound = NO;
	for (id tempObject in QueueFileArray)
	{
		NSDictionary *thisQueueDict = tempObject;
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 2 && nextPendingFound == NO) // pending		
        {
			nextPendingFound = YES;
            nextPendingIndex = [QueueFileArray indexOfObject: tempObject];
            [HBUtilities writeToActivityLog: "getNextPendingQueueIndex next pending encode index is:%d", nextPendingIndex];
		}
	}
    return nextPendingIndex;
}

/* This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void) setQueueEncodingItemsAsPending
{
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    for (id tempObject in QueueFileArray)
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
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    for (id tempObject in QueueFileArray)
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
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    for (id tempObject in QueueFileArray)
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
            (int)[fSrcTitlePopUp indexOfSelectedItem] );
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
    [queueFileJob setObject:[NSNumber numberWithInteger:[fSrcAnglePopUp indexOfSelectedItem] + 1] forKey:@"TitleAngle"];
    
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
    [queueFileJob setObject:[NSNumber numberWithInteger:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"ChapterStart"];
    [queueFileJob setObject:[NSNumber numberWithInteger:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"ChapterEnd"];
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
    [queueFileJob setObject:[NSNumber numberWithInteger:fPresetsView.indexOfSelectedItem] forKey:@"PresetIndexNum"];

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
        [queueFileJob setObject:@(fChapterTitlesController.createChapterMarkers) forKey:@"ChapterMarkers"];
    }
	
    /* We need to get the list of chapter names to put into an array and store 
     * in our queue, so they can be reapplied in prepareJob when this queue
     * item comes up if Chapter Markers is set to on.
     */
    [queueFileJob setObject:fChapterTitlesController.chapterTitlesArray forKey:@"ChapterNames"];
    
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
	[queueFileJob setObject:[NSNumber numberWithInteger:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
    /* Mux mp4 with http optimization */
    [queueFileJob setObject:[NSNumber numberWithInteger:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
    /* Add iPod uuid atom */
    [queueFileJob setObject:[NSNumber numberWithInteger:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
    
    /* Codecs */
	/* Video encoder */
    [fVideoController prepareVideoForQueueFileJob:queueFileJob];

	/* Picture Sizing */
	/* Use Max Picture settings for whatever the dvd is.*/
	[queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.keep_display_aspect] forKey:@"PictureKeepRatio"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->modulus] forKey:@"PictureModulus"];
    [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_width] forKey:@"PicturePARPixelWidth"];
    [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_height] forKey:@"PicturePARPixelHeight"];

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
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
    [queueFileJob setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController decomb]] forKey:@"PictureDecomb"];
    [queueFileJob setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
    [queueFileJob setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController denoise]] forKey:@"PictureDenoise"];
    [queueFileJob setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
    
    [queueFileJob setObject:[NSString stringWithFormat:@"%ld",(long)[fPictureController deblock]] forKey:@"PictureDeblock"];
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[fPictureController grayscale]] forKey:@"VideoGrayScale"];
    
    /* Auto Passthru */
    [queueFileJob setObject:@(fAudioController.settings.allowAACPassthru) forKey: @"AudioAllowAACPass"];
    [queueFileJob setObject:@(fAudioController.settings.allowAC3Passthru) forKey: @"AudioAllowAC3Pass"];
    [queueFileJob setObject:@(fAudioController.settings.allowDTSHDPassthru) forKey: @"AudioAllowDTSHDPass"];
    [queueFileJob setObject:@(fAudioController.settings.allowDTSPassthru) forKey: @"AudioAllowDTSPass"];
    [queueFileJob setObject:@(fAudioController.settings.allowMP3Passthru) forKey: @"AudioAllowMP3Pass"];
    // just in case we need it for display purposes
    [queueFileJob setObject:@(hb_audio_encoder_get_name((int)fAudioController.settings.encoderFallback)) forKey: @"AudioEncoderFallback"];
    // actual fallback encoder
    [queueFileJob setObject:@(fAudioController.settings.encoderFallback) forKey: @"JobAudioEncoderFallback"];

    /* Audio */
    NSMutableArray *audioArray = [[NSMutableArray alloc] initWithArray:[fAudioController audioTracks] copyItems:YES];
    [queueFileJob setObject:[NSArray arrayWithArray: audioArray] forKey:@"AudioList"];
    [audioArray release];

	/* Subtitles */
    NSMutableArray *subtitlesArray = [[NSMutableArray alloc] initWithArray:[fSubtitlesViewController subtitles] copyItems:YES];
    [queueFileJob setObject:[NSArray arrayWithArray: subtitlesArray] forKey:@"SubtitleList"];
    [subtitlesArray release];

    /* Now we go ahead and set the "job->values in the plist for passing right to fQueueEncodeLibhb */
     
    [queueFileJob setObject:[NSNumber numberWithInteger:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterStart"];
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterEnd"];
    
    
    [queueFileJob setObject:[NSNumber numberWithInteger:[[fDstFormatPopUp selectedItem] tag]] forKey:@"JobFileFormatMux"];
    
    /* Codecs */
    /* Framerate */
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate]                         forKey:@"JobVrate"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate_base]                    forKey:@"JobVrateBase"];

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

   /* We save all of the Queue data here 
    * and it also gets sent back to the queue controller*/
    [self saveQueueFileItem]; 
    
}


#pragma mark -
#pragma mark Queue Job Processing

- (void) incrementQueueItemDone:(NSInteger) queueItemDoneIndexNum
{
    /* Mark the encode just finished as done (status 0)*/
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:0] forKey:@"Status"];
	
    /* We save all of the Queue data here */
    [self saveQueueFileItem];

    /* Since we have now marked a queue item as done
     * we can go ahead and increment currentQueueEncodeIndex 
     * so that if there is anything left in the queue we can
     * go ahead and move to the next item if we want to */
    NSInteger queueItems = [QueueFileArray count];
    /* Check to see if there are any more pending items in the queue */
    NSInteger newQueueItemIndex = [self getNextPendingQueueIndex];
    /* If we still have more pending items in our queue, lets go to the next one */
    if (newQueueItemIndex >= 0 && newQueueItemIndex < queueItems)
    {
        /*Set our currentQueueEncodeIndex now to the newly found Pending encode as we own it */
        currentQueueEncodeIndex = newQueueItemIndex;
        /* now we mark the queue item as Status = 1 ( being encoded ) so another instance can not come along and try to scan it while we are scanning */
        [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
        [HBUtilities writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];
        /* now we can go ahead and scan the new pending queue item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];

    }
    else
    {
        [HBUtilities writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
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
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (NSInteger) scanTitleNum
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
            [HBUtilities writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        /*
         * Only scan 10 previews before an encode - additional previews are
         * only useful for autocrop and static previews, which are already taken
         * care of at this point
         */
        hb_system_sleep_prevent(fQueueEncodeLibhb);
        hb_scan(fQueueEncodeLibhb, [path UTF8String], (int)scanTitleNum, 10, 0, 0);
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
        [HBUtilities writeToActivityLog: "processNewQueueEncode WARNING nothing found in the title list"];
    }
    
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    [HBUtilities writeToActivityLog: "Preset: %s", [[queueToApply objectForKey:@"PresetName"] UTF8String]];
    [HBUtilities writeToActivityLog: "processNewQueueEncode number of passes expected is: %d", ([[queueToApply objectForKey:@"VideoTwoPass"] intValue] + 1)];
    hb_job_set_file(job, [[queueToApply objectForKey:@"DestinationPath"] UTF8String]);
    [self prepareJob];
    
    /*
     * If scanning we need to do some extra setup of the job.
     */
    if (job->indepth_scan == 1)
    {
        char *encoder_preset_tmp  = job->encoder_preset  != NULL ? strdup(job->encoder_preset)  : NULL;
        char *encoder_tune_tmp    = job->encoder_tune    != NULL ? strdup(job->encoder_tune)    : NULL;
        char *encoder_options_tmp = job->encoder_options != NULL ? strdup(job->encoder_options) : NULL;
        char *encoder_profile_tmp = job->encoder_profile != NULL ? strdup(job->encoder_profile) : NULL;
        char *encoder_level_tmp   = job->encoder_level   != NULL ? strdup(job->encoder_level)   : NULL;
        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        hb_job_set_encoder_preset (job, NULL);
        hb_job_set_encoder_tune   (job, NULL);
        hb_job_set_encoder_options(job, NULL);
        hb_job_set_encoder_profile(job, NULL);
        hb_job_set_encoder_level  (job, NULL);
        job->pass = -1;
        hb_add(fQueueEncodeLibhb, job);
        /*
         * reset the advanced settings
         */
        hb_job_set_encoder_preset (job, encoder_preset_tmp);
        hb_job_set_encoder_tune   (job, encoder_tune_tmp);
        hb_job_set_encoder_options(job, encoder_options_tmp);
        hb_job_set_encoder_profile(job, encoder_profile_tmp);
        hb_job_set_encoder_level  (job, encoder_level_tmp);
        free(encoder_preset_tmp);
        free(encoder_tune_tmp);
        free(encoder_options_tmp);
        free(encoder_profile_tmp);
        free(encoder_level_tmp);
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
- (void)rescanQueueItemToMainWindow:(NSString *) scanPath scanTitleNum: (NSUInteger) scanTitleNum selectedQueueItem: (NSUInteger) selectedQueueItem
{
    fqueueEditRescanItemNum = selectedQueueItem;
    [HBUtilities writeToActivityLog: "rescanQueueItemToMainWindow: Re-scanning queue item at index:%d",fqueueEditRescanItemNum];
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
        [HBUtilities writeToActivityLog: "applyQueueSettingsToMainWindow: queue item found"];
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
    fChapterTitlesController.createChapterMarkers = [[queueToApply objectForKey:@"ChapterMarkers"] boolValue];
    [fChapterTitlesController addChaptersFromQueue:[queueToApply objectForKey:@"ChapterNames"]];

    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
    [fDstMp4LargeFileCheck setState:[[queueToApply objectForKey:@"Mp4LargeFile"] intValue]];
    /* Mux mp4 with http optimization */
    [fDstMp4HttpOptFileCheck setState:[[queueToApply objectForKey:@"Mp4HttpOptimize"] intValue]];

    /* Set the state of ipod compatible with Mp4iPodCompatible. Only for x264*/
    [fDstMp4iPodFileCheck setState:[[queueToApply objectForKey:@"Mp4iPodCompatible"] intValue]];

    /* video encoder */
    [fVideoController applyVideoSettingsFromQueue:queueToApply];

    /* Auto Passthru */
    fAudioController.settings.allowAACPassthru = [[queueToApply objectForKey:@"AudioAllowAACPass"] boolValue];
    fAudioController.settings.allowAC3Passthru = [[queueToApply objectForKey:@"AudioAllowAC3Pass"] boolValue];
    fAudioController.settings.allowDTSHDPassthru = [[queueToApply objectForKey:@"AudioAllowDTSHDPass"] boolValue];
    fAudioController.settings.allowDTSPassthru = [[queueToApply objectForKey:@"AudioAllowDTSPass"] boolValue];
    fAudioController.settings.allowMP3Passthru = [[queueToApply objectForKey:@"AudioAllowMP3Pass"] boolValue];
    fAudioController.settings.encoderFallback = [queueToApply objectForKey:@"AudioEncoderFallback"];

    /* Audio */
    /* Now lets add our new tracks to the audio list here */
    [fAudioController addTracksFromQueue: queueToApply];
    
    /* Subtitles */
    [fSubtitlesViewController addTracksFromQueue:[queueToApply objectForKey:@"SubtitleList"]];

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

    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    job->modulus = [[queueToApply objectForKey:@"PictureModulus"]  intValue];
    job->maxWidth = job->maxHeight = 0;
    job->anamorphic.keep_display_aspect = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    job->width  = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
    job->height  = [[queueToApply objectForKey:@"PictureHeight"]  intValue];

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
    [fPictureController setTitle:fTitle];
    [self pictureSettingsDidChange];

    if ([queueToApply objectForKey:@"PresetIndexNum"]) // This item used a preset so insert that info
	{
        /* somehow we need to figure out a way to tie the queue item to a preset if it used one */
	}
    else
    {
        /* Deselect the currently selected Preset if there is one*/
        [fPresetsView deselect];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
    }
    
    /* We need to set this bool back to NO, in case the user wants to do a scan */
    //applyQueueToScan = NO;
    
    /* Not that source is loaded and settings applied, delete the queue item from the queue */
    [HBUtilities writeToActivityLog: "applyQueueSettingsToMainWindow: deleting queue item:%d",fqueueEditRescanItemNum];
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
            (int)[fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    hb_filter_object_t * filter;
    /* set job->angle for libdvdnav */
    job->angle = (int)[fSrcAnglePopUp indexOfSelectedItem] + 1;
    /* Chapter selection */
    job->chapter_start = (int)[fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end   = (int)[fSrcChapterEndPopUp   indexOfSelectedItem] + 1;
	
    /* Format (Muxer) and Video Encoder */
    job->mux = (int)[[fDstFormatPopUp selectedItem] tag];

    /* Video Encoder */
    [fVideoController prepareVideoForJobPreview:job andTitle:title];

    /* Subtitle settings */
    BOOL one_burned = NO;
    int i = 0;

    for (id subtitleDict in fSubtitlesViewController.subtitles)
    {
        int subtitle = [subtitleDict[keySubTrackIndex] intValue];
        int force = [subtitleDict[keySubTrackForced] intValue];
        int burned = [subtitleDict[keySubTrackBurned] intValue];
        int def = [subtitleDict[keySubTrackDefault] intValue];

        // if i is 0, then we are in the first item of the subtitles which we need to
        // check for the "Foreign Audio Search" which would be keySubTrackIndex of -1

        /* if we are on the first track and using "Foreign Audio Search" */
        if (i == 0 && subtitle == -1)
        {
            [HBUtilities writeToActivityLog: "Foreign Language Search: %d", 1];

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
            if ([subtitleDict[keySubTrackType] intValue] == SRTSUB)
            {
                hb_subtitle_config_t sub_config;

                sub_config.offset = [subtitleDict[keySubTrackSrtOffset] intValue];

                /* we need to srncpy file name and codeset */
                strncpy(sub_config.src_filename, [subtitleDict[keySubTrackSrtFilePath] UTF8String], 255);
                sub_config.src_filename[255] = 0;
                strncpy(sub_config.src_codeset, [subtitleDict[keySubTrackSrtCharCode] UTF8String], 39);
                sub_config.src_codeset[39] = 0;

                if( !burned && hb_subtitle_can_pass( SRTSUB, job->mux ) )
                {
                    sub_config.dest = PASSTHRUSUB;
                }
                else if( hb_subtitle_can_burn( SRTSUB ) )
                {
                    // Only allow one subtitle to be burned into the video
                    if( one_burned )
                        continue;
                    one_burned = TRUE;
                    sub_config.dest = RENDERSUB;
                }

                sub_config.force = 0;
                sub_config.default_track = def;
                hb_srt_add( job, &sub_config, [subtitleDict[keySubTrackLanguageIsoCode] UTF8String]);
                continue;
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
        i++;
    }
    if( one_burned )
    {
        filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d",
                                      job->crop[0], job->crop[1],
                                      job->crop[2], job->crop[3]] UTF8String] );
    }

    /* Auto Passthru */
    job->acodec_copy_mask = 0;
    if (fAudioController.settings.allowAACPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_FFAAC;
    }
    if (fAudioController.settings.allowAC3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_AC3;
    }
    if (fAudioController.settings.allowDTSHDPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_HD;
    }
    if (fAudioController.settings.allowDTSPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA;
    }
    if (fAudioController.settings.allowMP3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3;
    }
    job->acodec_fallback = fAudioController.settings.encoderFallback;

    // First clear out any audio tracks in the job currently
    int audiotrack_count = hb_list_count(job->list_audio);
    for (i = 0; i < audiotrack_count; i++)
    {
        hb_audio_t *temp_audio = (hb_audio_t *) hb_list_item(job->list_audio, 0);
        hb_list_rem(job->list_audio, temp_audio);
    }

    /* Audio tracks and mixdowns */
    for (NSDictionary *audioDict in fAudioController.audioTracks)
    {
        hb_audio_config_t *audio = (hb_audio_config_t *)calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [audioDict[@"Track"] intValue];
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track                     = audio->in.track;
        audio->out.codec                     = [audioDict[@"JobEncoder"] intValue];
        audio->out.compression_level         = hb_audio_compression_get_default(audio->out.codec);
        audio->out.mixdown                   = [audioDict[@"JobMixdown"] intValue];
        audio->out.normalize_mix_level       = 0;
        audio->out.bitrate                   = [audioDict[@"JobBitrate"] intValue];
        audio->out.samplerate                = [audioDict[@"JobSamplerate"] intValue];
        audio->out.dynamic_range_compression = [audioDict[@"TrackDRCSlider"] floatValue];
        audio->out.gain                      = [audioDict[@"TrackGainSlider"] floatValue];
        audio->out.dither_method             = hb_audio_dither_get_default();

        hb_audio_add(job, audio);
        free(audio);
    }

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
        hb_add_filter( job, filter, "2:1:1:2:3:3" );
	}
	else if ([fPictureController denoise] == 3) // Medium in popup
	{
        hb_add_filter( job, filter, "3:2:2:2:3:3" );
	}
	else if ([fPictureController denoise] == 4) // Strong in popup
	{
        hb_add_filter( job, filter, "7:7:7:5:5:5" );
	}
    
    
    /* Deblock  (uses pp7 default) */
    /* NOTE: even though there is a valid deblock setting of 0 for the filter, for 
     * the macgui's purposes a value of 0 actually means to not even use the filter
     * current hb_filter_deblock.settings valid ranges are from 5 - 15 
     */
    filter = hb_filter_init( HB_FILTER_DEBLOCK );
    if ([fPictureController deblock] != 0)
    {
        NSString *deblockStringValue = [NSString stringWithFormat: @"%ld",(long)[fPictureController deblock]];
        hb_add_filter( job, filter, [deblockStringValue UTF8String] );
    }

    /* Add Crop/Scale filter */
    filter = hb_filter_init( HB_FILTER_CROP_SCALE );
    hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d:%d:%d",
                                  job->width,job->height,
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );
}


#pragma mark -
#pragma mark Job Handling


- (void) prepareJob
{
    
    NSDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
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
        [HBUtilities writeToActivityLog: "Start / Stop set to chapters"];
        job->chapter_start = [[queueToApply objectForKey:@"JobChapterStart"] intValue];
        job->chapter_end   = [[queueToApply objectForKey:@"JobChapterEnd"] intValue];
    }
    else if ([[queueToApply objectForKey:@"fEncodeStartStop"] intValue] == 1)
    {
        /* we are pts based start / stop */
        [HBUtilities writeToActivityLog: "Start / Stop set to seconds…"];
        
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
        [HBUtilities writeToActivityLog: "Start / Stop set to frames…"];
        
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
         
        NSArray *chapterNamesArray = [queueToApply objectForKey:@"ChapterNames"];
        int i = 0;
        for (id tempObject in chapterNamesArray)
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
        const char *encoder_preset  = NULL;
        const char *encoder_tune    = NULL;
        const char *encoder_options = NULL;
        const char *encoder_profile = NULL;
        const char *encoder_level   = NULL;
        if ([[queueToApply objectForKey:@"x264UseAdvancedOptions"] intValue])
        {
            // we are using the advanced panel
            if ([(tmpString = [queueToApply objectForKey:@"x264Option"]) length])
            {
                encoder_options = [tmpString UTF8String];
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = [queueToApply objectForKey:@"x264Tune"]) length])
            {
                encoder_tune = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"x264OptionExtra"]) length])
            {
                encoder_options = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"h264Profile"]) length])
            {
                encoder_profile = [tmpString UTF8String];
            }
            if ([(tmpString = [queueToApply objectForKey:@"h264Level"]) length])
            {
                encoder_level = [tmpString UTF8String];
            }
            encoder_preset = [[queueToApply objectForKey:@"x264Preset"] UTF8String];
        }
        hb_job_set_encoder_preset (job, encoder_preset);
        hb_job_set_encoder_tune   (job, encoder_tune);
        hb_job_set_encoder_options(job, encoder_options);
        hb_job_set_encoder_profile(job, encoder_profile);
        hb_job_set_encoder_level  (job, encoder_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_encoder_options(job,
                                   [[queueToApply objectForKey:@"lavcOption"]
                                    UTF8String]);
    }

    /* Picture Size Settings */
    job->width = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
    job->height = [[queueToApply objectForKey:@"PictureHeight"]  intValue];

    job->anamorphic.keep_display_aspect = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    job->modulus = [[queueToApply objectForKey:@"PictureModulus"] intValue];
    job->anamorphic.par_width = [[queueToApply objectForKey:@"PicturePARPixelWidth"]  intValue];
    job->anamorphic.par_height = [[queueToApply objectForKey:@"PicturePARPixelHeight"]  intValue];
    job->anamorphic.dar_width = job->anamorphic.dar_height = 0;

    /* Here we use the crop values saved at the time the preset was saved */
    job->crop[0] = [[queueToApply objectForKey:@"PictureTopCrop"]  intValue];
    job->crop[1] = [[queueToApply objectForKey:@"PictureBottomCrop"]  intValue];
    job->crop[2] = [[queueToApply objectForKey:@"PictureLeftCrop"]  intValue];
    job->crop[3] = [[queueToApply objectForKey:@"PictureRightCrop"]  intValue];
    
    /* Video settings */
    /* Framerate */
    int fps_mode, fps_num, fps_den;
    if ([[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue] > 0)
    {
        /* a specific framerate has been chosen */
        fps_num = 27000000;
        fps_den = (int)[[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue];
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"cfr"])
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // PFR
            fps_mode = 2;
        }
    }
    else
    {
        /* same as source */
        fps_num = [[queueToApply objectForKey:@"JobVrate"]     intValue];
        fps_den = [[queueToApply objectForKey:@"JobVrateBase"] intValue];
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"cfr"])
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // VFR
            fps_mode = 0;
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

    // Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
    BOOL one_burned = NO;
    int i = 0;

    NSArray *subtitles = [queueToApply objectForKey:@"SubtitleList"];
    for (id subtitleDict in subtitles)
    {
        int subtitle = [subtitleDict[keySubTrackIndex] intValue];
        int force = [subtitleDict[keySubTrackForced] intValue];
        int burned = [subtitleDict[keySubTrackBurned] intValue];
        int def = [subtitleDict[keySubTrackDefault] intValue];

        // if i is 0, then we are in the first item of the subtitles which we need to
        // check for the "Foreign Audio Search" which would be keySubTrackIndex of -1

        // if we are on the first track and using "Foreign Audio Search"
        if (i == 0 && subtitle == -1)
        {
            [HBUtilities writeToActivityLog: "Foreign Language Search: %d", 1];

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
            // if we are getting the subtitles from an external srt file
            if ([subtitleDict[keySubTrackType] intValue] == SRTSUB)
            {
                hb_subtitle_config_t sub_config;

                sub_config.offset = [subtitleDict[keySubTrackSrtOffset] intValue];

                // we need to srncpy file name and codeset
                strncpy(sub_config.src_filename, [subtitleDict[keySubTrackSrtFilePath] UTF8String], 255);
                sub_config.src_filename[255] = 0;
                strncpy(sub_config.src_codeset, [subtitleDict[keySubTrackSrtCharCode] UTF8String], 39);
                sub_config.src_codeset[39] = 0;

                if( !burned && hb_subtitle_can_pass( SRTSUB, job->mux ) )
                {
                    sub_config.dest = PASSTHRUSUB;
                }
                else if( hb_subtitle_can_burn( SRTSUB ) )
                {
                    // Only allow one subtitle to be burned into the video
                    if( one_burned )
                        continue;
                    one_burned = TRUE;
                    sub_config.dest = RENDERSUB;
                }

                sub_config.force = 0;
                sub_config.default_track = def;
                hb_srt_add( job, &sub_config, [subtitleDict[keySubTrackLanguageIsoCode] UTF8String]);
                continue;
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
        i++;
    }
    if( one_burned )
    {
        filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d",
                                  job->crop[0], job->crop[1],
                                  job->crop[2], job->crop[3]] UTF8String] );
    }

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
    for (NSDictionary *audioDict in [queueToApply objectForKey:@"AudioList"])
    {
        audio           = (hb_audio_config_t *)calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [audioDict[@"Track"] intValue];
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track                     = audio->in.track;
        audio->out.codec                     = [audioDict[@"JobEncoder"] intValue];
        audio->out.compression_level         = hb_audio_compression_get_default(audio->out.codec);
        audio->out.mixdown                   = [audioDict[@"JobMixdown"] intValue];
        audio->out.normalize_mix_level       = 0;
        audio->out.bitrate                   = [audioDict[@"JobBitrate"] intValue];
        audio->out.samplerate                = [audioDict[@"JobSamplerate"] intValue];
        audio->out.dynamic_range_compression = [audioDict[@"TrackDRCSlider"] intValue];
        audio->out.gain                      = [audioDict[@"TrackGainSlider"] intValue];
        audio->out.dither_method             = hb_audio_dither_get_default();

        hb_audio_add(job, audio);
        free(audio);
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
        hb_add_filter( job, filter, "2:1:1:2:3:3" );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 3) // Medium in popup
	{
        hb_add_filter( job, filter, "3:2:2:2:3:3" );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 4) // Strong in popup
	{
        hb_add_filter( job, filter, "7:7:7:5:5:5" );	
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
    filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);

    [HBUtilities writeToActivityLog: "prepareJob exiting"];
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
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Warning!"];
        [alert setInformativeText:@"This is not a valid destination directory!"];
        [alert runModal];
        [alert release];
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
	for (id tempObject in QueueFileArray)
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"DestinationPath"] isEqualToString: [fDstFile2Field stringValue]])
		{
			fileExistsInQueue = YES;	
		}
	}

	if(fileExists == YES)
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"File already exists."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", [fDstFile2Field stringValue]];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:fWindow modalDelegate:self didEndSelector:@selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ) contextInfo:NULL];

    }
    else if (fileExistsInQueue == YES)
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"There is already a queue item for this destination."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", [fDstFile2Field stringValue]];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:fWindow modalDelegate:self didEndSelector:@selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ) contextInfo:NULL];
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
    [HBUtilities writeToActivityLog: "Rip: Pending queue count is %d", fPendingCount];
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
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Warning!"];
        [alert setInformativeText:@"This is not a valid destination directory!"];
        [alert runModal];
        [alert release];
        return;
    }
    
    /* We check for duplicate name here */
    if( [[NSFileManager defaultManager] fileExistsAtPath:[fDstFile2Field stringValue]] )
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"File already exists."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", [fDstFile2Field stringValue]];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:fWindow modalDelegate:self didEndSelector:@selector( overWriteAlertDone:returnCode:contextInfo: ) contextInfo:NULL];
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
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"The computer will sleep after encoding is done."];
        [alert setInformativeText:@"You have selected to sleep the computer after encoding. To turn off sleeping, go to the HandBrake preferences."];
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Preferences…"];

        NSInteger reminduser = [alert runModal];
        [alert release];
        if (reminduser == NSAlertSecondButtonReturn)
        {
            [self showPreferencesWindow:nil];
        }
    }
    else if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"])
    {
        /*Warn that computer will shut down after encoding*/
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"The computer will shut down after encoding is done."];
        [alert setInformativeText:@"You have selected to shut down the computer after encoding. To turn off shut down, go to the HandBrake preferences."];
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Preferences…"];

        NSInteger reminduser = [alert runModal];
        if (reminduser == NSAlertSecondButtonReturn)
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

    // Which window to attach the sheet to?
    NSWindow * docWindow;
    if ([sender respondsToSelector: @selector(window)])
    {
        docWindow = [sender window];
    }
    else
    {
        docWindow = fWindow;
    }

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"You are currently encoding. What would you like to do ?", nil)];
    [alert setInformativeText:NSLocalizedString(@"Your encode will be cancelled if you don't continue encoding.", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Continue Encoding", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Stop", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Continue", nil)];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert beginSheetModalForWindow:docWindow
                      modalDelegate:self
                     didEndSelector:@selector(didDimissCancel:returnCode:contextInfo:)
                        contextInfo:nil];
    [alert release];
}

- (void) didDimissCancel: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    /* No need to prevent system sleep here as we didn't allow it in Cancel: */
    hb_resume(fQueueEncodeLibhb);

    if (returnCode == NSAlertSecondButtonReturn)
    {
        [self doCancelCurrentJobAndStop];
    }
    else if (returnCode == NSAlertThirdButtonReturn)
    {
        [self doCancelCurrentJob];  // <- this also stops libhb
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
    NSInteger queueItems = [QueueFileArray count];
    /* If we still have more items in our queue, lets go to the next one */
    /* Check to see if there are any more pending items in the queue */
    NSInteger newQueueItemIndex = [self getNextPendingQueueIndex];
    /* If we still have more pending items in our queue, lets go to the next one */
    if (newQueueItemIndex >= 0 && newQueueItemIndex < queueItems)
    {
        /*Set our currentQueueEncodeIndex now to the newly found Pending encode as we own it */
        currentQueueEncodeIndex = newQueueItemIndex;
        /* now we mark the queue item as Status = 1 ( being encoded ) so another instance can not come along and try to scan it while we are scanning */
        [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
        [HBUtilities writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];
        /* now we can go ahead and scan the new pending queue item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];

    }
    else
    {
        [HBUtilities writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
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
    [HBUtilities writeToActivityLog: "cancelling current job and stopping the queue"];
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
    NSAlert *alert = [NSAlert alertWithMessageText:@"You are about to add ALL titles to the queue!"
                                     defaultButton:@"Cancel"
                                   alternateButton:@"Yes, I want to add all titles to the queue"
                                       otherButton:nil
                         informativeTextWithFormat:@"Current settings will be applied to all %ld titles. Are you sure you want to do this?", (long)[fSrcTitlePopUp numberOfItems]];
    [alert setAlertStyle:NSCriticalAlertStyle];

    [alert beginSheetModalForWindow:fWindow modalDelegate:self didEndSelector:@selector( addAllTitlesToQueueAlertDone:returnCode:contextInfo: ) contextInfo:NULL];
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
    NSInteger currentlySelectedTitle = [fSrcTitlePopUp indexOfSelectedItem];
    
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

- (void)updateFileName
{
    if (!SuccessfulScan)
    {
        return;
    }

    hb_list_t  *list  = hb_get_titles(fHandle);
    hb_title_t *title = (hb_title_t *)
    hb_list_item(list, (int)[fSrcTitlePopUp indexOfSelectedItem]);

    // Generate a new file name
    NSString *fileName = [HBUtilities automaticNameForSource:[browsedSourceDisplayName stringByDeletingPathExtension]
                                                       title: title->index
                                                    chapters:NSMakeRange([fSrcChapterStartPopUp indexOfSelectedItem] + 1, [fSrcChapterEndPopUp indexOfSelectedItem] + 1)
                                                     quality:fVideoController.selectedQualityType ? fVideoController.selectedQuality : 0
                                                     bitrate:!fVideoController.selectedQualityType ? fVideoController.selectedBitrate : 0
                                                  videoCodec:fVideoController.selectedCodec];

    // Swap the old one with the new one
    [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@/%@.%@",
                                     [[fDstFile2Field stringValue] stringByDeletingLastPathComponent],
                                     fileName,
                                     [[fDstFile2Field stringValue] pathExtension]]];
}

- (IBAction) titlePopUpChanged: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
        hb_list_item( list, (int)[fSrcTitlePopUp indexOfSelectedItem] );

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
	AutoCropTop = title->crop[0];
	AutoCropBottom = title->crop[1];
	AutoCropLeft = title->crop[2];
	AutoCropRight = title->crop[3];

	/* Update the others views */
	[[NSNotificationCenter defaultCenter] postNotification:
	 [NSNotification notificationWithName: HBTitleChangedNotification
								   object: self
								 userInfo: [NSDictionary dictionaryWithObjectsAndKeys:
											[NSData dataWithBytesNoCopy: &fTitle length: sizeof(fTitle) freeWhenDone: NO], keyTitleTag,
											nil]]];

    /* If Auto Naming is on. We create an output filename of dvd name - title number */
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
	{
        [self updateFileName];
	}

   /* lets call tableViewSelected to make sure that any preset we have selected is enforced after a title change */
    [self applyPreset];
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
        hb_list_item( list, (int)[fSrcTitlePopUp indexOfSelectedItem] );

    hb_chapter_t * chapter;
    int64_t        duration = 0;
    for( NSInteger i = [fSrcChapterStartPopUp indexOfSelectedItem];
         i <= [fSrcChapterEndPopUp indexOfSelectedItem]; i++ )
    {
        chapter = (hb_chapter_t *) hb_list_item( title->list_chapter, (int)i );
        duration += chapter->duration;
    }
    
    duration /= 90000; /* pts -> seconds */
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
        @"%02lld:%02lld:%02lld", duration / 3600, ( duration / 60 ) % 60,
        duration % 60]];

    /* We're changing the chapter range - we may need to flip the m4v/mp4 extension */
    if ([[fDstFormatPopUp selectedItem] tag] & HB_MUX_MASK_MP4)
    {
        [self autoSetM4vExtension:sender];
    }

    /* If Auto Naming is on it might need to be update if it includes the chapters range */
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
	{
        [self updateFileName];
	}
}

- (IBAction) startEndSecValueChanged: (id) sender
{

	int duration = [fSrcTimeEndEncodingField intValue] - [fSrcTimeStartEncodingField intValue];
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
        @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60,
        duration % 60]];
}

- (IBAction) startEndFrameValueChanged: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
    hb_list_item( list, (int)[fSrcTitlePopUp indexOfSelectedItem] );
    
    int duration = ([fSrcFrameEndEncodingField intValue] - [fSrcFrameStartEncodingField intValue]) / (title->rate / title->rate_base);
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
                                         @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60,
                                         duration % 60]];
}

- (IBAction) formatPopUpChanged: (id) sender
{
    NSString *string   = [fDstFile2Field stringValue];
    int videoContainer = (int)[[fDstFormatPopUp selectedItem] tag];
    const char *ext    = NULL;

    /* Initially set the large file (64 bit formatting) output checkbox to hidden */
    [fDstMp4LargeFileCheck   setHidden:YES];
    [fDstMp4HttpOptFileCheck setHidden:YES];
    [fDstMp4iPodFileCheck    setHidden:YES];

    // enable chapter markers and hide muxer-specific options
    [fDstMp4LargeFileCheck   setHidden:YES];
    [fDstMp4HttpOptFileCheck setHidden:YES];
    [fDstMp4iPodFileCheck    setHidden:YES];
    switch (videoContainer)
    {
        case HB_MUX_MP4V2:
            [fDstMp4LargeFileCheck   setHidden:NO];
        case HB_MUX_AV_MP4:
            [fDstMp4HttpOptFileCheck setHidden:NO];
            [fDstMp4iPodFileCheck    setHidden:NO];
            break;

        default:
            break;
    }
    // set the file extension
    ext = hb_container_get_default_extension(videoContainer);
    [fDstFile2Field setStringValue:[NSString stringWithFormat:@"%@.%s",
                                    [string stringByDeletingPathExtension],
                                    ext]];
    if (videoContainer & HB_MUX_MASK_MP4)
    {
        [self autoSetM4vExtension:sender];
    }

    /* post a notification for any interested observers to indicate that our video container has changed */
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:HBContainerChangedNotification
                                   object:self
                                 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
                                           [NSNumber numberWithInt:videoContainer], keyContainerTag,
                                           nil]]];

	[self customSettingUsed:sender];
}

- (void) autoSetM4vExtension:(NSNotification *)notification
{
    if (!([[fDstFormatPopUp selectedItem] tag] & HB_MUX_MASK_MP4))
        return;
    
    NSString * extension = @"mp4";
    
    BOOL anyCodecAC3 = [fAudioController anyCodecMatches: HB_ACODEC_AC3] || [fAudioController anyCodecMatches: HB_ACODEC_AC3_PASS];
    /* Chapter markers are enabled if the checkbox is ticked and we are doing p2p or we have > 1 chapter */
    BOOL chapterMarkers = (fChapterTitlesController.createChapterMarkers) &&
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

- (void)updateMp4Checkboxes:(NSNotification *)notification
{
    if (fVideoController.selectedCodec != HB_VCODEC_X264)
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
}

/* Method to determine if we should change the UI
To reflect whether or not a Preset is being used or if
the user is using "Custom" settings by determining the sender*/
- (IBAction) customSettingUsed: (id) sender
{
	if ([sender stringValue])
	{
		/* Deselect the currently selected Preset if there is one*/
        [fPresetsView deselect];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
	}

    /* If Auto Naming is on it might need to be update if it includes the quality token */
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
	{
        [self updateFileName];
	}
}

#pragma mark -
#pragma mark - Picture

/**
 * Registers changes made in the Picture Settings Window.
 */

- (void)pictureSettingsDidChange 
{
    // align picture settings and video filters in the UI using tabs
    fVideoController.pictureSettingsField = [self pictureSettingsSummary];
    fVideoController.pictureFiltersField = [self pictureFiltersSummary];

    /* Store storage resolution for unparse */
    if (fTitle)
    {
        fVideoController.fX264PresetsWidthForUnparse  = fTitle->job->width;
        fVideoController.fX264PresetsHeightForUnparse = fTitle->job->height;
        // width or height may have changed, unparse
        [fVideoController x264PresetsChangedDisplayExpandedOptions:nil];
    }
}

#pragma mark -
#pragma mark - Text Summaries

- (NSString*) pictureSettingsSummary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    if (fPictureController && fTitle && fTitle->job)
    {
        [summary appendString:[fPictureController pictureSizeInfoString]];
        if (fTitle->job->anamorphic.mode != HB_ANAMORPHIC_STRICT)
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
            [summary appendFormat:@" - Deblock (%ld)",
             (long)[fPictureController deblock]];
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
    if ([fDstMp4LargeFileCheck  isHidden] == NO  &&
        [fDstMp4LargeFileCheck isEnabled] == YES &&
        [fDstMp4LargeFileCheck     state] == NSOnState)
    {
        [summary appendString:@" - Large file size"];
    }
    if ([fDstMp4HttpOptFileCheck  isHidden] == NO  &&
        [fDstMp4HttpOptFileCheck isEnabled] == YES &&
        [fDstMp4HttpOptFileCheck     state] == NSOnState)
    {
        [summary appendString:@" - Web optimized"];
    }
    if ([fDstMp4iPodFileCheck  isHidden] == NO  &&
        [fDstMp4iPodFileCheck isEnabled] == YES &&
        [fDstMp4iPodFileCheck     state] == NSOnState)
    {
        [summary appendString:@" - iPod 5G support"];
    }
    if ([summary hasPrefix:@" - "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 3)];
    }
    return [NSString stringWithString:summary];
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
    if ([fPresetDrawer state] == NSDrawerClosedState)
    {
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"DefaultPresetsDrawerShow"];
    }
    else
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"DefaultPresetsDrawerShow"];
    }

    [fPresetDrawer toggle:self];
}

/**
 * Shows Picture Settings Window.
 */

- (IBAction) showPicturePanel: (id) sender
{
	[fPictureController showPictureWindow:sender];
}

- (IBAction) showPreviewWindow: (id) sender
{
	[fPictureController showPreviewWindow:sender];
}

#pragma mark - Preset  Methods

- (void)applyPreset
{
    if (fPresetsView.selectedPreset != nil)
    {
        hb_job_t * job = fTitle->job;

        // for mapping names via libhb
        const char *strValue;
        NSDictionary *chosenPreset = [fPresetsView.selectedPreset content];
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
        /* map legacy container names via libhb */
        strValue = hb_container_sanitize_name([[chosenPreset objectForKey:@"FileFormat"] UTF8String]);
        [fDstFormatPopUp selectItemWithTitle:[NSString stringWithFormat:@"%s", strValue]];
        [self formatPopUpChanged:nil];
        
        /* Chapter Markers*/
        fChapterTitlesController.createChapterMarkers = [[chosenPreset objectForKey:@"ChapterMarkers"] boolValue];
        /* check to see if we have only one chapter */
        [self chapterPopUpChanged:nil];
        
        /* Allow Mpeg4 64 bit formatting +4GB file sizes */
        [fDstMp4LargeFileCheck setState:[[chosenPreset objectForKey:@"Mp4LargeFile"] intValue]];
        /* Mux mp4 with http optimization */
        [fDstMp4HttpOptFileCheck setState:[[chosenPreset objectForKey:@"Mp4HttpOptimize"] intValue]];
        
        /* Video encoder */
        [fVideoController applySettingsFromPreset:chosenPreset];

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

        /* Audio */
        [fAudioController applySettingsFromPreset: chosenPreset];
        
        /*Subtitles*/
        [fSubtitlesViewController applySettingsFromPreset:chosenPreset];

        /* Picture Settings */
        /* Note: objectForKey:@"UsesPictureSettings" refers to picture size, which encompasses:
         * height, width, keep ar, anamorphic and crop settings.
         * picture filters are handled separately below.
         */
        int maxWidth = fTitle->width - job->crop[2] - job->crop[3];
        int maxHeight = fTitle->height - job->crop[0] - job->crop[1];
        job->maxWidth = job->maxHeight = 0;
        /* Check to see if the objectForKey:@"UsesPictureSettings is greater than 0, as 0 means use picture sizing "None" 
         * ( 2 is use max for source and 1 is use exact size when the preset was created ) and the 
         * preset completely ignores any picture sizing values in the preset.
         */
        if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] > 0)
        {
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
            
            /* crop may have changed, reset maxWidth/maxHeight */
            maxWidth = fTitle->width - job->crop[2] - job->crop[3];
            maxHeight = fTitle->height - job->crop[0] - job->crop[1];

            /* Set modulus */
            if ([chosenPreset objectForKey:@"PictureModulus"])
            {
                job->modulus = [[chosenPreset objectForKey:@"PictureModulus"]  intValue];
            }
            else
            {
                job->modulus = 16;
            }

            /*
             * Assume max picture settings initially
             */
            job->anamorphic.mode = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
            job->width = fTitle->width - job->crop[2] - job->crop[3];
            job->height = fTitle->height - job->crop[0] - job->crop[1];
            job->anamorphic.keep_display_aspect = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];

            /* Check to see if the objectForKey:@"UsesPictureSettings" is 2,
             * which means "Use max. picture size for source"
             * If not 2 it must be 1 here which means "Use the picture
             * size specified in the preset"
             */
            if ([[chosenPreset objectForKey:@"UsesPictureSettings"]    intValue] != 2 &&
                [[chosenPreset objectForKey:@"UsesMaxPictureSettings"] intValue] != 1)
            {
                /*
                 * if the preset specifies neither max. width nor height
                 * (both are 0), use the max. picture size
                 *
                 * if the specified non-zero dimensions exceed those of the
                 * source, also use the max. picture size (no upscaling)
                 */
                if ([[chosenPreset objectForKey:@"PictureWidth"]  intValue] > 0)
                {
                    job->maxWidth  = [[chosenPreset objectForKey:@"PictureWidth"]  intValue];
                }
                if ([[chosenPreset objectForKey:@"PictureHeight"]  intValue] > 0)
                {
                    job->maxHeight  = [[chosenPreset objectForKey:@"PictureHeight"]  intValue];
                }
            }
        }
        /* Modulus added to maxWidth/maxHeight to allow a small amount of
         * upscaling to the next mod boundary. This does not apply to 
         * explicit limits set for device compatibility.  It only applies
         * when limiting to cropped title dimensions.
         */
        maxWidth += job->modulus - 1;
        maxHeight += job->modulus - 1;
        if (job->maxWidth == 0 || job->maxWidth > maxWidth)
            job->maxWidth = maxWidth;
        if (job->maxHeight == 0 || job->maxHeight > maxHeight)
            job->maxHeight = maxHeight;

        int width, height, par_width, par_height;
        hb_set_anamorphic_size(job, &width, &height, &par_width, &par_height);
        job->width = width;
        job->height = height;
        job->anamorphic.par_width = par_width;
        job->anamorphic.par_height = par_height;

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
    }
    /* we call SetTitle: in fPictureController so we get an instant update in the Picture Settings window */
    [fPictureController setTitle:fTitle];
    [self pictureSettingsDidChange];
}

#pragma mark - Presets View Controller Delegate

- (void)selectionDidChange
{
    [self applyPreset];
}

#pragma mark -
#pragma mark Manage Presets

- (void) checkBuiltInsForUpdates
{
    /* if we have built in presets to update, then do so AlertBuiltInPresetUpdate*/
    if ([presetManager checkBuiltInsForUpdates])
    {
        if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertBuiltInPresetUpdate"] == YES)
        {
            /* Show an alert window that built in presets will be updated */
            /*On Screen Notification*/
            [NSApp requestUserAttention:NSCriticalRequest];
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"HandBrake has determined your built in presets are out of date…"];
            [alert setInformativeText:@"HandBrake will now update your built-in presets."];
            [alert runModal];
            [alert release];
        }
        /* when alert is dismissed, go ahead and update the built in presets */
        [presetManager generateBuiltInPresets];
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
    if (fTitle->job->anamorphic.mode != HB_ANAMORPHIC_STRICT)
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
    [fPresetNewPicSettingsPopUp selectItemWithTag: (1 + (fTitle->job->anamorphic.mode == HB_ANAMORPHIC_STRICT))];
    /* Save the current filters in the preset by default */
    [fPresetNewPicFiltersCheck setState:NSOnState];
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
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Warning!"];
        [alert setInformativeText:@"You need to insert a name for the preset."];
        [alert runModal];
        [alert release];
    }
    else
    {
        /* Here we create a custom user preset */
        [presetManager addPreset:[self createPreset]];

        [self closeAddPresetPanel:nil];
    }
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
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic Size and Cropping (includes Anamorphic)*/
    [preset setObject:[NSNumber numberWithInteger:[[fPresetNewPicSettingsPopUp selectedItem] tag]] forKey:@"UsesPictureSettings"];
    /* Get whether or not to use the current Picture Filter settings for the preset */
    [preset setObject:[NSNumber numberWithInteger:[fPresetNewPicFiltersCheck state]] forKey:@"UsesPictureFilters"];

    /* Get New Preset Description from the field in the AddPresetPanel*/
    [preset setObject:[fPresetNewDesc stringValue] forKey:@"PresetDescription"];
    /* File Format */
    [preset setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
    /* Chapter Markers fCreateChapterMarkers*/
    [preset setObject:@(fChapterTitlesController.createChapterMarkers) forKey:@"ChapterMarkers"];
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
    [preset setObject:[NSNumber numberWithInteger:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
    /* Mux mp4 with http optimization */
    [preset setObject:[NSNumber numberWithInteger:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
    /* Add iPod uuid atom */
    [preset setObject:[NSNumber numberWithInteger:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
    
    /* Codecs */
    /* Video encoder */
    [fVideoController prepareVideoForPreset:preset];

    /*Picture Settings*/
    hb_job_t * job = fTitle->job;
    
    /* Picture Sizing */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:[fPresetNewPicWidth intValue]] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:[fPresetNewPicHeight intValue]] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.keep_display_aspect] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:fTitle->job->modulus] forKey:@"PictureModulus"];

    /* Set crop settings here */
    [preset setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    /* Picture Filters */
    [preset setObject:[NSNumber numberWithInteger:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
    [preset setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
    [preset setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController denoise]] forKey:@"PictureDenoise"];
    [preset setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController deblock]] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController decomb]] forKey:@"PictureDecomb"];
    [preset setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInteger:[fPictureController grayscale]] forKey:@"VideoGrayScale"];

    /* Audio */
    [fAudioController.settings prepareAudioForPreset:preset];

    /* Subtitles */
    [fSubtitlesViewController prepareSubtitlesForPreset:preset];

    [preset autorelease];
    return preset;
    
}

#pragma mark -
#pragma mark Import Export Preset(s)

- (IBAction) browseExportPresetFile: (id) sender
{
    /* Open a panel to let the user choose where and how to save the export file */
    NSSavePanel *panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
    NSString *defaultExportDirectory = [NSString stringWithFormat: @"%@/Desktop/", NSHomeDirectory()];
    [panel setDirectoryURL:[NSURL fileURLWithPath:defaultExportDirectory]];
    [panel setNameFieldStringValue:@"HB_Export.plist"];
    [panel beginSheetModalForWindow:fWindow completionHandler:^(NSInteger result) {
        if( result == NSOKButton )
        {
            NSURL *exportPresetsFile = [panel URL];
            NSURL *presetExportDirectory = [exportPresetsFile URLByDeletingLastPathComponent];
            [[NSUserDefaults standardUserDefaults] setURL:presetExportDirectory forKey:@"LastPresetExportDirectoryURL"];

            /* We check for the presets.plist */
            if ([[NSFileManager defaultManager] fileExistsAtPath:[exportPresetsFile path]] == 0)
            {
                [[NSFileManager defaultManager] createFileAtPath:[exportPresetsFile path] contents:nil attributes:nil];
            }

            NSMutableArray *presetsToExport = [[[NSMutableArray alloc] initWithContentsOfURL:exportPresetsFile] autorelease];
            if (presetsToExport == nil)
            {
                presetsToExport = [[NSMutableArray alloc] init];
                /* now get and add selected presets to export */
            }
            if (fPresetsView.selectedPreset != nil)
            {
                [presetsToExport addObject:[fPresetsView.selectedPreset content]];
                [presetsToExport writeToURL:exportPresetsFile atomically:YES];
            }
        }
    }];
}

- (IBAction)browseImportPresetFile:(id)sender
{
    NSOpenPanel *panel;

    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowedFileTypes:@[@"plist", @"xml"]];

    NSURL *sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastPresetImportDirectoryURL"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastPresetImportDirectoryURL"];
	}
	else
	{
		sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    /* set this for allowed file types, not sure if we should allow xml or not */
    [panel setDirectoryURL:sourceDirectory];
    [panel beginSheetModalForWindow:fWindow completionHandler:^(NSInteger result)
    {
        NSURL *importPresetsFile = [panel URL];
        NSURL *importPresetsDirectory = nil;//[importPresetsFile URLByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setURL:importPresetsDirectory forKey:@"LastPresetImportDirectoryURL"];

        /* NOTE: here we need to do some sanity checking to verify we do not hose up our presets file   */
        NSMutableArray * presetsToImport = [[NSMutableArray alloc] initWithContentsOfURL:importPresetsFile];
        /* iterate though the new array of presets to import and add them to our presets array */
        for (id tempObject in presetsToImport)
        {
            /* make any changes to the incoming preset we see fit */
            /* make sure the incoming preset is not tagged as default */
            [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
            /* prepend "(imported) to the name of the incoming preset for clarification since it can be changed */
            NSString *prependedName = [@"(import) " stringByAppendingString:[tempObject objectForKey:@"PresetName"]] ;
            [tempObject setObject:prependedName forKey:@"PresetName"];
            
            /* actually add the new preset to our presets array */
            [presetManager addPreset:tempObject];
        }
        [presetsToImport autorelease];
    }];
}

#pragma mark -
#pragma mark Preset Menu

- (IBAction)selectDefaultPreset:(id)sender
{
	[fPresetsView selectPreset:presetManager.defaultPreset];
}

- (IBAction)insertFolder:(id)sender
{
    [fPresetsView insertFolder:sender];
}

- (IBAction)selectPresetFromMenu:(id)sender
{
    __block HBPreset *preset = nil;
    __block NSInteger i = -1;

    NSInteger tag = [sender tag];

    [presetManager.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
    {
        if (i == tag)
        {
            preset = obj;
            *stop = YES;
        }
        i++;
    }];

    [fPresetsView selectPreset:preset];
}

/**
 *  Adds the presets list to the menu.
 */
- (void)buildPresetsMenu
{
    // First we remove all the preset menu items
    // inserted previosly
    NSArray *menuItems = [presetsMenu.itemArray copy];
    for (NSMenuItem *item in menuItems)
    {
        if (item.tag != -1)
        {
            [presetsMenu removeItem:item];
        }
    }
    [menuItems release];

    __block NSUInteger i = 0;
    __block BOOL builtInEnded = NO;
    [presetManager.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
    {
        if (idx.length)
        {
            NSMenuItem *item = [[NSMenuItem alloc] init];
            item.title = [obj name];
            item.tag = i++;

            // Set an action only to the actual presets,
            // not on the folders.
            if ([obj isLeaf])
            {
                item.action = @selector(selectPresetFromMenu:);
            }
            // Make the default preset font bold.
            if ([obj isDefault])
            {
                NSAttributedString *newTitle = [[NSAttributedString alloc] initWithString:[obj name]
                                                                               attributes:@{NSFontAttributeName: [NSFont boldSystemFontOfSize:14]}];
                [item setAttributedTitle:newTitle];
                [newTitle release];
            }
            // Add a separator line after the last builtIn preset
            if ([obj isBuiltIn] == NO && builtInEnded == NO)
            {
                [presetsMenu addItem:[NSMenuItem separatorItem]];
                builtInEnded = YES;
            }

            item.indentationLevel = idx.length - 1;

            [presetsMenu addItem:item];
            [item release];
        }
    }];
}

/**
 * We use this method to recreate new, updated factory presets
 */
- (IBAction)addFactoryPresets:(id)sender
{
    [presetManager generateBuiltInPresets];
}

@end
