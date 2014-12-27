/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import "HBQueueController.h"
#import "HBCore.h"
#import "Controller.h"

#import "HBQueueOutlineView.h"
#import "HBImageAndTextCell.h"
#import "HBUtilities.h"

#import "HBJob.h"

#import "HBPicture+UIAdditions.h"
#import "HBFilters+UIAdditions.h"

#define HB_ROW_HEIGHT_TITLE_ONLY           17.0

// Pasteboard type for or drag operations
#define DragDropSimplePboardType @"HBQueueCustomOutlineViewPboardType"

#pragma mark -

//------------------------------------------------------------------------------------
// NSMutableAttributedString (HBAdditions)
//------------------------------------------------------------------------------------

@interface NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary;
@end

@implementation NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary
{
    NSAttributedString *s = [[[NSAttributedString alloc] initWithString:aString
                                                             attributes:aDictionary] autorelease];
    [self appendAttributedString:s];
}
@end

#pragma mark -

@interface HBQueueController () <HBQueueOutlineViewDelegate>
{
    HBController                 *fHBController;        // reference to HBController
    NSMutableArray               *fJobGroups;           // mirror image of the queue array from controller.mm

    int                          pidNum;                // Records the PID number from HBController for this instance
    int                          fEncodingQueueItem;    // corresponds to the index of fJobGroups encoding item
    int                          fPendingCount;         // Number of various kinds of job groups in fJobGroups.
    int                          fWorkingCount;
    BOOL                         fJobGroupCountsNeedUpdating;

    BOOL                         fCurrentJobPaneShown;  // NO when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    NSMutableIndexSet            *fSavedExpandedItems;  // used by save/restoreOutlineViewState to preserve which items are expanded
    NSMutableIndexSet            *fSavedSelectedItems;  // used by save/restoreOutlineViewState to preserve which items are selected

    NSTimer                      *fAnimationTimer;      // animates the icon of the current job in the queue outline view
    int                          fAnimationIndex;       // used to generate name of image used to animate the current job in the queue outline view

    IBOutlet NSTextField         *fProgressTextField;

    IBOutlet HBQueueOutlineView  *fOutlineView;
    IBOutlet NSTextField         *fQueueCountField;
    NSArray                      *fDraggedNodes;

    // Text Styles
    NSMutableParagraphStyle *ps;
    NSDictionary            *detailAttr;
    NSDictionary            *detailBoldAttr;
    NSDictionary            *titleAttr;
    NSDictionary            *shortHeightAttr;
}

@property (nonatomic, readonly) HBCore *queueCore;

/* control encodes in the window */
- (IBAction)removeSelectedQueueItem: (id)sender;
- (IBAction)revealSelectedQueueItem: (id)sender;
- (IBAction)editSelectedQueueItem: (id)sender;

@end

@implementation HBQueueController

//------------------------------------------------------------------------------------
// init
//------------------------------------------------------------------------------------
- (id)init
{
    if (self = [super initWithWindowNibName:@"Queue"])
    {
        // NSWindowController likes to lazily load its window nib. Since this
        // controller tries to touch the outlets before accessing the window, we
        // need to force it to load immadiately by invoking its accessor.
        //
        // If/when we switch to using bindings, this can probably go away.
        [self window];

        // Our defaults
        [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"QueueWindowIsOpen": @"NO"}];

        fJobGroups = [[NSMutableArray arrayWithCapacity:0] retain];

        [self initStyles];
    }

    return self;
}

- (void)setQueueArray:(NSMutableArray *)QueueFileArray
{
    [fJobGroups setArray:QueueFileArray];

    [fOutlineView reloadData];

    // lets get the stats on the status of the queue array
    
    fPendingCount = 0;
    fWorkingCount = 0;

	int i = 0;
	for (HBJob *job in fJobGroups)
	{
		if (job.state == HBJobStateWorking) // being encoded
		{
			fWorkingCount++;
            // we have an encoding job so, lets start the animation timer
            if (job.pidId == pidNum)
            {
                fEncodingQueueItem = i;
            }
		}
        if (job.state == HBJobStateReady) // pending
        {
			fPendingCount++;
		}
		i++;
	}

    // Set the queue status field in the queue window
    NSMutableString *string;
    if (fPendingCount == 0)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"No encode pending", @"" )];
    }
    else if (fPendingCount == 1)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode pending", @"" ), fPendingCount];
    }
    else
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encodes pending", @"" ), fPendingCount];
    }
    [fQueueCountField setStringValue:string];
    
}

/* This method sets the status string in the queue window
 * and is called from Controller.mm (fHBController)
 * instead of running another timer here polling libhb
 * for encoding status
 */
