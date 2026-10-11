/*  HBQueueRelinkWindowController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBJob;

NS_ASSUME_NONNULL_BEGIN

@interface HBQueueRelinkWindowController : NSWindowController

/// Works on copies, so cancelling leaves the supplied jobs untouched.
- (instancetype)initWithJobs:(NSArray<HBJob *> *)jobs;
@property (nonatomic, readonly) NSArray<HBJob *> *jobs;

/// The completion handler must save the jobs before returning, while access is held.
- (void)beginSheetForWindow:(NSWindow *)window completionHandler:(void (^)(NSModalResponse response))handler;

@end

NS_ASSUME_NONNULL_END
