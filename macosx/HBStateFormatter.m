/* HBStateFormatter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBStateFormatter.h"

@implementation HBStateFormatter

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _twoLines = YES;
        _showPassNumber = YES;
    }
    return self;
}

@end