- (void)setQueueStatusString:(NSString *)statusString
{
    [fProgressTextField setStringValue:statusString];
}

//------------------------------------------------------------------------------------
// dealloc
//------------------------------------------------------------------------------------
- (void)dealloc
{
    // clear the delegate so that windowWillClose is not attempted
    if( [[self window] delegate] == self )
        [[self window] setDelegate:nil];

    [fJobGroups release];

    [fSavedExpandedItems release];
    [fSavedSelectedItems release];

    [ps release];
    [detailAttr release];
    [detailBoldAttr release];
    [titleAttr release];
    [shortHeightAttr release];

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

//------------------------------------------------------------------------------------
// Receive HB handle
//------------------------------------------------------------------------------------
- (void)setCore: (HBCore *)core
{
    _queueCore = core;
}

//------------------------------------------------------------------------------------
// Receive HBController
//------------------------------------------------------------------------------------
- (void)setHBController: (HBController *)controller
{
    fHBController = controller;
}

- (void)setPidNum: (int)myPidnum
{
    pidNum = myPidnum;
    [HBUtilities writeToActivityLog: "HBQueueController : My Pidnum is %d", pidNum];
}

#pragma mark -

//------------------------------------------------------------------------------------
// Displays and brings the queue window to the front
//------------------------------------------------------------------------------------
- (IBAction) showQueueWindow: (id)sender
{
    [self showWindow:sender];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"QueueWindowIsOpen"];
    [self startAnimatingCurrentWorkingEncodeInQueue];
}

//------------------------------------------------------------------------------------
// windowDidLoad
//------------------------------------------------------------------------------------
- (void)windowDidLoad
{
    /* lets setup our queue list outline view for drag and drop here */
    [fOutlineView registerForDraggedTypes: @[DragDropSimplePboardType] ];
    [fOutlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [fOutlineView setVerticalMotionCanBeginDrag: YES];

    // Don't allow autoresizing of main column, else the "delete" column will get
    // pushed out of view.
    [fOutlineView setAutoresizesOutlineColumn: NO];

    // Show/hide UI elements
    fCurrentJobPaneShown = NO;     // it's shown in the nib
}

//------------------------------------------------------------------------------------
// windowWillClose
//------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
    [self stopAnimatingCurrentJobGroupInQueue];
}

#pragma mark Toolbar

//------------------------------------------------------------------------------------
// validateToolbarItem:
//------------------------------------------------------------------------------------
- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    // Optional method: This message is sent to us since we are the target of some
    // toolbar item actions.

    if (!self.queueCore) return NO;

    BOOL enable = NO;
    HBState s = self.queueCore.state;

    if ([[toolbarItem itemIdentifier] isEqualToString:@"HBQueueStartCancelToolbarIdentifier"])
    {
        if ((s == HBStatePaused) || (s == HBStateWorking) || (s == HBStateMuxing))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"stopencode"]];
            [toolbarItem setLabel: @"Stop"];
            [toolbarItem setToolTip: @"Stop Encoding"];
        }

        else if (fPendingCount > 0)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: @"Start"];
            [toolbarItem setToolTip: @"Start Encoding"];
        }

        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: @"Start"];
            [toolbarItem setToolTip: @"Start Encoding"];
        }
    }

    if ([[toolbarItem itemIdentifier] isEqualToString:@"HBQueuePauseResumeToolbarIdentifier"])
    {
        if (s == HBStatePaused)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: @"Resume"];
            [toolbarItem setToolTip: @"Resume Encoding"];
       }

        else if ((s == HBStateWorking) || (s == HBStateMuxing))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"pauseencode"]];
            [toolbarItem setLabel: @"Pause"];
            [toolbarItem setToolTip: @"Pause Encoding"];
        }
        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"pauseencode"]];
            [toolbarItem setLabel: @"Pause"];
            [toolbarItem setToolTip: @"Pause Encoding"];
        }
    }

    return enable;
}

#pragma mark -


#pragma mark Queue Item Controls

- (void)HB_deleteSelectionFromTableView:(NSTableView *)tableView
{
    [self removeSelectedQueueItem:tableView];
}

