/*  HBCore.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBCore.h"
#import "HBJob.h"
#import "HBDVDDetector.h"
#import "HBUtilities.h"

#include <dlfcn.h>

// These constants specify various status notifications sent by HBCore

/// Notification sent to update status while scanning. Matches HB_STATE_SCANNING constant in libhb.
NSString *HBCoreScanningNotification = @"HBCoreScanningNotification";

/// Notification sent after scanning is complete. Matches HB_STATE_SCANDONE constant in libhb.
NSString *HBCoreScanDoneNotification = @"HBCoreScanDoneNotification";

/// Notification sent to update status while searching. Matches HB_STATE_SEARCHING constant in libhb.
NSString *HBCoreSearchingNotification = @"HBCoreSearchingNotification";

/// Notification sent to update status while encoding. Matches HB_STATE_WORKING constant in libhb.
NSString *HBCoreWorkingNotification = @"HBCoreWorkingNotification";

/// Notification sent when encoding is paused. Matches HB_STATE_PAUSED constant in libhb.
NSString *HBCorePausedNotification = @"HBCorePausedNotification";

/// Notification sent after encoding is complete. Matches HB_STATE_WORKDONE constant in libhb.
NSString *HBCoreWorkDoneNotification = @"HBCoreWorkDoneNotification";

/// Notification sent to update status while muxing. Matches HB_STATE_MUXING constant in libhb.
NSString *HBCoreMuxingNotification = @"HBCoreMuxingNotification";

/**
 * Private methods of HBCore.
 */
@interface HBCore ()

/// Current state of HBCore.
@property (nonatomic, readwrite) HBState state;

/// Timer used to poll libhb for state changes.
@property (nonatomic, readwrite, retain) NSTimer *updateTimer;

/// Current scanned titles.
@property (nonatomic, readwrite, retain) NSArray *titles;

- (void)stateUpdateTimer:(NSTimer *)timer;

@end

@implementation HBCore

+ (void)setDVDNav:(BOOL)enabled
{
    hb_dvd_set_dvdnav(enabled);
}

+ (void)closeGlobal
{
    hb_global_close();
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

- (void)scan:(NSURL *)url titleNum:(NSUInteger)titleNum previewsNum:(NSUInteger)previewsNum minTitleDuration:(NSUInteger)minTitleDuration;
{
    // Start the timer to handle libhb state changes
    [self startUpdateTimer];

    NSString *path = url.path;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];

    if (detector.isVideoDVD)
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = detector.devicePath;
    }

    // convert minTitleDuration from seconds to the internal HB time
    uint64_t min_title_duration_ticks = 90000LL * minTitleDuration;

    // If there is no title number passed to scan, we use 0
    // which causes the default behavior of a full source scan
    if (titleNum > 0)
    {
        [HBUtilities writeToActivityLog:"%s scanning specifically for title: %d", self.name.UTF8String, titleNum];
    }
    else
    {
        // minimum title duration doesn't apply to title-specific scan
        // it doesn't apply to batch scan either, but we can't tell it apart from DVD & BD folders here
        [HBUtilities writeToActivityLog:"%s scanning titles with a duration of %d seconds or more", self.name.UTF8String, minTitleDuration];
    }

    hb_system_sleep_prevent(_hb_handle);

    hb_scan(_hb_handle, path.fileSystemRepresentation,
            (int)titleNum, (int)previewsNum,
            1, min_title_duration_ticks);

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateScanning;
}

/**
 *  Creates an array of lightweight HBTitles instances.
 */
- (void)scanDone
{
    hb_title_set_t *title_set = hb_get_title_set(_hb_handle);
    NSMutableArray *titles = [NSMutableArray array];

    for (int i = 0; i < hb_list_count(title_set->list_title); i++)
    {
        hb_title_t *title = (hb_title_t *) hb_list_item(title_set->list_title, i);
        [titles addObject:[[[HBTitle alloc] initWithTitle:title featured:(title->index == title_set->feature)] autorelease]];
    }

    self.titles = [[titles copy] autorelease];

    [HBUtilities writeToActivityLog:"%s scan done", self.name.UTF8String];
}

- (void)cancelScan
{
    hb_scan_stop(_hb_handle);

    [HBUtilities writeToActivityLog:"%s scan cancelled", self.name.UTF8String];
}

#pragma mark - Encodes

