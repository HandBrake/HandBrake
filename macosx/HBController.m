/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBController.h"
#import "HBAppDelegate.h"
#import "HBFocusRingView.h"
#import "HBControllerToolbarDelegate.h"
#import "HBQueueController.h"
#import "HBTitleSelectionController.h"
#import "NSWindow+HBAdditions.h"
#import "NSToolbar+HBAdditions.h"

#import "HBQueue.h"
#import "HBQueueWorker.h"

#import "HBPresetsMenuBuilder.h"

#import "HBSummaryViewController.h"
#import "HBPictureViewController.h"
#import "HBFiltersViewController.h"
#import "HBVideoController.h"
#import "HBAudioController.h"
#import "HBSubtitlesController.h"
#import "HBChapterTitlesController.h"

#import "HBPreviewController.h"
#import "HBPreviewGenerator.h"

#import "HBPresetsViewController.h"
#import "HBAddPresetController.h"
#import "HBRenamePresetController.h"

#import "HBAutoNamer.h"
#import "HBJob+HBAdditions.h"
#import "HBAttributedStringAdditions.h"

#import "HBPreferencesKeys.h"

static void *HBControllerScanCoreContext = &HBControllerScanCoreContext;
static void *HBControllerLogLevelContext = &HBControllerLogLevelContext;

@interface HBController () <HBPresetsViewControllerDelegate, HBTitleSelectionDelegate, NSMenuItemValidation, NSDraggingDestination, NSPopoverDelegate, NSPathControlDelegate>

@property (nonatomic, readonly, strong) HBCore *core;
@property (nonatomic, readonly, strong) HBAppDelegate *delegate;

@property (nonatomic, strong) NSArray<HBSecurityAccessToken *> *fileTokens;
@property (nonatomic, strong) NSArray<NSURL *> *subsURLs;
@property (nonatomic, strong) NSURL *destinationFolderURL;
@property (nonatomic, strong) HBSecurityAccessToken *destinationFolderToken;


@property (nonatomic, weak) IBOutlet NSTextField *sourceLabel;
@property (nonatomic, weak) IBOutlet NSPopUpButton *titlePopUp;
@property (nonatomic, weak) IBOutlet NSPathControl *destinationPathControl;

@property (nonatomic, strong) IBOutlet NSLayoutConstraint *bottomConstrain;
@property (nonatomic, readwrite) NSColor *labelColor;

/// Whether the window is visible or occluded,
/// useful to avoid updating the UI needlessly
@property (nonatomic) BOOL visible;
@property (nonatomic) BOOL suppressCopyProtectionWarning;

#pragma mark - Scan UI

@property (nonatomic, weak) IBOutlet NSProgressIndicator *scanIndicator;
@property (nonatomic, weak) IBOutlet NSBox *scanHorizontalLine;

#pragma mark - Controllers

@property (nonatomic, readonly, strong) HBSummaryViewController *summaryController;
@property (nonatomic, readonly, strong) HBPictureViewController *pictureViewController;
@property (nonatomic, readonly, strong) HBFiltersViewController *filtersViewController;
@property (nonatomic, readonly, strong) HBVideoController *videoController;
@property (nonatomic, readonly, strong) HBAudioController *audioController;
@property (nonatomic, readonly, strong) HBSubtitlesController *subtitlesViewController;
@property (nonatomic, readonly, strong) HBChapterTitlesController *chapterTitlesController;

@property (nonatomic, strong) IBOutlet NSTabView *mainTabView;
@property (nonatomic, strong) IBOutlet NSTabViewItem *summaryTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *pictureTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *filtersTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *videoTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *audioTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *subtitlesTab;
@property (nonatomic, strong) IBOutlet NSTabViewItem *chaptersTab;

@property (nonatomic, readonly, strong) HBPreviewController *previewController;
@property (nonatomic, strong) HBTitleSelectionController *titlesSelectionController;

#pragma mark - Presets

@property (nonatomic, readonly, strong) HBPresetsManager *presetManager;
@property (nonatomic, readonly, strong) HBPresetsMenuBuilder *presetsMenuBuilder;
@property (nonatomic, readonly, strong) HBPresetsViewController *presetView;

@property (nonatomic, readonly, strong) NSPopover *presetsPopover;
@property (nonatomic, strong) IBOutlet NSPopUpButton *presetsPopup;

@property (nonatomic, nullable, strong) HBPreset *selectedPreset;
@property (nonatomic, strong) HBPreset *currentPreset;

#pragma mark - Open panel accessory view

@property (nonatomic, weak) IBOutlet NSView *openTitleView;
@property (nonatomic) BOOL scanSpecificTitle;
@property (nonatomic) NSInteger scanSpecificTitleIdx;

#pragma mark - Job

@property (nonatomic, nullable) HBJob *job;
@property (nonatomic, nullable) HBAutoNamer *autoNamer;

#pragma mark - Queue

@property (nonatomic, readonly, weak) HBQueue *queue;
@property (nonatomic) id observerToken;

#define WINDOW_HEIGHT_OFFSET 30
@property (nonatomic) IBOutlet NSTextField *statusField;
@property (nonatomic) IBOutlet NSTextField *progressField;
@property (nonatomic, copy) NSString *progress;

#pragma mark - Toolbar

@property (nonatomic) HBControllerToolbarDelegate *toolbarDelegate;

@end

@interface HBController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
- (void)_touchBar_updateButtonsStateForScanCore:(HBState)state;
- (void)_touchBar_updateQueueButtonsState;
- (void)_touchBar_validateUserInterfaceItems;
@end

@implementation HBController

- (instancetype)initWithDelegate:(HBAppDelegate *)delegate queue:(HBQueue *)queue presetsManager:(HBPresetsManager *)manager
{
    self = [super initWithWindowNibName:@"MainWindow"];
    if (self)
    {
        // Init libhb
        NSInteger loggingLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
        _core = [[HBCore alloc] initWithLogLevel:loggingLevel name:@"ScanCore"];

        // Inits the controllers
        _previewController = [[HBPreviewController alloc] init];
        _previewController.documentController = self;

        _delegate = delegate;
        _queue = queue;

        _presetManager = manager;
        _selectedPreset = manager.defaultPreset;
        _currentPreset = manager.defaultPreset;

        _scanSpecificTitleIdx = 1;
        _progress = @"";

        // Check to see if the last destination has been set, use if so, if not, use Movies
#ifdef __SANDBOX_ENABLED__
        NSData *bookmark = [NSUserDefaults.standardUserDefaults objectForKey:HBLastDestinationDirectoryBookmark];
        if (bookmark)
        {
            _destinationFolderURL = [HBUtilities URLFromBookmark:bookmark];
        }
#else
        _destinationFolderURL = [NSUserDefaults.standardUserDefaults URLForKey:HBLastDestinationDirectoryURL];
#endif
        if (!_destinationFolderURL || [_destinationFolderURL checkResourceIsReachableAndReturnError:NULL] == NO)
        {
            _destinationFolderURL = HBUtilities.defaultDestinationFolderURL;
        }

#ifdef __SANDBOX_ENABLED__
        _destinationFolderToken = [HBSecurityAccessToken tokenWithObject:_destinationFolderURL];
#endif
    }

    return self;
}

