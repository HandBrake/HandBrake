/*  HBAppDelegate.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAppDelegate.h"

@import HandBrakeKit;

#import "HBQueue.h"

#import "HBUtilities.h"
#import "HBPresetsMenuBuilder.h"

#import "HBPreferencesController.h"
#import "HBQueueController.h"
#import "HBQueueDockTileController.h"
#import "HBOutputPanelController.h"
#import "HBController.h"

#define PRESET_FILE @"UserPresets.json"
#define QUEUE_FILE @"Queue.hbqueue"

@interface HBAppDelegate () <NSMenuItemValidation>

@property (nonatomic, strong) HBPresetsManager *presetsManager;
@property (nonatomic, strong) HBPresetsMenuBuilder *presetsMenuBuilder;
@property (nonatomic, unsafe_unretained) IBOutlet NSMenu *presetsMenu;

@property (nonatomic, strong) HBPreferencesController *preferencesController;

@property (nonatomic, strong) HBQueue *queue;
@property (nonatomic, strong) HBQueueController *queueController;
@property (nonatomic, strong) HBQueueDockTileController *queueDockTileController;

@property (nonatomic, strong) HBOutputPanelController *outputPanel;

@property (nonatomic, strong) HBController *mainController;

@end

@implementation HBAppDelegate

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Register the default preferences
        [HBPreferencesController registerUserDefaults];

        _outputPanel = [[HBOutputPanelController alloc] init];

        [HBCore initGlobal];
        [HBCore registerErrorHandler:^(NSString *error) {
            fprintf(stderr, "error: %s\n", error.UTF8String);
        }];
        [HBCore setDVDNav:[NSUserDefaults.standardUserDefaults boolForKey:HBUseDvdNav]];

        // we init the HBPresetsManager
        NSURL *appSupportURL = HBUtilities.appSupportURL;
        _presetsManager = [[HBPresetsManager alloc] initWithURL:[appSupportURL URLByAppendingPathComponent:PRESET_FILE]];

        // Queue
        _queue = [[HBQueue alloc] initWithURL:[appSupportURL URLByAppendingPathComponent:QUEUE_FILE]];
        _queueController = [[HBQueueController alloc] initWithQueue:_queue];
        _queueController.delegate = self;
        _queueDockTileController = [[HBQueueDockTileController alloc] initWithQueue:_queue dockTile:NSApplication.sharedApplication.dockTile image:NSApplication.sharedApplication.applicationIconImage];
        _mainController = [[HBController alloc] initWithDelegate:self queue:_queue presetsManager:_presetsManager];
    }
    return self;
}

#pragma mark - NSApplicationDelegate

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
    return YES;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
    if (!flag)
    {
        [self.mainController showWindow:nil];
    }
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    NSApplication.sharedApplication.automaticCustomizeTouchBarMenuItemEnabled = YES;
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;

    if ([ud boolForKey:HBQueueAutoClearCompletedItemsAtLaunch])
    {
        [self.queue removeCompletedAndCancelledItems];
    }

    // Reset "When done" action
    if ([ud boolForKey:HBResetWhenDoneOnLaunch])
    {
        [ud setInteger:HBDoneActionDoNothing forKey:HBQueueDoneAction];
    }


    self.presetsMenuBuilder = [[HBPresetsMenuBuilder alloc] initWithMenu:self.presetsMenu
                                                                  action:@selector(selectPresetFromMenu:)
                                                                    size:NSFont.systemFontSize
                                                          presetsManager:self.presetsManager];
    [self.presetsMenuBuilder build];

    // Open debug output window now if it was visible when HB was closed
    if ([ud boolForKey:@"OutputPanelIsOpen"])
    {
        [self showOutputPanel:nil];
    }

    // Now we re-check the queue array to see if there are
    // any remaining encodes to be done
    if (self.queue.items.count)
    {
        [self showMainWindow:self];
        [self showQueueWindow:self];
    }
    else
    {
        // Open queue window now if it was visible when HB was closed
        if ([ud boolForKey:@"QueueWindowIsOpen"])
        {
            [self showQueueWindow:nil];
        }

        [self showMainWindow:self];
        [self.mainController launchAction];
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // Remove encodes logs older than a month
        if ([ud boolForKey:HBClearOldLogs])
        {
            [self cleanEncodeLogs];
        }

        [self cleanPreviews];
    });
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app
{
    if (self.queue.isEncoding)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Are you sure you want to quit HandBrake?", @"Quit Alert -> message")];
        [alert setInformativeText:NSLocalizedString(@"If you quit HandBrake your current encode will be reloaded into your queue at next launch. Do you want to quit anyway?", @"Quit Alert -> informative text")];
        [alert addButtonWithTitle:NSLocalizedString(@"Quit", @"Quit Alert -> first button")];
        [alert addButtonWithTitle:NSLocalizedString(@"Don't Quit", @"Quit Alert -> second button")];
        [alert.buttons[1] setKeyEquivalent:@"\E"];
        [alert setAlertStyle:NSAlertStyleCritical];

        NSInteger result = [alert runModal];

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

- (void)applicationWillTerminate:(NSNotification *)notification
{
    [self.presetsManager savePresets];

    [NSUserDefaults.standardUserDefaults setBool:_queueController.window.isVisible forKey:@"QueueWindowIsOpen"];
    [NSUserDefaults.standardUserDefaults setBool:_outputPanel.window.isVisible forKey:@"OutputPanelIsOpen"];

    _mainController = nil;
    _queueController = nil;
    [_queue invalidateWorkers];
    _queue = nil;

    [HBCore closeGlobal];
}

- (void)application:(NSApplication *)sender openURLs:(nonnull NSArray<NSURL *> *)urls
{
    BOOL recursive = [NSUserDefaults.standardUserDefaults boolForKey:HBRecursiveScan];
    [self.mainController openURLs:urls recursive:recursive];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(toggleStartCancel:) || action == @selector(togglePauseResume:))
    {
        // Delegate the validation to the queue controller
        return [self.queueController validateMenuItem:menuItem];
    }
    else if (action == @selector(showAddPresetPanel:) ||
             action == @selector(showPreviewWindow:) ||
             action == @selector(browseSources:))
    {
        // Delegate the validation to the main controller
        return [self.mainController validateMenuItem:menuItem];
    }

    return YES;
}

#pragma mark - Clean ups

/**
 *  Clears the EncodeLogs folder, removes the logs
 *  older than a month.
 */
