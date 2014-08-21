/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import "HBQueueController.h"
#import "Controller.h"
#import "HBImageAndTextCell.h"
#import "HBUtilities.h"

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


@implementation HBQueueOutlineView

- (void)viewDidEndLiveResize
{
    // Since we disabled calculating row heights during a live resize, force them to
    // recalculate now.
    [self noteHeightOfRowsWithIndexesChanged:
            [NSIndexSet indexSetWithIndexesInRange: NSMakeRange(0, [self numberOfRows])]];
    [super viewDidEndLiveResize];
}

/* This should be for dragging, we take this info from the presets right now */
- (NSImage *)dragImageForRowsWithIndexes:(NSIndexSet *)dragRows
                            tableColumns:(NSArray *)tableColumns
                                   event:(NSEvent *)dragEvent
                                  offset:(NSPointPointer)dragImageOffset
{
    _isDragging = YES;

    // By default, NSTableView only drags an image of the first column. Change this to
    // drag an image of the queue's icon and desc and action columns.
    NSArray * cols = @[[self tableColumnWithIdentifier:@"desc"], [self tableColumnWithIdentifier:@"icon"],[self tableColumnWithIdentifier:@"action"]];
    return [super dragImageForRowsWithIndexes:dragRows tableColumns:cols event:dragEvent offset:dragImageOffset];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [super mouseDown:theEvent];
	_isDragging = NO;
}

- (void)keyDown:(NSEvent *)event
{
    id delegate = [self delegate];

    unichar key = [[event charactersIgnoringModifiers] characterAtIndex:0];
    if ((key == NSDeleteCharacter || key == NSDeleteFunctionKey) &&
               [delegate respondsToSelector:@selector(removeSelectedQueueItem:)])
    {
        if ([self selectedRow] == -1)
        {
            NSBeep();
        }
        else
        {
            [delegate removeSelectedQueueItem:self];
        }
        return;
    }
    else
    {
        [super keyDown:event];
    }
}

@end

#pragma mark -

@interface HBQueueController ()
{
    hb_handle_t                  *fQueueEncodeLibhb;              // reference to libhb
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

    /* lets get the stats on the status of the queue array */
    
    fPendingCount = 0;
    fWorkingCount = 0;
    
    /* We use a number system to set the encode status of the queue item
     * in controller.mm
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */
	int i = 0;
    NSDictionary *thisQueueDict = nil;
	for (id tempObject in fJobGroups)
	{
		thisQueueDict = tempObject;
		if ([thisQueueDict[@"Status"] intValue] == 1) // being encoded
		{
			fWorkingCount++;
            /* we have an encoding job so, lets start the animation timer */
            if (thisQueueDict[@"EncodingPID"] && [thisQueueDict[@"EncodingPID"] intValue] == pidNum)
            {
                fEncodingQueueItem = i;
            }
		}
        if ([thisQueueDict[@"Status"] intValue] == 2) // pending		
        {
			fPendingCount++;
		}
		i++;
	}
    
