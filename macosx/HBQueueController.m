
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

#import "HBStateFormatter.h"

#import "HBDistributedArray.h"
#import "NSArray+HBAdditions.h"

#import "HBDockTile.h"

#import "HBOutputRedirect.h"
#import "HBJobOutputFileWriter.h"
#import "HBPreferencesController.h"

// Pasteboard type for or drag operations
#define DragDropSimplePboardType    @"HBQueueCustomOutlineViewPboardType"

// DockTile update freqency in total percent increment
#define dockTileUpdateFrequency     0.1f

#define HB_ROW_HEIGHT_TITLE_ONLY    17.0

@interface HBQueueController () <NSOutlineViewDataSource, HBQueueOutlineViewDelegate>

@property (nonatomic, readonly) HBDockTile *dockTile;
@property (nonatomic, readwrite) double dockIconProgress;

@property (unsafe_unretained) IBOutlet NSTextField *progressTextField;
@property (unsafe_unretained) IBOutlet NSTextField *countTextField;
@property (unsafe_unretained) IBOutlet HBQueueOutlineView *outlineView;

@property (nonatomic, readonly) NSMutableDictionary *descriptions;

@property (nonatomic, readonly) HBDistributedArray *jobs;
@property (nonatomic, strong)   HBJob *currentJob;
@property (nonatomic, strong)   HBJobOutputFileWriter *currentLog;

@property (nonatomic, readwrite) BOOL stop;

@property (nonatomic, readwrite) NSUInteger pendingItemsCount;
@property (nonatomic, readwrite) NSUInteger completedItemsCount;

@property (nonatomic, strong) NSArray *dragNodesArray;

@end

@implementation HBQueueController

- (instancetype)init
{
    if (self = [super initWithWindowNibName:@"Queue"])
    {
        _descriptions = [[NSMutableDictionary alloc] init];

        // Load the dockTile and instiante initial text fields
        _dockTile = [[HBDockTile alloc] initWithDockTile:[[NSApplication sharedApplication] dockTile]
                                                  image:[[NSApplication sharedApplication] applicationIconImage]];

        int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];

        // Init a separate instance of libhb for the queue
        _core = [[HBCore alloc] initWithLogLevel:loggingLevel name:@"QueueCore"];

        [self loadQueueFile];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
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

    [self getQueueStats];
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

            return (self.pendingItemsCount > 0);
        }
        else if (self.core.state != HBStateIdle)
        {
            menuItem.title = NSLocalizedString(@"Stop Encoding", nil);

            return YES;
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
        action == @selector(revealSelectedQueueItems:))
    {
        return (self.outlineView.selectedRow != -1 || self.outlineView.clickedRow != -1);
    }

    if (action == @selector(resetJobState:))
    {
        return self.outlineView.targetedRowIndexes.count > 0;
    }

    if (action == @selector(clearAll:))
    {
        return self.jobs.count > 0;
    }

    if (action == @selector(clearCompleted:))
    {
        return self.completedItemsCount > 0;
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
            return YES;
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
    NSURL *queueURL = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"Queue.hbqueue"];
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
    [self removeQueueItemsAtIndexes:[NSIndexSet indexSetWithIndex:index]];
}

- (void)removeQueueItemsAtIndexes:(NSIndexSet *)indexes
{
    if (self.jobs.count > [indexes lastIndex])
    {
        [self.jobs removeObjectsAtIndexes:indexes];
    }
}

/**
 *  Updates the queue status label.
 */
