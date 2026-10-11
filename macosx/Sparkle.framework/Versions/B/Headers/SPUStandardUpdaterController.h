//
//  SPUStandardUpdaterController.h
//  Sparkle
//
//  Created by Mayur Pawashe on 2/28/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

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

@class SPUUpdater;
@class SPUStandardUserDriver;
@class NSMenuItem;
@protocol SPUUserDriver, SPUUpdaterDelegate, SPUStandardUserDriverDelegate;

/**
 A controller class that instantiates a `SPUUpdater` and allows binding UI to its updater settings.
 
 This class can be instantiated in a nib or created programmatically using `-initWithUpdaterDelegate:userDriverDelegate:` or `-initWithStartingUpdater:updaterDelegate:userDriverDelegate:`.
 
 The controller's updater targets the application's main bundle and uses Sparkle's standard user interface.
 Typically, this class is used by sticking it as a custom NSObject subclass in an Interface Builder nib (probably in MainMenu) but it works well programmatically too.
 
 The controller creates an `SPUUpdater` instance using a `SPUStandardUserDriver` and allows hooking up the check for updates action and handling menu item validation.
 It also allows hooking up the updater's and user driver's delegates.
 
 If you need more control over what bundle you want to update, or you want to provide a custom user interface (via `SPUUserDriver`), please use `SPUUpdater` directly instead.
 
 This class must be used on the main thread.
  */
SU_EXPORT NS_SWIFT_UI_ACTOR @interface SPUStandardUpdaterController : NSObject
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-interface-ivars"
    /**
     * Interface builder outlet for the updater's delegate.
     */
    IBOutlet __weak id<SPUUpdaterDelegate> updaterDelegate;
    
    /**
     * Interface builder outlet for the user driver's delegate.
     */
    IBOutlet __weak id<SPUStandardUserDriverDelegate> userDriverDelegate;
#pragma clang diagnostic pop
}

/**
 Accessible property for the updater. Some properties on the updater can be binded via KVO
 
 When instantiated from a nib, don't perform update checks before the application has finished launching in a MainMenu nib (i.e applicationDidFinishLaunching:) or before the corresponding window/view controller has been loaded (i.e, windowDidLoad or viewDidLoad). The updater is not guaranteed to be started yet before these points.
 */
@property (nonatomic, readonly) SPUUpdater *updater;

/**
 Accessible property for the updater's user driver.
 */
@property (nonatomic, readonly) SPUStandardUserDriver *userDriver;

/**
 Create a new `SPUStandardUpdaterController` from a nib.
 
 You cannot call this initializer directly. You must instantiate a `SPUStandardUpdaterController` inside of a nib (typically the MainMenu nib) to use it.
 
 To create a `SPUStandardUpdaterController` programmatically, use `-initWithUpdaterDelegate:userDriverDelegate:` or `-initWithStartingUpdater:updaterDelegate:userDriverDelegate:` instead.
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Create a new `SPUStandardUpdaterController` programmatically.
 
 The updater is started automatically. See `-startUpdater`  for more information.
 
 Note the `updaterDelegate` and `userDriverDelegate` are weakly referenced, so you are responsible for keeping them alive.
 */
- (instancetype)initWithUpdaterDelegate:(nullable id<SPUUpdaterDelegate>)updaterDelegate userDriverDelegate:(nullable id<SPUStandardUserDriverDelegate>)userDriverDelegate;

/**
 Create a new `SPUStandardUpdaterController` programmatically allowing you to specify whether or not to start the updater immediately.
 
 You can specify whether or not you want to start the updater immediately.
 If you do not start the updater, you must invoke `-startUpdater` at a later time to start it.
 
 Note the `updaterDelegate` and `userDriverDelegate` are weakly referenced, so you are responsible for keeping them alive.
 */
- (instancetype)initWithStartingUpdater:(BOOL)startUpdater updaterDelegate:(nullable id<SPUUpdaterDelegate>)updaterDelegate userDriverDelegate:(nullable id<SPUStandardUserDriverDelegate>)userDriverDelegate;

/**
 Starts the updater if it has not already been started.
 
 You should only call this method yourself if you opted out of starting the updater on initialization.
 Hence, do not call this yourself if you are instantiating this controller from a nib.
 
 This invokes  `-[SPUUpdater startUpdater:]`. If the application is misconfigured with Sparkle, an error is logged and an alert is shown to the user (after a few seconds) to contact the developer.
 If you want more control over this behavior, you can create your own `SPUUpdater` instead of using `SPUStandardUpdaterController`.
 */
- (void)startUpdater;

/**
 Explicitly checks for updates and displays a progress dialog while doing so.
 
 This method is meant for a main menu item.
 Connect any NSMenuItem to this action in Interface Builder or programmatically,
 and Sparkle will check for updates and report back its findings verbosely when it is invoked.
 
 When the target/action of the menu item is set to this controller and this method,
 this controller also handles enabling/disabling the menu item by checking
 `-[SPUUpdater canCheckForUpdates]`
 
 This action checks updates by invoking `-[SPUUpdater checkForUpdates]`
 */
- (IBAction)checkForUpdates:(nullable id)sender;

@end

NS_ASSUME_NONNULL_END
