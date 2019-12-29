/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRemoteCore.h"
#import "HBRemoteCoreProtocol.h"
#import "HBPreferencesKeys.h"
#import <IOKit/pwr_mgt/IOPMLib.h>

@import HandBrakeKit;

@interface HBRemoteCore () <HBRemoteProgressProtocol>

@property (nonatomic, readonly) NSXPCConnection *connection;
@property (nonatomic, readonly) id<HBRemoteCoreProtocol> proxy;

@property (nonatomic, readwrite) HBState state;

@property (nonatomic, readonly) NSInteger level;
@property (nonatomic, readonly, copy) NSString *name;

@property (nonatomic, readwrite, copy) HBCoreProgressHandler progressHandler;
@property (nonatomic, readwrite, copy) HBCoreCompletionHandler completionHandler;

@property (nonatomic, readwrite) IOPMAssertionID assertionID;

@end

@implementation HBRemoteCore

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _state = HBStateIdle;
        _stdoutRedirect = HBRedirect.stdoutRedirect;
        _stderrRedirect = HBRedirect.stderrRedirect;
        _assertionID = -1;

        [self connect];
    }
    return self;
}

- (void)connect
{
    _connection = [[NSXPCConnection alloc] initWithServiceName:@"fr.handbrake.HandBrakeXPCService"];
    _connection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(HBRemoteCoreProtocol)];

    _connection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(HBRemoteProgressProtocol)];
    _connection.exportedObject = self;

    __weak HBRemoteCore *weakSelf = self;

    _connection.interruptionHandler = ^{
        dispatch_sync(dispatch_get_main_queue(), ^{
            [weakSelf handleInterruption];
        });
    };

    _connection.invalidationHandler = ^{
        dispatch_sync(dispatch_get_main_queue(), ^{
            [weakSelf forwardError:@"XPC: Service connection was invalidated\n"];
        });
    };

    _proxy = [_connection remoteObjectProxyWithErrorHandler:^(NSError * _Nonnull error) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            [self forwardError:@"XPC: Service did report an error\n"];
            [HBUtilities writeErrorToActivityLog:error];
        });
    }];

    [_connection resume];
}

- (void)invalidate
{
    [[_connection synchronousRemoteObjectProxyWithErrorHandler:^(NSError * _Nonnull error) {}] tearDown];
    [_connection invalidate];
    _connection = nil;
    _proxy = nil;
}

- (void)handleInterruption
{
    [_proxy setDVDNav:[NSUserDefaults.standardUserDefaults boolForKey:HBUseDvdNav]];
    [_proxy setUpWithLogLevel:self.level name:self.name];

    HBCoreCompletionHandler handler = self.completionHandler;

    self.progressHandler = nil;
    self.completionHandler = nil;

    self.state = HBStateIdle;

    if (handler)
    {
        handler(HBCoreResultFailed);
    }

    [self forwardError:@"XPC: Service did crash\n"];
}

- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name
{
    self = [self init];
    if (self)
    {
        _level = level;
        _name = name;
        [_proxy setDVDNav:[NSUserDefaults.standardUserDefaults boolForKey:HBUseDvdNav]];
        [_proxy setUpWithLogLevel:level name:name];
    }
    return self;
}

- (void)updateState:(HBState)state {
    dispatch_sync(dispatch_get_main_queue(), ^{
        self.state = state;
    });
}

- (void)setLogLevel:(NSInteger)logLevel
{
    _logLevel = logLevel;
    [_proxy setLogLevel:logLevel];
}

- (void)preventSleep
{
    if (_assertionID != -1)
    {
        // nothing to do
        return;
    }

    CFStringRef reasonForActivity= CFSTR("HandBrake is currently scanning and/or encoding");

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleSystemSleep,
                                                   kIOPMAssertionLevelOn, reasonForActivity, &_assertionID);

    if (success != kIOReturnSuccess)
    {
        [HBUtilities writeToActivityLog:"HBRemoteCore: failed to prevent system sleep"];
    }
}

