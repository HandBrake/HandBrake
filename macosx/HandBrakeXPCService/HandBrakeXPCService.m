/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HandBrakeXPCService.h"
#import "HBOutputRedirect.h"

@import HandBrakeKit;

static void *HandBrakeXPCServiceContext = &HandBrakeXPCServiceContext;

@interface HandBrakeXPCService () <HBOutputRedirectListening>

@property (nonatomic, readonly) HBCore *core;

@property (nonatomic, readonly, copy) HBCoreProgressHandler progressHandler;
@property (nonatomic, readonly, copy) HBCoreCompletionHandler completionHandler;
@property (nonatomic, readwrite, copy) void (^reply)(HBCoreResult);

@property (nonatomic, readonly, weak) NSXPCConnection *connection;
@property (nonatomic, readonly) dispatch_queue_t queue;

@property (nonatomic, readwrite) NSArray<NSURL *> *urls;

@end

@implementation HandBrakeXPCService

- (instancetype)initWithConnection:(NSXPCConnection *)connection
{
    self = [super init];
    if (self)
    {
        _connection = connection;

        dispatch_queue_attr_t attr;
        attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_DEFAULT, 0);
        _queue = dispatch_queue_create("fr.handbrake.CoreQueue", attr);

        // Add ourself as stderr/stdout listener
        [HBOutputRedirect.stderrRedirect addListener:self queue:_queue];
        [HBOutputRedirect.stdoutRedirect addListener:self queue:_queue];
    }
    return self;
}

- (void)setDVDNav:(BOOL)enabled
{
    dispatch_sync(_queue, ^{
        [HBCore setDVDNav:enabled];
    });
}

- (void)setUpWithLogLevel:(NSInteger)level name:(NSString *)name
{
    _core = [[HBCore alloc] initWithLogLevel:level queue:_queue];
    _core.name = name;

    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        [self.connection.remoteObjectProxy updateProgress:progress.percent
                                                    hours:progress.hours
                                                  minutes:progress.minutes
                                                  seconds:progress.seconds
                                                    state:state
                                                     info:info];
    };
    _progressHandler = progressHandler;

    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        [HBCore cleanTemporaryFiles];
        [self stopAccessingSecurityScopedResources];
        self.reply(result);
        self.reply = nil;
    };
    _completionHandler = completionHandler;

    // Set up observers
    [self.core addObserver:self forKeyPath:@"state"
                   options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                   context:HandBrakeXPCServiceContext];
}

- (void)initGlobal
{
    [HBCore initGlobal];
}

- (void)closeGlobal
{
    [HBCore closeGlobal];
}

- (void)setLogLevel:(NSInteger)logLevel
{
    dispatch_sync(_queue, ^{
        self.core.logLevel = logLevel;
    });
}

- (void)provideResourceAccessWithBookmarks:(NSArray<NSData *> *)bookmarks
{
    dispatch_sync(_queue, ^{
        NSMutableArray<NSURL *> *urls = [NSMutableArray array];
        for (NSData *bookmark in bookmarks)
        {
            NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:0 relativeToURL:nil bookmarkDataIsStale:NULL error:NULL];
            if (url)
            {
                [urls addObject:url];
            }
        }
        self.urls = urls;
    });
}

- (void)stopAccessingSecurityScopedResources
{
    for (NSURL *url in self.urls)
    {
        [url stopAccessingSecurityScopedResource];
    }
}

- (void)setAutomaticallyPreventSleep:(BOOL)automaticallyPreventSleep
{
    dispatch_sync(_queue, ^{
        self.core.automaticallyPreventSleep = automaticallyPreventSleep;
    });
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HandBrakeXPCServiceContext)
    {
        [self.connection.remoteObjectProxy updateState:self.core.state];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)preventSleep
{
    dispatch_sync(_queue, ^{
        [self.core preventSleep];
    });
}

- (void)allowSleep
{
    dispatch_sync(_queue, ^{
        [self.core allowSleep];
    });
}

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews hardwareDecoder:(BOOL)hardwareDecoder keepDuplicateTitles:(BOOL)keepDuplicateTitles withReply:(void (^)(HBCoreResult))reply
{
    dispatch_sync(_queue, ^{
        self.reply = reply;

        [self.core scanURLs:@[url] titleIndex:index
                   previews:previewsNum
                minDuration:seconds
               keepPreviews:keepPreviews
            hardwareDecoder:hardwareDecoder
            keepDuplicateTitles:keepDuplicateTitles
            progressHandler:self.progressHandler
          completionHandler:self.completionHandler];
    });
}

- (void)cancelScan
{
    dispatch_sync(_queue, ^{
        [self.core cancelScan];
    });
}

 - (void)encodeJob:(HBJob *)job withReply:(void (^)(HBCoreResult))reply
{
    dispatch_sync(_queue, ^{
        self.reply = reply;

        // Reset the title in the job.
        job.title = self.core.titles.firstObject;

        [self.core encodeJob:job
             progressHandler:self.progressHandler
           completionHandler:self.completionHandler];
    });
}

- (void)cancelEncode
{
    dispatch_sync(_queue, ^{
        [self.core cancelEncode];
    });
}

- (void)pauseEncode
{
    dispatch_sync(_queue, ^{
        [self.core pause];
    });
}

- (void)resumeEncode
{
    dispatch_sync(_queue, ^{
        [self.core resume];
    });
}

- (void)redirect:(nonnull NSString *)text type:(HBRedirectType)type
{
    if (type == HBRedirectTypeOutput)
    {
        [self.connection.remoteObjectProxy forwardError:text];
    }
    else
    {
        [self.connection.remoteObjectProxy forwardOutput:text];
    }
}

@end
