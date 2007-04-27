//
//  GrowlApplicationBridge.h
//  Growl
//
//  Created by Evan Schoenberg on Wed Jun 16 2004.
//  Copyright 2004-2005 The Growl Project. All rights reserved.
//

/*!
 *	@header		GrowlApplicationBridge.h
 *	@abstract   Defines the GrowlApplicationBridge class.
 *	@discussion This header defines the GrowlApplicationBridge class as well as
 *	 the GROWL_PREFPANE_BUNDLE_IDENTIFIER constant.
 */

#ifndef __GrowlApplicationBridge_h__
#define __GrowlApplicationBridge_h__

#import <Foundation/Foundation.h>
#import "GrowlDefines.h"

//Forward declarations
@protocol GrowlApplicationBridgeDelegate;

/*!
 *	@defined    GROWL_PREFPANE_BUNDLE_IDENTIFIER
 *	@discussion The bundle identifier for the Growl prefpane.
 */
#define GROWL_PREFPANE_BUNDLE_IDENTIFIER	@"com.growl.prefpanel"

/*!
 *	@defined    GROWL_PREFPANE_NAME
 *	@discussion The file name of the Growl prefpane.
 */
#define GROWL_PREFPANE_NAME					@"Growl.prefPane"

//Internal notification when the user chooses not to install (to avoid continuing to cache notifications awaiting installation)
#define GROWL_USER_CHOSE_NOT_TO_INSTALL_NOTIFICATION @"User chose not to install"

//------------------------------------------------------------------------------
#pragma mark -

/*!
 *	@class      GrowlApplicationBridge
 *	@abstract   A class used to interface with Growl.
 *	@discussion This class provides a means to interface with Growl.
 *
 *	 Currently it provides a way to detect if Growl is installed and launch the
 *	 GrowlHelperApp if it's not already running.
 */
@interface GrowlApplicationBridge : NSObject {

}

/*!
 *	@method isGrowlInstalled
 *	@abstract Detects whether Growl is installed.
 *	@discussion Determines if the Growl prefpane and its helper app are installed.
 *	@result Returns YES if Growl is installed, NO otherwise.
 */
+ (BOOL) isGrowlInstalled;

/*!
 *	@method isGrowlRunning
 *	@abstract Detects whether GrowlHelperApp is currently running.
 *	@discussion Cycles through the process list to find whether GrowlHelperApp is running and returns its findings.
 *	@result Returns YES if GrowlHelperApp is running, NO otherwise.
 */
+ (BOOL) isGrowlRunning;

#pragma mark -

/*!
 *	@method setGrowlDelegate:
 *	@abstract Set the object which will be responsible for providing and receiving Growl information.
 *	@discussion This must be called before using GrowlApplicationBridge.
 *
 *	 The methods in the GrowlApplicationBridgeDelegate protocol are required
 *	 and return the basic information needed to register with Growl.
 *
 *	 The methods in the GrowlApplicationBridgeDelegate_InformalProtocol
 *	 informal protocol are individually optional.  They provide a greater
 *	 degree of interaction between the application and growl such as informing
 *	 the application when one of its Growl notifications is clicked by the user.
 *
 *	 The methods in the GrowlApplicationBridgeDelegate_Installation_InformalProtocol
 *	 informal protocol are individually optional and are only applicable when
 *	 using the Growl-WithInstaller.framework which allows for automated Growl
 *	 installation.
 *
 *	 When this method is called, data will be collected from inDelegate, Growl
 *	 will be launched if it is not already running, and the application will be
 *	 registered with Growl.
 *
 *	 If using the Growl-WithInstaller framework, if Growl is already installed
 *	 but this copy of the framework has an updated version of Growl, the user
 *	 will be prompted to update automatically.
 *
 *	@param inDelegate The delegate for the GrowlApplicationBridge. It must conform to the GrowlApplicationBridgeDelegate protocol.
 */
+ (void) setGrowlDelegate:(NSObject<GrowlApplicationBridgeDelegate> *)inDelegate;

/*!
 *	@method growlDelegate
 *	@abstract Return the object responsible for providing and receiving Growl information.
 *	@discussion See setGrowlDelegate: for details.
 *	@result The Growl delegate.
 */
+ (NSObject<GrowlApplicationBridgeDelegate> *) growlDelegate;

#pragma mark -

