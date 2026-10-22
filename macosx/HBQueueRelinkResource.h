/*  HBQueueRelinkResource.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBJob, HBSecurityAccessToken;

NS_ASSUME_NONNULL_BEGIN

/// A shared source or destination folder in a set of jobs being relinked.
@interface HBQueueRelinkResource : NSObject

+ (NSArray<HBQueueRelinkResource *> *)resourcesForJobs:(NSArray<HBJob *> *)jobs;

@property (nonatomic, readonly, getter=isSource) BOOL source;
@property (nonatomic, readonly, nullable) NSURL *originalURL;
@property (nonatomic, readonly, nullable) NSURL *replacementURL;
@property (nonatomic, readonly, getter=isMissing) BOOL missing;

/// Retains access until the caller has applied and saved the jobs.
- (BOOL)locateURL:(NSURL *)url accessToken:(HBSecurityAccessToken *)token error:(NSError **)error;
- (BOOL)validateReplacementWithError:(NSError **)error;
- (void)apply;

@end

NS_ASSUME_NONNULL_END
