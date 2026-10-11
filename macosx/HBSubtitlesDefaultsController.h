/*  HBSubtitlesDefaultsController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBSubtitlesDefaults;

NS_ASSUME_NONNULL_BEGIN

@protocol HBSubtitlesDefaultsControllerDelegate <NSObject>

- (void)subtitlesControllerDidEnd:(HBSubtitlesDefaults *)settings returnCode:(NSModalResponse)returnCode;

@end

@interface HBSubtitlesDefaultsController : NSViewController

- (instancetype)initWithSettings:(HBSubtitlesDefaults *)settings delegate:(id<HBSubtitlesDefaultsControllerDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END

