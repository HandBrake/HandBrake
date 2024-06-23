/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import "HBQueueController.h"

#import "HBAppDelegate.h"

#import "HBQueue.h"
#import "HBQueueWorker.h"
#import "HBQueueTableViewController.h"
#import "HBQueueDetailsViewController.h"
#import "HBQueueInfoViewController.h"
#import "HBQueueMultiSelectionViewController.h"

#import "HBQueueToolbarDelegate.h"

#import "HBPreferencesKeys.h"
#import "NSArray+HBAdditions.h"

@import HandBrakeKit;
@import QuickLookUI;

@interface HBQueueController () <NSToolbarItemValidation, NSMenuItemValidation, NSUserNotificationCenterDelegate, HBQueueTableViewControllerDelegate, HBQueueDetailsViewControllerDelegate>

@property (nonatomic) NSSplitViewController *splitViewController;
@property (nonatomic) HBQueueTableViewController *tableViewController;
@property (nonatomic) NSViewController *containerViewController;
@property (nonatomic) HBQueueInfoViewController *infoViewController;
@property (nonatomic) HBQueueMultiSelectionViewController *multiSelectionViewController;

/// Whether the window is visible or occluded,
/// useful to avoid updating the UI needlessly
@property (nonatomic) BOOL visible;

@property (nonatomic) HBQueueToolbarDelegate *toolbarDelegate;

@property (nonatomic) IBOutlet NSToolbarItem *ripToolbarItem;
@property (nonatomic) IBOutlet NSToolbarItem *pauseToolbarItem;

@property (nonatomic, readonly) dispatch_queue_t sendQueue;

@end

@interface HBQueueController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
- (void)_touchBar_updateButtonsState;
- (void)_touchBar_validateUserInterfaceItems;
- (IBAction)_touchBar_toggleStartCancel:(id)sender;
@end

@implementation HBQueueController

- (instancetype)initWithQueue:(HBQueue *)queue
{
    NSParameterAssert(queue);

    if (self = [super initWithWindowNibName:@"Queue"])
    {
        _queue = queue;
        _sendQueue = dispatch_queue_create("fr.handbrake.SendToQueue", DISPATCH_QUEUE_SERIAL);

        NSUserNotificationCenter.defaultUserNotificationCenter.delegate = self;

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueLowSpaceAlertNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
            [self queueLowDiskSpaceAlert];
        }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
            [self queueCompletedAlerts];
        }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteItemNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
            // Run the per item notification and actions
            HBQueueJobItem *item = note.userInfo[HBQueueItemNotificationItemKey];
            if (item.state == HBQueueItemStateCompleted)
            {
                [self sendToExternalApp:item];
            }

            if (item.state == HBQueueItemStateCompleted || item.state == HBQueueItemStateFailed)
            {
                [self itemCompletedAlerts:item];
            }
        }];
    }

    return self;
}

- (NSUndoManager *)windowWillReturnUndoManager:(NSWindow *)window
{
    return _queue.undoManager;
}