- (void)windowDidLoad
{
    self.window.tabbingMode = NSWindowTabbingModeDisallowed;

    if (@available (macOS 11, *))
    {
        self.window.toolbarStyle = NSWindowToolbarStyleExpanded;
    }

    self.toolbarDelegate = [[HBControllerToolbarDelegate alloc] initWithTarget:self];

    NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"HBMainWindowToolbar2"];
    toolbar.delegate = self.toolbarDelegate;
    toolbar.allowsUserCustomization = YES;
    toolbar.autosavesConfiguration = YES;
    toolbar.displayMode = NSToolbarDisplayModeIconAndLabel;
    self.window.toolbar = toolbar;

    [self enableUI:NO];

    // Bottom
    self.statusField.stringValue = @"";
    self.progressField.font = [NSFont monospacedDigitSystemFontOfSize:NSFont.smallSystemFontSize weight:NSFontWeightRegular];
    [self updateProgress];

    // Register HBController's Window as a receiver for files/folders drag & drop operations
    [self.window registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
    [self.mainTabView registerForDraggedTypes:@[NSPasteboardTypeFileURL]];

    _presetView = [[HBPresetsViewController alloc] initWithPresetManager:self.presetManager];
    _presetView.delegate = self;

    // Set up the presets popover
    _presetsPopover = [[NSPopover alloc] init];

    _presetsPopover.contentViewController = self.presetView;
    _presetsPopover.contentSize = NSMakeSize(300, 580);
    _presetsPopover.animates = YES;

    // AppKit will close the popover when the user interacts with a user interface element outside the popover.
    // note that interacting with menus or panels that become key only when needed will not cause a transient popover to close.
    _presetsPopover.behavior = NSPopoverBehaviorSemitransient;
    _presetsPopover.delegate = self;

    [self.presetView view];

    // Setup the view controllers
    _summaryController = [[HBSummaryViewController alloc] init];
    self.summaryTab.view = self.summaryController.view;

    _pictureViewController = [[HBPictureViewController alloc] init];
    self.pictureTab.view = self.pictureViewController.view;

    _filtersViewController = [[HBFiltersViewController alloc] init];
    self.filtersTab.view = self.filtersViewController.view;

    _videoController = [[HBVideoController alloc] init];
    self.videoTab.view = self.videoController.view;

    _audioController = [[HBAudioController alloc] init];
    self.audioTab.view = self.audioController.view;

    _subtitlesViewController = [[HBSubtitlesController alloc] init];
    self.subtitlesTab.view = self.subtitlesViewController.view;

    _chapterTitlesController = [[HBChapterTitlesController alloc] init];
    self.chaptersTab.view = self.chapterTitlesController.view;

    // Add the observers
    [self.core addObserver:self forKeyPath:@"state"
                   options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                   context:HBControllerScanCoreContext];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidStartNotification
                                                    object:_queue queue:NSOperationQueue.mainQueue
                                                usingBlock:^(NSNotification * _Nonnull note) {
        self.bottomConstrain.animator.constant = 0;
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteNotification
                                                    object:_queue queue:NSOperationQueue.mainQueue
                                                usingBlock:^(NSNotification * _Nonnull note) {
        self.bottomConstrain.animator.constant = -WINDOW_HEIGHT_OFFSET;
        self.statusField.stringValue = @"";
        self.progress = @"";
        [self updateProgress];
    }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidStartItemNotification
                                                    object:_queue queue:NSOperationQueue.mainQueue
                                                usingBlock:^(NSNotification * _Nonnull note) { [self setUpQueueObservers]; }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteItemNotification
                                                    object:_queue queue:NSOperationQueue.mainQueue
                                                usingBlock:^(NSNotification * _Nonnull note) { [self setUpQueueObservers]; }];

    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidChangeStateNotification
                                                    object:_queue queue:NSOperationQueue.mainQueue
                                                usingBlock:^(NSNotification * _Nonnull note) { [self updateQueueUI]; }];

    [self updateQueueUI];

    // Presets menu
    _presetsMenuBuilder = [[HBPresetsMenuBuilder alloc] initWithMenu:self.presetsPopup.menu
                                                              action:@selector(selectPresetFromMenu:)
                                                                size:[NSFont smallSystemFontSize]
                                                      presetsManager:self.presetManager];
    [self.presetsMenuBuilder build];

    // Log level
    [NSUserDefaultsController.sharedUserDefaultsController addObserver:self forKeyPath:@"values.LoggingLevel"
                                                               options:0 context:HBControllerLogLevelContext];

    self.bottomConstrain.constant = -WINDOW_HEIGHT_OFFSET;

    [self.window recalculateKeyViewLoop];
}

#pragma mark - Drag & drop handling

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
        NSArray<NSURL *> *subtitlesFileURLs = [HBUtilities extractURLs:fileURLs withExtension:HBUtilities.supportedExtensions];
        if (subtitlesFileURLs.count == fileURLs.count && self.job)
        {
            [self.subtitlesViewController addTracksFromExternalFiles:fileURLs];
            NSUInteger index = [self.mainTabView indexOfTabViewItem:self.subtitlesTab];
            [self.mainTabView selectTabViewItemAtIndex:index];
        }
        else
        {
            BOOL recursive = [NSUserDefaults.standardUserDefaults boolForKey:HBRecursiveScan];
            [self openURLs:fileURLs recursive:recursive];
        }
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
    if (context == HBControllerScanCoreContext)
    {
        HBState state = [change[NSKeyValueChangeNewKey] intValue];
        [self.toolbarDelegate updateToolbarButtonsStateForScanCore:state toolbar:self.window.toolbar];
        [self _touchBar_updateButtonsStateForScanCore:state];
        [self _touchBar_validateUserInterfaceItems];
    }
    else if (context == HBControllerLogLevelContext)
    {
        self.core.logLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)updateQueueUI
{
    [self.toolbarDelegate updateToolbarButtonsState:self.queue toolbar:self.window.toolbar];
    [self.window.toolbar validateVisibleItems];

    [self _touchBar_updateQueueButtonsState];
    [self _touchBar_validateUserInterfaceItems];

    NSUInteger count = self.queue.pendingItemsCount;
    [self.toolbarDelegate updateToolbarQueueBadge:count ? @(count).stringValue : @"" toolbar:self.window.toolbar];
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

    self.presetView.enabled = enabled;
}

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"scanSpecificTitleIdx"])
    {
        [self setValue:@0 forKey:key];
    }
}

#pragma mark - Queue progress

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
    if (self.window.occlusionState & NSWindowOcclusionStateVisible)
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
    self.progressField.stringValue = self.progress;
}

- (void)setUpQueueObservers
{
    [self removeQueueObservers];

    if (self->_queue.workingItemsCount > 1)
    {
        [self setUpForMultipleWorkers];
    }
    else if (self->_queue.workingItemsCount == 1)
    {
        [self setUpForSingleWorker];
    }
}

- (void)setUpForMultipleWorkers
{
    self.statusField.stringValue = [NSString stringWithFormat:NSLocalizedString(@"Encoding %lu Jobs", @""), self.queue.workingItemsCount];
    self.progress = NSLocalizedString(@"Working", @"");
    [self updateProgress];
}