- (void)cleanEncodeLogs
{
    NSURL *directoryUrl = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"EncodeLogs"];

    if (directoryUrl)
    {
        NSFileManager *manager = [[NSFileManager alloc] init];

        NSArray<NSURL *> *contents = [manager contentsOfDirectoryAtURL:directoryUrl
                                            includingPropertiesForKeys:nil
                                                               options:NSDirectoryEnumerationSkipsSubdirectoryDescendants |
                                                                        NSDirectoryEnumerationSkipsHiddenFiles |
                                                                        NSDirectoryEnumerationSkipsPackageDescendants
                                                                 error:NULL];

        NSDate *limit = [NSDate dateWithTimeIntervalSinceNow: -(60 * 60 * 24 * 7)];

        for (NSURL *fileURL in contents)
        {
            NSDate *creationDate = nil;
            [fileURL getResourceValue:&creationDate forKey:NSURLCreationDateKey error:NULL];
            if ([creationDate isLessThan:limit])
            {
                [manager removeItemAtURL:fileURL error:NULL];
            }
        }
    }
}

- (void)cleanPreviews
{
    NSURL *previewDirectory = [HBUtilities.appSupportURL URLByAppendingPathComponent:@"Previews"];

    if (previewDirectory)
    {
        NSFileManager *manager = [[NSFileManager alloc] init];
        NSArray<NSURL *> *contents = [manager contentsOfDirectoryAtURL:previewDirectory
                                            includingPropertiesForKeys:nil
                                                               options:NSDirectoryEnumerationSkipsSubdirectoryDescendants | NSDirectoryEnumerationSkipsPackageDescendants
                                                                 error:NULL];

        for (NSURL *url in contents)
        {
            NSError *error = nil;
            BOOL result = [manager removeItemAtURL:url error:&error];
            if (result == NO && error)
            {
                [HBUtilities writeToActivityLog:"Could not remove existing preview at: %s", url.lastPathComponent.UTF8String];
            }
        }
    }
}

#pragma mark - Rescan job

- (void)openJob:(HBJob *)job completionHandler:(void (^)(BOOL result))handler
{
    [self.mainController openJob:job completionHandler:handler];
}

#pragma mark - Menu actions

- (IBAction)toggleStartCancel:(id)sender
{
    [self.queueController toggleStartCancel:sender];
}

- (IBAction)togglePauseResume:(id)sender
{
    [self.queueController togglePauseResume:sender];
}

- (IBAction)browseSources:(id)sender
{
    [self.mainController browseSources:sender];
}

#pragma mark - Presets Menu actions

/**
 * We use this method to recreate new, updated factory presets
 */
- (IBAction)addFactoryPresets:(id)sender
{
    [self.presetsManager generateBuiltInPresets];
}

- (IBAction)reloadPreset:(id)sender
{
    [self.mainController reloadPreset:sender];
}

#pragma mark - Show Window Menu Items

/**
 * Shows preferences window.
 */
- (IBAction)showPreferencesWindow:(id)sender
{
    if (_preferencesController == nil)
    {
        _preferencesController = [[HBPreferencesController alloc] init];
    }

    [self.preferencesController showWindow:sender];
}

/**
 * Shows queue window.
 */
- (IBAction)showQueueWindow:(id)sender
{
    [self.queueController showWindow:sender];
}

/**
 * Shows debug output window.
 */
- (IBAction)showOutputPanel:(id)sender
{
    [self.outputPanel showWindow:sender];
}

- (IBAction)showPreviewWindow:(id)sender
{
    [self.mainController showPreviewWindow:sender];
}

/**
 * Shows main window.
 */
- (IBAction)showMainWindow:(id)sender
{
    [self.mainController showWindow:sender];
}

- (IBAction)openHomepage:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://handbrake.fr/"]];
}

- (IBAction)openForums:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://forum.handbrake.fr/"]];
}
- (IBAction)openUserGuide:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:HBUtilities.documentationURL];
}

@end

@interface NSApplication (TouchBar) <NSTouchBarDelegate>
@end

@implementation NSApplication (TouchBar)

static NSTouchBarItemIdentifier HBTouchBarMain = @"fr.handbrake.appDelegateTouchBar";
static NSTouchBarItemIdentifier HBTouchBarOpen = @"fr.handbrake.openSource";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarOpen];

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarOpen])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Open Source", @"Touch bar");

        NSButton *button = [NSButton buttonWithTitle:NSLocalizedString(@"Open Source", @"Touch bar") target:nil action:@selector(browseSources:)];

        item.view = button;
        return item;
    }

    return nil;
}

@end
