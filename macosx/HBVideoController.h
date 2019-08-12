/*  HBVideoController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBVideo;

NS_ASSUME_NONNULL_BEGIN

@interface HBVideoController : NSViewController

@property (nonatomic, readwrite, weak) HBVideo *video;

@end

NS_ASSUME_NONNULL_END

