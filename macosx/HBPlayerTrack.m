/*  HBPlayerTrack.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPlayerTrack.h"

@implementation HBPlayerTrack

- (instancetype)initWithTrackName:(NSString *)name object:(id)representedObject enabled:(BOOL)enabled
{
    self = [super init];
    if (self) {
        _name = name;
        _enabled = enabled;
        _representedObject = representedObject;
    }
    return self;
}

@end
