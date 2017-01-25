//
//  HBSecurityAccessToken.h
//  HandBrake
//
//  Created by Damiano Galassi on 24/01/17.
//
//

#import <Foundation/Foundation.h>

@protocol HBSecurityScope <NSObject>

/*  Given an instance, make the resource referenced by the job accessible to the process.
 */
- (BOOL)startAccessingSecurityScopedResource;

/*  Revokes the access granted to the url by a prior successful call to startAccessingSecurityScopedResource.
 */
- (void)stopAccessingSecurityScopedResource;

@end

@interface NSURL (HBSecurityScope) <HBSecurityScope>

- (BOOL)startAccessingSecurityScopedResource;
- (void)stopAccessingSecurityScopedResource;

@end

@interface HBSecurityAccessToken : NSObject

+ (instancetype)tokenWithObject:(id<HBSecurityScope>)object;
- (instancetype)initWithObject:(id<HBSecurityScope>)object;

@end
