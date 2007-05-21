/**
 * @file
 * @date 17.5.2007
 *
 * Interface of class HBOutputRedirect.
 */

#import <Cocoa/Cocoa.h>

/**
 * This class is used to redirect @c stdout and @c stderr outputs. It is never
 * created directly; @c stdoutRedirect and @c stderrRedirect class methods
 * should be use instead.
 *
 * @note Redirection is done by replacing @c _write functions for @c stdout and
 *		 @c stderr streams. Because of this messages written by NSLog(), for
 *		 example are not redirected. I consider this a good thing, but if more
 *		 universal redirecting is needed, it can be done at file descriptor
 *		 level.
 */
@interface HBOutputRedirect : NSObject
{
	/// Set that contains all registered listeners for this output.
	NSMutableSet *listeners;
	
	/// Selector that is called on listeners to forward the output.
	SEL forwardingSelector;

	/// Output stream (@c stdout or @c stderr) redirected by this object.
	FILE *stream;
	
	/// Pointer to old write function for the stream.
	int	(*oldWriteFunc)(void *, const char *, int);
	
	NSLock *lock;
}

+ (id)stdoutRedirect;
+ (id)stderrRedirect;

- (void)addListener:(id)aListener;
- (void)removeListener:(id)aListener;

@end

/* Here is another technique to redirect stderr, but it is done at lower level
   which also redirects NSLog() and other writes that are done directly to the
   file descriptor. This method is not used by HBOutputRedirect, but should
   be easy to implement if needed. Code is untested, but this is shows basic 
   idea for future reference.

	// Create a pipe
	NSPipe *pipe = [[NSPipe alloc] init];

	// Connect stderr to the writing end of the pipe
	dup2([[pipe fileHandleForWriting] fileDescriptor], STDERR_FILENO);	
	
	// Get reading end of the pipe, we can use this to read stderr
	NSFileHandle *fh = [pipe fileHandleForReading];
*/
