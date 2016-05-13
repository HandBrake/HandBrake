/*  HBAVPlayer.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#import "HBPlayer.h"

NS_ASSUME_NONNULL_BEGIN

@interface HBAVPlayer : NSObject <HBPlayer>

@property (nonatomic, readonly) CALayer *layer;

@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic) NSTimeInterval currentTime;

@property (nonatomic) float rate;
@property (nonatomic) float volume;

@property (nonatomic, readonly, getter=isPlayable) BOOL playable;

@end

NS_ASSUME_NONNULL_END
