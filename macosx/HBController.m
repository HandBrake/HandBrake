/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBController.h"

#import "HBQueueController.h"

#import "HBOutputPanelController.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"
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

#import "HBCore.h"
#import "HBJob.h"

@interface HBController () <HBPresetsViewControllerDelegate, HBPreviewControllerDelegate, HBPictureControllerDelegate>

@property (assign) IBOutlet NSView *openTitleView;
@property (nonatomic, readwrite) BOOL scanSpecificTitle;
@property (nonatomic, readwrite) NSInteger scanSpecificTitleIdx;

/**
 * The name of the source, it might differ from the source
 * last path component if it's a package or a folder.
 */
@property (nonatomic, copy) NSString *browsedSourceDisplayName;

/// The current job.
@property (nonatomic, retain) HBJob *job;

/// The job to be applied from the queue.
@property (nonatomic, retain) HBJob *jobFromQueue;

/// The current selected preset.
@property (nonatomic, retain) HBPreset *selectedPreset;
@property (nonatomic) BOOL customPreset;

///  The HBCore used for scanning.
@property (nonatomic, retain) HBCore *core;

@property (nonatomic, readwrite) NSColor *labelColor;

@end

@implementation HBController

- (instancetype)initWithQueue:(HBQueueController *)queueController presetsManager:(HBPresetsManager *)manager;
{
    self = [super initWithWindowNibName:@"MainWindow"];
    if (self)
    {
        // Init libhb
        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
        _core = [[HBCore alloc] initWithLoggingLevel:loggingLevel];
        _core.name = @"ScanCore";

        // Inits the controllers
        fPictureController = [[HBPictureController alloc] init];
        [fPictureController setDelegate:self];

        fPreviewController = [[HBPreviewController  alloc] initWithDelegate:self];
        [fPreviewController setCore:self.core];

        fQueueController = queueController;
        fQueueController.controller = self;

        presetManager = manager;
        _selectedPreset = [presetManager.defaultPreset retain];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(autoSetM4vExtension:) name:HBMixdownChangedNotification object:nil];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [fPreviewController release];
    [fPictureController release];

    [_browsedSourceDisplayName release];
    [_job release];
    [_jobFromQueue release];
    [_selectedPreset release];
    [_labelColor release];
    [_core release];

    [super dealloc];
}

- (void)windowDidLoad
{
    [self enableUI:NO];

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

    NSSize drawerSize = NSSizeFromString([[NSUserDefaults standardUserDefaults]
                                          stringForKey:@"HBDrawerSize"]);
    if (drawerSize.width)
    {
        [fPresetDrawer setContentSize: drawerSize];
    }

    // Align the start / stop widgets with the chapter popups
    NSPoint startPoint = [fSrcChapterStartPopUp frame].origin;
    startPoint.y += 2;

    NSPoint endPoint = [fSrcChapterEndPopUp frame].origin;
    endPoint.y += 2;

    [fSrcTimeStartEncodingField setFrameOrigin:startPoint];
    [fSrcTimeEndEncodingField setFrameOrigin:endPoint];

    [fSrcFrameStartEncodingField setFrameOrigin:startPoint];
    [fSrcFrameEndEncodingField setFrameOrigin:endPoint];

    // Bottom
    [fStatusField setStringValue:@""];

    // Register HBController's Window as a receiver for files/folders drag & drop operations
    [self.window registerForDraggedTypes:@[NSFilenamesPboardType]];

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
    
    [self.window recalculateKeyViewLoop];
}

#pragma mark -
#pragma mark Drag & drop handling

/** This method is used by OSX to know what kind of files can be drag & drop on the NSWindow
 * We only want filenames (and so folders too)
 */
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

    if ([pboard.types containsObject:NSFilenamesPboardType])
    {
        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        [self openFile:[NSURL fileURLWithPath:paths.firstObject]];
    }

    return YES;
}

#pragma mark - KVO

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

- (void)enableUI:(BOOL)enabled
{
    if (enabled)
    {
        self.labelColor = [NSColor controlTextColor];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
    }

    fPresetsView.enabled = enabled;
}

