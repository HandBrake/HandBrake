/*  HBHUDButtonCell.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBHUDView.h"

@implementation HBHUDView

+ (void)setupNewStyleHUD:(NSVisualEffectView *)view
{
    [view setWantsLayer:YES];
    [view.layer setCornerRadius:4];

    [view setBlendingMode:NSVisualEffectBlendingModeWithinWindow];
    [view setMaterial:NSVisualEffectMaterialDark];
    [view setState:NSVisualEffectStateActive];

    [view setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
}

- (void)drawRect:(NSRect)dirtyRect
{
    NSGraphicsContext  *theContext = [NSGraphicsContext currentContext];
    [theContext saveGraphicsState];

    NSRect rect = NSMakeRect(0.0, 0.0, [self frame].size.width, [self frame].size.height);

    // Draw a standard HUD with black transparent background and white border.
    [[NSColor colorWithCalibratedRed:0.0 green:0.0 blue:0.0 alpha:0.6] setFill];
    [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(rect, 1, 1) xRadius:14.0 yRadius:14.0] fill];

    [[NSColor whiteColor] setStroke];
    [NSBezierPath setDefaultLineWidth:2.0];
    [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(rect, 1, 1) xRadius:14.0 yRadius:14.0] stroke];

    [theContext restoreGraphicsState];
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if (NSClassFromString(@"NSVisualEffectView"))
    {
        // If NSVisualEffectView class is loaded
        // release ourself and return a NSVisualEffectView instance instead.
        self = [[NSClassFromString(@"NSVisualEffectView") alloc] initWithFrame:frame];
        if (self)
        {
            [HBHUDView setupNewStyleHUD:(NSVisualEffectView *)self];
        }
    }
    else
    {
        self = [super initWithFrame:frame];
    }

    return self;
}

@end