- (void)getQueueStats
{
    // lets get the stats on the status of the queue array
    NSUInteger pendingCount = 0;
    NSUInteger completedCount = 0;

    for (HBJob *job in self.jobs)
    {
        if (job.state == HBJobStateReady)
        {
            pendingCount++;
        }
        if (job.state == HBJobStateCompleted)
        {
            completedCount++;
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
    self.completedItemsCount = completedCount;
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
- (void)removeCompletedJobs
{
    [self.jobs beginTransaction];
    [self.jobs removeObjectsUsingBlock:^BOOL(HBJob *item) {
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
        self.currentLog = [[HBJobOutputFileWriter alloc] initWithJob:self.currentJob];
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
        [self jobCompletedAlerts];

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
    HBStateFormatter *formatter = [[HBStateFormatter alloc] init];

    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    [self.core scanURL:scanURL
            titleIndex:index
              previews:10
           minDuration:0
       progressHandler:^(HBState state, hb_state_t hb_state) {
           NSString *status = [formatter stateToString:hb_state title:nil];

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

    HBStateFormatter *converter = [[HBStateFormatter alloc] init];
    NSString *destinationName = self.currentJob.destURL.lastPathComponent;

    // We should be all setup so let 'er rip
    [self.core encodeJob:self.currentJob
         progressHandler:^(HBState state, hb_state_t hb_state) {
             NSString *string = [converter stateToString:hb_state title:destinationName];
             CGFloat progress = [converter stateToPercentComplete:hb_state];

             if (state == HBStateWorking)
             {
                 // Update dock icon
                 if (self.dockIconProgress < 100.0 * progress)
                 {
                     // ETA format is [XX]X:XX:XX when ETA is greater than one hour
                     // [X]X:XX when ETA is greater than 0 (minutes or seconds)
                     // When these conditions doesn't applied (eg. when ETA is undefined)
                     // we show just a tilde (~)

                     #define p hb_state.param.working
                     NSString *etaStr;
                     if (p.hours > 0)
                         etaStr = [NSString stringWithFormat:@"%d:%02d:%02d", p.hours, p.minutes, p.seconds];
                     else if (p.minutes > 0 || p.seconds > 0)
                         etaStr = [NSString stringWithFormat:@"%d:%02d", p.minutes, p.seconds];
                     else
                         etaStr = @"~";
                     #undef p

                     [self.dockTile updateDockIcon:progress withETA:etaStr];
                     self.dockIconProgress += dockTileUpdateFrequency;
                 }
             }
             else if (state == HBStateMuxing)
             {
                 [self.dockTile updateDockIcon:1.0 withETA:@""];
             }

             // Update text field
             self.progressTextField.stringValue = string;
             [self.controller setQueueInfo:string progress:progress hidden:NO];
         }
     completionHandler:^(BOOL success) {
         NSString *info = NSLocalizedString(@"Encode Finished.", @"");

         self.progressTextField.stringValue = info;
         [self.controller setQueueInfo:info progress:1.0 hidden:YES];

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

- (void)showDoneNotification:(NSURL *)fileURL
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
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"sendToMetaX"] == YES)
    {
        NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
        NSString *sendToApp = [workspace fullPathForApplication:[[NSUserDefaults standardUserDefaults] objectForKey:@"SendCompletedEncodeToApp"]];

        if (sendToApp)
        {
            if (![workspace openFile:fileURL.path withApplication:sendToApp])
            {
                [HBUtilities writeToActivityLog:"Failed to send file to: %s", sendToApp];
            }
        }
        else
        {
            [HBUtilities writeToActivityLog:"Send file to: app not found"];
        }
    }
}

- (void)jobCompletedAlerts
{
    // Both the Notification and Sending to tagger can be done as encodes roll off the queue
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionNotification ||
        [[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionAlertAndNotification)
    {
        // If Play System Alert has been selected in Preferences
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBAlertWhenDoneSound"] == YES)
        {
            NSBeep();
        }
        [self showDoneNotification:self.currentJob.destURL];
    }
}

- (void)queueCompletedAlerts
{
    // If Play System Alert has been selected in Preferences
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBAlertWhenDoneSound"] == YES)
    {
        NSBeep();
    }

    // If Alert Window or Window and Notification has been selected
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionAlert ||
        [[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionAlertAndNotification)
    {
        // On Screen Notification
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"Put down that cocktail…", @"")];
        [alert setInformativeText:NSLocalizedString(@"Your HandBrake queue is done!", @"")];
        [NSApp requestUserAttention:NSCriticalRequest];
        [alert runModal];
    }

    // If sleep has been selected
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionSleep)
    {
        // Sleep
        NSDictionary *errorDict;
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:
                                       @"tell application \"Finder\" to sleep"];
        [scriptObject executeAndReturnError: &errorDict];
    }
    // If Shutdown has been selected
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionShutDown)
    {
        // Shut Down
        NSDictionary *errorDict;
        NSAppleScript *scriptObject = [[NSAppleScript alloc] initWithSource:@"tell application \"Finder\" to shut down"];
        [scriptObject executeAndReturnError: &errorDict];
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
    NSMutableIndexSet *targetedRows = [[self.outlineView targetedRowIndexes] mutableCopy];

    if (targetedRows.count)
    {
        [self.jobs beginTransaction];

        // if this is a currently encoding job, we need to be sure to alert the user,
        // to let them decide to cancel it first, then if they do, we can come back and
        // remove it
        NSIndexSet *workingIndexes = [self.jobs indexesOfObjectsUsingBlock:^BOOL(HBJob *item) {
            return item.state == HBJobStateWorking;
        }];

        if ([targetedRows containsIndexes:workingIndexes])
        {
            [targetedRows removeIndexes:workingIndexes];
            NSArray *workingJobs = [self.jobs filteredArrayUsingBlock:^BOOL(HBJob *item) {
                return item.state == HBJobStateWorking;
            }];

            if ([workingJobs containsObject:self.currentJob])
            {
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
                                    contextInfo:NULL];
            }
        }

        // remove the non working items immediately
        [self removeQueueItemsAtIndexes:targetedRows];
        [self getQueueStats];

        [self.outlineView beginUpdates];
        [self.outlineView removeItemsAtIndexes:targetedRows inParent:nil withAnimation:NSTableViewAnimationEffectFade];
        [self.outlineView endUpdates];

        [self.jobs commit];
    }
}

- (void)didDimissCancelCurrentJob:(NSAlert *)alert
                       returnCode:(NSInteger)returnCode
                      contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        NSInteger index = [self.jobs indexOfObject:self.currentJob];
        [self cancelCurrentJobAndContinue];

        [self.jobs beginTransaction];
        [self removeQueueItemAtIndex:index];
        [self.jobs commit];
    }
}

/**
 * Show the finished encode in the finder
 */
- (IBAction)revealSelectedQueueItems: (id)sender
{
    NSIndexSet *targetedRows = [self.outlineView targetedRowIndexes];
    NSMutableArray *urls = [[NSMutableArray alloc] init];

    NSUInteger currentIndex = [targetedRows firstIndex];
    while (currentIndex != NSNotFound) {
        NSURL *url = [[self.jobs objectAtIndex:currentIndex] destURL];
        [urls addObject:url];
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:urls];
}

- (void)remindUserOfSleepOrShutdown
{
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionSleep)
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
    }
    else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"HBAlertWhenDone"] == HBDoneActionShutDown)
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
    [alert setMessageText:NSLocalizedString(@"You are currently encoding. What would you like to do?", nil)];
    [alert setInformativeText:NSLocalizedString(@"Your encode will be cancelled if you don't continue encoding.", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Continue Encoding", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Stop", nil)];
    [alert addButtonWithTitle:NSLocalizedString(@"Cancel Current and Continue", nil)];
    [alert setAlertStyle:NSCriticalAlertStyle];

    [alert beginSheetModalForWindow:window
                      modalDelegate:self
                     didEndSelector:@selector(didDimissCancel:returnCode:contextInfo:)
                        contextInfo:nil];
}

