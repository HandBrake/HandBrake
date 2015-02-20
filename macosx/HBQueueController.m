
/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import "HBQueueController.h"

#import "HBCore.h"
#import "HBController.h"
#import "HBAppDelegate.h"

#import "HBQueueOutlineView.h"
#import "HBUtilities.h"

#import "HBJob.h"
#import "HBJob+UIAdditions.h"

#import "HBDistributedArray.h"

#import "HBDockTile.h"

#import "HBOutputRedirect.h"
#import "HBJobOutputFileWriter.h"

// Pasteboard type for or drag operations
#define DragDropSimplePboardType    @"HBQueueCustomOutlineViewPboardType"

// DockTile update freqency in total percent increment
#define dockTileUpdateFrequency     0.1f

#define HB_ROW_HEIGHT_TITLE_ONLY    17.0

@interface HBQueueController () <HBQueueOutlineViewDelegate>

@property (nonatomic, readonly) HBDockTile *dockTile;
@property (nonatomic, readwrite) double dockIconProgress;

@property (assign) IBOutlet NSTextField *progressTextField;
@property (assign) IBOutlet NSTextField *countTextField;
@property (assign) IBOutlet HBQueueOutlineView *outlineView;

@property (nonatomic, readonly) NSMutableDictionary *descriptions;

@property (nonatomic, readonly) HBDistributedArray *jobs;
@property (nonatomic, retain)   HBJob *currentJob;
@property (nonatomic, retain)   HBJobOutputFileWriter *currentLog;

@property (nonatomic, readwrite) BOOL stop;

@property (nonatomic, readwrite) NSUInteger pendingItemsCount;
@property (nonatomic, readwrite) NSUInteger workingItemsCount;

@property (nonatomic, retain) NSArray *dragNodesArray;

@end

@implementation HBQueueController

- (instancetype)init
{
    if (self = [super initWithWindowNibName:@"Queue"])
    {
        _descriptions = [[NSMutableDictionary alloc] init];

        // Workaround to avoid a bug in Snow Leopard
        // we can switch back to [[NSApplication sharedApplication] applicationIconImage]
        // when we won't support it anymore.
        NSImage *appIcon = [NSImage imageNamed:@"HandBrake"];
        [appIcon setSize:NSMakeSize(1024, 1024)];

        // Load the dockTile and instiante initial text fields
        _dockTile = [[HBDockTile alloc] initWithDockTile:[[NSApplication sharedApplication] dockTile]
                                                  image:appIcon];

        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];

        // Init a separate instance of libhb for the queue
        _core = [[HBCore alloc] initWithLoggingLevel:loggingLevel];
        _core.name = @"QueueCore";

        [self loadQueueFile];
    }

    return self;
}

- (void)dealloc
{
    // clear the delegate so that windowWillClose is not attempted
    if ([[self window] delegate] == self)
        [[self window] setDelegate:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [_core release];
    [_jobs release];
    [_currentJob release];

    [_dockTile release];
    [_descriptions release];
    [_dragNodesArray release];

    [super dealloc];
}

- (void)windowDidLoad
{
    // lets setup our queue list outline view for drag and drop here
    [self.outlineView registerForDraggedTypes:@[DragDropSimplePboardType]];
    [self.outlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [self.outlineView setVerticalMotionCanBeginDrag:YES];

    // Don't allow autoresizing of main column, else the "delete" column will get
    // pushed out of view.
    [self.outlineView setAutoresizesOutlineColumn: NO];
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
}

/**
 * Displays and brings the queue window to the front
 */
- (IBAction)showWindow:(id)sender
{
    [super showWindow:sender];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"QueueWindowIsOpen"];
}

#pragma mark Toolbar

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(rip:))
    {
        if (self.core.state == HBStateIdle)
        {
            menuItem.title = NSLocalizedString(@"Start Encoding", nil);
            menuItem.keyEquivalent = @"s";

            return (self.pendingItemsCount > 0);
        }
        else if (self.core.state != HBStateIdle)
        {
            menuItem.title = NSLocalizedString(@"Stop Encoding", nil);
            menuItem.keyEquivalent = @".";

            return self.core.state != HBStateScanning;
        }
    }

    if (action == @selector(pause:))
    {
        if (self.core.state != HBStatePaused)
        {
            menuItem.title = NSLocalizedString(@"Pause Encoding", nil);
        }
        else
        {
            menuItem.title = NSLocalizedString(@"Resume Encoding", nil);
        }

        return (self.core.state == HBStateWorking || self.core.state == HBStatePaused);
    }

    if (action == @selector(editSelectedQueueItem:) ||
        action == @selector(removeSelectedQueueItem:) ||
        action == @selector(revealSelectedQueueItem:))
    {
        return (self.outlineView.selectedRow != -1 || self.outlineView.clickedRow != -1);
    }

    return YES;
}