- (void)setUpForSingleWorker
{
    HBQueueJobItem *firstWorkingItem = nil;
    for (HBQueueJobItem *item in self.queue.items)
    {
        if (item.state == HBQueueItemStateWorking)
        {
            firstWorkingItem = item;
            break;
        }
    }

    if (firstWorkingItem)
    {
        HBQueueWorker *worker = [self.queue workerForItem:firstWorkingItem];

        if (worker)
        {
            self.observerToken = [NSNotificationCenter.defaultCenter addObserverForName:HBQueueWorkerProgressNotification
                                                                                 object:worker queue:NSOperationQueue.mainQueue
                                                                             usingBlock:^(NSNotification * _Nonnull note) {
                self.progress = note.userInfo[HBQueueWorkerProgressNotificationInfoKey];

                if (self->_visible)
                {
                    [self updateProgress];
                }
            }];
        }
    }

    self.statusField.stringValue = [NSString stringWithFormat:NSLocalizedString(@"Encoding Job: %@", @""), firstWorkingItem.destinationFileName];
}

- (void)removeQueueObservers
{
    if (self.observerToken)
    {
        [NSNotificationCenter.defaultCenter removeObserver:self.observerToken];
        self.observerToken = nil;
    }
}

#pragma mark - UI Validation

- (BOOL)validateUserIterfaceItemForAction:(SEL)action
{
    if (self.core.state == HBStateScanning)
    {
        if (action == @selector(browseSources:))
        {
            return YES;
        }
        if (action == @selector(toggleStartCancel:) || action == @selector(addToQueue:))
        {
            return NO;
        }
    }
    else if (action == @selector(browseSources:))
    {
        return YES;
    }

    if (action == @selector(toggleStartCancel:))
    {
        if (self.queue.isEncoding)
        {
            return YES;
        }
        else
        {
            return (self.job != nil || self.queue.canEncode);
        }
    }

    if (action == @selector(togglePauseResume:))
    {
        return self.queue.canPause || self.queue.canResume;
    }

    if (action == @selector(addToQueue:))
    {
        return (self.job != nil);
    }

    return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    return [self validateUserIterfaceItemForAction:anItem.action];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(addToQueue:) ||
        action == @selector(addAllTitlesToQueue:) ||
        action == @selector(addTitlesToQueue:) ||
        action == @selector(showAddPresetPanel:) ||
        action == @selector(revealDestinationItemsInFinder:) ||
        action == @selector(revealSourceItemsInFinder:))
    {
        return self.job && self.window.attachedSheet == nil;
    }
    if (action == @selector(selectDefaultPreset:))
    {
        return self.window.attachedSheet == nil;
    }
    if (action == @selector(togglePauseResume:))
    {
        return [self.delegate validateMenuItem:menuItem];
    }
    if (action == @selector(toggleStartCancel:))
    {
        BOOL result = [self.delegate validateMenuItem:menuItem];

        if ([menuItem.title isEqualToString:NSLocalizedString(@"Start Encoding", @"Menu Start/Stop Item")])
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
            menuItem.state = NSControlStateValueOn;
        }
        else
        {
            menuItem.state = NSControlStateValueOff;
        }
        return (self.job != nil);
    }
    if (action == @selector(exportPreset:) ||
        action == @selector(selectDefaultPreset:))
    {
        return self.job != nil;
    }
    if (action == @selector(deletePreset:) ||
        action == @selector(setDefaultPreset:))
    {
        return self.job != nil && self.selectedPreset;
    }
    if (action == @selector(savePreset:))
    {
        return self.job != nil && self.selectedPreset && self.selectedPreset.isBuiltIn == NO;
    }
    if (action == @selector(showRenamePresetPanel:))
    {
        return self.selectedPreset && self.selectedPreset.isBuiltIn == NO;
    }
    if (action == @selector(switchToNextTitle:) ||
        action == @selector(switchToPreviousTitle:))
    {
        return self.core.titles.count > 1 && self.job != nil;
    }

    return YES;
}

#pragma mark - Get New Source

- (void)launchAction
{
    if (self.core.state != HBStateScanning && !self.job)
    {
        if ([NSUserDefaults.standardUserDefaults boolForKey:HBShowOpenPanelAtLaunch])
        {
            [self browseSources:self];
        }
    }
}

- (NSModalResponse)runCopyProtectionAlert
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"Copy-Protected sources are not supported.", @"Copy Protection Alert -> message")];
    [alert setInformativeText:NSLocalizedString(@"Please note that HandBrake does not support the removal of copy-protection from DVD Discs. You can if you wish use any other 3rd party software for this function. This warning will be shown only once each time HandBrake is run.", @"Copy Protection Alert -> informative text")];
    [alert addButtonWithTitle:NSLocalizedString(@"Attempt Scan Anyway", @"Copy Protection Alert -> first button")];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"Copy Protection Alert -> second button")];

    [NSApp requestUserAttention:NSCriticalRequest];

    return [alert runModal];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)scanURLs:(NSArray<NSURL *> *)fileURLs titleIndex:(NSUInteger)index keepDuplicateTitles:(BOOL)keepDuplicateTitles completionHandler:(void(^)(NSArray<HBTitle *> *titles))completionHandler
{
    [self showWindow:self];

    // Check if we can scan the source and if there is any warning.
    NSError *outError = NULL;
    BOOL canScan = YES;

    if (fileURLs.count == 1)
    {
        canScan = [self.core canScan:fileURLs error:&outError];
    }

    // Notify the user that we don't support removal of copy protection.
    if (canScan && outError.code == 101 && !self.suppressCopyProtectionWarning)
    {
        self.suppressCopyProtectionWarning = YES;
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
        NSUInteger previewsCount = [NSUserDefaults.standardUserDefaults integerForKey:HBPreviewsNumber];
        NSUInteger minTitleDuration = [NSUserDefaults.standardUserDefaults boolForKey:HBMinTitleScan] ?
                                       [NSUserDefaults.standardUserDefaults integerForKey:HBMinTitleScanSeconds] : 0;
        NSUInteger maxTitleDuration = [NSUserDefaults.standardUserDefaults boolForKey:HBMaxTitleScan] ?
                                       [NSUserDefaults.standardUserDefaults integerForKey:HBMaxTitleScanSeconds] : 0;

        [self.core scanURLs:fileURLs
                 titleIndex:index
                   previews:previewsCount
                minDuration:minTitleDuration
                maxDuration:maxTitleDuration
               keepPreviews:YES
            hardwareDecoder:[NSUserDefaults.standardUserDefaults boolForKey:HBUseHardwareDecoder]
            keepDuplicateTitles:keepDuplicateTitles
            progressHandler:^(HBState state, HBProgress progress, NSString *info)
         {
             self.sourceLabel.stringValue = info;
             self.scanIndicator.hidden = NO;
             self.scanHorizontalLine.hidden = YES;
             self.scanIndicator.doubleValue = progress.percent;
         }
         completionHandler:^(HBCoreResult result)
         {
             self.scanHorizontalLine.hidden = NO;
             self.scanIndicator.hidden = YES;
             self.scanIndicator.indeterminate = NO;
             self.scanIndicator.doubleValue = 0.0;

             if (result.code == HBCoreResultCodeDone)
             {
                 for (HBTitle *title in self.core.titles)
                 {
                     [self.titlePopUp addItemWithTitle:title.description];
                 }
             }
             else
             {
                 // We display a message if a valid source was not chosen
                 self.sourceLabel.stringValue = NSLocalizedString(@"No Valid Source Found", @"Main Window -> Info text");
             }

             completionHandler(self.core.titles);

             // Clear the undo manager, the completion handler
             // set the job in the main window
             // and we don't want to make it undoable
             [self.window.undoManager removeAllActions];
             [self.window.toolbar validateVisibleItems];
             [self _touchBar_validateUserInterfaceItems];
         }];
    }
    else
    {
        completionHandler(@[]);
    }
}

