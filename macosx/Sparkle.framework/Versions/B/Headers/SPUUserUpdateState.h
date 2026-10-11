//
//  SPUUserUpdateState.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/29/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#ifndef SPUUserUpdateState_h
#define SPUUserUpdateState_h

#import <Foundation/Foundation.h>

#if defined(BUILDING_SPARKLE_SOURCES_EXTERNALLY)
// Ignore incorrect warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wquoted-include-in-framework-header"
#import "SUExport.h"
#pragma clang diagnostic pop
#else
#import <Sparkle/SUExport.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 A choice made by the user when prompted with a new update.
 */
typedef NS_ENUM(NSInteger, SPUUserUpdateChoice) {
    /**
     Dismisses the update and skips being notified of it in the future.
     */
    SPUUserUpdateChoiceSkip,
    /**
     Downloads (if needed) and installs the update.
     */
    SPUUserUpdateChoiceInstall,
    /**
     Dismisses the update until Sparkle reminds the user of it at a later time.
     */
    SPUUserUpdateChoiceDismiss,
};

/**
 Describes the current stage an update is undergoing.
 */
typedef NS_ENUM(NSInteger, SPUUserUpdateStage) {
    /**
     The update has not been downloaded.
     */
    SPUUserUpdateStageNotDownloaded,
    /**
     The update has already been downloaded but not begun installing.
     */
    SPUUserUpdateStageDownloaded,
    /**
     The update has already been downloaded and began installing in the background.
     */
    SPUUserUpdateStageInstalling
};

/**
 This represents the user's current update state.
 */
SU_EXPORT NS_SWIFT_SENDABLE @interface SPUUserUpdateState : NSObject<NSSecureCoding>

- (instancetype)init NS_UNAVAILABLE;

/**
 The current update stage.
 
 This stage indicates if data has been already downloaded or not, or if an update is currently being installed.
 */
@property (nonatomic, readonly) SPUUserUpdateStage stage;

/**
 Indicates whether or not the update check was initiated by the user.
 */
@property (nonatomic, readonly) BOOL userInitiated;

@end

NS_ASSUME_NONNULL_END

#endif /* SPUUserUpdateState_h */
