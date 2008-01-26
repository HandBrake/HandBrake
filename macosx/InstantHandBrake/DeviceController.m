//
//  DeviceController.m
//  InstantHandBrake
//
//  Created by Damiano Galassi on 23/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.m0k.org/>.
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
        deviceArray = [[NSMutableArray alloc] init];
        [self populateList];
    }
    return self;
}

- (id) populateList
{
    [deviceArray addObject:[[Device alloc] initWithDeviceName: @"iPod"]];
    [deviceArray addObject:[[Device alloc] initWithDeviceName: @"PSP"]];
    [deviceArray addObject:[[Device alloc] initWithDeviceName: @"Zune"]];
    [deviceArray addObject:[[Device alloc] initWithDeviceName: @"AppleTV"]];
    
    Preset * newPreset = [[Preset alloc] initWithMuxer: HB_MUX_IPOD
                                            videoCodec: HB_VCODEC_X264
                                          videoBitRate: 1500
                                     videoCodecOptions: @"bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subq=6:no-fast-pskip=1:level=30"
                                            audioCodec: HB_ACODEC_FAAC
                                          audioBitrate: 128
                                       audioSampleRate: 48000
                                              maxWidth: 640
                                             maxHeight: 480
                                            anamorphic: 0];
    
    [[deviceArray objectAtIndex:0] addPreset:newPreset];
    
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
                                   
    [[deviceArray objectAtIndex:1] addPreset:newPreset];

}

- (NSArray *) deviceList
{
    NSArray *deviceList = [deviceArray copy];
    
    [deviceList autorelease];
    
    return deviceList;
} 

@end
