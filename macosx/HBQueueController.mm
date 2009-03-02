/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import "HBQueueController.h"
#import "Controller.h"
#import "HBImageAndTextCell.h"

#define HB_ROW_HEIGHT_TITLE_ONLY           17.0
#define HB_ROW_HEIGHT_FULL_DESCRIPTION           200.0
// Pasteboard type for or drag operations
#define DragDropSimplePboardType 	@"MyCustomOutlineViewPboardType"

//------------------------------------------------------------------------------------
#pragma mark -
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// NSMutableAttributedString (HBAdditions)
//------------------------------------------------------------------------------------

@interface NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary;
@end

@implementation NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary
{
    NSAttributedString * s = [[[NSAttributedString alloc]
        initWithString: aString
        attributes: aDictionary] autorelease];
    [self appendAttributedString: s];
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
- (NSImage *)dragImageForRowsWithIndexes:(NSIndexSet *)dragRows tableColumns:(NSArray *)tableColumns event:(NSEvent*)dragEvent offset:(NSPointPointer)dragImageOffset
{
    fIsDragging = YES;

    // By default, NSTableView only drags an image of the first column. Change this to
    // drag an image of the queue's icon and desc and action columns.
    NSArray * cols = [NSArray arrayWithObjects: [self tableColumnWithIdentifier:@"desc"], [self tableColumnWithIdentifier:@"icon"],[self tableColumnWithIdentifier:@"action"], nil];
    return [super dragImageForRowsWithIndexes:dragRows tableColumns:cols event:dragEvent offset:dragImageOffset];
}



- (void) mouseDown:(NSEvent *)theEvent
{
    [super mouseDown:theEvent];
	fIsDragging = NO;
}



- (BOOL) isDragging;
{
    return fIsDragging;
}



@end

#pragma mark Toolbar Identifiers
// Toolbar identifiers
static NSString*    HBQueueToolbar                            = @"HBQueueToolbar1";
static NSString*    HBQueueStartCancelToolbarIdentifier       = @"HBQueueStartCancelToolbarIdentifier";
static NSString*    HBQueuePauseResumeToolbarIdentifier       = @"HBQueuePauseResumeToolbarIdentifier";

#pragma mark -

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
        [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
            @"NO",      @"QueueWindowIsOpen",
            @"NO",      @"QueueShowsDetail",
            @"YES",     @"QueueShowsJobsAsGroups",
            nil]];

        fJobGroups = [[NSMutableArray arrayWithCapacity:0] retain];
       } 
        return self;
}

- (void)setQueueArray: (NSMutableArray *)QueueFileArray
{
    [fJobGroups setArray:QueueFileArray];
    fIsDragging = NO; 
    /* First stop any timer working now */
    [self stopAnimatingCurrentJobGroupInQueue];
    [fOutlineView reloadData];
    
    
    
    /* lets get the stats on the status of the queue array */
    
    fEncodingQueueItem = 0;
    fPendingCount = 0;
    fCompletedCount = 0;
    fCanceledCount = 0;
    fWorkingCount = 0;
    
    /* We use a number system to set the encode status of the queue item
     * in controller.mm
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */
    
	int i = 0;
	for(id tempObject in fJobGroups)
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 0) // Completed
		{
			fCompletedCount++;	
		}
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 1) // being encoded
		{
			fWorkingCount++;
            fEncodingQueueItem = i;	
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 2) // pending		
        {
			fPendingCount++;
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 3) // cancelled		
        {
			fCanceledCount++;
		}
		i++;
	}
    
    /* We should fire up the encoding timer here based on fWorkingCount */
    
    if (fWorkingCount > 0)
    {
        /* we have an encoding job so, lets start the animation timer */
        [self startAnimatingCurrentWorkingEncodeInQueue];
    }
    
    /* Set the queue status field in the queue window */
    NSMutableString * string;
    if (fPendingCount == 1)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode pending", @"" ), fPendingCount];
    }
    else
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode(s) pending", @"" ), fPendingCount];
    }
    [fQueueCountField setStringValue:string];
    
}
/* This method sets the status string in the queue window
 * and is called from Controller.mm (fHBController)
 * instead of running another timer here polling libhb
 * for encoding status
 */
