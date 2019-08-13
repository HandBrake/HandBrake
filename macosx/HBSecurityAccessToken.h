/*  HBSecurityAccessToken.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBJob.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBSecurityScope <NSObject>

/*  Given an instance, make the resource referenced by the job accessible to the process.
 */
- (BOOL)startAccessingSecurityScopedResource;

/*  Revokes the access granted to the url by a prior successful call to startAccessingSecurityScopedResource.
 */
- (void)stopAccessingSecurityScopedResource;

@end

@interface NSURL (HBSecurityScope) <HBSecurityScope>
@end

@interface HBJob (HBSecurityScope) <HBSecurityScope>
@end

@interface HBSecurityAccessToken : NSObject

+ (instancetype)tokenWithObject:(id<HBSecurityScope>)object;
- (instancetype)initWithObject:(id<HBSecurityScope>)object;

@end

NS_ASSUME_NONNULL_END
