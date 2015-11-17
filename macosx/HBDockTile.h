//
//  HBDockTile.h
//  HandBrake
//
//  Created by Damiano Galassi on 20/08/14.
//
//

#import <Cocoa/Cocoa.h>

@interface HBDockTile : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDockTile:(NSDockTile *)dockTile image:(NSImage *)image NS_DESIGNATED_INITIALIZER;

/**
 *  Updates two DockTextFields on the dockTile,
 *  one with total percentage, the other one with the ETA.
 *  The ETA string is formated by the callers *
 */
- (void)updateDockIcon:(double)progress withETA:(NSString *)etaStr;


/**
 *  Updates two DockTextFields on the dockTile,
 *  one with total percentage, the other one with the ETA.
 *
 *  ETA format is [XX]X:XX:XX when ETA is greater than one hour
 *  [X]X:XX when ETA is greater than 0 (minutes or seconds)
 *  When these conditions doesn't applied (eg. when ETA is undefined)
 *  we show just a tilde (~)
 */
- (void)updateDockIcon:(double)progress hours:(NSInteger)hours minutes:(NSInteger)minutes seconds:(NSInteger)seconds;

@end