- (void)showOpenPanelForDestination:(NSURL *)destinationURL
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = NO;
    panel.canChooseDirectories = YES;
    panel.directoryURL = destinationURL;
    panel.message = NSLocalizedString(@"HandBrake does not have permission to write to this folder. To allow HandBrake to write to this folder, click \"Allow\"", @"Main Window -> Same as source destination open panel");
    panel.prompt = NSLocalizedString(@"Allow", @"Main Window -> Same as source destination open panel");

    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
     {
         if (result == NSModalResponseOK)
         {
             self.destinationFolderURL = panel.URL;
             self.destinationFolderToken = [HBSecurityAccessToken tokenWithAlreadyAccessedObject:panel.URL];
             if (self.job)
             {
                 [self.job setDestinationFolderURL:self.destinationFolderURL sameAsSource:YES];
                 [self.window.undoManager removeAllActions];
             }
         }
     }];
}

- (void)askForPermissionAndSetDestinationURLs:(NSArray<NSURL *> *)destinationURLs sourceURLs:(NSArray<NSURL *> *)sourceURLs
{
    if (destinationURLs.count == 0)
    {
        return;
    }

    if (sourceURLs.count == 1)
    {
        // There is no need to ask for permission
        // if the source is a already a folder
        NSNumber *isDirectory = nil;
        NSURL *sourceURL = sourceURLs.firstObject;
        [sourceURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

        if (isDirectory.boolValue == YES)
        {
            self.destinationFolderURL = sourceURL;
            self.destinationFolderToken = [HBSecurityAccessToken tokenWithObject:sourceURL];
            [self.job setDestinationFolderURL:sourceURL sameAsSource:YES];
            return;
        }
    }

    if (![self.destinationFolderURL isEqualTo:destinationURLs.firstObject])
    {
#ifdef __SANDBOX_ENABLED__
        [self showOpenPanelForDestination:destinationURLs.firstObject];
#else
        [self.job setDestinationFolderURL:destinationURLs.firstObject sameAsSource:YES];
#endif
    }
}

- (void)cleanUp
{
    self.job = nil;
    self.fileTokens = nil;
    self.subsURLs = nil;

    [self.titlePopUp removeAllItems];
    self.window.representedURL = nil;
    self.window.title = NSLocalizedString(@"HandBrake", @"Main Window -> title");

    // Clear the undo manager, we can't undo this action
    [self.window.undoManager removeAllActions];
}

- (void)openURLs:(NSArray<NSURL *> *)fileURLs recursive:(BOOL)recursive titleIndex:(NSUInteger)index
{
    // Save the current settings
    [self updateCurrentPreset];
    [self cleanUp];

    NSMutableArray<HBSecurityAccessToken *> *tokens = [[NSMutableArray alloc] init];
    for (NSURL *fileURL in fileURLs)
    {
        [tokens addObject:[HBSecurityAccessToken tokenWithAlreadyAccessedObject:fileURL]];
    }
    self.fileTokens = tokens;

    NSMutableArray<NSString *> *excludedExtensions = [[NSMutableArray alloc] init];
    for (NSString *extension in [NSUserDefaults.standardUserDefaults arrayForKey:HBExcludedFileExtensions])
    {
        // Make sure there are only NSString instances in the array
        // Third parties can write to user defaults too and add different kind of objects.
        if ([extension isKindOfClass:[NSString class]])
        {
            [excludedExtensions addObject:extension];
        }
    }

    NSArray<NSURL *> *expandedFileURLs  = [HBUtilities expandURLs:fileURLs recursive:recursive];
    NSArray<NSURL *> *subtitlesFileURLs = [HBUtilities extractURLs:expandedFileURLs withExtension:HBUtilities.supportedExtensions];
    NSArray<NSURL *> *trimmedFileURLs   = [HBUtilities trimURLs:expandedFileURLs withExtension:excludedExtensions];

    [self scanURLs:trimmedFileURLs titleIndex:index keepDuplicateTitles:[NSUserDefaults.standardUserDefaults boolForKey:HBKeepDuplicateTitles] completionHandler:^(NSArray<HBTitle *> *titles)
    {
        NSArray<NSURL *> *baseURLs = [HBUtilities baseURLs:trimmedFileURLs];

        if (titles.count)
        {
            for (NSURL *fileURL in fileURLs)
            {
                [NSDocumentController.sharedDocumentController noteNewRecentDocumentURL:fileURL];
            }

            HBTitle *featuredTitle = titles.firstObject;
            for (HBTitle *title in titles)
            {
                if (title.isFeatured)
                {
                    featuredTitle = title;
                }
            }

            self.subsURLs = subtitlesFileURLs;

            HBJob *job = [self jobFromTitle:featuredTitle];
            if (job)
            {
                self.job = job;
                if (featuredTitle.isStream && [NSUserDefaults.standardUserDefaults boolForKey:HBUseSourceFolderDestination])
                {
                    [self askForPermissionAndSetDestinationURLs:baseURLs sourceURLs:fileURLs];
                }
            }
            else
            {
                [self cleanUp];
                self.sourceLabel.stringValue = NSLocalizedString(@"No Valid Preset", @"Main Window -> Info text");
            }
        }

        // Set the last searched source directory in the prefs here
        [NSUserDefaults.standardUserDefaults setURL:baseURLs.firstObject forKey:HBLastSourceDirectoryURL];
    }];
}

- (void)openURLs:(NSArray<NSURL *> *)fileURLs recursive:(BOOL)recursive
{
    if (self.core.state != HBStateScanning)
    {
        [self openURLs:fileURLs recursive:recursive titleIndex:0];
    }
}

/**
 * Rescans the a job back into the main window
 */
- (void)openJob:(HBJob *)job completionHandler:(void (^)(BOOL result))handler
{
    if (self.core.state != HBStateScanning)
    {
        [self cleanUp];
        [job refreshSecurityScopedResources];
        self.fileTokens = @[[HBSecurityAccessToken tokenWithObject:job.fileURL]];

        [self scanURLs:@[job.fileURL] titleIndex:job.titleIdx keepDuplicateTitles:job.keepDuplicateTitles completionHandler:^(NSArray<HBTitle *> *titles)
        {
            if (titles.count)
            {
                // Match the title index if the scan returned a cached result
                for (HBTitle *title in titles)
                {
                    if (title.index == job.titleIdx)
                    {
                        job.title = title;
                    }
                }

                if (job.title == nil)
                {
                    job.title = titles.firstObject;
                }

                self.job = job;
                job.undo = self.window.undoManager;

                self.currentPreset = [self createPresetFromCurrentSettings];
                self.selectedPreset = nil;

                handler(YES);
            }
            else
            {
                handler(NO);
                [self cleanUp];
            }
        }];
    }
    else
    {
        handler(NO);
    }
}

- (NSArray<NSURL *> *)matchSubtitlesURLsWith:(NSURL *)sourceURL
{
    NSMutableArray<NSURL *> *URLs = [[NSMutableArray alloc] init];
    NSString *sourcePrefix = [sourceURL.path stringByDeletingPathExtension];

    for (NSURL *url in self.subsURLs)
    {
        if ([url.path hasPrefix:sourcePrefix])
        {
            [URLs addObject:url];
        }
    }

    return URLs;
}

- (nullable HBJob *)jobFromTitle:(HBTitle *)title
{
    // If there is already a title loaded, save the current settings to a preset
    [self updateCurrentPreset];

    NSArray<NSURL *> *subtitlesURLs = [self matchSubtitlesURLsWith:title.url];
    HBJob *job = [[HBJob alloc] initWithTitle:title preset:self.currentPreset subtitles:subtitlesURLs];

    if (job)
    {
        [job setDestinationFolderURL:self.destinationFolderURL
                        sameAsSource:[NSUserDefaults.standardUserDefaults boolForKey:HBUseSourceFolderDestination]];

        // If the source is not a stream, and autonaming is disabled,
        // keep the existing file name.
        if (self.job.destinationFileName.length == 0 || title.isStream || [NSUserDefaults.standardUserDefaults boolForKey:HBDefaultAutoNaming])
        {
            job.destinationFileName = job.defaultName;
        }
        else
        {
            job.destinationFileName = self.job.destinationFileName;
        }

        job.undo = self.window.undoManager;
    }

    return job;
}

- (void)removeJobObservers
{
    if (self.job)
    {
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        [center removeObserver:self name:HBContainerChangedNotification object:_job];
        [center removeObserver:self name:HBPictureChangedNotification object:_job.picture];
        [center removeObserver:self name:HBFiltersChangedNotification object:_job.filters];
        [center removeObserver:self name:HBVideoChangedNotification object:_job.video];
        self.autoNamer = nil;
    }
}

/**
 *  Observe the job settings changes.
 *  This is used to update the file name and extension
 *  and the custom preset string.
 */
- (void)addJobObservers
{
    if (self.job)
    {
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        [center addObserver:self selector:@selector(formatChanged:) name:HBContainerChangedNotification object:_job];
        [center addObserver:self selector:@selector(customSettingUsed) name:HBPictureChangedNotification object:_job.picture];
        [center addObserver:self selector:@selector(customSettingUsed) name:HBFiltersChangedNotification object:_job.filters];
        [center addObserver:self selector:@selector(customSettingUsed) name:HBVideoChangedNotification object:_job.video];
        self.autoNamer = [[HBAutoNamer alloc] initWithJob:self.job];
    }
}

- (void)setJob:(HBJob *)job
{
    if (job != _job)
    {
        [[self.window.undoManager prepareWithInvocationTarget:self] setJob:_job];
    }

    [self removeJobObservers];
    _job = job;

    // Set the jobs info to the view controllers
    self.summaryController.job = job;
    self.pictureViewController.picture = job.picture;
    self.filtersViewController.filters = job.filters;
    self.videoController.video = job.video;
    self.audioController.audio = job.audio;
    self.subtitlesViewController.subtitles = job.subtitles;
    self.chapterTitlesController.job = job;

    if (job)
    {
        HBPreviewGenerator *generator = [[HBPreviewGenerator alloc] initWithCore:self.core job:job];
        self.previewController.generator = generator;
        self.summaryController.generator = generator;

        HBTitle *title = job.title;

        // Update the title selection popup.
        [self.titlePopUp selectItemWithTitle:title.description];

        // Grok the output file name from title.name upon title change
        if (title.isStream && self.core.titles.count > 1)
        {
            // Change the source to read out the parent folder also
            self.sourceLabel.stringValue = [NSString stringWithFormat:@"%@/%@, %@", title.url.URLByDeletingLastPathComponent.lastPathComponent,
                                            title.name, title.shortFormatDescription];
        }
        else
        {
            self.sourceLabel.stringValue = [NSString stringWithFormat:@"%@, %@", title.name, title.shortFormatDescription];
        }

        self.window.representedURL = job.fileURL;
        self.window.title = job.fileURL.lastPathComponent;
    }
    else
    {
        [self.previewController.generator invalidate];
        self.previewController.generator = nil;
        self.summaryController.generator = nil;
    }
    self.previewController.picture = job.picture;

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
    [panel setAllowsMultipleSelection:YES];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:YES];

    NSURL *sourceDirectory;
	if ([NSUserDefaults.standardUserDefaults URLForKey:HBLastSourceDirectoryURL])
	{
		sourceDirectory = [NSUserDefaults.standardUserDefaults URLForKey:HBLastSourceDirectoryURL];
	}
	else
	{
        sourceDirectory = [NSURL fileURLWithPath:[NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) firstObject]
                                                       isDirectory:YES];
	}

    panel.directoryURL = sourceDirectory;
    panel.accessoryView = self.openTitleView;
    panel.accessoryViewDisclosed = YES;

    [panel beginSheetModalForWindow:self.window completionHandler: ^(NSInteger result)
    {
        if (result == NSModalResponseOK)
         {
             BOOL recursive = [NSUserDefaults.standardUserDefaults boolForKey:HBRecursiveScan];
             NSInteger titleIdx = self.scanSpecificTitle ? self.scanSpecificTitleIdx : 0;
             [self openURLs:panel.URLs recursive:recursive titleIndex:titleIdx];
         }
     }];
}

