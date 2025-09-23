/*  HBHUDButtonCell.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBHUDView.h"

@implementation HBHUDView

- (instancetype)initWithFrame:(NSRect)frame
{
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 160000
    if (@available(macOS 26.0, *))
    {
        NSGlassEffectView *glassView = [[NSGlassEffectView alloc] initWithFrame:frame];
        if (glassView)
        {
            glassView.cornerRadius = 20;
            glassView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
        }
        self = (HBHUDView *)glassView;
    }
    else
#endif
    {
        self = [super initWithFrame:frame];
        if (self)
        {
            self.wantsLayer = YES;

            CGFloat radius = 4;
            if (@available (macOS 11, *))
            {
                radius = 10;
            }
            self.layer.cornerRadius = radius;

            self.blendingMode = NSVisualEffectBlendingModeWithinWindow;
            self.material = NSVisualEffectMaterialDark;
            self.state = NSVisualEffectStateActive;

            self.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
        }
    }

    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end