- (NSSize)drawerWillResizeContents:(NSDrawer *) drawer toSize:(NSSize)contentSize {
    [[NSUserDefaults standardUserDefaults] setObject:NSStringFromSize(contentSize) forKey:@"HBDrawerSize"];
    return contentSize;
}

#pragma mark - UI Validation

- (BOOL)validateToolbarItem:(NSToolbarItem *)toolbarItem
{
    SEL action = toolbarItem.action;

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

        if (action == @selector(rip:) || action == @selector(addToQueue:))
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

    HBState queueState = fQueueController.core.state;

    if (queueState == HBStateScanning || queueState == HBStateWorking || queueState == HBStateSearching || queueState == HBStateMuxing)
    {
        if (action == @selector(rip:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
            [toolbarItem setLabel: @"Stop"];
            [toolbarItem setPaletteLabel: @"Stop"];
            [toolbarItem setToolTip: @"Stop Encoding"];
            return YES;
        }
        if (action == @selector(pause:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"pauseencode"]];
            [toolbarItem setLabel: @"Pause"];
            [toolbarItem setPaletteLabel: @"Pause Encoding"];
            [toolbarItem setToolTip: @"Pause Encoding"];
            return YES;
        }
    }
    else if (queueState == HBStatePaused)
    {
        if (action == @selector(pause:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: @"Resume"];
            [toolbarItem setPaletteLabel: @"Resume Encoding"];
            [toolbarItem setToolTip: @"Resume Encoding"];
            return YES;
        }
        if (action == @selector(rip:))
            return YES;
    }
    else
    {
        if (action == @selector(rip:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
            if (fQueueController.pendingItemsCount > 0)
                [toolbarItem setLabel: @"Start Queue"];
            else
                [toolbarItem setLabel: @"Start"];
            [toolbarItem setPaletteLabel: @"Start Encoding"];
            [toolbarItem setToolTip: @"Start Encoding"];
        }
    }

    if (self.job)
    {
        if (action == @selector(showPicturePanel:) ||
            action == @selector(showPreviewWindow:) ||
            action == @selector(addToQueue:))
        {
            return YES;
        }
    }
    else
    {
        if (action == @selector(showPicturePanel:) ||
            action == @selector(showPreviewWindow:) ||
            action == @selector(addToQueue:))
        {
            return NO;
        }
    }

    // If there are any pending queue items, make sure the start/stop button is active.
    if (action == @selector(rip:) && (fQueueController.pendingItemsCount > 0 || self.job))
    {
        return YES;
    }

    return YES;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = [menuItem action];

    if (action == @selector(addToQueue:) || action == @selector(addAllTitlesToQueue:) ||
        action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:) ||
        action == @selector(showPreviewWindow:))
    {
        return self.job && self.window.attachedSheet == nil;
    }
    if (action == @selector(selectDefaultPreset:))
    {
        return self.window.attachedSheet == nil;
    }
    if (action == @selector(pause:))
    {
        return [fQueueController validateMenuItem:menuItem];
    }
    if (action == @selector(rip:))
    {
        BOOL result = [fQueueController validateMenuItem:menuItem];

        if ([menuItem.title isEqualToString:NSLocalizedString(@"Start Encoding", nil)])
        {
            if (!result && self.job)
            {
                return YES;
            }
        }

        return result;
    }
    if (action == @selector(browseSources:))
    {
        if (self.core.state == HBStateScanning) {
            return NO;
        }
        else
        {
            return self.window.attachedSheet == nil;
        }
    }
    if (action == @selector(selectPresetFromMenu:))
    {
        if ([menuItem.representedObject isEqualTo:self.selectedPreset])
        {
            menuItem.state = NSOnState;
        }
        else
        {
            menuItem.state = NSOffState;
        }
        return (self.job != nil);
    }

    return YES;
}

#pragma mark - Get New Source

- (void)launchAction
{
    if (self.core.state != HBStateScanning && !self.job)
    {
        // We show whichever open source window specified in LaunchSourceBehavior preference key
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
        {
            [self browseSources:nil];
        }
    }
    
}

- (void)openFile:(NSURL *)fileURL
{
    if (self.core.state != HBStateScanning)
    {
        self.browsedSourceDisplayName = fileURL.lastPathComponent;
        [self performScan:fileURL scanTitleNum:0];
    }
}

