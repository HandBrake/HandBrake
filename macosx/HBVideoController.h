/*  HBVideoController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBAdvancedController;
@class HBJob;

/**
 *  HBVideoController
 */
@interface HBVideoController : NSViewController

- (instancetype)initWithAdvancedController:(HBAdvancedController *)advancedController;

@property (nonatomic, readwrite, unsafe_unretained) HBJob *job;

@end
