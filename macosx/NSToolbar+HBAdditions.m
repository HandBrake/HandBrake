/*  NSToolbar+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSToolbar+HBAdditions.h"
#import "HBToolbarItem.h"

@implementation NSToolbarItem (HBAdditions)

+ (instancetype)HB_toolbarItemWithIdentifier:(NSString *)itemIdentifier
                                       label:(NSString *)label
                                paletteLabel:(nullable NSString *)palettelabel
                                  symbolName:(NSString *)symbolName
                                       image:(NSString *)imageName
                                       style:(HBToolbarItemStyle)style
                                      target:(nullable id)target
                                      action:(SEL)action
{
    NSToolbarItem *item = [[HBToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
    item.label = label;
    item.paletteLabel = palettelabel ? palettelabel : label;
    item.toolTip = palettelabel;
    item.target = target;
    item.action = action;
    [item HB_setSymbol:symbolName configuration:nil fallbackImage:imageName];

    BOOL bordered = (style & HBToolbarItemStyleBordered) != 0;
    BOOL button   = (style & HBToolbarItemStyleButton) != 0;

    if (style & HBToolbarItemStyleDefault)
    {
        if (@available(macOS 14, *))
        {
            bordered = YES;
            item.bordered = bordered;
        }
        else if (@available(macOS 11, *))
        {
            bordered = YES;
            button = YES;
            item.bordered = bordered;
        }
    }

    if (button)
    {
        NSButton *buttonView = [NSButton buttonWithImage:item.image target:item.target action:item.action];
        buttonView.bordered = bordered;

        if (bordered)
        {
            buttonView.bezelStyle = NSBezelStyleTexturedRounded;
        }
        else
        {
            buttonView.imageScaling = NSImageScaleProportionallyUpOrDown;
        }
        if (@available(macOS 11, *))
        {
            NSDictionary *items = NSDictionaryOfVariableBindings(buttonView);
            NSArray *constraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:[buttonView(>=40)]" options:0 metrics:nil views:items];
            [NSLayoutConstraint activateConstraints:constraints];
        }
        else
        {
            item.minSize = bordered ? NSMakeSize(32, 16) : NSMakeSize(32, 32);
        }
        item.view = buttonView;
    }

    return item;
}

- (void)HB_setSymbol:(NSString *)symbolName configuration:(nullable id)configuration fallbackImage:(NSString *)imageName
{
    NSImage *image = nil;

    if (@available(macOS 11, *))
    {
        image = [NSImage imageWithSystemSymbolName:symbolName accessibilityDescription:nil];
        if (configuration)
        {
            image = [image imageWithSymbolConfiguration:configuration];
        }
        if (image == nil)
        {
            image = [NSImage imageNamed:symbolName];
        }
    }
    else
    {
        image = [NSImage imageNamed:imageName];
    }
    self.image = image;
}

@end


@implementation NSToolbar (HBAdditions)

- (nullable NSToolbarItem *)HB_toolbarItemWithIdentifier:(NSString *)itemIdentifier
{
    for (NSToolbarItem *item in self.items)
    {
        if ([item.itemIdentifier isEqualToString:itemIdentifier])
        {
            return item;
        }
    }
    return nil;
}

- (nullable NSToolbarItem *)HB_visibleToolbarItemWithIdentifier:(NSString *)itemIdentifier
{
    for (NSToolbarItem *item in self.visibleItems)
    {
        if ([item.itemIdentifier isEqualToString:itemIdentifier])
        {
            return item;
        }
    }
    return nil;
}

@end
