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

/// Output stream (@c stdout or @c stderr) redirected by this object.
@property (nonatomic, readonly) FILE *stream;

/// Pointer to old write function for the stream.
@property (nonatomic, readonly) int (*oldWriteFunc)(void *, const char *, int);

@end

/**
 * Function that replaces stdout->_write and forwards stdout to g_stdoutRedirect.
 */
int	stdoutwrite(void *inFD, const char *buffer, int size)
{
    @autoreleasepool
    {
        NSString *string = [[NSString alloc] initWithBytes:buffer length:size encoding:NSUTF8StringEncoding];
        if (string)
        {
            [g_stdoutRedirect forwardOutput:string];
        }
    }
    return size;
}

int	stderrwrite(void *inFD, const char *buffer, int size)
{
    @autoreleasepool
    {
        NSString *string = [[NSString alloc] initWithBytes:buffer length:size encoding:NSUTF8StringEncoding];
        if (string)
        {
            [g_stderrRedirect forwardOutput:string];
        }
    }
    return size;
}

@implementation HBOutputRedirect

/**
 * Returns HBOutputRedirect object used to redirect stdout.
 */
+ (instancetype)stdoutRedirect
{
	if (!g_stdoutRedirect)
    {
		g_stdoutRedirect = [[HBOutputRedirect alloc] initWithStream:stdout type:HBRedirectTypeOutput];
    }
	return g_stdoutRedirect;
}

/**
 * Returns HBOutputRedirect object used to redirect stderr.
 */
+ (instancetype)stderrRedirect
{
	if (!g_stderrRedirect)
    {
        g_stderrRedirect = [[HBOutputRedirect alloc] initWithStream:stderr type:HBRedirectTypeError];
    }
	return g_stderrRedirect;
}

/**
 * Private constructor which should not be called from outside. This is used to
 * initialize the class at @c stdoutRedirect and @c stderrRedirect.
 *
 * @param stream	Stream that will be redirected (stdout or stderr).
 * @param type   	Type that will be called in listeners to redirect the stream.
 *
 * @return New HBOutputRedirect object.
 */
- (instancetype)initWithStream:(FILE *)stream type:(HBRedirectType)type
{
	if (self = [self initWithType:type])
	{
		_stream = stream;
		_oldWriteFunc = NULL;
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
	if (!_oldWriteFunc)
	{
		_oldWriteFunc = _stream->_write;
		_stream->_write = _stream == stdout ? stdoutwrite : stderrwrite;
	}
}

/**
 * Stops redirecting of the stream by returning the stream's _write function
 * to original.
 */
- (void)stopRedirect
{
	if (_oldWriteFunc)
	{
		_stream->_write = _oldWriteFunc;
		_oldWriteFunc = NULL;
	}
}

@end
