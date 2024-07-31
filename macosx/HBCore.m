/*  HBCore.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBCore.h"
#import "HBJob.h"
#import "HBJob+HBJobConversion.h"
#import "HBDVDDetector.h"
#import "HBUtilities.h"
#import "HBImageUtilities.h"
#import "HBDirectUtilities.h"

#import "HBStateFormatter+Private.h"
#import "HBTitle+Private.h"
#import "HBJob+Private.h"

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

typedef void (^HBCoreCleanupHandler)(void);

/**
 * Private methods of HBCore.
 */
@interface HBCore ()

/// Pointer to a hb_state_s struct containing the detailed state information of libhb.
@property (nonatomic, readonly) hb_state_t *hb_state;

/// Pointer to a libhb handle used by this HBCore instance.
@property (nonatomic, readonly) hb_handle_t *hb_handle;

/// Current state of HBCore.
@property (nonatomic, readwrite) HBState state;

/// Timer used to poll libhb for state changes.
@property (nonatomic, readwrite) dispatch_source_t updateTimer;
@property (nonatomic, readonly) dispatch_queue_t updateTimerQueue;

/// Current scanned titles.
@property (nonatomic, readwrite, copy) NSArray<HBTitle *> *titles;

/// Progress handler.
@property (nonatomic, readwrite, copy) HBCoreProgressHandler progressHandler;

/// Completion handler.
@property (nonatomic, readwrite, copy) HBCoreCompletionHandler completionHandler;

/// Cleanup handle, used for internal HBCore cleanup.
@property (nonatomic, readwrite, copy) HBCoreCleanupHandler cleanupHandler;

/// Progress
@property (nonatomic, readwrite) NSProgress *progress;

@end

HB_OBJC_DIRECT_MEMBERS
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
    hb_global_close();
}

+ (void)registerErrorHandler:(void (^)(NSString *error))handler
{
    errorHandler = [handler copy];
    hb_register_error_handler(&hb_error_handler);
}

+ (nullable NSURL *)temporaryDirectoryURL
{
    const char *path = hb_get_temporary_directory();
    if (path)
    {
        return [[NSURL alloc] initFileURLWithFileSystemRepresentation:path isDirectory:YES relativeToURL:nil];
    }
    else
    {
        return nil;
    }
}

+ (void)cleanTemporaryFiles
{
    NSURL *directory = [HBCore temporaryDirectoryURL];

    if (directory)
    {
        NSFileManager *manager = [[NSFileManager alloc] init];
        NSArray<NSURL *> *contents = [manager contentsOfDirectoryAtURL:directory
                                            includingPropertiesForKeys:nil
                                                               options:NSDirectoryEnumerationSkipsSubdirectoryDescendants | NSDirectoryEnumerationSkipsPackageDescendants
                                                                 error:NULL];

        for (NSURL *url in contents)
        {
            NSError *error = nil;
            BOOL result = [manager removeItemAtURL:url error:&error];
            if (result == NO && error)
            {
                [HBUtilities writeToActivityLog:"Could not remove existing temporary file at: %s", url.lastPathComponent.UTF8String];
            }
        }
    }
}

- (instancetype)init
{
    return [self initWithLogLevel:0 queue:dispatch_get_main_queue()];
}

- (instancetype)initWithLogLevel:(NSInteger)level queue:(dispatch_queue_t)queue
{
    self = [super init];
    if (self)
    {
        _name = @"HBCore";
        _automaticallyPreventSleep = NO;
        _state = HBStateIdle;
        _updateTimerQueue = queue;
        _titles = @[];

        _stateFormatter = [[HBStateFormatter alloc] init];
        _hb_state = malloc(sizeof(hb_state_t));
        bzero(_hb_state, sizeof(hb_state_t));
        _logLevel = level;

        _hb_handle = hb_init((int)level);
        if (!_hb_handle)
        {
            return nil;
        }

        // macOS Sonoma moved the parent of our temporary folder
        // to the app sandbox container, and the user might have deleted it,
        // so ensure the whole path is available to avoid failing later
        // when trying to write the temp files
        NSURL *directoryURL = HBCore.temporaryDirectoryURL;
        if (directoryURL)
        {
            [NSFileManager.defaultManager createDirectoryAtURL:directoryURL withIntermediateDirectories:YES attributes:nil error:NULL];
        }
    }

    return self;
}

- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name
{
    self = [self initWithLogLevel:level queue:dispatch_get_main_queue()];
    if (self)
    {
        _name = [name copy];
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
}

- (void)setLogLevel:(NSInteger)logLevel
{
    _logLevel = logLevel;
    hb_log_level_set(_hb_handle, (int)logLevel);
}

- (void)preventSleep
{
    NSAssert(!self.automaticallyPreventSleep, @"[HBCore preventSleep:] called with automaticallyPreventSleep enabled.");
    hb_system_sleep_prevent(_hb_handle);
}

- (void)allowSleep
{
    NSAssert(!self.automaticallyPreventSleep, @"[HBCore allowSleep:] called with automaticallyPreventSleep enabled.");
    hb_system_sleep_allow(_hb_handle);
}

- (void)preventAutoSleep
{
    if (self.automaticallyPreventSleep)
    {
        hb_system_sleep_prevent(_hb_handle);
    }
}

- (void)allowAutoSleep
{
    if (self.automaticallyPreventSleep)
    {
        hb_system_sleep_allow(_hb_handle);
    }
}

#pragma mark - Scan

- (BOOL)canScan:(NSArray<NSURL *> *)urls error:(NSError * __autoreleasing *)error
{
    NSAssert(urls, @"[HBCore canScan:] called with nil urls.");

    for (NSURL *url in urls)
    {
#ifdef __SANDBOX_ENABLED__
        __unused HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:url];
#endif

        if (![url checkResourceIsReachableAndReturnError:NULL])
        {
            if (error)
            {
                *error = [NSError errorWithDomain:@"HBErrorDomain"
                                             code:100
                                         userInfo:@{ NSLocalizedDescriptionKey: @"Unable to find the file at the specified URL" }];
            }
            
            return NO;
        }
        
        HBDVDDetector *detector = [HBDVDDetector detectorForPath:url.path];
        
        if (detector.isVideoDVD || detector.isVideoBluRay)
        {
            [HBUtilities writeToActivityLog:"%s trying to open a physical disc at: %s", self.name.UTF8String, url.path.UTF8String];
            void *lib = NULL;
            
            if (detector.isVideoDVD)
            {
                lib = dlopen("libdvdcss.2.dylib", RTLD_LAZY);
                if (!lib)
                {
                    lib = dlopen("/usr/local/lib/libdvdcss.2.dylib", RTLD_LAZY);
                }
            }
            else if (detector.isVideoBluRay)
            {
                lib = dlopen("libaacs.dylib", RTLD_LAZY);
                if (!lib)
                {
                    lib = dlopen("/usr/local/lib/libaacs.dylib", RTLD_LAZY);
                }
            }
            
            if (lib)
            {
                dlclose(lib);
                [HBUtilities writeToActivityLog:"%s library found for decrypting physical disc", self.name.UTF8String];
            }
            else
            {
                const char *dlError = dlerror();
                
                if (dlError)
                {
                    [HBUtilities writeToActivityLog:"dlopen error: %s", dlError];
                }
                
                // Notify the user that we don't support removal of copy protection.
                [HBUtilities writeToActivityLog:"%s, library not found for decrypting physical disc", self.name.UTF8String];
                
                if (error) {
                    *error = [NSError errorWithDomain:@"HBErrorDomain"
                                                 code:101
                                             userInfo:@{ NSLocalizedDescriptionKey: @"library not found for decrypting physical disc" }];
                }
            }
        }

#ifdef __SANDBOX_ENABLED__
        token = nil;
#endif
    }

    return YES;
}

- (void)scanURLs:(NSArray<NSURL *> *)urls titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews hardwareDecoder:(BOOL)hardwareDecoder keepDuplicateTitles:(BOOL)keepDuplicateTitles progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    NSAssert(self.state == HBStateIdle, @"[HBCore scanURL:] called while another scan or encode already in progress");
    NSAssert(urls, @"[HBCore scanURL:] called with nil url.");

#ifdef __SANDBOX_ENABLED__
    __block NSMutableArray<HBSecurityAccessToken *> *tokens = [[NSMutableArray alloc] init];
    for (NSURL *url in urls)
    {
        [tokens addObject:[HBSecurityAccessToken tokenWithObject:url]];
    }
    self.cleanupHandler = ^{ tokens = nil; };
#endif

    // Reset the titles array
    self.titles = @[];

    // Copy the progress/completion blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateScanning;

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

    [self preventAutoSleep];

    hb_list_t *files_list = hb_list_init();
    for (NSURL *url in urls)
    {
        hb_list_add(files_list, (char *)url.fileSystemRepresentation);
    }

    hb_scan(_hb_handle, files_list,
              (int)index, (int)previewsNum,
              keepPreviews, min_title_duration_ticks,
              0, 0, NULL, hardwareDecoder ? HB_DECODE_SUPPORT_VIDEOTOOLBOX : 0, keepDuplicateTitles);

    hb_list_close(&files_list);

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.2];
}

/**
 *  Creates an array of lightweight HBTitles instances.
 */
