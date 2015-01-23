/*  HBCore.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "hb.h"

@class HBJob;

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

typedef void (^HBCoreProgressHandler)(HBState state, hb_state_t hb_state);
typedef void (^HBCoreCompletionHandler)(BOOL success);

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
 * @param loggingLevel         the desired libhb logging level.
 */
- (instancetype)initWithLoggingLevel:(int)loggingLevel;

/**
 * Current state of HBCore.
 */
@property (nonatomic, readonly) HBState state;

/**
 * Pointer to a libhb handle used by this HBCore instance.
 */
@property (nonatomic, readonly) hb_handle_t *hb_handle;

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
- (BOOL)canScan:(NSURL *)url error:(NSError **)error;

/**
 *  Starts the asynchronous execution of a scan.
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
 */
- (void)cancelScan;

/**
 *  An array of HBTitles found by the latest scan.
 */
@property (nonatomic, readonly) NSArray *titles;

/**
 *  Starts an asynchronous encoding session with the passed job.
 *
 *  @param job                 the job to encode
 *  @param progressHandler     a block called periodically with the progress information.
 *  @param completionHandler   a block called with the scan result
 */
- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;

/**
 * Stops encoding session and releases resources.
 */
- (void)cancelEncode;

/**
 *  Pauses the encoding session.
 */
- (void)pause;

/**
 *  Resumes a paused encoding session.
 */
- (void)resume;

@end