- (void)didDimissCancel:(NSAlert *)alert
             returnCode:(NSInteger)returnCode
            contextInfo:(void *)contextInfo
{
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

- (IBAction)resetJobState:(id)sender
{
    [self.jobs beginTransaction];

    NSIndexSet *targetedRows = [self.outlineView targetedRowIndexes];
    NSMutableIndexSet *updatedIndexes = [NSMutableIndexSet indexSet];

    NSUInteger currentIndex = [targetedRows firstIndex];
    while (currentIndex != NSNotFound) {
        HBJob *job = self.jobs[currentIndex];

        if (job.state == HBJobStateCanceled || job.state == HBJobStateCompleted)
        {
            job.state = HBJobStateReady;
            [updatedIndexes addIndex:currentIndex];
        }
        currentIndex = [targetedRows indexGreaterThanIndex:currentIndex];
    }

    [self.outlineView reloadDataForRowIndexes:updatedIndexes columnIndexes:[NSIndexSet indexSetWithIndex:0]];
    [self getQueueStats];

    [self.jobs commit];
}

/**
 * Send the selected queue item back to the main window for rescan and possible edit.
 */
- (IBAction)editSelectedQueueItem:(id)sender
{
    [self.jobs beginTransaction];

    NSInteger row = [self.outlineView clickedRow];
    if (row != NSNotFound)
    {
        // if this is a currently encoding job, we need to be sure to alert the user,
        // to let them decide to cancel it first, then if they do, we can come back and
        // remove it
        HBJob *job = self.jobs[row];
        if (job == self.currentJob)
        {
            NSString *alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Stop This Encode and Edit It?", nil)];

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
            [alert addButtonWithTitle:NSLocalizedString(@"Stop Encoding and Edit", nil)];
            [alert setAlertStyle:NSCriticalAlertStyle];

            [alert beginSheetModalForWindow:docWindow
                              modalDelegate:self
                             didEndSelector:@selector(didDimissEditCurrentJob:returnCode:contextInfo:)
                                contextInfo:(__bridge void *)(job)];
        }
        else if (job.state != HBJobStateWorking)
        {
            // since we are not a currently encoding item, we can just be edit it
            HBJob *item = [[self.jobs[row] representedObject] copy];
            if ([self.controller openJob:item])
            {
                // Now that source is loaded and settings applied, delete the queue item from the queue
                [self.outlineView beginUpdates];
                [self removeQueueItemAtIndex:row];
                [self.outlineView removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:row] inParent:nil withAnimation:NSTableViewAnimationEffectFade];
                [self.outlineView endUpdates];

                [self getQueueStats];
            }
        }
    }

    [self.jobs commit];
}

