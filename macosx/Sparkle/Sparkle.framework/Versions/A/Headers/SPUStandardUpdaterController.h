//
//  SPUStandardUpdaterController.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/28/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/SUExport.h>

NS_ASSUME_NONNULL_BEGIN

@class SPUUpdater;
@protocol SPUUserDriver, SPUStandardUserDriverProtocol, SPUUpdaterDelegate, SPUStandardUserDriverDelegate;

/*!
 A controller class that instantiates a SPUUpdater and allows binding UI to it.
 
 This class can be instantiated in a nib or created using initWithUpdaterDelegate:userDriverDelegate:. The controller's updater targets the application's main bundle
 and uses Sparkle's standard user interface. Typically, this class is used by sticking it as a custom NSObject subclass in an Interface Builder nib (probably in MainMenu).
 
 The controller creates an SPUUpdater instance and allows hooking up the check for updates action and menu item validation. It also allows hooking
 up the updater's and user driver's delegates.
 
 This controller class may not be valuable to you if:
 * You want to control or defer the instantiation of an SPUUpdater, or don't want to be tied into a nib's instantiation, or don't want to use a nib
 * You want to target a bundle that's not the main bundle
 * You want to provide a custom user interface, or perhaps one that provides little-to-none
 * You don't want to use a convenience class that provides very little glue ;)
 
  */
SU_EXPORT @interface SPUStandardUpdaterController : NSObject

/*!
 Interface builder outlet for the updater's delegate.
 
 This property should only be set using Interface Builder by creating a connection using the outlet
 */
@property (nonatomic, weak, nullable) IBOutlet id<SPUUpdaterDelegate> updaterDelegate;

/*!
 Interface builder outlet for the user driver's delegate.
 
 This property should only be set using Interface Builder by creating a connection using the outlet
 */
@property (nonatomic, weak, nullable) IBOutlet id<SPUStandardUserDriverDelegate> userDriverDelegate;

/*!
 Accessible property for the updater. Some properties on the updater can be binded via KVO
 
 This is nil before being loaded from the nib.
 You may access this property after your application has finished launching, or after your window controller has finished loading.
 */
@property (nonatomic, readonly, nullable) SPUUpdater *updater;

/*!
 Accessible property for the updater's user driver.
 
 This is nil before being loaded from the nib.
 You may access this property after your application has finished launching, or after your window controller has finished loading.
 */
@property (nonatomic, readonly, nullable) id <SPUStandardUserDriverProtocol> userDriver;

/*!
 Use initWithUpdaterDelegate:userDriverDelegate: instead.
 */
- (instancetype)init NS_UNAVAILABLE;

/*!
 Create a new SPUStandardUpdaterController programmatically.
 */
- (instancetype)initWithUpdaterDelegate:(nullable id<SPUUpdaterDelegate>)updaterDelegate userDriverDelegate:(nullable id<SPUStandardUserDriverDelegate>)userDriverDelegate;

/*!
 Explicitly checks for updates and displays a progress dialog while doing so.
 
 This method is meant for a main menu item.
 Connect any menu item to this action in Interface Builder,
 and Sparkle will check for updates and report back its findings verbosely
 when it is invoked.
 
 This action checks updates by invoking -[SPUUpdater checkForUpdates]
 */
- (IBAction)checkForUpdates:(id)sender;

/*!
 Validates if the menu item for checkForUpdates: can be invoked or not
 
 This validates the menu item by checking -[SPUStandardUserDriver canCheckForUpdates]
 */
- (BOOL)validateMenuItem:(NSMenuItem *)item;

@end

NS_ASSUME_NONNULL_END