//------------------------------------------------------------------------------------
// Delete encodes from the queue window and accompanying array
// Also handling first cancelling the encode if in fact its currently encoding.
//------------------------------------------------------------------------------------
- (IBAction)removeSelectedQueueItem: (id)sender
{
    NSIndexSet *targetedRow = [fOutlineView targetedRowIndexes];
    NSUInteger row = [targetedRow firstIndex];
    if (row == NSNotFound)
        return;
    /* if this is a currently encoding job, we need to be sure to alert the user,
     * to let them decide to cancel it first, then if they do, we can come back and
     * remove it */
    
    if ([fJobGroups[row] state] == HBJobStateWorking)
    {
       /* We pause the encode here so that it doesn't finish right after and then
        * screw up the sync while the window is open
        */
       [fHBController Pause:NULL];
         NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It ?", nil)];
        // Which window to attach the sheet to?
        NSWindow *docWindow = nil;
        if ([sender respondsToSelector: @selector(window)])
        {
            docWindow = [sender window];
        }

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:alertTitle];
        [alert setInformativeText:NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Keep Encoding", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Delete", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:docWindow
                          modalDelegate:self
                         didEndSelector:@selector(didDimissCancelCurrentJob:returnCode:contextInfo:)
                            contextInfo:nil];
        [alert release];
    }
    else
    { 
        // since we are not a currently encoding item, we can just be removed
        [fHBController removeQueueFileItem:row];
    }
}

- (void) didDimissCancelCurrentJob: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    /* We resume encoding and perform the appropriate actions 
     * Note: Pause: is a toggle type method based on hb's current
     * state, if it paused, it will resume encoding and vice versa.
     * In this case, we are paused from the calling window, so calling
     * [fHBController Pause:NULL]; Again will resume encoding
     */
    [fHBController Pause:NULL];
    if (returnCode == NSAlertSecondButtonReturn)
    {
        /* We need to save the currently encoding item number first */
        int encodingItemToRemove = fEncodingQueueItem;
        /* Since we are encoding, we need to let fHBController Cancel this job
         * upon which it will move to the next one if there is one
         */
        [fHBController doCancelCurrentJob];
        /* Now, we can go ahead and remove the job we just cancelled since
         * we have its item number from above
         */
        [fHBController removeQueueFileItem:encodingItemToRemove];
    }
    
}

//------------------------------------------------------------------------------------
// Show the finished encode in the finder
//------------------------------------------------------------------------------------
- (IBAction)revealSelectedQueueItem: (id)sender
{
    NSIndexSet *targetedRow = [fOutlineView targetedRowIndexes];
    NSInteger row = [targetedRow firstIndex];
    if (row != NSNotFound)
    {
        while (row != NSNotFound)
        {
            HBJob *queueItemToOpen = [fOutlineView itemAtRow:row];
            [[NSWorkspace sharedWorkspace] selectFile:queueItemToOpen.destURL.path inFileViewerRootedAtPath:nil];

            row = [targetedRow indexGreaterThanIndex: row];
        }
    }
}

//------------------------------------------------------------------------------------
// Starts or cancels the processing of jobs depending on the current state
//------------------------------------------------------------------------------------
- (IBAction)toggleStartCancel: (id)sender
{
    if (!self.queueCore) return;

    HBState s = self.queueCore.state;
    if ((s == HBStatePaused) || (s == HBStateWorking) || (s == HBStateMuxing))
    {
        [fHBController Cancel: self];
    }
    else if (fPendingCount > 0)
    {
        [fHBController Rip: NULL];
    }
}

//------------------------------------------------------------------------------------
// Toggles the pause/resume state of libhb
//------------------------------------------------------------------------------------
- (IBAction)togglePauseResume: (id)sender
{
    if (!self.queueCore) return;

    HBState s = self.queueCore.state;
    if (s == HBStatePaused)
    {
        [self.queueCore resume];
        [self startAnimatingCurrentWorkingEncodeInQueue];
    }
    else if ((s == HBStateWorking) || (s == HBStateMuxing))
    {
        [self.queueCore pause];
        [self stopAnimatingCurrentJobGroupInQueue];
    }
}

//------------------------------------------------------------------------------------
// Send the selected queue item back to the main window for rescan and possible edit.
//------------------------------------------------------------------------------------
- (IBAction)editSelectedQueueItem: (id)sender
{
    NSInteger row = [fOutlineView clickedRow];
    if (row == NSNotFound)
    {
        return;
    }

    /* if this is a currently encoding job, we need to be sure to alert the user,
     * to let them decide to cancel it first, then if they do, we can come back and
     * remove it */

    HBJob *job = fJobGroups[row];
    if (job.state == HBJobStateWorking)
    {
        // We pause the encode here so that it doesn't finish right after and then
        // screw up the sync while the window is open
        [fHBController Pause:NULL];
        NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It ?", nil)];
        // Which window to attach the sheet to?
        NSWindow *docWindow = nil;
        if ([sender respondsToSelector: @selector(window)])
        {
            docWindow = [sender window];
        }

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:alertTitle];
        [alert setInformativeText:NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Keep Encoding", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Delete", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:docWindow
                          modalDelegate:self
                         didEndSelector:@selector(didDimissCancelCurrentJob:returnCode:contextInfo:)
                            contextInfo:nil];
        [alert release];
    }
    else
    { 
        /* since we are not a currently encoding item, we can just be cancelled */
        [fHBController rescanQueueItemToMainWindow:row];
    }
}


