//
//  HBDockTile.h
//  HandBrake
//
//  Created by Damiano Galassi on 20/08/14.
//
//

#import <Cocoa/Cocoa.h>

@interface HBDockTile : NSObject

- (instancetype)initWithDockTile:(NSDockTile *)dockTile image:(NSImage *)image NS_DESIGNATED_INITIALIZER;

/**
 *  Updates two DockTextFields on the dockTile,
 *  one with total percentage, the other one with the ETA.
 *  The ETA string is formated by the callers *
 */
- (void)updateDockIcon:(double)progress withETA:(NSString *)etaStr;

@end