- (void)setJob:(HBJob *)job
{
    // Set the jobs info to the view controllers
    fPictureController.picture = job.picture;
    fPictureController.filters = job.filters;
    fPreviewController.job = job;

    fVideoController.job = job;
    fAudioController.audio = job.audio;
    fSubtitlesViewController.subtitles = job.subtitles;
    fChapterTitlesController.job = job;

    if (job)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:_job];
        [[NSNotificationCenter defaultCenter] removeObserver:_job.picture];
        [[NSNotificationCenter defaultCenter] removeObserver:_job.filters];
        [[NSNotificationCenter defaultCenter] removeObserver:_job.video];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pictureSettingsDidChange) name:HBPictureChangedNotification object:job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pictureSettingsDidChange) name:HBFiltersChangedNotification object:job.filters];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(formatChanged:) name:HBContainerChangedNotification object:job];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(customSettingUsed) name:HBVideoChangedNotification object:job.video];
    }

    // Retain the new job
    [_job autorelease];
    _job = [job retain];

    [self enableUI:(job != nil)];
}

/**
 * Opens the source browse window, called from Open Source widgets
 */
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
	if ([[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastSourceDirectoryURL"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastSourceDirectoryURL"];
	}
	else
	{
		sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
	}

    [panel setDirectoryURL:sourceDirectory];
    [panel setAccessoryView:self.openTitleView];

    [panel beginSheetModalForWindow:self.window completionHandler: ^(NSInteger result)
    {
         if (result == NSOKButton)
         {
             NSURL *scanURL = panel.URL;
             // we set the last searched source directory in the prefs here
             [[NSUserDefaults standardUserDefaults] setURL:scanURL.URLByDeletingLastPathComponent forKey:@"HBLastSourceDirectoryURL"];

             NSURL *url = panel.URL;

             // We check to see if the chosen file at path is a package
             if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:url.path])
             {
                 [HBUtilities writeToActivityLog:"trying to open a package at: %s", url.path.UTF8String];
                 // We check to see if this is an .eyetv package
                 if ([url.pathExtension isEqualToString:@"eyetv"])
                 {
                     [HBUtilities writeToActivityLog:"trying to open eyetv package"];
                     // We're looking at an EyeTV package - try to open its enclosed .mpg media file
                     self.browsedSourceDisplayName = url.URLByDeletingPathExtension.lastPathComponent;
                     NSString *mpgname;
                     NSUInteger n = [[url.path stringByAppendingString: @"/"]
                                     completePathIntoString: &mpgname caseSensitive: YES
                                     matchesIntoArray: nil
                                     filterTypes: @[@"mpg"]];
                     if (n > 0)
                     {
                         // Found an mpeg inside the eyetv package, make it our scan path
                         [HBUtilities writeToActivityLog:"found mpeg in eyetv package"];
                         url = [NSURL fileURLWithPath:mpgname];
                     }
                     else
                     {
                         // We did not find an mpeg file in our package, so we do not call performScan
                         [HBUtilities writeToActivityLog:"no valid mpeg in eyetv package"];
                     }
                 }
                 // We check to see if this is a .dvdmedia package
                 else if ([url.pathExtension isEqualToString:@"dvdmedia"])
                 {
                     // path IS a package - but dvdmedia packages can be treaded like normal directories
                     self.browsedSourceDisplayName = url.URLByDeletingPathExtension.lastPathComponent;
                     [HBUtilities writeToActivityLog:"trying to open dvdmedia package"];
                 }
                 else
                 {
                     // The package is not an eyetv package, try to open it anyway
                     self.browsedSourceDisplayName = url.lastPathComponent;
                     [HBUtilities writeToActivityLog:"not a known to package"];
                 }
             }
             else
             {
                 // path is not a package, so we call perform scan directly on our file
                 if ([url.lastPathComponent isEqualToString:@"VIDEO_TS"])
                 {
                     [HBUtilities writeToActivityLog:"trying to open video_ts folder (video_ts folder chosen)"];
                     // If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
                     url = url.URLByDeletingLastPathComponent;
                     self.browsedSourceDisplayName = url.lastPathComponent;
                 }
                 else
                 {
                     [HBUtilities writeToActivityLog:"trying to open a folder or file"];
                     // if not the VIDEO_TS Folder, we can assume the chosen folder is the source name
                     // make sure we remove any path extension
                     self.browsedSourceDisplayName = url.lastPathComponent;
                 }
             }

             NSInteger titleIdx = 0;
             if (self.scanSpecificTitle)
             {
                 titleIdx = self.scanSpecificTitleIdx;
             }
             [self performScan:url scanTitleNum:titleIdx];
         }
     }];
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void)performScan:(NSURL *)scanURL scanTitleNum:(NSInteger)scanTitleNum
{
    // Save the current settings
    if (self.job)
    {
        self.selectedPreset = [self createPresetFromCurrentSettings];
    }

    self.job = nil;
    [fSrcTitlePopUp removeAllItems];

    NSError *outError = NULL;
    BOOL suppressWarning = [[NSUserDefaults standardUserDefaults] boolForKey:@"suppresslibdvdcss"];

    // Check if we can scan the source and if there is any warning.
    BOOL canScan = [self.core canScan:scanURL error:&outError];

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

        [self.core scanURL:scanURL
               titleIndex:scanTitleNum
            previews:hb_num_previews minDuration:min_title_duration_seconds
        progressHandler:^(HBState state, hb_state_t hb_state)
        {
            #define p hb_state.param.scanning
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
        }
    completationHandler:^(BOOL success)
        {
            fScanHorizontalLine.hidden = NO;
            fScanIndicator.hidden = YES;
            fScanIndicator.indeterminate = NO;
            fScanIndicator.doubleValue = 0.0;

            if (success)
            {
                [self showNewScan];
            }
            else
            {
                // We display a message if a valid source was not chosen
                fSrcDVD2Field.stringValue = NSLocalizedString(@"No Valid Source Found", @"");
            }
            [self.window.toolbar validateVisibleItems];
        }];
    }
}

