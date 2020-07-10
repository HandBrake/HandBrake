//
//  SPUStandardUserDriver.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/14/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/SPUUserDriver.h>
#import <Sparkle/SPUStandardUserDriverProtocol.h>
#import <Sparkle/SUExport.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SPUStandardUserDriverDelegate;

/*!
 Sparkle's standard built-in user driver for updater interactions
 */
SU_EXPORT @interface SPUStandardUserDriver : NSObject <SPUUserDriver, SPUStandardUserDriverProtocol>

/*!
 Initializes a Sparkle's standard user driver for user update interactions
 
 @param hostBundle The target bundle of the host that is being updated.
 @param delegate The delegate to this user driver. Pass nil if you don't want to provide one.
 */
- (instancetype)initWithHostBundle:(NSBundle *)hostBundle delegate:(nullable id<SPUStandardUserDriverDelegate>)delegate;

/*!
 * Enable or disable hideOnDeactivate for standard update window.
 */
@property (nonatomic) BOOL hideOnDeactivate;

@end

NS_ASSUME_NONNULL_END