/*!
 *	@method notifyWithTitle:description:notificationName:iconData:priority:isSticky:clickContext:
 *	@abstract Send a Growl notification.
 *	@discussion This is the preferred means for sending a Growl notification.
 *	 The notification name and at least one of the title and description are
 *	 required (all three are preferred).  All other parameters may be
 *	 <code>nil</code> (or 0 or NO as appropriate) to accept default values.
 *
 *	 If using the Growl-WithInstaller framework, if Growl is not installed the
 *	 user will be prompted to install Growl. If the user cancels, this method
 *	 will have no effect until the next application session, at which time when
 *	 it is called the user will be prompted again. The user is also given the
 *	 option to not be prompted again.  If the user does choose to install Growl,
 *	 the requested notification will be displayed once Growl is installed and
 *	 running.
 *
 *	@param title		The title of the notification displayed to the user.
 *	@param description	The full description of the notification displayed to the user.
 *	@param notifName	The internal name of the notification. Should be human-readable, as it will be displayed in the Growl preference pane.
 *	@param iconData		<code>NSData</code> object to show with the notification as its icon. If <code>nil</code>, the application's icon will be used instead.
 *	@param priority		The priority of the notification. The default value is 0; positive values are higher priority and negative values are lower priority. Not all Growl displays support priority.
 *	@param isSticky		If YES, the notification will remain on screen until clicked. Not all Growl displays support sticky notifications.
 *	@param clickContext	A context passed back to the Growl delegate if it implements -(void)growlNotificationWasClicked: and the notification is clicked. Not all display plugins support clicking. The clickContext must be plist-encodable (completely of <code>NSString</code>, <code>NSArray</code>, <code>NSNumber</code>, <code>NSDictionary</code>, and <code>NSData</code> types).
 */
+ (void) notifyWithTitle:(NSString *)title
			 description:(NSString *)description
		notificationName:(NSString *)notifName
				iconData:(NSData *)iconData
				priority:(signed int)priority
				isSticky:(BOOL)isSticky
			clickContext:(id)clickContext;

/*!
 *	@method notifyWithTitle:description:notificationName:iconData:priority:isSticky:clickContext:identifier:
 *	@abstract Send a Growl notification.
 *	@discussion This is the preferred means for sending a Growl notification.
 *	 The notification name and at least one of the title and description are
 *	 required (all three are preferred).  All other parameters may be
 *	 <code>nil</code> (or 0 or NO as appropriate) to accept default values.
 *
 *	 If using the Growl-WithInstaller framework, if Growl is not installed the
 *	 user will be prompted to install Growl. If the user cancels, this method
 *	 will have no effect until the next application session, at which time when
 *	 it is called the user will be prompted again. The user is also given the
 *	 option to not be prompted again.  If the user does choose to install Growl,
 *	 the requested notification will be displayed once Growl is installed and
 *	 running.
 *
 *	@param title		The title of the notification displayed to the user.
 *	@param description	The full description of the notification displayed to the user.
 *	@param notifName	The internal name of the notification. Should be human-readable, as it will be displayed in the Growl preference pane.
 *	@param iconData		<code>NSData</code> object to show with the notification as its icon. If <code>nil</code>, the application's icon will be used instead.
 *	@param priority		The priority of the notification. The default value is 0; positive values are higher priority and negative values are lower priority. Not all Growl displays support priority.
 *	@param isSticky		If YES, the notification will remain on screen until clicked. Not all Growl displays support sticky notifications.
 *	@param clickContext	A context passed back to the Growl delegate if it implements -(void)growlNotificationWasClicked: and the notification is clicked. Not all display plugins support clicking. The clickContext must be plist-encodable (completely of <code>NSString</code>, <code>NSArray</code>, <code>NSNumber</code>, <code>NSDictionary</code>, and <code>NSData</code> types).
 *	@param identifier	An identifier for this notification. Notifications with equal identifiers are coalesced.
 */
+ (void) notifyWithTitle:(NSString *)title
			 description:(NSString *)description
		notificationName:(NSString *)notifName
				iconData:(NSData *)iconData
				priority:(signed int)priority
				isSticky:(BOOL)isSticky
			clickContext:(id)clickContext
			  identifier:(NSString *)identifier;