- (void)showNewScan
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

    for (HBTitle *title in self.core.titles)
    {
        // Set Source Name at top of window with the browsedSourceDisplayName grokked right before -performScan
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

        self.jobFromQueue = nil;
    }
}

#pragma mark - GUI Controls Changed Methods

- (IBAction)browseDestination:(id)sender
{
    // Open a panel to let the user choose and update the text field
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.directoryURL = self.job.destURL.URLByDeletingLastPathComponent;
    panel.nameFieldStringValue = self.job.destURL.lastPathComponent;

    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
     {
         if (result == NSFileHandlingPanelOKButton)
         {
             self.job.destURL = panel.URL;

             // Save this path to the prefs so that on next browse destination window it opens there
             [[NSUserDefaults standardUserDefaults] setURL:panel.URL.URLByDeletingLastPathComponent
                                                    forKey:@"HBLastDestinationDirectory"];
         }
     }];
}

- (void)updateFileName
{
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

- (NSURL *)destURLForJob:(HBJob *)job
{
    // Check to see if the last destination has been set,use if so, if not, use Desktop
    NSURL *destURL = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectory"];
    if (!destURL || ![[NSFileManager defaultManager] fileExistsAtPath:destURL.path])
    {
        destURL = [NSURL fileURLWithPath:[NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) firstObject]
                             isDirectory:YES];
    }

    destURL = [destURL URLByAppendingPathComponent:job.title.name];
    // use the correct extension based on the container
    const char *ext = hb_container_get_default_extension(self.job.container);
    destURL = [destURL URLByAppendingPathExtension:@(ext)];

    return destURL;
}

- (IBAction)titlePopUpChanged:(id)sender
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
        self.job.destURL = [self destURLForJob:self.job];

        // set m4v extension if necessary - do not override user-specified .mp4 extension
        if (self.job.container & HB_MUX_MASK_MP4)
        {
            [self autoSetM4vExtension:nil];
        }
    }

    // If we are a stream type and a batch scan, grok the output file name from title->name upon title change
    if ((title.hb_title->type == HB_STREAM_TYPE || title.hb_title->type == HB_FF_STREAM_TYPE) && self.core.titles.count > 1)
    {
        // Change the source to read out the parent folder also
        fSrcDVD2Field.stringValue = [NSString stringWithFormat:@"%@/%@", self.browsedSourceDisplayName, title.name];
    }

    // apply the current preset
    if (!self.jobFromQueue)
    {
        [self applyPreset:self.selectedPreset];
    }
}

