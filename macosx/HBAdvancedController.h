/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

@class HBVideo;

/**
 *  HBAdvancedController
 */
@interface HBAdvancedController : NSViewController <HBViewValidation>

@property (nonatomic, readwrite, retain) HBVideo *videoSettings;
@property (nonatomic, readwrite, getter=isHidden) BOOL hidden;

@end