- (BOOL)validateToolbarItem:(NSToolbarItem *)theItem
{
    SEL action = theItem.action;
    HBState s = self.core.state;

    if (action == @selector(toggleStartCancel:))
    {
        if ((s == HBStateScanning) || (s == HBStatePaused) || (s == HBStateWorking) || (s == HBStateMuxing))
        {
            theItem.image = [NSImage imageNamed:@"stopencode"];
            theItem.label = NSLocalizedString(@"Stop", @"");
            theItem.toolTip = NSLocalizedString(@"Stop Encoding", @"");
            return s != HBStateScanning;
        }
        else
        {
            theItem.image = [NSImage imageNamed:@"encode"];
            theItem.label = NSLocalizedString(@"Start", @"");
            theItem.toolTip = NSLocalizedString(@"Start Encoding", @"");
            return (self.pendingItemsCount > 0);
        }
    }

    if (action == @selector(togglePauseResume:))
    {
        if (s == HBStatePaused)
        {
            theItem.image = [NSImage imageNamed:@"encode"];
            theItem.label = NSLocalizedString(@"Resume", @"");
            theItem.toolTip = NSLocalizedString(@"Resume Encoding", @"");
            return YES;
        }
        else
        {
            theItem.image = [NSImage imageNamed:@"pauseencode"];
            theItem.label = NSLocalizedString(@"Pause", @"");
            theItem.toolTip = NSLocalizedString(@"Pause Encoding", @"");
            return (s == HBStateWorking || s == HBStateMuxing);
        }
    }

    return NO;
}

#pragma mark -
#pragma mark Queue File

- (void)reloadQueue
{
    [self getQueueStats];
    [self.outlineView reloadData];
}

- (void)loadQueueFile
{
    NSURL *queueURL = [NSURL fileURLWithPath:[[HBUtilities appSupportPath] stringByAppendingPathComponent:@"Queue/Queue.hbqueue"]];
    _jobs = [[HBDistributedArray alloc] initWithURL:queueURL];

    [self reloadQueue];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(reloadQueue) name:HBDistributedArrayChanged object:_jobs];
}

- (void)addJob:(HBJob *)item
{
    [self.jobs beginTransaction];
    [self.jobs addObject:item];
    [self.jobs commit];

    [self reloadQueue];
}

- (void)addJobsFromArray:(NSArray *)items;
{
    [self.jobs beginTransaction];
    [self.jobs addObjectsFromArray:items];
    [self.jobs commit];

    [self reloadQueue];
}

- (BOOL)jobExistAtURL:(NSURL *)url
{
    for (HBJob *item in self.jobs)
    {
        if ([item.destURL isEqualTo:url])
        {
            return YES;
        }
    }
    return NO;
}

- (void)removeQueueItemAtIndex:(NSUInteger)index
{
    [self.jobs beginTransaction];
    if (self.jobs.count > index)
    {
        [self.jobs removeObjectAtIndex:index];
    }
    [self.jobs commit];

    [self reloadQueue];
}

/**
 *  Updates the queue status label.
 */
- (void)getQueueStats
{
    // lets get the stats on the status of the queue array
    NSUInteger pendingCount = 0;
    NSUInteger workingCount = 0;

    for (HBJob *job in self.jobs)
    {
        if (job.state == HBJobStateWorking) // being encoded
        {
            workingCount++;
        }
        if (job.state == HBJobStateReady) // pending
        {
            pendingCount++;
        }
    }

    NSString *string;
    if (pendingCount == 0)
    {
        string = NSLocalizedString(@"No encode pending", @"");
    }
    else if (pendingCount == 1)
    {
        string = [NSString stringWithFormat: NSLocalizedString(@"%d encode pending", @""), pendingCount];
    }
    else
    {
        string = [NSString stringWithFormat: NSLocalizedString(@"%d encodes pending", @""), pendingCount];
    }

    self.countTextField.stringValue = string;
    [self.controller setQueueState:string];

    self.pendingItemsCount = pendingCount;
    self.workingItemsCount = workingCount;
}

