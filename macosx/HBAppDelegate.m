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

@interface HBAppDelegate ()

@property (nonatomic, retain) HBPresetsManager *presetsManager;
@property (assign) IBOutlet NSMenu *presetsMenu;

@property (nonatomic, retain) HBPreferencesController *preferencesController;
@property (nonatomic, retain) HBQueueController *queueController;

@property (nonatomic, retain) HBOutputPanelController *outputPanel;

@property (nonatomic, retain) HBController *mainController;

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

        // Lets report the HandBrake version number here to the activity log and text log file
        NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
        NSString *versionStringFull = [NSString stringWithFormat:@"Handbrake Version: %@  (%@)", infoDict[@"CFBundleShortVersionString"], infoDict[@"CFBundleVersion"]];
        [HBUtilities writeToActivityLog: "%s", versionStringFull.UTF8String];

        // we init the HBPresetsManager
        NSURL *presetsURL = [NSURL fileURLWithPath:[[HBUtilities appSupportPath] stringByAppendingPathComponent:@"UserPresets.plist"]];
        _presetsManager = [[HBPresetsManager alloc] initWithURL:presetsURL];

        _queueController = [[HBQueueController alloc] init];
        _queueController.outputPanel = _outputPanel;
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
    // Updates built-ins presets if needed
    [self checkBuiltInsForUpdates];
    [self buildPresetsMenu];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(buildPresetsMenu) name:HBPresetsChangedNotification object:nil];

    // Get the number of HandBrake instances currently running
    NSUInteger instances = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]].count;

    // If we are a single instance it is safe to clean up the previews if there are any
    // left over. This is a bit of a kludge but will prevent a build up of old instance
    // live preview cruft. No danger of removing an active preview directory since they
    // are created later in HBPreviewController if they don't exist at the moment a live
    // preview encode is initiated.
    if (instances == 1)
    {
        NSString *previewDirectory = [[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Previews"];
        NSError *error = nil;
        NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:previewDirectory error:&error];
        for (NSString *file in files)
        {
            BOOL result = [[NSFileManager defaultManager] removeItemAtPath:[previewDirectory stringByAppendingPathComponent:file] error:&error];
            if (result == NO && error)
            {
                [HBUtilities writeToActivityLog: "Could not remove existing preview at : %s", file.UTF8String];
            }
        }
    }

    [self showMainWindow:self];

    // Now we re-check the queue array to see if there are
    // any remaining encodes to be done in it and ask the
    // user if they want to reload the queue
    if (self.queueController.count)
    {
        // On Screen Notification
        // We check to see if there is already another instance of hb running.
        // Note: hbInstances == 1 means we are the only instance of HandBrake.app
        NSAlert *alert = nil;
        if (instances > 1)
        {
            alert = [[NSAlert alloc] init];
            [alert setMessageText:NSLocalizedString(@"There is already an instance of HandBrake running.", @"")];
            [alert setInformativeText:NSLocalizedString(@"HandBrake will now load up the existing queue.", nil)];
            [alert addButtonWithTitle:NSLocalizedString(@"Reload Queue", nil)];
        }
        else
        {
            if (self.queueController.workingItemsCount > 0 || self.queueController.pendingItemsCount > 0)
            {
                NSString *alertTitle;

                if (self.queueController.workingItemsCount > 0)
                {
                    alertTitle = [NSString stringWithFormat:
                                  NSLocalizedString(@"HandBrake Has Detected %d Previously Encoding Item(s) and %d Pending Item(s) In Your Queue.", @""),
                                  self.queueController.workingItemsCount, self.queueController.pendingItemsCount];
                }
                else
                {
                    alertTitle = [NSString stringWithFormat:
                                  NSLocalizedString(@"HandBrake Has Detected %d Pending Item(s) In Your Queue.", @""),
                                  self.queueController.pendingItemsCount];
                }

                alert = [[NSAlert alloc] init];
                [alert setMessageText:alertTitle];
                [alert setInformativeText:NSLocalizedString(@"Do you want to reload them ?", nil)];
                [alert addButtonWithTitle:NSLocalizedString(@"Reload Queue", nil)];
                [alert addButtonWithTitle:NSLocalizedString(@"Empty Queue", nil)];
                [alert setAlertStyle:NSCriticalAlertStyle];
            }
            else
            {
                // Since we addressed any pending or previously encoding items above, we go ahead and make sure
                // the queue is empty of any finished items or cancelled items.
                [self.queueController removeAllJobs];
                [self.mainController launchAction];
            }
        }

        if (alert)
        {
            NSModalResponse response = [alert runModal];

            if (response == NSAlertSecondButtonReturn)
            {
                [HBUtilities writeToActivityLog:"didDimissReloadQueue NSAlertSecondButtonReturn Chosen"];
                [self.queueController removeAllJobs];
                [self.mainController launchAction];
            }
            else
            {
                [HBUtilities writeToActivityLog:"didDimissReloadQueue NSAlertFirstButtonReturn Chosen"];
                if (instances == 1)
                {
                    [self.queueController setEncodingJobsAsPending];
                }

                [self showQueueWindow:nil];
            }

            [alert release];
        }
    }
    else
    {
        [self.mainController launchAction];
    }

    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showOutputPanel:nil];

    // Open queue window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
        [self showQueueWindow:nil];
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
    else if (self.queueController.pendingItemsCount > 0)
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

- (void)applicationWillTerminate:(NSNotification *)notification
{
    [self.presetsManager savePresets];

    [_mainController release];
    _mainController = nil;
    [_queueController release];
    _queueController = nil;

    [HBCore closeGlobal];
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
    [self.mainController openFile:[NSURL fileURLWithPath:filenames.firstObject]];
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
    else if (action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:) ||
             action == @selector(showPreviewWindow:) || action == @selector(browseSources:))
    {
        // Delegate the validation to the main controller
        return [self.mainController validateMenuItem:menuItem];
    }

    return YES;
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

- (void)checkBuiltInsForUpdates
{
    // if we have built in presets to update, then do so AlertBuiltInPresetUpdate
    if ([self.presetsManager checkBuiltInsForUpdates])
    {
        if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertBuiltInPresetUpdate"] == YES)
        {
            // Show an alert window that built in presets will be updated
            [NSApp requestUserAttention:NSCriticalRequest];
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"HandBrake has determined your built in presets are out of date…"];
            [alert setInformativeText:@"HandBrake will now update your built-in presets."];
            [alert runModal];
            [alert release];
        }
        // when alert is dismissed, go ahead and update the built in presets
        [self.presetsManager generateBuiltInPresets];
    }
}

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
    [menuItems release];

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
                 [self.presetsMenu addItem:[NSMenuItem separatorItem]];
                 builtInEnded = YES;
             }

             item.indentationLevel = idx.length - 1;

             [self.presetsMenu addItem:item];
             [item release];
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
    [self.outputPanel showOutputPanel:sender];
}

- (IBAction)showPicturePanel:(id)sender
{
    [self.mainController showPicturePanel:self];
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