/*!	@method	notifyWithDictionary:
 *	@abstract	Notifies using a userInfo dictionary suitable for passing to
 *	 <code>NSDistributedNotificationCenter</code>.
 *	@param	userInfo	The dictionary to notify with.
 *	@discussion	Before Growl 0.6, your application would have posted
 *	 notifications using <code>NSDistributedNotificationCenter</code> by
 *	 creating a userInfo dictionary with the notification data. This had the
 *	 advantage of allowing you to add other data to the dictionary for programs
 *	 besides Growl that might be listening.
 *
 *	 This method allows you to use such dictionaries without being restricted
 *	 to using <code>NSDistributedNotificationCenter</code>. The keys for this dictionary
 *	 can be found in GrowlDefines.h.
 */
+ (void) notifyWithDictionary:(NSDictionary *)userInfo;

#pragma mark -

/*!	@method	registerWithDictionary:
 *	@abstract	Register your application with Growl without setting a delegate.
 *	@discussion	When you call this method with a dictionary,
 *	 GrowlApplicationBridge registers your application using that dictionary.
 *	 If you pass <code>nil</code>, GrowlApplicationBridge will ask the delegate
 *	 (if there is one) for a dictionary, and if that doesn't work, it will look
 *	 in your application's bundle for an auto-discoverable plist.
 *	 (XXX refer to more information on that)
 *
 *	 If you pass a dictionary to this method, it must include the
 *	 <code>GROWL_APP_NAME</code> key, unless a delegate is set.
 *
 *	 This method is mainly an alternative to the delegate system introduced
 *	 with Growl 0.6. Without a delegate, you cannot receive callbacks such as
 *	 <code>-growlIsReady</code> (since they are sent to the delegate). You can,
 *	 however, set a delegate after registering without one.
 *
 *	 This method was introduced in Growl.framework 0.7.
 */
+ (BOOL) registerWithDictionary:(NSDictionary *)regDict;

/*!	@method	reregisterGrowlNotifications
 *	@abstract	Reregister the notifications for this application.
 *	@discussion	This method does not normally need to be called.  If your
 *	 application changes what notifications it is registering with Growl, call
 *	 this method to have the Growl delegate's
 *	 <code>-registrationDictionaryForGrowl</code> method called again and the
 *	 Growl registration information updated.
 *
 *	 This method is now implemented using <code>-registerWithDictionary:</code>.
 */
+ (void) reregisterGrowlNotifications;

#pragma mark -

/*!	@method	setWillRegisterWhenGrowlIsReady:
 *	@abstract	Tells GrowlApplicationBridge to register with Growl when Growl
 *	 launches (or not).
 *	@discussion	When Growl has started listening for notifications, it posts a
 *	 <code>GROWL_IS_READY</code> notification on the Distributed Notification
 *	 Center. GrowlApplicationBridge listens for this notification, using it to
 *	 perform various tasks (such as calling your delegate's
 *	 <code>-growlIsReady</code> method, if it has one). If this method is
 *	 called with <code>YES</code>, one of those tasks will be to reregister
 *	 with Growl (in the manner of <code>-reregisterGrowlNotifications</code>).
 *
 *	 This attribute is automatically set back to <code>NO</code> (the default)
 *	 after every <code>GROWL_IS_READY</code> notification.
 *	@param	flag	<code>YES</code> if you want GrowlApplicationBridge to register with
 *	 Growl when next it is ready; <code>NO</code> if not.
 */
+ (void) setWillRegisterWhenGrowlIsReady:(BOOL)flag;
/*!	@method	willRegisterWhenGrowlIsReady
 *	@abstract	Reports whether GrowlApplicationBridge will register with Growl
 *	 when Growl next launches.
 *	@result	<code>YES</code> if GrowlApplicationBridge will register with Growl
 *	 when next it posts GROWL_IS_READY; <code>NO</code> if not.
 */
+ (BOOL) willRegisterWhenGrowlIsReady;

#pragma mark -

/*!	@method	registrationDictionaryFromDelegate
 *	@abstract	Asks the delegate for a registration dictionary.
 *	@discussion	If no delegate is set, or if the delegate's
 *	 <code>-registrationDictionaryForGrowl</code> method returns
 *	 <code>nil</code>, this method returns <code>nil</code>.
 *
 *	 This method does not attempt to clean up the dictionary in any way - for
 *	 example, if it is missing the <code>GROWL_APP_NAME</code> key, the result
 *	 will be missing it too. Use <code>+[GrowlApplicationBridge
 *	 registrationDictionaryByFillingInDictionary:]</code> or
 *	 <code>+[GrowlApplicationBridge
 *	 registrationDictionaryByFillingInDictionary:restrictToKeys:]</code> to try
 *	 to fill in missing keys.
 *
 *	 This method was introduced in Growl.framework 0.7.
 *	@result A registration dictionary.
 */