- (void)chapterPopUpChanged:(NSNotification *)notification
{
    // We're changing the chapter range - we may need to flip the m4v/mp4 extension
    if (self.job.container & HB_MUX_MASK_MP4)
    {
        [self autoSetM4vExtension:notification];
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

- (void)autoSetM4vExtension:(NSNotification *)notification
{
    if (!(self.job.container & HB_MUX_MASK_MP4))
        return;

    NSString *extension = @"mp4";

    BOOL anyCodecAC3 = [self.job.audio anyCodecMatches:HB_ACODEC_AC3] || [self.job.audio anyCodecMatches:HB_ACODEC_AC3_PASS];
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
    // Deselect the currently selected Preset if there is one
    [fPresetsView deselect];
    // Change UI to show "Custom" settings are being used
    self.job.presetName = NSLocalizedString(@"Custom", @"");

    // If Auto Naming is on it might need to be update if it includes the quality token
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
    {
        [self updateFileName];
    }
}

#pragma mark - Queue progress

- (void)setQueueState:(NSString *)info
{
    fQueueStatus.stringValue = info;
}

#define WINDOW_HEIGHT 591
#define WINDOW_HEIGHT_OFFSET 36

- (void)setQueueInfo:(NSString *)info progress:(double)progress hidden:(BOOL)hidden
{
    fStatusField.stringValue = info;
    fRipIndicator.doubleValue = progress;

    if (hidden)
    {
        if (fRipIndicatorShown)
        {
            NSRect frame = self.window.frame;
            if (frame.size.width <= WINDOW_HEIGHT)
                frame.size.width = WINDOW_HEIGHT;
            frame.size.height += -WINDOW_HEIGHT_OFFSET;
            frame.origin.y -= -WINDOW_HEIGHT_OFFSET;
            [self.window setFrame:frame display:YES animate:YES];
            fRipIndicatorShown = NO;

            // Refresh the toolbar buttons
            [self.window.toolbar validateVisibleItems];
        }
    }
    else
    {
        // If progress bar hasn't been revealed at the bottom of the window, do
        // that now.
        if (!fRipIndicatorShown)
        {
            NSRect frame = self.window.frame;
            if (frame.size.width <= WINDOW_HEIGHT)
                frame.size.width = WINDOW_HEIGHT;
            frame.size.height += WINDOW_HEIGHT_OFFSET;
            frame.origin.y -= WINDOW_HEIGHT_OFFSET;
            [self.window setFrame:frame display:YES animate:YES];
            fRipIndicatorShown = YES;

            // Refresh the toolbar buttons
            [self.window.toolbar validateVisibleItems];
        }
    }
}

#pragma mark - Job Handling

/**
 *  Actually adds a job to the queue
 */
- (void)doAddToQueue
{
    [fQueueController addJob:[[self.job copy] autorelease]];
}

/**
 * Puts up an alert before ultimately calling doAddToQueue
 */
- (IBAction)addToQueue:(id)sender
{
	// We get the destination directory from the destination field here
	NSString *destinationDirectory = self.job.destURL.path.stringByDeletingLastPathComponent;
	// We check for a valid destination here
	if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
	{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Warning!", @"")];
        [alert setInformativeText:NSLocalizedString(@"This is not a valid destination directory!", @"")];
        [alert runModal];
        [alert release];
        return;
	}

	if ([[NSFileManager defaultManager] fileExistsAtPath:self.job.destURL.path])
    {
        // File exist, warn user
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"File already exists.", @"")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @""), self.job.destURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"")];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(overwriteAddToQueueAlertDone:returnCode:contextInfo:) contextInfo:NULL];
        [alert release];
    }
    else if ([fQueueController jobExistAtURL:self.job.destURL])
    {
        // File exist in queue, warn user
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"There is already a queue item for this destination.", @"")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @""), self.job.destURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"")];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(overwriteAddToQueueAlertDone:returnCode:contextInfo:) contextInfo:NULL];
        [alert release];
    }
    else
    {
        [self doAddToQueue];
    }
}

