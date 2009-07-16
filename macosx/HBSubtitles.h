/* $Id: HBSubtitles.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#include "hb.h"




@interface HBSubtitles : NSObject {
hb_title_t                   *fTitle;

NSMutableArray               *subtitleArray; // contains the output subtitle track info
NSMutableArray               *subtitleSourceArray;// contains the source subtitle track info
NSMutableArray               *languagesArray; // array of languages taken from lang.c
int                           languagesArrayDefIndex;
NSMutableArray               *charCodeArray; // array of character codes
int                           charCodeArrayDefIndex;
int                           container;

}

// Trigger a refresh of data
- (void)resetWithTitle:(hb_title_t *)title;

// Create new subtitle track
- (void)addSubtitleTrack;
- (NSDictionary *)createSubtitleTrack;
- (NSMutableArray*) getSubtitleArray: (NSMutableArray *) subtitlesArray ;
// Add an srt file
- (void)createSubtitleSrtTrack:(NSString *)filePath;

- (void)containerChanged:(int) newContainer;

// Table View Delegates
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(NSInteger)rowIndex;
      
- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(NSInteger)rowIndex;

- (void)tableView:(NSTableView *)aTableView
        willDisplayCell:(id)aCell
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(NSInteger)rowIndex;

@end
