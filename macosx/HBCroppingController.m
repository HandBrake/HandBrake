/* HBCroppingController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBCroppingController.h"

@import HandBrakeKit;

@interface HBCroppingController ()
@property (nonatomic, readonly) HBPicture *picture;
@end

@implementation HBCroppingController

- (instancetype)initWithPicture:(HBPicture *)picture
{
    self = [super initWithNibName:@"HBCroppingView" bundle:nil];
    if (self)
    {
        _picture = picture;
    }
	return self;
}

@end