- (NSUInteger)count
{
    return self.jobs.count;
}

/**
 * Used to get the next pending queue item and return it if found
 */
- (HBJob *)getNextPendingQueueItem
{
    for (HBJob *job in self.jobs)
    {
        if (job.state == HBJobStateReady)
        {
            return job;
        }
    }
    return nil;
}

/**
 * This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void)setEncodingJobsAsPending
{
    [self.jobs beginTransaction];
    for (HBJob *job in self.jobs)
    {
        // We want to keep any queue item that is pending or was previously being encoded
        if (job.state == HBJobStateWorking)
        {
            job.state = HBJobStateReady;
        }
    }
    [self.jobs commit];

    [self reloadQueue];
}

/**
 * This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as cancelled encodes
 */
- (void)clearEncodedJobs
{
    [self.jobs beginTransaction];
    [self removeItemsUsingBlock:^BOOL(HBJob *item) {
        return (item.state == HBJobStateCompleted || item.state == HBJobStateCanceled);
    }];
    [self.jobs commit];
    [self reloadQueue];
}

/**
 * This method will clear the queue of all encodes. effectively creating an empty queue
 */
- (void)removeAllJobs
{
    [self.jobs beginTransaction];
    [self.jobs removeAllObjects];
    [self.jobs commit];

    [self reloadQueue];
}

- (void)removeItemsUsingBlock:(BOOL (^)(HBJob *item))predicate
{
    NSMutableArray *itemsToRemove = [NSMutableArray array];
    for (HBJob *item in self.jobs)
    {
        if (predicate(item))
        {
            [itemsToRemove addObject:item];
        }
    }
    [self.jobs removeObjectsInArray:itemsToRemove];
}

/**
 * this is actually called from the queue controller to modify the queue array and return it back to the queue controller
 */
- (void)moveObjectsInQueueArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    [self.jobs beginTransaction];

    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;

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

    id object = [self.jobs[removeIndex] retain];
    [self.jobs removeObjectAtIndex:removeIndex];
    [self.jobs insertObject:object atIndex:insertIndex];
    [object release];

    // We save all of the Queue data here
    // and it also gets sent back to the queue controller
    [self.jobs commit];
    [self reloadQueue];
}

#pragma mark -
#pragma mark Queue Job Processing

/**
 *  Starts the queue
 */
- (void)encodeNextQueueItem
{
    // Check to see if there are any more pending items in the queue
    HBJob *nextJob = [self getNextPendingQueueItem];

    // If we still have more pending items in our queue, lets go to the next one
    if (nextJob)
    {
        self.currentJob = nextJob;
        // now we mark the queue item as working so another instance can not come along and try to scan it while we are scanning
        self.currentJob.state = HBJobStateWorking;

        // Tell HB to output a new activity log file for this encode
        self.currentLog = [[[HBJobOutputFileWriter alloc] initWithJob:self.currentJob] autorelease];
        [[HBOutputRedirect stderrRedirect] addListener:self.currentLog];
        [[HBOutputRedirect stdoutRedirect] addListener:self.currentLog];

        // now we can go ahead and scan the new pending queue item
        [self performScan:self.currentJob.fileURL titleIdx:self.currentJob.titleIdx];
    }
    else
    {
        self.currentJob = nil;

        [HBUtilities writeToActivityLog:"Queue Done, there are no more pending encodes"];

        // Since there are no more items to encode, go to queueCompletedAlerts
        // for user specified alerts after queue completed
        [self queueCompletedAlerts];
    }
}

