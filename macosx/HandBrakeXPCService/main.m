/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HandBrakeXPCService.h"

@import HandBrakeKit;

@interface HBXPCServiceDelegate : NSObject <NSXPCListenerDelegate>
@end

@implementation HBXPCServiceDelegate

- (BOOL)listener:(NSXPCListener *)listener shouldAcceptNewConnection:(NSXPCConnection *)newConnection
{
    newConnection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(HBRemoteCoreProtocol)];
    newConnection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(HBRemoteProgressProtocol)];

    HandBrakeXPCService *exportedObject = [[HandBrakeXPCService alloc] initWithConnection:newConnection];
    newConnection.exportedObject = exportedObject;

    [newConnection resume];

    return YES;
}

@end

int main(int argc, const char *argv[])
{
    HBUtilities.resolveBookmarks = NO;

    HBXPCServiceDelegate *delegate = [HBXPCServiceDelegate new];

    NSXPCListener *listener = [NSXPCListener serviceListener];
    listener.delegate = delegate;

    [listener resume];

    return 0;
}