- (void)windowDidLoad
{
    self.window.tabbingMode = NSWindowTabbingModeDisallowed;

    if (@available (macOS 11, *))
    {
        self.window.toolbarStyle = NSWindowToolbarStyleUnified;
        self.window.titlebarSeparatorStyle = NSTitlebarSeparatorStyleLine;
    }

    // Set up toolbar
    self.toolbarDelegate = [[HBQueueToolbarDelegate alloc] init];

    NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"HBQueueWindowToolbar2"];
    toolbar.delegate = self.toolbarDelegate;
    toolbar.allowsUserCustomization = YES;
    toolbar.autosavesConfiguration = YES;
    toolbar.displayMode = NSToolbarDisplayModeIconAndLabel;
    self.window.toolbar = toolbar;

    // Set up the child view controllers
    _splitViewController = [[NSSplitViewController alloc] init];
    _splitViewController.view.wantsLayer = YES;
    [_splitViewController.view setFrameSize:NSMakeSize(780, 500)];
    _splitViewController.splitView.vertical = YES;

    _tableViewController = [[HBQueueTableViewController alloc] initWithQueue:self.queue delegate:self];
    _containerViewController = [[HBQueueDetailsViewController alloc] init];
    _infoViewController = [[HBQueueInfoViewController alloc] initWithDelegate:self];
    _multiSelectionViewController = [[HBQueueMultiSelectionViewController alloc] init];

    NSSplitViewItem *tableItem = [NSSplitViewItem splitViewItemWithViewController:_tableViewController];
    tableItem.minimumThickness = 160;

    [_splitViewController addSplitViewItem:tableItem];

    NSSplitViewItem *detailsItem = [NSSplitViewItem splitViewItemWithViewController:_containerViewController];
    detailsItem.canCollapse = YES;
    detailsItem.minimumThickness = 240;

    [_splitViewController addSplitViewItem:detailsItem];

    _splitViewController.splitView.autosaveName = @"HBQueueSplitViewAutosave";
    _splitViewController.splitView.identifier = @"HBQueueSplitViewIdentifier";

    self.window.contentViewController = _splitViewController;
    self.window.frameAutosaveName = @"HBQueueWindowFrameAutosave";
    [self.window setFrameFromString:@"HBQueueWindowFrameAutosave"];

    // Set up observers
    [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidChangeStateNotification object:_queue queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        [self updateUI];
    }];

    [self updateUI];
    [self tableViewDidSelectItemsAtIndexes:[NSIndexSet indexSet]];
}

- (void)updateUI
{
    [self.toolbarDelegate updateToolbarButtonsState:self.queue toolbar:self.window.toolbar];
    [self.window.toolbar validateVisibleItems];

    [self _touchBar_updateButtonsState];
    [self _touchBar_validateUserInterfaceItems];

    NSString *subtitle;
    if (self.queue.pendingItemsCount == 0)
    {
        self.window.title = NSLocalizedString(@"Queue", @"Queue window title");
        subtitle = NSLocalizedString(@"No encode pending", @"Queue status");
    }
    else
    {
        if (self.queue.pendingItemsCount == 1)
        {
            subtitle = [NSString stringWithFormat: NSLocalizedString(@"%lu encode pending", @"Queue status"), (unsigned long)self.queue.pendingItemsCount];
        }
        else
        {
            subtitle = [NSString stringWithFormat: NSLocalizedString(@"%lu encodes pending", @"Queue status"), (unsigned long)self.queue.pendingItemsCount];
        }

        if (@available(macOS 11, *)) {} else
        {
            self.window.title = [NSString stringWithFormat: NSLocalizedString(@"Queue (%@)", @"Queue window title"), subtitle];
        }
    }

    if (@available(macOS 11, *))
    {
        self.window.subtitle = subtitle;
    }
}

#pragma mark Toolbar

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(toggleStartCancel:))
    {
        if (self.queue.isEncoding)
        {
            menuItem.title = NSLocalizedString(@"Stop Encoding", @"Queue -> start/stop menu");
            return YES;
        }
        else
        {
            menuItem.title = NSLocalizedString(@"Start Encoding", @"Queue -> start/stop menu");
            return self.queue.canEncode;
        }
    }

    if (action == @selector(togglePauseResume:))
    {
        if (self.queue.canPause)
        {
            menuItem.title = NSLocalizedString(@"Pause Encoding", @"Queue -> pause/resume menu");
        }
        else
        {
            menuItem.title = NSLocalizedString(@"Resume Encoding", @"Queue -> pause/resume men");
        }

        return self.queue.canPause || self.queue.canResume;
    }

    if (action == @selector(removeAll:) || action == @selector(resetAll:))
    {
        return self.queue.items.count > 0;
    }

    if (action == @selector(resetFailed:))
    {
        return self.queue.failedItemsCount > 0;
    }

    if (action == @selector(removeCompleted:))
    {
        return self.queue.completedItemsCount > 0;
    }

    return YES;
}