- (void)encodeCompleted
{
    // Since we are done with this encode, tell output to stop writing to the
    // individual encode log.
    [[HBOutputRedirect stderrRedirect] removeListener:self.currentLog];
    [[HBOutputRedirect stdoutRedirect] removeListener:self.currentLog];
    self.currentLog = nil;

    // Check to see if the encode state has not been cancelled
    // to determine if we should check for encode done notifications.
    if (self.currentJob.state != HBJobStateCanceled)
    {
        // Both the Growl Alert and Sending to tagger can be done as encodes roll off the queue
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Growl Notification"] ||
            [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
        {
            // If Play System Alert has been selected in Preferences
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"AlertWhenDoneSound"] == YES)
            {
                NSBeep();
            }
            [self showGrowlDoneNotification:self.currentJob.destURL];
        }

        // Mark the encode just finished as done
        self.currentJob.state = HBJobStateCompleted;

        // Send to tagger
        [self sendToExternalApp:self.currentJob.destURL];
    }

    self.currentJob = nil;

    // since we have successfully completed an encode, we go to the next
    if (!self.stop)
    {
        [self encodeNextQueueItem];
    }
    self.stop = NO;

    [self.window.toolbar validateVisibleItems];
    [self reloadQueue];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)performScan:(NSURL *)scanURL titleIdx:(NSInteger)index
{
    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    [self.core scanURL:scanURL
            titleIndex:index
              previews:10
           minDuration:0
       progressHandler:^(HBState state, hb_state_t hb_state) {
           NSMutableString *status = [NSMutableString stringWithFormat:
                                      NSLocalizedString( @"Queue Scanning title %d of %d…", @"" ),
                                      hb_state.param.scanning.title_cur, hb_state.param.scanning.title_count];
           if (hb_state.param.scanning.preview_cur)
           {
               [status appendFormat:@", preview %d…", hb_state.param.scanning.preview_cur];
           }

           self.progressTextField.stringValue = status;
           [self.controller setQueueInfo:status progress:0 hidden:NO];
       }
   completionHandler:^(BOOL success) {
       if (success)
       {
           [self doEncodeQueueItem];
       }
       else
       {
           [self.jobs beginTransaction];

           self.currentJob.state = HBJobStateCanceled;
           [self encodeCompleted];

           [self.jobs commit];
           [self reloadQueue];
       }

       [self.window.toolbar validateVisibleItems];
   }];
}

/**
 * This assumes that we have re-scanned and loaded up a new queue item to send to libhb
 */
