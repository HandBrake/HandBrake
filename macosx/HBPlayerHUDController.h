/*  HBPlayerHUDController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBPlayer.h"
#import "HBHUD.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBPlayerHUDControllerDelegate <NSObject>

- (void)stopPlayer;

@end

@interface HBPlayerHUDController : NSViewController <HBHUD>

@property (nonatomic, nullable, assign) id<HBPlayerHUDControllerDelegate> delegate;
@property (nonatomic, nullable) id<HBPlayer> player;

@end

NS_ASSUME_NONNULL_END

