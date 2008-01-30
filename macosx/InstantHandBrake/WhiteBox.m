/* WhiteBox

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "WhiteBox.h"

@implementation WhiteBox

- (void) drawRect: (NSRect) rect
{
    [[NSColor whiteColor] set];
    [[NSBezierPath bezierPathWithRect: rect] fill];
    [super drawRect: rect];
}
    
@end