- (void)setQueueStatusString: (NSString *)statusString
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

#pragma mark -

//------------------------------------------------------------------------------------
// Displays and brings the queue window to the front
//------------------------------------------------------------------------------------
- (IBAction) showQueueWindow: (id)sender
{
    [self showWindow:sender];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"QueueWindowIsOpen"];
}



//------------------------------------------------------------------------------------
// awakeFromNib
//------------------------------------------------------------------------------------
- (void)awakeFromNib
{
    [self setupToolbar];

    if( ![[self window] setFrameUsingName:@"Queue"] )
        [[self window] center];
    [self setWindowFrameAutosaveName:@"Queue"];
    [[self window] setExcludedFromWindowsMenu:YES];

    /* lets setup our queue list outline view for drag and drop here */
    [fOutlineView registerForDraggedTypes: [NSArray arrayWithObject:DragDropSimplePboardType] ];
    [fOutlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [fOutlineView setVerticalMotionCanBeginDrag: YES];


    // Don't allow autoresizing of main column, else the "delete" column will get
    // pushed out of view.
    [fOutlineView setAutoresizesOutlineColumn: NO];

#if HB_OUTLINE_METRIC_CONTROLS
    [fIndentation setHidden: NO];
    [fSpacing setHidden: NO];
    [fIndentation setIntegerValue:[fOutlineView indentationPerLevel]];  // debug
    [fSpacing setIntegerValue:3];       // debug
#endif

    // Show/hide UI elements
    fCurrentJobPaneShown = NO;     // it's shown in the nib
    //[self showCurrentJobPane:NO];

    //[self updateQueueCountField];
}


//------------------------------------------------------------------------------------
// windowWillClose
//------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
}

#pragma mark Toolbar

//------------------------------------------------------------------------------------
// setupToolbar
//------------------------------------------------------------------------------------
- (void)setupToolbar
{
    // Create a new toolbar instance, and attach it to our window
    NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier: HBQueueToolbar] autorelease];

    // Set up toolbar properties: Allow customization, give a default display mode, and remember state in user defaults
    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];

    // We are the delegate
    [toolbar setDelegate: self];

    // Attach the toolbar to our window
    [[self window] setToolbar:toolbar];
}

//------------------------------------------------------------------------------------
// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:
//------------------------------------------------------------------------------------
- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
        itemForItemIdentifier:(NSString *)itemIdentifier
        willBeInsertedIntoToolbar:(BOOL)flag
{
    // Required delegate method: Given an item identifier, this method returns an item.
    // The toolbar will use this method to obtain toolbar items that can be displayed
    // in the customization sheet, or in the toolbar itself.

    NSToolbarItem *toolbarItem = nil;

    if ([itemIdentifier isEqual: HBQueueStartCancelToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];

        // Set the text label to be displayed in the toolbar and customization palette
        [toolbarItem setLabel: @"Start"];
        [toolbarItem setPaletteLabel: @"Start/Cancel"];

        // Set up a reasonable tooltip, and image
        [toolbarItem setToolTip: @"Start Encoding"];
        [toolbarItem setImage: [NSImage imageNamed: @"Play"]];

        // Tell the item what message to send when it is clicked
        [toolbarItem setTarget: self];
        [toolbarItem setAction: @selector(toggleStartCancel:)];
    }

    if ([itemIdentifier isEqual: HBQueuePauseResumeToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];

        // Set the text label to be displayed in the toolbar and customization palette
        [toolbarItem setLabel: @"Pause"];
        [toolbarItem setPaletteLabel: @"Pause/Resume"];

        // Set up a reasonable tooltip, and image
        [toolbarItem setToolTip: @"Pause Encoding"];
        [toolbarItem setImage: [NSImage imageNamed: @"Pause"]];

        // Tell the item what message to send when it is clicked
        [toolbarItem setTarget: self];
        [toolbarItem setAction: @selector(togglePauseResume:)];
    }

    return toolbarItem;
}

//------------------------------------------------------------------------------------
// toolbarDefaultItemIdentifiers:
//------------------------------------------------------------------------------------
- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    // Required delegate method: Returns the ordered list of items to be shown in the
    // toolbar by default.

    return [NSArray arrayWithObjects:
        HBQueueStartCancelToolbarIdentifier,
        HBQueuePauseResumeToolbarIdentifier,
        nil];
}

