//
//  SPUStatusCompletionResults.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/29/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#ifndef SPUStatusCompletionResults_h
#define SPUStatusCompletionResults_h

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SPUUserInitiatedCheckStatus) {
    SPUUserInitiatedCheckDone,
    SPUUserInitiatedCheckCanceled
};

typedef NS_ENUM(NSUInteger, SPUDownloadUpdateStatus) {
    SPUDownloadUpdateDone,
    SPUDownloadUpdateCanceled
};

typedef NS_ENUM(NSUInteger, SPUInstallUpdateStatus) {
    SPUInstallUpdateNow,
    SPUInstallAndRelaunchUpdateNow,
    SPUDismissUpdateInstallation
};

typedef NS_ENUM(NSInteger, SPUUpdateAlertChoice) {
    SPUInstallUpdateChoice,
    SPUInstallLaterChoice,
    SPUSkipThisVersionChoice
};

typedef NS_ENUM(NSInteger, SPUInformationalUpdateAlertChoice) {
    SPUDismissInformationalNoticeChoice,
    SPUSkipThisInformationalVersionChoice
};

#endif /* SPUStatusCompletionResults_h */
