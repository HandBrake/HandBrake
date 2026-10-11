//
//  SUErrors.h
//  Sparkle
//
//  Created by C.W. Betts on 10/13/14.
//  Copyright (c) 2014 Sparkle Project. All rights reserved.
//

#ifndef SUERRORS_H
#define SUERRORS_H

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

/**
 * Error domain used by Sparkle
 */
SU_EXPORT extern NSString *const SUSparkleErrorDomain;

typedef NS_ENUM(OSStatus, SUError) {
    // Configuration phase errors
    SUNoPublicDSAFoundError = 0001,
    SUInsufficientSigningError = 0002,
    SUInsecureFeedURLError = 0003,
    SUInvalidFeedURLError = 0004,
    SUInvalidUpdaterError = 0005,
    SUInvalidHostBundleIdentifierError = 0006,
    SUInvalidHostVersionError = 0007,
    
    // Appcast phase errors.
    SUAppcastParseError = 1000,
    SUNoUpdateError = 1001,
    SUAppcastError = 1002,
    SURunningFromDiskImageError = 1003,
    SUResumeAppcastError = 1004,
    SURunningTranslocated = 1005,
    SUWebKitTerminationError = 1006,
    SUReleaseNotesError = 1007,

    // Download phase errors.
    SUTemporaryDirectoryError = 2000,
    SUDownloadError = 2001,

    // Extraction phase errors.
    SUUnarchivingError = 3000,
    SUSignatureError = 3001,
    SUValidationError = 3002,
    
    // Installation phase errors.
    SUFileCopyFailure = 4000,
    SUAuthenticationFailure = 4001,
    SUMissingUpdateError = 4002,
    SUMissingInstallerToolError = 4003,
    SURelaunchError = 4004,
    SUInstallationError = 4005,
    SUDowngradeError = 4006,
    SUInstallationCanceledError = 4007,
    SUInstallationAuthorizeLaterError = 4008,
    SUNotValidUpdateError = 4009,
    SUAgentInvalidationError = 4010,
    //SUInstallationRootInteractiveError = 4011,
    SUInstallationWriteNoPermissionError = 4012,
    
    // API misuse errors.
    SUIncorrectAPIUsageError = 5000
};

/**
 The reason why a new update is not available.
 */
typedef NS_ENUM(OSStatus, SPUNoUpdateFoundReason) {
    /**
     A new update is unavailable for an unknown reason.
     */
    SPUNoUpdateFoundReasonUnknown,
    /**
     A new update is unavailable because the user is on the latest known version in the appcast feed.
     */
    SPUNoUpdateFoundReasonOnLatestVersion,
    /**
     A new update is unavailable because the user is on a version newer than the latest known version in the appcast feed.
     */
    SPUNoUpdateFoundReasonOnNewerThanLatestVersion,
    /**
     A new update is unavailable because the user's operating system version is too old for the update.
     */
    SPUNoUpdateFoundReasonSystemIsTooOld,
    /**
     A new update is unavailable because the user's operating system version is too new for the update.
     */
    SPUNoUpdateFoundReasonSystemIsTooNew,
    /**
     A new update is unavailable because the user's system is an Intel Mac that doesn't support ARM64.
     */
    SPUNoUpdateFoundReasonHardwareDoesNotSupportARM64,
};

SU_EXPORT extern NSString *const SPUNoUpdateFoundReasonKey;
SU_EXPORT extern NSString *const SPULatestAppcastItemFoundKey;
SU_EXPORT extern NSString *const SPUNoUpdateFoundUserInitiatedKey;

#endif