- (BOOL)validateUserIterfaceItemForAction:(SEL)action
{
    if (action == @selector(toggleStartCancel:) || action == @selector(_touchBar_toggleStartCancel:))
    {
        return self.queue.isEncoding || self.queue.canEncode;
    }

    if (action == @selector(togglePauseResume:))
    {
        return self.queue.canPause || self.queue.canResume;
    }

    if (action == @selector(toggleDetails:) ||
        action == @selector(toggleQuickLook:))
    {
        return YES;
    }

    return NO;
}

- (BOOL)validateToolbarItem:(NSToolbarItem *)theItem
{
    return [self validateUserIterfaceItemForAction:theItem.action];
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
    self.visible = self.window.occlusionState & NSWindowOcclusionStateVisible ? YES : NO;
}

#pragma mark - Private queue editing methods

/**
 * Delete encodes from the queue window and accompanying array
 * Also handling first cancelling the encode if in fact its currently encoding.
 */
- (void)removeQueueItemsAtIndexes:(NSIndexSet *)indexes
{
    if (indexes.count)
    {
        NSMutableIndexSet *mutableIndexes = [indexes mutableCopy];
        // if this is a currently encoding job, we need to be sure to alert the user,
        // to let them decide to cancel it first, then if they do, we can come back and
        // remove it
        NSIndexSet *workingIndexes = [self.queue.items HB_indexesOfObjectsUsingBlock:^BOOL(HBQueueJobItem *item) {
            return item.state == HBQueueItemStateWorking;
        }];

        NSIndexSet *workingSelectedIndexes = [workingIndexes HB_intersectionWith:indexes];
        [mutableIndexes removeIndexes:workingSelectedIndexes];

        if (workingSelectedIndexes.count)
        {
            NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It?", @"Queue Stop Alert -> stop and remove message")];

            // Which window to attach the sheet to?
            NSWindow *targetWindow = self.window;

            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:alertTitle];
            [alert setInformativeText:NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", @"Queue Stop Alert -> stop and remove informative text")];
            [alert addButtonWithTitle:NSLocalizedString(@"Keep Encoding", @"Queue Stop Alert -> stop and remove first button")];
            [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Delete", @"Queue Stop Alert -> stop and remove second button")];
            if (@available(macOS 11, *))
            {
                alert.buttons.lastObject.hasDestructiveAction = true;
            }
            [alert setAlertStyle:NSAlertStyleCritical];

            [alert beginSheetModalForWindow:targetWindow completionHandler:^(NSModalResponse returnCode) {
                if (returnCode == NSAlertSecondButtonReturn)
                {
                    [self.queue cancelItemsAtIndexes:workingSelectedIndexes];
                    [self.queue removeItemsAtIndexes:workingSelectedIndexes];
                }
            }];
        }

        // remove the non working items immediately
        [self.queue removeItemsAtIndexes:mutableIndexes];
    }
}

- (void)doEditQueueItem:(HBQueueJobItem *)item
{
    [self.queue prepareItemForEditingAtIndex:[self.queue.items indexOfObject:item]];

    [self.delegate openJob:[item.job copy] completionHandler:^(BOOL result) {
        NSInteger index = [self.queue.items indexOfObject:item];
        if (index != NSNotFound)
        {
            [self.queue resetItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
            if (result)
            {
                // Now that source is loaded and settings applied, delete the queue item from the queue
                [self.queue removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
            }
            else
            {
                NSBeep();
            }
        }
        else
        {
            item.state = HBQueueItemStateReady;
        }
    }];
}

/**
 * Send the selected queue item back to the main window for rescan and possible edit.
 */
- (void)editQueueItem:(HBQueueJobItem *)item
{
    // if this is a currently encoding item, we need to be sure to alert the user,
    // to let them decide to cancel it first, then if they do, we can come back and
    // remove it
    if (item.state == HBQueueItemStateWorking)
    {
        NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Edit It?", @"Queue Edit Alert -> stop and edit message")];

        // Which window to attach the sheet to?
        NSWindow *docWindow = self.window;

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:alertTitle];
        [alert setInformativeText:NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", @"Queue Edit Alert -> stop and edit informative text")];
        [alert addButtonWithTitle:NSLocalizedString(@"Keep Encoding", @"Queue Edit Alert -> stop and edit first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Edit", @"Queue Edit Alert -> stop and edit second button")];
        if (@available(macOS 11, *))
        {
            alert.buttons.lastObject.hasDestructiveAction = true;
        }
        [alert setAlertStyle:NSAlertStyleCritical];

        [alert beginSheetModalForWindow:docWindow completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSAlertSecondButtonReturn)
            {
                [self doEditQueueItem:item];
            }
        }];
    }
    else if (item.state != HBQueueItemStateWorking && item.state != HBQueueItemStateRescanning)
    {
        [self doEditQueueItem:item];
    }
}

- (void)resetQueueItemsAtIndexes:(NSIndexSet *)indexes
{
    [self.queue resetItemsAtIndexes:indexes];
}

#pragma mark - Encode Done Actions

NSString * const HBQueueItemNotificationPathKey = @"HBQueueItemNotificationPathKey";

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    // Show the file in Finder when a done notification was clicked.
    NSString *path = notification.userInfo[HBQueueItemNotificationPathKey];
    if ([path isKindOfClass:[NSString class]] && path.length)
    {
        NSURL *fileURL = [NSURL fileURLWithPath:path];
        [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[fileURL]];
    }
}

