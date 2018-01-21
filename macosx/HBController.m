/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBController.h"
#import "HBFocusRingView.h"
#import "HBToolbarBadgedItem.h"
#import "HBQueueController.h"
#import "HBTitleSelectionController.h"
#import "NSWindow+HBAdditions.h"

#import "HBPresetsManager.h"
#import "HBPreset.h"
#import "HBMutablePreset.h"
#import "HBPresetsMenuBuilder.h"

#import "HBSummaryViewController.h"
#import "HBPictureViewController.h"
#import "HBFiltersViewController.h"
#import "HBVideoController.h"
#import "HBAudioController.h"
#import "HBSubtitlesController.h"
#import "HBAdvancedController.h"
#import "HBChapterTitlesController.h"

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"

#import "HBPresetsViewController.h"
#import "HBAddPresetController.h"

@import HandBrakeKit;

@interface HBController () <HBPresetsViewControllerDelegate, HBTitleSelectionDelegate, NSDrawerDelegate, NSDraggingDestination, NSPopoverDelegate>
{
    IBOutlet NSTabView *fMainTabView;

    // Picture controller
    HBPictureViewController * fPictureViewController;
    IBOutlet NSTabViewItem  * fPictureTab;

    // Filters controller
    HBFiltersViewController * fFiltersViewController;
    IBOutlet NSTabViewItem  * fFiltersTab;

    // Video view controller
    HBVideoController       * fVideoController;
    IBOutlet NSTabViewItem  * fVideoTab;

    // Subtitles view controller
    HBSubtitlesController   * fSubtitlesViewController;
    IBOutlet NSTabViewItem  * fSubtitlesTab;

    // Audio view controller
    HBAudioController       * fAudioController;
    IBOutlet NSTabViewItem  * fAudioTab;

    // Chapters view controller
    HBChapterTitlesController    * fChapterTitlesController;
    IBOutlet NSTabViewItem       * fChaptersTitlesTab;

    // Advanced options tab
    HBAdvancedController         * fAdvancedOptions;
    IBOutlet NSTabViewItem       * fAdvancedTab;

    // Picture Preview
    HBPreviewController           * fPreviewController;

    // Queue panel
    HBQueueController            * fQueueController;

    // Source box
    IBOutlet NSProgressIndicator * fScanIndicator;
    IBOutlet NSBox               * fScanHorizontalLine;

    IBOutlet NSTextField         * fSrcDVD2Field;
    IBOutlet NSPopUpButton       * fSrcTitlePopUp;

    // Bottom
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fRipIndicator;
    BOOL                           fRipIndicatorShown;

    // User Preset
    HBPresetsManager             * presetManager;
    HBPresetsViewController      * fPresetsView;
}

@property (nonatomic, strong) IBOutlet NSLayoutConstraint *bottomConstrain;

@property (nonatomic, strong) HBPresetsMenuBuilder *presetsMenuBuilder;
@property (nonatomic, strong) IBOutlet NSPopUpButton *presetsPopup;
@property (nonatomic, strong) IBOutlet NSDrawer *presetsDrawer;

@property (nonatomic, strong) IBOutlet NSToolbarItem *presetsItem;
@property (nonatomic, strong) NSPopover *presetsPopover;

@property (nonatomic, strong) HBSummaryViewController *summaryController;
@property (nonatomic, strong) IBOutlet NSTabViewItem *summaryTab;

@property (nonatomic, weak) IBOutlet HBToolbarBadgedItem *showQueueToolbarItem;

@property (nonatomic, unsafe_unretained) IBOutlet NSView *openTitleView;
@property (nonatomic, readwrite) BOOL scanSpecificTitle;
@property (nonatomic, readwrite) NSInteger scanSpecificTitleIdx;

@property (nonatomic, readwrite, strong) HBTitleSelectionController *titlesSelectionController;

/// The current job.
@property (nonatomic, strong, nullable) HBJob *job;

/// The current selected preset.
@property (nonatomic, strong) HBPreset *currentPreset;

/// The current destination.
@property (nonatomic, strong) NSURL *currentDestination;

/// Whether the job has been edited after a preset was applied.
@property (nonatomic) BOOL edited;

/// The HBCore used for scanning.
@property (nonatomic, strong) HBCore *core;

/// Whether the window is visible or occluded,
/// useful to avoid updating the UI needlessly
@property (nonatomic) BOOL visible;

/// Queue progress info
@property (nonatomic, copy) NSString *progressInfo;
@property (nonatomic) double progress;

@property (nonatomic, readwrite) NSColor *labelColor;

@end

#define WINDOW_HEIGHT_OFFSET_INIT 48
#define WINDOW_HEIGHT_OFFSET      30

@implementation HBController

