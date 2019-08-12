/* HBAudioController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBAudio;

NS_ASSUME_NONNULL_BEGIN

@interface HBAudioController : NSViewController

@property (nonatomic, readwrite, weak) HBAudio *audio;

@end

NS_ASSUME_NONNULL_END