- (void)showNotificationWithTitle:(NSString *)title description:(NSString *)description url:(NSURL *)fileURL playSound:(BOOL)playSound
{
    NSUserNotification *notification = [[NSUserNotification alloc] init];
    notification.title = title;
    notification.informativeText = description;
    notification.soundName = playSound ? NSUserNotificationDefaultSoundName : nil;

    if (fileURL)
    {
        notification.hasActionButton = YES;
        notification.actionButtonTitle = NSLocalizedString(@"Show", @"Notification -> Show in Finder");
        notification.userInfo = @{ HBQueueItemNotificationPathKey: fileURL.path };
    }
    else
    {
        notification.hasActionButton = NO;
    }

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}

/**
 *  Sends the URL to the external app
 *  selected in the preferences.
 *
 *  @param job the job of the file to send
 */
- (void)sendToExternalApp:(HBQueueJobItem *)item
{
    // This end of encode action is called as each encode rolls off of the queue
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBSendToAppEnabled] == YES)
    {
        NSURL *destinationFolderURL = item.destinationFolderURL;
        NSString *destinationPath = item.destinationURL.path;

        dispatch_async(_sendQueue, ^{
#ifdef __SANDBOX_ENABLED__
            BOOL accessingSecurityScopedResource = [destinationFolderURL startAccessingSecurityScopedResource];
#endif

            NSWorkspace *workspace = NSWorkspace.sharedWorkspace;
            NSString *app = [workspace fullPathForApplication:[NSUserDefaults.standardUserDefaults objectForKey:HBSendToApp]];

            if (app)
            {
                if (![workspace openFile:destinationPath withApplication:app])
                {
                    [HBUtilities writeToActivityLog:"Failed to send file to: %s", app];
                }
            }
            else
            {
                [HBUtilities writeToActivityLog:"Send file to: app not found"];
            }

#ifdef __SANDBOX_ENABLED__
            if (accessingSecurityScopedResource)
            {
                [destinationFolderURL stopAccessingSecurityScopedResource];
            }
#endif
        });
    }
}

/**
 *  Runs the alert for a single job
 */
