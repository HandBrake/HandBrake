//
//  DeviceController.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 23/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.m0k.org/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import <Cocoa/Cocoa.h>
#import "device.h"

@interface DeviceController : NSObject {
    NSMutableArray        * deviceArray;
}

- (id)init;
- (id)populateList;
- (NSArray *) deviceList;

@end
