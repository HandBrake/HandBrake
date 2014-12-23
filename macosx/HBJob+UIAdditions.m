/*  HBJob+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+UIAdditions.h"
#include "hb.h"

@implementation HBJob (UIAdditions)

- (BOOL)mp4OptionsEnabled
{
    if (self.container & HB_MUX_MASK_MP4)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

@end
