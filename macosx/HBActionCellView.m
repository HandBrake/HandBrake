/*  HBActionCellView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBActionCellView.h"

@implementation HBActionCellView

- (void)awakeFromNib
{
    [super awakeFromNib];
    if (@available(macOS 26, *))
    {
        self.button.image = [NSImage imageWithSystemSymbolName:@"ellipsis" accessibilityDescription:nil];;
    }
}

@end