- (instancetype)initWithQueue:(HBQueueController *)queueController presetsManager:(HBPresetsManager *)manager;
{
    self = [super initWithWindowNibName:@"MainWindow"];
    if (self)
    {
        // Init libhb
        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
        _core = [[HBCore alloc] initWithLogLevel:loggingLevel name:@"ScanCore"];

        // Inits the controllers
        fPreviewController = [[HBPreviewController alloc] init];
        fPreviewController.documentController = self;

        fQueueController = queueController;
        fQueueController.controller = self;

        presetManager = manager;
        _currentPreset = manager.defaultPreset;

        _scanSpecificTitleIdx = 1;

        // Progress
        _progressInfo = @"";
        if (NSAppKitVersionNumber < NSAppKitVersionNumber10_10)
        {
            _visible = YES;
        }

        // Check to see if the last destination has been set, use if so, if not, use Movies
#ifdef __SANDBOX_ENABLED__
        NSData *bookmark = [[NSUserDefaults standardUserDefaults] objectForKey:@"HBLastDestinationDirectoryBookmark"];
        if (bookmark)
        {
            _currentDestination = [HBUtilities URLFromBookmark:bookmark];
        }
#else
        _currentDestination = [[NSUserDefaults standardUserDefaults] URLForKey:@"HBLastDestinationDirectoryURL"];
#endif

        if (!_currentDestination)
        {
            _currentDestination = [NSURL fileURLWithPath:[NSSearchPathForDirectoriesInDomains(NSMoviesDirectory, NSUserDomainMask, YES) firstObject]
                                             isDirectory:YES];
        }

#ifdef __SANDBOX_ENABLED__
        [_currentDestination startAccessingSecurityScopedResource];
#endif
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)windowDidLoad
{
    [self enableUI:NO];

    // Bottom
    fRipIndicator.hidden = YES;
    [self updateProgress];

    // Register HBController's Window as a receiver for files/folders drag & drop operations
    [self.window registerForDraggedTypes:@[NSFilenamesPboardType]];
    [fMainTabView registerForDraggedTypes:@[NSFilenamesPboardType]];

    fPresetsView = [[HBPresetsViewController alloc] initWithPresetManager:presetManager];
    fPresetsView.delegate = self;

    if (NSAppKitVersionNumber < NSAppKitVersionNumber10_10)
    {
        self.presetsDrawer = [[NSDrawer alloc] initWithContentSize:NSMakeSize(240, 550) preferredEdge:NSRectEdgeMaxX];
        self.presetsDrawer.parentWindow = self.window;
        self.presetsDrawer.delegate = self;
        self.presetsDrawer.preferredEdge = NSRectEdgeMaxX;

        // Set up the preset drawer
        self.presetsDrawer.contentView = fPresetsView.view;
        self.presetsDrawer.contentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        NSSize drawerSize = NSSizeFromString([[NSUserDefaults standardUserDefaults]
                                              stringForKey:@"HBDrawerSize"]);
        if (drawerSize.width > 0)
        {
            self.presetsDrawer.contentSize = drawerSize;
        }

        // Show/Hide the Presets drawer upon launch based
        // on user preference DefaultPresetsDrawerShow
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBDefaultPresetsDrawerShow"])
        {
            [self.presetsDrawer open:self];
        }
    }
    else
    {
        fPresetsView.showHeader = YES;

        // Set up the presets popover
        self.presetsPopover = [[NSPopover alloc] init];

        self.presetsPopover.contentViewController = fPresetsView;
        self.presetsPopover.contentSize = NSMakeSize(280, 580);
        self.presetsPopover.animates = YES;

        // AppKit will close the popover when the user interacts with a user interface element outside the popover.
        // note that interacting with menus or panels that become key only when needed will not cause a transient popover to close.
        self.presetsPopover.behavior = NSPopoverBehaviorSemitransient;
        self.presetsPopover.delegate = self;
    }

    // Set up the summary view
    self.summaryController = [[HBSummaryViewController alloc] init];
    self.summaryTab.view = self.summaryController.view;

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

    // setup the picture view controller
    fPictureViewController = [[HBPictureViewController alloc] init];
    [fPictureTab setView:[fPictureViewController view]];

    // setup the filters view controller
    fFiltersViewController = [[HBFiltersViewController alloc] init];
    [fFiltersTab setView:[fFiltersViewController view]];

    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
                                                              forKeyPath:@"values.HBShowAdvancedTab"
                                                                 options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                                                                 context:NULL];


    self.presetsMenuBuilder = [[HBPresetsMenuBuilder alloc] initWithMenu:self.presetsPopup.menu
                                                                  action:@selector(selectPresetFromMenu:)
                                                                    size:[NSFont smallSystemFontSize]
                                                          presetsManager:presetManager];
    [self.presetsMenuBuilder build];

    self.bottomConstrain.constant = -WINDOW_HEIGHT_OFFSET_INIT;

    [self.window recalculateKeyViewLoop];
}

#pragma mark -
#pragma mark Drag & drop handling

