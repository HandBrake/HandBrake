/*  HBCore.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

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

// These constants specify various status notifications sent by HBCore
extern NSString *HBCoreScanningNotification;
extern NSString *HBCoreScanDoneNotification;
extern NSString *HBCoreSearchingNotification;
extern NSString *HBCoreWorkingNotification;
extern NSString *HBCorePausedNotification;
extern NSString *HBCoreWorkDoneNotification;
extern NSString *HBCoreMuxingNotification;

/**
 * HBCore is an Objective-C interface to the low-level HandBrake library.
 * HBCore monitors state changes of libhb and provides notifications via
 * NSNotificationCenter to any object who needs them. It can also be used
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
 *  Performs the final cleanup for the process.
 */
+ (void)closeGlobal;

/**
 * Opens low level HandBrake library. This should be called once before other
 * functions HBCore are used.
 *
 * @param loggingLevel         the desired libhb logging level.
 *
 * @return YES if libhb was opened, NO if there was an error.
 */
- (instancetype)initWithLoggingLevel:(int)loggingLevel;

/**
 * Current state of HBCore.
 */
@property (nonatomic, readonly) HBState state;

/**
 * Pointer to a hb_state_s struct containing state information of libhb.
 */
@property (nonatomic, readonly) hb_state_t *hb_state;

/**
 * Pointer to a libhb handle used by this HBCore instance.
 */
@property (nonatomic, readonly) hb_handle_t *hb_handle;

/**
 *  The name of the core, used in for debugging purpose.
 */
@property (nonatomic, copy) NSString *name;

/**
 *  Determines whether the scan operation can scan a particural URL or whether an additional decription lib is needed..
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
 *  @param url              the URL of the input file.
 *  @param titleNum         the number of the desired title. Use 0 to scan every title.
 *  @param previewsNum      the number of previews image to generate.
 *  @param minTitleDuration the minimum duration of the wanted titles in seconds.
 */
- (void)scan:(NSURL *)url titleNum:(NSUInteger)titleNum previewsNum:(NSUInteger)previewsNum minTitleDuration:(NSUInteger)minTitleDuration;

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
 *  @param job the job to encode.
 */
- (void)encodeJob:(HBJob *)job;

/**
 * Starts the libhb encoding session.
 *
 *	This method must be called after all jobs have been added.
 */
- (void)start;

/**
 * Stops encoding session and releases resources.
 */
- (void)stop;

/**
 *  Pauses the encoding session.
 */
- (void)pause;

/**
 *  Resumes a paused encoding session.
 */
- (void)resume;

@end