- (void)allowSleep
{
    if (_assertionID == -1)
    {
        // nothing to do
        return;
    }

    IOReturn success = IOPMAssertionRelease(_assertionID);

    if (success == kIOReturnSuccess)
    {
        _assertionID = -1;
    }
}

- (void)preventAutoSleep
{
    if (self.automaticallyPreventSleep)
    {
        [self preventSleep];
    }
}

- (void)allowAutoSleep
{
    if (self.automaticallyPreventSleep)
    {
        [self allowSleep];
    }
}

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews progressHandler:(nonnull HBCoreProgressHandler)progressHandler completionHandler:(nonnull HBCoreCompletionHandler)completionHandler
{
    [self preventAutoSleep];

#ifdef __SANDBOX_ENABLED__
    __block HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:url];

    NSData *bookmark = [url bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    if (bookmark)
    {
        [_proxy provideResourceAccessWithBookmarks:@[bookmark]];
    }
#endif

    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    self.state = HBStateScanning;

    __weak HBRemoteCore *weakSelf = self;

    [_proxy scanURL:url titleIndex:index previews:previewsNum minDuration:seconds keepPreviews:keepPreviews withReply:^(HBCoreResult result) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            HBCoreCompletionHandler handler = weakSelf.completionHandler;
            weakSelf.completionHandler = nil;
            weakSelf.progressHandler = nil;
#ifdef __SANDBOX_ENABLED__
            token = nil;
#endif
            [weakSelf allowAutoSleep];
            handler(result);
        });
    }];
}

- (void)cancelScan
{
    [_proxy cancelScan];
}

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    [self preventAutoSleep];

#ifdef __SANDBOX_ENABLED__
    __block HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:job];

    NSMutableArray<NSData *> *bookmarks = [NSMutableArray array];

    for (HBSubtitlesTrack *track in job.subtitles.tracks)
    {
        if (track.fileURL)
        {
            NSData *subtitlesBookmark = [track.fileURL bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
            if (subtitlesBookmark)
            {
                [bookmarks addObject:subtitlesBookmark];
            }
        }
    }

    NSData *bookmark = [job.outputURL bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    if (bookmark)
    {
        [bookmarks addObject:bookmark];
    }

    [_proxy provideResourceAccessWithBookmarks:bookmarks];

#endif

    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    self.state = HBStateWorking;

    __weak HBRemoteCore *weakSelf = self;

    [_proxy encodeJob:job withReply:^(HBCoreResult result) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            HBCoreCompletionHandler handler = weakSelf.completionHandler;
            weakSelf.completionHandler = nil;
            weakSelf.progressHandler = nil;
#ifdef __SANDBOX_ENABLED__
            token = nil;
#endif
            [weakSelf allowAutoSleep];
            handler(result);
        });
    }];
}

- (void)cancelEncode
{
    [_proxy cancelEncode];
}

- (void)updateProgress:(double)currentProgress hours:(int)hours minutes:(int)minutes seconds:(int)seconds state:(HBState)state info:(NSString *)info {

    __weak HBRemoteCore *weakSelf = self;

    dispatch_sync(dispatch_get_main_queue(), ^{
        HBProgress progress = {currentProgress , hours, minutes, seconds};
        weakSelf.state = state;
        weakSelf.progressHandler(state, progress, info);
    });
}

- (void)forwardOutput:(NSString *)text
{
    [_stdoutRedirect forwardOutput:text];
    [HBUtilities writeToActivityLogWithNoHeader:text];
}

- (void)forwardError:(NSString *)text
{
    [_stdoutRedirect forwardOutput:text];
    [HBUtilities writeToActivityLogWithNoHeader:text];
}

- (void)pause
{
    [_proxy pauseEncode];
    [self allowAutoSleep];
}

- (void)resume
{
    [_proxy resumeEncode];
    [self preventAutoSleep];
}

@end
