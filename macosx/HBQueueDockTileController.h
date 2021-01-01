//
//  HBQueueDockTileController.h
//  HandBrake
//
//  Created by Damiano Galassi on 09/04/2020.
//  Copyright © 2021 HandBrake. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class HBQueue;

NS_ASSUME_NONNULL_BEGIN

@interface HBQueueDockTileController : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithQueue:(HBQueue *)queue dockTile:(NSDockTile *)dockTile image:(NSImage *)image NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
