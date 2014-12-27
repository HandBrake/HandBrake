/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "Controller.h"

#import "HBQueueController.h"

#import "HBOutputPanelController.h"
#import "HBPreferencesController.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"
#import "HBDockTile.h"
#import "HBUtilities.h"

#import "HBVideoController.h"
#import "HBAudioController.h"
#import "HBSubtitlesController.h"
#import "HBAdvancedController.h"
#import "HBChapterTitlesController.h"

#import "HBPictureController.h"
#import "HBPreviewController.h"

#import "HBPresetsViewController.h"
#import "HBAddPresetController.h"

#import "HBPicture+UIAdditions.h"
#import "HBFilters+UIAdditions.h"

#import "HBCore.h"
#import "HBJob.h"

// DockTile update freqency in total percent increment
#define dockTileUpdateFrequency                  0.1f

@interface HBController () <HBPresetsViewControllerDelegate, HBPreviewControllerDelegate, HBPictureControllerDelegate>

@property (nonatomic, copy) NSString *browsedSourceDisplayName;

// The current job.
@property (nonatomic, retain) HBJob *job;

// The job to be applied from the queue.
@property (nonatomic, retain) HBJob *jobFromQueue;

// The current selected preset.
@property (nonatomic, retain) HBPreset *selectedPreset;
@property (nonatomic) BOOL customPreset;

/**
 *  The HBCore used for scanning.
 */
@property (nonatomic, retain) HBCore *core;

/**
 *  The HBCore used for encoding.
 */
@property (nonatomic, retain) HBCore *queueCore;

@end