- (nullable NSArray<NSURL *> *)fileURLsFromPasteboard:(NSPasteboard *)pasteboard
{
    NSDictionary *options = @{NSPasteboardURLReadingFileURLsOnlyKey: @YES};
    return [pasteboard readObjectsForClasses:@[[NSURL class]] options:options];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    NSArray<NSURL *> *fileURLs = [self fileURLsFromPasteboard:[sender draggingPasteboard]];
    [self.window.contentView setShowFocusRing:YES];
    return fileURLs.count ? NSDragOperationGeneric : NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSArray<NSURL *> *fileURLs = [self fileURLsFromPasteboard:[sender draggingPasteboard]];

    if (fileURLs.count)
    {
        [self openURL:fileURLs.firstObject];
    }

    [self.window.contentView setShowFocusRing:NO];
    return YES;
}

- (void)draggingExited:(nullable id <NSDraggingInfo>)sender
{
    [self.window.contentView setShowFocusRing:NO];
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"values.HBShowAdvancedTab"])
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBShowAdvancedTab"])
        {
            if (![[fMainTabView tabViewItems] containsObject:fAdvancedTab])
            {
                [fMainTabView insertTabViewItem:fAdvancedTab atIndex:5];
            }
        }
        else
        {
            [fMainTabView removeTabViewItem:fAdvancedTab];
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

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"scanSpecificTitleIdx"])
    {
        [self setValue:@0 forKey:key];
    }
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
            [toolbarItem setLabel: NSLocalizedString(@"Cancel Scan", nil)];
            [toolbarItem setPaletteLabel: NSLocalizedString(@"Cancel Scanning", nil)];
            [toolbarItem setToolTip: NSLocalizedString(@"Cancel Scanning Source", nil)];
            return YES;
        }

        if (action == @selector(rip:) || action == @selector(addToQueue:))
            return NO;
    }
    else
    {
        if (action == @selector(browseSources:))
        {
            [toolbarItem setImage:[NSImage imageNamed:@"source"]];
            [toolbarItem setLabel:NSLocalizedString(@"Open Source", nil)];
            [toolbarItem setPaletteLabel:NSLocalizedString(@"Open Source", nil)];
            [toolbarItem setToolTip:NSLocalizedString(@"Open Source", nil)];
            return YES;
        }
    }

    HBState queueState = fQueueController.core.state;

    if (queueState == HBStateScanning || queueState == HBStateWorking || queueState == HBStateSearching || queueState == HBStateMuxing)
    {
        if (action == @selector(rip:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"stopencode"]];
            [toolbarItem setLabel: NSLocalizedString(@"Stop", nil)];
            [toolbarItem setPaletteLabel: NSLocalizedString(@"Stop", nil)];
            [toolbarItem setToolTip: NSLocalizedString(@"Stop Encoding", nil)];
            return YES;
        }
        if (action == @selector(pause:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"pauseencode"]];
            [toolbarItem setLabel: NSLocalizedString(@"Pause", nil)];
            [toolbarItem setPaletteLabel: NSLocalizedString(@"Pause Encoding", nil)];
            [toolbarItem setToolTip: NSLocalizedString(@"Pause Encoding", nil)];
            return YES;
        }
    }
    else if (queueState == HBStatePaused)
    {
        if (action == @selector(pause:))
        {
            [toolbarItem setImage: [NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: NSLocalizedString(@"Resume", nil)];
            [toolbarItem setPaletteLabel: NSLocalizedString(@"Resume Encoding", nil)];
            [toolbarItem setToolTip: NSLocalizedString(@"Resume Encoding", nil)];
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
                [toolbarItem setLabel: NSLocalizedString(@"Start Queue", nil)];
            else
                [toolbarItem setLabel: NSLocalizedString(@"Start", nil)];
            [toolbarItem setPaletteLabel: NSLocalizedString(@"Start Encoding", nil)];
            [toolbarItem setToolTip: NSLocalizedString(@"Start Encoding", nil)];
        }

        if (action == @selector(rip:))
        {
            return (self.job != nil || fQueueController.pendingItemsCount > 0);
        }

        if (action == @selector(pause:))
        {
            return NO;
        }
    }

    if (action == @selector(addToQueue:))
    {
        return (self.job != nil);
    }

    return YES;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(addToQueue:) || action == @selector(addAllTitlesToQueue:) ||
        action == @selector(addTitlesToQueue:) || action == @selector(showAddPresetPanel:))
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
        if ([menuItem.representedObject isEqualTo:self.currentPreset] && self.edited == NO)
        {
            menuItem.state = NSOnState;
        }
        else
        {
            menuItem.state = NSOffState;
        }
        return (self.job != nil);
    }
    if (action == @selector(exportPreset:))
    {
        return [fPresetsView validateUserInterfaceItem:menuItem];
    }

    return YES;
}

#pragma mark - Get New Source

- (void)launchAction
{
    if (self.core.state != HBStateScanning && !self.job)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBShowOpenPanelAtLaunch"])
        {
            [self browseSources:nil];
        }
    }
}