- (void)doEncodeQueueItem
{
    // Reset the title in the job.
    self.currentJob.title = self.core.titles[0];

    // We should be all setup so let 'er rip
    [self.core encodeJob:self.currentJob
         progressHandler:^(HBState state, hb_state_t hb_state) {
             NSMutableString *string = nil;
             CGFloat progress = 0;
             #define p hb_state.param.working
             switch (state)
             {
                 case HBStateSearching:
                 {
                     string = [NSMutableString stringWithFormat:
                               NSLocalizedString(@"Searching for start point… :  %.2f %%", @""),
                               100.0 * p.progress];

                     if (p.seconds > -1)
                     {
                         [string appendFormat:NSLocalizedString(@" (ETA %02dh%02dm%02ds)", @"" ), p.hours, p.minutes, p.seconds];
                     }

                     break;
                 }
                 case HBStateWorking:
                 {
                     NSString *pass_desc = @"";
                     if (p.job_cur == 1 && p.job_count > 1)
                     {
                         if ([self.currentJob.subtitles.tracks.firstObject[keySubTrackIndex] intValue] == -1)
                         {
                             pass_desc = NSLocalizedString(@"(subtitle scan)", @"");
                         }
                     }

                     if (pass_desc.length)
                     {
                         string = [NSMutableString stringWithFormat:
                                   NSLocalizedString(@"Encoding: %@ \nPass %d %@ of %d, %.2f %%", @""),
                                   self.currentJob.destURL.lastPathComponent,
                                   p.job_cur, pass_desc, p.job_count, 100.0 * p.progress];
                     }
                     else
                     {
                         string = [NSMutableString stringWithFormat:
                                   NSLocalizedString(@"Encoding: %@ \nPass %d of %d, %.2f %%", @""),
                                   self.currentJob.destURL.lastPathComponent,
                                   p.job_cur, p.job_count, 100.0 * p.progress];
                     }

                     if (p.seconds > -1)
                     {
                         if (p.rate_cur > 0.0)
                         {
                             [string appendFormat:
                              NSLocalizedString(@" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", @""),
                              p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
                         }
                         else
                         {
                             [string appendFormat:
                              NSLocalizedString(@" (ETA %02dh%02dm%02ds)", @""),
                              p.hours, p.minutes, p.seconds];
                         }
                     }

                     progress = (p.progress + p.job_cur - 1) / p.job_count;

                     // Update dock icon
                     if (self.dockIconProgress < 100.0 * progress)
                     {
                         // ETA format is [XX]X:XX:XX when ETA is greater than one hour
                         // [X]X:XX when ETA is greater than 0 (minutes or seconds)
                         // When these conditions doesn't applied (eg. when ETA is undefined)
                         // we show just a tilde (~)

                         NSString *etaStr = @"";
                         if (p.hours > 0)
                             etaStr = [NSString stringWithFormat:@"%d:%02d:%02d", p.hours, p.minutes, p.seconds];
                         else if (p.minutes > 0 || p.seconds > 0)
                             etaStr = [NSString stringWithFormat:@"%d:%02d", p.minutes, p.seconds];
                         else
                             etaStr = @"~";

                         [self.dockTile updateDockIcon:progress withETA:etaStr];

                         self.dockIconProgress += dockTileUpdateFrequency;
                     }

                     break;
                 }
                 case HBStateMuxing:
                 {
                     string = [NSMutableString stringWithString:NSLocalizedString(@"Muxing…", @"")];

                     // Update dock icon
                     [self.dockTile updateDockIcon:1.0 withETA:@""];

                     break;
                 }
                 case HBStatePaused:
                 {
                     string = [NSMutableString stringWithString:NSLocalizedString(@"Paused", @"")];
                     break;
                 }
                 default:
                     break;
             }
             #undef p

             // Update text field
             self.progressTextField.stringValue = string;
             [self.controller setQueueInfo:string progress:progress * 100.0 hidden:NO];
         }
     completionHandler:^(BOOL success) {
         NSString *info = NSLocalizedString(@"Encode Finished.", @"");

         self.progressTextField.stringValue = info;
         [self.controller setQueueInfo:info progress:100.0 hidden:YES];

         // Restore dock icon
         [self.dockTile updateDockIcon:-1.0 withETA:@""];
         self.dockIconProgress = 0;

         [self.jobs beginTransaction];

         [self encodeCompleted];

         [self.jobs commit];
         [self reloadQueue];
     }];

    // We are done using the title, remove it from the job
    self.currentJob.title = nil;
}

/**
 * Cancels the current job
 */
- (void)doCancelCurrentJob
{
    self.currentJob.state = HBJobStateCanceled;

    if (self.core.state == HBStateScanning)
    {
        [self.core cancelScan];
    }
    else
    {
        [self.core cancelEncode];
    }
}

/**
 * Cancels the current job and starts processing the next in queue.
 */
- (void)cancelCurrentJobAndContinue
{
    [self.jobs beginTransaction];

    [self doCancelCurrentJob];

    [self.jobs commit];
    [self reloadQueue];
}

/**
 * Cancels the current job and stops libhb from processing the remaining encodes.
 */
- (void)cancelCurrentJobAndStop
{
    [self.jobs beginTransaction];

    self.stop = YES;
    [self doCancelCurrentJob];

    [self.jobs commit];
    [self reloadQueue];
}

#pragma mark - Encode Done Actions

#define SERVICE_NAME @"Encode Done"

/**
 *  Register a test notification and make
 *  it enabled by default
 */
- (NSDictionary *)registrationDictionaryForGrowl
{
    return @{GROWL_NOTIFICATIONS_ALL: @[SERVICE_NAME],
             GROWL_NOTIFICATIONS_DEFAULT: @[SERVICE_NAME]};
}

- (void)showGrowlDoneNotification:(NSURL *)fileURL
{
    // This end of encode action is called as each encode rolls off of the queue
    // Setup the Growl stuff
    NSString *growlMssg = [NSString stringWithFormat:@"your HandBrake encode %@ is done!", fileURL.lastPathComponent];
    [GrowlApplicationBridge notifyWithTitle:@"Put down that cocktail…"
                                description:growlMssg
                           notificationName:SERVICE_NAME
                                   iconData:nil
                                   priority:0
                                   isSticky:1
                               clickContext:nil];
}

- (void)sendToExternalApp:(NSURL *)fileURL
{
    // This end of encode action is called as each encode rolls off of the queue
    if([[NSUserDefaults standardUserDefaults] boolForKey: @"sendToMetaX"] == YES)
    {
        NSString *sendToApp = [[NSUserDefaults standardUserDefaults] objectForKey:@"SendCompletedEncodeToApp"];
        if (![sendToApp isEqualToString:@"None"])
        {
            [HBUtilities writeToActivityLog: "trying to send encode to: %s", [sendToApp UTF8String]];
            NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@%@%@", @"tell application \"",sendToApp,@"\" to open (POSIX file \"", fileURL.path, @"\")"]];
            [myScript executeAndReturnError: nil];
            [myScript release];
        }
    }
}

