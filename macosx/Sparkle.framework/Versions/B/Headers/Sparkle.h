//
//  Sparkle.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/16/06. (Modified by CDHW on 23/12/07)
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#ifndef SPARKLE_H
#define SPARKLE_H

// This list should include the shared headers. It doesn't matter if some of them aren't shared (unless
// there are name-space collisions) so we can list all of them to start with:

#import <Sparkle/SUExport.h>
#import <Sparkle/SUAppcast.h>
#import <Sparkle/SUAppcastItem.h>
#import <Sparkle/SUStandardVersionComparator.h>
#import <Sparkle/SPUUpdater.h>
#import <Sparkle/SPUUpdaterDelegate.h>
#import <Sparkle/SPUUpdaterSettings.h>
#import <Sparkle/SUVersionComparisonProtocol.h>
#import <Sparkle/SUVersionDisplayProtocol.h>
#import <Sparkle/SUErrors.h>
#import <Sparkle/SPUUpdatePermissionRequest.h>
#import <Sparkle/SUUpdatePermissionResponse.h>
#import <Sparkle/SPUUserDriver.h>
#import <Sparkle/SPUDownloadData.h>

// UI bits
#import <Sparkle/SPUStandardUpdaterController.h>
#import <Sparkle/SPUStandardUserDriver.h>
#import <Sparkle/SPUStandardUserDriverDelegate.h>

// Deprecated bits
#import <Sparkle/SUUpdater.h>
#import <Sparkle/SUUpdaterDelegate.h>

#endif