- (NSModalResponse)runCopyProtectionAlert
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"Copy-Protected sources are not supported.", nil)];
    [alert setInformativeText:NSLocalizedString(@"Please note that HandBrake does not support the removal of copy-protection from DVD Discs. You can if you wish use any other 3rd party software for this function.", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Attempt Scan Anyway", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];

    [NSApp requestUserAttention:NSCriticalRequest];

    return [alert runModal];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)scanURL:(NSURL *)fileURL titleIndex:(NSUInteger)index completionHandler:(void(^)(NSArray<HBTitle *> *titles))completionHandler
{
    // Save the current settings
    if (self.job)
    {
        self.currentPreset = [self createPresetFromCurrentSettings];
    }

    self.job = nil;
    [fSrcTitlePopUp removeAllItems];
    self.window.representedURL = nil;
    self.window.title = NSLocalizedString(@"HandBrake", nil);

    NSURL *mediaURL = [HBUtilities mediaURLFromURL:fileURL];

    NSError *outError = NULL;
    BOOL suppressWarning = [[NSUserDefaults standardUserDefaults] boolForKey:@"suppressCopyProtectionAlert"];

    // Check if we can scan the source and if there is any warning.
    BOOL canScan = [self.core canScan:mediaURL error:&outError];

    // Notify the user that we don't support removal of copy proteciton.
    if (canScan && [outError code] == 101 && !suppressWarning)
    {
        // Only show the user this warning once. They may be using a solution we don't know about. Notifying them each time is annoying.
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"suppressCopyProtectionAlert"];

        if ([self runCopyProtectionAlert] == NSAlertFirstButtonReturn)
        {
            // User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails
            [HBUtilities writeToActivityLog:"User overrode copy-protection warning - trying to open physical dvd without decryption"];
        }
        else
        {
            // User chose to cancel the scan
            [HBUtilities writeToActivityLog:"Cannot open physical dvd, scan canceled"];
            canScan = NO;
        }
    }

    if (canScan)
    {
        int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
        int min_title_duration_seconds = [[[NSUserDefaults standardUserDefaults] objectForKey:@"MinTitleScanSeconds"] intValue];

        [self.core scanURL:mediaURL
                titleIndex:index
                  previews:hb_num_previews minDuration:min_title_duration_seconds
           progressHandler:^(HBState state, HBProgress progress, NSString *info)
         {
             fSrcDVD2Field.stringValue = info;
             fScanIndicator.hidden = NO;
             fScanHorizontalLine.hidden = YES;
             fScanIndicator.doubleValue = progress.percent;
         }
         completionHandler:^(HBCoreResult result)
         {
             fScanHorizontalLine.hidden = NO;
             fScanIndicator.hidden = YES;
             fScanIndicator.indeterminate = NO;
             fScanIndicator.doubleValue = 0.0;

             if (result == HBCoreResultDone)
             {
                 for (HBTitle *title in self.core.titles)
                 {
                     [fSrcTitlePopUp addItemWithTitle:title.description];
                 }
                 self.window.representedURL = mediaURL;
                 self.window.title = mediaURL.lastPathComponent;
             }
             else
             {
                 // We display a message if a valid source was not chosen
                 fSrcDVD2Field.stringValue = NSLocalizedString(@"No Valid Source Found", @"");
             }

             // Set the last searched source directory in the prefs here
             if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:mediaURL.URLByDeletingLastPathComponent.path])
             {
                 [[NSUserDefaults standardUserDefaults] setURL:mediaURL.URLByDeletingLastPathComponent.URLByDeletingLastPathComponent forKey:@"HBLastSourceDirectoryURL"];
             }
             else
             {
                 [[NSUserDefaults standardUserDefaults] setURL:mediaURL.URLByDeletingLastPathComponent forKey:@"HBLastSourceDirectoryURL"];
             }

             completionHandler(self.core.titles);
             [self.window.toolbar validateVisibleItems];
         }];
    }
    else
    {
        completionHandler(@[]);
    }
}

- (void)openURL:(NSURL *)fileURL titleIndex:(NSUInteger)index
{
    [self scanURL:fileURL titleIndex:index completionHandler:^(NSArray<HBTitle *> *titles)
    {
        if (titles.count)
        {
            [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:fileURL];

            HBTitle *featuredTitle = titles.firstObject;
            for (HBTitle *title in titles)
            {
                if (title.isFeatured)
                {
                    featuredTitle = title;
                }
            }

            HBJob *job = [self jobFromTitle:featuredTitle];
            self.job = job;
        }
    }];
}

- (void)openURL:(NSURL *)fileURL
{
    if (self.core.state != HBStateScanning)
    {
        [self openURL:fileURL titleIndex:0];
    }
}

/**
 * Rescans the a job back into the main window
 */
- (void)openJob:(HBJob *)job completionHandler:(void (^)(BOOL result))handler
{
    if (self.core.state != HBStateScanning)
    {
        [self scanURL:job.fileURL titleIndex:job.titleIdx completionHandler:^(NSArray<HBTitle *> *titles)
        {
            if (titles.count)
            {
                // If the scan was cached, reselect
                // the original title
                for (HBTitle *title in titles)
                {
                    if (title.index == job.titleIdx)
                    {
                        job.title = title;
                        break;
                    }
                }

                // Else just one title or a title specific rescan
                // select the first title
                if (!job.title)
                {
                    job.title = titles.firstObject;
                }
                self.job = job;

                handler(YES);
            }
            else
            {
                handler(NO);
            }
        }];
    }
    else
    {
        handler(NO);
    }
}

