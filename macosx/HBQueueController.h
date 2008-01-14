/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;
@class HBJob;
@class HBJobGroup;

#define HB_QUEUE_DRAGGING 0             // <--- NOT COMPLETELY FUNCTIONAL YET
#define HB_OUTLINE_METRIC_CONTROLS 0    // for tweaking the outline cell spacings

// hb_job_t contains a sequence_id field. The high word is a unique job group id.
// The low word contains the "sequence id" which is a value starting at 0 and
// incremented for each pass in the job group. Use the function below to create and
// interpret a sequence_id field.
int MakeJobID(int jobGroupID, int sequenceNum);
bool IsFirstPass(int jobID);

typedef enum _HBQueueJobGroupStatus
{
    HBStatusNone          = 0,
    HBStatusPending       = 1,
    HBStatusWorking       = 2,
    HBStatusCompleted     = 3,
    HBStatusCanceled      = 4
} HBQueueJobGroupStatus;

// Notification sent whenever the status of a HBJobGroup changes (via setStatus). The
// user info contains one object, @"HBOldJobGroupStatus", which is an NSNumber
// containing the previous status of the job group.
extern NSString * HBJobGroupStatusNotification;

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
// HBJob is the UI's equivalent to libhb's hb_job_t struct. It is used mainly for
// drawing the job's description. HBJob are referred to in the UI as 'passes'.
//------------------------------------------------------------------------------------

@interface HBJob : NSObject
{
    HBJobGroup                   *jobGroup;         // The group this job belongs to
    
    // The following fields match up with similar fields found in hb_job_t and it's
    // various substructures.
@public
    // from hb_job_s
    int                         sequence_id;        // This is how we xref to the jobs inside libhb

    int                         chapter_start;
    int                         chapter_end;
    int                         chapter_markers;
    int                         crop[4];
    int                         deinterlace;
    int                         width;
    int                         height;
    int                         keep_ratio;
    int                         grayscale;
    int                         pixel_ratio;
    int                         pixel_aspect_width;
    int                         pixel_aspect_height;
    int                         vcodec;
    float                       vquality;
    int                         vbitrate;
    int                         vrate;
    int                         vrate_base;
    int                         pass;
    int                         h264_level;
    int                         crf;
    NSString                    *x264opts;

    int                         audio_mixdowns[8];
    int                         acodec;
    int                         abitrate;
    int                         arate;
    int                         subtitle;

    int                         mux;
    NSString                    *file;

    // from hb_title_s
    NSString                    *titleName;
    int                         titleIndex;
    int                         titleWidth;
    int                         titleHeight;

    // from hb_subtitle_s
    NSString                    *subtitleLang;
}

+ (HBJob*)             jobWithLibhbJob: (hb_job_t *) job;
- (id)                 initWithLibhbJob: (hb_job_t *) job;
- (HBJobGroup *)       jobGroup;
- (void)               setJobGroup: (HBJobGroup *)aJobGroup;
- (NSMutableAttributedString *) attributedDescriptionWithIcon: (BOOL)withIcon
                              withTitle: (BOOL)withTitle
                           withPassName: (BOOL)withPassName
                         withFormatInfo: (BOOL)withFormatInfo
                        withDestination: (BOOL)withDestination
                        withPictureInfo: (BOOL)withPictureInfo
                          withVideoInfo: (BOOL)withVideoInfo
                           withx264Info: (BOOL)withx264Info
                          withAudioInfo: (BOOL)withAudioInfo
                       withSubtitleInfo: (BOOL)withSubtitleInfo;

// Attributes used by attributedDescriptionWithIcon:::::::::
+ (NSMutableParagraphStyle *) descriptionParagraphStyle;
+ (NSDictionary *) descriptionDetailAttribute;
+ (NSDictionary *) descriptionDetailBoldAttribute;
+ (NSDictionary *) descriptionTitleAttribute;
+ (NSDictionary *) descriptionShortHeightAttribute;

@end

//------------------------------------------------------------------------------------
// HBJobGroup is what's referred to in the UI as an 'encode'. A job group contains
// multiple HBJobs, one for each 'pass' of the encode. Whereas libhb keeps a simple
// list of jobs in it's queue, the queue controller presents them to the user as a
// series of encodes and passes (HBJObGroups and HBJobs).
//------------------------------------------------------------------------------------

@interface HBJobGroup : NSObject
{
    NSMutableArray               *fJobs;   // array of HBJob
    NSMutableAttributedString    *fDescription;
    BOOL                         fNeedsDescription;
    float                        fLastDescriptionHeight;
    float                        fLastDescriptionWidth;
    HBQueueJobGroupStatus        fStatus;
    NSString                     *fPresetName;
}

// Creating a job group
+ (HBJobGroup *)       jobGroup;

// Adding jobs
- (void)               addJob: (HBJob *)aJob;

// Querying a job group
- (unsigned int)       count;
- (HBJob *)            jobAtIndex: (unsigned)index;
- (unsigned int)       indexOfJob: (HBJob *)aJob;
- (NSEnumerator *)     jobEnumerator;
- (void)               setStatus: (HBQueueJobGroupStatus)status;
- (HBQueueJobGroupStatus)  status;
- (void)               setPresetName: (NSString *)name;
- (NSString *)         presetName;
- (NSString *)         destinationPath;
- (NSString *)         name;

// Creating a description
- (void)               setNeedsDescription: (BOOL)flag;
- (NSMutableAttributedString *) attributedDescription;
- (float)              heightOfDescriptionForWidth:(float)width;
- (float)              lastDescriptionHeight;

@end

//------------------------------------------------------------------------------------

@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;              // reference to libhb
    HBController                 *fHBController;        // reference to HBController
    NSMutableArray               *fJobGroups;           // libhb's job list organized in a hierarchy of HBJobGroup and HBJob
    HBJobGroup                   *fCurrentJobGroup;     // the HJobGroup currently being processed by libhb
    HBJob                        *fCurrentJob;          // the HJob (pass) currently being processed by libhb
    int                          fCurrentJobID;         // this is how we track when hbib has started processing a different job. This is the job's sequence_id.

    unsigned int                 fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    unsigned int                 fCompletedCount;       // Don't access these directly as they may not always be up-to-date.
    unsigned int                 fCanceledCount;        // Use the accessor functions instead.
    unsigned int                 fWorkingCount;
    BOOL                         fJobGroupCountsNeedUpdating;
    
    BOOL                         fCurrentJobPaneShown;  // NO when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    NSMutableIndexSet            *fSavedExpandedItems;  // used by save/restoreOutlineViewState to preserve which items are expanded
    NSMutableIndexSet            *fSavedSelectedItems;  // used by save/restoreOutlineViewState to preserve which items are selected
#if HB_QUEUE_DRAGGING
    NSArray                      *fDraggedNodes;
#endif
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
- (void)libhbStateChanged: (hb_state_t &)state;
- (void)libhbWillStop;

// Adding items to the queue
- (void) addJobGroup: (HBJobGroup *) aJobGroup;

// Getting the currently processing job group
- (HBJobGroup *) currentJobGroup;
- (HBJob *) currentJob;

// Getting job groups
- (HBJobGroup *) pendingJobGroupWithDestinationPath: (NSString *)path;

// Getting queue statistics
- (unsigned int) pendingCount;
- (unsigned int) completedCount;
- (unsigned int) canceledCount;
- (unsigned int) workingCount;

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

