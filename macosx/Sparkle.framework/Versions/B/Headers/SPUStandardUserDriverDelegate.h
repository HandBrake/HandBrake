//
//  SPUStandardUserDriverDelegate.h
//  Sparkle
//
//  Created by Mayur Pawashe on 3/3/16.
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

@protocol SUVersionDisplay;
@class SUAppcastItem;
@class SPUUserUpdateState;

/**
 A protocol for Sparkle's standard user driver's delegate
 
 This includes methods related to UI interactions
 */
SU_EXPORT @protocol SPUStandardUserDriverDelegate <NSObject>

@optional

/**
 Called before showing a modal alert window,
 to give the opportunity to hide attached windows that may get in the way.
 */
- (void)standardUserDriverWillShowModalAlert;

/**
 Called after showing a modal alert window,
 to give the opportunity to hide attached windows that may get in the way.
 */
- (void)standardUserDriverDidShowModalAlert;

/**
 Returns an object that formats version numbers for display to the user.
 If you don't implement this method or return @c nil, the standard version formatter will be used.
 */
- (_Nullable id <SUVersionDisplay>)standardUserDriverRequestsVersionDisplayer;

/**
 Decides whether or not the standard user driver should provide an option to show full release notes to the user.
 
 When a user checks for new updates and no new update is found, Sparkle by default will offer to show the application's version history to the user
 by providing a "Version History" button in the no new update available alert.
 
 If this delegate method is implemented to return `NO`, then Sparkle will not provide an option to show full release notes to the user.
 
 @param item The appcast item corresponding to the latest version available.
 @return @c YES to allow Sparkle to show full release notes to the user, otherwise @c NO to disallow this.
 */
- (BOOL)standardUserDriverShouldShowVersionHistoryForAppcastItem:(SUAppcastItem *)item;

/**
 Handles showing the full release notes to the user.
 
 When a user checks for new updates and no new update is found, Sparkle will offer to show the application's version history to the user
 by providing a "Version History" button in the no new update available alert.
 
 If this delegate method is not implemented, Sparkle will instead offer to open the
 `fullReleaseNotesLink` (or `releaseNotesLink` if the former is unavailable) from the appcast's latest `item` in the user's web browser.
 
 If this delegate method is implemented, Sparkle will instead ask the delegate to show the full release notes to the user.
 A delegate may want to implement this method if they want to show in-app or offline release notes.
 
 @param item The appcast item corresponding to the latest version available.
 */
- (void)standardUserDriverShowVersionHistoryForAppcastItem:(SUAppcastItem *)item;

/**
 Specifies whether or not the download, extraction, and installing status windows allows to be minimized.
 
 By default, the status window showing the current status of the update (download, extraction, ready to install) is allowed to be minimized
 for regular application bundle updates.
 
 @return @c YES if the status window is allowed to be minimized (default behavior), otherwise @c NO.
 */
- (BOOL)standardUserDriverAllowsMinimizableStatusWindow;

/**
 Declares whether or not gentle scheduled update reminders are supported.
 
 The delegate may implement scheduled update reminders that are presented in a gentle manner by implementing one or both of:
 `-standardUserDriverWillHandleShowingUpdate:forUpdate:state:` and `-standardUserDriverShouldHandleShowingScheduledUpdate:andInImmediateFocus:`
 
 Visit https://sparkle-project.org/documentation/gentle-reminders for more information and examples.
 
 @return @c YES if gentle scheduled update reminders are implemented by standard user driver delegate, otherwise @c NO (default).
 */
@property (nonatomic, readonly) BOOL supportsGentleScheduledUpdateReminders;

/**
 Specifies if the standard user driver should handle showing a new scheduled update, or if its delegate should handle showing the update instead.
 
 If you implement this method and return @c NO the delegate is then responsible for showing the update,
 which must be implemented and done in `-standardUserDriverWillHandleShowingUpdate:forUpdate:state:`
 The motivation for the delegate being responsible for showing updates is to override Sparkle's default behavior
 and add gentle reminders for new updates.
 
 Returning @c YES is the default behavior and allows the standard user driver to handle showing the update.
 
 If the standard user driver handles showing the update, `immediateFocus` reflects whether or not it will show the update in immediate and utmost focus.
 The standard user driver may choose to show the update in immediate and utmost focus when the app was launched recently
 or the system has been idle for some time.
 
 If `immediateFocus` is @c NO the standard user driver may want to defer showing the update until the user comes back to the app.
 For background running applications, when `immediateFocus` is  @c NO the standard user driver will always want to show
 the update alert immediately, but behind other running applications or behind the app's own windows if it's currently active.
 
 There should be no side effects made when implementing this method so you should just return @c YES or @c NO
 You will also want to implement `-standardUserDriverWillHandleShowingUpdate:forUpdate:state:` for adding additional update reminders.
 
 This method is not called for user-initiated update checks. The standard user driver always handles those.
 
 Visit https://sparkle-project.org/documentation/gentle-reminders for more information and examples.
 
 @param update The update the standard user driver should show.
 @param immediateFocus If @c immediateFocus is @c YES, then the standard user driver proposes to show the update in immediate and utmost focus. See discussion for more details.
 
 @return @c YES if the standard user should handle showing the scheduled update (default behavior), otherwise @c NO if the delegate handles showing it.
 */
