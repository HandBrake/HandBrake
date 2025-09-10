/*  HBToolbarItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBToolbarItem.h"
#import "HBBadgeView.h"

@interface HBToolbarItem ()

@property (nonatomic, nullable) HBBadgeView *HB_badgeView;

@end

@implementation HBToolbarItem

- (instancetype)init
{
    self = [super init];
    return self;
}

- (void)setHB_badgeValue:(NSString *)badgeValue
{
    if (![_HB_badgeValue isEqualToString:badgeValue])
    {
        _HB_badgeValue = [badgeValue copy];
    }

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 160000
    if (@available(macOS 26.0, *))
    {
        self.badge = badgeValue.length ? [NSItemBadge badgeWithText:badgeValue] : nil;
    }
    else
#endif
    {
        if (self.view && self.HB_badgeView == nil)
        {
            NSSize size = self.view.frame.size;
            NSRect frame = NSMakeRect(0, 0, size.width, size.height);
            self.HB_badgeView = [[HBBadgeView alloc] initWithFrame:frame];
            self.HB_badgeView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [self.view addSubview:self.HB_badgeView];
        }
        self.HB_badgeView.badgeValue = badgeValue;
    }
}

- (NSMenuItem *)menuFormRepresentation
{
    if ([self.view respondsToSelector:@selector(menu)])
    {
        NSMenuItem *menuItem = [[NSMenuItem alloc] init];
        menuItem.title = self.label;

        BOOL comboButton = NO;
        if (@available(macOS 13.0, *))
        {
            if ([self.view isKindOfClass:[NSComboButton class]])
            {
                comboButton = YES;
            }
        }

        if (comboButton)
        {
            menuItem.submenu = [[NSMenu alloc] init];
            [menuItem.submenu addItem:[[NSMenuItem alloc] initWithTitle:self.label action:self.action keyEquivalent:@""]];
            [menuItem.submenu addItem:[NSMenuItem separatorItem]];

            NSMenu *menu = [self.view menu];
            for (NSMenuItem *item in menu.itemArray)
            {
                [menuItem.submenu addItem:[item copy]];
            }
        }
        else if ([self.view isKindOfClass:[NSSegmentedControl class]])
        {
            menuItem.submenu = [[NSMenu alloc] init];
            NSSegmentedControl *control = (NSSegmentedControl *)self.view;
            for (int index = 0; index < control.segmentCount; index += 1)
            {
                if (menuItem.submenu.numberOfItems)
                {
                    [menuItem.submenu addItem:[NSMenuItem separatorItem]];
                }

                NSMenu *controlMenu = [control menuForSegment:index];
                if (controlMenu)
                {
                    for (NSMenuItem *item in controlMenu.itemArray)
                    {
                        [menuItem.submenu addItem:[item copy]];
                    }
                }
                else
                {
                    [menuItem.submenu addItem:[[NSMenuItem alloc] initWithTitle:self.label action:self.action keyEquivalent:@""]];
                }
            }
        }
        else if ([[self.view menu] numberOfItems])
        {
            menuItem.submenu = [self.view menu];
        }
        else
        {
            menuItem.action = self.action;
        }

        return menuItem;
    }
    else
    {
        return [[NSMenuItem alloc] initWithTitle:self.label action:self.action keyEquivalent:@""];
    }
}

- (void)validate
{
    id target = [NSApplication.sharedApplication targetForAction:self.action to:self.target from:self];
    if ([target respondsToSelector:@selector(validateToolbarItem:)])
    {
        self.enabled = [target validateToolbarItem:self];
    }
    else if ([target respondsToSelector:@selector(validateUserInterfaceItem:)])
    {
        self.enabled = [target validateUserInterfaceItem:self];
    }
    else
    {
        [super validate];
    }
}

@end