#pragma mark -
#pragma mark Animate Encoding Item

//------------------------------------------------------------------------------------
// Starts animating the job icon of the currently processing job in the queue outline
// view.
//------------------------------------------------------------------------------------
- (void) startAnimatingCurrentWorkingEncodeInQueue
{
    if (!fAnimationTimer)
        fAnimationTimer = [[NSTimer scheduledTimerWithTimeInterval:1.0/12.0     // 1/12 because there are 6 images in the animation cycle
                target:self
                selector:@selector(animateWorkingEncodeInQueue:)
                userInfo:nil
                repeats:YES] retain];
}

//------------------------------------------------------------------------------------
// If a job is currently processing, its job icon in the queue outline view is
// animated to its next state.
//------------------------------------------------------------------------------------
- (void) animateWorkingEncodeInQueue:(NSTimer*)theTimer
{
    if (fWorkingCount > 0)
    {
        fAnimationIndex++;
        fAnimationIndex %= 6;   // there are 6 animation images; see outlineView:objectValueForTableColumn:byItem: below.
        [self animateWorkingEncodeIconInQueue];
    }
}

/* We need to make sure we denote only working encodes even for multiple instances */
- (void) animateWorkingEncodeIconInQueue
{
    NSInteger row = fEncodingQueueItem; /// need to set to fEncodingQueueItem
    NSInteger col = [fOutlineView columnWithIdentifier: @"icon"];
    if (row != -1 && col != -1)
    {
        NSRect frame = [fOutlineView frameOfCellAtColumn:col row:row];
        [fOutlineView setNeedsDisplayInRect: frame];
    }
}

//------------------------------------------------------------------------------------
// Stops animating the job icon of the currently processing job in the queue outline
// view.
//------------------------------------------------------------------------------------
- (void) stopAnimatingCurrentJobGroupInQueue
{
    if (fAnimationTimer && [fAnimationTimer isValid])
    {
        [fAnimationTimer invalidate];
        [fAnimationTimer release];
        fAnimationTimer = nil;
    }
}


#pragma mark -

- (void)moveObjectsInArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;

    while (index != NSNotFound)
    {
        NSUInteger removeIndex;

        if (index >= insertIndex)
        {
            removeIndex = index + aboveInsertIndexCount;
            aboveInsertIndexCount++;
        }
        else
        {
            removeIndex = index;
            insertIndex--;
        }

        id object = [array[removeIndex] retain];
        [array removeObjectAtIndex:removeIndex];
        [array insertObject:object atIndex:insertIndex];
        [object release];

        index = [indexSet indexLessThanIndex:index];
    }
}


#pragma mark -
#pragma mark NSOutlineView delegate


- (id)outlineView:(NSOutlineView *)fOutlineView child:(NSInteger)index ofItem:(id)item
{
    if (item == nil)
        return fJobGroups[index];

    // We are only one level deep, so we can't be asked about children
    NSAssert (NO, @"HBQueueController outlineView:child:ofItem: can't handle nested items.");
    return nil;
}

