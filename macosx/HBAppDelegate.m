/*  HBAppDelegate.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAppDelegate.h"

#import "HBUtilities.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"

#import "HBPreferencesController.h"
#import "HBQueueController.h"
#import "HBOutputPanelController.h"
#import "HBCore.h"
#import "HBController.h"

#define PRESET_FILE @"UserPresets.json"
#define QUEUE_FILE @"Queue.hbqueue"

@interface HBAppDelegate ()

@property (nonatomic, strong) HBPresetsManager *presetsManager;
@property (unsafe_unretained) IBOutlet NSMenu *presetsMenu;

@property (nonatomic, strong) HBPreferencesController *preferencesController;
@property (nonatomic, strong) HBQueueController *queueController;

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

        [HBCore initGlobal];
        [HBCore registerErrorHandler:^(NSString *error) {
            fprintf(stderr, "error: %s\n", error.UTF8String);
        }];
        [HBCore setDVDNav:[[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue]];

        _outputPanel = [[HBOutputPanelController alloc] init];

        // we init the HBPresetsManager
        NSURL *appSupportURL = [HBUtilities appSupportURL];
        _presetsManager = [[HBPresetsManager alloc] initWithURL:[appSupportURL URLByAppendingPathComponent:PRESET_FILE]];

        // Queue
        _queueController = [[HBQueueController alloc] initWithURL:[appSupportURL URLByAppendingPathComponent:QUEUE_FILE]];
        _queueController.delegate = self;
        _mainController = [[HBController alloc] initWithQueue:_queueController presetsManager:_presetsManager];

        // Set the Growl Delegate
        [GrowlApplicationBridge setGrowlDelegate:_queueController];
    }
    return self;
}

#pragma mark - NSApplicationDelegate

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
    [self buildPresetsMenu];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(buildPresetsMenu) name:HBPresetsChangedNotification object:nil];

    // Get the number of HandBrake instances currently running
    NSUInteger instances = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]].count;

    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showOutputPanel:nil];

    // On Screen Notification
    // We check to see if there is already another instance of hb running.
    if (instances > 1)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"There is already an instance of HandBrake running.", nil)];
        [alert setInformativeText:NSLocalizedString(@"The queue will be shared between the instances.", nil)];
        [alert runModal];
    }
    else
    {
        [self.queueController setEncodingJobsAsPending];
        [self.queueController removeCompletedJobs];
    }

    // Now we re-check the queue array to see if there are
    // any remaining encodes to be done
    if (self.queueController.count)
    {
        [self showMainWindow:self];
        [self showQueueWindow:self];
    }
    else
    {
        // Open queue window now if it was visible when HB was closed
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
            [self showQueueWindow:nil];

        [self showMainWindow:self];
        [self.mainController launchAction];
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // Remove encodes logs older than a month
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBClearOldLogs"])
        {
            [self cleanEncodeLogs];
        }

        // If we are a single instance it is safe to clean up the previews if there are any
        // left over. This is a bit of a kludge but will prevent a build up of old instance
        // live preview cruft. No danger of removing an active preview directory since they
        // are created later in HBPreviewController if they don't exist at the moment a live
        // preview encode is initiated.
        if (instances == 1)
        {
            [self cleanPreviews];
        }
    });
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app
{
    if (self.queueController.core.state != HBStateIdle)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil)];
        [alert setInformativeText:NSLocalizedString(@"If you quit HandBrake your current encode will be reloaded into your queue at next launch. Do you want to quit anyway?", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Quit", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Don't Quit", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

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

    [[NSUserDefaults standardUserDefaults] setBool:_queueController.window.isVisible forKey:@"QueueWindowIsOpen"];
    [[NSUserDefaults standardUserDefaults] setBool:_outputPanel.window.isVisible forKey:@"OutputPanelIsOpen"];

    _mainController = nil;
    _queueController = nil;

    [HBCore closeGlobal];
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    [self.mainController openURL:[NSURL fileURLWithPath:filenames.firstObject]];
    [NSApp replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(rip:) || action == @selector(pause:))
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
    NSURL *directoryUrl = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"EncodeLogs"];

    if (directoryUrl)
    {
        NSArray *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:directoryUrl
                                                          includingPropertiesForKeys:nil
                                                                             options:NSDirectoryEnumerationSkipsSubdirectoryDescendants |
                                                                                     NSDirectoryEnumerationSkipsHiddenFiles |
                                                                                     NSDirectoryEnumerationSkipsPackageDescendants
                                                                               error:NULL];

        NSDate *limit = [NSDate dateWithTimeIntervalSinceNow: -(60 * 60 * 24 * 30)];
        NSFileManager *manager = [[NSFileManager alloc] init];

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
    NSURL *previewDirectory = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"Previews"];

    if (previewDirectory)
    {
        NSArray *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:previewDirectory
                                                          includingPropertiesForKeys:nil
                                                                             options:NSDirectoryEnumerationSkipsSubdirectoryDescendants |
                                                                                     NSDirectoryEnumerationSkipsPackageDescendants
                                                                               error:NULL];

        NSFileManager *manager = [[NSFileManager alloc] init];
        for (NSURL *url in contents)
        {
            NSError *error = nil;
            BOOL result = [manager removeItemAtURL:url error:&error];
            if (result == NO && error)
            {
                [HBUtilities writeToActivityLog: "Could not remove existing preview at : %s", url.lastPathComponent.UTF8String];
            }
        }
    }
}

#pragma mark - Menu actions

- (IBAction)rip:(id)sender
{
    [self.queueController rip:self];
}

- (IBAction)pause:(id)sender
{
    [self.queueController togglePauseResume:self];
}

- (IBAction)browseSources:(id)sender
{
    [self.mainController browseSources:self];
}

#pragma mark - Presets Menu actions

/**
 *  Adds the presets list to the menu.
 */
- (void)buildPresetsMenu
{
    // First we remove all the preset menu items
    // inserted previosly
    NSArray *menuItems = [self.presetsMenu.itemArray copy];
    for (NSMenuItem *item in menuItems)
    {
        if (item.tag != -1)
        {
            [self.presetsMenu removeItem:item];
        }
    }

    __block NSUInteger i = 0;
    __block BOOL builtInEnded = NO;
    [self.presetsManager.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
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
             if ([obj isEqualTo:self.presetsManager.defaultPreset])
             {
                 NSAttributedString *newTitle = [[NSAttributedString alloc] initWithString:[obj name]
                                                                                attributes:@{NSFontAttributeName: [NSFont boldSystemFontOfSize:14]}];
                 [item setAttributedTitle:newTitle];
             }
             // Add a separator line after the last builtIn preset
             if ([obj isBuiltIn] == NO && builtInEnded == NO)
             {
                 [self.presetsMenu addItem:[NSMenuItem separatorItem]];
                 builtInEnded = YES;
             }

             item.indentationLevel = idx.length - 1;

             [self.presetsMenu addItem:item];
         }
     }];
}

/**
 * We use this method to recreate new, updated factory presets
 */
- (IBAction)addFactoryPresets:(id)sender
{
    [self.presetsManager generateBuiltInPresets];
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

    [self.preferencesController showWindow:self];
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
    [self.mainController showPreviewWindow:self];
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
    [[NSWorkspace sharedWorkspace] openURL:[NSURL
                                            URLWithString:@"http://handbrake.fr/"]];
}

- (IBAction)openForums:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL
                                            URLWithString:@"http://forum.handbrake.fr/"]];
}
- (IBAction)openUserGuide:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL
                                            URLWithString:@"http://trac.handbrake.fr/wiki/HandBrakeGuide"]];
}

@end
