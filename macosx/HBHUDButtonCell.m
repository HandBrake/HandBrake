/*  HBHUDButtonCell.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBHUDButtonCell.h"

@implementation HBHUDButtonCell

- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView
{
    NSAttributedString *attrLabel = [[NSAttributedString alloc] initWithString:[title string]
                                                                    attributes:@{ NSFontAttributeName:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:self.controlSize]],
                                                                                  NSForegroundColorAttributeName: [NSColor whiteColor]}];

    return [super drawTitle:attrLabel withFrame:frame inView:controlView];
}

@end
