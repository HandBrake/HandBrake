/*  HBCore.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "hb.h"

@class HBJob;
@class HBPicture;
@class HBTitle;

NS_ASSUME_NONNULL_BEGIN

// These constants specify the current state of HBCore.
typedef NS_ENUM(NSUInteger, HBState) {
    HBStateIdle      = HB_STATE_IDLE,        ///< HB is doing nothing
    HBStateScanning  = HB_STATE_SCANNING,    ///< HB is scanning
    HBStateScanDone  = HB_STATE_SCANDONE,    ///< Scanning has been completed
    HBStateWorking   = HB_STATE_WORKING,     ///< HB is encoding
    HBStatePaused    = HB_STATE_PAUSED,      ///< Encoding is paused
    HBStateWorkDone  = HB_STATE_WORKDONE,    ///< Encoding has been completed
    HBStateMuxing    = HB_STATE_MUXING,      ///< HB is muxing
    HBStateSearching = HB_STATE_SEARCHING    ///< HB is searching
};

// These constants specify the result of a scan or encode.
typedef NS_ENUM(NSUInteger, HBCoreResult) {
    HBCoreResultDone,
    HBCoreResultCancelled,
    HBCoreResultFailed,
};

typedef void (^HBCoreProgressHandler)(HBState state, hb_state_t hb_state);
typedef void (^HBCoreCompletionHandler)(HBCoreResult result);

/**
 * HBCore is an Objective-C interface to the low-level HandBrake library.
 * HBCore monitors state changes of libhb. It can also be used
 * to implement properties that can be directly bound to elements of the gui.
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
 */
- (instancetype)initWithLogLevel:(int)level NS_DESIGNATED_INITIALIZER;

/**
 *  Opens low level HandBrake library. This should be called once before other
 *  functions HBCore are used.
 *
 *  @param level the desired libhb logging level
 *  @param name  the instance debug name
 */
- (instancetype)initWithLogLevel:(int)level name:(NSString *)name;

/**
 *  Log level.
 */
@property (nonatomic, readwrite) int logLevel;

/**
 * Current state of HBCore.
 */
@property (nonatomic, readonly) HBState state;

/**
 *  The name of the core, used for debugging purpose.
 */
@property (nonatomic, copy) NSString *name;

/**
 *  Determines whether the scan operation can scan a particural URL or whether an additional decryption lib is needed.
 *
 *  @param url   the URL of the input file.
 *  @param error an error containing additional info.
 *
 *  @return YES is the file at URL is scannable.
 */
- (BOOL)canScan:(NSURL *)url error:(NSError * __autoreleasing *)error;

/**
 *  Initiates an asynchronous scan operation and returns immediately.
 *
 *  @param url                 the URL of the input file.
 *  @param index            the index of the desired title. Use 0 to scan every title.
 *  @param previewsNum         the number of previews image to generate.
 *  @param seconds             the minimum duration of the wanted titles in seconds.
 *  @param progressHandler     a block called periodically with the progress information.
 *  @param completionHandler   a block called with the scan result.
 */
- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;

/**
 *  Cancels the scan execution.
 *  Cancel can be invoked when the scan is running.
 */
- (void)cancelScan;

/**
 *  An array of HBTitles found by the latest scan.
 */
@property (nonatomic, readonly, nullable) NSArray<HBTitle *> *titles;

/**
 *  This function converts an image created by libhb (specified via index)
 *  into an CGImage.
 *
 *  @param index       the index of the desired image.
 *  @param title       Handle to hb_title_t of desired title
 *  @param frame       a HBPicture instance that describe the image's frame.
 *  @param deinterlace whether the preview image must be deinterlaced or not.
 *
 *  @return a CGImageRef of the wanted image, NULL if the index is out of bounds.
 */
- (nullable CGImageRef)copyImageAtIndex:(NSUInteger)index
                               forTitle:(HBTitle *)title
                           pictureFrame:(HBPicture *)frame
                            deinterlace:(BOOL)deinterlace CF_RETURNS_RETAINED;

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
