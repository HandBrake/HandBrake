/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBVideo;

/**
 *  HBAdvancedController
 */
@interface HBAdvancedController : NSViewController

@property (nonatomic, readwrite, weak) HBVideo *videoSettings;

@property (nonatomic, readwrite, getter=isHidden) BOOL hidden;
@property (nonatomic, readwrite, getter=isEnabled) BOOL enabled;
@end