- (void)itemCompletedAlerts:(HBQueueJobItem *)item
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;

    if ([ud boolForKey:HBQueueNotificationWhenJobDone])
    {
        NSString *title;
        NSString *description;
        if (item.state == HBQueueItemStateCompleted)
        {
            title = NSLocalizedString(@"Put down that cocktail…", @"Queue notification alert message");
            description = [NSString stringWithFormat:NSLocalizedString(@"Your encode %@ is done!", @"Queue done notification message"),
                                     item.destinationFileName];

        }
        else
        {
            title = NSLocalizedString(@"Encode failed", @"Queue done notification failed message");
            description = [NSString stringWithFormat:NSLocalizedString(@"Your encode %@ couldn't be completed.", @"Queue done notification message"),
                           item.destinationFileName];
        }

        bool playSound = [ud boolForKey:HBQueueNotificationPlaySound];

        [self showNotificationWithTitle:title
                            description:description
                                    url:item.destinationURL
                              playSound:playSound];
    }

    if ([ud boolForKey:HBQueueAutoClearCompletedItems])
    {
        [self.queue removeCompletedItems];
    }
}

/**
 *  Runs the global queue completed alerts
 */
- (void)queueCompletedAlerts
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;

    if ([ud boolForKey:HBQueueNotificationWhenDone])
    {
        bool playSound = [ud boolForKey:HBQueueNotificationPlaySound];

        [self showNotificationWithTitle:NSLocalizedString(@"Put down that cocktail…", @"Queue notification alert message")
                            description:NSLocalizedString(@"Your queue is done!", @"Queue done notification message")
                                    url:nil
                              playSound:playSound];
    }

    // If sleep has been selected
    if ([ud integerForKey:HBQueueDoneAction] == HBDoneActionSleep)
    {
        // Sleep
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:@"tell application \"System Events\" to sleep"];
        [scriptObject executeAndReturnError:NULL];
    }

    // If Shutdown has been selected
    if ([ud integerForKey:HBQueueDoneAction] == HBDoneActionShutDown)
    {
        // Shut Down
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:@"tell application \"System Events\" to shut down"];
        [scriptObject executeAndReturnError:NULL];
    }

    // If Quit HB has been selected
    if ([ud integerForKey:HBQueueDoneAction] == HBDoneActionQuit)
    {
        [NSApp terminate:self];
    }
}

- (void)queueLowDiskSpaceAlert
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"Your destination disk is almost full.", @"Queue -> disk almost full alert message")];
    [alert setInformativeText:NSLocalizedString(@"You need to make more space available on your destination disk.",@"Queue -> disk almost full alert informative text")];
    [NSApp requestUserAttention:NSCriticalRequest];
    [alert runModal];
}

- (void)remindUserOfSleepOrShutdown
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;

    if ([ud integerForKey:HBQueueDoneAction] == HBDoneActionSleep)
    {
        // Warn that computer will sleep after encoding
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The computer will sleep after encoding is done.", @"Queue Done Alert -> sleep message")];
        [alert setInformativeText:NSLocalizedString(@"You have selected to sleep the computer after encoding. To turn off sleeping, go to the HandBrake preferences.", @"Queue Done Alert -> sleep informative text")];
        [alert addButtonWithTitle:NSLocalizedString(@"OK", @"Queue Done Alert -> sleep first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Preferences…", @"Queue Done Alert -> sleep second button")];

        NSInteger response = [alert runModal];
        if (response == NSAlertSecondButtonReturn)
        {
            [self.delegate showPreferencesWindow:nil];
        }

        [self promptForAppleEventAuthorization];
    }
    else if ([ud integerForKey:HBQueueDoneAction] == HBDoneActionShutDown)
    {
        // Warn that computer will shut down after encoding
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The computer will shut down after encoding is done.", @"Queue Done Alert -> shut down message")];
        [alert setInformativeText:NSLocalizedString(@"You have selected to shut down the computer after encoding. To turn off shut down, go to the HandBrake preferences.", @"Queue Done Alert -> shut down informative text")];
        [alert addButtonWithTitle:NSLocalizedString(@"OK", @"Queue Done Alert -> shut down first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Preferences…", @"Queue Done Alert -> shut down second button")];

        NSInteger response = [alert runModal];
        if (response == NSAlertSecondButtonReturn)
        {
            [self.delegate showPreferencesWindow:nil];
        }

        [self promptForAppleEventAuthorization];
    }
}