- (HBJob *)jobFromTitle:(HBTitle *)title
{
    // If there is already a title load, save the current settings to a preset
    if (self.job)
    {
        self.currentPreset = [self createPresetFromCurrentSettings];
    }

    HBJob *job = [[HBJob alloc] initWithTitle:title andPreset:self.currentPreset];
    job.outputURL = self.currentDestination;

    // If the source is not a stream, and autonaming is disabled,
    // keep the existing file name.
    if (self.job.outputFileName.length == 0 || title.isStream || [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
    {
        job.outputFileName = [HBUtilities defaultNameForJob:job];
    }
    else
    {
        job.outputFileName = self.job.outputFileName;
    }

    return job;
}

- (void)removeJobObservers
{
    if (self.job)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBContainerChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBFiltersChangedNotification object:_job.filters];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBVideoChangedNotification object:_job.video];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBAudioEncoderChangedNotification object:_job.audio];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBChaptersChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBRangeChangedNotification object:_job.range];
    }
}

/**
 *  Observe the job settings changes.
 *  This is used to update the file name and extention
 *  and the custom preset string.
 */
- (void)addJobObservers
{
    if (self.job)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(formatChanged:) name:HBContainerChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(customSettingUsed) name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(customSettingUsed) name:HBFiltersChangedNotification object:_job.filters];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(customSettingUsed) name:HBVideoChangedNotification object:_job.video];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBAudioEncoderChangedNotification object:_job.audio];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBChaptersChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(chapterPopUpChanged:) name:HBRangeChangedNotification object:_job.range];
    }
}

- (void)setJob:(HBJob *)job
{
    [self removeJobObservers];

    // Clear the undo manager
    [_job.undo removeAllActions];
    _job.undo = nil;

    // Retain the new job
    _job = job;

    job.undo = self.window.undoManager;

    // Set the jobs info to the view controllers
    self.summaryController.job = job;
    fPictureViewController.picture = job.picture;
    fFiltersViewController.filters = job.filters;
    fVideoController.video = job.video;
    fAudioController.audio = job.audio;
    fSubtitlesViewController.subtitles = job.subtitles;
    fChapterTitlesController.job = job;

    if (job)
    {
        HBPreviewGenerator *generator = [[HBPreviewGenerator alloc] initWithCore:self.core job:job];
        fPreviewController.generator = generator;
        self.summaryController.generator = generator;

        HBTitle *title = job.title;

        // Update the title selection popup.
        [fSrcTitlePopUp selectItemWithTitle:title.description];

        // Grok the output file name from title.name upon title change
        if (title.isStream && self.core.titles.count > 1)
        {
            // Change the source to read out the parent folder also
            fSrcDVD2Field.stringValue = [NSString stringWithFormat:@"%@/%@, %@", title.url.URLByDeletingLastPathComponent.lastPathComponent, title.name, title.shortFormatDescription];
        }
        else
        {
            fSrcDVD2Field.stringValue = [NSString stringWithFormat:@"%@, %@", title.name, title.shortFormatDescription];
        }
    }
    else
    {
        fPreviewController.generator = nil;
        self.summaryController.generator = nil;
    }
    fPreviewController.picture = job.picture;

    [self enableUI:(job != nil)];

    [self addJobObservers];
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
        sourceDirectory = [NSURL fileURLWithPath:[NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) firstObject]
                                                       isDirectory:YES];
	}

    [panel setDirectoryURL:sourceDirectory];
    [panel setAccessoryView:self.openTitleView];

    if ([panel respondsToSelector:@selector(isAccessoryViewDisclosed)])
    {
        panel.accessoryViewDisclosed = YES;
    }

    [panel beginSheetModalForWindow:self.window completionHandler: ^(NSInteger result)
    {
         if (result == NSFileHandlingPanelOKButton)
         {
             NSInteger titleIdx = self.scanSpecificTitle ? self.scanSpecificTitleIdx : 0;
             [self openURL:panel.URL titleIndex:titleIdx];
         }
     }];
}

#pragma mark - GUI Controls Changed Methods

- (IBAction)browseDestination:(id)sender
{
    // Open a panel to let the user choose and update the text field
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = NO;
    panel.canChooseDirectories = YES;
    panel.canCreateDirectories = YES;
    panel.prompt = NSLocalizedString(@"Choose", nil);

    if (self.job.outputURL)
    {
        panel.directoryURL = self.job.outputURL;
    }

    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
     {
         if (result == NSFileHandlingPanelOKButton)
         {
             self.job.outputURL = panel.URL;
             self.currentDestination = panel.URL;

             // Save this path to the prefs so that on next browse destination window it opens there
             [[NSUserDefaults standardUserDefaults] setObject:[HBUtilities bookmarkFromURL:panel.URL]
                                                       forKey:@"HBLastDestinationDirectoryBookmark"];
             [[NSUserDefaults standardUserDefaults] setURL:panel.URL
                                                    forKey:@"HBLastDestinationDirectoryURL"];

         }
     }];
}

