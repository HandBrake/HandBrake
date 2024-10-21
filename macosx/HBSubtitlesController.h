/* $Id: HBSubtitles.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBSubtitles;

NS_ASSUME_NONNULL_BEGIN

@interface HBSubtitlesController : NSViewController

@property (nonatomic, readwrite, weak) HBSubtitles *subtitles;

- (void)addTracksFromExternalFiles:(NSArray<NSURL *> *)fileURLs;

@end

NS_ASSUME_NONNULL_END