- (void)promptForAppleEventAuthorization
{
    HBPrivacyConsentState result = [HBUtilities determinePermissionToAutomateTarget:@"com.apple.systemevents" promptIfNeeded:YES];
    if (result != HBPrivacyConsentStateGranted)
    {
        [HBUtilities writeToActivityLog:"Failed to get permission to automate system events"];
    }
}

#pragma mark - UI Actions

/**
 * Rip: puts up an alert before ultimately calling doRip
 */
- (IBAction)toggleStartCancel:(id)sender
{
    // Rip or Cancel ?
    if (self.queue.isEncoding)
    {
        [self cancelRip:sender];
    }
    // If there are pending items in the queue, then this is a rip the queue
    else if (self.queue.canEncode)
    {
        // We check to see if we need to warn the user that the computer will go to sleep
        // or shut down when encoding is finished
        [self remindUserOfSleepOrShutdown];
        [self.queue start];
    }
}

/**
* Starts or cancels the processing of items depending on the current state
 * Displays an alert asking user if the want to cancel encoding of current item.
 */
- (IBAction)cancelRip:(id)sender
{
    // Which window to attach the sheet to?
    NSWindow *window = self.window;
    if ([sender respondsToSelector:@selector(window)])
    {
        window = [sender window];
    }

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"You are currently encoding. What would you like to do?", @"Queue Alert -> cancel rip message")];
    [alert setInformativeText:NSLocalizedString(@"Select Continue Encoding to dismiss this dialog without making changes.", @"Queue Alert -> cancel rip informative text")];
    [alert addButtonWithTitle:NSLocalizedString(@"Continue Encoding", @"Queue Alert -> cancel rip first button")];
    [alert addButtonWithTitle:NSLocalizedString(@"Skip Current Job", @"Queue Alert -> cancel rip second button")];
    if (@available(macOS 11, *))
    {
        alert.buttons.lastObject.hasDestructiveAction = YES;
    }
    [alert addButtonWithTitle:NSLocalizedString(@"Stop After Current Job", @"Queue Alert -> cancel rip third button")];
    [alert addButtonWithTitle:NSLocalizedString(@"Stop All", @"Queue Alert -> cancel rip fourth button")];
    if (@available(macOS 11, *))
    {
        alert.buttons.lastObject.hasDestructiveAction = YES;
    }
    [alert setAlertStyle:NSAlertStyleCritical];

    [alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertSecondButtonReturn)
        {
            [self.queue cancelCurrentAndContinue];
        }
        else if (returnCode == NSAlertThirdButtonReturn)
        {
            [self.queue finishCurrentAndStop];
        }
        else if (returnCode == NSAlertThirdButtonReturn + 1)
        {
            [self.queue cancelCurrentAndStop];
        }
    }];
}

/**
 * Toggles the pause/resume state of libhb
 */
- (IBAction)togglePauseResume:(id)sender
{
    if (self.queue.canResume)
    {
        [self.queue resume];
    }
    else if (self.queue.canPause)
    {
        [self.queue pause];
    }
}

- (IBAction)toggleDetails:(id)sender
{
    NSSplitViewItem *detailsItem = self.splitViewController.splitViewItems[1];
    detailsItem.animator.collapsed = !detailsItem.isCollapsed;
}

- (IBAction)toggleQuickLook:(id)sender
{
    if (QLPreviewPanel.sharedPreviewPanelExists && QLPreviewPanel.sharedPreviewPanel.isVisible)
    {
        [QLPreviewPanel.sharedPreviewPanel orderOut:sender];
    }
    else
    {
        [QLPreviewPanel.sharedPreviewPanel makeKeyAndOrderFront:sender];
    }
}

