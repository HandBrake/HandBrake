/*  HBComboView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBComboView.h"

@interface HBComboView ()
{
    NSMenu *_menu;
}

@property (nonatomic, readonly) NSButton *button;
@property (nonatomic, readonly) NSPopUpButton *popUpButton;

@end

@implementation HBComboView

- (instancetype)initWithTitle:(NSString *)title image:(NSImage *)image target:(id)target action:(SEL)action menu:(NSMenu *)menu
{
    self = [super initWithFrame:NSMakeRect(0, 0, 72, 32)];

    if (self)
    {
        _action = action;

        _button = [NSButton buttonWithImage:image target:target action:action];
        _button.bordered = NO;
        [self addSubview:_button];

        NSBox *separator = [[NSBox alloc] initWithFrame:NSMakeRect(44, 6, 4, 22)];
        separator.boxType = NSBoxSeparator;
        [self addSubview:separator];

        _popUpButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(50, 0, 12, 32)];
        _popUpButton.pullsDown = YES;
        _popUpButton.bordered = NO;
        _popUpButton.menu = [menu copy];
        [_popUpButton.menu insertItem:[[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""] atIndex:0];
        [self addSubview:_popUpButton];

        _menu = [menu copy];
        NSMenuItem *addToQueue = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:@""];
        [_menu insertItem:addToQueue atIndex:0];
        [_menu insertItem:[NSMenuItem separatorItem] atIndex:1];
    }

    return self;
}

- (void)setMenu:(NSMenu *)menu
{
    _menu = menu;
}

- (NSMenu *)menu
{
    return _menu;
}

- (void)setEnabled:(BOOL)enabled
{
    _enabled = enabled;
    self.button.enabled = enabled;
    self.popUpButton.enabled = enabled;
}

@end
