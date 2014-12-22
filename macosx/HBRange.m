/*  HBRange.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRange.h"
#import "NSCodingMacro.h"

@implementation HBRange

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBRangeVersion"];
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];
    
    return self;
}

@end
