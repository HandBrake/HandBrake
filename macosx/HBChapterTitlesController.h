/*  ChapterTitles.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

/**
 *  HBChapterTitlesController
 *  Responds to HBTitleChangedNotification notifications.
 */
@interface HBChapterTitlesController : NSViewController <HBViewValidation>

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
