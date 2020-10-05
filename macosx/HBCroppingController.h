/* HBCroppingController.h

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPicture;

NS_ASSUME_NONNULL_BEGIN

@interface HBCroppingController : NSViewController

- (instancetype)initWithPicture:(HBPicture *)picture;

@end

NS_ASSUME_NONNULL_END

