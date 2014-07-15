/* $Id: HBSubtitles.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

/**
 *  HBSubtitlesController
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBSubtitlesController : NSViewController

- (void)enableUI:(BOOL)b;
- (void)addTracksFromQueue:(NSMutableArray *)newSubtitleArray;

// Get the list of subtitles tracks
@property (readonly, nonatomic) NSArray *subtitleArray;

@end
