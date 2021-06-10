/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRedirect.h"
#import "HBDirectUtilities.h"

HB_OBJC_DIRECT_MEMBERS
@interface HBOutputRedirectListenerEntry : NSObject

@property (nonatomic, readonly) id<HBOutputRedirectListening> listener;
@property (nonatomic, readonly) dispatch_queue_t queue;

- (instancetype)initWithListener:(id<HBOutputRedirectListening>)listener queue:(dispatch_queue_t)queue;

@end

HB_OBJC_DIRECT_MEMBERS
@implementation HBOutputRedirectListenerEntry

- (instancetype)initWithListener:(id<HBOutputRedirectListening>)entryListener queue:(dispatch_queue_t)entryQueue
{
    self = [super init];
    if (self)
    {
        _listener = entryListener;
        _queue = entryQueue;
    }
    return self;
}

@end

HB_OBJC_DIRECT_MEMBERS
@interface HBRedirect ()

/// Set that contains all registered listeners for this output.
@property (nonatomic, readonly) NSMutableSet<HBOutputRedirectListenerEntry *> *listenerEntries;

/// Selector that is called on listeners to forward the output.
@property (nonatomic, readonly) HBRedirectType type;

@end

HB_OBJC_DIRECT_MEMBERS
@implementation HBRedirect

/**
 * Returns HBOutputRedirect object used to redirect stdout.
 */
+ (instancetype)stdoutRedirect
{
    return [[HBRedirect alloc] initWithType:HBRedirectTypeOutput];
}

/**
 * Returns HBOutputRedirect object used to redirect stderr.
 */
+ (instancetype)stderrRedirect
{
    return [[HBRedirect alloc] initWithType:HBRedirectTypeError];
}

- (HBOutputRedirectListenerEntry *)entryForListener:(id <HBOutputRedirectListening>)listener
{
    for (HBOutputRedirectListenerEntry *entry in _listenerEntries)
    {
        if  (entry.listener == listener)
        {
            return entry;
        }
    }
    return nil;
}

- (BOOL)containsListener:(id <HBOutputRedirectListening>)listener
{
    HBOutputRedirectListenerEntry *entry = [self entryForListener:listener];
    return entry && [_listenerEntries containsObject:listener];
}

/**
 * Adds specified object as listener for this output. Method @c stdoutRedirect:
 * or @c stderrRedirect: of the listener is called to redirect the output.
 */
- (void)addListener:(id <HBOutputRedirectListening>)listener queue:(dispatch_queue_t)queue
{
    NSAssert2([listener respondsToSelector:@selector(redirect:type:)], @"Object %@ doesn't respond to selector \"%@\"", listener, NSStringFromSelector(@selector(redirect:type:)));

    if (![self containsListener:listener])
    {
        HBOutputRedirectListenerEntry *entry = [[HBOutputRedirectListenerEntry alloc] initWithListener:listener queue:queue];
        [_listenerEntries addObject:entry];
    }

    if (_listenerEntries.count > 0)
    {
        [self startRedirect];
    }
}

/**
 * Stops forwarding for this output to the specified listener object.
 */
- (void)removeListener:(id <HBOutputRedirectListening>)listener
{
    HBOutputRedirectListenerEntry *entry = [self entryForListener:listener];
    if (entry)
    {
        [_listenerEntries removeObject:entry];
    }

    // If last listener is removed, stop redirecting output and autorelease
    // self. Remember to set proper global pointer to NULL so the object is
    // recreated again when needed.
    if (_listenerEntries.count == 0)
    {
        [self stopRedirect];
    }
}

- (instancetype)initWithType:(HBRedirectType)type
{
    if (self = [super init])
    {
        _listenerEntries = [[NSMutableSet alloc] init];
        _type = type;
    }
    return self;
}

- (void)startRedirect {}
- (void)stopRedirect {}

/**
 * Called from @c stdoutwrite() and @c stderrwrite() to forward the output to
 * listeners.
 */
- (void)forwardOutput:(NSString *)string
{
    for (HBOutputRedirectListenerEntry *entry in _listenerEntries)
    {
        dispatch_async(entry.queue, ^{
            [entry.listener redirect:string type:self->_type];
        });
    }
}

@end
