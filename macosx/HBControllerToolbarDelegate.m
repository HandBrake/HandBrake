/* HBControllerToolbarDelegate.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBControllerToolbarDelegate.h"
#import "NSToolbar+HBAdditions.h"
#import "HBToolbarItem.h"
#import "HBSegmentedControl.h"
#import "HBAppDelegate.h"
#import "HBController.h"


@interface HBControllerToolbarDelegate ()

@property (nonatomic, readonly) id redConf;
@property (nonatomic, readonly) id greenConf;

@end

@implementation HBControllerToolbarDelegate

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        if (@available(macOS 12.0, *))
        {
            _redConf = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemRedColor]];
            _greenConf = [NSImageSymbolConfiguration configurationWithPaletteColors:@[NSColor.systemGreenColor]];
        }
    }
    return self;
}

- (nullable NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    HBToolbarItemStyle style = HBToolbarItemStyleDefault;

    if ([itemIdentifier isEqualToString:TOOLBAR_OPEN])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Open Source", @"Main Window Open Toolbar Item")
                                              paletteLabel:nil
                                                symbolName:@"film"
                                                     image:@"source"
                                                     style:style
                                                    action:@selector(browseSources:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_ADD])
    {
        NSString *label = NSLocalizedString(@"Add To Queue", @"Main Window Add Toolbar Item");
        SEL action = @selector(addToQueue:);

        NSMenu *menu = [[NSMenu alloc] init];
        [menu addItemWithTitle:NSLocalizedString(@"Add Titles To Queue…", @"Main Window Add Toolbar Item")
                        action:@selector(addTitlesToQueue:) keyEquivalent:@""];
        [menu addItemWithTitle:NSLocalizedString(@"Add All Titles To Queue", @"Main Window Add Toolbar Item")
                        action:@selector(addAllTitlesToQueue:) keyEquivalent:@""];

        if (@available(macOS 11, *))
        {
            NSToolbarItem *item = [[HBToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
            item.label = label;
            item.paletteLabel = label;

            if (@available(macOS 14, *))
            {
                NSImage *image = [NSImage imageWithSystemSymbolName:@"photo.badge.plus" accessibilityDescription:nil];

                NSComboButton *button = [NSComboButton comboButtonWithImage:image menu:menu target:nil action:action];
                NSDictionary *items = NSDictionaryOfVariableBindings(button);
                NSArray *constraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:[button(>=62)]" options:0 metrics:nil views:items];
                [NSLayoutConstraint activateConstraints:constraints];

                item.view = button;
            }
            else if (@available(macOS 11, *))
            {
                NSImage *image = [NSImage imageNamed:@"photo.badge.plus"];

                NSSegmentedControl *control = [HBSegmentedControl segmentedControlWithImages:@[image, image]
                                                                                trackingMode:NSSegmentSwitchTrackingMomentary
                                                                                      target:nil action:action];
                [control setWidth:36 forSegment:0];
                [control setMenu:menu forSegment:1];
                [control setShowsMenuIndicator:YES forSegment:1];
                [control setWidth:14 forSegment:1];
                [control setImage:nil forSegment:1];

                item.view = control;
            }
            return item;
        }
        else
        {
            return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                         label:label
                                                  paletteLabel:label
                                                    symbolName:@"photo.badge.plus"
                                                         image:@"addqueue"
                                                         style:style
                                                        action:@selector(addToQueue:)];
        }
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_START])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Start", @"Main Window Start Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Start/Stop Encoding", @"Main Window Start Toolbar Item")
                                                symbolName:@"play.fill"
                                                     image:@"encode"
                                                     style:style
                                                    action:@selector(toggleStartCancel:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_PAUSE])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Pause", @"Main Window Pause Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Pause/Resume Encoding", @"Main Window Pause Toolbar Item")
                                                symbolName:@"pause.fill"
                                                     image:@"pauseencode"
                                                     style:style
                                                    action:@selector(togglePauseResume:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_PRESET])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Presets", @"Main Window Presets Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Show Presets List", @"Main Window Presets Toolbar Item")
                                                symbolName:@"slider.horizontal.3"
                                                     image:@"presets"
                                                     style:style | HBToolbarItemStyleButton
                                                    action:@selector(togglePresets:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_PREVIEW])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Preview", @"Main Window Preview Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Show Preview Window", @"Main Window Preview Toolbar Item")
                                                symbolName:@"eye"
                                                     image:@"preview"
                                                     style:style
                                                    action:@selector(showPreviewWindow:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_QUEUE])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Queue", @"Main Window Presets Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Show Queue Window", @"Main Window Presets Toolbar Item")
                                                symbolName:@"photo.stack"
                                                     image:@"showqueue"
                                                     style:style | HBToolbarItemStyleButton
                                                    action:@selector(showQueueWindow:)];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_ACTIVITY])
    {
        return [NSToolbarItem HB_toolbarItemWithIdentifier:itemIdentifier
                                                     label:NSLocalizedString(@"Activity", @"Main Window Activity Toolbar Item")
                                              paletteLabel:NSLocalizedString(@"Show Activity Window", @"Main Window Activity Toolbar Item")
                                                symbolName:@"text.viewfinder"
                                                     image:@"activity"
                                                     style:style
                                                    action:@selector(showOutputPanel:)];
    }
    return nil;
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
    return @[TOOLBAR_OPEN, NSToolbarSpaceItemIdentifier, TOOLBAR_ADD, NSToolbarSpaceItemIdentifier, TOOLBAR_START, TOOLBAR_PAUSE, NSToolbarFlexibleSpaceItemIdentifier, TOOLBAR_PRESET, TOOLBAR_PREVIEW, TOOLBAR_QUEUE, TOOLBAR_ACTIVITY];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
    return @[TOOLBAR_OPEN, TOOLBAR_ADD, TOOLBAR_START, TOOLBAR_PAUSE, TOOLBAR_PRESET, TOOLBAR_PREVIEW, TOOLBAR_QUEUE, TOOLBAR_ACTIVITY, NSToolbarSpaceItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier];
}

- (void)updateToolbarButtonsStateForScanCore:(HBState)state toolbar:(NSToolbar *)toolbar
{
    NSToolbarItem *openToolbarItem = [toolbar HB_toolbarItemWithIdentifier:TOOLBAR_OPEN];

    if (state == HBStateIdle)
    {
        [openToolbarItem HB_setSymbol:@"film" configuration:nil fallbackImage:@"source"];
        openToolbarItem.label = NSLocalizedString(@"Open Source",  @"Toolbar Open/Cancel Item");
        openToolbarItem.toolTip = NSLocalizedString(@"Open Source", @"Toolbar Open/Cancel Item");
    }
    else
    {
        [openToolbarItem HB_setSymbol:@"stop.fill" configuration:self.redConf fallbackImage:@"stopencode"];
        openToolbarItem.label = NSLocalizedString(@"Cancel Scan", @"Toolbar Open/Cancel Item");
        openToolbarItem.toolTip = NSLocalizedString(@"Cancel Scanning Source", @"Toolbar Open/Cancel Item");
    }
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
        startToolbarItem.label = queue.pendingItemsCount > 0 ? NSLocalizedString(@"Start Queue", @"Toolbar Start/Stop Item") :  NSLocalizedString(@"Start", @"Toolbar Start/Stop Item");
        startToolbarItem.toolTip = NSLocalizedString(@"Start Encoding", @"Toolbar Start/Stop Item");
    }
}

- (void)updateToolbarQueueBadge:(NSString *)value toolbar:(NSToolbar *)toolbar
{
    HBToolbarItem *queueToolbarItem = (HBToolbarItem *)[toolbar HB_toolbarItemWithIdentifier:TOOLBAR_QUEUE];
    queueToolbarItem.badgeValue = value;
}

@end