- (BOOL)outlineView:(NSOutlineView *)fOutlineView isItemExpandable:(id)item
{
    // Our outline view has no levels, but we can still expand every item. Doing so
    // just makes the row taller. See heightOfRowByItem below.
    return YES;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item
{
    // Our outline view has no levels, but we can still expand every item. Doing so
    // just makes the row taller. See heightOfRowByItem below.
    return ![(HBQueueOutlineView *)outlineView isDragging];
}

- (NSInteger)outlineView:(NSOutlineView *)fOutlineView numberOfChildrenOfItem:(id)item
{
    // Our outline view has no levels, so number of children will be zero for all
    // top-level items.
    if (item == nil)
        return [fJobGroups count];
    else
        return 0;
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    id item = [notification userInfo][@"NSObject"];
    NSInteger row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    id item = [notification userInfo][@"NSObject"];
    NSInteger row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (CGFloat)outlineView:(NSOutlineView *)outlineView heightOfRowByItem:(id)item
{
    if ([outlineView isItemExpanded: item])
    {
        // It is important to use a constant value when calculating the height. Querying the tableColumn width will not work, since it dynamically changes as the user resizes -- however, we don't get a notification that the user "did resize" it until after the mouse is let go. We use the latter as a hook for telling the table that the heights changed. We must return the same height from this method every time, until we tell the table the heights have changed. Not doing so will quicly cause drawing problems.
        NSTableColumn *tableColumnToWrap = (NSTableColumn *) [outlineView tableColumns][1];
        NSInteger columnToWrap = [outlineView.tableColumns indexOfObject:tableColumnToWrap];
        
        // Grab the fully prepared cell with our content filled in. Note that in IB the cell's Layout is set to Wraps.
        NSCell *cell = [outlineView preparedCellAtColumn:columnToWrap row:[outlineView rowForItem:item]];
        
        // See how tall it naturally would want to be if given a restricted with, but unbound height
        NSRect constrainedBounds = NSMakeRect(0, 0, [tableColumnToWrap width], CGFLOAT_MAX);
        NSSize naturalSize = [cell cellSizeForBounds:constrainedBounds];
        
        // Make sure we have a minimum height -- use the table's set height as the minimum.
        if (naturalSize.height > [outlineView rowHeight])
            return naturalSize.height;
        else
            return [outlineView rowHeight];
    }
    else
    {
        return HB_ROW_HEIGHT_TITLE_ONLY;
    }
}

- (void)initStyles
{
    // Attributes
    ps = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] retain];
    [ps setHeadIndent: 40.0];
    [ps setParagraphSpacing: 1.0];
    [ps setTabStops:@[]];    // clear all tabs
    [ps addTabStop: [[[NSTextTab alloc] initWithType: NSLeftTabStopType location: 20.0] autorelease]];

    detailAttr = [@{NSFontAttributeName: [NSFont systemFontOfSize:10.0],
                    NSParagraphStyleAttributeName: ps} retain];

    detailBoldAttr = [@{NSFontAttributeName: [NSFont boldSystemFontOfSize:10.0],
                        NSParagraphStyleAttributeName: ps} retain];

    titleAttr = [@{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont systemFontSize]],
                   NSParagraphStyleAttributeName: ps} retain];

    shortHeightAttr = [@{NSFontAttributeName: [NSFont systemFontOfSize:2.0]} retain];
}

