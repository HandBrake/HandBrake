/* HBFocusRingView

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import "HBFocusRingView.h"

@implementation HBFocusRingView

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    if (self.showFocusRing)
    {
        [NSGraphicsContext saveGraphicsState];
        NSRect focusRect = NSInsetRect(self.bounds, 2, 2);
        NSSetFocusRingStyle(NSFocusRingOnly);
        NSRectFill(focusRect);
        [NSGraphicsContext restoreGraphicsState];
    }
}

- (void)setShowFocusRing:(BOOL)showFocusRing
{
    _showFocusRing = showFocusRing;
    [self setNeedsDisplay:YES];
}

@end