//------------------------------------------------------------------------------------
// toolbarAllowedItemIdentifiers:
//------------------------------------------------------------------------------------
- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    // Required delegate method: Returns the list of all allowed items by identifier.
    // By default, the toolbar does not assume any items are allowed, even the
    // separator. So, every allowed item must be explicitly listed.

    return [NSArray arrayWithObjects:
        HBQueueStartCancelToolbarIdentifier,
        HBQueuePauseResumeToolbarIdentifier,
        NSToolbarCustomizeToolbarItemIdentifier,
        NSToolbarFlexibleSpaceItemIdentifier,
        NSToolbarSpaceItemIdentifier,
        NSToolbarSeparatorItemIdentifier,
        nil];
}

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

    if ([[toolbarItem itemIdentifier] isEqual: HBQueueStartCancelToolbarIdentifier])
    {
        if ((s.state == HB_STATE_PAUSED) || (s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Stop"]];
            [toolbarItem setLabel: @"Stop"];
            [toolbarItem setToolTip: @"Stop Encoding"];
        }

        else if (fPendingCount > 0)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
            [toolbarItem setLabel: @"Start"];
            [toolbarItem setToolTip: @"Start Encoding"];
        }

        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
            [toolbarItem setLabel: @"Start"];
            [toolbarItem setToolTip: @"Start Encoding"];
        }
    }

    if ([[toolbarItem itemIdentifier] isEqual: HBQueuePauseResumeToolbarIdentifier])
    {
        if (s.state == HB_STATE_PAUSED)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
            [toolbarItem setLabel: @"Resume"];
            [toolbarItem setToolTip: @"Resume Encoding"];
       }

        else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Pause"]];
            [toolbarItem setLabel: @"Pause"];
            [toolbarItem setToolTip: @"Pause Encoding"];
        }
        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"Pause"]];
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
    
    if ([[[fJobGroups objectAtIndex:row] objectForKey:@"Status"] integerValue] == 1)
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
    /* since we are not a currently encoding item, we can just be cancelled */
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
         [[NSWorkspace sharedWorkspace] selectFile:[queueItemToOpen objectForKey:@"DestinationPath"] inFileViewerRootedAtPath:nil];

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
        [fHBController Cancel: fQueuePane]; // sender == fQueuePane so that warning alert shows up on queue window

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

#pragma mark -


#pragma mark Animate Endcoding Item




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

        id object = [[array objectAtIndex:removeIndex] retain];
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
        return [fJobGroups objectAtIndex:index];

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
return ![(HBQueueOutlineView*)outlineView isDragging];
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
    id item = [[notification userInfo] objectForKey:@"NSObject"];
    NSInteger row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    id item = [[notification userInfo] objectForKey:@"NSObject"];
    NSInteger row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (CGFloat)outlineView:(NSOutlineView *)outlineView heightOfRowByItem:(id)item
{
    if ([outlineView isItemExpanded: item])
    {
        // Short-circuit here if in a live resize primarily to fix a bug but also to
        // increase resposivness during a resize. There's a bug in NSTableView that
        // causes row heights to get messed up if you try to change them during a live
        // resize. So if in a live resize, simply return the previously calculated
        // height. The row heights will get fixed up after the resize because we have
        // implemented viewDidEndLiveResize to force all of them to be recalculated.
        // if ([outlineView inLiveResize] && [item lastDescriptionHeight] > 0)
         //   return [item lastDescriptionHeight];

        // CGFloat width = [[outlineView tableColumnWithIdentifier: @"desc"] width];
        // Column width is NOT what is ultimately used. I can't quite figure out what
        // width to use for calculating text metrics. No matter how I tweak this value,
        // there are a few conditions in which the drawn text extends below the bounds
        // of the row cell. In previous versions, which ran under Tiger, I was
        // reducing width by 47 pixles.
        // width -= 2;     // (?) for intercell spacing

        // CGFloat height = [item heightOfDescriptionForWidth: width];
        // return height;
        
        return HB_ROW_HEIGHT_FULL_DESCRIPTION;
    }
    else
        return HB_ROW_HEIGHT_TITLE_ONLY;
}

