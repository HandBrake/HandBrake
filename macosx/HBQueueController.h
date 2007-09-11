/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>

#include "hb.h"
@class HBController;

// HB_OUTLINE_QUEUE turns on an outline view for the queue.
#define HB_OUTLINE_QUEUE 0


@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;
    HBController                 *fHBController;
    NSViewAnimation              *fAnimation;
    BOOL                         fCurrentJobHidden;  // YES when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    BOOL                         fShowsJobsAsGroups;
    BOOL                         fShowsDetail;
#if HB_OUTLINE_QUEUE
    NSMutableArray               *fEncodes;
    IBOutlet NSOutlineView       *fOutlineView;
#endif
    
    //  +---------------fQueueWindow----------------+
    //  |+-------------fCurrentJobPane-------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  |+---------------fQueuePane----------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  +-------------------------------------------+
    
    IBOutlet NSWindow            *fQueueWindow;

    // fCurrentJobPane - visible only when processing a job
    IBOutlet NSView              *fCurrentJobPane;
    IBOutlet NSImageView         *fJobIconView;
    IBOutlet NSTextField         *fJobDescTextField;
    IBOutlet NSProgressIndicator *fProgressBar;
    IBOutlet NSTextField         *fProgressTextField;
    
    // fQueuePane - always visible; fills entire window when fCurrentJobPane is hidden
    IBOutlet NSView              *fQueuePane;
    IBOutlet NSTableView         *fTaskView;
    IBOutlet NSTextField         *fQueueCountField;
    
}

- (void)setHandle: (hb_handle_t *)handle;
- (void)setHBController: (HBController *)controller;
- (void)updateQueueUI;
- (void)updateCurrentJobUI;

- (IBAction)showQueueWindow: (id)sender;
- (IBAction)removeSelectedJob: (id)sender;
- (IBAction)cancelCurrentJob: (id)sender;
- (IBAction)showDetail: (id)sender;
- (IBAction)hideDetail: (id)sender;
- (IBAction)showJobsAsGroups: (id)sender;
- (IBAction)showJobsAsPasses: (id)sender;
- (IBAction)toggleStartPause: (id)sender;

@end
