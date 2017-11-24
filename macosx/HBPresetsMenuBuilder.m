/*  HBPresetsMenuBuilder.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsMenuBuilder.h"

@interface HBPresetsMenuBuilder ()

@property (nonatomic, readonly) CGFloat size;
@property (nonatomic, readonly) HBPresetsManager *manager;
@property (nonatomic, readonly) SEL action;

@end

@implementation HBPresetsMenuBuilder

- (instancetype)initWithMenu:(NSMenu *)menu action:(SEL)action size:(CGFloat)size presetsManager:(HBPresetsManager *)manager
{
    self = [super init];
    if (self)
    {
        _menu = menu;
        _size = size;
        _manager = manager;
        _action = action;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(build) name:HBPresetsChangedNotification object:nil];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

/**
 *  Adds the presets list to the menu.
 */
- (void)build
{
    // First we remove all the preset menu items
    // inserted previously
    NSArray *menuItems = [self.menu.itemArray copy];
    for (NSMenuItem *item in menuItems)
    {
        if (item.tag == 2)
        {
            [self.menu removeItem:item];
        }
    }

    BOOL builtInSeparatorInserted = NO;
    for (HBPreset *preset in self.manager.root.children)
    {
        if (preset.isBuiltIn == NO && builtInSeparatorInserted == NO)
        {
            [self.menu addItem:[NSMenuItem separatorItem]];
            builtInSeparatorInserted = YES;
        }
        [self.menu addItem:[self buildMenuItemWithPreset:preset]];
    }
}

- (NSMenuItem *)buildMenuItemWithPreset:(HBPreset *)preset
{
    NSMenuItem *item = [[NSMenuItem alloc] init];
    item.title = preset.name;
    item.toolTip = preset.presetDescription;
    item.tag = 2;

    if (preset.isLeaf)
    {
        item.action = self.action;
        item.representedObject = preset;

        // Make the default preset font bold.
        if ([preset isEqualTo:self.manager.defaultPreset])
        {
            NSAttributedString *newTitle = [[NSAttributedString alloc] initWithString:preset.name
                                                                           attributes:@{NSFontAttributeName: [NSFont boldSystemFontOfSize:self.size]}];
            [item setAttributedTitle:newTitle];
        }
    }
    else
    {
        NSMenu *menu = [[NSMenu alloc] init];
        for (HBPreset *childPreset in preset.children)
        {
            [menu addItem:[self buildMenuItemWithPreset:childPreset]];
        }

        item.submenu = menu;
    }

    return item;
}

@end
