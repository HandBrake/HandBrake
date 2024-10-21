/*  HBSegmentedCell.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSegmentedControl.h"

@implementation HBSegmentedControl : NSSegmentedControl

+ (Class)cellClass
{
    return [HBSegmentedCell class];
}

@end

@implementation HBSegmentedCell

- (SEL)action
{
    //this allows connected menu to popup instantly (because no action is returned for menu button)
    if ([self menuForSegment:[self selectedSegment]])
    {
        return nil;
    }
    else
    {
        return [super action];
    }
}

@end
