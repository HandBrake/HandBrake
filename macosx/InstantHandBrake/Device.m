//
//  Preset.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 15/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.fr/>.
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

- (id) initWithCoder:(NSCoder *) coder
{
    deviceName = [[coder decodeObjectForKey:@"DeviceName"] retain];
    presetsArray = [[coder decodeObjectForKey:@"Presets"] retain];
    
    return self;
}

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeObject: deviceName forKey:@"DeviceName"];
    [encoder encodeObject: presetsArray forKey:@"Presets"];
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
    return [presetsArray objectAtIndex:0];
}

@end
