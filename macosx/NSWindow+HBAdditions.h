//
//  NSWindow+HBAdditions.h
//  HandBrake
//
//  Created by Damiano Galassi on 25/04/16.
//
//

#import <Cocoa/Cocoa.h>

@interface NSWindow (HBAdditions)

/**
 *  Resizes the entire window to accomodate a view of a particular size.
 */
- (void)HB_resizeToBestSizeForViewSize:(NSSize)viewSize center:(NSPoint)center animate:(BOOL)performAnimation;


/**
 *  Calculates and returns the center point of the window
 */
- (NSPoint)HB_centerPoint;


@end