- (void)queueCompletedAlerts
{
    // If Play System Alert has been selected in Preferences
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"AlertWhenDoneSound"] == YES)
    {
        NSBeep();
    }

    // If Alert Window or Window and Growl has been selected
    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window"] ||
        [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
    {
        // On Screen Notification
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Put down that cocktail…", @"")];
        [alert setInformativeText:NSLocalizedString(@"Your HandBrake queue is done!", @"")];
        [NSApp requestUserAttention:NSCriticalRequest];
        [alert runModal];
        [alert release];
    }

    // If sleep has been selected
    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"])
    {
        // Sleep
        NSDictionary *errorDict;
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to sleep"];
        [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
    // If Shutdown has been selected
    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"] )
    {
        // Shut Down
        NSDictionary *errorDict;
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:@"tell application \"Finder\" to shut down"];
        [scriptObject executeAndReturnError: &errorDict];
        [scriptObject release];
    }
}

#pragma mark - Queue Item Controls

- (void)HB_deleteSelectionFromTableView:(NSTableView *)tableView
{
    [self removeSelectedQueueItem:tableView];
}

/**
 * Delete encodes from the queue window and accompanying array
 * Also handling first cancelling the encode if in fact its currently encoding.
 */
- (IBAction)removeSelectedQueueItem:(id)sender
{
    NSIndexSet *targetedRow = [self.outlineView targetedRowIndexes];
    NSUInteger row = [targetedRow firstIndex];
    if (row == NSNotFound)
    {
        return;
    }

    // if this is a currently encoding job, we need to be sure to alert the user,
    // to let them decide to cancel it first, then if they do, we can come back and
    // remove it
    if (self.jobs[row] == self.currentJob)
    {
        // We pause the encode here so that it doesn't finish right after and then
        // screw up the sync while the window is open
        [self togglePauseResume:nil];

        NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Remove It?", nil)];

        // Which window to attach the sheet to?
        NSWindow *targetWindow = nil;
        if ([sender respondsToSelector: @selector(window)])
        {
            targetWindow = [sender window];
        }

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:alertTitle];
        [alert setInformativeText:NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Keep Encoding", nil)];
        [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Delete", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:targetWindow
                          modalDelegate:self
                         didEndSelector:@selector(didDimissCancelCurrentJob:returnCode:contextInfo:)
                            contextInfo:self.jobs[row]];
        [alert release];
    }
    else if ([self.jobs[row] state] != HBJobStateWorking)
    {
        // since we are not a currently encoding item, we can just be removed
        [self removeQueueItemAtIndex:row];
    }
}

- (void)didDimissCancelCurrentJob:(NSAlert *)alert
                       returnCode:(NSInteger)returnCode
                      contextInfo:(void *)contextInfo
{
    // We resume encoding and perform the appropriate actions
    // Note: Pause: is a toggle type method based on hb's current
    // state, if it paused, it will resume encoding and vice versa.
    // In this case, we are paused from the calling window, so calling
    // [self togglePauseResume:nil]; Again will resume encoding
    [self togglePauseResume:nil];

    if (returnCode == NSAlertSecondButtonReturn)
    {
        NSInteger index = [self.jobs indexOfObject:self.currentJob];
        [self cancelCurrentJobAndContinue];
        [self removeQueueItemAtIndex:index];
    }
}

/**
 * Show the finished encode in the finder
 */
- (IBAction)revealSelectedQueueItem: (id)sender
{
    NSIndexSet *targetedRow = [self.outlineView targetedRowIndexes];
    NSInteger row = [targetedRow firstIndex];
    if (row != NSNotFound)
    {
        while (row != NSNotFound)
        {
            HBJob *queueItemToOpen = [self.outlineView itemAtRow:row];
            [[NSWorkspace sharedWorkspace] selectFile:queueItemToOpen.destURL.path inFileViewerRootedAtPath:nil];

            row = [targetedRow indexGreaterThanIndex: row];
        }
    }
}

- (void)remindUserOfSleepOrShutdown
{
    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString:@"Put Computer To Sleep"])
    {
        // Warn that computer will sleep after encoding
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The computer will sleep after encoding is done.", @"")];
        [alert setInformativeText:NSLocalizedString(@"You have selected to sleep the computer after encoding. To turn off sleeping, go to the HandBrake preferences.", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"OK", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Preferences…",@"")];

        NSInteger response = [alert runModal];
        if (response == NSAlertSecondButtonReturn)
        {
            [self.delegate showPreferencesWindow:nil];
        }
        [alert release];
    }
    else if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString:@"Shut Down Computer"])
    {
        // Warn that computer will shut down after encoding
        NSBeep();
        [NSApp requestUserAttention:NSCriticalRequest];

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The computer will shut down after encoding is done.", @"")];
        [alert setInformativeText:NSLocalizedString(@"You have selected to shut down the computer after encoding. To turn off shut down, go to the HandBrake preferences.", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"OK", @"")];
        [alert addButtonWithTitle:NSLocalizedString(@"Preferences…", @"")];

        NSInteger response = [alert runModal];
        if (response == NSAlertSecondButtonReturn)
        {
            [self.delegate showPreferencesWindow:nil];
        }
        [alert release];
    }
}