- (HBCoreResult)scanDone
{
    hb_title_set_t *title_set = hb_get_title_set(_hb_handle);
    NSMutableArray *titles = [NSMutableArray array];

    for (int i = 0; i < hb_list_count(title_set->list_title); i++)
    {
        hb_title_t *title = (hb_title_t *) hb_list_item(title_set->list_title, i);
        [titles addObject:[[HBTitle alloc] initWithTitle:title handle:self.hb_handle featured:(title->index == title_set->feature)]];
    }

    self.titles = [titles copy];

    [HBUtilities writeToActivityLog:"%s scan done", self.name.UTF8String];

    HBCoreResult result = {0};
    result.code = (self.titles.count > 0) ? HBCoreResultCodeDone : HBCoreResultCodeUnknown;
    return result;
}

- (void)cancelScan
{
    hb_scan_stop(_hb_handle);
    [HBUtilities writeToActivityLog:"%s scan canceled", self.name.UTF8String];
}

#pragma mark - Preview images

- (nullable CGImageRef)copyImageAtIndex:(NSUInteger)index job:(HBJob *)job CF_RETURNS_RETAINED
{
    CGImageRef img = NULL;

    hb_job_t *hb_job = job.hb_job;
    hb_dict_t *job_dict = hb_job_to_dict(hb_job);
    hb_job_close(&hb_job);
    hb_image_t *image = hb_get_preview3(_hb_handle, (int)index, job_dict);

    if (image)
    {
        // Create an CGImageRef and copy the libhb image into it.
        // The image data returned by hb_get_preview3 is 4 bytes per pixel, BGRA format.
        // Alpha is ignored.
        CFMutableDataRef imgData = CFDataCreateMutable(kCFAllocatorDefault, 0);
        CFDataSetLength(imgData, 3 * image->width * image->height);

        UInt8 *src_line = image->data;
        UInt8 *dst = CFDataGetMutableBytePtr(imgData);
        for (int r = 0; r < image->height; r++)
        {
            UInt8 *src = src_line;
            for (int c = 0; c < image->width; c++)
            {
                *dst++ = src[2];
                *dst++ = src[1];
                *dst++ = src[0];
                src += 4;
            }
            src_line += image->plane[0].stride;
        }

        CGDataProviderRef provider = CGDataProviderCreateWithCFData(imgData);

        CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaNone;
        CGColorSpaceRef colorSpace = copyColorSpace(image->color_prim,
                                                    image->color_transfer,
                                                    image->color_matrix);

        img = CGImageCreate(image->width,
                            image->height,
                            8,
                            24,
                            image->width * 3,
                            colorSpace,
                            bitmapInfo,
                            provider,
                            NULL,
                            NO,
                            kCGRenderingIntentDefault);

        CGColorSpaceRelease(colorSpace);
        CGDataProviderRelease(provider);
        CFRelease(imgData);

        hb_image_close(&image);
    }

    hb_value_free(&job_dict);

    return img;
}

- (NSUInteger)imagesCountForTitle:(HBTitle *)title
{
    return title.hb_title->preview_count;
}

#pragma mark - Encodes

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    NSAssert(self.state == HBStateIdle, @"[HBCore encodeJob:] called while another scan or encode already in progress");
    NSAssert(job, @"[HBCore encodeJob:] called with nil job");

    // Copy the progress/completion blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateWorking;

#ifdef __SANDBOX_ENABLED__
    HBJob *jobCopy = [job copy];
    __block HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:jobCopy];
    self.cleanupHandler = ^{ token = nil; };
#endif

    // Add the job to libhb
    hb_job_t *hb_job = job.hb_job;
    hb_job_set_file(hb_job, job.destinationURL.fileSystemRepresentation);
    hb_add(_hb_handle, hb_job);

    // Free the job
    hb_job_close(&hb_job);

    [self preventAutoSleep];
    [self startProgressReporting:job.destinationURL];
    hb_start(_hb_handle);

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.5];

    [HBUtilities writeToActivityLog:"%s started encoding %s", self.name.UTF8String, job.destinationFileName.UTF8String];
    [HBUtilities writeToActivityLog:"%s with preset %s", self.name.UTF8String, job.presetName.UTF8String];
}

- (HBCoreResult)workDone
{
    [self stopProgressReporting];

    // HB_STATE_WORKDONE happens as a result of libhb finishing all its jobs
    // or someone calling hb_stop. In the latter case, hb_stop does not clear
    // out the remaining passes/jobs in the queue. We'll do that here.
    hb_job_t *job;
    while ((job = hb_job(_hb_handle, 0)))
    {
        hb_rem(_hb_handle, job);
    }

    HBCoreResult result = {_hb_state->param.working.rate_avg, (HBCoreResultCode)_hb_state->param.working.error};

    switch (result.code)
    {
        case HBCoreResultCodeDone:
            [HBUtilities writeToActivityLog:"%s work done", self.name.UTF8String];
            break;
        case HBCoreResultCodeCanceled:
            [HBUtilities writeToActivityLog:"%s work canceled", self.name.UTF8String];
            break;
        default:
            [HBUtilities writeToActivityLog:"%s work failed", self.name.UTF8String];
            break;
    }
    return result;
}