/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Register the defaults preferences
        [HBPreferencesController registerUserDefaults];

        /* Check for and create the App Support Preview directory if necessary */
        NSString *previewDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Previews"];
        if (![[NSFileManager defaultManager] fileExistsAtPath:previewDirectory])
        {
            [[NSFileManager defaultManager] createDirectoryAtPath:previewDirectory
                                      withIntermediateDirectories:YES
                                                       attributes:nil
                                                            error:NULL];
        }

        // Inits the controllers
        outputPanel = [[HBOutputPanelController alloc] init];
        fPictureController = [[HBPictureController alloc] init];
        fPreviewController = [[HBPreviewController  alloc] initWithDelegate:self];
        fQueueController = [[HBQueueController alloc] init];

        // we init the HBPresetsManager class
        NSURL *presetsURL = [NSURL fileURLWithPath:[[HBUtilities appSupportPath] stringByAppendingPathComponent:@"UserPresets.plist"]];
        presetManager = [[HBPresetsManager alloc] initWithURL:presetsURL];
        _selectedPreset = [presetManager.defaultPreset retain];

        // Workaround to avoid a bug in Snow Leopard
        // we can switch back to [[NSApplication sharedApplication] applicationIconImage]
        // when we won't support it anymore.
        NSImage *appIcon = [NSImage imageNamed:@"HandBrake"];
        [appIcon setSize:NSMakeSize(1024, 1024)];

        // Load the dockTile and instiante initial text fields
        dockTile = [[HBDockTile alloc] initWithDockTile:[[NSApplication sharedApplication] dockTile]
                                                  image:appIcon];

        [dockTile updateDockIcon:-1.0 withETA:@""];

        // Lets report the HandBrake version number here to the activity log and text log file
        NSString *versionStringFull = [[NSString stringWithFormat:@"Handbrake Version: %@",
                                        [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"]]
                                       stringByAppendingString:[NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
        [HBUtilities writeToActivityLog: "%s", [versionStringFull UTF8String]];

        // Optional dvd nav UseDvdNav
        [HBCore setDVDNav:[[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue]];

        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
        // Init libhb
        _core = [[HBCore alloc] initWithLoggingLevel:loggingLevel];
        _core.name = @"ScanCore";

        // Init a separate instance of libhb for user scanning and setting up jobs
        _queueCore = [[HBCore alloc] initWithLoggingLevel:loggingLevel];
        _queueCore.name = @"QueueCore";

        // Registers the observers to the cores notifications.
        [self registerScanCoreNotifications];
        [self registerQueueCoreNotifications];

        // Set the Growl Delegate
        [GrowlApplicationBridge setGrowlDelegate: self];

        [fPictureController setDelegate:self];

        [fPreviewController setCore:self.core];

        [fQueueController setCore:self.queueCore];
        [fQueueController setHBController:self];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(autoSetM4vExtension:) name:HBMixdownChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pictureSettingsDidChange) name:HBPictureChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pictureSettingsDidChange) name:HBFiltersChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(formatChanged:) name:HBContainerChangedNotification object:nil];
    }

    return self;
}

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    // Checks for presets updates
    [self checkBuiltInsForUpdates];

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

    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showDebugOutputPanel:nil];

    // Open queue window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
        [self showQueueWindow:nil];

	[self openMainWindow:nil];

    /* Now we re-check the queue array to see if there are
     * any remaining encodes to be done in it and ask the
     * user if they want to reload the queue */
    if ([QueueFileArray count] > 0)
	{
        /* run  getQueueStats to see whats in the queue file */
        [self getQueueStats];
        /* this results in these values
         * fPendingCount = 0;
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
            }
            else
            {
                /* Since we addressed any pending or previously encoding items above, we go ahead and make sure
                 * the queue is empty of any finished items or cancelled items */
                [self clearQueueAllItems];

                if (self.core.state != HBStateScanning && !self.job)
                {
                    // We show whichever open source window specified in LaunchSourceBehavior preference key
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
    else
    {
        if (self.core.state != HBStateScanning && !self.job)
        {
            // We show whichever open source window specified in LaunchSourceBehavior preference key
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
    currentQueueEncodeNameString = @"";
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    [self openFile:filenames.firstObject];
    [NSApp replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (void)openFile:(NSString *)filePath
{
    if (self.core.state != HBStateScanning)
    {
        self.browsedSourceDisplayName = filePath.lastPathComponent;
        [self performScan:filePath scanTitleNum:0];
    }
}

- (void)setJob:(HBJob *)job
{
    // Set the jobs info to the view controllers
    fPictureController.picture = job.picture;
    fPictureController.filters = job.filters;
    fPreviewController.job = job;

    fVideoController.video = job.video;
    fAudioController.job = job;
    fSubtitlesViewController.job = job;
    fChapterTitlesController.job = job;

    // Retain the new job
    [_job autorelease];
    _job = [job retain];
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
    NSPasteboard *pboard = [sender draggingPasteboard];

    if ([[pboard types] containsObject:NSFilenamesPboardType])
    {
        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        [self openFile:paths.firstObject];
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
    if (self.queueCore.state != HBStateIdle)
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
    [presetManager savePresets];
    [presetManager release];

    [self closeQueueFSEvent];
    [currentQueueEncodeNameString release];
    [outputPanel release];
	[fQueueController release];
    [fPreviewController release];
    [fPictureController release];
    [dockTile release];

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    self.core = nil;
    self.queueCore = nil;
    self.browsedSourceDisplayName = nil;
    [HBCore closeGlobal];
}


- (void) awakeFromNib
{    
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

    // Presets initialization
    [self buildPresetsMenu];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(buildPresetsMenu) name:HBPresetsChangedNotification object:nil];

    [fPresetDrawer setDelegate:self];
    NSSize drawerSize = NSSizeFromString([[NSUserDefaults standardUserDefaults]
                                           stringForKey:@"HBDrawerSize"]);
    if (drawerSize.width)
    {
        [fPresetDrawer setContentSize: drawerSize];
    }

	/* Show/Dont Show Presets drawer upon launch based
     on user preference DefaultPresetsDrawerShow */
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBDefaultPresetsDrawerShow"])
	{
		[fPresetDrawer open:self];
	}

    /* Setup the start / stop popup */
    [fEncodeStartStopPopUp removeAllItems];
    [fEncodeStartStopPopUp addItemWithTitle: @"Chapters"];
    [fEncodeStartStopPopUp addItemWithTitle: @"Seconds"];
    [fEncodeStartStopPopUp addItemWithTitle: @"Frames"];

    // Align the start / stop widgets with the chapter popups
    NSPoint startPoint = [fSrcChapterStartPopUp frame].origin;
    startPoint.y += 2;

    NSPoint endPoint = [fSrcChapterEndPopUp frame].origin;
    endPoint.y += 2;

    [fSrcTimeStartEncodingField setFrameOrigin:startPoint];
    [fSrcTimeEndEncodingField setFrameOrigin:endPoint];
    
    [fSrcFrameStartEncodingField setFrameOrigin:startPoint];
    [fSrcFrameEndEncodingField setFrameOrigin:endPoint];
    
    /* Destination box*/
    NSMenuItem *menuItem;
    [fDstFormatPopUp removeAllItems];
    for (const hb_container_t *container = hb_container_get_next(NULL);
         container != NULL;
         container  = hb_container_get_next(container))
    {
        NSString *title = nil;
        if (container->format & HB_MUX_MASK_MP4)
        {
            title = @"MP4 File";
        }
        else if (container->format & HB_MUX_MASK_MKV)
        {
            title = @"MKV File";
        }
        else
        {
            title = [NSString stringWithUTF8String:container->name];
        }
        menuItem = [[fDstFormatPopUp menu] addItemWithTitle:title
                                                     action:nil
                                              keyEquivalent:@""];
        [menuItem setTag:container->format];
    }

    /* Bottom */
    [fStatusField setStringValue: @""];

    /* Register HBController's Window as a receiver for files/folders drag & drop operations */
    [fWindow registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

    // Set up the preset drawer
    fPresetsView = [[HBPresetsViewController alloc] initWithPresetManager:presetManager];
    [fPresetDrawer setContentView:[fPresetsView view]];
    fPresetsView.delegate = self;
    [[fPresetDrawer contentView] setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];

    // Set up the chapters title view
    fChapterTitlesController = [[HBChapterTitlesController alloc] init];
    [fChaptersTitlesTab setView:[fChapterTitlesController view]];

    // setup the subtitles view
    fSubtitlesViewController = [[HBSubtitlesController alloc] init];
	[fSubtitlesTab setView:[fSubtitlesViewController view]];

	// setup the audio controller
    fAudioController = [[HBAudioController alloc] init];
	[fAudioTab setView:[fAudioController view]];

    // setup the advanced view controller
    fAdvancedOptions = [[HBAdvancedController alloc] init];
	[fAdvancedTab setView:[fAdvancedOptions view]];

    // setup the video view controller
    fVideoController = [[HBVideoController alloc] initWithAdvancedController:fAdvancedOptions];
	[fVideoTab setView:[fVideoController view]];

    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
                                                              forKeyPath:@"values.HBShowAdvancedTab"
                                                                 options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                                                                 context:NULL];

    [fWindow recalculateKeyViewLoop];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == NULL)
    {
        if ([keyPath isEqualToString:@"values.HBShowAdvancedTab"])
        {
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBShowAdvancedTab"])
            {
                if (![[fMainTabView tabViewItems] containsObject:fAdvancedTab])
                {
                    [fMainTabView insertTabViewItem:fAdvancedTab atIndex:3];
                    [fAdvancedTab release];
                }
            }
            else
            {
                [fAdvancedTab retain];
                [fMainTabView removeTabViewItem:fAdvancedTab];
            }
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void) enableUI: (BOOL) b
{
    NSControl * controls[] =
    {
        fSrcTitleField, fSrcTitlePopUp,
        fSrcChapterStartPopUp, fSrcChapterToField,
        fSrcChapterEndPopUp, fSrcDuration1Field, fSrcDuration2Field,
        fDstFormatField, fDstFormatPopUp, fDstFile1Field, fDstFile2Field,
        fDstBrowseButton, fSrcAngleLabel, fSrcAnglePopUp,
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

    fPresetsView.enabled = b;
}

/**
 *  Registers the observers to the scan core notifications
 */
- (void)registerScanCoreNotifications
{
    NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreScanningNotification object:self.core queue:mainQueue usingBlock:^(NSNotification *note) {
        hb_state_t s = *(self.core.hb_state);
        #define p s.param.scanning
        if (p.preview_cur)
        {
            fSrcDVD2Field.stringValue = [NSString stringWithFormat:
                                         NSLocalizedString( @"Scanning title %d of %d, preview %d…", @"" ),
                                         p.title_cur, p.title_count,
                                         p.preview_cur];
        }
        else
        {
            fSrcDVD2Field.stringValue = [NSString stringWithFormat:
                                          NSLocalizedString( @"Scanning title %d of %d…", @"" ),
                                          p.title_cur, p.title_count];
        }
        fScanIndicator.hidden = NO;
        fScanHorizontalLine.hidden = YES;
        fScanIndicator.doubleValue = 100.0 * p.progress;
        #undef p
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreScanDoneNotification object:self.core queue:mainQueue usingBlock:^(NSNotification *note) {
        fScanHorizontalLine.hidden = NO;
        fScanIndicator.hidden = YES;
        fScanIndicator.indeterminate = NO;
        fScanIndicator.doubleValue = 0.0;

        [HBUtilities writeToActivityLog:"ScanDone state received from fHandle"];
        [self showNewScan];
        [[fWindow toolbar] validateVisibleItems];
    }];
}

- (void)registerQueueCoreNotifications
{
    NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreScanningNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        hb_state_t s = *(self.queueCore.hb_state);
        #define p s.param.scanning
        NSString *scan_status;
        if (p.preview_cur)
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
        fStatusField.stringValue = scan_status;

        // Set the status string in fQueueController as well
        [fQueueController setQueueStatusString: scan_status];
        #undef p
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreScanDoneNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        [HBUtilities writeToActivityLog:"ScanDone state received from fQueueEncodeLibhb"];
        [self processNewQueueEncode];
        [[fWindow toolbar] validateVisibleItems];
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreSearchingNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        hb_state_t s = *(self.queueCore.hb_state);
        #define p s.param.working
        NSMutableString *string = [NSMutableString stringWithFormat:
                                   NSLocalizedString(@"Searching for start point… :  %.2f %%", @""),
                                   100.0 * p.progress];

        if (p.seconds > -1)
        {
            [string appendFormat:NSLocalizedString(@" (ETA %02dh%02dm%02ds)", @"" ), p.hours, p.minutes, p.seconds];
        }

        fStatusField.stringValue = string;
        // Set the status string in fQueueController as well
        [fQueueController setQueueStatusString: string];

        // Update slider
        CGFloat progress_total = (p.progress + p.job_cur - 1) / p.job_count;
        fRipIndicator.indeterminate = NO;
        fRipIndicator.doubleValue = 100.0 * progress_total;

        // If progress bar hasn't been revealed at the bottom of the window, do
        // that now. This code used to be in doRip. I moved it to here to handle
        // the case where hb_start is called by HBQueueController and not from
        // HBController.
        if (!fRipIndicatorShown)
        {
            NSRect frame = [fWindow frame];
            if (frame.size.width <= 591)
                frame.size.width = 591;
            frame.size.height += 36;
            frame.origin.y -= 36;
            [fWindow setFrame:frame display:YES animate:YES];
            fRipIndicatorShown = YES;
        }
        #undef p
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreWorkingNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        hb_state_t s = *(self.queueCore.hb_state);
        #define p s.param.working
        // Update text field
        NSString *pass_desc;
        if (p.job_cur == 1 && p.job_count > 1)
        {
            HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
            if (queueJob.subtitlesTracks.count &&
                [[queueJob.subtitlesTracks firstObject][keySubTrackIndex] intValue] == -1)
            {
                pass_desc = NSLocalizedString(@"(subtitle scan)", @"");
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

        NSMutableString *string;
        if (pass_desc.length)
        {
            string = [NSMutableString stringWithFormat:
                      NSLocalizedString(@"Encoding: %@ \nPass %d %@ of %d, %.2f %%", @""),
                      currentQueueEncodeNameString,
                      p.job_cur, pass_desc, p.job_count, 100.0 * p.progress];
        }
        else
        {
            string = [NSMutableString stringWithFormat:
                      NSLocalizedString(@"Encoding: %@ \nPass %d of %d, %.2f %%", @""),
                      currentQueueEncodeNameString,
                      p.job_cur, p.job_count, 100.0 * p.progress];
        }

        if (p.seconds > -1)
        {
            if (p.rate_cur > 0.0)
            {
                [string appendFormat:
                 NSLocalizedString(@" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", @""),
                 p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
            }
            else
            {
                [string appendFormat:
                 NSLocalizedString(@" (ETA %02dh%02dm%02ds)", @""),
                 p.hours, p.minutes, p.seconds];
            }
        }

        fStatusField.stringValue = string;
        [fQueueController setQueueStatusString:string];

        // Update slider
        CGFloat progress_total = (p.progress + p.job_cur - 1) / p.job_count;
        fRipIndicator.indeterminate = NO;
        fRipIndicator.doubleValue = 100.0 * progress_total;

        // If progress bar hasn't been revealed at the bottom of the window, do
        // that now. This code used to be in doRip. I moved it to here to handle
        // the case where hb_start is called by HBQueueController and not from
        // HBController.
        if (!fRipIndicatorShown)
        {
            NSRect frame = [fWindow frame];
            if (frame.size.width <= 591)
                frame.size.width = 591;
            frame.size.height += 36;
            frame.origin.y -= 36;
            [fWindow setFrame:frame display:YES animate:YES];
            fRipIndicatorShown = YES;
        }

        /* Update dock icon */
        if (dockIconProgress < 100.0 * progress_total)
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

            [dockTile updateDockIcon:progress_total withETA:etaStr];

            dockIconProgress += dockTileUpdateFrequency;
        }
        #undef p
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreMuxingNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        // Update text field
        fStatusField.stringValue = NSLocalizedString(@"Muxing…", @"");
        // Set the status string in fQueueController as well
        [fQueueController setQueueStatusString:NSLocalizedString(@"Muxing…", @"")];
        // Update slider
        fRipIndicator.indeterminate = YES;
        [fRipIndicator startAnimation: nil];

        // Update dock icon
        [dockTile updateDockIcon:1.0 withETA:@""];
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCorePausedNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        NSString *paused = NSLocalizedString(@"Paused", @"");
        fStatusField.stringValue = paused;
        [fQueueController setQueueStatusString:paused];
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:HBCoreWorkDoneNotification object:self.queueCore queue:mainQueue usingBlock:^(NSNotification *note) {
        fStatusField.stringValue = NSLocalizedString(@"Encode Finished.", @"");
        // Set the status string in fQueueController as well
        [fQueueController setQueueStatusString:NSLocalizedString(@"Encode Finished.", @"")];
        fRipIndicator.indeterminate = NO;
        fRipIndicator.doubleValue = 0.0;
        [fRipIndicator setIndeterminate: NO];

        [[fWindow toolbar] validateVisibleItems];

        // Restore dock icon
        [dockTile updateDockIcon:-1.0 withETA:@""];
        dockIconProgress = 0;

        if (fRipIndicatorShown)
        {
            NSRect frame = [fWindow frame];
            if( frame.size.width <= 591 )
                frame.size.width = 591;
            frame.size.height += -36;
            frame.origin.y -= -36;
            [fWindow setFrame:frame display:YES animate:YES];
            fRipIndicatorShown = NO;
        }
        // Since we are done with this encode, tell output to stop writing to the
        // individual encode log.
        [outputPanel endEncodeLog];

        // Check to see if the encode state has not been cancelled
        // to determine if we should check for encode done notifications.
        if (fEncodeState != 2)
        {
            // Get the output file name for the finished encode
            HBJob *finishedJob = QueueFileArray[currentQueueEncodeIndex];

            // Both the Growl Alert and Sending to tagger can be done as encodes roll off the queue
            if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Growl Notification"] ||
                [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
            {
                // If Play System Alert has been selected in Preferences
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"AlertWhenDoneSound"] == YES)
                {
                    NSBeep();
                }
                [self showGrowlDoneNotification:finishedJob.destURL];
            }

            // Send to tagger
            [self sendToExternalApp:finishedJob.destURL];

            // since we have successfully completed an encode, we increment the queue counter
            [self incrementQueueItemDone:currentQueueEncodeIndex];
        }
    }];
}

#pragma mark -
#pragma mark Toolbar

- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    SEL action = toolbarItem.action;
        
    if (self.core)
    {
        if (self.core.state == HBStateScanning)
        {
            
            if (action == @selector(browseSources:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
                [toolbarItem setLabel: @"Cancel Scan"];
                [toolbarItem setPaletteLabel: @"Cancel Scanning"];
                [toolbarItem setToolTip: @"Cancel Scanning Source"];
                return YES;
            }
            
            if (action == @selector(Rip:) || action == @selector(addToQueue:))
                return NO;
        }
        else
        {
            if (action == @selector(browseSources:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"source"]];
                [toolbarItem setLabel: @"Source"];
                [toolbarItem setPaletteLabel: @"Source"];
                [toolbarItem setToolTip: @"Choose Video Source"];
                return YES;
            }
        }

        HBState queueState = self.queueCore.state;

        if (queueState == HBStateWorking || queueState == HBStateSearching || queueState == HBStateMuxing)
        {
            if (action == @selector(Rip:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
                [toolbarItem setLabel: @"Stop"];
                [toolbarItem setPaletteLabel: @"Stop"];
                [toolbarItem setToolTip: @"Stop Encoding"];
                return YES;
            }
            if (action == @selector(Pause:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"pauseencode"]];
                [toolbarItem setLabel: @"Pause"];
                [toolbarItem setPaletteLabel: @"Pause Encoding"];
                [toolbarItem setToolTip: @"Pause Encoding"];
                return YES;
            }
            if (self.job)
            {
                if (action == @selector(addToQueue:))
                    return YES;
                if (action == @selector(showPicturePanel:))
                    return YES;
                if (action == @selector(showPreviewWindow:))
                    return YES;
            }
        }
        else if (queueState == HBStatePaused)
        {
            if (action == @selector(Pause:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
                [toolbarItem setLabel: @"Resume"];
                [toolbarItem setPaletteLabel: @"Resume Encoding"];
                [toolbarItem setToolTip: @"Resume Encoding"];
                return YES;
            }
            if (action == @selector(Rip:))
                return YES;
            if (action == @selector(addToQueue:))
                return YES;
            if (action == @selector(showPicturePanel:))
                return YES;
            if (action == @selector(showPreviewWindow:))
                return YES;
        }
        else if (queueState == HBStateScanning)
        {
            return NO;
        }
        else if (queueState == HBStateWorkDone || queueState == HBStateScanDone || self.job)
        {
            if (action == @selector(Rip:))
            {
                [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
                if (QueueFileArray.count > 0)
                    [toolbarItem setLabel: @"Start Queue"];
                else
                    [toolbarItem setLabel: @"Start"];
                [toolbarItem setPaletteLabel: @"Start Encoding"];
                [toolbarItem setToolTip: @"Start Encoding"];
                return YES;
            }
            if (action == @selector(addToQueue:))
                return YES;
            if (action == @selector(showPicturePanel:))
                return YES;
            if (action == @selector(showPreviewWindow:))
                return YES;
        }

    }
    /* If there are any pending queue items, make sure the start/stop button is active */
    if (action == @selector(Rip:) && fPendingCount > 0)
        return YES;
    if (action == @selector(showQueueWindow:))
        return YES;
    if (action == @selector(toggleDrawer:))
        return YES;
    if (action == @selector(browseSources:))
        return YES;
    if (action == @selector(showDebugOutputPanel:))
        return YES;
    
    return NO;
}

- (BOOL) validateMenuItem: (NSMenuItem *) menuItem
{
    SEL action = [menuItem action];

    if (self.queueCore)
    {
        HBState queueState = self.queueCore.state;

        if (action == @selector(addToQueue:) || action == @selector(addAllTitlesToQueue:) || action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:))
            return self.job && [fWindow attachedSheet] == nil;
        
        if (action == @selector(selectDefaultPreset:))
            return [fWindow attachedSheet] == nil;
        if (action == @selector(Pause:))
        {
            if (queueState == HBStateWorking)
            {
                if(![[menuItem title] isEqualToString:@"Pause Encoding"])
                    [menuItem setTitle:@"Pause Encoding"];
                return YES;
            }
            else if (queueState == HBStatePaused)
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
            if (queueState == HBStateWorking || queueState == HBStateMuxing || queueState == HBStatePaused)
            {
                if(![[menuItem title] isEqualToString:@"Stop Encoding"])
                    [menuItem setTitle:@"Stop Encoding"];
                return YES;
            }
            else if (self.job)
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
            if (self.core.state == HBStateScanning)
                return NO;
            else
                return [fWindow attachedSheet] == nil;
        }
        if (action == @selector(selectPresetFromMenu:))
        {
            if (!self.customPreset && [menuItem.representedObject isEqualTo:self.selectedPreset])
            {
                [menuItem setState:NSOnState];
            }
            else
            {
                [menuItem setState:NSOffState];
            }
        }
    }

    return YES;
}

#pragma mark -
#pragma mark Encode Done Actions

#define SERVICE_NAME @"Encode Done"

/**
 *  Register a test notification and make
 *  it enabled by default
 */
- (NSDictionary *)registrationDictionaryForGrowl 
{ 
    return @{GROWL_NOTIFICATIONS_ALL: @[SERVICE_NAME],
             GROWL_NOTIFICATIONS_DEFAULT: @[SERVICE_NAME]};
}

- (void)showGrowlDoneNotification:(NSURL *)fileURL
{
    // This end of encode action is called as each encode rolls off of the queue
    // Setup the Growl stuff
    NSString *growlMssg = [NSString stringWithFormat:@"your HandBrake encode %@ is done!", fileURL.lastPathComponent];
    [GrowlApplicationBridge notifyWithTitle:@"Put down that cocktail…"
                                description:growlMssg
                           notificationName:SERVICE_NAME
                                   iconData:nil
                                   priority:0
                                   isSticky:1
                               clickContext:nil];
}

- (void)sendToExternalApp:(NSURL *)fileURL
{
    // This end of encode action is called as each encode rolls off of the queue
    if([[NSUserDefaults standardUserDefaults] boolForKey: @"sendToMetaX"] == YES)
    {
        NSString *sendToApp = [[NSUserDefaults standardUserDefaults] objectForKey:@"SendCompletedEncodeToApp"];
        if (![sendToApp isEqualToString:@"None"])
        {
            [HBUtilities writeToActivityLog: "trying to send encode to: %s", [sendToApp UTF8String]];
            NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@%@%@", @"tell application \"",sendToApp,@"\" to open (POSIX file \"", fileURL.path, @"\")"]];
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
- (IBAction)browseSources:(id)sender
{
    if (self.core.state == HBStateScanning)
    {
        [self.core cancelScan];
        return;
    }

    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:YES];

    NSURL *sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastSourceDirectoryURL"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastSourceDirectoryURL"];
	}
	else
	{
		sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
	}

    [panel setDirectoryURL:sourceDirectory];

    [panel beginSheetModalForWindow:fWindow completionHandler: ^(NSInteger result)
    {
        if (result == NSOKButton)
        {
            NSURL *scanURL = panel.URL;
            // we set the last searched source directory in the prefs here
            [[NSUserDefaults standardUserDefaults] setURL:scanURL.URLByDeletingLastPathComponent forKey:@"LastSourceDirectoryURL"];

            // we order out sheet, which is the browse window as we need to open
            // the title selection sheet right away
            [panel orderOut:self];

            if (sender == fOpenSourceTitleMMenu || [[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
            {
                // We put the chosen source path in the source display text field for the
                // source title selection sheet in which the user specifies the specific title to be
                // scanned  as well as the short source name in fSrcDsplyNameTitleScan just for display
                // purposes in the title panel

                // Full Path
                [fScanSrcTitlePathField setStringValue:scanURL.path];
                NSString *displayTitlescanSourceName;

                if ([scanURL.lastPathComponent isEqualToString: @"VIDEO_TS"])
                {
                    // If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
                    // we have to use the title->path value so we get the proper name of the volume if a physical dvd is the source
                    displayTitlescanSourceName = scanURL.URLByDeletingLastPathComponent.lastPathComponent;
                }
                else
                {
                    // if not the VIDEO_TS Folder, we can assume the chosen folder is the source name
                    displayTitlescanSourceName = scanURL.lastPathComponent;
                }
                // we set the source display name in the title selection dialogue
                [fSrcDsplyNameTitleScan setStringValue:displayTitlescanSourceName];
                // we set the attempted scans display name for main window to displayTitlescanSourceName
                self.browsedSourceDisplayName = displayTitlescanSourceName;
                // We show the actual sheet where the user specifies the title to be scanned
                // as we are going to do a title specific scan

                [self showSourceTitleScanPanel:nil];
            }
            else
            {
                // We are just doing a standard full source scan, so we specify "0" to libhb
                NSURL *url = panel.URL;

                // We check to see if the chosen file at path is a package
                if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:url.path])
                {
                    [HBUtilities writeToActivityLog: "trying to open a package at: %s", url.path.UTF8String];
                    // We check to see if this is an .eyetv package
                    if ([url.pathExtension isEqualToString: @"eyetv"])
                    {
                        [HBUtilities writeToActivityLog:"trying to open eyetv package"];
                        // We're looking at an EyeTV package - try to open its enclosed .mpg media file
                        self.browsedSourceDisplayName = url.URLByDeletingPathExtension.lastPathComponent;
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
                        self.browsedSourceDisplayName = url.URLByDeletingPathExtension.lastPathComponent;
                        [HBUtilities writeToActivityLog:"trying to open dvdmedia package"];
                        [self performScan:[url path] scanTitleNum:0];
                    }
                    else
                    {
                        /* The package is not an eyetv package, try to open it anyway */
                        self.browsedSourceDisplayName = url.lastPathComponent;
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
                        self.browsedSourceDisplayName = url.URLByDeletingLastPathComponent.lastPathComponent;
                    }
                    else
                    {
                        [HBUtilities writeToActivityLog:"trying to open video_ts folder (parent directory chosen)"];
                        /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                        /* make sure we remove any path extension as this can also be an '.mpg' file */
                        self.browsedSourceDisplayName = url.lastPathComponent;
                    }
                    [self performScan:[url path] scanTitleNum:0];
                }
            }
        }
    }];
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
        // We setup the scan status in the main window to indicate a source title scan
        [fSrcDVD2Field setStringValue: @"Opening a new source title…"];
        [fScanIndicator setHidden: NO];
        [fScanHorizontalLine setHidden: YES];
        [fScanIndicator setIndeterminate: YES];
        [fScanIndicator startAnimation: nil];
		
        // We use the performScan method to actually perform the specified scan passing the path and the title
        // to be scanned
        [self performScan:[fScanSrcTitlePathField stringValue] scanTitleNum:[fScanSrcTitleNumField intValue]];
    }
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void)performScan:(NSString *)scanPath scanTitleNum:(NSInteger)scanTitleNum
{
    // Save the current settings
    if (self.job)
    {
        self.selectedPreset = [self createPresetFromCurrentSettings];
    }

    self.job = nil;
    [self enableUI:NO];

    NSError *outError = NULL;
    NSURL *fileURL = [NSURL fileURLWithPath:scanPath];
    BOOL suppressWarning = [[NSUserDefaults standardUserDefaults] boolForKey:@"suppresslibdvdcss"];

    // Check if we can scan the source and if there is any warning.
    BOOL canScan = [self.core canScan:fileURL error:&outError];

    // Notify the user that we don't support removal of copy proteciton.
    if (canScan && [outError code] == 101 && !suppressWarning)
    {
        // Only show the user this warning once. They may be using a solution we don't know about. Notifying them each time is annoying.
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"suppresslibdvdcss"];

        // Compatible libdvdcss not found
        [HBUtilities writeToActivityLog: "libdvdcss.2.dylib not found for decrypting physical dvd"];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Please note that HandBrake does not support the removal of copy-protection from DVD Discs. You can if you wish install libdvdcss or any other 3rd party software for this function."];
        [alert setInformativeText:@"Videolan.org provides libdvdcss if you are not currently using another solution."];
        [alert addButtonWithTitle:@"Get libdvdcss.pkg"];
        [alert addButtonWithTitle:@"Cancel Scan"];
        [alert addButtonWithTitle:@"Attempt Scan Anyway"];
        [NSApp requestUserAttention:NSCriticalRequest];
        NSInteger status = [alert runModal];
        [alert release];

        if (status == NSAlertFirstButtonReturn)
        {
            /* User chose to go download vlc (as they rightfully should) so we send them to the vlc site */
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://download.videolan.org/libdvdcss/1.2.12/macosx/"]];
            canScan = NO;
        }
        else if (status == NSAlertSecondButtonReturn)
        {
            /* User chose to cancel the scan */
            [HBUtilities writeToActivityLog: "Cannot open physical dvd, scan cancelled"];
            canScan = NO;
        }
        else
        {
            /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
            [HBUtilities writeToActivityLog:"User overrode copy-protection warning - trying to open physical dvd without decryption"];
        }
    }

    if (canScan)
    {
        int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
        int min_title_duration_seconds = [[[NSUserDefaults standardUserDefaults] objectForKey:@"MinTitleScanSeconds"] intValue];

        [self.core scan:fileURL
               titleNum:scanTitleNum
                previewsNum:hb_num_previews minTitleDuration:min_title_duration_seconds];
    }
}

- (void)showNewScan
{
    if (!self.core.titles.count)
    {
        // We display a message if a valid source was not chosen
        fSrcDVD2Field.stringValue = @"No Valid Source Found";
    }
    else
    {
        if (self.jobFromQueue)
        {
            // we are a rescan of an existing queue item and need to apply the queued settings to the scan
            [HBUtilities writeToActivityLog: "showNewScan: This is a queued item rescan"];
        }
        else
        {
            [HBUtilities writeToActivityLog: "showNewScan: This is a new source item scan"];
        }

        [fSrcTitlePopUp removeAllItems];

        for (HBTitle *title in self.core.titles)
        {
            // Set Source Name at top of window with the browsedSourceDisplayName grokked right before -performScan
            if (!self.browsedSourceDisplayName)
            {
                self.browsedSourceDisplayName = @"NoNameDetected";
            }
            fSrcDVD2Field.stringValue = self.browsedSourceDisplayName;

            [fSrcTitlePopUp addItemWithTitle:title.description];

            // See if this is the main feature according
            if (title.isFeatured)
            {
                [fSrcTitlePopUp selectItemWithTitle:title.description];
            }
        }

        // Select the first item is nothing is selected
        if (!fSrcTitlePopUp.selectedItem)
        {
            [fSrcTitlePopUp selectItemAtIndex:0];
        }

        // Updates the main window ui
        [self enableUI:YES];
        [self titlePopUpChanged:nil];

        // Open preview window now if it was visible when HB was closed
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PreviewWindowIsOpen"])
            [self showPreviewWindow:nil];

        // Open picture sizing window now if it was visible when HB was closed
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PictureSizeWindowIsOpen"])
            [self showPicturePanel:nil];

        if (self.jobFromQueue)
        {
            [fPresetsView deselect];
            [self pictureSettingsDidChange];

            self.jobFromQueue = nil;
        }
    }
}

#pragma mark -
#pragma mark New Output Destination

- (IBAction)browseFile:(id)sender
{
    // Open a panel to let the user choose and update the text field
    NSSavePanel *panel = [NSSavePanel savePanel];

	// We get the current file name and path from the destination field here
    [panel setDirectoryURL:self.job.destURL.URLByDeletingLastPathComponent];
    [panel setNameFieldStringValue:self.job.destURL.lastPathComponent];

    [panel beginSheetModalForWindow:fWindow completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton)
        {
            self.job.destURL = panel.URL;

            // Save this path to the prefs so that on next browse destination window it opens there
            NSString *destinationDirectory = [panel.URL.path stringByDeletingLastPathComponent];
            [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        }
    }];
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
	[[NSUserDefaults standardUserDefaults] setObject:NSStringFromSize( contentSize ) forKey:@"HBDrawerSize"];
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
    if (numEvents >= 1)
    {
        // Reload the queue only if one of the following events happened.
        FSEventStreamEventFlags flags = eventFlags[0];
        if (flags & (kFSEventStreamEventFlagItemCreated | kFSEventStreamEventFlagItemRemoved | kFSEventStreamEventFlagItemRenamed | kFSEventStreamEventFlagItemModified))
        {
            HBController *hb = (HBController *)clientCallBackInfo;
            [hb reloadQueue];
        }
    }
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
                                 kFSEventStreamCreateFlagIgnoreSelf | kFSEventStreamCreateFlagMarkSelf
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
	// We declare the default NSFileManager into fileManager
	NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *appSupportPath = [HBUtilities appSupportPath];

	// We define the location of the user presets file
    QueueFile = [[appSupportPath stringByAppendingPathComponent:@"Queue/Queue.hbqueue"] retain];

    // We check for the Queue.plist
	if (![fileManager fileExistsAtPath:QueueFile])
	{
        if (![fileManager fileExistsAtPath:[appSupportPath stringByAppendingPathComponent:@"Queue"]])
        {
            [fileManager createDirectoryAtPath:[appSupportPath stringByAppendingPathComponent:@"Queue"] withIntermediateDirectories:YES attributes:nil error:NULL];
        }
		[fileManager createFileAtPath:QueueFile contents:nil attributes:nil];
	}

    @try
    {
        QueueFileArray = [[NSKeyedUnarchiver unarchiveObjectWithFile:QueueFile] retain];
    }
    @catch (NSException *exception)
    {
        [HBUtilities writeToActivityLog:"failed to read the queue to disk"];
    }

	// lets check to see if there is anything in the queue file .plist
    if (QueueFileArray == nil)
	{
        // if not, then lets initialize an empty array
		QueueFileArray = [[NSMutableArray alloc] init];
    }
    else
    {
        // ONLY clear out encoded items if we are single instance
        if (hbInstanceNum == 1)
        {
            [self clearQueueEncodedItems];
        }
    }
}

- (void)reloadQueue
{
    [HBUtilities writeToActivityLog:"Queue reloaded"];

    NSMutableArray *tempQueueArray = nil;;
    @try
    {
        tempQueueArray = [NSKeyedUnarchiver unarchiveObjectWithFile:QueueFile];
    }
    @catch (NSException *exception)
    {
        tempQueueArray = nil;
        [HBUtilities writeToActivityLog:"failed to read the queue to disk"];
    }

    [QueueFileArray setArray:tempQueueArray];
    // Send Fresh QueueFileArray to fQueueController to update queue window
    [fQueueController setQueueArray:QueueFileArray];
    [self getQueueStats];
}

- (void)addQueueFileItem
{
    [QueueFileArray addObject:[[self.job copy] autorelease]];
    [self saveQueueFileItem];
}

- (void)removeQueueFileItem:(NSUInteger)queueItemToRemove
{
    [QueueFileArray removeObjectAtIndex:queueItemToRemove];
    [self saveQueueFileItem];
}

- (void)saveQueueFileItem
{
    if (![NSKeyedArchiver archiveRootObject:QueueFileArray toFile:QueueFile])
    {
        [HBUtilities writeToActivityLog:"failed to write the queue to disk"];
    }
    [fQueueController setQueueArray: QueueFileArray];
    [self getQueueStats];
}

/**
 *  Updates the queue status label on the main window.
 */
- (void)getQueueStats
{
    // lets get the stats on the status of the queue array
    fPendingCount = 0;
    fWorkingCount = 0;

	int i = 0;
	for (HBJob *job in QueueFileArray)
	{
		if (job.state == HBJobStateWorking) // being encoded
		{
			fWorkingCount++;
            // check to see if we are the instance doing this encoding
            if (job.pidId == pidNum)
            {
                currentQueueEncodeIndex = i;
            }
		}
        if (job.state == HBJobStateReady) // pending
        {
			fPendingCount++;
		}
		i++;
	}

    // Set the queue status field in the main window
    NSString *string;
    if (fPendingCount == 0)
    {
        string = NSLocalizedString( @"No encode pending", @"");
    }
    else if (fPendingCount == 1)
    {
        string = [NSString stringWithFormat: NSLocalizedString( @"%d encode pending", @"" ), fPendingCount];
    }
    else
    {
        string = [NSString stringWithFormat: NSLocalizedString( @"%d encodes pending", @"" ), fPendingCount];
    }

    [fQueueStatus setStringValue:string];
}

/**
 * Used to get the next pending queue item index and return it if found
 */
- (NSInteger)getNextPendingQueueIndex
{
    // initialize nextPendingIndex to -1, this value tells incrementQueueItemDone that there are no pending items in the queue
    NSInteger nextPendingIndex = -1;
	BOOL nextPendingFound = NO;
	for (HBJob *job in QueueFileArray)
	{
        if (job.state == HBJobStateReady && nextPendingFound == NO) // pending
        {
			nextPendingFound = YES;
            nextPendingIndex = [QueueFileArray indexOfObject:job];
            [HBUtilities writeToActivityLog: "getNextPendingQueueIndex next pending encode index is:%d", nextPendingIndex];
		}
	}
    return nextPendingIndex;
}

/**
 * This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void) setQueueEncodingItemsAsPending
{
    for (HBJob *job in QueueFileArray)
    {
        // We want to keep any queue item that is pending or was previously being encoded
        if (job.state == HBJobStateWorking || job.state == HBJobStateReady)
        {
            // If the queue item is marked as "working"
            // then change its status back to ready which effectively
            // puts it back into the queue to be encoded
            if (job.state == HBJobStateWorking)
            {
                job.state = HBJobStateReady;
            }
        }
    }

    [self saveQueueFileItem];
}

/**
 * This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as cancelled encodes
 */
- (void) clearQueueEncodedItems
{
    NSMutableArray *tempArray = [NSMutableArray array];
    for (HBJob *job in QueueFileArray)
    {
        /* If the queue item is either completed (0) or cancelled (3) from the
         * last session, then we put it in tempArray to be deleted from QueueFileArray.
         * NOTE: this means we retain pending (2) and also an item that is marked as
         * still encoding (1). If the queue has an item that is still marked as encoding
         * from a previous session, we can conlude that HB was either shutdown, or crashed
         * during the encodes so we keep it and tell the user in the "Load Queue Alert"
         */
        if (job.state == HBJobStateCompleted || job.state == HBJobStateCanceled)
        {
            [tempArray addObject:job];
        }
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/**
 * This method will clear the queue of all encodes. effectively creating an empty queue
 */
- (void) clearQueueAllItems
{
    NSMutableArray *tempArray = [NSMutableArray array];
    // we look here to see if the preset is we move on to the next one
    for (HBJob *job in QueueFileArray)
    {
        [tempArray addObject:job];
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/**
 * this is actually called from the queue controller to modify the queue array and return it back to the queue controller
 */
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

    // We save all of the Queue data here
    // and it also gets sent back to the queue controller
    [self saveQueueFileItem]; 
    
}

#pragma mark -
#pragma mark Queue Job Processing

- (void)incrementQueueItemDone:(NSInteger)queueItemDoneIndexNum
{
    // Mark the encode just finished as done (status 0)
    HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
    queueJob.state = HBJobStateCompleted;

    // We save all of the Queue data here
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
        // Set our currentQueueEncodeIndex now to the newly found Pending encode as we own it
        currentQueueEncodeIndex = newQueueItemIndex;
        // now we mark the queue item as Status = 1 ( being encoded ) so another instance can not come along and try to scan it while we are scanning
        queueJob = QueueFileArray[currentQueueEncodeIndex];
        queueJob.state = HBJobStateWorking;
        [HBUtilities writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];
        queueJob = QueueFileArray[currentQueueEncodeIndex];
        // now we can go ahead and scan the new pending queue item
        [self performNewQueueScan:queueJob.fileURL.path scanTitleNum:queueJob.titleIdx];
    }
    else
    {
        [HBUtilities writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
        // Since there are no more items to encode, go to queueCompletedAlerts
        // for user specified alerts after queue completed
        [self queueCompletedAlerts];
    }
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (NSInteger) scanTitleNum
{
    HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
    // Tell HB to output a new activity log file for this encode
    [outputPanel startEncodeLog:[queueJob.destURL.path stringByDeletingLastPathComponent]];

    // We now flag the queue item as being owned by this instance of HB using the PID
    queueJob.pidId = pidNum;
    // Get the currentQueueEncodeNameString from the queue item to display in the status field */
    currentQueueEncodeNameString = [[queueJob.destURL.path lastPathComponent]retain];
    // We save all of the Queue data here
    [self saveQueueFileItem];

    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    NSURL *fileURL = [NSURL fileURLWithPath:scanPath];
    [self.queueCore scan:fileURL titleNum:scanTitleNum previewsNum:10 minTitleDuration:0];
}

/**
 * This assumes that we have re-scanned and loaded up a new queue item to send to libhb as fQueueEncodeLibhb 
 */
- (void)processNewQueueEncode
{
    HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];

    if (self.queueCore.titles.count)
    {
        // Reset the title in the job.
        queueJob.title = self.queueCore.titles[0];

        // We should be all setup so let 'er rip
        [self.queueCore encodeJob:queueJob];
        fEncodeState = 1;

        // Lets mark our new encode as 1 or "Encoding"
        queueJob.state = HBJobStateWorking;
        [self saveQueueFileItem];
    }
    else
    {
        [HBUtilities writeToActivityLog: "processNewQueueEncode WARNING nothing found in the title list"];
    }
}

#pragma mark -
#pragma mark Queue Item Editing

/* Rescans the chosen queue item back into the main window */
- (void)rescanQueueItemToMainWindow:(NSUInteger)selectedQueueItem
{
    [HBUtilities writeToActivityLog:"rescanQueueItemToMainWindow: Re-scanning queue item at index:%d", selectedQueueItem];

    // Set the browsedSourceDisplayName for showNewScan
    self.jobFromQueue = QueueFileArray[selectedQueueItem];
    self.browsedSourceDisplayName = self.jobFromQueue.fileURL.lastPathComponent;

    [self performScan:self.jobFromQueue.fileURL.path scanTitleNum:self.jobFromQueue.titleIdx];

    // Now that source is loaded and settings applied, delete the queue item from the queue
    [HBUtilities writeToActivityLog: "applyQueueSettingsToMainWindow: deleting queue item:%d", selectedQueueItem];
    [self removeQueueFileItem:selectedQueueItem];
}

#pragma mark -
#pragma mark Job Handling

/* addToQueue: puts up an alert before ultimately calling doAddToQueue
 */
- (IBAction)addToQueue:(id)sender
{
	// We get the destination directory from the destination field here
	NSString *destinationDirectory = self.job.destURL.path.stringByDeletingLastPathComponent;
	// We check for a valid destination here
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
    
    // We check for and existing file here
    if([[NSFileManager defaultManager] fileExistsAtPath:self.job.destURL.path])
    {
        fileExists = YES;
    }
    
    // We now run through the queue and make sure we are not overwriting an exisiting queue item
	for (HBJob *job in QueueFileArray)
	{
		if ([job.destURL isEqualTo:self.job.destURL])
		{
			fileExistsInQueue = YES;	
		}
	}

	if (fileExists == YES)
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"File already exists."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", self.job.destURL.path];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:fWindow modalDelegate:self didEndSelector:@selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ) contextInfo:NULL];

    }
    else if (fileExistsInQueue == YES)
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"There is already a queue item for this destination."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", self.job.destURL.path];
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
    [self addQueueFileItem];
}

/* Rip: puts up an alert before ultimately calling doRip
*/
- (IBAction) Rip: (id) sender
{
    [HBUtilities writeToActivityLog: "Rip: Pending queue count is %d", fPendingCount];
    /* Rip or Cancel ? */
    if (self.queueCore.state == HBStateWorking || self.queueCore.state == HBStatePaused)
	{
        [self Cancel: sender];
        return;
    }
    
    // We check to see if we need to warn the user that the computer will go to sleep
    // or shut down when encoding is finished
    [self remindUserOfSleepOrShutdown];
    
    // If there are pending jobs in the queue, then this is a rip the queue
    if (fPendingCount > 0)
    {
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        // here lets start the queue with the first pending item
        HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
        [self performNewQueueScan:queueJob.fileURL.path scanTitleNum:queueJob.titleIdx];
        
        return;
    }
    
    // Before adding jobs to the queue, check for a valid destination.
    NSString *destinationDirectory = self.job.destURL.path.stringByDeletingLastPathComponent;
    if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Warning!"];
        [alert setInformativeText:@"This is not a valid destination directory!"];
        [alert runModal];
        [alert release];
        return;
    }
    
    // We check for duplicate name here
    if( [[NSFileManager defaultManager] fileExistsAtPath:self.job.destURL.path] )
    {
        NSAlert *alert = [NSAlert alertWithMessageText:@"File already exists."
                                         defaultButton:@"Cancel"
                                       alternateButton:@"Overwrite"
                                           otherButton:nil
                             informativeTextWithFormat:@"Do you want to overwrite %@?", self.job.destURL.path];
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
        
        // go right to processing the new queue encode
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
        [self performNewQueueScan:queueJob.fileURL.path scanTitleNum:queueJob.titleIdx];
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
        // if there are no jobs in the queue, then add this one to the queue and rip
        // otherwise, just rip the queue
        if (fPendingCount == 0)
        {
            [self doAddToQueue];
        }

        NSString *destinationDirectory = [self.job.destURL.path stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        currentQueueEncodeIndex = [self getNextPendingQueueIndex];
        HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
        [self performNewQueueScan:queueJob.fileURL.path scanTitleNum:queueJob.titleIdx];
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
        [alert release];
    }

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
    [self.queueCore pause];

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
    [self.queueCore resume];

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
    // Stop the current job.
    [self.queueCore stop];

    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [QueueFileArray[currentQueueEncodeIndex] setState:HBJobStateCanceled];
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
        /* now we mark the queue item as worling so another instance can not come along and try to scan it while we are scanning */
        [QueueFileArray[currentQueueEncodeIndex] setState:HBJobStateWorking];
        [HBUtilities writeToActivityLog: "incrementQueueItemDone new pending items found: %d", currentQueueEncodeIndex];
        [self saveQueueFileItem];

        HBJob *queueJob = QueueFileArray[currentQueueEncodeIndex];
        // now we can go ahead and scan the new pending queue item
        [self performNewQueueScan:queueJob.fileURL.path scanTitleNum:queueJob.titleIdx];
    }
    else
    {
        [HBUtilities writeToActivityLog: "incrementQueueItemDone there are no more pending encodes"];
    }
}

- (void) doCancelCurrentJobAndStop
{
    [self.queueCore stop];

    fEncodeState = 2;   // don't alert at end of processing since this was a cancel

    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [QueueFileArray[currentQueueEncodeIndex] setState:HBJobStateCanceled];
    // and as always, save it in Queue.plist
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    // so now lets move to 
    currentQueueEncodeIndex++ ;
    [HBUtilities writeToActivityLog: "cancelling current job and stopping the queue"];
}
- (IBAction) Pause: (id) sender
{
    if (self.queueCore.state == HBStatePaused)
    {
        [self.queueCore resume];
    }
    else
    {
        [self.queueCore pause];
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
                         informativeTextWithFormat:@"Current preset will be applied to all %ld titles. Are you sure you want to do this?", (long)[fSrcTitlePopUp numberOfItems]];
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
    // first get the currently selected index so we can choose it again after cycling through the available titles.
    NSInteger currentlySelectedTitle = [fSrcTitlePopUp indexOfSelectedItem];
    
    /* For each title in the fSrcTitlePopUp, select it */
    for (int i = 0; i < [fSrcTitlePopUp numberOfItems]; i++)
    {
        [fSrcTitlePopUp selectItemAtIndex:i];
        // Now call titlePopUpChanged to load it up
        [self titlePopUpChanged:nil];
        // now add the title to the queue
        [self addToQueue:nil];   
    }
    // Now that we are done, reselect the previously selected title.
    [fSrcTitlePopUp selectItemAtIndex: currentlySelectedTitle];
    // Now call titlePopUpChanged to load it up
    [self titlePopUpChanged:nil]; 
}

#pragma mark -
#pragma mark GUI Controls Changed Methods

- (void)updateFileName
{
    if (!self.job)
    {
        return;
    }

    HBTitle *title = self.job.title;

    // Generate a new file name
    NSString *fileName = [HBUtilities automaticNameForSource:title.name
                                                       title:title.hb_title->index
                                                    chapters:NSMakeRange(self.job.range.chapterStart + 1, self.job.range.chapterStop + 1)
                                                     quality:self.job.video.qualityType ? self.job.video.quality : 0
                                                     bitrate:!self.job.video.qualityType ? self.job.video.avgBitrate : 0
                                                  videoCodec:self.job.video.encoder];

    // Swap the old one with the new one
    self.job.destURL = [[self.job.destURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:
                        [NSString stringWithFormat:@"%@.%@", fileName, self.job.destURL.pathExtension]];
}

- (IBAction) titlePopUpChanged: (id) sender
{
    // If there is already a title load, save the current settings to a preset
    if (self.job)
    {
        self.selectedPreset = [self createPresetFromCurrentSettings];
    }

    HBTitle *title = self.core.titles[fSrcTitlePopUp.indexOfSelectedItem];

    // Check if we are reapplying a job from the queue, or creating a new one
    if (self.jobFromQueue)
    {
        self.jobFromQueue.title = title;
        self.job = self.jobFromQueue;
    }
    else
    {
        self.job = [[[HBJob alloc] initWithTitle:title andPreset:self.selectedPreset] autorelease];

        // use the correct extension based on the container
        const char *ext = hb_container_get_default_extension(self.job.container);

        // Check to see if the last destination has been set,use if so, if not, use Desktop
        if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"])
        {
            NSURL *fileURL = [NSURL fileURLWithPath:[[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"]];
            fileURL = [fileURL URLByAppendingPathComponent:self.browsedSourceDisplayName.stringByDeletingPathExtension];
            fileURL = [fileURL URLByAppendingPathExtension:@(ext)];
            self.job.destURL = fileURL;
        }
        else
        {
            self.job.destURL = [NSURL fileURLWithPath:[NSString stringWithFormat:@"%@/Desktop/%@.%s",
                                                       NSHomeDirectory(),
                                                       self.browsedSourceDisplayName.stringByDeletingPathExtension,
                                                       ext]];
        }

        // set m4v extension if necessary - do not override user-specified .mp4 extension
        if (self.job.container & HB_MUX_MASK_MP4)
        {
            [self autoSetM4vExtension:nil];
        }
    }

    // If we are a stream type and a batch scan, grok the output file name from title->name upon title change
    if ((title.hb_title->type == HB_STREAM_TYPE || title.hb_title->type == HB_FF_STREAM_TYPE) && self.core.titles.count > 1)
    {
        // we set the default name according to the new title->name
        self.job.destURL = [[self.job.destURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:
                            [NSString stringWithFormat:@"%@.%@", title.name, self.job.destURL.pathExtension]];

        // Change the source to read out the parent folder also
        fSrcDVD2Field.stringValue = [NSString stringWithFormat:@"%@/%@", self.browsedSourceDisplayName, title.name];
    }

    // apply the current preset
    if (!self.jobFromQueue)
    {
        // Set Auto Crop to on upon selecting a new title
        self.job.picture.autocrop = YES;

        [self applyPreset:self.selectedPreset];
    }
}

- (IBAction) encodeStartStopPopUpChanged: (id) sender;
{
    // We are chapters
    if ([fEncodeStartStopPopUp indexOfSelectedItem] == 0)
    {
        self.job.range.type = HBRangeTypeChapters;

        [fSrcChapterStartPopUp  setHidden: NO];
        [fSrcChapterEndPopUp  setHidden: NO];

        [fSrcTimeStartEncodingField  setHidden: YES];
        [fSrcTimeEndEncodingField  setHidden: YES];

        [fSrcFrameStartEncodingField  setHidden: YES];
        [fSrcFrameEndEncodingField  setHidden: YES];

        [self chapterPopUpChanged:nil];
    }
    // We are time based (seconds)
    else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 1)
    {
        self.job.range.type = HBRangeTypeSeconds;

        [fSrcChapterStartPopUp  setHidden: YES];
        [fSrcChapterEndPopUp  setHidden: YES];

        [fSrcTimeStartEncodingField  setHidden: NO];
        [fSrcTimeEndEncodingField  setHidden: NO];

        [fSrcFrameStartEncodingField  setHidden: YES];
        [fSrcFrameEndEncodingField  setHidden: YES];
    }
    // We are frame based
    else if ([fEncodeStartStopPopUp indexOfSelectedItem] == 2)
    {
        self.job.range.type = HBRangeTypeFrames;

        [fSrcChapterStartPopUp  setHidden: YES];
        [fSrcChapterEndPopUp  setHidden: YES];

        [fSrcTimeStartEncodingField  setHidden: YES];
        [fSrcTimeEndEncodingField  setHidden: YES];

        [fSrcFrameStartEncodingField  setHidden: NO];
        [fSrcFrameEndEncodingField  setHidden: NO];
    }
}

- (IBAction) chapterPopUpChanged: (id) sender
{
    // We're changing the chapter range - we may need to flip the m4v/mp4 extension
    if ([[fDstFormatPopUp selectedItem] tag] & HB_MUX_MASK_MP4)
    {
        [self autoSetM4vExtension:sender];
    }

    // If Auto Naming is on it might need to be update if it includes the chapters range
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
	{
        [self updateFileName];
	}
}

- (void)formatChanged:(NSNotification *)notification
{
    if (self.job)
    {
        int videoContainer = self.job.container;

        // set the file extension
        const char *ext = hb_container_get_default_extension(videoContainer);
        self.job.destURL = [[self.job.destURL URLByDeletingPathExtension] URLByAppendingPathExtension:@(ext)];

        if (videoContainer & HB_MUX_MASK_MP4)
        {
            [self autoSetM4vExtension:notification];
        }
    }
}

- (void) autoSetM4vExtension:(NSNotification *)notification
{
    if (!(self.job.container & HB_MUX_MASK_MP4))
        return;
    
    NSString *extension = @"mp4";
    
    BOOL anyCodecAC3 = [fAudioController anyCodecMatches: HB_ACODEC_AC3] || [fAudioController anyCodecMatches: HB_ACODEC_AC3_PASS];
    // Chapter markers are enabled if the checkbox is ticked and we are doing p2p or we have > 1 chapter
    BOOL chapterMarkers = (self.job.chaptersEnabled) &&
                          (self.job.range.type != HBRangeTypeChapters ||
                           self.job.range.chapterStart < self.job.range.chapterStop);

    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultMpegExtension"] isEqualToString: @".m4v"] || 
        ((YES == anyCodecAC3 || YES == chapterMarkers) &&
         [[[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultMpegExtension"] isEqualToString: @"Auto"]))
    {
        extension = @"m4v";
    }
    
    if ([extension isEqualTo:self.job.destURL.pathExtension])
    {
        return;
    }
    else
    {
        self.job.destURL = [[self.job.destURL URLByDeletingPathExtension] URLByAppendingPathExtension:extension];
   }
}

/**
 * Method to determine if we should change the UI
 * To reflect whether or not a Preset is being used or if
 * the user is using "Custom" settings by determining the sender
 */
- (void)customSettingUsed
{
    // Deselect the currently selected Preset if there is one*/
    [fPresetsView deselect];
    // Change UI to show "Custom" settings are being used */
    fPresetSelectedDisplay.stringValue = NSLocalizedString(@"Custom", @"");
    self.customPreset = YES;

    // If Auto Naming is on it might need to be update if it includes the quality token
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
    fVideoController.pictureSettings = self.job.picture.summary;
    fVideoController.pictureFilters = self.job.filters.summary;

    [fPreviewController reloadPreviews];
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
    if (fPreferencesController == nil)
    {
        fPreferencesController = [[HBPreferencesController alloc] init];
    }

    NSWindow *window = [fPreferencesController window];
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
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"HBDefaultPresetsDrawerShow"];
    }
    else
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"HBDefaultPresetsDrawerShow"];
    }

    [fPresetDrawer toggle:self];
}

/**
 * Shows Picture Settings Window.
 */

- (IBAction) showPicturePanel: (id) sender
{
	[fPictureController showPictureWindow];
}

- (IBAction) showPreviewWindow: (id) sender
{
	[fPreviewController showWindow:sender];
}

#pragma mark - Preset  Methods

- (void)applyPreset:(HBPreset *)preset
{
    if (preset != nil && self.job)
    {
        self.selectedPreset = preset;
        self.customPreset = NO;

        NSDictionary *chosenPreset = preset.content;

        [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];

        if ([[chosenPreset objectForKey:@"Default"] intValue] == 1)
        {
            [fPresetSelectedDisplay setStringValue:[NSString stringWithFormat:@"%@ (Default)", [chosenPreset objectForKey:@"PresetName"]]];
        }
        else
        {
            [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];
        }

        // Apply the preset to the current job
        [self.job applyPreset:preset];

        // check to see if we have only one chapter
        [self chapterPopUpChanged:nil];

        // Audio
        [fAudioController applySettingsFromPreset: chosenPreset];
        
        // Subtitles
        [fSubtitlesViewController applySettingsFromPreset:chosenPreset];

        // If Auto Naming is on. We create an output filename of dvd name - title number
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
        {
            [self updateFileName];
        }

        [self pictureSettingsDidChange];
    }
}

#pragma mark - Presets View Controller Delegate

- (void)selectionDidChange
{
    [self applyPreset:fPresetsView.selectedPreset];
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

- (IBAction)showAddPresetPanel:(id)sender
{
	/* Show the add panel */
    HBAddPresetController *addPresetController = [[HBAddPresetController alloc] initWithPreset:[self createPresetFromCurrentSettings]
                                                                                     videoSize:NSMakeSize(self.job.picture.width, self.job.picture.height)];

    [NSApp beginSheet:addPresetController.window modalForWindow:fWindow modalDelegate:self didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:) contextInfo:addPresetController];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    HBAddPresetController *addPresetController = (HBAddPresetController *)contextInfo;

    if (returnCode == NSModalResponseContinue)
    {
        [presetManager addPreset:addPresetController.preset];
    }

    [addPresetController release];
}

- (HBPreset *)createPresetFromCurrentSettings
{
    NSMutableDictionary *preset = [NSMutableDictionary dictionary];
    NSDictionary *currentPreset = self.selectedPreset.content;

    preset[@"PresetBuildNumber"] = [NSString stringWithFormat: @"%d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]];
    preset[@"PresetName"] = fPresetSelectedDisplay.stringValue;
    preset[@"Folder"] = @NO;

	// Set whether or not this is a user preset or factory 0 is factory, 1 is user
    preset[@"Type"] = @1;
    preset[@"Default"] = @0;

    // Get the whether or not to apply pic Size and Cropping (includes Anamorphic)
    preset[@"UsesPictureSettings"] = currentPreset[@"UsesPictureSettings"];
    // Get whether or not to use the current Picture Filter settings for the preset
    preset[@"UsesPictureFilters"] = currentPreset[@"UsesPictureFilters"];

    preset[@"PictureWidth"]  = currentPreset[@"PictureWidth"];
    preset[@"PictureHeight"] = currentPreset[@"PictureHeight"];

    preset[@"PresetDescription"] = currentPreset[@"PresetDescription"];

    [self.job applyCurrentSettingsToPreset:preset];

    return [[[HBPreset alloc] initWithName:preset[@"PresetName"] content:preset builtIn:NO] autorelease];
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
        NSMutableArray *presetsToImport = [[NSMutableArray alloc] initWithContentsOfURL:importPresetsFile];
        /* iterate though the new array of presets to import and add them to our presets array */
        for (NSMutableDictionary *tempObject in presetsToImport)
        {
            /* make any changes to the incoming preset we see fit */
            /* make sure the incoming preset is not tagged as default */
            [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
            /* prepend "(imported) to the name of the incoming preset for clarification since it can be changed */
            NSString *prependedName = [@"(import) " stringByAppendingString:[tempObject objectForKey:@"PresetName"]] ;
            [tempObject setObject:prependedName forKey:@"PresetName"];
            
            /* actually add the new preset to our presets array */
            [presetManager addPresetFromDictionary:tempObject];
        }
        [presetsToImport autorelease];
    }];
}

#pragma mark -
#pragma mark Preset Menu

- (IBAction)selectDefaultPreset:(id)sender
{
    [self applyPreset:presetManager.defaultPreset];
    [fPresetsView setSelection:_selectedPreset];
}

- (IBAction)insertFolder:(id)sender
{
    [fPresetsView insertFolder:sender];
}

- (IBAction)selectPresetFromMenu:(id)sender
{
    // Retrieve the preset stored in the NSMenuItem
    HBPreset *preset = [sender representedObject];

    [self applyPreset:preset];
    [fPresetsView setSelection:preset];
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
                item.representedObject = obj;
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
