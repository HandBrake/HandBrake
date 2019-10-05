/*  NSWindow+HBAdditions.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface NSWindow (HBAdditions)

/**
 *  Resizes the entire window to accommodate a view of a particular size.
 */
- (void)HB_resizeToBestSizeForViewSize:(NSSize)viewSize keepInScreenRect:(BOOL)keepInScreenRect centerPoint:(NSPoint)center animate:(BOOL)animateFlag;

/**
 *  Calculates and returns the center point of the window
 */
- (NSPoint)HB_centerPoint;

/**
 End editing on the field editor
 and keep the current first responder.

 @return success
 */
- (BOOL)HB_endEditing;

/**
 Force end editing.
 */
- (void)HB_forceEndEditing;


@end