- (IBAction)titlePopUpChanged:(NSPopUpButton *)sender
{
    HBTitle *title = self.core.titles[sender.indexOfSelectedItem];
    HBJob *job = [self jobFromTitle:title];
    self.job = job;
}

- (void)chapterPopUpChanged:(NSNotification *)notification
{
    // We're changing the chapter range - we may need to flip the m4v/mp4 extension
    if (self.job.container & 0x030000 /*HB_MUX_MASK_MP4*/)
    {
        [self updateFileExtension:notification];
    }

    // If Auto Naming is on it might need to be update if it includes the chapters range
    [self updateFileName];
}

- (void)formatChanged:(NSNotification *)notification
{
    [self updateFileExtension:notification];
    [self customSettingUsed];
}

- (void)updateFileName
{
    [self updateFileExtension:nil];

    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"] && self.job)
    {
        // Generate a new file name
        NSString *fileName = [HBUtilities automaticNameForJob:self.job];

        // Swap the old one with the new one
        self.job.outputFileName = [NSString stringWithFormat:@"%@.%@", fileName, self.job.outputFileName.pathExtension];
    }
}

- (void)updateFileExtension:(NSNotification *)notification
{
    if (self.job)
    {
        NSString *extension = [HBUtilities automaticExtForJob:self.job];
        if (![extension isEqualTo:self.job.outputFileName.pathExtension])
        {
            self.job.outputFileName = [[self.job.outputFileName stringByDeletingPathExtension] stringByAppendingPathExtension:extension];
        }
    }
}

/**
 * Method to determine if we should change the UI
 * To reflect whether or not a Preset is being used or if
 * the user is using "Custom" settings by determining the sender
 */
- (void)customSettingUsed
{
    // Update the preset and file name only if we are not
    // undoing or redoing, because if so it's already stored
    // in the undo manager.
    NSUndoManager *undo = self.window.undoManager;
    if (!(undo.isUndoing || undo.isRedoing))
    {
        // Change UI to show "Custom" settings are being used
        if (![self.job.presetName hasSuffix:NSLocalizedString(@"(Modified)", nil)])
        {
            self.job.presetName = [NSString stringWithFormat:@"%@ %@", self.job.presetName, NSLocalizedString(@"(Modified)", nil)];
        }
        self.edited = YES;
        [self updateFileName];
    }
}

#pragma mark - Queue progress

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
    if ([self.window occlusionState] & NSWindowOcclusionStateVisible)
    {
        self.visible = YES;
        [self updateProgress];
    }
    else
    {
        self.visible = NO;
    }
}

- (void)updateProgress
{
    fStatusField.stringValue = self.progressInfo;
    fRipIndicator.doubleValue = self.progress;
}

- (void)setQueueState:(NSUInteger)count
{
    self.showQueueToolbarItem.badgeValue = count ? @(count).stringValue : nil;
}

- (void)setQueueInfo:(NSString *)info progress:(double)progress hidden:(BOOL)hidden
{
    self.progressInfo = info;
    self.progress = progress;

    if (_visible)
    {
        [self updateProgress];
    }

    if (hidden)
    {
        if (fRipIndicatorShown)
        {
            self.bottomConstrain.animator.constant = -WINDOW_HEIGHT_OFFSET;
            fRipIndicator.hidden = YES;
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
            self.bottomConstrain.animator.constant = 0;
            fRipIndicatorShown = YES;
            fRipIndicator.hidden = NO;

            // Refresh the toolbar buttons
            [self.window.toolbar validateVisibleItems];
        }
    }
}

#pragma mark - Job Handling

/**
 Check if the job destination if a valid one,
 if so, call the didEndSelector
 Note: rework this to use a block in the future

 @param job the job
 @param didEndSelector the selector to call if the check is successful
 */
- (void)runDestinationAlerts:(HBJob *)job didEndSelector:(SEL)didEndSelector
{
    if ([[NSFileManager defaultManager] fileExistsAtPath:job.outputURL.path] == 0)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Warning!", @"")];
        [alert setInformativeText:NSLocalizedString(@"This is not a valid destination directory!", @"")];
        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:didEndSelector contextInfo:NULL];
    }
    else if ([job.fileURL isEqual:job.completeOutputURL])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"")];
        [alert setInformativeText:NSLocalizedString(@"The destination is the same as the source, you can not overwrite your source file!", @"")];
        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:didEndSelector contextInfo:NULL];
    }
    else if ([[NSFileManager defaultManager] fileExistsAtPath:job.completeOutputURL.path])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @""), job.completeOutputURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"")];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:didEndSelector contextInfo:NULL];
    }
    else if ([fQueueController jobExistAtURL:job.completeOutputURL])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"There is already a queue item for this destination.", @"")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @""), job.completeOutputURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"")];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:didEndSelector contextInfo:NULL];
    }
    else
    {
        NSInteger returnCode = NSAlertSecondButtonReturn;
        NSMethodSignature *methodSignature = [self methodSignatureForSelector:didEndSelector];
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:methodSignature];
        [invocation setTarget:self];
        [invocation setSelector:didEndSelector];
        [invocation setArgument:&returnCode atIndex:3];
        [invocation invoke];
    }
}