- (void)encodeJob:(HBJob *)job
{
    hb_job_t *hb_job = job.hb_job;

    [HBUtilities writeToActivityLog: "processNewQueueEncode number of passes expected is: %d", (job.video.twoPass + 1)];
    hb_job_set_file(hb_job, job.destURL.path.fileSystemRepresentation);

    // If scanning we need to do some extra setup of the job.
    if (hb_job->indepth_scan == 1)
    {
        char *encoder_preset_tmp  = hb_job->encoder_preset  != NULL ? strdup(hb_job->encoder_preset)  : NULL;
        char *encoder_tune_tmp    = hb_job->encoder_tune    != NULL ? strdup(hb_job->encoder_tune)    : NULL;
        char *encoder_options_tmp = hb_job->encoder_options != NULL ? strdup(hb_job->encoder_options) : NULL;
        char *encoder_profile_tmp = hb_job->encoder_profile != NULL ? strdup(hb_job->encoder_profile) : NULL;
        char *encoder_level_tmp   = hb_job->encoder_level   != NULL ? strdup(hb_job->encoder_level)   : NULL;
        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        hb_job_set_encoder_preset (hb_job, NULL);
        hb_job_set_encoder_tune   (hb_job, NULL);
        hb_job_set_encoder_options(hb_job, NULL);
        hb_job_set_encoder_profile(hb_job, NULL);
        hb_job_set_encoder_level  (hb_job, NULL);
        hb_job->pass = -1;
        hb_add(self.hb_handle, hb_job);
        /*
         * reset the advanced settings
         */
        hb_job_set_encoder_preset (hb_job, encoder_preset_tmp);
        hb_job_set_encoder_tune   (hb_job, encoder_tune_tmp);
        hb_job_set_encoder_options(hb_job, encoder_options_tmp);
        hb_job_set_encoder_profile(hb_job, encoder_profile_tmp);
        hb_job_set_encoder_level  (hb_job, encoder_level_tmp);
        free(encoder_preset_tmp);
        free(encoder_tune_tmp);
        free(encoder_options_tmp);
        free(encoder_profile_tmp);
        free(encoder_level_tmp);
    }

    if (job.video.twoPass)
    {
        hb_job->indepth_scan = 0;
        hb_job->pass = 1;
        hb_add(self.hb_handle, hb_job);
        hb_job->pass = 2;
        hb_add(self.hb_handle, hb_job);
    }
    else
    {
        hb_job->indepth_scan = 0;
        hb_job->pass = 0;
        hb_add(self.hb_handle, hb_job);
    }

    // Free the job
    hb_job_close(&hb_job);

    [self start];
}

- (void)start
{
    // Start the timer to handle libhb state changes
    [self startUpdateTimer];

    hb_system_sleep_prevent(_hb_handle);
    hb_start(_hb_handle);

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateWorking;

    [HBUtilities writeToActivityLog:"%s work started", self.name.UTF8String];
}

- (void)workDone
{
    // HB_STATE_WORKDONE happpens as a result of libhb finishing all its jobs
    // or someone calling hb_stop. In the latter case, hb_stop does not clear
    // out the remaining passes/jobs in the queue. We'll do that here.
    hb_job_t *job;
    while ((job = hb_job(_hb_handle, 0)))
        hb_rem(_hb_handle, job);

    [HBUtilities writeToActivityLog:"%s work done", self.name.UTF8String];
}

- (void)stop
{
    hb_stop(_hb_handle);
    hb_system_sleep_allow(_hb_handle);

    [HBUtilities writeToActivityLog:"%s stop", self.name.UTF8String];
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
 * Starts the timer used to polls libhb for state changes.
 */
- (void)startUpdateTimer
{
    if (!self.updateTimer)
    {
        self.updateTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
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
            return @selector(handleHBStateWorking);
        case HB_STATE_SCANNING:
            return @selector(handleHBStateScanning);
        case HB_STATE_MUXING:
            return @selector(handleHBStateMuxing);
        case HB_STATE_PAUSED:
            return @selector(handleHBStatePaused);
        case HB_STATE_SEARCHING:
            return @selector(handleHBStateSearching);
        case HB_STATE_SCANDONE:
            return @selector(handleHBStateScanDone);
        case HB_STATE_WORKDONE:
            return @selector(handleHBStateWorkDone);
        default:
            NSAssert1(NO, @"[HBCore selectorForState:] unknown state %lu", stateValue);
            return NULL;
    }
}

/**
 * This method polls libhb continuously for state changes and processes them.
 * Additional processing for each state is performed in methods that start
 * with 'handle' (e.g. handleHBStateScanning).
 */
- (void)stateUpdateTimer:(NSTimer *)timer
{
    if (!_hb_handle)
    {
        // Libhb is not open so we cannot do anything.
        return;
    }
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
 * Processes HBStateScanning state information. Current implementation just
 * sends HBCoreScanningNotification.
 */
- (void)handleHBStateScanning
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreScanningNotification object:self];    
}

/**
 * Processes HBStateScanDone state information. Current implementation just
 * sends HBCoreScanDoneNotification.
 */
- (void)handleHBStateScanDone
{
    [self scanDone];
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreScanDoneNotification object:self];    
}

/**
 * Processes HBStateWorking state information. Current implementation just
 * sends HBCoreWorkingNotification.
 */
- (void)handleHBStateWorking
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreWorkingNotification object:self];    
}

/**
 * Processes HBStatePaused state information. Current implementation just
 * sends HBCorePausedNotification.
 */
- (void)handleHBStatePaused
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCorePausedNotification object:self];    
}

/**
 * Processes HBStateWorkDone state information. Current implementation just
 * sends HBCoreWorkDoneNotification.
 */
- (void)handleHBStateWorkDone
{
    [self workDone];
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreWorkDoneNotification object:self];    
}

/**
 * Processes HBStateMuxing state information. Current implementation just
 * sends HBCoreMuxingNotification.
 */
- (void)handleHBStateMuxing
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreMuxingNotification object:self];    
}

/**
 * Processes HBStateSearching state information. Current implementation just
 * sends HBCoreSearchingNotification.
 */
- (void)handleHBStateSearching
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBCoreSearchingNotification object:self];
}

@end
