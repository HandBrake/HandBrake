/* HBQueueController.h

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <Growl/Growl.h>

NS_ASSUME_NONNULL_BEGIN

@class HBAppDelegate;
@class HBController;
@class HBOutputPanelController;
@class HBCore;
@class HBJob;

@interface HBQueueController : NSWindowController <NSToolbarDelegate, NSWindowDelegate, GrowlApplicationBridgeDelegate>

- (instancetype)initWithURL:(NSURL *)queueURL;

/// The HBCore used for encoding.
@property (nonatomic, readonly) HBCore *core;

@property (nonatomic, assign, nullable) HBController *controller;
@property (nonatomic, weak, nullable) HBAppDelegate *delegate;

@property (nonatomic, readonly) NSUInteger count;
@property (nonatomic, readonly) NSUInteger pendingItemsCount;

- (void)addJob:(HBJob *)item;
- (void)addJobsFromArray:(NSArray<HBJob *> *)items;

- (BOOL)jobExistAtURL:(NSURL *)url;

- (void)removeAllJobs;
- (void)removeCompletedJobs;

- (void)setEncodingJobsAsPending;

- (IBAction)rip:(id)sender;
- (IBAction)cancelRip:(id)sender;

- (IBAction)togglePauseResume:(id)sender;

@end

NS_ASSUME_NONNULL_END