+ (NSDictionary *) registrationDictionaryFromDelegate;

/*!	@method	registrationDictionaryFromBundle:
 *	@abstract	Looks in a bundle for a registration dictionary.
 *	@discussion	This method looks in a bundle for an auto-discoverable
 *	 registration dictionary file using <code>-[NSBundle
 *	 pathForResource:ofType:]</code>. If it finds one, it loads the file using
 *	 <code>+[NSDictionary dictionaryWithContentsOfFile:]</code> and returns the
 *	 result.
 *
 *	 If you pass <code>nil</code> as the bundle, the main bundle is examined.
 *
 *	 This method does not attempt to clean up the dictionary in any way - for
 *	 example, if it is missing the <code>GROWL_APP_NAME</code> key, the result
 *	 will be missing it too. Use <code>+[GrowlApplicationBridge
 *	 registrationDictionaryByFillingInDictionary:]</code> or
 *	 <code>+[GrowlApplicationBridge
 *	 registrationDictionaryByFillingInDictionary:restrictToKeys:]</code> to try
 *	 to fill in missing keys.
 *
 *	 This method was introduced in Growl.framework 0.7.
 *	@result A registration dictionary.
 */
+ (NSDictionary *) registrationDictionaryFromBundle:(NSBundle *)bundle;

/*!	@method	bestRegistrationDictionary
 *	@abstract	Obtains a registration dictionary, filled out to the best of
 *	 GrowlApplicationBridge's knowledge.
 *	@discussion	This method creates a registration dictionary as best
 *	 GrowlApplicationBridge knows how.
 *
 *	 First, GrowlApplicationBridge contacts the Growl delegate (if there is
 *	 one) and gets the registration dictionary from that. If no such dictionary
 *	 was obtained, GrowlApplicationBridge looks in your application's main
 *	 bundle for an auto-discoverable registration dictionary file. If that
 *	 doesn't exist either, this method returns <code>nil</code>.
 *
 *	 Second, GrowlApplicationBridge calls
 *	 <code>+registrationDictionaryByFillingInDictionary:</code> with whatever
 *	 dictionary was obtained. The result of that method is the result of this
 *	 method.
 *
 *	 GrowlApplicationBridge uses this method when you call
 *	 <code>+setGrowlDelegate:</code>, or when you call
 *	 <code>+registerWithDictionary:</code> with <code>nil</code>.
 *
 *	 This method was introduced in Growl.framework 0.7.
 *	@result	A registration dictionary.
 */
+ (NSDictionary *) bestRegistrationDictionary;

#pragma mark -

/*!	@method	registrationDictionaryByFillingInDictionary:
 *	@abstract	Tries to fill in missing keys in a registration dictionary.
 *	@discussion	This method examines the passed-in dictionary for missing keys,
 *	 and tries to work out correct values for them. As of 0.7, it uses:
 *
 *	 Key							             Value
 *	 ---							             -----
 *	 <code>GROWL_APP_NAME</code>                 <code>CFBundleExecutableName</code>
 *	 <code>GROWL_APP_ICON</code>                 The icon of the application.
 *	 <code>GROWL_APP_LOCATION</code>             The location of the application.
 *	 <code>GROWL_NOTIFICATIONS_DEFAULT</code>    <code>GROWL_NOTIFICATIONS_ALL</code>
 *
 *	 Keys are only filled in if missing; if a key is present in the dictionary,
 *	 its value will not be changed.
 *
 *	 This method was introduced in Growl.framework 0.7.
 *	@param	regDict	The dictionary to fill in.
 *	@result	The dictionary with the keys filled in. This is an autoreleased
 *	 copy of <code>regDict</code>.
 */