/**
 *  Actually adds a job to the queue
 */
- (void)doAddToQueue
{
    [fQueueController addJob:[self.job copy]];
}

/**
 * Puts up an alert before ultimately calling doAddToQueue
 */
- (IBAction)addToQueue:(id)sender
{
    if ([self.window HB_endEditing])
    {
        [self runDestinationAlerts:self.job
                    didEndSelector:@selector(overwriteAddToQueueAlertDone:returnCode:contextInfo:)];
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
    if (fQueueController.core.state == HBStateWorking || fQueueController.core.state == HBStatePaused || fQueueController.core.state == HBStateSearching)
	{
        // Displays an alert asking user if the want to cancel encoding of current job.
        [fQueueController cancelRip:self];
    }
    // If there are pending jobs in the queue, then this is a rip the queue
    else if (fQueueController.pendingItemsCount > 0)
    {
        [fQueueController rip:self];
    }
    else
    {
        if ([self.window HB_endEditing])
        {
            [self runDestinationAlerts:self.job
                        didEndSelector:@selector(overWriteAlertDone:returnCode:contextInfo:)];
        }
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
    [fQueueController togglePauseResume:sender];
}

#pragma mark -
#pragma mark Batch Queue Titles Methods

- (IBAction)addTitlesToQueue:(id)sender
{
    [self.window HB_endEditing];

    self.titlesSelectionController = [[HBTitleSelectionController alloc] initWithTitles:self.core.titles
                                                                             presetName:self.job.presetName
                                                                               delegate:self];

    [NSApp beginSheet:self.titlesSelectionController.window
       modalForWindow:self.window
        modalDelegate:nil
       didEndSelector:NULL
          contextInfo:NULL];
}

- (void)didSelectTitles:(NSArray<HBTitle *> *)titles
{
    [self.titlesSelectionController.window orderOut:nil];
    [NSApp endSheet:self.titlesSelectionController.window];

    [self doAddTitlesToQueue:titles];
}

- (void)doAddTitlesToQueue:(NSArray<HBTitle *> *)titles;
{
    NSMutableArray<HBJob *> *jobs = [[NSMutableArray alloc] init];
    BOOL fileExists = NO;
    BOOL fileOverwritesSource = NO;

    // Get the preset from the loaded job.
    HBPreset *preset = [self createPresetFromCurrentSettings];

    for (HBTitle *title in titles)
    {
        HBJob *job = [[HBJob alloc] initWithTitle:title andPreset:preset];
        job.outputURL = self.currentDestination;
        job.outputFileName = [HBUtilities defaultNameForJob:job];
        job.title = nil;
        [jobs addObject:job];
    }

    NSMutableSet<NSURL *> *destinations = [[NSMutableSet alloc] init];
    for (HBJob *job in jobs)
    {
        if ([destinations containsObject:job.completeOutputURL])
        {
            fileExists = YES;
            break;
        }
        else
        {
            [destinations addObject:job.completeOutputURL];
        }

        if ([[NSFileManager defaultManager] fileExistsAtPath:job.completeOutputURL.path] || [fQueueController jobExistAtURL:job.completeOutputURL])
        {
            fileExists = YES;
            break;
        }
    }

    for (HBJob *job in jobs)
    {
        if ([job.fileURL isEqual:job.completeOutputURL]) {
            fileOverwritesSource = YES;
            break;
        }
    }

    if (fileOverwritesSource)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"")];
        [alert setInformativeText:NSLocalizedString(@"The destination is the same as the source, you can not overwrite your source file!", @"")];
        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(overwriteAddTitlesToQueueAlertDone:returnCode:contextInfo:) contextInfo:NULL];
    }
    else if (fileExists)
    {
        // File exist, warn user
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"File already exists.", nil)];
        [alert setInformativeText:NSLocalizedString(@"One or more file already exists. Do you want to overwrite?", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:self.window modalDelegate:self didEndSelector:@selector(overwriteAddTitlesToQueueAlertDone:returnCode:contextInfo:) contextInfo:(void *)CFBridgingRetain(jobs)];
    }
    else
    {
        [fQueueController addJobsFromArray:jobs];
    }
}

- (void)overwriteAddTitlesToQueueAlertDone:(NSAlert *)alert
                                returnCode:(NSInteger)returnCode
                               contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        NSArray *jobs = CFBridgingRelease(contextInfo);
        [fQueueController addJobsFromArray:jobs];
    }
}

- (IBAction)addAllTitlesToQueue:(id)sender
{
    [self doAddTitlesToQueue:self.core.titles];
}

#pragma mark - Picture

