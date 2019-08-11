/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HBRedirectType) {
    HBRedirectTypeOutput,
    HBRedirectTypeError
};

@protocol HBOutputRedirectListening <NSObject>

- (void)redirect:(NSString *)text type:(HBRedirectType)type;

@end

@interface HBRedirect : NSObject

+ (instancetype)stdoutRedirect;
+ (instancetype)stderrRedirect;

- (void)addListener:(id <HBOutputRedirectListening>)aListener queue:(dispatch_queue_t)queue;
- (void)removeListener:(id <HBOutputRedirectListening>)aListener;

// Methods to subclass

- (instancetype)initWithType:(HBRedirectType)type;
- (void)forwardOutput:(NSString *)text;
- (void)startRedirect;
- (void)stopRedirect;

@end

NS_ASSUME_NONNULL_END