- (void)didDimissEditCurrentJob:(NSAlert *)alert
                       returnCode:(NSInteger)returnCode
                      contextInfo:(void *)contextInfo
{
    if (returnCode == NSAlertSecondButtonReturn)
    {
        HBJob *job = (__bridge HBJob *)contextInfo;
        NSInteger index = [self.jobs indexOfObject:job];

        if (job == self.currentJob)
        {
            [self cancelCurrentJobAndContinue];
        }

        if ([self.controller openJob:job])
        {
            [self.jobs beginTransaction];
            [self.controller openJob:job];
            [self removeQueueItemAtIndex:index];
            [self.jobs commit];
            [self getQueueStats];
        }
    }
}

- (IBAction)clearAll:(id)sender
{
    [self.jobs beginTransaction];
    [self.jobs removeObjectsUsingBlock:^BOOL(HBJob *item) {
        return (item.state != HBJobStateWorking);
    }];
    [self.jobs commit];
    [self reloadQueue];
}

- (IBAction)clearCompleted:(id)sender
{
    [self.jobs beginTransaction];
    [self.jobs removeObjectsUsingBlock:^BOOL(HBJob *item) {
        return (item.state == HBJobStateCompleted);
    }];
    [self.jobs commit];
    [self reloadQueue];
}

#pragma mark -
#pragma mark NSOutlineView data source

- (id)outlineView:(NSOutlineView *)fOutlineView child:(NSInteger)index ofItem:(id)item
{
    if (item == nil)
    {
        return self.jobs[index];
    }

    // We are only one level deep, so we can't be asked about children
    NSAssert(NO, @"HBQueueController outlineView:child:ofItem: can't handle nested items.");
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

#pragma mark NSOutlineView delegate

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    id item = notification.userInfo[@"NSObject"];
    NSInteger row = [self.outlineView rowForItem:item];
    [self.outlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    id item = notification.userInfo[@"NSObject"];
    NSInteger row = [self.outlineView rowForItem:item];
    [self.outlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (CGFloat)outlineView:(NSOutlineView *)outlineView heightOfRowByItem:(id)item
{
    if ([outlineView isItemExpanded:item])
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
    if ([tableColumn.identifier isEqualToString:@"action"])
    {
        [cell setEnabled: YES];
        BOOL highlighted = [outlineView isRowSelected:[outlineView rowForItem: item]] && [[outlineView window] isKeyWindow] && ([[outlineView window] firstResponder] == outlineView);

        HBJob *job = item;
        if (job.state == HBJobStateCompleted)
        {
            [cell setAction: @selector(revealSelectedQueueItems:)];
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

#pragma mark NSOutlineView delegate (dragging related)

- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
    // Dragging is only allowed of the pending items.
    if ([items[0] state] != HBJobStateReady)
    {
        return NO;
    }

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
    if (encodingIndex != NSNotFound && index <= encodingIndex)
    {
        return NSDragOperationNone;
        index = MAX (index, encodingIndex);
	}

    [outlineView setDropItem:item dropChildIndex:index];
    return NSDragOperationGeneric;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index
{
    [self.jobs beginTransaction];
    [self.outlineView beginUpdates];

    for (id object in self.dragNodesArray.reverseObjectEnumerator)
    {
        NSUInteger sourceIndex = [self.jobs indexOfObject:object];
        [self.jobs removeObjectAtIndex:sourceIndex];

        if (sourceIndex < index)
        {
            index--;
        }

        [self.jobs insertObject:object atIndex:index];

        NSUInteger destIndex = [self.jobs indexOfObject:object];

        [self.outlineView moveItemAtIndex:sourceIndex inParent:nil toIndex:destIndex inParent:nil];
    }

    [self.outlineView endUpdates];
    [self.jobs commit];

    return YES;
}

@end