- (IBAction)showPreviewWindow:(id)sender
{
	[fPreviewController showWindow:sender];
}

- (IBAction)showTabView:(id)sender
{
    NSInteger tag = [sender tag];
    [fMainTabView selectTabViewItemAtIndex:tag];
}

#pragma mark - Presets View Controller Delegate

- (void)selectionDidChange
{
    if (fPresetsView.selectedPreset != self.currentPreset || self.edited)
    {
        [self applyPreset:fPresetsView.selectedPreset];
    }
}

#pragma mark -  Presets

- (BOOL)popoverShouldDetach:(NSPopover *)popover
{
    if (popover == self.presetsPopover) {
        return YES;
    }

    return NO;
}

- (IBAction)togglePresets:(id)sender
{
    if (self.presetsPopover)
    {
        if (!self.presetsPopover.isShown)
        {
            NSView *target = [sender isKindOfClass:[NSView class]] ? (NSView *)sender : self.presetsItem.view.window ? self.presetsItem.view : self.window.contentView;
            [self.presetsPopover showRelativeToRect:target.bounds ofView:target preferredEdge:NSMaxYEdge];
        }
        else
        {
            [self.presetsPopover close];
        }
    }
    else
    {
        if (self.presetsDrawer.state == NSDrawerClosedState)
        {
            [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"HBDefaultPresetsDrawerShow"];
        }
        else
        {
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"HBDefaultPresetsDrawerShow"];
        }

        [self.presetsDrawer toggle:self];
    }
}

- (void)setCurrentPreset:(HBPreset *)currentPreset
{
    NSParameterAssert(currentPreset);

    if (currentPreset != _currentPreset)
    {
        NSUndoManager *undo = self.window.undoManager;
        [[undo prepareWithInvocationTarget:self] setCurrentPreset:_currentPreset];

        _currentPreset = currentPreset;
    }
}

- (void)setEdited:(BOOL)edited
{
    if (edited != _edited)
    {
        NSUndoManager *undo = self.window.undoManager;
        [[undo prepareWithInvocationTarget:self] setEdited:_edited];

        _edited = edited;
    }
}

- (void)reloadPreset:(id)sender;
{
    // Reload the currently selected preset if it is selected.
    if (self.currentPreset != NULL){
        [self applyPreset:self.currentPreset];
    }
}

- (void)applyPreset:(HBPreset *)preset
{
    NSParameterAssert(preset);

    if (self.job)
    {
        self.currentPreset = preset;
        self.edited = NO;

        // Remove the job observer so we don't update the file name
        // too many times while the preset is being applied
        [self removeJobObservers];

        // Apply the preset to the current job
        [self.job applyPreset:self.currentPreset];

        // If Auto Naming is on, update the destination
        [self updateFileName];

        [self addJobObservers];
    }
}

- (IBAction)showAddPresetPanel:(id)sender
{
    [self.window HB_endEditing];

    BOOL defaultToCustom = ((self.job.picture.width + self.job.picture.cropRight + self.job.picture.cropLeft) < self.job.picture.sourceWidth) ||
                           ((self.job.picture.height + self.job.picture.cropTop + self.job.picture.cropBottom) < self.job.picture.sourceHeight);

    // Show the add panel
    HBAddPresetController *addPresetController = [[HBAddPresetController alloc] initWithPreset:[self createPresetFromCurrentSettings]
                                                                                 presetManager:presetManager
                                                                                   customWidth:self.job.picture.width
                                                                                  customHeight:self.job.picture.height
                                                                               defaultToCustom:defaultToCustom];

    [NSApp beginSheet:addPresetController.window modalForWindow:self.window modalDelegate:self didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:) contextInfo:(void *)CFBridgingRetain(addPresetController)];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    __unused HBAddPresetController *addPresetController = (HBAddPresetController *)CFBridgingRelease(contextInfo);
}

- (HBPreset *)createPresetFromCurrentSettings
{
    HBMutablePreset *preset = [self.currentPreset mutableCopy];

	// Set whether or not this is a user preset or factory 0 is factory, 1 is user
    preset[@"Type"] = @1;
    preset[@"Default"] = @NO;

    [self.job writeToPreset:preset];

    return [preset copy];
}

#pragma mark -
#pragma mark Import Export Preset(s)

- (IBAction)exportPreset:(id)sender
{
    [fPresetsView exportPreset:sender];
}

- (IBAction)importPreset:(id)sender
{
    [fPresetsView importPreset:sender];
}

#pragma mark -
#pragma mark Preset Menu

- (IBAction)selectDefaultPreset:(id)sender
{
    [self applyPreset:presetManager.defaultPreset];
    fPresetsView.selectedPreset = presetManager.defaultPreset;
}

- (IBAction)insertCategory:(id)sender
{
    [fPresetsView insertCategory:sender];
}

- (IBAction)selectPresetFromMenu:(id)sender
{
    // Retrieve the preset stored in the NSMenuItem
    HBPreset *preset = [sender representedObject];

    [self applyPreset:preset];
    fPresetsView.selectedPreset = preset;
}

@end
