/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRemoteCore.h"
#import "HBRemoteCoreProtocol.h"
#import "HBPreferencesKeys.h"

@import HandBrakeKit;

@interface HBRemoteCore () <HBRemoteProgressProtocol>

@property (nonatomic, readonly) NSXPCConnection *connection;
@property (nonatomic, readonly) id<HBRemoteCoreProtocol> proxy;

@property (nonatomic, readwrite) HBState state;

@property (nonatomic, readonly) NSInteger level;
@property (nonatomic, readonly, copy) NSString *name;
@property (nonatomic, readonly, copy) NSString *serviceName;

@property (nonatomic, readwrite, copy) HBCoreProgressHandler progressHandler;
@property (nonatomic, readwrite, copy) HBCoreCompletionHandler completionHandler;

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
        _automaticallyPreventSleep = NO;
        _level = 1;
        _name = @"HandBrakeXPC";
        _serviceName = @"fr.handbrake.HandBrakeXPCService";
    }
    return self;
}

- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name serviceName:(NSString *)serviceName
{
    self = [self init];
    if (self)
    {
        _level = level;
        _name = [name copy];
        _serviceName = [serviceName copy];
    }
    return self;
}

- (void)connect
{
    _connection = [[NSXPCConnection alloc] initWithServiceName:self.serviceName];
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

    [_proxy setDVDNav:[NSUserDefaults.standardUserDefaults boolForKey:HBUseDvdNav]];
    [_proxy setUpWithLogLevel:self.level name:self.name];
    [_proxy setAutomaticallyPreventSleep:self.automaticallyPreventSleep];
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
    [_proxy setAutomaticallyPreventSleep:self.automaticallyPreventSleep];

    HBCoreCompletionHandler handler = self.completionHandler;

    self.progressHandler = nil;
    self.completionHandler = nil;

    self.state = HBStateIdle;

    if (handler)
    {
        HBCoreResult result = {0, HBCoreResultCodeUnknown};
        result.code = HBCoreResultCodeUnknown;
        handler(result);
    }

    [self forwardError:@"XPC: Service did crash\n"];
}

- (void)updateState:(HBState)state
{
    dispatch_sync(dispatch_get_main_queue(), ^{
        self.state = state;
    });
}

- (void)setLogLevel:(NSInteger)logLevel
{
    _logLevel = logLevel;
    [_proxy setLogLevel:logLevel];
}

- (void)setAutomaticallyPreventSleep:(BOOL)automaticallyPreventSleep
{
    _automaticallyPreventSleep = automaticallyPreventSleep;
    [_proxy setAutomaticallyPreventSleep:automaticallyPreventSleep];
}

- (void)allowSleep
{
    [_proxy allowSleep];
}

- (void)preventSleep
{
    [_proxy preventSleep];
}

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews progressHandler:(nonnull HBCoreProgressHandler)progressHandler completionHandler:(nonnull HBCoreCompletionHandler)completionHandler
{
    if (!_connection)
    {
        [self connect];
    }

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

    NSData *destinationBookmark = [job.destinationFolderURL bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    if (destinationBookmark)
    {
        [bookmarks addObject:destinationBookmark];
    }

    NSData *sourceBookmark = [job.fileURL bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    if (sourceBookmark)
    {
        [bookmarks addObject:sourceBookmark];
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
            handler(result);
        });
    }];
}

- (void)cancelEncode
{
    [_proxy cancelEncode];
}

- (void)updateProgress:(double)currentProgress hours:(int)hours minutes:(int)minutes seconds:(int)seconds state:(HBState)state info:(NSString *)info
{
    __weak HBRemoteCore *weakSelf = self;

    dispatch_sync(dispatch_get_main_queue(), ^{
        if (weakSelf.progressHandler)
        {
            HBProgress progress = {currentProgress , hours, minutes, seconds};
            weakSelf.progressHandler(state, progress, info);
        }
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
}

- (void)resume
{
    [_proxy resumeEncode];
}

@end
