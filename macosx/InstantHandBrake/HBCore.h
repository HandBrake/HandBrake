/**
 * @file
 * Interface of class HBCore.
 */

#import <Cocoa/Cocoa.h>

extern const NSString *HBStateIdle;
extern const NSString *HBStateScanning;
extern const NSString *HBStateScanDone;
extern const NSString *HBStateWorking;
extern const NSString *HBStatePaused;
extern const NSString *HBStateWorkDone;
extern const NSString *HBStateMuxing;
extern const NSString *HBStateAll;

extern NSString *HBCoreScanningNotification;
extern NSString *HBCoreScanDoneNotification;
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
{
    /// Pointer to libhb handle.
    struct hb_handle_s *hb_handle;
    
    /// Pointer to latest state information returned by libhb.
    struct hb_state_s *hb_state;
    
    /// Timer used to poll libhb for state changes.
    NSTimer *updateTimer;

    /// Current state of HBCore; one of the HBState* constants.
    const NSString *state;  
}

- (id)init;
- (BOOL)openInDebugMode:(BOOL)debugMode checkForUpdates:(BOOL)checkForUpdates;
- (BOOL)close;
- (void)removeAllJobs;
- (NSString *)state;
- (struct hb_handle_s *)hb_handle;
- (const struct hb_state_s *)hb_state;

@end
