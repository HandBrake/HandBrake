//
//  SPUStandardUserDriverProtocol.h
//  Sparkle
//
//  Created by Mayur Pawashe on 3/5/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol SPUStandardUserDriverDelegate;

/*!
 Protocol for Sparkle's standard built-in user driver for updater interactions.
 
 Other user drivers may wish to implement this protocol.
 Note that this protocol does not adopt SUUserDriver because one may want to *only* export properties from this protocol.
 */
@protocol SPUStandardUserDriverProtocol <NSObject>

/*!
 Indicates whether or not an update is in progress as far as the user's perspective is concerned
 
 A typical application may rely on this property for its check for updates menu item validation
 */
@property (nonatomic, readonly) BOOL canCheckForUpdates;

@end