- (id)outlineView:(NSOutlineView *)fOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"desc"])
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        /* Below should be put into a separate method but I am way too f'ing lazy right now */
        NSMutableAttributedString *finalString = [[NSMutableAttributedString alloc] initWithString: @""];
        HBJob *job = item;

        /* First line, we should strip the destination path and just show the file name and add the title num and chapters (if any) */
        NSString *summaryInfo;
        
        NSString *titleString = [NSString stringWithFormat:@"Title %d", job.titleIdx];
        
        NSString *startStopString = @"";
        if (job.range.type == HBRangeTypeChapters)
        {
            // Start Stop is chapters
            startStopString = (job.range.chapterStart == job.range.chapterStop) ?
            [NSString stringWithFormat:@"Chapter %d", job.range.chapterStart] :
            [NSString stringWithFormat:@"Chapters %d through %d", job.range.chapterStart, job.range.chapterStop];
        }
        else if (job.range.type == HBRangeTypeSeconds)
        {
            // Start Stop is seconds
            startStopString = [NSString stringWithFormat:@"Seconds %d through %d", job.range.secondsStart, job.range.secondsStop];
        }
        else if (job.range.type == HBRangeTypeFrames)
        {
            // Start Stop is Frames
            startStopString = [NSString stringWithFormat:@"Frames %d through %d", job.range.frameStart, job.range.frameStop];
        }
        NSString *passesString = @"";
        // check to see if our first subtitle track is Foreign Language Search, in which case there is an in depth scan
        if (job.subtitlesTracks.count && [job.subtitlesTracks[0][@"keySubTrackIndex"] intValue] == -1)
        {
            passesString = [passesString stringByAppendingString:@"1 Foreign Language Search Pass - "];
        }
        if (job.video.twoPass == YES)
        {
            passesString = [passesString stringByAppendingString:@"1 Video Pass"];
        }
        else
        {
            if (job.video.turboTwoPass == YES)
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes First Turbo"];
            }
            else
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes"];
            }
        }

        [finalString appendString:[NSString stringWithFormat:@"%@", job.fileURL.path.lastPathComponent] withAttributes:titleAttr];


        /* lets add the output file name to the title string here */
        NSString *outputFilenameString = job.destURL.lastPathComponent;
        
        summaryInfo = [NSString stringWithFormat: @" (%@, %@, %@) -> %@", titleString, startStopString, passesString, outputFilenameString];

        [finalString appendString:[NSString stringWithFormat:@"%@\n", summaryInfo] withAttributes:detailAttr];

        // Insert a short-in-height line to put some white space after the title
        [finalString appendString:@"\n" withAttributes:shortHeightAttr];
        // End of Title Stuff

        // Second Line  (Preset Name)
        // FIXME
        //[finalString appendString: @"Preset: " withAttributes:detailBoldAttr];
        //[finalString appendString:[NSString stringWithFormat:@"%@\n", item[@"PresetName"]] withAttributes:detailAttr];


        // Third Line  (Format Summary)
        NSString *audioCodecSummary = @"";	//	This seems to be set by the last track we have available...
        // Lets also get our audio track detail since we are going through the logic for use later

		NSMutableArray *audioDetails = [NSMutableArray arrayWithCapacity:job.audioTracks.count];
        BOOL autoPassthruPresent = NO;

        for (HBAudio *audioTrack in job.audioTracks)
        {
            audioCodecSummary = [NSString stringWithFormat: @"%@", audioTrack.codec[keyAudioCodecName]];
            NSNumber *drc = audioTrack.drc;
            NSNumber *gain = audioTrack.gain;
            NSString *detailString = [NSString stringWithFormat: @"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps), DRC: %@, Gain: %@",
                            audioTrack.track[keyAudioTrackName],
                            audioTrack.codec[keyAudioCodecName],
                            audioTrack.mixdown[keyAudioMixdownName],
                            audioTrack.sampleRate[keyAudioSampleRateName],
                            audioTrack.bitRate[keyAudioBitrateName],
                            (0.0 < [drc floatValue]) ? (NSObject *)drc : (NSObject *)@"Off",
                            (0.0 != [gain floatValue]) ? (NSObject *)gain : (NSObject *)@"Off"
                            ];
            [audioDetails addObject: detailString];
            // check if we have an Auto Passthru output track
            if ([audioTrack.codec[keyAudioCodecName] isEqualToString: @"Auto Passthru"])
            {
                autoPassthruPresent = YES;
            }
        }

        NSString *jobFormatInfo;
        if (job.chaptersEnabled)
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio, Chapter Markers\n",
                             @(hb_container_get_name(job.container)), @(hb_video_encoder_get_name(job.video.encoder)), audioCodecSummary];
        else
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio\n",
                             @(hb_container_get_name(job.container)), @(hb_video_encoder_get_name(job.video.encoder)), audioCodecSummary];
        
        [finalString appendString: @"Format: " withAttributes:detailBoldAttr];
        [finalString appendString: jobFormatInfo withAttributes:detailAttr];

        // Optional String for muxer options
        NSMutableString *containerOptions = [NSMutableString stringWithString:@""];
        if ((job.container & HB_MUX_MASK_MP4) && job.mp4HttpOptimize)
        {
            [containerOptions appendString:@" - Web optimized"];
        }
        if ((job.container & HB_MUX_MASK_MP4)  && job.mp4iPodCompatible)
        {
            [containerOptions appendString:@" - iPod 5G support"];
        }
        if ([containerOptions hasPrefix:@" - "])
        {
            [containerOptions deleteCharactersInRange:NSMakeRange(0, 3)];
        }
        if (containerOptions.length)
        {
            [finalString appendString:@"Container Options: " withAttributes:detailBoldAttr];
            [finalString appendString:containerOptions       withAttributes:detailAttr];
            [finalString appendString:@"\n"                  withAttributes:detailAttr];
        }

        // Fourth Line (Destination Path)
        [finalString appendString: @"Destination: " withAttributes:detailBoldAttr];
        [finalString appendString: job.destURL.path withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];


        // Fifth Line Picture Details
        NSString *pictureInfo = [NSString stringWithFormat:@"%@", job.picture.summary];
        if (job.picture.keepDisplayAspect)
        {
            pictureInfo = [pictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
        }
        [finalString appendString:@"Picture: " withAttributes:detailBoldAttr];
        [finalString appendString:pictureInfo  withAttributes:detailAttr];
        [finalString appendString:@"\n"        withAttributes:detailAttr];

        /* Optional String for Picture Filters */
        if (job.filters.summary.length)
        {
            NSString *pictureFilters = [NSString stringWithFormat:@"%@", job.filters.summary];
            [finalString appendString:@"Filters: "   withAttributes:detailBoldAttr];
            [finalString appendString:pictureFilters withAttributes:detailAttr];
            [finalString appendString:@"\n"          withAttributes:detailAttr];
        }

        // Sixth Line Video Details
        NSString * videoInfo = [NSString stringWithFormat:@"Encoder: %@", @(hb_video_encoder_get_name(job.video.encoder))];
        
        // for framerate look to see if we are using vfr detelecine
        if (job.video.frameRate == 0)
        {
            if (job.video.frameRateMode == 0)
            {
                // we are using same as source with vfr detelecine
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Variable Frame Rate)", videoInfo];
            }
            else
            {
                // we are using a variable framerate without dropping frames
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Constant Frame Rate)", videoInfo];
            }
        }
        else
        {
            // we have a specified, constant framerate
            if (job.video.frameRate == 0)
            {
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Peak Frame Rate)", videoInfo, @(hb_video_framerate_get_name(job.video.frameRate))];
            }
            else
            {
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Constant Frame Rate)", videoInfo, @(hb_video_framerate_get_name(job.video.frameRate))];
            }
        }


        if (job.video.qualityType == 0) // ABR
        {
            videoInfo = [NSString stringWithFormat:@"%@ Bitrate: %d(kbps)", videoInfo, job.video.avgBitrate];
        }
        else // CRF
        {
            videoInfo = [NSString stringWithFormat:@"%@ Constant Quality: %.2f", videoInfo ,job.video.quality];
        }

        [finalString appendString: @"Video: " withAttributes:detailBoldAttr];
        [finalString appendString: videoInfo withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];


        if (job.video.encoder == HB_VCODEC_X264 || job.video.encoder == HB_VCODEC_X265)
        {
            // we are using x264/x265
            NSString *encoderPresetInfo = @"";
            if (job.video.advancedOptions)
            {
                // we are using the old advanced panel
                if (job.video.videoOptionExtra.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString:job.video.videoOptionExtra];
                }
                else
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString:@"default settings"];
                }
            }
            else
            {
                // we are using the x264 system
                encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@"Preset: %@", job.video.preset]];
                if (job.video.tune.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Tune: %@", job.video.tune]];
                }
                if (job.video.videoOptionExtra.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Options: %@", job.video.videoOptionExtra]];
                }
                if (job.video.profile.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Profile: %@", job.video.profile]];
                }
                if (job.video.level.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Level: %@", job.video.level]];
                }
            }
            [finalString appendString: @"Encoder Options: " withAttributes:detailBoldAttr];
            [finalString appendString: encoderPresetInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        else
        {
            // we are using libavcodec
            NSString *lavcInfo = @"";
            if (job.video.videoOptionExtra.length)
            {
                lavcInfo = [lavcInfo stringByAppendingString:job.video.videoOptionExtra];
            }
            else
            {
                lavcInfo = [lavcInfo stringByAppendingString: @"default settings"];
            }
            [finalString appendString: @"Encoder Options: " withAttributes:detailBoldAttr];
            [finalString appendString: lavcInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }


        // Seventh Line Audio Details
		int audioDetailCount = 0;
		for (NSString *anAudioDetail in audioDetails) {
			audioDetailCount++;
			if (anAudioDetail.length) {
				[finalString appendString: [NSString stringWithFormat: @"Audio Track %d ", audioDetailCount] withAttributes: detailBoldAttr];
				[finalString appendString: anAudioDetail withAttributes: detailAttr];
				[finalString appendString: @"\n" withAttributes: detailAttr];
			}
		}

        // Eigth Line Auto Passthru Details
        // only print Auto Passthru settings if we have an Auro Passthru output track
        if (autoPassthruPresent == YES)
        {
            NSString *autoPassthruFallback = @"", *autoPassthruCodecs = @"";
            HBAudioDefaults *audioDefaults = job.audioDefaults;
            autoPassthruFallback = [autoPassthruFallback stringByAppendingString:@(hb_audio_encoder_get_name(audioDefaults.encoderFallback))];
            if (audioDefaults.allowAACPassthru)
            {
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"AAC"];
            }
            if (audioDefaults.allowAC3Passthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"AC3"];
            }
            if (audioDefaults.allowDTSHDPassthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"DTS-HD"];
            }
            if (audioDefaults.allowDTSPassthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"DTS"];
            }
            if (audioDefaults.allowMP3Passthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"MP3"];
            }
            [finalString appendString: @"Auto Passthru Codecs: " withAttributes: detailBoldAttr];
            if (autoPassthruCodecs.length)
            {
                [finalString appendString: autoPassthruCodecs withAttributes: detailAttr];
            }
            else
            {
                [finalString appendString: @"None" withAttributes: detailAttr];
            }
            [finalString appendString: @"\n" withAttributes: detailAttr];
            [finalString appendString: @"Auto Passthru Fallback: " withAttributes: detailBoldAttr];
            [finalString appendString: autoPassthruFallback withAttributes: detailAttr];
            [finalString appendString: @"\n" withAttributes: detailAttr];
        }

        // Ninth Line Subtitle Details
        for (NSDictionary *track in job.subtitlesTracks)
        {
            /* remember that index 0 of Subtitles can contain "Foreign Audio Search*/
            [finalString appendString: @"Subtitle: " withAttributes:detailBoldAttr];
            [finalString appendString: track[@"keySubTrackName"] withAttributes:detailAttr];
            if ([track[@"keySubTrackForced"] intValue] == 1)
            {
                [finalString appendString: @" - Forced Only" withAttributes:detailAttr];
            }
            if ([track[@"keySubTrackBurned"] intValue] == 1)
            {
                [finalString appendString: @" - Burned In" withAttributes:detailAttr];
            }
            if ([track[@"keySubTrackDefault"] intValue] == 1)
            {
                [finalString appendString: @" - Default" withAttributes:detailAttr];
            }
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }

        [pool release];

        return [finalString autorelease];
    }
    else if ([[tableColumn identifier] isEqualToString:@"icon"])
    {
        HBJob *job = item;
        if (job.state == HBJobStateCompleted)
        {
            return [NSImage imageNamed:@"EncodeComplete"];
        }
        else if (job.state == HBJobStateWorking)
        {
            return [NSImage imageNamed: [NSString stringWithFormat: @"EncodeWorking%d", fAnimationIndex]];
        }
        else if (job.state == HBJobStateCanceled)
        {
            return [NSImage imageNamed:@"EncodeCanceled"];
        }
        else
        {
            return [NSImage imageNamed:@"JobSmall"];
        }
    }
    else
    {
        return @"";
    }
}
/* This method inserts the proper action icons into the far right of the queue window */
- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"desc"])
    {
        // nb: The "desc" column is currently an HBImageAndTextCell. However, we are longer
        // using the image portion of the cell so we could switch back to a regular NSTextFieldCell.

        // Set the image here since the value returned from outlineView:objectValueForTableColumn: didn't specify the image part
        [cell setImage:nil];
    }
    else if ([[tableColumn identifier] isEqualToString:@"action"])
    {
        [cell setEnabled: YES];
        BOOL highlighted = [outlineView isRowSelected:[outlineView rowForItem: item]] && [[outlineView window] isKeyWindow] && ([[outlineView window] firstResponder] == outlineView);

        HBJob *job = item;
        if (job.state == HBJobStateCompleted)
        {
            [cell setAction: @selector(revealSelectedQueueItem:)];
            if (highlighted)
            {
                [cell setImage:[NSImage imageNamed:@"RevealHighlight"]];
                [cell setAlternateImage:[NSImage imageNamed:@"RevealHighlightPressed"]];
            }
            else
                [cell setImage:[NSImage imageNamed:@"Reveal"]];
        }
        else
        {
            [cell setAction: @selector(removeSelectedQueueItem:)];
            if (highlighted)
            {
                [cell setImage:[NSImage imageNamed:@"DeleteHighlight"]];
                [cell setAlternateImage:[NSImage imageNamed:@"DeleteHighlightPressed"]];
            }
            else
                [cell setImage:[NSImage imageNamed:@"Delete"]];
        }
    }
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayOutlineCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    // By default, the disclosure image gets centered vertically in the cell. We want
    // always at the top.
    if ([outlineView isItemExpanded: item])
        [cell setImagePosition: NSImageAbove];
    else
        [cell setImagePosition: NSImageOnly];
}

