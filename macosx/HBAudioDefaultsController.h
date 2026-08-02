/*  HBAudioDefaultsController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBAudioDefaults;

NS_ASSUME_NONNULL_BEGIN

@protocol HBAudioDefaultsControllerDelegate <NSObject>

- (void)audioControllerDidEnd:(HBAudioDefaults *)settings returnCode:(NSModalResponse)returnCode;

@end

@interface HBAudioDefaultsController : NSViewController

- (instancetype)initWithSettings:(HBAudioDefaults *)settings delegate:(id<HBAudioDefaultsControllerDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END

