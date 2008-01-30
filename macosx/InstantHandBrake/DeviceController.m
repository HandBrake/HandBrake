//
//  DeviceController.m
//  InstantHandBrake
//
//  Created by Damiano Galassi on 23/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.fr/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import "DeviceController.h"
#import "hb.h"

@implementation DeviceController

- (id)init
{
    if (self = [super init])
    {
        devicesArray = [[NSMutableArray alloc] init];
        appSupportPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/Instant HandBrake"];
        [self loadBuiltInDevices];
        [self loadDevices];
    }
    return self;
}

- (void) dealloc
{
    [devicesArray release];
    [super dealloc];
}

- (BOOL) loadBuiltInDevices
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSArray *path = [bundle pathsForResourcesOfType:@"ihbdevice" inDirectory:@"Devices"];

    Device *newDevice;
    NSString *file;
    
    NSEnumerator *dirEnum = [path objectEnumerator];
    while (file = [dirEnum nextObject])
    {
        newDevice = [NSKeyedUnarchiver unarchiveObjectWithFile:file];
        [devicesArray addObject:newDevice];
    }
    
    if ( ![devicesArray count] )
        return NO;
    else
        return YES;
}

- (BOOL) loadDevices
{
    NSString *file;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    Device *newDevice;
    
    NSDirectoryEnumerator *dirEnum = [fileManager enumeratorAtPath:appSupportPath];
    while (file = [dirEnum nextObject])
    {
        if ([[file pathExtension] isEqualToString: @"ihbdevice"])
        {
            newDevice = [NSKeyedUnarchiver unarchiveObjectWithFile:[appSupportPath stringByAppendingPathComponent:file]];
            [devicesArray addObject:newDevice];
        }
    }
    
    if ( ![devicesArray count] )
        return NO;
    else
        return YES;
}

/* Use this to create a new device preset for now */

- (id) populateList
{
    [devicesArray addObject:[[Device alloc] initWithDeviceName: @"iPod"]];
    [devicesArray addObject:[[Device alloc] initWithDeviceName: @"PSP"]];
    [devicesArray addObject:[[Device alloc] initWithDeviceName: @"AppleTV"]];
    
    Preset *newPreset;
    newPreset = [[Preset alloc] initWithMuxer: HB_MUX_IPOD
                                   videoCodec: HB_VCODEC_X264
                                 videoBitRate: 1500
                            videoCodecOptions: @"bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subq=6:no-fast-pskip=1:level=30"
                                   audioCodec: HB_ACODEC_FAAC
                                 audioBitrate: 128
                              audioSampleRate: 48000
                                     maxWidth: 640
                                    maxHeight: 480
                                   anamorphic: 0];
    
    [[devicesArray objectAtIndex:0] addPreset:newPreset];
    
    newPreset = [[Preset alloc] initWithMuxer: HB_MUX_MP4
                                   videoCodec: HB_VCODEC_X264
                                 videoBitRate: 600
                            videoCodecOptions: @""
                                   audioCodec: HB_ACODEC_FAAC
                                 audioBitrate: 128
                              audioSampleRate: 48000
                                     maxWidth: 480
                                    maxHeight: 272
                                   anamorphic: 0];
                                   
    [[devicesArray objectAtIndex:1] addPreset:newPreset];
    
    newPreset = [[Preset alloc] initWithMuxer: HB_MUX_MP4
                                   videoCodec: HB_VCODEC_X264
                                 videoBitRate: 2500
                            videoCodecOptions: @"bframes=3:ref=1:subq=5:me=umh:no-fast-pskip=1:trellis=1:cabac=0"
                                   audioCodec: HB_ACODEC_FAAC
                                 audioBitrate: 160
                              audioSampleRate: 48000
                                     maxWidth: 720
                                    maxHeight: 576
                                   anamorphic: 1];
                                   
    [[devicesArray objectAtIndex:2] addPreset:newPreset];
}

- (NSArray *) devicesList
{
    NSArray *deviceList = [devicesArray copy];
    [deviceList autorelease];
    
    return deviceList;
}

- (BOOL) saveDevices
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL noErr = YES;
    
    if( ![fileManager fileExistsAtPath:appSupportPath] )
        [fileManager createDirectoryAtPath:appSupportPath attributes:nil];
    
    NSEnumerator *enumerator;
    Device *object;
    enumerator = [devicesArray objectEnumerator];
    
    while( object = [enumerator nextObject] )
    {
        NSString * saveLocation = [NSString stringWithFormat:@"%@/%@.ihbdevice", appSupportPath, [object name]];
        if (![fileManager fileExistsAtPath:saveLocation]) 
        {
            noErr = [NSKeyedArchiver archiveRootObject:object
                        toFile:saveLocation];
        }
    }
    return noErr;
}

@end
