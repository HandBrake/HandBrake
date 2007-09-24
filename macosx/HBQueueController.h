/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;

#define HB_QUEUE_DRAGGING 0             // <--- NOT COMPLETELY FUNCTIONAL YET
#define HB_OUTLINE_METRIC_CONTROLS 0    // for tweaking the outline cell spacings

//------------------------------------------------------------------------------------

// HBQueueOutlineView exists solely to get around a bug in variable row height outline
// views in which row heights get messed up during live resizes. See this discussion:
// http://lists.apple.com/archives/cocoa-dev/2005/Oct/msg00871.html
// However, the recommeneded fix (override drawRect:) does not work. Instead, this
// subclass implements viewDidEndLiveResize in order to recalculate all row heights.
@interface HBQueueOutlineView : NSOutlineView
{
}
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
}

+ (HBJobGroup *)       jobGroup;
- (unsigned int)       count;
- (void)               addJob: (HBJob *)aJob;
- (HBJob *)            jobAtIndex: (unsigned)index;
- (unsigned)           indexOfJob: (HBJob *)aJob;
- (NSEnumerator *)     jobEnumerator;
- (void)               setNeedsDescription: (BOOL)flag;
- (NSMutableAttributedString *) attributedDescriptionWithHBHandle: (hb_handle_t *)handle;
- (float)              heightOfDescriptionForWidth:(float)width withHBHandle: (hb_handle_t *)handle;
- (float)              lastDescriptionHeight;

@end

//------------------------------------------------------------------------------------

@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;
    HBController                 *fHBController;
    NSMutableArray               *fJobGroups;           // hblib's job list organized in a hierarchy of HBJobGroup and HBJob
    NSViewAnimation              *fAnimation;           // for revealing the fCurrentJobPane
    BOOL                         fCurrentJobPaneShown;  // NO when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    hb_job_t                     *fLastKnownCurrentJob;
    NSMutableIndexSet            *fSavedExpandedItems;
    unsigned int                 fSavedSelectedItem;
#if HB_QUEUE_DRAGGING
    NSArray	 	             	 *fDraggedNodes;
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
    IBOutlet HBQueueOutlineView  *fOutlineView;
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
- (IBAction)toggleStartCancel: (id)sender;
- (IBAction)togglePauseResume: (id)sender;

#if HB_OUTLINE_METRIC_CONTROLS
- (IBAction)imageSpacingChanged: (id)sender;
- (IBAction)indentChanged: (id)sender;
#endif

@end

