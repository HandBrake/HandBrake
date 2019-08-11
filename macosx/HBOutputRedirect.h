/**
 * @file
 * @date 17.5.2007
 *
 * Interface of class HBOutputRedirect.
 */

#import <Foundation/Foundation.h>
#import "HBRedirect.h"

NS_ASSUME_NONNULL_BEGIN

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
@interface HBOutputRedirect : HBRedirect

@end

NS_ASSUME_NONNULL_END
