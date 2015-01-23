/*  HBCore.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBCore.h"
#import "HBJob.h"
#import "HBJob+HBJobConversion.h"
#import "HBDVDDetector.h"
#import "HBUtilities.h"

#include <dlfcn.h>

static BOOL globalInitialized = NO;

static void (^errorHandler)(NSString *error) = NULL;
static void hb_error_handler(const char *errmsg)
{
    NSString *error = @(errmsg);
    if (error)
    {
        errorHandler(error);
    }
}

/**
 * Private methods of HBCore.
 */
@interface HBCore ()

/// Pointer to a hb_state_s struct containing the detailed state information of libhb.
@property (nonatomic, readonly) hb_state_t *hb_state;

/// Current state of HBCore.
@property (nonatomic, readwrite) HBState state;

/// Timer used to poll libhb for state changes.
@property (nonatomic, readwrite, retain) NSTimer *updateTimer;

/// Current scanned titles.
@property (nonatomic, readwrite, retain) NSArray *titles;

/// Progress handler.
@property (nonatomic, readwrite, copy) HBCoreProgressHandler progressHandler;

/// Completion handler.
@property (nonatomic, readwrite, copy) HBCoreCompletionHandler completionHandler;

/// User cancelled.
@property (nonatomic, readwrite, getter=isCancelled) BOOL cancelled;

@end

@implementation HBCore

+ (void)setDVDNav:(BOOL)enabled
{
    hb_dvd_set_dvdnav(enabled);
}

+ (void)initGlobal
{
    hb_global_init();
    globalInitialized = YES;
}

+ (void)closeGlobal
{
    NSAssert(globalInitialized, @"[HBCore closeGlobal] global closed but not initialized");
    [errorHandler release];
    hb_global_close();
}

+ (void)registerErrorHandler:(void (^)(NSString *error))handler
{
    errorHandler = [handler copy];
    hb_register_error_handler(&hb_error_handler);
}

/**
 * Initializes HBCore.
 */
- (instancetype)init
{
    return [self initWithLoggingLevel:0];
}

/**
 * Opens low level HandBrake library. This should be called once before other
 * functions HBCore are used.
 *
 * @param debugMode         If set to YES, libhb will print verbose debug output.
 *
 * @return YES if libhb was opened, NO if there was an error.
 */
- (instancetype)initWithLoggingLevel:(int)loggingLevel
{
    self = [super init];
    if (self)
    {
        _name = @"HBCore";
        _state = HBStateIdle;
        _hb_state = malloc(sizeof(struct hb_state_s));

        _hb_handle = hb_init(loggingLevel, 0);
        if (!_hb_handle)
        {
            [self release];
            return nil;
        }
    }

    return self;
}

/**
 * Releases resources.
 */
- (void)dealloc
{
    [self stopUpdateTimer];
    hb_close(&_hb_handle);
    _hb_handle = NULL;

    free(_hb_state);
    [super dealloc];
}

#pragma mark - Scan