- (void)cancelEncode
{
    hb_stop(_hb_handle);
    [HBUtilities writeToActivityLog:"%s encode canceled", self.name.UTF8String];
}

- (void)pause
{
    hb_pause(_hb_handle);
    self.state = HBStatePaused;
    [self allowAutoSleep];
}

- (void)resume
{
    hb_resume(_hb_handle);
    self.state = HBStateWorking;
    [self preventAutoSleep];
}

#pragma mark - Progress

- (void)startProgressReporting:(NSURL *)fileURL
{
    if (fileURL)
    {
        NSDictionary *userInfo = @{NSProgressFileURLKey : fileURL};

        self.progress = [[NSProgress alloc] initWithParent:nil userInfo:userInfo];
        self.progress.totalUnitCount = 100;
        self.progress.kind = NSProgressKindFile;
        self.progress.pausable = NO;
        self.progress.cancellable = NO;

        [self.progress publish];
    }
}

- (void)stopProgressReporting
{
    self.progress.completedUnitCount = 100;
    [self.progress unpublish];
    self.progress = nil;
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
        dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, _updateTimerQueue);
        if (timer)
        {
            dispatch_source_set_timer(timer, dispatch_walltime(NULL, 0), (uint64_t)(seconds * NSEC_PER_SEC), (uint64_t)(seconds * NSEC_PER_SEC / 10));
            dispatch_source_set_event_handler(timer, ^{
                [self updateState];
            });
            dispatch_resume(timer);
        }
        self.updateTimer = timer;
    }
}

/**
 * Stops the update timer.
 */
- (void)stopUpdateTimer
{
    if (self.updateTimer)
    {
        dispatch_source_cancel(self.updateTimer);
        self.updateTimer = NULL;
    }
}

/**
 * This method polls libhb continuously for state changes and processes them.
 * Additional processing for each state is performed in methods that start
 * with 'handle'.
 */
- (void)updateState
{
    hb_get_state(_hb_handle, _hb_state);

    if (_hb_state->state == HB_STATE_IDLE)
    {
        // Libhb reported HB_STATE_IDLE, so nothing interesting has happened.
        return;
    }

    // Update HBCore state to reflect the current state of libhb
    if (_state != _hb_state->state)
    {
        self.state = _hb_state->state;
    }

    // Call the handler for the current state
    if (_hb_state->state == HB_STATE_WORKDONE || _hb_state->state == HB_STATE_SCANDONE)
    {
        [self handleCompletion];
    }
    else
    {
        [self handleProgress];
    }
}

#pragma mark - Blocks callbacks

/**
 * Processes progress state information.
 */
- (void)handleProgress
{
    if (self.progressHandler)
    {
        hb_state_t state = *(self.hb_state);
        HBProgress progress = {0, 0, 0, 0};
        progress.percent = [self.stateFormatter stateToPercentComplete:state];

        if (state.state == HB_STATE_WORKING || state.state == HB_STATE_PAUSED)
        {
            progress.hours = state.param.working.hours;
            progress.minutes = state.param.working.minutes;
            progress.seconds = state.param.working.seconds;
        }

        NSString *info = [self.stateFormatter stateToString:state];

        self.progressHandler(self.state, progress, info);

        if (state.state != HB_STATE_SEARCHING &&
            self.progress.completedUnitCount < progress.percent * 100)
        {
            self.progress.completedUnitCount = progress.percent * 100;
        }
    }
}

/**
 * Processes completion state information.
 */
- (void)handleCompletion
{
    // Libhb reported HB_STATE_WORKDONE or HB_STATE_SCANDONE,
    // so nothing interesting will happen after this point, stop the timer.
    [self stopUpdateTimer];

    // Set the state to idle, because the update timer won't fire again.
    self.state = HBStateIdle;

    // Reallow system sleep.
    [self allowAutoSleep];

    // Call the completion block and clean ups the handlers
    self.progressHandler = nil;

#ifdef __SANDBOX_ENABLED__
    self.cleanupHandler();
    self.cleanupHandler = nil;
#endif

    HBCoreResult result = (_hb_state->state == HB_STATE_WORKDONE) ? [self workDone] : [self scanDone];
    [self runCompletionBlockAndCleanUpWithResult:result];
}

/**
 *  Runs the completion block and clean ups the internal blocks.
 *
 *  @param result the result to pass to the completion block.
 */
- (void)runCompletionBlockAndCleanUpWithResult:(HBCoreResult)result
{
    if (self.completionHandler)
    {
        // Retain the completion block, because it could be replaced
        // inside the same block.
        HBCoreCompletionHandler completionHandler = self.completionHandler;
        self.completionHandler = nil;
        completionHandler(result);
    }
}

@end