+ (NSDictionary *) registrationDictionaryByFillingInDictionary:(NSDictionary *)regDict;
/*!	@method	registrationDictionaryByFillingInDictionary:restrictToKeys:
 *	@abstract	Tries to fill in missing keys in a registration dictionary.
 *	@discussion	This method examines the passed-in dictionary for missing keys,
 *	 and tries to work out correct values for them. As of 0.7, it uses:
 *
 *	 Key							             Value
 *	 ---							             -----
 *	 <code>GROWL_APP_NAME</code>                 <code>CFBundleExecutableName</code>
 *	 <code>GROWL_APP_ICON</code>                 The icon of the application.
 *	 <code>GROWL_APP_LOCATION</code>             The location of the application.
 *	 <code>GROWL_NOTIFICATIONS_DEFAULT</code>    <code>GROWL_NOTIFICATIONS_ALL</code>
 *
 *	 Only those keys that are listed in <code>keys</code> will be filled in.
 *	 Other missing keys are ignored. Also, keys are only filled in if missing;
 *	 if a key is present in the dictionary, its value will not be changed.
 *
 *	 This method was introduced in Growl.framework 0.7.
 *	@param	regDict	The dictionary to fill in.
 *	@param	keys	The keys to fill in. If <code>nil</code>, any missing keys are filled in.
 *	@result	The dictionary with the keys filled in. This is an autoreleased
 *	 copy of <code>regDict</code>.
 */
+ (NSDictionary *) registrationDictionaryByFillingInDictionary:(NSDictionary *)regDict restrictToKeys:(NSSet *)keys;

@end

//------------------------------------------------------------------------------
#pragma mark -

/*!
 *	@protocol GrowlApplicationBridgeDelegate
 *	@abstract Required protocol for the Growl delegate.
 *	@discussion The methods in this protocol are required and are called
 *	 automatically as needed by GrowlApplicationBridge. See
 *	 <code>+[GrowlApplicationBridge setGrowlDelegate:]</code>.
 *	 See also <code>GrowlApplicationBridgeDelegate_InformalProtocol</code>.
 */

@protocol GrowlApplicationBridgeDelegate

// -registrationDictionaryForGrowl has moved to the informal protocol as of 0.7.

@end

//------------------------------------------------------------------------------
#pragma mark -

/*!
 *	@category NSObject(GrowlApplicationBridgeDelegate_InformalProtocol)
 *	@abstract Methods which may be optionally implemented by the GrowlDelegate.
 *	@discussion The methods in this informal protocol will only be called if implemented by the delegate.
 */
@interface NSObject (GrowlApplicationBridgeDelegate_InformalProtocol)

/*!
 *	@method registrationDictionaryForGrowl
 *	@abstract Return the dictionary used to register this application with Growl.
 *	@discussion The returned dictionary gives Growl the complete list of
 *	 notifications this application will ever send, and it also specifies which
 *	 notifications should be enabled by default.  Each is specified by an array
 *	 of <code>NSString</code> objects.
 *
 *	 For most applications, these two arrays can be the same (if all sent
 *	 notifications should be displayed by default).
 *
 *	 The <code>NSString</code> objects of these arrays will correspond to the
 *	 <code>notificationName:</code> parameter passed in
 *	 <code>+[GrowlApplicationBridge
 *	 notifyWithTitle:description:notificationName:iconData:priority:isSticky:clickContext:]</code> calls.
 *
 *	 The dictionary should have 2 key object pairs:
 *	 key: GROWL_NOTIFICATIONS_ALL		object: <code>NSArray</code> of <code>NSString</code> objects
 *	 key: GROWL_NOTIFICATIONS_DEFAULT	object: <code>NSArray</code> of <code>NSString</code> objects
 *
 *	 You do not need to implement this method if you have an auto-discoverable
 *	 plist file in your app bundle. (XXX refer to more information on that)
 *
 *	@result The <code>NSDictionary</code> to use for registration.
 */
- (NSDictionary *) registrationDictionaryForGrowl;

/*!
 *	@method applicationNameForGrowl
 *	@abstract Return the name of this application which will be used for Growl bookkeeping.
 *	@discussion This name is used both internally and in the Growl preferences.
 *
 *	 This should remain stable between different versions and incarnations of
 *	 your application.
 *	 For example, "SurfWriter" is a good app name, whereas "SurfWriter 2.0" and
 *	 "SurfWriter Lite" are not.
 *
 *	 You do not need to implement this method if you are providing the
 *	 application name elsewhere, meaning in an auto-discoverable plist file in
 *	 your app bundle (XXX refer to more information on that) or in the result
 *	 of -registrationDictionaryForGrowl.
 *
 *	@result The name of the application using Growl.
 */
- (NSString *) applicationNameForGrowl;

/*!
 *	@method applicationIconDataForGrowl
 *	@abstract Return the <code>NSData</code> to treat as the application icon.
 *	@discussion The delegate may optionally return an <code>NSData</code>
 *	 object to use as the application icon; if this is not implemented, the
 *	 application's own icon is used.  This is not generally needed.
 *	@result The <code>NSData</code> to treat as the application icon.
 */