- (CGFloat) heightOfDescriptionForWidth:(CGFloat)width
{
    // Try to return the cached value if no changes have happened since the last time
    //if ((width == fLastDescriptionWidth) && (fLastDescriptionHeight != 0) && !fNeedsDescription)
       // return fLastDescriptionHeight;

    //if (fNeedsDescription)
    //    [self updateDescription];

    // Calculate the height
    //NSRect bounds = [fDescription boundingRectWithSize:NSMakeSize(width, 10000) options:NSStringDrawingUsesLineFragmentOrigin];
    //fLastDescriptionHeight = bounds.size.height + 6.0; // add some border to bottom
    //fLastDescriptionWidth = width;
    return HB_ROW_HEIGHT_FULL_DESCRIPTION;

/* supposedly another way to do this, in case boundingRectWithSize isn't working
    NSTextView* tmpView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, width, 1)];
    [[tmpView textStorage] setAttributedString:aString];
    [tmpView setHorizontallyResizable:NO];
    [tmpView setVerticallyResizable:YES];
//    [[tmpView textContainer] setHeightTracksTextView: YES];
//    [[tmpView textContainer] setContainerSize: NSMakeSize(width, 10000)];
    [tmpView sizeToFit];
    float height = [tmpView frame].size.height;
    [tmpView release];
    return height;
*/
}

- (CGFloat) lastDescriptionHeight
{
    return HB_ROW_HEIGHT_FULL_DESCRIPTION;
}