#pragma mark - GUI Controls Changed Methods

- (void)setDestinationFolderURL:(NSURL *)destinationFolderURL
{
    self.job.destinationFolderURL = destinationFolderURL;
    _destinationFolderURL = destinationFolderURL;

    // Save this path to the prefs so that on next browse destination window it opens there
    [NSUserDefaults.standardUserDefaults setObject:[HBUtilities bookmarkFromURL:destinationFolderURL]
                                              forKey:HBLastDestinationDirectoryBookmark];
    [NSUserDefaults.standardUserDefaults setURL:destinationFolderURL
                                         forKey:HBLastDestinationDirectoryURL];
}

- (IBAction)browseDestination:(id)sender
{
    // Open a panel to let the user choose and update the text field
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = NO;
    panel.canChooseDirectories = YES;
    panel.canCreateDirectories = YES;
    panel.prompt = NSLocalizedString(@"Choose", @"Main Window -> Destination open panel");

    if (self.job.destinationFolderURL)
    {
        panel.directoryURL = self.job.destinationFolderURL;
    }

    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
     {
         if (result == NSModalResponseOK)
         {
             self.destinationFolderURL = panel.URL;
             self.destinationFolderToken = [HBSecurityAccessToken tokenWithAlreadyAccessedObject:panel.URL];
         }
     }];
}

- (NSDragOperation)pathControl:(NSPathControl *)pathControl validateDrop:(id <NSDraggingInfo>)info
{
    NSPasteboard *pboard = info.draggingPasteboard;

    if ([pboard availableTypeFromArray:@[NSPasteboardTypeFileURL]])
    {
        NSURL *URL = [[pboard readObjectsForClasses:@[[NSURL class]] options:nil] firstObject];
        if (URL.hasDirectoryPath)
        {
            return NSDragOperationGeneric;
        }
    }

    return NSDragOperationNone;
}

- (BOOL)pathControl:(NSPathControl *)pathControl acceptDrop:(id <NSDraggingInfo>)info
{
    NSPasteboard *pboard = info.draggingPasteboard;

    if ([pboard availableTypeFromArray:@[NSPasteboardTypeFileURL]])
    {
        NSURL *URL = [[pboard readObjectsForClasses:@[[NSURL class]] options:nil] firstObject];
        if (URL.hasDirectoryPath)
        {
            self.destinationFolderURL = URL;
            self.destinationFolderToken = [HBSecurityAccessToken tokenWithAlreadyAccessedObject:URL];
        }
        return YES;
    }

    return NO;
}