- (BOOL)canScan:(NSURL *)url error:(NSError **)error
{
    if (![[NSFileManager defaultManager] fileExistsAtPath:url.path]) {
        if (*error) {
            *error = [NSError errorWithDomain:@"HBErrorDomain"
                                         code:100
                                     userInfo:@{ NSLocalizedDescriptionKey: @"Unable to find the file at the specified URL" }];
        }

        return NO;
    }

    HBDVDDetector *detector = [HBDVDDetector detectorForPath:url.path];

    if (detector.isVideoDVD)
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.

        [HBUtilities writeToActivityLog:"%s trying to open a physical dvd at: %s", self.name.UTF8String, url.path.UTF8String];

        // Notify the user that we don't support removal of copy protection.
        void *dvdcss = dlopen("libdvdcss.2.dylib", RTLD_LAZY);
        if (dvdcss)
        {
            // libdvdcss was found so all is well
            [HBUtilities writeToActivityLog:"%s libdvdcss.2.dylib found for decrypting physical dvd", self.name.UTF8String];
            dlclose(dvdcss);
        }
        else
        {
            // compatible libdvdcss not found
            [HBUtilities writeToActivityLog:"%s, libdvdcss.2.dylib not found for decrypting physical dvd", self.name.UTF8String];

            if (error) {
                *error = [NSError errorWithDomain:@"HBErrorDomain"
                                             code:101
                                         userInfo:@{ NSLocalizedDescriptionKey: @"libdvdcss.2.dylib not found for decrypting physical dvd" }];
            }
        }
    }

    return YES;
}

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    NSAssert(self.state == HBStateIdle, @"[HBCore scanURL:] called while another scan or encode already in progress");

    // Reset the titles array
    self.titles = nil;

    // Copy the progress/completation blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.2];

    NSString *path = url.path;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];

    if (detector.isVideoDVD)
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = detector.devicePath;
    }

    // convert minTitleDuration from seconds to the internal HB time
    uint64_t min_title_duration_ticks = 90000LL * seconds;

    // If there is no title number passed to scan, we use 0
    // which causes the default behavior of a full source scan
    if (index > 0)
    {
        [HBUtilities writeToActivityLog:"%s scanning specifically for title: %d", self.name.UTF8String, index];
    }
    else
    {
        // minimum title duration doesn't apply to title-specific scan
        // it doesn't apply to batch scan either, but we can't tell it apart from DVD & BD folders here
        [HBUtilities writeToActivityLog:"%s scanning titles with a duration of %d seconds or more", self.name.UTF8String, seconds];
    }

    hb_system_sleep_prevent(_hb_handle);

    hb_scan(_hb_handle, path.fileSystemRepresentation,
            (int)index, (int)previewsNum,
            1, min_title_duration_ticks);

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateScanning;
}

/**
 *  Creates an array of lightweight HBTitles instances.
 */
- (BOOL)scanDone
{
    if (self.isCancelled)
    {
        self.cancelled = NO;
        return NO;
    }

    hb_title_set_t *title_set = hb_get_title_set(_hb_handle);
    NSMutableArray *titles = [NSMutableArray array];

    for (int i = 0; i < hb_list_count(title_set->list_title); i++)
    {
        hb_title_t *title = (hb_title_t *) hb_list_item(title_set->list_title, i);
        [titles addObject:[[[HBTitle alloc] initWithTitle:title featured:(title->index == title_set->feature)] autorelease]];
    }

    self.titles = [[titles copy] autorelease];

    [HBUtilities writeToActivityLog:"%s scan done", self.name.UTF8String];

    return (self.titles.count > 0);
}

- (void)cancelScan
{
    self.cancelled = YES;
    hb_scan_stop(_hb_handle);
    [HBUtilities writeToActivityLog:"%s scan cancelled", self.name.UTF8String];
}

#pragma mark - Encodes

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;
{
    NSAssert(self.state == HBStateIdle, @"[HBCore encodeJob:] called while another scan or encode already in progress");

    // Copy the progress/completation blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.5];

    // Add the job to libhb
    hb_job_t *hb_job = job.hb_job;
    hb_job_set_file(hb_job, job.destURL.path.fileSystemRepresentation);
    hb_add(_hb_handle, hb_job);

    // Free the job
    hb_job_close(&hb_job);

    hb_system_sleep_prevent(_hb_handle);
    hb_start(_hb_handle);

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateWorking;

    [HBUtilities writeToActivityLog:"%s started encoding %s", self.name.UTF8String, job.destURL.lastPathComponent.UTF8String];
}

- (BOOL)workDone
{
    // HB_STATE_WORKDONE happpens as a result of libhb finishing all its jobs
    // or someone calling hb_stop. In the latter case, hb_stop does not clear
    // out the remaining passes/jobs in the queue. We'll do that here.
    hb_job_t *job;
    while ((job = hb_job(_hb_handle, 0)))
    {
        hb_rem(_hb_handle, job);
    }

    [HBUtilities writeToActivityLog:"%s work done", self.name.UTF8String];

    if (self.isCancelled)
    {
        self.cancelled = NO;
        return NO;
    }
    else
    {
        return YES;
    }
}

