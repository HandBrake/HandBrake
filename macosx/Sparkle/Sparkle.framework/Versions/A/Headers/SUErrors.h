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
#import <Sparkle/SUExport.h>

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

    // Download phase errors.
    SUTemporaryDirectoryError = 2000,
    SUDownloadError = 2001,

    // Extraction phase errors.
    SUUnarchivingError = 3000,
    SUSignatureError = 3001,
    
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
    SUNotAllowedInteractionError = 4009,
    
    // API misuse errors.
    SUIncorrectAPIUsageError = 5000
};

#endif