- (IBAction)revealPathItemInFinder:(id)sender
{
    NSURL *URL = self.destinationPathControl.clickedPathItem.URL;
    if (URL == nil)
    {
        URL = self.destinationFolderURL;
    }
    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[URL]];
}

- (IBAction)titlePopUpChanged:(NSPopUpButton *)sender
{
    HBTitle *title = self.core.titles[sender.indexOfSelectedItem];
    HBJob *job = [self jobFromTitle:title];
    if (job)
    {
        self.job = job;
    }
}

- (void)formatChanged:(NSNotification *)notification
{
    [self customSettingUsed];
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
        if (![self.job.presetName hasSuffix:NSLocalizedString(@"(Modified)", @"Main Window -> preset modified")])
        {
            self.job.presetName = [NSString stringWithFormat:@"%@ %@", self.job.presetName, NSLocalizedString(@"(Modified)", @"Main Window -> preset modified")];
        }
    }
}

- (IBAction)switchToNextTitle:(id)sender
{
    NSArray<HBTitle *> *titles = self.core.titles;
    if (titles && self.job)
    {
        NSUInteger index = [titles indexOfObject:self.job.title];
        if (index != NSNotFound && index < titles.count - 1)
        {
            HBTitle *title = titles[index + 1];
            HBJob *job = [self jobFromTitle:title];
            if (job)
            {
                self.job = job;
            }
        }
    }
}

- (IBAction)switchToPreviousTitle:(id)sender
{
    NSArray<HBTitle *> *titles = self.core.titles;
    if (titles && self.job)
    {
        NSUInteger index = [titles indexOfObject:self.job.title];
        if (index != NSNotFound && index > 0)
        {
            HBTitle *title = titles[index - 1];
            HBJob *job = [self jobFromTitle:title];
            if (job)
            {
                self.job = job;
            }
        }
    }
}

- (IBAction)revealDestinationItemsInFinder:(id)sender
{
    if (self.job.destinationURL)
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[self.job.destinationFolderURL]];
    }
}

- (IBAction)revealSourceItemsInFinder:(id)sender
{
    if (self.job.fileURL)
    {
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[self.job.fileURL]];
    }
}

#pragma mark - Job Handling

/**
 Check if the job destination if a valid one,
 if so, call the handler
 @param job the job
 @param completionHandler the block to call if the check is successful
 */
- (void)runDestinationAlerts:(HBJob *)job completionHandler:(void (^ __nullable)(NSModalResponse returnCode))handler
{
    if ([job.destinationFolderURL checkResourceIsReachableAndReturnError:NULL] == NO)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Warning!", @"Invalid destination alert -> message")];
        [alert setInformativeText:NSLocalizedString(@"This is not a valid destination directory!", @"Invalid destination alert -> informative text")];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert beginSheetModalForWindow:self.window completionHandler:handler];
    }
    else if ([job.fileURL isEqual:job.destinationURL]||
             [job.fileURL.absoluteString.lowercaseString isEqualToString:job.destinationURL.absoluteString.lowercaseString])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"Destination same as source alert -> message")];
        [alert setInformativeText:NSLocalizedString(@"The destination is the same as the source, you can not overwrite your source file!", @"Destination same as source alert -> informative text")];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert beginSheetModalForWindow:self.window completionHandler:handler];
    }
    else if ([job.destinationURL checkResourceIsReachableAndReturnError:NULL])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"File already exists alert -> message")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @"File already exists alert -> informative text"), job.destinationURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"File already exists alert -> first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"File already exists alert -> second button")];
        if (@available(macOS 11, *))
        {
            alert.buttons.lastObject.hasDestructiveAction = true;
        }
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert beginSheetModalForWindow:self.window completionHandler:handler];
    }
    else if ([_queue itemExistAtURL:job.destinationURL])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"There is already a queue item for this destination.", @"File already exists in queue alert -> message")];
        [alert setInformativeText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to overwrite %@?", @"File already exists in queue alert -> informative text"), job.destinationURL.path]];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"File already exists in queue alert -> first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"File already exists in queue alert -> second button")];
        if (@available(macOS 11, *))
        {
            alert.buttons.lastObject.hasDestructiveAction = true;
        }
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert beginSheetModalForWindow:self.window completionHandler:handler];
    }
    else
    {
        handler(NSAlertSecondButtonReturn);
    }
}

/**
 *  Actually adds a job to the queue
 */
- (void)doAddToQueue
{
    [_queue addJob:[self.job copy]];
}

/**
 * Puts up an alert before ultimately calling doAddToQueue
 */
- (IBAction)addToQueue:(id)sender
{
    if ([self.window HB_endEditing])
    {
        [self runDestinationAlerts:self.job completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSAlertSecondButtonReturn)
            {
                [self doAddToQueue];
            }
        }];
    }
}

- (void)doRip
{
    // if there are no jobs in the queue, then add this one to the queue and rip
    // otherwise, just rip the queue
    if (_queue.pendingItemsCount == 0)
    {
        [self doAddToQueue];
    }

    [_delegate toggleStartCancel:self];
}

/**
 * Puts up an alert before ultimately calling doRip
 */
- (IBAction)toggleStartCancel:(id)sender
{
    // Rip or Cancel ?
    if (_queue.isEncoding || _queue.canEncode)
	{
        // Displays an alert asking user if the want to cancel encoding of current job.
        [_delegate toggleStartCancel:self];
    }
    else
    {
        if ([self.window HB_endEditing])
        {
            [self runDestinationAlerts:self.job completionHandler:^(NSModalResponse returnCode) {
                if (returnCode == NSAlertSecondButtonReturn)
                {
                    [self doRip];
                }
            }];
        }
    }
}

- (IBAction)togglePauseResume:(id)sender
{
    [_delegate togglePauseResume:sender];
}

#pragma mark -
#pragma mark Batch Queue Titles Methods

- (IBAction)addTitlesToQueue:(id)sender
{
    [self.window HB_endEditing];

    self.titlesSelectionController = [[HBTitleSelectionController alloc] initWithTitles:self.core.titles
                                                                             presetName:self.job.presetName
                                                                               delegate:self];

    [self.window beginSheet:self.titlesSelectionController.window completionHandler:nil];
}

- (void)didSelectTitles:(NSArray<HBTitle *> *)titles range:(nullable HBTitleSelectionRange *)range
{
    [self.window endSheet:self.titlesSelectionController.window];

    [self doAddTitlesToQueue:titles range:range];
}