    /* Set the queue status field in the queue window */
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
- (void)setHandle: (hb_handle_t *)handle
{
    fQueueEncodeLibhb = handle;
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

    if (!fQueueEncodeLibhb) return NO;

    BOOL enable = NO;

    hb_state_t s;
    hb_get_state2 (fQueueEncodeLibhb, &s);

    if ([[toolbarItem itemIdentifier] isEqualToString:@"HBQueueStartCancelToolbarIdentifier"])
    {
        if ((s.state == HB_STATE_PAUSED) || (s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
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
        if (s.state == HB_STATE_PAUSED)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"encode"]];
            [toolbarItem setLabel: @"Resume"];
            [toolbarItem setToolTip: @"Resume Encoding"];
       }

        else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
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
//------------------------------------------------------------------------------------
// Delete encodes from the queue window and accompanying array
// Also handling first cancelling the encode if in fact its currently encoding.
//------------------------------------------------------------------------------------
- (IBAction)removeSelectedQueueItem: (id)sender
{
    NSIndexSet * selectedRows = [fOutlineView selectedRowIndexes];
    NSUInteger row = [selectedRows firstIndex];
    if( row == NSNotFound )
        return;
    /* if this is a currently encoding job, we need to be sure to alert the user,
     * to let them decide to cancel it first, then if they do, we can come back and
     * remove it */
    
    if ([fJobGroups[row][@"Status"] integerValue] == 1)
    {
       /* We pause the encode here so that it doesn't finish right after and then
        * screw up the sync while the window is open
        */
       [fHBController Pause:NULL];
         NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It ?", nil)];
        // Which window to attach the sheet to?
        NSWindow * docWindow = nil;
        if ([sender respondsToSelector: @selector(window)])
            docWindow = [sender window];
        
        
        NSBeginCriticalAlertSheet(
                                  alertTitle,
                                  NSLocalizedString(@"Keep Encoding", nil),
                                  nil,
                                  NSLocalizedString(@"Stop Encoding and Delete", nil),
                                  docWindow, self,
                                  nil, @selector(didDimissCancelCurrentJob:returnCode:contextInfo:), nil,
                                  NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil));
        
        // didDimissCancelCurrentJob:returnCode:contextInfo: will be called when the dialog is dismissed
    }
    else
    { 
    /* since we are not a currently encoding item, we can just be removed */
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
    if (returnCode == NSAlertOtherReturn)
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
    NSIndexSet * selectedRows = [fOutlineView selectedRowIndexes];
    NSInteger row = [selectedRows firstIndex];
    if (row != NSNotFound)
    {
        while (row != NSNotFound)
        {
            NSMutableDictionary *queueItemToOpen = [fOutlineView itemAtRow: row];
            [[NSWorkspace sharedWorkspace] selectFile:queueItemToOpen[@"DestinationPath"] inFileViewerRootedAtPath:nil];

            row = [selectedRows indexGreaterThanIndex: row];
        }
    }
}

//------------------------------------------------------------------------------------
// Starts or cancels the processing of jobs depending on the current state
//------------------------------------------------------------------------------------
- (IBAction)toggleStartCancel: (id)sender
{
    if (!fQueueEncodeLibhb) return;

    hb_state_t s;
    hb_get_state2 (fQueueEncodeLibhb, &s);

    if ((s.state == HB_STATE_PAUSED) || (s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        [fHBController Cancel: self];

    else if (fPendingCount > 0)
        [fHBController Rip: NULL];
}

//------------------------------------------------------------------------------------
// Toggles the pause/resume state of libhb
//------------------------------------------------------------------------------------
- (IBAction)togglePauseResume: (id)sender
{
    if (!fQueueEncodeLibhb) return;
    
    hb_state_t s;
    hb_get_state2 (fQueueEncodeLibhb, &s);
    
    if (s.state == HB_STATE_PAUSED)
    {
        hb_resume (fQueueEncodeLibhb);
        [self startAnimatingCurrentWorkingEncodeInQueue];
    }
    else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
    {
        hb_pause (fQueueEncodeLibhb);
        [self stopAnimatingCurrentJobGroupInQueue];
    }
}


//------------------------------------------------------------------------------------
// Send the selected queue item back to the main window for rescan and possible edit.
//------------------------------------------------------------------------------------
- (IBAction)editSelectedQueueItem: (id)sender
{
    NSIndexSet * selectedRows = [fOutlineView selectedRowIndexes];
    NSUInteger row = [selectedRows firstIndex];
    if( row == NSNotFound )
        return;
    /* if this is a currently encoding job, we need to be sure to alert the user,
     * to let them decide to cancel it first, then if they do, we can come back and
     * remove it */
    
    if ([fJobGroups[row][@"Status"] integerValue] == 1)
    {
       /* We pause the encode here so that it doesn't finish right after and then
        * screw up the sync while the window is open
        */
       [fHBController Pause:NULL];
         NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It ?", nil)];
        // Which window to attach the sheet to?
        NSWindow * docWindow = nil;
        if ([sender respondsToSelector: @selector(window)])
            docWindow = [sender window];
        
        
        NSBeginCriticalAlertSheet(
                                  alertTitle,
                                  NSLocalizedString(@"Keep Encoding", nil),
                                  nil,
                                  NSLocalizedString(@"Stop Encoding and Delete", nil),
                                  docWindow, self,
                                  nil, @selector(didDimissCancelCurrentJob:returnCode:contextInfo:), nil,
                                  NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil));
        
    }
    else
    { 
        /* since we are not a currently encoding item, we can just be cancelled */
        [fHBController rescanQueueItemToMainWindow:fJobGroups[row][@"SourcePath"]
                                      scanTitleNum:[fJobGroups[row][@"TitleNumber"] integerValue]
                                 selectedQueueItem:row];
    
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
        NSMutableAttributedString * finalString = [[NSMutableAttributedString alloc] initWithString: @""];
        
        /* First line, we should strip the destination path and just show the file name and add the title num and chapters (if any) */
        NSString * summaryInfo;
        
        NSString * titleString = [NSString stringWithFormat:@"Title %d", [item[@"TitleNumber"] intValue]];
        
        NSString * startStopString = @"";
        if ([item[@"fEncodeStartStop"] intValue] == 0)
        {
            /* Start Stop is chapters */
            startStopString = ([item[@"ChapterStart"] intValue] == [item[@"ChapterEnd"] intValue]) ?
            [NSString stringWithFormat:@"Chapter %d", [item[@"ChapterStart"] intValue]] :
            [NSString stringWithFormat:@"Chapters %d through %d", [item[@"ChapterStart"] intValue], [item[@"ChapterEnd"] intValue]];
        }
        else if ([item[@"fEncodeStartStop"] intValue] == 1)
        {
            /* Start Stop is seconds */
            startStopString = [NSString stringWithFormat:@"Seconds %d through %d", [item[@"StartSeconds"] intValue], [item[@"StartSeconds"] intValue] + [item[@"StopSeconds"] intValue]];
        }
        else if ([item[@"fEncodeStartStop"] intValue] == 2)
        {
            /* Start Stop is Frames */
            startStopString = [NSString stringWithFormat:@"Frames %d through %d", [item[@"StartFrame"] intValue], [item[@"StartFrame"] intValue] + [item[@"StopFrame"] intValue]];
        }
        
        NSString * passesString = @"";
        /* check to see if our first subtitle track is Foreign Language Search, in which case there is an in depth scan */
        if ([item[@"SubtitleList"] count] && [item[@"SubtitleList"][0][@"keySubTrackIndex"] intValue] == -1)
        {
          passesString = [passesString stringByAppendingString:@"1 Foreign Language Search Pass - "];
        }
        if ([item[@"VideoTwoPass"] intValue] == 0)
        {
            passesString = [passesString stringByAppendingString:@"1 Video Pass"];
        }
        else
        {
            if ([item[@"VideoTurboTwoPass"] intValue] == 1)
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes First Turbo"];
            }
            else
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes"];
            }
        }
        
        [finalString appendString:[NSString stringWithFormat:@"%@", item[@"SourceName"]] withAttributes:titleAttr];
        
        /* lets add the output file name to the title string here */
        NSString * outputFilenameString = [item[@"DestinationPath"] lastPathComponent];
        
        summaryInfo = [NSString stringWithFormat: @" (%@, %@, %@) -> %@", titleString, startStopString, passesString, outputFilenameString];
        
        [finalString appendString:[NSString stringWithFormat:@"%@\n", summaryInfo] withAttributes:detailAttr];  
        
        // Insert a short-in-height line to put some white space after the title
        [finalString appendString:@"\n" withAttributes:shortHeightAttr];
        // End of Title Stuff
        
        /* Second Line  (Preset Name)*/
        [finalString appendString: @"Preset: " withAttributes:detailBoldAttr];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", item[@"PresetName"]] withAttributes:detailAttr];
        
        /* Third Line  (Format Summary) */
        NSString * audioCodecSummary = @"";	//	This seems to be set by the last track we have available...
        /* Lets also get our audio track detail since we are going through the logic for use later */

		NSMutableArray *audioDetails = [NSMutableArray arrayWithCapacity: [item[@"AudioList"] count]];
        BOOL autoPassthruPresent = NO;

        for (NSDictionary *audioTrack in item[@"AudioList"])
        {
            audioCodecSummary = [NSString stringWithFormat: @"%@", audioTrack[@"Encoder"]];
            NSNumber *drc = audioTrack[@"TrackDRCSlider"];
            NSNumber *gain = audioTrack[@"TrackGainSlider"];
            NSString *detailString = [NSString stringWithFormat: @"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps), DRC: %@, Gain: %@",
                            audioTrack[@"TrackDescription"],
                            audioTrack[@"Encoder"],
                            audioTrack[@"Mixdown"],
                            audioTrack[@"Samplerate"],
                            audioTrack[@"Bitrate"],
                            (0.0 < [drc floatValue]) ? (NSObject *)drc : (NSObject *)@"Off",
                            (0.0 != [gain floatValue]) ? (NSObject *)gain : (NSObject *)@"Off"
                            ];
            [audioDetails addObject: detailString];
            // check if we have an Auto Passthru output track
            if ([audioTrack[@"Encoder"] isEqualToString: @"Auto Passthru"])
            {
                autoPassthruPresent = YES;
            }
        }

        NSString * jobFormatInfo;
        if ([item[@"ChapterMarkers"] intValue] == 1)
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio, Chapter Markers\n", item[@"FileFormat"], item[@"VideoEncoder"], audioCodecSummary];
        else
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio\n", item[@"FileFormat"], item[@"VideoEncoder"], audioCodecSummary];
        
        
        [finalString appendString: @"Format: " withAttributes:detailBoldAttr];
        [finalString appendString: jobFormatInfo withAttributes:detailAttr];
        
        /* Optional String for muxer options */
        if ([item[@"MuxerOptionsSummary"] length])
        {
            NSString *containerOptions = [NSString stringWithFormat:@"%@",
                                          item[@"MuxerOptionsSummary"]];
            [finalString appendString:@"Container Options: " withAttributes:detailBoldAttr];
            [finalString appendString:containerOptions       withAttributes:detailAttr];
            [finalString appendString:@"\n"                  withAttributes:detailAttr];
        }
        
        /* Fourth Line (Destination Path)*/
        [finalString appendString: @"Destination: " withAttributes:detailBoldAttr];
        [finalString appendString: item[@"DestinationPath"] withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        /* Fifth Line Picture Details*/
        NSString *pictureInfo = [NSString stringWithFormat:@"%@",
                                 item[@"PictureSettingsSummary"]];
        if ([item[@"PictureKeepRatio"] intValue] == 1)
        {
            pictureInfo = [pictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
        }
        [finalString appendString:@"Picture: " withAttributes:detailBoldAttr];
        [finalString appendString:pictureInfo  withAttributes:detailAttr];
        [finalString appendString:@"\n"        withAttributes:detailAttr];
        
        /* Optional String for Picture Filters */
        if ([item[@"PictureFiltersSummary"] length])
        {
            NSString *pictureFilters = [NSString stringWithFormat:@"%@",
                                        item[@"PictureFiltersSummary"]];
            [finalString appendString:@"Filters: "   withAttributes:detailBoldAttr];
            [finalString appendString:pictureFilters withAttributes:detailAttr];
            [finalString appendString:@"\n"          withAttributes:detailAttr];
        }
        
        /* Sixth Line Video Details*/
        NSString * videoInfo;
        videoInfo = [NSString stringWithFormat:@"Encoder: %@", item[@"VideoEncoder"]];
        
        /* for framerate look to see if we are using vfr detelecine */
        if ([item[@"JobIndexVideoFramerate"] intValue] == 0)
        {
            if ([item[@"VideoFramerateMode"] isEqualToString:@"vfr"])
            {
                /* we are using same as source with vfr detelecine */
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Variable Frame Rate)", videoInfo];
            }
            else
            {
                /* we are using a variable framerate without dropping frames */
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Constant Frame Rate)", videoInfo];
            }
        }
        else
        {
            /* we have a specified, constant framerate */
            if ([item[@"VideoFramerateMode"] isEqualToString:@"pfr"])
            {
            videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Peak Frame Rate)", videoInfo ,item[@"VideoFramerate"]];
            }
            else
            {
            videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Constant Frame Rate)", videoInfo ,item[@"VideoFramerate"]];
            }
        }
        