#pragma mark - table view controller delegate

- (void)tableViewDidSelectItemsAtIndexes:(NSIndexSet *)indexes
{
    NSUInteger count = indexes.count;

    if (count != 1)
    {
        self.multiSelectionViewController.count = count;
        [self switchToViewController:self.multiSelectionViewController];
    }
    else
    {
        NSArray<id<HBQueueItem>> *items = [self.queue.items objectsAtIndexes:indexes];
        self.infoViewController.item = items.firstObject;
        [self switchToViewController:self.infoViewController];
    }
}

- (void)switchToViewController:(NSViewController *)viewController
{
    NSViewController *firstChild = self.containerViewController.childViewControllers.firstObject;

    if (firstChild != viewController)
    {
        if (firstChild)
        {
            [firstChild.view removeFromSuperviewWithoutNeedingDisplay];
            [firstChild removeFromParentViewController];
        }

        [self.containerViewController addChildViewController:viewController];
        viewController.view.frame = self.containerViewController.view.bounds;
        viewController.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [self.containerViewController.view addSubview:viewController.view];
    }
}

- (void)tableViewEditItem:(id<HBQueueItem>)item
{
    if ([item isKindOfClass:[HBQueueJobItem class]])
    {
        [self editQueueItem:item];
    }
}

- (void)tableViewRemoveItemsAtIndexes:(nonnull NSIndexSet *)indexes
{
    [self removeQueueItemsAtIndexes:indexes];
}

- (void)tableViewResetItemsAtIndexes:(nonnull NSIndexSet *)indexes {
    [self resetQueueItemsAtIndexes:indexes];
}

- (void)detailsViewEditItem:(nonnull id<HBQueueItem>)item
{
    if ([item isKindOfClass:[HBQueueJobItem class]])
    {
        [self editQueueItem:item];
    }
}

- (void)detailsViewResetItem:(nonnull id<HBQueueItem>)item
{
    if ([item isKindOfClass:[HBQueueJobItem class]])
    {
        NSUInteger index = [self.queue.items indexOfObject:item];
        [self resetQueueItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
    }
}

- (IBAction)resetAll:(id)sender
{
    [self.queue resetAllItems];
}

- (IBAction)resetFailed:(id)sender
{
    [self.queue resetFailedItems];
}

- (IBAction)removeAll:(id)sender
{
    [self.queue removeNotWorkingItems];
}

- (IBAction)removeCompleted:(id)sender
{
    [self.queue removeCompletedItems];
}

@end

@implementation HBQueueController (TouchBar)

@dynamic touchBar;

static NSTouchBarItemIdentifier HBTouchBarMain = @"fr.handbrake.queueWindowTouchBar";

static NSTouchBarItemIdentifier HBTouchBarRip = @"fr.handbrake.rip";
static NSTouchBarItemIdentifier HBTouchBarPause = @"fr.handbrake.pause";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarRip, HBTouchBarPause];

    bar.customizationIdentifier = HBTouchBarMain;
    bar.customizationAllowedItemIdentifiers = @[HBTouchBarRip, HBTouchBarPause];

    return bar;
}

- (IBAction)_touchBar_toggleStartCancel:(id)sender
{
    [self toggleStartCancel:self];
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarRip])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Start/Stop Encoding", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPlayTemplate] target:self action:@selector(_touchBar_toggleStartCancel:)];

        item.view = button;
        return item;
    }
    else if ([identifier isEqualTo:HBTouchBarPause])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Pause/Resume Encoding", @"Touch bar");

        NSButton *button = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameTouchBarPauseTemplate] target:self action:@selector(togglePauseResume:)];

        item.view = button;
        return item;
    }

    return nil;
}

- (void)_touchBar_updateButtonsState
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
