/*  ChapterTitles.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>
#include "hb.h"

@interface ChapterTitles : NSObject {
    hb_title_t *fTitle;
}

// Trigger a refresh of data
- (void)resetWithTitle:(hb_title_t *)title;

// Table View Delegates
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(int)rowIndex;
      
- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(int)rowIndex;
@end