- (void)cancelEncode
{
    self.cancelled = YES;
    hb_stop(_hb_handle);
    [HBUtilities writeToActivityLog:"%s encode cancelled", self.name.UTF8String];
}


- (void)pause
{
    hb_pause(_hb_handle);
    hb_system_sleep_allow(_hb_handle);
}

- (void)resume
{
    hb_resume(_hb_handle);
    hb_system_sleep_prevent(_hb_handle);
}

#pragma mark - State updates

/**
 *  Starts the timer used to polls libhb for state changes.
 *
 *  @param seconds The number of seconds between firings of the timer.
 */
- (void)startUpdateTimerWithInterval:(NSTimeInterval)seconds
{
    if (!self.updateTimer)
    {
        self.updateTimer = [NSTimer scheduledTimerWithTimeInterval:seconds
                                                            target:self
                                                          selector:@selector(stateUpdateTimer:)
                                                          userInfo:NULL
                                                           repeats:YES];

        [[NSRunLoop currentRunLoop] addTimer:self.updateTimer forMode:NSEventTrackingRunLoopMode];
    }
}

/**
 * Stops the update timer.
 */
- (void)stopUpdateTimer
{
    [self.updateTimer invalidate];
    self.updateTimer = nil;
}

/**
 * Transforms a libhb state constant to a matching HBCore selector.
 */
- (const SEL)selectorForState:(HBState)stateValue
{
    switch (stateValue)
    {
        case HB_STATE_WORKING:
        case HB_STATE_SCANNING:
        case HB_STATE_MUXING:
        case HB_STATE_PAUSED:
        case HB_STATE_SEARCHING:
            return @selector(handleProgress);
        case HB_STATE_SCANDONE:
            return @selector(handleScanCompletion);
        case HB_STATE_WORKDONE:
            return @selector(handleWorkCompletion);
        default:
            NSAssert1(NO, @"[HBCore selectorForState:] unknown state %lu", stateValue);
            return NULL;
    }
}

/**
 * This method polls libhb continuously for state changes and processes them.
 * Additional processing for each state is performed in methods that start
 * with 'handle'.
 */
- (void)stateUpdateTimer:(NSTimer *)timer
{
    hb_get_state(_hb_handle, _hb_state);

    if (_hb_state->state == HB_STATE_IDLE)
    {
        // Libhb reported HB_STATE_IDLE, so nothing interesting has happened.
        return;
    }

    // Update HBCore state to reflect the current state of libhb
    self.state = _hb_state->state;

    // Determine name of the method that does further processing for this state.
    SEL sel = [self selectorForState:self.state];

    if (_hb_state->state == HB_STATE_WORKDONE || _hb_state->state == HB_STATE_SCANDONE)
    {
        // Libhb reported HB_STATE_WORKDONE or HB_STATE_SCANDONE,
        // so nothing interesting will happen after this point, stop the timer.
        [self stopUpdateTimer];

        // Set the state to idle, because the update timer won't fire again.
        self.state = HBStateIdle;
        hb_system_sleep_allow(_hb_handle);
    }

    // Call the determined selector.
    [self performSelector:sel];
}

#pragma mark - Notifications

/**
 * Processes progress state information.
 */
- (void)handleProgress
{
    if (self.progressHandler)
    {
        self.progressHandler(self.state, *(self.hb_state));
    }
}

/**
 *  Runs the completion block and clean ups the internal blocks.
 *
 *  @param result the result to pass to the completion block.
 */
- (void)runCompletionBlockAndCleanUpWithResult:(BOOL)result
{
    if (self.completionHandler)
    {
        HBCoreCompletionHandler completionHandler = [self.completionHandler retain];
        self.progressHandler = nil;
        self.completionHandler = nil;
        completionHandler(result);
        [completionHandler release];
    }
}

/**
 * Processes scan completion.
 */
- (void)handleScanCompletion
{
    BOOL result = [self scanDone];
    [self runCompletionBlockAndCleanUpWithResult:result];
}

/**
 * Processes work completion.
 */
- (void)handleWorkCompletion
{
    BOOL result = [self workDone];
    [self runCompletionBlockAndCleanUpWithResult:result];
}

@end