/**
 * Rip: puts up an alert before ultimately calling doRip
 */
- (IBAction)rip:(id)sender
{
    // Rip or Cancel ?
    if (self.core.state == HBStateWorking || self.core.state == HBStatePaused)
    {
        [self cancelRip:sender];
    }
    // If there are pending jobs in the queue, then this is a rip the queue
    else if (self.pendingItemsCount > 0)
    {
        // We check to see if we need to warn the user that the computer will go to sleep
        // or shut down when encoding is finished
        [self remindUserOfSleepOrShutdown];

        [self.jobs beginTransaction];

        [self encodeNextQueueItem];

        [self.jobs commit];
        [self reloadQueue];
    }
}

/**
 * Displays an alert asking user if the want to cancel encoding of current job.
 * Cancel: returns immediately after posting the alert. Later, when the user
 * acknowledges the alert, doCancelCurrentJob is called.
 */
- (IBAction)cancelRip:(id)sender
{
    [self.core pause];

    // Which window to attach the sheet to?
    NSWindow *window;
    if ([sender respondsToSelector:@selector(window)])
    {
        window = [sender window];
    }
    else
    {
        window = self.window;
    }

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"You are currently encoding. What would you like to do ?", nil)];
    [alert setInformativeText:NSLocalizedString(@"Your encode will be cancelled if you don't continue encoding.", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Continue Encoding", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Stop", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Continue", nil)];
    [alert setAlertStyle:NSCriticalAlertStyle];

    [alert beginSheetModalForWindow:window
                      modalDelegate:self
                     didEndSelector:@selector(didDimissCancel:returnCode:contextInfo:)
                        contextInfo:nil];
    [alert release];
}

- (void)didDimissCancel:(NSAlert *)alert
             returnCode:(NSInteger)returnCode
            contextInfo:(void *)contextInfo
{
    [self.core resume];

    if (returnCode == NSAlertSecondButtonReturn)
    {
        [self cancelCurrentJobAndStop];
    }
    else if (returnCode == NSAlertThirdButtonReturn)
    {
        [self cancelCurrentJobAndContinue];
    }
}

/**
 * Starts or cancels the processing of jobs depending on the current state
 */
- (IBAction)toggleStartCancel:(id)sender
{
    HBState s = self.core.state;
    if ((s == HBStatePaused) || (s == HBStateWorking) || (s == HBStateMuxing))
    {
        [self cancelRip:self];
    }
    else if (self.pendingItemsCount > 0)
    {
        [self rip:self];
    }
}

/**
 * Toggles the pause/resume state of libhb
 */
- (IBAction)togglePauseResume:(id)sender
{
    HBState s = self.core.state;
    if (s == HBStatePaused)
    {
        [self.core resume];
    }
    else if (s == HBStateWorking || s == HBStateMuxing)
    {
        [self.core pause];
    }
}

/**
 * Send the selected queue item back to the main window for rescan and possible edit.
 */
- (IBAction)editSelectedQueueItem:(id)sender
{
    NSInteger row = [self.outlineView clickedRow];
    if (row == NSNotFound)
    {
        return;
    }

    // if this is a currently encoding job, we need to be sure to alert the user,
    // to let them decide to cancel it first, then if they do, we can come back and
    // remove it
    HBJob *job = self.jobs[row];
    if (job.state == HBJobStateWorking)
    {
        // We pause the encode here so that it doesn't finish right after and then
        // screw up the sync while the window is open
        [self togglePauseResume:nil];
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
                            contextInfo:job];
        [alert release];
    }
    else
    { 
        // since we are not a currently encoding item, we can just be cancelled
        HBJob *item = [[[self.jobs[row] representedObject] copy] autorelease];
        [self.controller rescanJobToMainWindow:item];

        // Now that source is loaded and settings applied, delete the queue item from the queue
        [self removeQueueItemAtIndex:row];
    }
}

