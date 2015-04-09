/**
 * @file
 * @date 17.5.2007
 *
 * Implementation of class HBOutputRedirect.
 */

#import "HBOutputRedirect.h"

/// Global pointer to HBOutputRedirect object that manages redirects for stdout.
static HBOutputRedirect *g_stdoutRedirect = nil;

/// Global pointer to HBOutputRedirect object that manages redirects for stderr.
static HBOutputRedirect *g_stderrRedirect = nil;

static int stdoutwrite(void *inFD, const char *buffer, int size);
static int stderrwrite(void *inFD, const char *buffer, int size);

@interface HBOutputRedirect ()
{
    /// Set that contains all registered listeners for this output.
    NSMutableSet *listeners;

    /// Selector that is called on listeners to forward the output.
    SEL forwardingSelector;

    /// Output stream (@c stdout or @c stderr) redirected by this object.
    FILE *stream;

    /// Pointer to old write function for the stream.
    int	(*oldWriteFunc)(void *, const char *, int);
}

@end


@interface HBOutputRedirect (Private)
- (instancetype)initWithStream:(FILE *)aStream selector:(SEL)aSelector;
- (void)startRedirect;
- (void)stopRedirect;
- (void)forwardOutput:(NSData *)data;
@end

/**
 * Function that replaces stdout->_write and forwards stdout to g_stdoutRedirect.
 */
int	stdoutwrite(void *inFD, const char *buffer, int size)
{
    @autoreleasepool
    {
        NSData *data = [[NSData alloc] initWithBytes:buffer length:size];
        [g_stdoutRedirect performSelectorOnMainThread:@selector(forwardOutput:) withObject:data waitUntilDone:NO];
    }
    return size;
}

int	stderrwrite(void *inFD, const char *buffer, int size)
{
    @autoreleasepool
    {
        NSData *data = [[NSData alloc] initWithBytes:buffer length:size];
        [g_stderrRedirect performSelectorOnMainThread:@selector(forwardOutput:) withObject:data waitUntilDone:NO];
    }
    return size;
}

@implementation HBOutputRedirect

/**
 * Returns HBOutputRedirect object used to redirect stdout.
 */
+ (id)stdoutRedirect
{
	if (!g_stdoutRedirect)
		g_stdoutRedirect = [[HBOutputRedirect alloc] initWithStream:stdout selector:@selector(stdoutRedirect:)];
		
	return g_stdoutRedirect;
}

/**
 * Returns HBOutputRedirect object used to redirect stderr.
 */
+ (id)stderrRedirect
{
	if (!g_stderrRedirect)
		g_stderrRedirect = [[HBOutputRedirect alloc] initWithStream:stderr selector:@selector(stderrRedirect:)];
		
	return g_stderrRedirect;
}

/**
 * Adds specified object as listener for this output. Method @c stdoutRedirect:
 * or @c stderrRedirect: of the listener is called to redirect the output.
 */
- (void)addListener:(id <HBOutputRedirectListening>)aListener
{
	NSAssert2([aListener respondsToSelector:forwardingSelector], @"Object %@ doesn't respond to selector \"%@\"", aListener, NSStringFromSelector(forwardingSelector));

	if (![listeners containsObject:aListener])
	{
		[listeners addObject:aListener];
	}
	
	if ([listeners count] > 0)
		[self startRedirect];
}

/**
 * Stops forwarding for this output to the specified listener object.
 */
- (void)removeListener:(id <HBOutputRedirectListening>)aListener
{
	if ([listeners containsObject:aListener])
	{
		[listeners removeObject:aListener];
	}

	// If last listener is removed, stop redirecting output and autorelease
	// self. Remember to set proper global pointer to NULL so the object is
	// recreated again when needed.
	if ([listeners count] == 0)
	{
		[self stopRedirect];

		if (self == g_stdoutRedirect)
			g_stdoutRedirect = nil;
		else if (self == g_stderrRedirect)
			g_stderrRedirect = nil;
	}
}

@end

@implementation HBOutputRedirect (Private)

/**
 * Private constructor which should not be called from outside. This is used to
 * initialize the class at @c stdoutRedirect and @c stderrRedirect.
 *
 * @param aStream	Stream that wil be redirected (stdout or stderr).
 * @param aSelector	Selector that will be called in listeners to redirect the stream.
 *
 * @return New HBOutputRedirect object.
 */
- (instancetype)initWithStream:(FILE *)aStream selector:(SEL)aSelector
{
	if (self = [super init])
	{
		listeners = [[NSMutableSet alloc] init];
		forwardingSelector = aSelector;
		stream = aStream;
		oldWriteFunc = NULL;
	}
	return self;
}

/**
 * Starts redirecting the stream by redirecting its output to function
 * @c stdoutwrite() or @c stderrwrite(). Old _write function is stored to
 * @c oldWriteFunc so it can be restored. 
 */
- (void)startRedirect
{
	if (!oldWriteFunc)
	{
		oldWriteFunc = stream->_write;
		stream->_write = stream == stdout ? stdoutwrite : stderrwrite;
	}
}

/**
 * Stops redirecting of the stream by returning the stream's _write function
 * to original.
 */
- (void)stopRedirect
{
	if (oldWriteFunc)
	{
		stream->_write = oldWriteFunc;
		oldWriteFunc = NULL;
	}
}

/**
 * Called from @c stdoutwrite() and @c stderrwrite() to forward the output to 
 * listeners.
 */ 
- (void)forwardOutput:(NSData *)data
{
	NSString *string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (string)
    {
        [listeners makeObjectsPerformSelector:forwardingSelector withObject:string];
    }
}

@end
