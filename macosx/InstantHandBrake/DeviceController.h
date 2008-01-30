//
//  DeviceController.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 23/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.fr/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import <Cocoa/Cocoa.h>
#import "device.h"

@interface DeviceController : NSObject {
    NSMutableArray      *devicesArray;
    NSString            *appSupportPath;
}

- (id)init;
- (BOOL) loadDevices;
- (BOOL) loadBuiltInDevices;
- (id)populateList;
- (NSArray *) devicesList;
- (BOOL) saveDevices;


@end