/**
 * Called from the alert posted by addToQueue
 * that asks the user if they want to overwrite an exiting movie file.
 */
- (void)overwriteAddToQueueAlertDone:(NSAlert *)alert
                          returnCode:(NSInteger)returnCode
                         contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        [self doAddToQueue];
    }
}

/**
 * Rescans the chosen queue item back into the main window
 */
- (void)rescanJobToMainWindow:(HBJob *)queueItem
{
    // Set the browsedSourceDisplayName for showNewScan
    self.jobFromQueue = queueItem;
    self.browsedSourceDisplayName = self.jobFromQueue.fileURL.lastPathComponent;

    [self performScan:self.jobFromQueue.fileURL scanTitleNum:self.jobFromQueue.titleIdx];
}

- (void)doRip
{
    // if there are no jobs in the queue, then add this one to the queue and rip
    // otherwise, just rip the queue
    if (fQueueController.pendingItemsCount == 0)
    {
        [self doAddToQueue];
    }

    [fQueueController rip:self];
}

/**
 * Puts up an alert before ultimately calling doRip
 */
- (IBAction)rip:(id)sender
{
    // Rip or Cancel ?
    if (fQueueController.core.state == HBStateWorking || fQueueController.core.state == HBStatePaused)
	{
        // Displays an alert asking user if the want to cancel encoding of current job.
        [fQueueController cancelRip:self];
        return;
    }

    // If there are pending jobs in the queue, then this is a rip the queue
    if (fQueueController.pendingItemsCount > 0)
    {
        [fQueueController rip:self];
        return;
    }

    // Before adding jobs to the queue, check for a valid destination.
    NSString *destinationDirectory = self.job.destURL.path.stringByDeletingLastPathComponent;
    if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Warning!", @"")];
        [alert setInformativeText:NSLocalizedString(@"This is not a valid destination directory!", @"")];
        [alert runModal];
        [alert release];
        return;
    }

    // We check for duplicate name here
    if ([[NSFileManager defaultManager] fileExistsAtPath:self.job.destURL.path])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Warning!", @"")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @""), self.job.destURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"")];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(overWriteAlertDone:returnCode:contextInfo:) contextInfo:NULL];
        // overWriteAlertDone: will be called when the alert is dismissed. It will call doRip.
        [alert release];
    }
    else
    {
        [self doRip];
    }
}

/**
 * overWriteAlertDone: called from the alert posted by Rip: that asks the user if they
 * want to overwrite an exiting movie file.
 */
- (void)overWriteAlertDone:(NSAlert *)alert
                returnCode:(NSInteger)returnCode
               contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        [self doRip];
    }
}

- (IBAction)pause:(id)sender
{
    if (fQueueController.core.state == HBStatePaused)
    {
        [fQueueController.core resume];
    }
    else
    {
        [fQueueController.core pause];
    }
}

#pragma mark -
#pragma mark Batch Queue Titles Methods

- (IBAction)addAllTitlesToQueue:(id)sender
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"You are about to add ALL titles to the queue!", @"")];
    [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Current preset will be applied to all %ld titles. Are you sure you want to do this?", @""), self.core.titles.count]];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
    [alert addButtonWithTitle:NSLocalizedString(@"Yes, I want to add all titles to the queue", @"")];
    [alert setAlertStyle:NSCriticalAlertStyle];

    [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(addAllTitlesToQueueAlertDone:returnCode:contextInfo:) contextInfo:NULL];
    [alert release];
}

- (void)addAllTitlesToQueueAlertDone:(NSAlert *)alert
                          returnCode:(NSInteger)returnCode
                         contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        [self doAddAllTitlesToQueue];
    }
}

- (void)doAddAllTitlesToQueue
{
    NSMutableArray *jobs = [[NSMutableArray alloc] init];

    for (HBTitle *title in self.core.titles)
    {
        HBJob *job = [[HBJob alloc] initWithTitle:title andPreset:self.selectedPreset];
        job.destURL = [self destURLForJob:job];
        [jobs addObject:job];
        [job release];
    }

    [fQueueController addJobsFromArray:jobs];
    [jobs release];
}