- (void)doAddTitlesToQueue:(NSArray<HBTitle *> *)titles range:(nullable HBTitleSelectionRange *)range
{
    NSMutableArray<HBJob *> *jobs = [[NSMutableArray alloc] init];
    BOOL fileExists = NO;
    BOOL fileOverwritesSource = NO;
    BOOL useSourceFolderDestination = [NSUserDefaults.standardUserDefaults boolForKey:HBUseSourceFolderDestination];

    // Get the preset from the loaded job.
    HBPreset *preset = [self createPresetFromCurrentSettings];

    for (HBTitle *title in titles)
    {
        HBJob *job = [[HBJob alloc] initWithTitle:title preset:preset];

        if (job)
        {
            [job applySelectionRange:range];
            [job setDestinationFolderURL:self.destinationFolderURL
                            sameAsSource:useSourceFolderDestination];
            job.destinationFileName = job.defaultName;
            job.title = nil;

            [jobs addObject:job];
        }
    }

    NSMutableSet<NSURL *> *destinations = [[NSMutableSet alloc] init];
    for (HBJob *job in jobs)
    {
        if ([destinations containsObject:job.destinationURL])
        {
            fileExists = YES;
            break;
        }
        else
        {
            [destinations addObject:job.destinationURL];
        }

        if ([job.destinationURL checkResourceIsReachableAndReturnError:NULL] || [_queue itemExistAtURL:job.destinationURL])
        {
            fileExists = YES;
            break;
        }
    }

    for (HBJob *job in jobs)
    {
        if ([job.fileURL isEqual:job.destinationURL]) {
            fileOverwritesSource = YES;
            break;
        }
    }

    if (fileOverwritesSource)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"A file already exists at the selected destination.", @"Destination same as source alert -> message")];
        [alert setInformativeText:NSLocalizedString(@"The destination is the same as the source, you can not overwrite your source file!", @"Destination same as source alert -> informative text")];
        [alert beginSheetModalForWindow:self.window completionHandler:nil];
    }
    else if (fileExists)
    {
        // File exist, warn user
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"File already exists.", @"File already exists alert -> message")];
        [alert setInformativeText:NSLocalizedString(@"One or more file already exists. Do you want to overwrite?", @"File already exists alert -> informative text")];
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"File already exists alert -> first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Overwrite", @"File already exists alert -> second button")];
        if (@available(macOS 11, *))
        {
            alert.buttons.lastObject.hasDestructiveAction = true;
        }
        [alert setAlertStyle:NSAlertStyleCritical];

        [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSAlertSecondButtonReturn)
            {
                [self->_queue addJobs:jobs];
            }
        }];
    }
    else
    {
        [_queue addJobs:jobs];
    }
}

- (IBAction)addAllTitlesToQueue:(id)sender
{
    [self doAddTitlesToQueue:self.core.titles range:nil];
}

#pragma mark - Picture

- (IBAction)showPreviewWindow:(id)sender
{
	[self.previewController showWindow:sender];
}

- (IBAction)showTabView:(id)sender
{
    NSInteger tag = [sender tag];
    [self.mainTabView selectTabViewItemAtIndex:tag];
}

#pragma mark - Presets View Controller Delegate

- (void)selectionDidChange
{
    if (self.job)
    {
        BOOL success = [self doApplyPreset:self.presetView.selectedPreset];
        if (success == YES)
        {
            self.selectedPreset = self.presetView.selectedPreset;
        }
    }
    else
    {
        self.currentPreset = self.presetView.selectedPreset;
        self.selectedPreset = self.presetView.selectedPreset;
        [self.window.undoManager removeAllActions];
    }
}

#pragma mark -  Presets

- (BOOL)popoverShouldDetach:(NSPopover *)popover
{
    if (popover == self.presetsPopover)
    {
        return YES;
    }

    return NO;
}

- (void)popoverDidDetach:(NSPopover *)popover
{
    if (popover == self.presetsPopover)
    {
        self.presetView.showHeader = YES;
    }
}

- (IBAction)togglePresets:(id)sender
{
    NSToolbarItem *presetsToolbarItem = [self.window.toolbar HB_visibleToolbarItemWithIdentifier:TOOLBAR_PRESET];

    if (self.presetsPopover.isShown)
    {
        [self.presetsPopover close];
    }
    else
    {
        NSView *target = presetsToolbarItem.view.window ? presetsToolbarItem.view : self.window.contentView;
        if (self.window.toolbar.visible && presetsToolbarItem)
        {
#ifdef MAC_OS_VERSION_14_0
            if (@available (macOS 14, *))
            {
                [self.presetsPopover showRelativeToToolbarItem:presetsToolbarItem];
                self.presetView.showHeader = NO;
            }
            else
#endif
            {
                [self.presetsPopover showRelativeToRect:target.bounds ofView:target preferredEdge:NSMaxYEdge];
            }
        }
        else
        {
            [self.presetsPopover showRelativeToRect:target.bounds ofView:target preferredEdge:NSMaxYEdge];
        }
    }
}

- (void)setSelectedPreset:(HBPreset *)selectedPreset
{
    if (selectedPreset != _selectedPreset)
    {
        [[self.window.undoManager prepareWithInvocationTarget:self] setSelectedPreset:_selectedPreset];
        _selectedPreset = selectedPreset;
    }
}

- (void)setCurrentPreset:(HBPreset *)currentPreset
{
    NSParameterAssert(currentPreset);

    if (currentPreset != _currentPreset)
    {
        [[self.window.undoManager prepareWithInvocationTarget:self] setCurrentPreset:_currentPreset];
        _currentPreset = currentPreset;
    }
}

- (IBAction)reloadPreset:(id)sender
{
    HBPreset *preset = self.selectedPreset ? self.selectedPreset : self.currentPreset;
    if (preset)
    {
        [self doApplyPreset:preset];
    }
}

- (void)applyPreset:(HBPreset *)preset
{
    BOOL success = [self doApplyPreset:preset];
    if (success == YES)
    {
        self.selectedPreset = preset;
        self.presetView.selectedPreset = preset;
    }
}

- (BOOL)doApplyPreset:(HBPreset *)preset
{
    BOOL success = NO;

    if (self.job)
    {
        // Remove the job observer so we don't update the file name
        // too many times while the preset is being applied
        [self removeJobObservers];

        NSError *error = nil;
        success = [self.job applyPreset:preset error:&error];

        [self addJobObservers];

        if (success == NO)
        {
            [self presentError:error];
        }
        else
        {
            self.currentPreset = preset;
            [self.autoNamer updateFileExtension];
            // If Auto Naming is on, update the destination
            [self.autoNamer updateFileName];
        }
    }

    return success;
}

- (IBAction)showAddPresetPanel:(id)sender
{
    [self.window HB_endEditing];

    // Show the add panel
    HBAddPresetController *addPresetController = [[HBAddPresetController alloc] initWithPreset:[self createPresetFromCurrentSettings]
                                                                                 presetManager:self.presetManager
                                                                                   customWidth:self.job.picture.maxWidth
                                                                                  customHeight:self.job.picture.maxHeight
                                                                           resolutionLimitMode:self.job.picture.resolutionLimitMode];

    [self.window beginSheet:addPresetController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            [self applyPreset:addPresetController.preset];
        }
    }];
}

- (HBMutablePreset *)createPresetFromCurrentSettings
{
    HBMutablePreset *preset = [self.currentPreset mutableCopy];
    [self.job writeToPreset:preset];
    return preset;
}

- (void)updateCurrentPreset
{
    if (self.job)
    {
        if ([NSUserDefaults.standardUserDefaults boolForKey:HBKeepPresetEdits] == NO)
        {
            if (self.selectedPreset)
            {
                self.currentPreset = self.selectedPreset;
            }
        }
        else
        {
            self.currentPreset = [self createPresetFromCurrentSettings];
        }
    }
}

