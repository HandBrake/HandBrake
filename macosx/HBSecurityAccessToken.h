/*  HBSecurityAccessToken.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol HBSecurityScope <NSObject>

///  Given an instance, make the resource referenced by the instance accessible to the process.
- (BOOL)startAccessingSecurityScopedResource;

/// Revokes the access granted to the instance by a prior successful call to startAccessingSecurityScopedResource.
- (void)stopAccessingSecurityScopedResource;

/// Refresh the resources (for example if the instance stores a security scoped bookmark, it will recreate the urls from the bookmark.
- (void)refreshSecurityScopedResources;

@end

@interface HBSecurityAccessToken : NSObject

- (instancetype)initWithObject:(id<HBSecurityScope>)object;
- (instancetype)initWithAlreadyAccessedObject:(id<HBSecurityScope>)object;

+ (instancetype)tokenWithObject:(id<HBSecurityScope>)object;
+ (instancetype)tokenWithAlreadyAccessedObject:(id<HBSecurityScope>)object;

@end

NS_ASSUME_NONNULL_END