- (BOOL)standardUserDriverShouldHandleShowingScheduledUpdate:(SUAppcastItem *)update andInImmediateFocus:(BOOL)immediateFocus;

/**
 Called before an update will be shown to the user.
 
 If the standard user driver handles showing the update, `handleShowingUpdate` will be `YES`.
 Please see `-standardUserDriverShouldHandleShowingScheduledUpdate:andInImmediateFocus:` for how the standard user driver
 may handle showing scheduled updates when `handleShowingUpdate` is `YES` and `state.userInitiated` is `NO`.
 
 If the delegate declared it handles showing the update by returning @c NO in `-standardUserDriverShouldHandleShowingScheduledUpdate:andInImmediateFocus:`
 then the delegate should handle showing update reminders in this method, or at some later point.
 In this case, `handleShowingUpdate` will be @c NO.
 To bring the update alert in focus, you may call `-[SPUStandardUpdaterController checkForUpdates:]` or `-[SPUUpdater checkForUpdates]`.
 You may want to show additional UI indicators in your application that will show this update in focus
 and want to dismiss additional UI indicators in `-standardUserDriverWillFinishUpdateSession` or `-standardUserDriverDidReceiveUserAttentionForUpdate:`
  
 If `state.userInitiated` is @c YES then the standard user driver always handles showing the new update and `handleShowingUpdate` will be @c YES.
 In this case, it may still be useful for the delegate to intercept this method right before a new update will be shown.
 
 This method is not called when bringing an update that has already been presented back in focus.
 
 Visit https://sparkle-project.org/documentation/gentle-reminders for more information and examples.
 
 @param handleShowingUpdate @c YES if the standard user driver handles showing the update, otherwise @c NO if the delegate handles showing the update.
 @param update The update that will be shown.
 @param state The user state of the update which includes if the update check was initiated by the user.
 */
- (void)standardUserDriverWillHandleShowingUpdate:(BOOL)handleShowingUpdate forUpdate:(SUAppcastItem *)update state:(SPUUserUpdateState *)state;

/**
 Called when a new update first receives attention from the user.
 
 This occurs either when the user first brings the update alert in utmost focus or when the user makes a choice to install an update or dismiss/skip it.
 
 This may be useful to intercept for dismissing custom attention-based UI indicators (e.g, user notifications) introduced when implementing
 `-standardUserDriverWillHandleShowingUpdate:forUpdate:state:`
 
 For custom UI indicators that need to still be on screen after the user has started to install an update, please see `-standardUserDriverWillFinishUpdateSession`.
 
 @param update The new update that the user gave attention to.
 */
- (void)standardUserDriverDidReceiveUserAttentionForUpdate:(SUAppcastItem *)update;

/**
 Called before the standard user driver session will finish its current update session.
 
 This may occur after the user has dismissed / skipped a new update or after an update error has occurred.
 For updaters updating external/other bundles, this may also be called after an update has been successfully installed.
 
 This may be useful to intercept for dismissing custom UI indicators introduced when implementing
 `-standardUserDriverWillHandleShowingUpdate:forUpdate:state:`
 
 For UI indicators that need to be dismissed when the user has given attention to a new update alert,
 please see `-standardUserDriverDidReceiveUserAttentionForUpdate:`
 */
- (void)standardUserDriverWillFinishUpdateSession;

/**
 Called before the standard user driver shows plain-text or markdown release notes text to the user.
 
 The delegate has the opportunity to return a new attributed string for the release notes text that will be shown to the user.
 The `bundleDisplayVersion` and `bundleVersion` are supplied in case they're useful for creating a new attributed string.
 
 This method will not be invoked for HTML release notes. It is only applicable to plain-text and markdown release notes.
 
 @param releaseNotesAttributedString The release notes text that the standard user driver wants to show to the user.
 @param update The new update the release notes will be shown for.
 @param bundleDisplayVersion The current display version  (or `CFBundleShortVersionString`)  of the bundle that is being updated.
 @param bundleVersion The current version   (or `CFBundleVersion`) of the bundle that is being updated.
 @return A new attributed string for the release notes text to show, or @c nil if the `releaseNotesAttributedString` should still be used.
 */
- (NSAttributedString * _Nullable)standardUserDriverWillShowReleaseNotesText:(NSAttributedString *)releaseNotesAttributedString forUpdate:(SUAppcastItem *)update withBundleDisplayVersion:(NSString *)bundleDisplayVersion bundleVersion:(NSString *)bundleVersion;

@end

NS_ASSUME_NONNULL_END