- (IBAction)showRenamePresetPanel:(id)sender
{
    [self.window HB_endEditing];

    __block HBRenamePresetController *renamePresetController = [[HBRenamePresetController alloc] initWithPreset:self.selectedPreset
                                                                                                  presetManager:self.presetManager];
    [self.window beginSheet:renamePresetController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            self.job.presetName = renamePresetController.preset.name;
        }
        renamePresetController = nil;
    }];
}

#pragma mark - Import Export Preset(s)

- (IBAction)exportPreset:(id)sender
{
    self.presetView.selectedPreset = self.selectedPreset;
    [self.presetView exportPreset:sender];
}

- (IBAction)importPreset:(id)sender
{
    [self.presetView importPreset:sender];
}

#pragma mark - Preset Menu

- (IBAction)selectDefaultPreset:(id)sender
{
    [self applyPreset:self.presetManager.defaultPreset];
}

- (IBAction)setDefaultPreset:(id)sender
{
    [self.presetManager setDefaultPreset:self.selectedPreset];
}

- (IBAction)savePreset:(id)sender
{
    [self.window HB_endEditing];

    NSIndexPath *indexPath = [self.presetManager indexPathOfPreset:self.selectedPreset];
    if (indexPath)
    {
        HBMutablePreset *preset = [self createPresetFromCurrentSettings];
        preset.name = self.selectedPreset.name;
        preset.isDefault = self.selectedPreset.isDefault;

        [self.presetManager replacePresetAtIndexPath:indexPath withPreset:preset];

        self.job.presetName = preset.name;
        self.selectedPreset = preset;
        self.presetView.selectedPreset = preset;

        [self.presetManager savePresets];
        [self.window.undoManager removeAllActions];
    }
}

- (IBAction)deletePreset:(id)sender
{
    self.presetView.selectedPreset = self.selectedPreset;
    [self.presetView deletePreset:self];
}

- (IBAction)insertCategory:(id)sender
{
    [self.presetView insertCategory:sender];
}

- (IBAction)selectPresetFromMenu:(id)sender
{
    // Retrieve the preset stored in the NSMenuItem
    HBPreset *preset = [sender representedObject];
    [self applyPreset:preset];
}

@end

@implementation HBController (TouchBar)

@dynamic touchBar;

static NSTouchBarItemIdentifier HBTouchBarMain = @"fr.handbrake.mainWindowTouchBar";

static NSTouchBarItemIdentifier HBTouchBarOpen = @"fr.handbrake.openSource";
static NSTouchBarItemIdentifier HBTouchBarAddToQueue = @"fr.handbrake.addToQueue";
static NSTouchBarItemIdentifier HBTouchBarAddTitlesToQueue = @"fr.handbrake.addTitlesToQueue";
static NSTouchBarItemIdentifier HBTouchBarRip = @"fr.handbrake.rip";
static NSTouchBarItemIdentifier HBTouchBarPause = @"fr.handbrake.pause";
static NSTouchBarItemIdentifier HBTouchBarPreview = @"fr.handbrake.preview";
static NSTouchBarItemIdentifier HBTouchBarActivity = @"fr.handbrake.activity";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarOpen, NSTouchBarItemIdentifierFixedSpaceSmall, HBTouchBarAddToQueue, NSTouchBarItemIdentifierFixedSpaceLarge, HBTouchBarRip, HBTouchBarPause, NSTouchBarItemIdentifierFixedSpaceLarge, HBTouchBarPreview, HBTouchBarActivity, NSTouchBarItemIdentifierOtherItemsProxy];

    bar.customizationIdentifier = HBTouchBarMain;
    bar.customizationAllowedItemIdentifiers = @[HBTouchBarOpen, HBTouchBarAddToQueue, HBTouchBarAddTitlesToQueue, HBTouchBarRip, HBTouchBarPause, HBTouchBarPreview, HBTouchBarActivity, NSTouchBarItemIdentifierFlexibleSpace];

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarOpen])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Open Source", @"Touch bar");

        NSButton *button = [NSButton buttonWithTitle:NSLocalizedString(@"Open Source", @"Touch bar") target:self action:@selector(browseSources:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarAddToQueue])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Add To Queue", @"Touch bar");

        NSButton *button = nil;
        if (@available (macOS 11, *))
        {
            NSImage *image = [NSImage imageNamed:@"photo.badge.plus"];
            button = [NSButton buttonWithImage:image target:self action:@selector(addToQueue:)];
        }
        else
        {
            button = [NSButton buttonWithTitle:NSLocalizedString(@"Add To Queue", @"Touch bar") target:self action:@selector(addToQueue:)];
        }

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarAddTitlesToQueue])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Add Titles To Queue", @"Touch bar");

        NSButton *button = [NSButton buttonWithTitle:NSLocalizedString(@"Add Titles To Queue", @"Touch bar") target:self action:@selector(addTitlesToQueue:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarRip])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Start/Stop Encoding", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPlayTemplate] target:self action:@selector(toggleStartCancel:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarPause])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Pause Encoding", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPauseTemplate] target:self action:@selector(togglePauseResume:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarPreview])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Show Preview Window", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarQuickLookTemplate] target:self action:@selector(showPreviewWindow:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarActivity])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Show Activity Window", @"Touch bar");

        NSImage *image = nil;
        if (@available (macOS 11, *))
        {
            image = [NSImage imageNamed:@"text.viewfinder"];
        }
        else
        {
            image = [NSImage imageNamed:NSImageNameTouchBarPlayTemplate];
        }

        NSButton *button = [NSButton buttonWithImage:image target:nil action:@selector(showOutputPanel:)];

        item.view = button;
        return item;
    }

    return nil;
}

- (void)_touchBar_updateButtonsStateForScanCore:(HBState)state
{
    NSButton *openButton = (NSButton *)[[self.touchBar itemForIdentifier:HBTouchBarOpen] view];

    if (state == HBStateIdle)
    {
        openButton.title = NSLocalizedString(@"Open Source", @"Touch bar");
        openButton.bezelColor = nil;
    }
    else
    {
        openButton.title = NSLocalizedString(@"Cancel Scan", @"Touch bar");
        openButton.bezelColor = [NSColor systemRedColor];
    }
}

- (void)_touchBar_updateQueueButtonsState
{
    NSButton *ripButton = (NSButton *)[[self.touchBar itemForIdentifier:HBTouchBarRip] view];
    NSButton *pauseButton = (NSButton *)[[self.touchBar itemForIdentifier:HBTouchBarPause] view];

    if (self.queue.isEncoding)
    {
        ripButton.image = [NSImage imageNamed:NSImageNameTouchBarRecordStopTemplate];
    }
    else
    {
        ripButton.image = [NSImage imageNamed:NSImageNameTouchBarPlayTemplate];
    }

    if (self.queue.canResume)
    {
        pauseButton.image = [NSImage imageNamed:NSImageNameTouchBarPlayTemplate];
    }
    else
    {
        pauseButton.image = [NSImage imageNamed:NSImageNameTouchBarPauseTemplate];
    }
}

- (void)_touchBar_validateUserInterfaceItems
{
    for (NSTouchBarItemIdentifier identifier in self.touchBar.itemIdentifiers) {
        NSTouchBarItem *item = [self.touchBar itemForIdentifier:identifier];
        NSView *view = item.view;
        if ([view isKindOfClass:[NSButton class]]) {
            NSButton *button = (NSButton *)view;
            BOOL enabled = [self validateUserIterfaceItemForAction:button.action];
            button.enabled = enabled;
        }
    }
}

@end
