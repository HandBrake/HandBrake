/*  HBCore.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

@class HBJob;
@class HBPicture;
@class HBTitle;
@class HBStateFormatter;

NS_ASSUME_NONNULL_BEGIN

struct HBProgress
{
    double percent;

    int   hours;
    int   minutes;
    int   seconds;
};
typedef struct HBProgress HBProgress;

// These constants specify the current state of HBCore.
typedef NS_ENUM(NSUInteger, HBState) {
    HBStateIdle      = 1,       ///< HB is doing nothing
    HBStateScanning  = 2,       ///< HB is scanning
    HBStateScanDone  = 4,       ///< Scanning has been completed
    HBStateWorking   = 8,       ///< HB is encoding
    HBStatePaused    = 16,      ///< Encoding is paused
    HBStateWorkDone  = 32,      ///< Encoding has been completed
    HBStateMuxing    = 64,      ///< HB is muxing
    HBStateSearching = 128      ///< HB is searching
};

// These constants specify the result of a scan or encode.
typedef NS_ENUM(NSUInteger, HBCoreResultCode) {
    HBCoreResultCodeDone       = 0,
    HBCoreResultCodeCanceled   = 1,
    HBCoreResultCodeWrongInput = 2,
    HBCoreResultCodeInit       = 3,
    HBCoreResultCodeUnknown    = 4,
    HBCoreResultCodeRead       = 5
};

struct HBCoreResult {
    float            avgFps;
    HBCoreResultCode code;
};
typedef struct HBCoreResult HBCoreResult;

typedef void (^HBCoreProgressHandler)(HBState state, HBProgress progress, NSString *info);
typedef void (^HBCoreCompletionHandler)(HBCoreResult result);

/**
 * HBCore is an Objective-C interface to the low-level HandBrake library.
 * HBCore monitors state changes of libhb. It can also be used
 * to implement properties that can be directly bound to elements of the gui.
 *
 * Instance methods must be called on the same queue as the queue
 * passed to initWithLogLevel:queue:
 * Convenience inits use the main queue by default.
 *
 * copyImageAtIndex: can be called on a different queue,
 * but the caller must ensure the validity of the title.
 */
@interface HBCore : NSObject

/**
 * Set the status of libdvdnav in low level HandBrake library.
 * This should be called once before other functions HBCore are used.
 *
 * @param enabled         whether libdvdnav is enabled or not.
 */
+ (void)setDVDNav:(BOOL)enabled;

/**
 *  Inits libhb globals.
 */
+ (void)initGlobal;

/**
 *  Performs the final cleanup for the process.
 */
+ (void)closeGlobal;

/**
 *  Clear temporary files (for example analyze pass stats).
 */
+ (void)cleanTemporaryFiles;

/**
 *  Registers a global error handler block.
 *
 *  @param handler a block called with the error message.
 */
+ (void)registerErrorHandler:(void (^)(NSString *error))handler;

/**
 * Opens low level HandBrake library. This should be called once before other
 * functions HBCore are used.
 *
 * @param level         the desired libhb logging level.
 * @param queue         the queue on which the callbacks will be called.
 */
- (instancetype)initWithLogLevel:(NSInteger)level queue:(dispatch_queue_t)queue NS_DESIGNATED_INITIALIZER;

/**
 *  Opens low level HandBrake library. This should be called once before other
 *  functions HBCore are used.
 *
 *  @param level the desired libhb logging level
 *  @param name  the instance debug name
 */
- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name;

/**
 *  Log level.
 */
@property (nonatomic, readwrite) NSInteger logLevel;

/**
 * Set whether system sleep will be disable or not during a scan/encode
 * Enabled by default.
 */
@property (nonatomic, readwrite) BOOL automaticallyPreventSleep;

/**
 * Manually prevent system sleep if automaticallyPreventSleep is set to NO.
 */
- (void)preventSleep;

/**
 * Manually allow system sleep if automaticallyPreventSleep is set to NO.
 */
- (void)allowSleep;

/**
 *  State formatter.
 */
@property (nonatomic, readwrite, strong) HBStateFormatter *stateFormatter;

/**
 * Current state of HBCore.
 */
@property (nonatomic, readonly) HBState state;

/**
 *  The name of the core, used for debugging purpose.
 */
@property (nonatomic, copy) NSString *name;

/**
 *  Determines whether the scan operation can scan a particular URL or whether an additional decryption lib is needed.
 *
 *  @param urls   the URLs of the input files.
 *  @param error an error containing additional info.
 *
 *  @return YES is the file at URL is scannable.
 */
- (BOOL)canScan:(NSArray<NSURL *> *)urls error:(NSError * __autoreleasing *)error;

/**
 *  Initiates an asynchronous scan operation and returns immediately.
 *
 *  @param urls            the URLs of the input files.
 *  @param index            the index of the desired title. Use 0 to scan every title.
 *  @param previewsNum         the number of previews image to generate.
 *  @param seconds             the minimum duration of the wanted titles in seconds.
 *  @param keepPreviews        whether the previews images are kept on disk or discarded.
 *  @param progressHandler     a block called periodically with the progress information.
 *  @param completionHandler   a block called with the scan result.
 */
- (void)scanURLs:(NSArray<NSURL *> *)urls titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews hardwareDecoder:(BOOL)hardwareDecoder progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;

/**
 *  Cancels the scan execution.
 *  Cancel can be invoked when the scan is running.
 */
- (void)cancelScan;

/**
 *  An array of HBTitles found by the latest scan.
 */
@property (nonatomic, readonly, copy) NSArray<HBTitle *> *titles;

/**
 *  This function converts an image created by libhb (specified via index)
 *  into an CGImage.
 *
 *  @param index       the index of the desired image.
 *  @param job           a HBJob instance.
 *
 *  @return a CGImageRef of the wanted image, NULL if the index is out of bounds.
 */
- (nullable CGImageRef)copyImageAtIndex:(NSUInteger)index job:(HBJob *)job CF_RETURNS_RETAINED;

/**
 *  Returns the counts of the available previews images.
 */
- (NSUInteger)imagesCountForTitle:(HBTitle *)title;

/**
 *  Initiates an asynchronous encode operation and returns immediately.
 *
 *  @param job                 the job to encode
 *  @param progressHandler     a block called periodically with the progress information.
 *  @param completionHandler   a block called with the scan result
 */
- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;

/**
 *  Stops encode operation and releases resources.
 *  Cancel can be invoked when the encode is running.
 */
- (void)cancelEncode;

/**
 *  Pauses the encode operation.
 *  Pause can be invoked when the encode is running.
 */
- (void)pause;

/**
 *  Resumes a paused encoding session.
 *  Resume can be invoked when the encode is running.
 */
- (void)resume;

@end

NS_ASSUME_NONNULL_END
