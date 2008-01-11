/**
 * @file
 * Implementation of class HBCore.
 */

#import "HBCore.h"
#include "hb.h"

// These constants specify the current state of HBCore.

const NSString *HBStateIdle = @"HBStateIdle";           ///< HB is doing nothing (HB_STATE_IDLE)
const NSString *HBStateScanning = @"HBStateScanning";   ///< HB is scanning (HB_STATE_SCANNING)
const NSString *HBStateScanDone = @"HBStateScanDone";   ///< Scanning has been completed (HB_STATE_SCANDONE)
const NSString *HBStateWorking = @"HBStateWorking";     ///< HB is encoding (HB_STATE_WORKING)
const NSString *HBStatePaused = @"HBStatePaused";       ///< Encoding is paused (HB_STATE_PAUSED)
const NSString *HBStateWorkDone = @"HBStateWorkDone";   ///< Encoding has been completed (HB_STATE_WORKDONE)
const NSString *HBStateMuxing = @"HBStateMuxing";       ///< HB is muxing (HB_STATE_MUXING)


// These constants specify various status notifications sent by HBCore

/// Notification sent to update status while scanning. Matches HB_STATE_SCANNING constant in libhb.
NSString *HBCoreScanningNotification = @"HBCoreScanningNotification";

/// Notification sent after scanning is complete. Matches HB_STATE_SCANDONE constant in libhb.
NSString *HBCoreScanDoneNotification = @"HBCoreScanDoneNotification";

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
@interface HBCore (Private)
- (NSString *)stateAsString:(int)stateValue;
@end

@implementation HBCore

/**
 * Initializes HBCore.
 */
- (id)init
{
    if (self = [super init])
    {
        state = HBStateIdle;    
        hb_state = malloc(sizeof(struct hb_state_s));   
    }
    return self;
}

/**
 * Releases resources.
 */
- (void)dealloc
{
    free(hb_state);    
    [super dealloc];
}

/**
 * Opens low level HandBrake library. This should be called once before other
 * functions HBCore are used.
 *
 * @param debugMode         If set to YES, libhb will print verbose debug output.
 * @param checkForUpdates   If set to YES, libhb checks for updated versions.
 *
 * @return YES if libhb was opened, NO if there was an error.
 */
- (BOOL)openInDebugMode:(BOOL)debugMode checkForUpdates:(BOOL)checkForUpdates;
{
    NSAssert(!hb_handle, @"[HBCore openInDebugMode:checkForUpdates:] libhb is already open");
    if (hb_handle)
        return NO;

    state = HBStateIdle;    

    hb_handle = hb_init(debugMode ? HB_DEBUG_ALL : HB_DEBUG_NONE, checkForUpdates);
    if (!hb_handle)
        return NO;

    updateTimer = [[NSTimer scheduledTimerWithTimeInterval:0.5
                                                    target:self
                                                  selector:@selector(stateUpdateTimer:) 
                                                  userInfo:NULL 
                                                   repeats:YES] retain];

    [[NSRunLoop currentRunLoop] addTimer:updateTimer forMode:NSEventTrackingRunLoopMode];        
    return YES;
}

/**
 * Closes low level HandBrake library and releases resources.
 *
 * @return YES if libhb was closed successfully, NO if there was an error.
 */
- (BOOL)close
{
    NSAssert(hb_handle, @"[HBCore close] libhb is not open");
    if (!hb_handle)
        return NO;
        
    [updateTimer invalidate];
    [updateTimer release];
    updateTimer = nil;
    hb_close(&hb_handle);
    hb_handle = NULL;
    return YES;
}

/**
 * Returns libhb handle used by this HBCore instance.
 */ 
- (struct hb_handle_s *)hb_handle
{
    return hb_handle;
}

/**
 * Returns current state of HBCore.
 *
 * @return One of the HBState* string constants.
 */
- (const NSString *)state
{
    return state;
}

/**
 * Returns latest hb_state_s information struct returned by libhb.
 *
 * @return Pointer to a hb_state_s struct containing state information of libhb.
 */
- (const struct hb_state_s *)hb_state
{
    return hb_state;
}

@end 

@implementation HBCore (Private)

/**
 * Transforms a libhb state constant to a matching HBCore state constant.
 */
- (const NSString *)stateAsString:(int)stateValue
{
    switch (stateValue)
    {
        case HB_STATE_IDLE:
            return HBStateIdle;        
        case HB_STATE_SCANNING:
            return HBStateScanning;
        case HB_STATE_SCANDONE:
            return HBStateScanDone;
        case HB_STATE_WORKING:
            return HBStateWorking;
        case HB_STATE_PAUSED:
            return HBStatePaused;
        case HB_STATE_WORKDONE:
            return HBStateWorkDone;
        case HB_STATE_MUXING:
            return HBStateMuxing;        
        default:
            NSAssert1(NO, @"[HBCore stateAsString:] unknown state %d", stateValue);
            return nil;
    }
}

/**
 * This method polls libhb continuously for state changes and processes them.
 * Additional processing for each state is performed in methods that start
 * with 'handle' (e.g. handleHBStateScanning).
 */
- (void)stateUpdateTimer:(NSTimer *)timer
{
    if (!hb_handle)
    {
        // Libhb is not open so we cannot do anything.
        return;
    }

    hb_get_state(hb_handle, hb_state);

    if (hb_state->state == HB_STATE_IDLE)
    {
        // Libhb reported HB_STATE_IDLE, so nothing interesting has happened.
        return;
    }
        
    // Update HBCore state to reflect the current state of libhb
    NSString *newState = [self stateAsString:hb_state->state];
    if (newState != state)
    {
        [self willChangeValueForKey:@"state"];
        state = newState;
        [self didChangeValueForKey:@"state"];
    }

    // Determine name of the method that does further processing for this state
    // and call it. 
    SEL sel = NSSelectorFromString([NSString stringWithFormat:@"handle%@", state]);
    if ([self respondsToSelector:sel])
        [self performSelector:sel];
}

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

@end
