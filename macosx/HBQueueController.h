/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>

#include "hb.h"
@class HBController;

// HB_OUTLINE_QUEUE turns on an outline view for the queue.
#define HB_OUTLINE_QUEUE 1
#define HB_OUTLINE_METRIC_CONTROLS 0    // for tweaking the outline cell spacings


@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;
    HBController                 *fHBController;
    NSViewAnimation              *fAnimation;
    BOOL                         fCurrentJobHidden;  // YES when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    BOOL                         fShowsJobsAsGroups;
    BOOL                         fShowsDetail;
#if HB_OUTLINE_QUEUE
    NSMutableArray               *fEncodes;   // hblib's job list organized in a hierarchy. Contents are NSArrays of HBJobs.
    NSMutableIndexSet            *fSavedExpandedItems;
    unsigned int                 fSavedSelectedItem;
    hb_job_t                     *fLastKnownCurrentJob;
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
#if HB_OUTLINE_QUEUE
    IBOutlet NSOutlineView       *fOutlineView;
#else
    IBOutlet NSTableView         *fTaskView;
#endif
    IBOutlet NSTextField         *fQueueCountField;
#if HB_OUTLINE_METRIC_CONTROLS
    IBOutlet NSSlider            *fIndentation; // debug
    IBOutlet NSSlider            *fSpacing;     // debug
#endif
    
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
- (IBAction)toggleStartCancel: (id)sender;
- (IBAction)togglePauseResume: (id)sender;

#if HB_OUTLINE_METRIC_CONTROLS
- (IBAction)imageSpacingChanged: (id)sender;
- (IBAction)indentChanged: (id)sender;
#endif

@end
