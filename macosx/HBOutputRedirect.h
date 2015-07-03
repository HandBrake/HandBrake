/**
 * @file
 * @date 17.5.2007
 *
 * Interface of class HBOutputRedirect.
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol HBOutputRedirectListening <NSObject>

- (void)stdoutRedirect:(NSString *)text;
- (void)stderrRedirect:(NSString *)text;

@end

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

+ (id)stdoutRedirect;
+ (id)stderrRedirect;

- (void)addListener:(id <HBOutputRedirectListening>)aListener;
- (void)removeListener:(id <HBOutputRedirectListening>)aListener;

@end

NS_ASSUME_NONNULL_END