#pragma mark -
#pragma mark NSOutlineView delegate (dragging related)

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------


- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
    // Dragging is only allowed of the pending items.
    if ([items[0] state] != HBJobStateReady) // 2 is pending
    {
        return NO;
    }
    
    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    fDraggedNodes = items;
    
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:@[DragDropSimplePboardType] owner:self];
    
    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:DragDropSimplePboardType];
    
    return YES;
}


/* This method is used to validate the drops. */
- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index
{
    // Don't allow dropping ONTO an item since they can't really contain any children.
    BOOL isOnDropTypeProposal = index == NSOutlineViewDropOnItemIndex;
    if (isOnDropTypeProposal)
    {
        return NSDragOperationNone;
    }
    
    // Don't allow dropping INTO an item since they can't really contain any children.
    if (item != nil)
    {
        index = [fOutlineView rowForItem: item] + 1;
        item = nil;
    }
    
    // NOTE: Should we allow dropping a pending job *above* the
    // finished or already encoded jobs ?
    // We do not let the user drop a pending job before or *above*
    // already finished or currently encoding jobs.
    if (index <= fEncodingQueueItem)
    {
        return NSDragOperationNone;
        index = MAX (index, fEncodingQueueItem);
	}
    
    [outlineView setDropItem:item dropChildIndex:index];
    return NSDragOperationGeneric;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index
{
    NSMutableIndexSet *moveItems = [NSMutableIndexSet indexSet];

    for (id obj in fDraggedNodes)
        [moveItems addIndex:[fJobGroups indexOfObject:obj]];

    // Successful drop, we use moveObjectsInQueueArray:... in fHBController
    // to properly rearrange the queue array, save it to plist and then send it back here.
    // since Controller.mm is handling all queue array manipulation.
    // We *could do this here, but I think we are better served keeping that code together.
    [fHBController moveObjectsInQueueArray:fJobGroups fromIndexes:moveItems toIndex: index];
    return YES;
}

@end
