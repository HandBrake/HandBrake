/* HBQueueToolbarDelegate.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueToolbarDelegate.h"
#import "HBQueueController.h"
#import "NSToolbar+HBAdditions.h"
#import "HBToolbarItem.h"
#import "HBPreferencesKeys.h"

@interface HBQueueToolbarDelegate ()

@property (nonatomic, nullable, weak) id target;

@property (nonatomic, readonly) id redConf;
@property (nonatomic, readonly) id greenConf;

@end

@implementation HBQueueToolbarDelegate

- (instancetype)initWithTarget:(id)target
{
    self = [super init];
    if (self)
    {
        _target = target;

        if (@available(macOS 12.0, *))
        {
            _redConf = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemRedColor]];
            _greenConf = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemGreenColor]];
        }
    }
    return self;
}

- (IBAction)setWhenDoneAction:(id)sender
{
    if ([sender respondsToSelector:@selector(selectedIndex)])
    {
        [NSUserDefaults.standardUserDefaults setInteger:[sender selectedIndex] forKey:HBQueueDoneAction];
    }
    else if ([sender respondsToSelector:@selector(tag)])
    {
        [NSUserDefaults.standardUserDefaults setInteger:[sender tag] forKey:HBQueueDoneAction];
    }
}

- (nullable NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    HBToolbarItemStyle style = HBToolbarItemStyleDefault;

    if ([itemIdentifier isEqualToString:TOOLBAR_START])
    {
        NSToolbarItem *item = [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"Start", @"Main Window Start Toolbar Item")
                                  paletteLabel:NSLocalizedString(@"Start/Stop Encoding", @"Main Window Start Toolbar Item")
                                    symbolName:@"play.fill"
                                         image:@"encode"
                                         style:style
                                        target:self.target
                                        action:@selector(toggleStartCancel:)];
        if (@available(macOS 13.0, *))
        {
            NSSet<NSString *> *labels = [NSSet setWithObjects:NSLocalizedString(@"Start", @"Main Window Start Toolbar Item"),
                                                              NSLocalizedString(@"Stop", @"Toolbar Start/Stop Item"), nil];
            item.possibleLabels = labels;
        }
        return item;
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_PAUSE])
    {
        NSToolbarItem *item = [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"Pause", @"Main Window Pause Toolbar Item")
                                  paletteLabel:NSLocalizedString(@"Pause/Resume Encoding", @"Main Window Pause Toolbar Item")
                                    symbolName:@"pause.fill"
                                         image:@"pauseencode"
                                         style:style
                                        target:self.target
                                        action:@selector(togglePauseResume:)];
        if (@available(macOS 13.0, *))
        {
            NSSet<NSString *> *labels = [NSSet setWithObjects:NSLocalizedString(@"Pause", @"Main Window Pause Toolbar Item"),
                                                              NSLocalizedString(@"Resume", @"Toolbar Pause Item"), nil];
            item.possibleLabels = labels;
        }
        return item;
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_WHEN_DONE])
    {
        NSArray<NSString *> *titles = @[NSLocalizedString(@"Do Nothing", @"Queue Window When Done Toolbar Item"),
                                        NSLocalizedString(@"Put Computer to Sleep", @"Queue Window When Done Toolbar Item"),
                                        NSLocalizedString(@"Shut Down Computer", @"Queue Window When Done Toolbar Item"),
                                        NSLocalizedString(@"Quit HandBrake", @"Queue Window When Done Toolbar Item")];
        NSString *label = NSLocalizedString(@"When Done", @"Queue Window When Done Toolbar Item");
        SEL action = @selector(setWhenDoneAction:);

        if (@available(macOS 10.15, *))
        {

            NSToolbarItemGroup *item = [NSToolbarItemGroup groupWithItemIdentifier:itemIdentifier
                                                                            titles:titles
                                                                     selectionMode:NSToolbarItemGroupSelectionModeSelectOne
                                                                            labels:titles
                                                                            target:self action:action];
            item.controlRepresentation = NSToolbarItemGroupControlRepresentationCollapsed;
            item.label = label;
            item.paletteLabel = label;

            [item bind:@"selectedIndex"
              toObject:NSUserDefaultsController.sharedUserDefaultsController
           withKeyPath:@"values.HBQueueDoneAction" options:nil];

            return item;
        }
        else
        {
            NSPopUpButton *popUpButton = [[NSPopUpButton alloc] init];
            popUpButton.bezelStyle = NSBezelStyleTexturedRounded;
            popUpButton.menu.autoenablesItems = NO;

            NSUInteger index = 0;
            for (NSString *title in titles)
            {
                NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:@""];
                item.tag = index;
                item.action = action;
                item.target = self;
                [popUpButton.menu addItem:item];
                index += 1;
            }

            [popUpButton bind:@"selectedTag"
                     toObject:NSUserDefaultsController.sharedUserDefaultsController
                  withKeyPath:@"values.HBQueueDoneAction" options:nil];

            NSToolbarItem *item = [[HBToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
            item.label = label;
            item.paletteLabel = label;
            item.view = popUpButton;
            item.minSize = NSMakeSize(200, 16);

            return item;
        }
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_ACTION])
    {
        NSMenu *menu = [[NSMenu alloc] init];
        [menu addItemWithTitle:NSLocalizedString(@"Reset All Jobs", @"Queue Window Action Toolbar Item")
                        action:@selector(resetAll:) keyEquivalent:@""];
        [menu addItemWithTitle:NSLocalizedString(@"Reset Failed Jobs", @"Queue Window Action Toolbar Item")
                        action:@selector(resetFailed:) keyEquivalent:@""];
        [menu addItem:[NSMenuItem separatorItem]];
        [menu addItemWithTitle:NSLocalizedString(@"Remove All Jobs", @"Queue Window Action Toolbar Item")
                        action:@selector(removeAll:) keyEquivalent:@""];
        [menu addItemWithTitle:NSLocalizedString(@"Remove Completed Jobs", @"Queue Window Action Toolbar Item")
                        action:@selector(removeCompleted:) keyEquivalent:@""];
        NSString *label = NSLocalizedString(@"Action", @"Queue Window Action Toolbar Item");

        if (@available(macOS 10.15, *))
        {
            NSMenuToolbarItem *item = [[NSMenuToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
            item.label = label;
            item.paletteLabel = label;
            item.menu = menu;
            [item HB_setSymbol:@"ellipsis.circle" configuration:nil fallbackImage:@"NSActionTemplate"];
            return item;
        }
        else
        {
            NSMenuItem *imageMenu = [[NSMenuItem alloc] init];
            imageMenu.image = [NSImage imageNamed:@"NSActionTemplate"];
            imageMenu.title = @"";
            [menu insertItem:imageMenu atIndex:0];

            NSPopUpButton *popUpButton = [[NSPopUpButton alloc] init];
            popUpButton.bezelStyle = NSBezelStyleTexturedRounded;
            popUpButton.pullsDown = YES;
            popUpButton.menu = menu;

            NSToolbarItem *item = [[HBToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
            item.label = label;
            item.paletteLabel = label;
            item.view = popUpButton;
            item.minSize = NSMakeSize(48, 16);

            return item;
        }
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_DETAILS])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Details", @"Queue Window Details Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Details", @"Queue Window Details Toolbar Item")
                                                symbolName:@"sidebar.right"
                                                     image:@"details"
                                                     style:HBToolbarItemStyleBordered | HBToolbarItemStyleButton
                                                    target:self.target
                                                    action:@selector(toggleDetails:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_QUICKLOOK])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"Quick Look", @"Queue Window Quick Look Toolbar Item")
                                  paletteLabel:NSLocalizedString(@"Quick Look", @"Queue Window Quick Look Toolbar Item")
                                    symbolName:@"eye"
                                         image:@"NSQuickLookTemplate"
                                         style:HBToolbarItemStyleBordered | HBToolbarItemStyleButton
                                        target:self.target
                                        action:@selector(toggleQuickLook:)];
    }
    return nil;
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
    return @[TOOLBAR_START, TOOLBAR_PAUSE, NSToolbarFlexibleSpaceItemIdentifier, TOOLBAR_WHEN_DONE, TOOLBAR_ACTION, TOOLBAR_DETAILS];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
    return @[TOOLBAR_START, TOOLBAR_PAUSE, TOOLBAR_WHEN_DONE, TOOLBAR_ACTION, TOOLBAR_DETAILS, TOOLBAR_QUICKLOOK, NSToolbarSpaceItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier];
}

- (void)updateToolbarButtonsState:(HBQueue *)queue toolbar:(NSToolbar *)toolbar
{
    NSToolbarItem *startToolbarItem = [toolbar HB_toolbarItemWithIdentifier:TOOLBAR_START];
    NSToolbarItem *pauseToolbarItem = [toolbar HB_toolbarItemWithIdentifier:TOOLBAR_PAUSE];

    if (queue.canResume)
    {
        [pauseToolbarItem HB_setSymbol:@"play.fill" configuration:self.greenConf fallbackImage:@"encode"];
        pauseToolbarItem.label = NSLocalizedString(@"Resume", @"Toolbar Pause Item");
        pauseToolbarItem.toolTip = NSLocalizedString(@"Resume Encoding", @"Toolbar Pause Item");
    }
    else
    {
        [pauseToolbarItem HB_setSymbol:@"pause.fill" configuration:self.greenConf fallbackImage:@"pauseencode"];
        pauseToolbarItem.label = NSLocalizedString(@"Pause", @"Toolbar Pause Item");
        pauseToolbarItem.toolTip = NSLocalizedString(@"Pause Encoding", @"Toolbar Pause Item");

    }
    if (queue.isEncoding)
    {
        [startToolbarItem HB_setSymbol:@"stop.fill" configuration:self.redConf fallbackImage:@"stopencode"];
        startToolbarItem.label = NSLocalizedString(@"Stop", @"Toolbar Start/Stop Item");
        startToolbarItem.toolTip = NSLocalizedString(@"Stop Encoding", @"Toolbar Start/Stop Item");
    }
    else
    {
        [startToolbarItem HB_setSymbol:@"play.fill" configuration:self.greenConf fallbackImage:@"encode"];
        startToolbarItem.label = NSLocalizedString(@"Start", @"Toolbar Start/Stop Item");
        startToolbarItem.toolTip = NSLocalizedString(@"Start Encoding", @"Toolbar Start/Stop Item");
    }
}

@end
