//
//  SPUUpdateCheck.h
//  SPUUpdateCheck
//
//  Created by Mayur Pawashe on 8/28/21.
//  Copyright © 2021 Sparkle Project. All rights reserved.
//

#ifndef SPUUpdateCheck_h
#define SPUUpdateCheck_h

/**
 Describes the type of update check being performed.
 
 Each update check corresponds to an update check method on `SPUUpdater`.
 */
typedef NS_ENUM(NSInteger, SPUUpdateCheck)
{
    /**
     The user-initiated update check corresponding to `-[SPUUpdater checkForUpdates]`.
     */
    SPUUpdateCheckUpdates = 0,
    /**
     The background scheduled update check corresponding to `-[SPUUpdater checkForUpdatesInBackground]`.
     */
    SPUUpdateCheckUpdatesInBackground = 1,
    /**
     The informational probe update check corresponding to `-[SPUUpdater checkForUpdateInformation]`.
     */
    SPUUpdateCheckUpdateInformation = 2
};

#endif /* SPUUpdateCheck_h */