- (NSData *) applicationIconDataForGrowl;

/*!
 *	@method growlIsReady
 *	@abstract Informs the delegate that Growl has launched.
 *	@discussion Informs the delegate that Growl (specifically, the
 *	 GrowlHelperApp) was launched successfully or was already running.  The
 *	 application can take actions with the knowledge that Growl is installed and
 *	 functional.
 */
- (void) growlIsReady;

/*!
 *	@method growlNotificationWasClicked:
 *	@abstract Informs the delegate that a Growl notification was clicked.
 *	@discussion Informs the delegate that a Growl notification was clicked.  It
 *	 is only sent for notifications sent with a non-<code>nil</code>
 *	 clickContext, so if you want to receive a message when a notification is
 *	 clicked, clickContext must not be <code>nil</code> when calling
 *	 <code>+[GrowlApplicationBridge notifyWithTitle: description:notificationName:iconData:priority:isSticky:clickContext:]</code>.
 *	@param clickContext The clickContext passed when displaying the notification originally via +[GrowlApplicationBridge notifyWithTitle:description:notificationName:iconData:priority:isSticky:clickContext:].
 */
- (void) growlNotificationWasClicked:(id)clickContext;

/*!
 *	@method growlNotificationTimedOut:
 *	@abstract Informs the delegate that a Growl notification timed out.
 *	@discussion Informs the delegate that a Growl notification timed out. It
 *	 is only sent for notifications sent with a non-<code>nil</code>
 *	 clickContext, so if you want to receive a message when a notification is
 *	 clicked, clickContext must not be <code>nil</code> when calling
 *	 <code>+[GrowlApplicationBridge notifyWithTitle: description:notificationName:iconData:priority:isSticky:clickContext:]</code>.
 *	@param clickContext The clickContext passed when displaying the notification originally via +[GrowlApplicationBridge notifyWithTitle:description:notificationName:iconData:priority:isSticky:clickContext:].
 */
- (void) growlNotificationTimedOut:(id)clickContext;

@end

#pragma mark -
/*!
 *	@category NSObject(GrowlApplicationBridgeDelegate_Installation_InformalProtocol)
 *	@abstract Methods which may be optionally implemented by the Growl delegate when used with Growl-WithInstaller.framework.
 *	@discussion The methods in this informal protocol will only be called if
 *	 implemented by the delegate.  They allow greater control of the information
 *	 presented to the user when installing or upgrading Growl from within your
 *	 application when using Growl-WithInstaller.framework.
 */
@interface NSObject (GrowlApplicationBridgeDelegate_Installation_InformalProtocol)

/*!
 *	@method growlInstallationWindowTitle
 *	@abstract Return the title of the installation window.
 *	@discussion If not implemented, Growl will use a default, localized title.
 *	@result An NSString object to use as the title.
 */
- (NSString *)growlInstallationWindowTitle;

/*!
 *	@method growlUpdateWindowTitle
 *	@abstract Return the title of the upgrade window.
 *	@discussion If not implemented, Growl will use a default, localized title.
 *	@result An NSString object to use as the title.
 */
- (NSString *)growlUpdateWindowTitle;

/*!
 *	@method growlInstallationInformation
 *	@abstract Return the information to display when installing.
 *	@discussion This information may be as long or short as desired (the window
 *	 will be sized to fit it).  It will be displayed to the user as an
 *	 explanation of what Growl is and what it can do in your application.  It
 *	 should probably note that no download is required to install.
 *
 *	 If this is not implemented, Growl will use a default, localized explanation.
 *	@result An NSAttributedString object to display.
 */
- (NSAttributedString *)growlInstallationInformation;

/*!
 *	@method growlUpdateInformation
 *	@abstract Return the information to display when upgrading.
 *	@discussion This information may be as long or short as desired (the window
 *	 will be sized to fit it).  It will be displayed to the user as an
 *	 explanation that an updated version of Growl is included in your
 *	 application and no download is required.
 *
 *	 If this is not implemented, Growl will use a default, localized explanation.
 *	@result An NSAttributedString object to display.
 */
- (NSAttributedString *)growlUpdateInformation;

@end

//private
@interface GrowlApplicationBridge (GrowlInstallationPrompt_private)
+ (void) _userChoseNotToInstallGrowl;
@end

#endif /* __GrowlApplicationBridge_h__ */
