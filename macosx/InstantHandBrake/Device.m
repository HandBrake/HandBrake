//
//  Preset.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 15/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.m0k.org/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import "Device.h"


@implementation Device

- (id)initWithDeviceName:(NSString *) name
{
    if (self = [super init])
    {
        deviceName = name;
        presetsArray = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void) dealloc
{
    [presetsArray release];
    [super dealloc];
}

- (void) addPreset: (Preset *) preset
{
    [presetsArray addObject:preset];
    [preset release];
}

- (NSString *) name
{
    return deviceName;
}

- (Preset *) firstPreset
{
    return [presetsArray objectAtIndex:0]; ;
}

@end
