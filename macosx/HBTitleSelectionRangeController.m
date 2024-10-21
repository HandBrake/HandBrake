/* HBTitleSelectionRangeController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionRangeController.h"

@implementation HBTitleSelectionRangeController

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles
{
    self = [super initWithNibName:@"HBTitleSelectionRange" bundle:nil];
    if (self)
    {
        _range = [[HBTitleSelectionRange alloc] initWithTitles:titles];
    }
    return self;
}

- (void)setEnabled:(BOOL)enabled
{
    if (enabled != _enabled)
    {
        [[self.undoManager prepareWithInvocationTarget:self] setEnabled:_enabled];
    }
    _enabled = enabled;
}

@end
