/* $Id: HBSubtitles.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBSubtitles;

/**
 *  HBSubtitlesController
 */
@interface HBSubtitlesController : NSViewController

@property (nonatomic, readwrite, weak) HBSubtitles *subtitles;

@end
