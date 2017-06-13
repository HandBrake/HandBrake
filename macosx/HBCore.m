/*  HBCore.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBCore.h"
#import "HBJob.h"
#import "HBJob+HBJobConversion.h"
#import "HBDVDDetector.h"
#import "HBUtilities.h"

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
    hb_global_close();
}

+ (void)registerErrorHandler:(void (^)(NSString *error))handler
{
    errorHandler = [handler copy];
    hb_register_error_handler(&hb_error_handler);
}

- (instancetype)init
{
    return [self initWithLogLevel:0 queue:dispatch_get_main_queue()];
}

- (instancetype)initWithLogLevel:(int)level queue:(dispatch_queue_t)queue
{
    self = [super init];
    if (self)
    {
        _name = @"HBCore";
        _automaticallyPreventSleep = YES;
        _state = HBStateIdle;
        _updateTimerQueue = queue;
        _titles = @[];

        _stateFormatter = [[HBStateFormatter alloc] init];
        _hb_state = malloc(sizeof(hb_state_t));
        bzero(_hb_state, sizeof(hb_state_t));
        _logLevel = level;

        _hb_handle = hb_init(level);
        if (!_hb_handle)
        {
            return nil;
        }
    }

    return self;
}

- (instancetype)initWithLogLevel:(int)level name:(NSString *)name
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

- (void)setLogLevel:(int)logLevel
{
    _logLevel = logLevel;
    hb_log_level_set(_hb_handle, logLevel);
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

- (BOOL)canScan:(NSURL *)url error:(NSError * __autoreleasing *)error
{
    NSAssert(url, @"[HBCore canScan:] called with nil url.");

#ifdef __SANDBOX_ENABLED__
    BOOL accessingSecurityScopedResource = [url startAccessingSecurityScopedResource];
#endif

    if (![[NSFileManager defaultManager] fileExistsAtPath:url.path]) {
        if (error) {
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
        }
        else if (detector.isVideoBluRay)
        {
            lib = dlopen("libaacs.dylib", RTLD_LAZY);
        }

        if (lib)
        {
            dlclose(lib);
            [HBUtilities writeToActivityLog:"%s library found for decrypting physical disc", self.name.UTF8String];
        }
        else
        {
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
    if (accessingSecurityScopedResource)
    {
        [url stopAccessingSecurityScopedResource];
    }
#endif

    return YES;
}

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    NSAssert(self.state == HBStateIdle, @"[HBCore scanURL:] called while another scan or encode already in progress");
    NSAssert(url, @"[HBCore scanURL:] called with nil url.");

#ifdef __SANDBOX_ENABLED__
    __block HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:url];
    self.cleanupHandler = ^{ token = nil; };
#endif

    // Reset the titles array
    self.titles = @[];

    // Copy the progress/completion blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

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

    hb_scan(_hb_handle, url.path.fileSystemRepresentation,
            (int)index, (int)previewsNum,
            1, min_title_duration_ticks);

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.2];

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateScanning;
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

    return (self.titles.count > 0) ? HBCoreResultDone : HBCoreResultFailed;
}

- (void)cancelScan
{
    hb_scan_stop(_hb_handle);
    [HBUtilities writeToActivityLog:"%s scan cancelled", self.name.UTF8String];
}

#pragma mark - Preview images

- (CGImageRef)CGImageRotatedByAngle:(CGImageRef)imgRef angle:(CGFloat)angle flipped:(BOOL)flipped CF_RETURNS_RETAINED
{
    CGFloat angleInRadians = angle * (M_PI / 180);
    CGFloat width = CGImageGetWidth(imgRef);
    CGFloat height = CGImageGetHeight(imgRef);

    CGRect imgRect = CGRectMake(0, 0, width, height);
    CGAffineTransform transform = CGAffineTransformMakeRotation(angleInRadians);
    CGRect rotatedRect = CGRectApplyAffineTransform(imgRect, transform);

    CGColorSpaceRef colorSpace = CGImageGetColorSpace(imgRef);
    CGContextRef bmContext = CGBitmapContextCreate(NULL,
                                                   (size_t)rotatedRect.size.width,
                                                   (size_t)rotatedRect.size.height,
                                                   8,
                                                   0,
                                                   colorSpace,
                                                   kCGImageAlphaPremultipliedFirst);
    CGContextSetAllowsAntialiasing(bmContext, FALSE);
    CGContextSetInterpolationQuality(bmContext, kCGInterpolationNone);

    // Rotate
    CGContextTranslateCTM(bmContext,
                          + (rotatedRect.size.width / 2),
                          + (rotatedRect.size.height / 2));
    CGContextRotateCTM(bmContext, -angleInRadians);
    CGContextTranslateCTM(bmContext,
                          - (rotatedRect.size.width / 2),
                          - (rotatedRect.size.height / 2));

    // Flip
    if (flipped)
    {
        CGAffineTransform flipHorizontal = CGAffineTransformMake(-1, 0, 0, 1, floor(rotatedRect.size.width), 0);
        CGContextConcatCTM(bmContext, flipHorizontal);
    }

    CGContextDrawImage(bmContext,
                       CGRectMake((rotatedRect.size.width - width)/2.0f,
                                  (rotatedRect.size.height - height)/2.0f,
                                  width,
                                  height),
                       imgRef);

    CGImageRef rotatedImage = CGBitmapContextCreateImage(bmContext);
    CFRelease(bmContext);

    return rotatedImage;
}

- (CGColorSpaceRef)copyColorSpaceWithColorPrimaries:(int)colorPrimaries
{
    const CGFloat whitePoint[] = {0.95047, 1.0, 1.08883};
    const CGFloat blackPoint[] = {0, 0, 0};

    // See https://developer.apple.com/library/content/technotes/tn2257/_index.html
    const CGFloat gamma[] = {1.961, 1.961, 1.961};

    // RGB/XYZ Matrices (D65 white point)
    switch (colorPrimaries) {
        case HB_COLR_PRI_EBUTECH:
        {
            // Rec. 601, 625 line
            const CGFloat matrix[] = {0.4305538, 0.2220043, 0.0201822,
                                      0.3415498, 0.7066548, 0.1295534,
                                      0.1783523, 0.0713409, 0.9393222};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_SMPTEC:
        {
            // Rec. 601, 525 line
            const CGFloat matrix[] = {0.3935209, 0.2123764, 0.0187391,
                                      0.3652581, 0.7010599, 0.1119339,
                                      0.1916769, 0.0865638, 0.9583847};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_BT2020:
        {
            // Rec. 2020
            const CGFloat matrix[] = {0.6369580, 0.2627002, 0.0000000,
                                      0.1446169, 0.6779981, 0.0280727,
                                      0.1688810, 0.0593017, 1.0609851};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_BT709:
        default:
        {
            // Rec. 709
            const CGFloat matrix[] = {0.4124564, 0.2126729, 0.0193339,
                                      0.3575761, 0.7151522, 0.1191920,
                                      0.1804375, 0.0721750, 0.9503041};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
    }
}

- (CGColorSpaceRef)copyColorSpaceWithPrimaries:(int)primaries transfer:(int)transfer matrix:(int)matrix
{
    if (NSAppKitVersionNumber < NSAppKitVersionNumber10_11)
    {
        return [self copyColorSpaceWithColorPrimaries:primaries];
    }
    
    CFStringRef primariesKey = NULL;
    switch (primaries)
    {
        case HB_COLR_PRI_EBUTECH:
            primariesKey = kCVImageBufferColorPrimaries_EBU_3213;
            break;

        case HB_COLR_PRI_SMPTEC:
            primariesKey = kCVImageBufferColorPrimaries_SMPTE_C;
            break;

        case HB_COLR_PRI_BT2020:
            primariesKey = kCVImageBufferColorPrimaries_ITU_R_2020;
            break;

        case HB_COLR_PRI_BT709:
        default:
            primariesKey = kCVImageBufferColorPrimaries_ITU_R_709_2;
    }

    CFStringRef transferKey = NULL;
    switch (transfer)
    {
        case HB_COLR_TRA_SMPTE240M:
            transferKey = kCVImageBufferTransferFunction_SMPTE_240M_1995;
            break;

        case HB_COLR_TRA_BT2020_10:
        case HB_COLR_TRA_BT2020_12:
            transferKey = kCVImageBufferTransferFunction_ITU_R_2020;
            break;

        case HB_COLR_TRA_BT709:
        default:
            transferKey = kCVImageBufferTransferFunction_ITU_R_709_2;
    }

    CFStringRef matrixKey = NULL;
    switch (matrix)
    {
        case HB_COLR_MAT_SMPTE170M:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_601_4;
            break;

        case HB_COLR_MAT_SMPTE240M:
            matrixKey = kCVImageBufferYCbCrMatrix_SMPTE_240M_1995;
            break;

        case HB_COLR_MAT_BT2020_NCL:
        case HB_COLR_MAT_BT2020_CL:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_2020;
            break;

        case HB_COLR_MAT_BT709:
        default:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_709_2;;
    }

    const void *keys[3] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey, kCVImageBufferYCbCrMatrixKey };
    const void *values[3] = { primariesKey, transferKey, matrixKey};
    CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CGColorSpaceRef colorSpace = CVImageBufferCreateColorSpaceFromAttachments(attachments);

    CFRelease(attachments);

    return colorSpace;
}
        
- (CGImageRef)copyImageAtIndex:(NSUInteger)index
                      forTitle:(HBTitle *)title
                  pictureFrame:(HBPicture *)frame
                   deinterlace:(BOOL)deinterlace
                        rotate:(int)angle
                       flipped:(BOOL)flipped CF_RETURNS_RETAINED
{
    CGImageRef img = NULL;

    hb_geometry_settings_t geo;
    memset(&geo, 0, sizeof(geo));
    geo.geometry.width = frame.displayWidth;
    geo.geometry.height = frame.height;
    // ignore the par.
    geo.geometry.par.num = 1;
    geo.geometry.par.den = 1;
    int crop[4] = {frame.cropTop, frame.cropBottom, frame.cropLeft, frame.cropRight};
    memcpy(geo.crop, crop, sizeof(int[4]));

    hb_image_t *image = hb_get_preview2(_hb_handle, title.index, (int)index, &geo, deinterlace);

    if (image)
    {
        // Create an CGImageRef and copy the libhb image into it.
        // The image data returned by hb_get_preview2 is 4 bytes per pixel, BGRA format.
        // Alpha is ignored.
        CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaNone;
        CFMutableDataRef imgData = CFDataCreateMutable(kCFAllocatorDefault, 3 * image->width * image->height);
        CGDataProviderRef provider = CGDataProviderCreateWithCFData(imgData);
        CGColorSpaceRef colorSpace = [self copyColorSpaceWithPrimaries:title.hb_title->color_prim
                                                              transfer:title.hb_title->color_transfer
                                                                matrix:title.hb_title->color_matrix];

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

        hb_image_close(&image);
    }

    if (angle || flipped)
    {
        CGImageRef rotatedImg = [self CGImageRotatedByAngle:img angle:angle flipped:flipped];
        CGImageRelease(img);
        return rotatedImg;
    }
    else
    {
        return img;
    }
}

- (NSUInteger)imagesCountForTitle:(HBTitle *)title
{
    return title.hb_title->preview_count;
}

#pragma mark - Encodes

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;
{
    NSAssert(self.state == HBStateIdle, @"[HBCore encodeJob:] called while another scan or encode already in progress");
    NSAssert(job, @"[HBCore encodeJob:] called with nil job");

    // Copy the progress/completion blocks
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

#ifdef __SANDBOX_ENABLED__
    HBJob *jobCopy = [job copy];
    __block HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:jobCopy];
    self.cleanupHandler = ^{ token = nil; };
#endif

    // Add the job to libhb
    hb_job_t *hb_job = job.hb_job;
    hb_job_set_file(hb_job, job.completeOutputURL.path.fileSystemRepresentation);
    hb_add(_hb_handle, hb_job);

    // Free the job
    hb_job_close(&hb_job);

    [self preventAutoSleep];

    hb_start(_hb_handle);

    // Start the timer to handle libhb state changes
    [self startUpdateTimerWithInterval:0.5];

    // Set the state, so the UI can be update
    // to reflect the current state instead of
    // waiting for libhb to set it in a background thread.
    self.state = HBStateWorking;

    [HBUtilities writeToActivityLog:"%s started encoding %s", self.name.UTF8String, job.outputFileName.UTF8String];
    [HBUtilities writeToActivityLog:"%s with preset %s", self.name.UTF8String, job.presetName.UTF8String];
}

- (HBCoreResult)workDone
{
    // HB_STATE_WORKDONE happpens as a result of libhb finishing all its jobs
    // or someone calling hb_stop. In the latter case, hb_stop does not clear
    // out the remaining passes/jobs in the queue. We'll do that here.
    hb_job_t *job;
    while ((job = hb_job(_hb_handle, 0)))
    {
        hb_rem(_hb_handle, job);
    }

    HBCoreResult result = HBCoreResultDone;
    switch (_hb_state->param.workdone.error)
    {
        case HB_ERROR_NONE:
            result = HBCoreResultDone;
            [HBUtilities writeToActivityLog:"%s work done", self.name.UTF8String];
            break;
        case HB_ERROR_CANCELED:
            result = HBCoreResultCancelled;
            [HBUtilities writeToActivityLog:"%s work canceled", self.name.UTF8String];
            break;
        default:
            result = HBCoreResultFailed;
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
        dispatch_release(self.updateTimer);
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
    self.state = _hb_state->state;

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
