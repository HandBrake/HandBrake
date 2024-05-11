/*  HBQueueWorker.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueWorker.h"

#import "HBRemoteCore.h"
#import "HBJobOutputFileWriter.h"
#import "HBPreferencesKeys.h"

static void *HBQueueWorkerContext = &HBQueueWorkerContext;
static void *HBQueueWorkerLogLevelContext = &HBQueueWorkerLogLevelContext;

NSString * const HBQueueWorkerDidChangeStateNotification = @"HBQueueWorkerDidChangeStateNotification";

NSString * const HBQueueWorkerProgressNotification = @"HBQueueWorkerProgressNotification";
NSString * const HBQueueWorkerProgressNotificationPercentKey = @"HBQueueWorkerProgressNotificationPercentKey";
NSString * const HBQueueWorkerProgressNotificationHoursKey = @"HBQueueWorkerProgressNotificationHoursKey";
NSString * const HBQueueWorkerProgressNotificationMinutesKey = @"HBQueueWorkerProgressNotificationMinutesKey";
NSString * const HBQueueWorkerProgressNotificationSecondsKey = @"HBQueueWorkerProgressNotificationSecondsKey";
NSString * const HBQueueWorkerProgressNotificationInfoKey = @"HBQueueWorkerProgressNotificationInfoKey";

NSString * const HBQueueWorkerDidStartItemNotification = @"HBQueueWorkerDidStartItemNotification";
NSString * const HBQueueWorkerDidCompleteItemNotification = @"HBQueueWorkerDidCompleteItemNotification";
NSString * const HBQueueWorkerItemNotificationItemKey = @"HBQueueWorkerItemNotificationItemKey";

@interface HBQueueWorker ()

@property (nonatomic, readonly) HBRemoteCore *core;

@property (nonatomic, nullable) HBQueueJobItem *item;
@property (nonatomic, nullable) HBJobOutputFileWriter *currentLog;

@end

@implementation HBQueueWorker

- (instancetype)initWithXPCServiceName:(NSString *)serviceName
{
    self = [super init];
    if (self)
    {
        NSInteger loggingLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
        _core = [[HBRemoteCore alloc] initWithLogLevel:loggingLevel name:serviceName serviceName:serviceName];

        // Set up observers
        [self.core addObserver:self forKeyPath:@"state"
                       options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                       context:HBQueueWorkerContext];

        [NSUserDefaultsController.sharedUserDefaultsController addObserver:self forKeyPath:@"values.LoggingLevel"
                                                                    options:0 context:HBQueueWorkerLogLevelContext];
    }
    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBQueueWorkerContext)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidChangeStateNotification object:self];
    }
    else if (context == HBQueueWorkerLogLevelContext)
    {
        self.core.logLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc
{
    [self.core removeObserver:self forKeyPath:@"state" context:HBQueueWorkerContext];
    [NSUserDefaultsController.sharedUserDefaultsController removeObserver:self forKeyPath:@"values.LoggingLevel" context:HBQueueWorkerLogLevelContext];
    [self.core invalidate];
}

- (void)invalidate
{
    [self.core invalidate];
}

- (BOOL)canEncode
{
    return self.item == nil;
}

- (BOOL)isEncoding
{
    return self.item != nil;
}

- (BOOL)canPause
{
    HBState s = self.core.state;
    return (s == HBStateWorking || s == HBStateMuxing);
}

- (void)pause
{
    [self.item pausedAtDate:[NSDate date]];
    [self.core pause];
}

- (BOOL)canResume
{
    return self.core.state == HBStatePaused;
}

- (void)resume
{
    [self.item resumedAtDate:[NSDate date]];
    [self.core resume];
}

- (void)completedItem:(HBQueueJobItem *)item result:(HBCoreResult)result
{
    NSParameterAssert(item);

    item.endedDate = [NSDate date];

    // Since we are done with this encode, tell output to stop writing to the
    // individual encode log.
    [self.core.stderrRedirect removeListener:self.currentLog];
    [self.core.stdoutRedirect removeListener:self.currentLog];

    self.currentLog = nil;

    // Mark the encode just finished
    [self.item setDoneWithResult:result];
    self.item = nil;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @1.0,
                                                               HBQueueWorkerProgressNotificationInfoKey: @""}];

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidCompleteItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerItemNotificationItemKey: item}];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)encodeItem:(HBQueueJobItem *)item
{
    NSParameterAssert(item);

    // now we mark the queue item as working so another instance can not come along and try to scan it while we are scanning
    item.startedDate = [NSDate date];
    item.state = HBQueueItemStateWorking;

    self.item = item;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidStartItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerItemNotificationItemKey: item}];

    // Tell HB to output a new activity log file for this encode
    self.currentLog = [[HBJobOutputFileWriter alloc] initWithJob:item.job];
    if (self.currentLog)
    {
        item.activityLogURL = self.currentLog.url;

        dispatch_queue_t mainQueue = dispatch_get_main_queue();
        [self.core.stderrRedirect addListener:self.currentLog queue:mainQueue];
        [self.core.stdoutRedirect addListener:self.currentLog queue:mainQueue];
    }

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                          object:self
                                                        userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @0,
                                                                   HBQueueWorkerProgressNotificationInfoKey: info}];
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        if (result.code == HBCoreResultCodeDone)
        {
            [self realEncodeItem:item];
        }
        else
        {
            [self completedItem:item result:result];
        }
    };

    [item.job refreshSecurityScopedResources];

    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    [self.core scanURL:item.fileURL
            titleIndex:item.job.titleIdx
              previews:10
           minDuration:0
          keepPreviews:NO
       hardwareDecoder:[NSUserDefaults.standardUserDefaults boolForKey:HBUseHardwareDecoder]
       progressHandler:progressHandler
     completionHandler:completionHandler];
}

/**
 * This assumes that we have re-scanned and loaded up a new queue item to send to libhb
 */
- (void)realEncodeItem:(HBQueueJobItem *)item
{
    HBJob *job = item.job;

    if ([NSUserDefaults.standardUserDefaults boolForKey:HBUseHardwareDecoder])
    {
        job.hwDecodeUsage = HBJobHardwareDecoderUsageFullPathOnly;

        if ([NSUserDefaults.standardUserDefaults boolForKey:HBAlwaysUseHardwareDecoder])
        {
            job.hwDecodeUsage = HBJobHardwareDecoderUsageAlways;
        }
    }
    else
    {
        job.hwDecodeUsage = HBJobHardwareDecoderUsageNone;
    }

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        if (state == HBStateMuxing)
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @1,
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
        else
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @(progress.percent),
                                                                       HBQueueWorkerProgressNotificationHoursKey: @(progress.hours),
                                                                       HBQueueWorkerProgressNotificationMinutesKey: @(progress.minutes),
                                                                       HBQueueWorkerProgressNotificationSecondsKey: @(progress.seconds),
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        [self completedItem:item result:result];
    };

    // We should be all setup so let 'er rip
    [self.core encodeJob:job progressHandler:progressHandler completionHandler:completionHandler];
}

/**
 * Cancels the current job
 */
- (void)cancel
{
    if (self.core.state == HBStateScanning)
    {
        [self.core cancelScan];
    }
    else
    {
        [self.core cancelEncode];
    }
}

@end