- (id)outlineView:(NSOutlineView *)fOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    // nb: The "desc" column is currently an HBImageAndTextCell. However, we are longer
    // using the image portion of the cell so we could switch back to a regular NSTextFieldCell.
    
    if ([[tableColumn identifier] isEqualToString:@"desc"])
    {
        /* This should have caused the description we wanted to show*/
        //return [item objectForKey:@"SourceName"];
        
        /* code to build the description as per old queue */
        //return [self formatEncodeItemDescription:item];
        
        /* Below should be put into a separate method but I am way too f'ing lazy right now */
        NSMutableAttributedString * finalString = [[[NSMutableAttributedString alloc] initWithString: @""] autorelease];
        // Attributes
        NSMutableParagraphStyle * ps = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] retain];
        [ps setHeadIndent: 40.0];
        [ps setParagraphSpacing: 1.0];
        [ps setTabStops:[NSArray array]];    // clear all tabs
        [ps addTabStop: [[[NSTextTab alloc] initWithType: NSLeftTabStopType location: 20.0] autorelease]];
        
        
        NSDictionary* detailAttr = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSFont systemFontOfSize:10.0], NSFontAttributeName,
                                    ps, NSParagraphStyleAttributeName,
                                    nil];
        
        NSDictionary* detailBoldAttr = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSFont boldSystemFontOfSize:10.0], NSFontAttributeName,
                                        ps, NSParagraphStyleAttributeName,
                                        nil];
        
        NSDictionary* titleAttr = [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSFont systemFontOfSize:[NSFont systemFontSize]], NSFontAttributeName,
                                   ps, NSParagraphStyleAttributeName,
                                   nil];
        
        NSDictionary* shortHeightAttr = [NSDictionary dictionaryWithObjectsAndKeys:
                                         [NSFont systemFontOfSize:2.0], NSFontAttributeName,
                                         nil];
        
        /* First line, we should strip the destination path and just show the file name and add the title num and chapters (if any) */
        //finalDescription = [finalDescription stringByAppendingString:[NSString stringWithFormat:@"Source: %@ Output: %@\n", [item objectForKey:@"SourceName"],[item objectForKey:@"DestinationPath"]]];
        NSString * summaryInfo;
        
        NSString * titleString = [NSString stringWithFormat:@"Title %d", [[item objectForKey:@"TitleNumber"] intValue]];
        
        NSString * chapterString = ([[item objectForKey:@"ChapterStart"] intValue] == [[item objectForKey:@"ChapterEnd"] intValue]) ?
        [NSString stringWithFormat:@"Chapter %d", [[item objectForKey:@"ChapterStart"] intValue]] :
        [NSString stringWithFormat:@"Chapters %d through %d", [[item objectForKey:@"ChapterStart"] intValue], [[item objectForKey:@"ChapterEnd"] intValue]];
        
        NSString * passesString;
        if ([[item objectForKey:@"VideoTwoPass"] intValue] == 0)
        {
            passesString = [NSString stringWithFormat:@"1 Video Pass"];
        }
        else
        {
            if ([[item objectForKey:@"VideoTurboTwoPass"] intValue] == 1)
            {
                passesString = [NSString stringWithFormat:@"2 Video Passes Turbo"];
            }
            else
            {
                passesString = [NSString stringWithFormat:@"2 Video Passes"];
            }
        }
        
        [finalString appendString:[NSString stringWithFormat:@"%@", [item objectForKey:@"SourceName"]] withAttributes:titleAttr];
        
        /* lets add the output file name to the title string here */
        NSString * outputFilenameString = [[item objectForKey:@"DestinationPath"] lastPathComponent];
        
        summaryInfo = [NSString stringWithFormat: @" (%@, %@, %@) -> %@", titleString, chapterString, passesString, outputFilenameString];
        
        [finalString appendString:[NSString stringWithFormat:@"%@\n", summaryInfo] withAttributes:detailAttr];  
        
        // Insert a short-in-height line to put some white space after the title
        [finalString appendString:@"\n" withAttributes:shortHeightAttr];
        // End of Title Stuff
        
        /* Second Line  (Preset Name)*/
        [finalString appendString: @"Preset: " withAttributes:detailBoldAttr];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", [item objectForKey:@"PresetName"]] withAttributes:detailAttr];
        
        /* Third Line  (Format Summary) */
        NSString * audioCodecSummary = @"";
        /* Lets also get our audio track detail since we are going through the logic for use later */
        NSString * audioDetail1 = @"None";
        NSString * audioDetail2 = @"None";
        NSString * audioDetail3 = @"None";
        NSString * audioDetail4 = @"None";
        if ([[item objectForKey:@"Audio1Track"] intValue] > 0)
        {
            audioCodecSummary = [NSString stringWithFormat:@"%@", [item objectForKey:@"Audio1Encoder"]];
            audioDetail1 = [NSString stringWithFormat:@"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps)",
                            [item objectForKey:@"Audio1TrackDescription"] ,
                            [item objectForKey:@"Audio1Encoder"],
                            [item objectForKey:@"Audio1Mixdown"] ,
                            [item objectForKey:@"Audio1Samplerate"],
                            [item objectForKey:@"Audio1Bitrate"]];
            
            if ([[item objectForKey:@"Audio1TrackDRCSlider"] floatValue] > 1.00)
            {
                audioDetail1 = [NSString stringWithFormat:@"%@, DRC: %@",audioDetail1,[item objectForKey:@"Audio1TrackDRCSlider"]];
            }
            else
            {
                audioDetail1 = [NSString stringWithFormat:@"%@, DRC: Off",audioDetail1];
            }
        }
        
        if ([[item objectForKey:@"Audio2Track"] intValue] > 0)
        {
            audioCodecSummary = [NSString stringWithFormat:@"%@, %@",audioCodecSummary ,[item objectForKey:@"Audio2Encoder"]];
            audioDetail2 = [NSString stringWithFormat:@"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps)",
                            [item objectForKey:@"Audio2TrackDescription"] ,
                            [item objectForKey:@"Audio2Encoder"],
                            [item objectForKey:@"Audio2Mixdown"] ,
                            [item objectForKey:@"Audio2Samplerate"],
                            [item objectForKey:@"Audio2Bitrate"]];
            
            if ([[item objectForKey:@"Audio2TrackDRCSlider"] floatValue] > 1.00)
            {
                audioDetail2 = [NSString stringWithFormat:@"%@, DRC: %@",audioDetail2,[item objectForKey:@"Audio2TrackDRCSlider"]];
            }
            else
            {
                audioDetail2 = [NSString stringWithFormat:@"%@, DRC: Off",audioDetail2];
            }
        }
        
        if ([[item objectForKey:@"Audio3Track"] intValue] > 0)
        {
            audioCodecSummary = [NSString stringWithFormat:@"%@, %@",audioCodecSummary ,[item objectForKey:@"Audio3Encoder"]];
            audioDetail3 = [NSString stringWithFormat:@"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps)",
                            [item objectForKey:@"Audio3TrackDescription"] ,
                            [item objectForKey:@"Audio3Encoder"],
                            [item objectForKey:@"Audio3Mixdown"] ,
                            [item objectForKey:@"Audio3Samplerate"],
                            [item objectForKey:@"Audio3Bitrate"]];
            
            if ([[item objectForKey:@"Audio3TrackDRCSlider"] floatValue] > 1.00)
            {
                audioDetail3 = [NSString stringWithFormat:@"%@, DRC: %@",audioDetail3,[item objectForKey:@"Audio3TrackDRCSlider"]];
            }
            else
            {
                audioDetail3 = [NSString stringWithFormat:@"%@, DRC: Off",audioDetail3];
            }
        }
        
        if ([[item objectForKey:@"Audio4Track"] intValue] > 0)
        {
            audioCodecSummary = [NSString stringWithFormat:@"%@, %@",audioCodecSummary ,[item objectForKey:@"Audio3Encoder"]];
            audioDetail4 = [NSString stringWithFormat:@"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps)",
                            [item objectForKey:@"Audio4TrackDescription"] ,
                            [item objectForKey:@"Audio4Encoder"],
                            [item objectForKey:@"Audio4Mixdown"] ,
                            [item objectForKey:@"Audio4Samplerate"],
                            [item objectForKey:@"Audio4Bitrate"]];
            
            if ([[item objectForKey:@"Audio4TrackDRCSlider"] floatValue] > 1.00)
            {
                audioDetail4 = [NSString stringWithFormat:@"%@, DRC: %@",audioDetail4,[item objectForKey:@"Audio4TrackDRCSlider"]];
            }
            else
            {
                audioDetail4 = [NSString stringWithFormat:@"%@, DRC: Off",audioDetail4];
            }
        }
        
        NSString * jobFormatInfo;
        if ([[item objectForKey:@"ChapterMarkers"] intValue] == 1)
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio, Chapter Markers\n", [item objectForKey:@"FileFormat"], [item objectForKey:@"VideoEncoder"], audioCodecSummary];
        else
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio\n", [item objectForKey:@"FileFormat"], [item objectForKey:@"VideoEncoder"], audioCodecSummary];
        
        
        [finalString appendString: @"Format: " withAttributes:detailBoldAttr];
        [finalString appendString: jobFormatInfo withAttributes:detailAttr];
        
        /* Optional String for mp4 options */
        if ([[item objectForKey:@"FileFormat"] isEqualToString: @"MP4 file"])
        {
            NSString * MP4Opts = @"";
            BOOL mp4OptsPresent = NO;
            if( [[item objectForKey:@"Mp4LargeFile"] intValue] == 1)
            {
                mp4OptsPresent = YES;
                MP4Opts = [MP4Opts stringByAppendingString:@" - Large file size"];
            }
            if( [[item objectForKey:@"Mp4HttpOptimize"] intValue] == 1)
            {
                mp4OptsPresent = YES;
                MP4Opts = [MP4Opts stringByAppendingString:@" - Web optimized"];
            }
            
            if( [[item objectForKey:@"Mp4iPodCompatible"] intValue] == 1)
            {
                mp4OptsPresent = YES;
                MP4Opts = [MP4Opts stringByAppendingString:@" - iPod 5G support "];
            }
            if (mp4OptsPresent == YES)
            {
                [finalString appendString: @"MP4 Options: " withAttributes:detailBoldAttr];
                [finalString appendString: MP4Opts withAttributes:detailAttr];
                [finalString appendString:@"\n" withAttributes:detailAttr];
            }
        }
        
        /* Fourth Line (Destination Path)*/
        [finalString appendString: @"Destination: " withAttributes:detailBoldAttr];
        [finalString appendString: [item objectForKey:@"DestinationPath"] withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        /* Fifth Line Picture Details*/
        NSString * pictureInfo;
        pictureInfo = [NSString stringWithFormat:@"%@", [item objectForKey:@"PictureSizingSummary"]];
        if ([[item objectForKey:@"PictureKeepRatio"] intValue] == 1)
        {
            pictureInfo = [pictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
        }
        if ([[item objectForKey:@"VideoGrayScale"] intValue] == 1)
        {
            pictureInfo = [pictureInfo stringByAppendingString:@", Grayscale"];
        }
        
        [finalString appendString: @"Picture: " withAttributes:detailBoldAttr];
        [finalString appendString: pictureInfo withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        /* Optional String for Picture Filters */
        
        NSString * pictureFilters = @"";
        BOOL pictureFiltersPresent = NO;
        
        if( [[item objectForKey:@"PictureDetelecine"] intValue] == 1)
        {
            pictureFiltersPresent = YES;
            pictureFilters = [pictureFilters stringByAppendingString:@" - Detelecine (Default)"];
        }
        else if( [[item objectForKey:@"PictureDetelecine"] intValue] == 2)
        {
            pictureFiltersPresent = YES;
            pictureFilters = [pictureFilters stringByAppendingString:[NSString stringWithFormat:@" - Detelecine (%@)",[item objectForKey:@"PictureDetelecineCustom"]]];
        }
        
        if( [[item objectForKey:@"PictureDecombDeinterlace"] intValue] == 1)
        {
            if ([[item objectForKey:@"PictureDecomb"] intValue] != 0)
            {
                pictureFiltersPresent = YES;
                if( [[item objectForKey:@"PictureDecomb"] intValue] == 1)
                {
                    pictureFiltersPresent = YES;
                    pictureFilters = [pictureFilters stringByAppendingString:@" - Decomb (Default)"];
                }
                if( [[item objectForKey:@"PictureDecomb"] intValue] == 2)
                {
                    pictureFiltersPresent = YES;
                    pictureFilters = [pictureFilters stringByAppendingString:[NSString stringWithFormat:@" - Decomb (%@)",[item objectForKey:@"PictureDecombCustom"]]];
                }
            }
        }
        else
        {
            if ([[item objectForKey:@"PictureDeinterlace"] intValue] != 0)
            {
                pictureFiltersPresent = YES;
                if ([[item objectForKey:@"PictureDeinterlace"] intValue] == 1)
                {
                    pictureFilters = [pictureFilters stringByAppendingString:@" - Deinterlace (Fast)"];
                }
                else if ([[item objectForKey:@"PictureDeinterlace"] intValue] == 2)
                {
                    pictureFilters = [pictureFilters stringByAppendingString:@" - Deinterlace (Slow)"];           
                }
                else if ([[item objectForKey:@"PictureDeinterlace"] intValue] == 3)
                {
                    pictureFilters = [pictureFilters stringByAppendingString:@" - Deinterlace (Slower)"];            
                }
                else if ([[item objectForKey:@"PictureDeinterlace"] intValue] == 4)
                {
                    pictureFilters = [pictureFilters stringByAppendingString:[NSString stringWithFormat:@" - Deinterlace (%@)",[item objectForKey:@"PictureDeinterlaceCustom"]]];            
                }
                
            }
        }
        if ([[item objectForKey:@"PictureDenoise"] intValue] != 0)
        {
            pictureFiltersPresent = YES;
            if ([[item objectForKey:@"PictureDenoise"] intValue] == 1)
            {
                pictureFilters = [pictureFilters stringByAppendingString:@" - Denoise (Weak)"];
            }
            else if ([[item objectForKey:@"PictureDenoise"] intValue] == 2)
            {
                pictureFilters = [pictureFilters stringByAppendingString:@" - Denoise (Medium)"];           
            }
            else if ([[item objectForKey:@"PictureDenoise"] intValue] == 3)
            {
                pictureFilters = [pictureFilters stringByAppendingString:@" - Denoise (Strong)"];            
            }
            else if ([[item objectForKey:@"PictureDenoise"] intValue] == 4)
            {
                pictureFilters = [pictureFilters stringByAppendingString:[NSString stringWithFormat:@" - Denoise (%@)",[item objectForKey:@"PictureDenoiseCustom"]]];            
            }
            
        }
        if ([[item objectForKey:@"PictureDeblock"] intValue] != 0)
        {
            pictureFiltersPresent = YES;
            pictureFilters = [pictureFilters stringByAppendingString: [NSString stringWithFormat:@" - Deblock (pp7) (%d)",[[item objectForKey:@"PictureDeblock"] intValue]]];
        }
        
        if ([[item objectForKey:@"VideoGrayScale"] intValue] == 1)
        {
            pictureFiltersPresent = YES;
            pictureFilters = [pictureFilters stringByAppendingString:@" - Grayscale"];
        }
        
        if (pictureFiltersPresent == YES)
        {
            [finalString appendString: @"Filters: " withAttributes:detailBoldAttr];
            [finalString appendString: pictureFilters withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        
        /* Sixth Line Video Details*/
        NSString * videoInfo;
        videoInfo = [NSString stringWithFormat:@"Encoder: %@", [item objectForKey:@"VideoEncoder"]];
        
        /* for framerate look to see if we are using vfr detelecine */
        if ([[item objectForKey:@"JobIndexVideoFramerate"] intValue] == 0)
        {
            if ([[item objectForKey:@"PictureDetelecine"] intValue] == 1)
            {
                /* we are using same as source with vfr detelecine */
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (vfr detelecine)", videoInfo];
            }
            else
            {
                /* we are using a variable framerate without dropping frames */
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (variable)", videoInfo];
            }
        }
        else
        {
            /* we have a specified, constant framerate */
            videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (constant framerate)", videoInfo ,[item objectForKey:@"VideoFramerate"]];
        }
        
        if ([[item objectForKey:@"VideoQualityType"] intValue] == 0)// Target Size MB
        {
            videoInfo = [NSString stringWithFormat:@"%@ Target Size: %@(MB) (%d(kbps) abr)", videoInfo ,[item objectForKey:@"VideoTargetSize"],[[item objectForKey:@"VideoAvgBitrate"] intValue]];
        }
        else if ([[item objectForKey:@"VideoQualityType"] intValue] == 1) // ABR
        {
            videoInfo = [NSString stringWithFormat:@"%@ Bitrate: %d(kbps)", videoInfo ,[[item objectForKey:@"VideoAvgBitrate"] intValue]];
        }
        else // CRF
        {
            videoInfo = [NSString stringWithFormat:@"%@ Constant Quality: %.2f", videoInfo ,[[item objectForKey:@"VideoQualitySlider"] floatValue]];
        }
        
        [finalString appendString: @"Video: " withAttributes:detailBoldAttr];
        [finalString appendString: videoInfo withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        if ([[item objectForKey:@"VideoEncoder"] isEqualToString: @"H.264 (x264)"])
        {
            [finalString appendString: @"x264 Options: " withAttributes:detailBoldAttr];
            [finalString appendString: [item objectForKey:@"x264Option"] withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        
        
        /* Seventh Line Audio Details*/
        [finalString appendString: @"Audio Track 1: " withAttributes:detailBoldAttr];
        [finalString appendString: audioDetail1 withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        [finalString appendString: @"Audio Track 2: " withAttributes:detailBoldAttr];
        [finalString appendString: audioDetail2 withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        [finalString appendString: @"Audio Track 3: " withAttributes:detailBoldAttr];
        [finalString appendString: audioDetail3 withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];
        
        [finalString appendString: @"Audio Track 4: " withAttributes:detailBoldAttr];
        [finalString appendString: audioDetail4 withAttributes:detailAttr];
        
        return finalString;
    }
    else if ([[tableColumn identifier] isEqualToString:@"icon"])
    {
        if ([[item objectForKey:@"Status"] intValue] == 0)
        {
            return [NSImage imageNamed:@"EncodeComplete"];
        }
        else if ([[item objectForKey:@"Status"] intValue] == 1)
        {
            return [NSImage imageNamed: [NSString stringWithFormat: @"EncodeWorking%d", fAnimationIndex]];
        }
        else if ([[item objectForKey:@"Status"] intValue] == 3)
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
        if ([[item objectForKey:@"Status"] intValue] == 0)
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
    // By default, the discolsure image gets centered vertically in the cell. We want
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
    if ([[[items objectAtIndex:0] objectForKey:@"Status"] integerValue] != 2) // 2 is pending
    {
        return NO;
    }
    
    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    fDraggedNodes = items;
    
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:[NSArray arrayWithObjects: DragDropSimplePboardType, nil] owner:self];
    
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
