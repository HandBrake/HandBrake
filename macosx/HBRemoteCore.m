/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRemoteCore.h"
#import "HBRemoteCoreProtocol.h"

@import HandBrakeKit;

@interface HBRemoteCore () <HBRemoteProgressProtocol>

@property (nonatomic, readonly) NSXPCConnection *connection;

@property (nonatomic, readonly) id<HBRemoteCoreProtocol> proxy;

@property (nonatomic, readwrite) HBState state;

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

    _proxy = [_connection remoteObjectProxyWithErrorHandler:^(NSError * _Nonnull error) {
        
    }];

    [_connection resume];
}

- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name
{
    self = [self init];
    if (self)
    {
        [_proxy setUpWithLogLevel:level name:name];
    }
    return self;
}

- (void)dealloc
{
    [_connection.remoteObjectProxy tearDown];
    [_connection invalidate];
}

- (void)updateState:(HBState)state {
    dispatch_sync(dispatch_get_main_queue(), ^{
        self.state = state;
    });
}

- (void)setAutomaticallyPreventSleep:(BOOL)automaticallyPreventSleep
{
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

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds progressHandler:(nonnull HBCoreProgressHandler)progressHandler completionHandler:(nonnull HBCoreCompletionHandler)completionHandler
{
    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    NSData *bookmark = [url bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    [_connection.remoteObjectProxy provideResourceAccessWithBookmarks:@[bookmark]];

    self.state = HBStateScanning;

    [_connection.remoteObjectProxy scanURL:url titleIndex:index previews:previewsNum minDuration:seconds withReply:^(HBCoreResult result) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.progressHandler = nil;
            self.completionHandler(result);
        });
    }];
}

- (void)cancelScan
{
    [_proxy cancelScan];
}

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler
{
    NSData *bookmark = [job.outputURL bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:NULL];
    [_connection.remoteObjectProxy provideResourceAccessWithBookmarks:@[bookmark]];

    self.progressHandler = progressHandler;
    self.completionHandler = completionHandler;

    self.state = HBStateWorking;

    [_connection.remoteObjectProxy encodeJob:job withReply:^(HBCoreResult result) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.progressHandler = nil;
            self.completionHandler(result);
        });
    }];
}

- (void)cancelEncode
{
    [_proxy cancelEncode];
}

- (void)updateProgress:(double)currentProgress hours:(int)hours minutes:(int)minutes seconds:(int)seconds state:(HBState)state info:(NSString *)info {
    dispatch_sync(dispatch_get_main_queue(), ^{
        HBProgress progress = {currentProgress , hours, minutes, seconds};
        self.state = state;
        self.progressHandler(state, progress, info);
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