- (IBAction)clearAll:(id)sender
{
    [self.jobs beginTransaction];
    [self removeItemsUsingBlock:^BOOL(HBJob *item) {
        return (item.state != HBJobStateWorking);
    }];
    [self.jobs commit];
    [self reloadQueue];
}

- (IBAction)clearCompleted:(id)sender
{
    [self.jobs beginTransaction];
    [self removeItemsUsingBlock:^BOOL(HBJob *item) {
        return (item.state == HBJobStateCompleted);
    }];
    [self.jobs commit];
    [self reloadQueue];
}

#pragma mark -
#pragma mark NSOutlineView delegate

- (id)outlineView:(NSOutlineView *)fOutlineView child:(NSInteger)index ofItem:(id)item
{
    if (item == nil)
    {
        return self.jobs[index];
    }

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
    {
        return [self.jobs count];
    }
    else
    {
        return 0;
    }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    id item = [notification userInfo][@"NSObject"];
    NSInteger row = [self.outlineView rowForItem:item];
    [self.outlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    id item = [notification userInfo][@"NSObject"];
    NSInteger row = [self.outlineView rowForItem:item];
    [self.outlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
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

- (id)outlineView:(NSOutlineView *)fOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([tableColumn.identifier isEqualToString:@"desc"])
    {
        HBJob *job = item;

        if (self.descriptions[@(job.hash)])
        {
            return self.descriptions[@(job.hash)];
        }

        NSAttributedString *finalString = job.attributedDescription;

        self.descriptions[@(job.hash)] = finalString;;

        return finalString;
    }
    else if ([tableColumn.identifier isEqualToString:@"icon"])
    {
        HBJob *job = item;
        if (job.state == HBJobStateCompleted)
        {
            return [NSImage imageNamed:@"EncodeComplete"];
        }
        else if (job.state == HBJobStateWorking)
        {
            return [NSImage imageNamed:@"EncodeWorking0"];
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

/**
 * This method inserts the proper action icons into the far right of the queue window
 */
- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([tableColumn.identifier isEqualToString:@"desc"])
    {
        // nb: The "desc" column is currently an HBImageAndTextCell. However, we are longer
        // using the image portion of the cell so we could switch back to a regular NSTextFieldCell.

        // Set the image here since the value returned from outlineView:objectValueForTableColumn: didn't specify the image part
        [cell setImage:nil];
    }
    else if ([tableColumn.identifier isEqualToString:@"action"])
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
    if ([outlineView isItemExpanded:item])
    {
        [cell setImagePosition: NSImageAbove];
    }
    else
    {
        [cell setImagePosition: NSImageOnly];
    }
}

#pragma mark -
#pragma mark NSOutlineView delegate (dragging related)

- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
    // Dragging is only allowed of the pending items.
    if ([items[0] state] != HBJobStateReady)
    {
        return NO;
    }

    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    self.dragNodesArray = items;

    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:@[DragDropSimplePboardType] owner:self];

    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:DragDropSimplePboardType];

    return YES;
}

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
        index = [self.outlineView rowForItem: item] + 1;
        item = nil;
    }

    // We do not let the user drop a pending job before or *above*
    // already finished or currently encoding jobs.
    NSInteger encodingIndex = [self.jobs indexOfObject:self.currentJob];
    if (index <= encodingIndex)
    {
        return NSDragOperationNone;
        index = MAX (index, encodingIndex);
	}

    [outlineView setDropItem:item dropChildIndex:index];
    return NSDragOperationGeneric;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index
{
    NSMutableIndexSet *moveItems = [NSMutableIndexSet indexSet];

    for (id obj in self.dragNodesArray)
    {
        [moveItems addIndex:[self.jobs indexOfObject:obj]];
    }

    // Successful drop, we use moveObjectsInQueueArray:... in fHBController
    // to properly rearrange the queue array, save it to plist and then send it back here.
    // since Controller.mm is handling all queue array manipulation.
    // We could do this here, but I think we are better served keeping that code together.
    [self moveObjectsInQueueArray:self.jobs fromIndexes:moveItems toIndex: index];

    return YES;
}

@end
