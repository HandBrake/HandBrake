//
//  SUUserDriverCoreComponent.h
//  Sparkle
//
//  Created by Mayur Pawashe on 3/4/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Sparkle/SPUStatusCompletionResults.h>
#import <Sparkle/SUExport.h>

@protocol SPUStandardUserDriverDelegate;

SU_EXPORT @interface SPUUserDriverCoreComponent : NSObject

- (void)showCanCheckForUpdates:(BOOL)canCheckForUpdates;

@property (nonatomic, readonly) BOOL canCheckForUpdates;

- (void)registerInstallUpdateHandler:(void (^)(SPUInstallUpdateStatus))installUpdateHandler;
- (void)installUpdateWithChoice:(SPUInstallUpdateStatus)choice;

- (void)registerUpdateCheckStatusHandler:(void (^)(SPUUserInitiatedCheckStatus))updateCheckStatusCompletion;
- (void)cancelUpdateCheckStatus;
- (void)completeUpdateCheckStatus;

- (void)registerDownloadStatusHandler:(void (^)(SPUDownloadUpdateStatus))downloadUpdateStatusCompletion;
- (void)cancelDownloadStatus;
- (void)completeDownloadStatus;

- (void)registerAcknowledgement:(void (^)(void))acknowledgement;
- (void)acceptAcknowledgement;

- (void)dismissUpdateInstallation;

@end