#pragma mark - Picture

/**
 * Registers changes made in the Picture Settings Window.
 */
- (void)pictureSettingsDidChange 
{
    [fPreviewController reloadPreviews];
    [self customSettingUsed];
}

- (IBAction)toggleDrawer:(id)sender
{
    if (fPresetDrawer.state == NSDrawerClosedState)
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
- (IBAction)showPicturePanel:(id)sender
{
	[fPictureController showPictureWindow];
}

- (IBAction)showPreviewWindow:(id)sender
{
	[fPreviewController showWindow:sender];
}

#pragma mark - Presets View Controller Delegate

- (void)selectionDidChange
{
    [self applyPreset:fPresetsView.selectedPreset];
}

#pragma mark -  Presets

- (void)applyPreset:(HBPreset *)preset
{
    if (preset != nil && self.job)
    {
        self.selectedPreset = preset;

        // Apply the preset to the current job
        [self.job applyPreset:preset];

        // If Auto Naming is on. We create an output filename of dvd name - title number
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
        {
            [self updateFileName];
        }

        [fPreviewController reloadPreviews];
    }
}

- (IBAction)showAddPresetPanel:(id)sender
{
	// Show the add panel
    HBAddPresetController *addPresetController = [[HBAddPresetController alloc] initWithPreset:[self createPresetFromCurrentSettings]
                                                                                     videoSize:NSMakeSize(self.job.picture.width, self.job.picture.height)];

    [NSApp beginSheet:addPresetController.window modalForWindow:self.window modalDelegate:self didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:) contextInfo:addPresetController];
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
    preset[@"PresetName"] = self.job.presetName;
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
    // Open a panel to let the user choose where and how to save the export file
    NSSavePanel *panel = [NSSavePanel savePanel];
	// We get the current file name and path from the destination field here
    NSURL *defaultExportDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
    [panel setDirectoryURL:defaultExportDirectory];
    [panel setNameFieldStringValue:@"HB_Export.plist"];
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result) {
        if( result == NSOKButton )
        {
            NSURL *exportPresetsFile = [panel URL];
            NSURL *presetExportDirectory = [exportPresetsFile URLByDeletingLastPathComponent];
            [[NSUserDefaults standardUserDefaults] setURL:presetExportDirectory forKey:@"LastPresetExportDirectoryURL"];

            // We check for the presets.plist
            if ([[NSFileManager defaultManager] fileExistsAtPath:[exportPresetsFile path]] == 0)
            {
                [[NSFileManager defaultManager] createFileAtPath:[exportPresetsFile path] contents:nil attributes:nil];
            }

            NSMutableArray *presetsToExport = [[[NSMutableArray alloc] initWithContentsOfURL:exportPresetsFile] autorelease];
            if (presetsToExport == nil)
            {
                presetsToExport = [[NSMutableArray alloc] init];
                // now get and add selected presets to export
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
    NSOpenPanel *panel = [NSOpenPanel openPanel];
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

    // set this for allowed file types, not sure if we should allow xml or not.
    [panel setDirectoryURL:sourceDirectory];
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
    {
        NSURL *importPresetsFile = [panel URL];
        NSURL *importPresetsDirectory = nil;//[importPresetsFile URLByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setURL:importPresetsDirectory forKey:@"LastPresetImportDirectoryURL"];

        // NOTE: here we need to do some sanity checking to verify we do not hose up our presets file
        NSMutableArray *presetsToImport = [[NSMutableArray alloc] initWithContentsOfURL:importPresetsFile];
        // iterate though the new array of presets to import and add them to our presets array
        for (NSMutableDictionary *dict in presetsToImport)
        {
            // make any changes to the incoming preset we see fit
            // make sure the incoming preset is not tagged as default
            dict[@"Default"] = @0;
            // prepend "(imported) to the name of the incoming preset for clarification since it can be changed
            NSString *prependedName = [@"(import) " stringByAppendingString:dict[@"PresetName"]] ;
            dict[@"PresetName"] = prependedName;

            // actually add the new preset to our presets array
            [presetManager addPresetFromDictionary:dict];
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

@end
