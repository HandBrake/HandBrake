/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;

#define HB_QUEUE_DRAGGING 0             // <--- NOT COMPLETELY FUNCTIONAL YET
#define HB_OUTLINE_METRIC_CONTROLS 1    // for tweaking the outline cell spacings

typedef enum _HBQueueJobGroupStatus
{
    HBStatusNone          = 0,
    HBStatusPending       = 1,
    HBStatusWorking       = 2,
    HBStatusComplete      = 3,
    HBStatusCanceled      = 4
} HBQueueJobGroupStatus;

//------------------------------------------------------------------------------------
// As usual, we need to subclass NSOutlineView to handle a few special cases:
//
// (1) variable row heights during live resizes
// HBQueueOutlineView exists solely to get around a bug in variable row height outline
// views in which row heights get messed up during live resizes. See this discussion:
// http://lists.apple.com/archives/cocoa-dev/2005/Oct/msg00871.html
// However, the recommeneded fix (override drawRect:) does not work. Instead, this
// subclass implements viewDidEndLiveResize in order to recalculate all row heights.
//
// (2) prevent expanding of items during drags
// During dragging operations, we don't want outline items to expand, since a queue
// doesn't really have children items.
//
// (3) generate a drag image that incorporates more than just the first column
// By default, NSTableView only drags an image of the first column. Change this to
// drag an image of the queue's icon and desc columns.

@interface HBQueueOutlineView : NSOutlineView
{
#if HB_QUEUE_DRAGGING
BOOL                        fIsDragging;
#endif
}
#if HB_QUEUE_DRAGGING
- (BOOL) isDragging;
#endif
@end

//------------------------------------------------------------------------------------

@interface HBJob : NSObject
{
    hb_job_t                     *hbJob;
}

+ (HBJob*)             jobWithJob: (hb_job_t *) job;
- (id)                 initWithJob: (hb_job_t *) job;
- (hb_job_t *)         job;
- (NSMutableAttributedString *) attributedDescriptionWithHBHandle: (hb_handle_t *)handle
                               withIcon: (BOOL)withIcon
                              withTitle: (BOOL)withTitle
                           withPassName: (BOOL)withPassName
                         withFormatInfo: (BOOL)withFormatInfo
                        withDestination: (BOOL)withDestination
                        withPictureInfo: (BOOL)withPictureInfo
                          withVideoInfo: (BOOL)withVideoInfo
                           withx264Info: (BOOL)withx264Info
                          withAudioInfo: (BOOL)withAudioInfo
                       withSubtitleInfo: (BOOL)withSubtitleInfo;

@end

//------------------------------------------------------------------------------------

@interface HBJobGroup : NSObject
{
    NSMutableArray               *fJobs;   // array of HBJob
    NSMutableAttributedString    *fDescription;
    BOOL                         fNeedsDescription;
    float                        fLastDescriptionHeight;
    float                        fLastDescriptionWidth;
    HBQueueJobGroupStatus        fStatus;
    NSString                     *fPath;
}

// Creating a job group
+ (HBJobGroup *)       jobGroup;

// Adding jobs
- (void)               addJob: (HBJob *)aJob;

// Removing jobs
- (void)               removeAllJobs;

// Querying a job group
- (unsigned int)       count;
- (HBJob *)            jobAtIndex: (unsigned)index;
- (unsigned)           indexOfJob: (HBJob *)aJob;
- (NSEnumerator *)     jobEnumerator;
- (void)               setStatus: (HBQueueJobGroupStatus)status;
- (HBQueueJobGroupStatus)  status;
- (void)               setPath: (NSString *)path;
- (NSString *)         path;

// Creating a description
- (void)               setNeedsDescription: (BOOL)flag;
- (NSMutableAttributedString *) attributedDescriptionWithHBHandle: (hb_handle_t *)handle;
- (float)              heightOfDescriptionForWidth:(float)width withHBHandle: (hb_handle_t *)handle;
- (float)              lastDescriptionHeight;

@end

//------------------------------------------------------------------------------------

@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;              // reference to hblib
    HBController                 *fHBController;        // reference to HBController
    NSMutableArray               *fJobGroups;           // hblib's job list organized in a hierarchy of HBJobGroup and HBJob
    HBJobGroup                   *fCurrentJobGroup;     // the HJobGroup current being processed by hblib
    BOOL                         fCurrentJobPaneShown;  // NO when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    hb_job_t                     *fLastKnownCurrentJob; // this is how we track when hbib has started processing a different job
    NSMutableIndexSet            *fSavedExpandedItems;  // used by save/restoreOutlineViewState to preserve which items are expanded
    NSMutableIndexSet            *fSavedSelectedItems;  // used by save/restoreOutlineViewState to preserve which items are selected
#if HB_QUEUE_DRAGGING
    NSArray                      *fDraggedNodes;
#endif
    NSMutableArray               *fCompleted;           // HBJobGroups that have been completed. These also appear in fJobGroups.
    NSTimer                      *fAnimationTimer;      // animates the icon of the current job in the queue outline view
    int                          fAnimationIndex;       // used to generate name of image used to animate the current job in the queue outline view
    
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
    IBOutlet HBQueueOutlineView  *fOutlineView;
    IBOutlet NSTextField         *fQueueCountField;
#if HB_OUTLINE_METRIC_CONTROLS
    IBOutlet NSSlider            *fIndentation; // debug
    IBOutlet NSSlider            *fSpacing;     // debug
#endif
    
}

- (void)setHandle: (hb_handle_t *)handle;
- (void)setHBController: (HBController *)controller;
- (void)hblibJobListChanged;
- (void)hblibStateChanged: (hb_state_t &)state;
- (void)hblibWillStop;

- (IBAction)showQueueWindow: (id)sender;
- (IBAction)removeSelectedJobGroups: (id)sender;
- (IBAction)revealSelectedJobGroups: (id)sender;
- (IBAction)cancelCurrentJob: (id)sender;
- (IBAction)toggleStartCancel: (id)sender;
- (IBAction)togglePauseResume: (id)sender;

#if HB_OUTLINE_METRIC_CONTROLS
- (IBAction)imageSpacingChanged: (id)sender;
- (IBAction)indentChanged: (id)sender;
#endif

@end

