/*  ChapterTitles.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

/**
 *  HBChapterTitlesController
 *  Responds to HBTitleChangedNotification notifications.
 */
@interface HBChapterTitlesController : NSViewController

- (void)enableUI:(BOOL)b;
- (void)addChaptersFromQueue:(NSMutableArray *)newChaptersArray;

/**
 *  Enable/disable chapters markers
 */
@property (readwrite, nonatomic) BOOL createChapterMarkers;

/**
 *  Get the list of chapter titles
 */
@property (readonly, nonatomic) NSArray *chapterTitlesArray;


@end