        if ([item[@"VideoQualityType"] intValue] == 0)// Target Size MB
        {
            videoInfo = [NSString stringWithFormat:@"%@ Target Size: %@(MB) (%d(kbps) abr)", videoInfo ,item[@"VideoTargetSize"],[item[@"VideoAvgBitrate"] intValue]];
        }
        else if ([item[@"VideoQualityType"] intValue] == 1) // ABR
        {
            videoInfo = [NSString stringWithFormat:@"%@ Bitrate: %d(kbps)", videoInfo ,[item[@"VideoAvgBitrate"] intValue]];
        }
        else // CRF
        {
            videoInfo = [NSString stringWithFormat:@"%@ Constant Quality: %.2f", videoInfo ,[item[@"VideoQualitySlider"] floatValue]];
        }
        
        [finalString appendString: @"Video: " withAttributes:detailBoldAttr];
        [finalString appendString: videoInfo withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        if ([item[@"VideoEncoder"] isEqualToString: @"H.264 (x264)"] || [item[@"VideoEncoder"] isEqualToString: @"H.265 (x265)"])
        {
            /* we are using x264/x265 */
            NSString *encoderPresetInfo = @"";
            if ([item[@"x264UseAdvancedOptions"] intValue])
            {
                // we are using the old advanced panel
                if (item[@"x264Option"] &&
                    [item[@"x264Option"] length])
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: item[@"x264Option"]];
                }
                else
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: @"default settings"];
                }
            }
            else
            {
                // we are using the x264 system
                encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@"Preset: %@", item[@"VideoPreset"]]];
                if ([item[@"VideoTune"] length])
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Tune: %@", item[@"VideoTune"]]];
                }
                if ([item[@"VideoOptionExtra"] length])
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Options: %@", item[@"VideoOptionExtra"]]];
                }
                if ([item[@"VideoProfile"] length])
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Profile: %@", item[@"VideoProfile"]]];
                }
                if ([item[@"VideoLevel"] length])
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Level: %@", item[@"VideoLevel"]]];
                }
            }
            [finalString appendString: @"Encoder Options: " withAttributes:detailBoldAttr];
            [finalString appendString: encoderPresetInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        else if (![item[@"VideoEncoder"] isEqualToString: @"VP3 (Theora)"])
        {
            /* we are using libavcodec */
            NSString *lavcInfo = @"";
            if (item[@"lavcOption"] &&
                [item[@"lavcOption"] length])
            {
                lavcInfo = [lavcInfo stringByAppendingString: item[@"lavcOption"]];
            }
            else
            {
                lavcInfo = [lavcInfo stringByAppendingString: @"default settings"];
            }
            [finalString appendString: @"ffmpeg: " withAttributes:detailBoldAttr];
            [finalString appendString: lavcInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        
        
        
        
        /* Seventh Line Audio Details*/
        NSEnumerator *audioDetailEnumerator = [audioDetails objectEnumerator];
		NSString *anAudioDetail;
		int audioDetailCount = 0;
		while (nil != (anAudioDetail = [audioDetailEnumerator nextObject])) {
			audioDetailCount++;
			if (0 < [anAudioDetail length]) {
				[finalString appendString: [NSString stringWithFormat: @"Audio Track %d ", audioDetailCount] withAttributes: detailBoldAttr];
				[finalString appendString: anAudioDetail withAttributes: detailAttr];
				[finalString appendString: @"\n" withAttributes: detailAttr];
			}
		}
        
        /* Eigth Line Auto Passthru Details */
        // only print Auto Passthru settings if we have an Auro Passthru output track
        if (autoPassthruPresent == YES)
        {
            NSString *autoPassthruFallback = @"", *autoPassthruCodecs = @"";
            autoPassthruFallback = [autoPassthruFallback stringByAppendingString: item[@"AudioEncoderFallback"]];
            if (0 < [item[@"AudioAllowAACPass"] intValue])
            {
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @"AAC"];
            }
            if (0 < [item[@"AudioAllowAC3Pass"] intValue])
            {
                if (0 < [autoPassthruCodecs length])
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @"AC3"];
            }
            if (0 < [item[@"AudioAllowDTSHDPass"] intValue])
            {
                if (0 < [autoPassthruCodecs length])
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @"DTS-HD"];
            }
            if (0 < [item[@"AudioAllowDTSPass"] intValue])
            {
                if (0 < [autoPassthruCodecs length])
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @"DTS"];
            }
            if (0 < [item[@"AudioAllowMP3Pass"] intValue])
            {
                if (0 < [autoPassthruCodecs length])
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString: @"MP3"];
            }
            [finalString appendString: @"Auto Passthru Codecs: " withAttributes: detailBoldAttr];
            if (0 < [autoPassthruCodecs length])
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
        
        /* Ninth Line Subtitle Details */
        
        int i = 0;
        NSEnumerator *enumerator = [item[@"SubtitleList"] objectEnumerator];
        id tempObject;
        while (tempObject = [enumerator nextObject])
        {
            /* remember that index 0 of Subtitles can contain "Foreign Audio Search*/
            [finalString appendString: @"Subtitle: " withAttributes:detailBoldAttr];
            [finalString appendString: tempObject[@"keySubTrackName"] withAttributes:detailAttr];
            if ([tempObject[@"keySubTrackForced"] intValue] == 1)
            {
                [finalString appendString: @" - Forced Only" withAttributes:detailAttr];
            }
            if ([tempObject[@"keySubTrackBurned"] intValue] == 1)
            {
                [finalString appendString: @" - Burned In" withAttributes:detailAttr];
            }
            if ([tempObject[@"keySubTrackDefault"] intValue] == 1)
            {
                [finalString appendString: @" - Default" withAttributes:detailAttr];
            }
            [finalString appendString:@"\n" withAttributes:detailAttr];
            i++;
        }

        [pool release];

        return [finalString autorelease];
    }
    else if ([[tableColumn identifier] isEqualToString:@"icon"])
    {
        if ([item[@"Status"] intValue] == 0)
        {
            return [NSImage imageNamed:@"EncodeComplete"];
        }
        else if ([item[@"Status"] intValue] == 1)
        {
            return [NSImage imageNamed: [NSString stringWithFormat: @"EncodeWorking%d", fAnimationIndex]];
        }
        else if ([item[@"Status"] intValue] == 3)
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
        
        if ([item[@"Status"] intValue] == 0 || ([item[@"Status"] intValue] == 1 && [item[@"EncodingPID"] intValue] != pidNum))
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
    if ([items[0][@"Status"] integerValue] != 2) // 2 is pending
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

    for( id obj in fDraggedNodes )
        [moveItems addIndex:[fJobGroups indexOfObject:obj]];

    // Successful drop, we use moveObjectsInQueueArray:... in fHBController
    // to properly rearrange the queue array, save it to plist and then send it back here.
    // since Controller.mm is handling all queue array manipulation.
    // We *could do this here, but I think we are better served keeping that code together.
    [fHBController moveObjectsInQueueArray:fJobGroups fromIndexes:moveItems toIndex: index];
    return YES;
}

@end
