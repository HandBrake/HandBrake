//
//  GrowlApplicationBridge-Carbon.h
//  Growl
//
//  Created by Peter Hosey on Wed Jun 18 2004.
//  Based on GrowlApplicationBridge.h by Evan Schoenberg.
//  This source code is in the public domain. You may freely link it into any
//    program.
//

#ifndef _GROWLAPPLICATIONBRIDGE_CARBON_H_
#define _GROWLAPPLICATIONBRIDGE_CARBON_H_

#include <sys/cdefs.h>
#include <Carbon/Carbon.h>

#ifndef GROWL_EXPORT
#define GROWL_EXPORT __attribute__((visibility("default"))) DEPRECATED_ATTRIBUTE
#endif

/*!	@header GrowlApplicationBridge-Carbon.h
 *	@abstract	Declares an API that Carbon applications can use to interact with Growl.
 *	@discussion	GrowlApplicationBridge uses a delegate to provide information //XXX
 *	 to Growl (such as your application's name and what notifications it may
 *	 post) and to provide information to your application (such as that Growl
 *	 is listening for notifications or that a notification has been clicked).
 *
 *	 You can set the Growldelegate with Growl_SetDelegate and find out the
 *	 current delegate with Growl_GetDelegate. See struct Growl_Delegate for more
 *	 information about the delegate.
 */

__BEGIN_DECLS

/*!	@struct Growl_Delegate
 *	@abstract Delegate to supply GrowlApplicationBridge with information and respond to events.
 *	@discussion The Growl delegate provides your interface to
 *	 GrowlApplicationBridge. When GrowlApplicationBridge needs information about
 *	 your application, it looks for it in the delegate; when Growl or the user
 *	 does something that you might be interested in, GrowlApplicationBridge
 *	 looks for a callback in the delegate and calls it if present
 *	 (meaning, if it is not <code>NULL</code>).
 *	XXX on all of that
 *  @field size The size of the delegate structure.
 * 	@field applicationName The name of your application.
 * 	@field registrationDictionary A dictionary describing your application and the notifications it can send out.
 * 	@field applicationIconData Your application's icon.
 * 	@field growlInstallationWindowTitle The title of the installation window.
 * 	@field growlInstallationInformation Text to display in the installation window.
 * 	@field growlUpdateWindowTitle The title of the update window.
 * 	@field growlUpdateInformation Text to display in the update window.
 * 	@field referenceCount A count of owners of the delegate.
 * 	@field retain Called when GrowlApplicationBridge receives this delegate.
 * 	@field release Called when GrowlApplicationBridge no longer needs this delegate.
 * 	@field growlIsReady Called when GrowlHelperApp is listening for notifications.
 * 	@field growlNotificationWasClicked Called when a Growl notification is clicked.
 *  @field growlNotificationTimedOut Called when a Growl notification timed out.
 */
struct Growl_Delegate {
	/*	@discussion This should be sizeof(struct Growl_Delegate).
	 */
	size_t size;

	/*All of these attributes are optional.
	 *Optional attributes can be NULL; required attributes that
	 *	 are NULL cause setting the Growl delegate to fail.
	 *XXX - move optional/required status into the discussion for each field
	 */

	/* This name is used both internally and in the Growl preferences.
	 *
	 *	 This should remain stable between different versions and incarnations of
	 *	 your application.
	 *	 For example, "SurfWriter" is a good app name, whereas "SurfWriter 2.0" and
	 *	 "SurfWriter Lite" are not.
	 *
	 * This can be <code>NULL</code> if it is provided elsewhere, namely in an
	 *	 auto-discoverable plist file in your app bundle
	 *	 (XXX refer to more information on that) or in registrationDictionary.
	 */
	CFStringRef applicationName;

	/*
	 * Must contain at least these keys:
	 *	GROWL_NOTIFICATIONS_ALL (CFArray):
	 *		Contains the names of all notifications your application may post.
	 *
	 * Can also contain these keys:
	 *	GROWL_NOTIFICATIONS_DEFAULT (CFArray):
	 *		Names of notifications that should be enabled by default.
	 *		If omitted, GROWL_NOTIFICATIONS_ALL will be used.
	 *	GROWL_APP_NAME (CFString):
	 *		Same as the applicationName member of this structure.
	 *		If both are present, the applicationName member shall prevail.
	 *		If this key is present, you may omit applicationName (set it to <code>NULL</code>).
	 *	GROWL_APP_ICON (CFData):
	 *		Same as the iconData member of this structure.
	 *		If both are present, the iconData member shall prevail.
	 *		If this key is present, you may omit iconData (set it to <code>NULL</code>).
	 *
	 * If you change the contents of this dictionary after setting the delegate,
	 *	be sure to call Growl_Reregister.
	 *
	 * This can be <code>NULL</code> if you have an auto-discoverable plist file in your app
	 *	 bundle. (XXX refer to more information on that)
	 */
	CFDictionaryRef registrationDictionary;

	/* The data can be in any format supported by NSImage. As of
	 *	 Mac OS X 10.3, this includes the .icns, TIFF, JPEG, GIF, PNG, PDF, and
	 *	 PICT formats.
	 *
	 *	 If this is not supplied, Growl will look up your application's icon by
	 *	 its application name.
	 */
	CFDataRef applicationIconData;

	/* Installer display attributes
	 *
	 * These four attributes are used by the Growl installer, if this framework
	 *	supports it.
	 * For any of these being <code>NULL</code>, a localised default will be
	 *	supplied.
	 */

	/*	If this is <code>NULL</code>, Growl will use a default,
	 *	 localized title.
	 *
	 *	 Only used if you're using Growl-WithInstaller.framework. Otherwise,
	 *	 this member is ignored.
	 */
	CFStringRef growlInstallationWindowTitle;
	/*	This information may be as long or short as desired (the
	 *	 window will be sized to fit it).  If Growl is not installed, it will
	 *	 be displayed to the user as an explanation of what Growl is and what
	 *	 it can do in your application.
	 *	 It should probably note that no download is required to install.
	 *
	 *	 If this is <code>NULL</code>, Growl will use a default, localized
	 *	 explanation.
	 *
	 *	 Only used if you're using Growl-WithInstaller.framework. Otherwise,
	 *	 this member is ignored.
	 */
	CFStringRef growlInstallationInformation;
	/*	If this is <code>NULL</code>, Growl will use a default,
	 *	 localized title.
	 *
	 *	 Only used if you're using Growl-WithInstaller.framework. Otherwise,
	 *	 this member is ignored.
	 */
	CFStringRef growlUpdateWindowTitle;
	/*	This information may be as long or short as desired (the
	 *	 window will be sized to fit it).  If an older version of Growl is
	 *	 installed, it will be displayed to the user as an explanation that an
	 *	 updated version of Growl is included in your application and
	 *	 no download is required.
	 *
	 *	 If this is <code>NULL</code>, Growl will use a default, localized
	 *	 explanation.
	 *
	 *	 Only used if you're using Growl-WithInstaller.framework. Otherwise,
	 *	 this member is ignored.
	 */
	CFStringRef growlUpdateInformation;

	/*	This member is provided for use by your retain and release
	 *	 callbacks (see below).
	 *
	 *	 GrowlApplicationBridge never directly uses this member. Instead, it
	 *	 calls your retain callback (if non-<code>NULL</code>) and your release
	 *	 callback (if non-<code>NULL</code>).
	 */
	unsigned referenceCount;

	//Functions. Currently all of these are optional (any of them can be NULL).

	/*	When you call Growl_SetDelegate(newDelegate), it will call
	 *	 oldDelegate->release(oldDelegate), and then it will call
	 *	 newDelegate->retain(newDelegate), and the return value from retain
	 *	 is what will be set as the delegate.
	 *	 (This means that this member works like CFRetain and -[NSObject retain].)
	 *	 This member is optional (it can be <code>NULL</code>).
	 *	 For a delegate allocated with malloc, this member would be
	 *	 <code>NULL</code>.
	 *	@result	A delegate to which GrowlApplicationBridge holds a reference.
	 */
	void *(*retain)(void *);
	/*	When you call Growl_SetDelegate(newDelegate), it will call
	 *	 oldDelegate->release(oldDelegate), and then it will call
	 *	 newDelegate->retain(newDelegate), and the return value from retain
	 *	 is what will be set as the delegate.
	 *	 (This means that this member works like CFRelease and
	 *	  -[NSObject release].)
	 *	 This member is optional (it can be NULL).
	 *	 For a delegate allocated with malloc, this member might be
	 *	 <code>free</code>(3).
	 */
	void (*release)(void *);

	/*	Informs the delegate that Growl (specifically, the GrowlHelperApp) was
	 *	 launched successfully (or was already running). The application can
	 *	 take actions with the knowledge that Growl is installed and functional.
	 */
	void (*growlIsReady)(void);

	/*	Informs the delegate that a Growl notification was clicked. It is only
	 *	 sent for notifications sent with a non-<code>NULL</code> clickContext,
	 *	 so if you want to receive a message when a notification is clicked,
	 *	 clickContext must not be <code>NULL</code> when calling
	 *	 Growl_PostNotification or
	 *	 Growl_NotifyWithTitleDescriptionNameIconPriorityStickyClickContext.
	 */
	void (*growlNotificationWasClicked)(CFPropertyListRef clickContext);

	/*	Informs the delegate that a Growl notification timed out. It is only
	 *	 sent for notifications sent with a non-<code>NULL</code> clickContext,
	 *	 so if you want to receive a message when a notification is clicked,
	 *	 clickContext must not be <code>NULL</code> when calling
	 *	 Growl_PostNotification or
	 *	 Growl_NotifyWithTitleDescriptionNameIconPriorityStickyClickContext.
	 */
	void (*growlNotificationTimedOut)(CFPropertyListRef clickContext);
};

/*!	@struct Growl_Notification
 *	@abstract Structure describing a Growl notification.
 *	@discussion XXX
 * 	@field size The size of the notification structure.
 * 	@field name Identifies the notification.
 * 	@field title Short synopsis of the notification.
 *  @field description Additional text.
 * 	@field iconData An icon for the notification.
 * 	@field priority An indicator of the notification's importance.
 * 	@field reserved Bits reserved for future usage.
 * 	@field isSticky Requests that a notification stay on-screen until dismissed explicitly.
 * 	@field clickContext An identifier to be passed to your click callback when a notification is clicked.
 * 	@field clickCallback A callback to call when the notification is clicked.
 */
struct Growl_Notification {
	/*	This should be sizeof(struct Growl_Notification).
	 */
 	size_t size;

	/*	The notification name distinguishes one type of
	 *	 notification from another. The name should be human-readable, as it
	 *	 will be displayed in the Growl preference pane.
	 *
	 *	 The name is used in the GROWL_NOTIFICATIONS_ALL and
	 *	 GROWL_NOTIFICATIONS_DEFAULT arrays in the registration dictionary, and
	 *	 in this member of the Growl_Notification structure.
	 */
	CFStringRef name;

	/*	A notification's title describes the notification briefly.
	 *	 It should be easy to read quickly by the user.
	 */
	CFStringRef title;

	/*	The description supplements the title with more
	 *	 information. It is usually longer and sometimes involves a list of
	 *	 subjects. For example, for a 'Download complete' notification, the
	 *	 description might have one filename per line. GrowlMail in Growl 0.6
	 *	 uses a description of '%d new mail(s)' (formatted with the number of
	 *	 messages).
	 */
	CFStringRef description;

	/*	The notification icon usually indicates either what
	 *	 happened (it may have the same icon as e.g. a toolbar item that
	 *	 started the process that led to the notification), or what it happened
	 *	 to (e.g. a document icon).
	 *
	 *	 The icon data is optional, so it can be <code>NULL</code>. In that
	 *	 case, the application icon is used alone. Not all displays support
	 *	 icons.
	 *
	 *	 The data can be in any format supported by NSImage. As of Mac OS X
	 *	 10.3, this includes the .icns, TIFF, JPEG, GIF, PNG, PDF, and PICT form
	 *	 ats.
	 */
	CFDataRef iconData;

	/*	Priority is new in Growl 0.6, and is represented as a
	 *	 signed integer from -2 to +2. 0 is Normal priority, -2 is Very Low
	 *	 priority, and +2 is Very High priority.
	 *
	 *	 Not all displays support priority. If you do not wish to assign a
	 *	 priority to your notification, assign 0.
	 */
	signed int priority;

	/*	These bits are not used in Growl 0.6. Set them to 0.
	 */
	unsigned reserved: 31;

	/*	When the sticky bit is clear, in most displays,
	 *	 notifications disappear after a certain amount of time. Sticky
	 *	 notifications, however, remain on-screen until the user dismisses them
	 *	 explicitly, usually by clicking them.
	 *
	 *	 Sticky notifications were introduced in Growl 0.6. Most notifications
	 *	 should not be sticky. Not all displays support sticky notifications,
	 *	 and the user may choose in Growl's preference pane to force the
	 *	 notification to be sticky or non-sticky, in which case the sticky bit
	 *	 in the notification will be ignored.
	 */
	unsigned isSticky: 1;

	/*	If this is not <code>NULL</code>, and your click callback
	 *	 is not <code>NULL</code> either, this will be passed to the callback
	 *	 when your notification is clicked by the user.
	 *
	 *	 Click feedback was introduced in Growl 0.6, and it is optional. Not
	 *	 all displays support click feedback.
	 */
	CFPropertyListRef clickContext;

	/*	If this is not <code>NULL</code>, it will be called instead
	 *	 of the Growl delegate's click callback when clickContext is
	 *	 non-<code>NULL</code> and the notification is clicked on by the user.
	 *
	 *	 Click feedback was introduced in Growl 0.6, and it is optional. Not
	 *	 all displays support click feedback.
	 *
	 *	 The per-notification click callback is not yet supported as of Growl
	 *	 0.7.
	 */
	void (*clickCallback)(CFPropertyListRef clickContext);

	CFStringRef identifier;
};

#pragma mark -
#pragma mark Easy initialisers

/*!	@defined	InitGrowlDelegate
 *	@abstract	Callable macro. Initializes a Growl delegate structure to defaults.
 *	@discussion	Call with a pointer to a struct Growl_Delegate. All of the
 *	 members of the structure will be set to 0 or <code>NULL</code>, except for
 *	 size (which will be set to <code>sizeof(struct Growl_Delegate)</code>) and
 *	 referenceCount (which will be set to 1).
 */
#define InitGrowlDelegate(delegate) \
	do { \
		if (delegate) { \
			(delegate)->size = sizeof(struct Growl_Delegate); \
			(delegate)->applicationName = NULL; \
			(delegate)->registrationDictionary = NULL; \
			(delegate)->applicationIconData = NULL; \
			(delegate)->growlInstallationWindowTitle = NULL; \
			(delegate)->growlInstallationInformation = NULL; \
			(delegate)->growlUpdateWindowTitle = NULL; \
			(delegate)->growlUpdateInformation = NULL; \
			(delegate)->referenceCount = 1U; \
			(delegate)->retain = NULL; \
			(delegate)->release = NULL; \
			(delegate)->growlIsReady = NULL; \
			(delegate)->growlNotificationWasClicked = NULL; \
			(delegate)->growlNotificationTimedOut = NULL; \
		} \
	} while(0)

/*!	@defined	InitGrowlNotification
 *	@abstract	Callable macro. Initializes a Growl notification structure to defaults.
 *	@discussion	Call with a pointer to a struct Growl_Notification. All of
 *	 the members of the structure will be set to 0 or <code>NULL</code>, except
 *	 for size (which will be set to
 *	<code>sizeof(struct Growl_Notification)</code>).
 */
#define InitGrowlNotification(notification) \
	do { \
		if (notification) { \
			(notification)->size = sizeof(struct Growl_Notification); \
			(notification)->name = NULL; \
			(notification)->title = NULL; \
			(notification)->description = NULL; \
			(notification)->iconData = NULL; \
			(notification)->priority = 0; \
			(notification)->reserved = 0U; \
			(notification)->isSticky = false; \
			(notification)->clickContext = NULL; \
			(notification)->clickCallback = NULL; \
			(notification)->identifier = NULL; \
		} \
	} while(0)

#pragma mark -
#pragma mark Public API

//	@functiongroup	Managing the Growl delegate

/*!	@function	Growl_SetDelegate
 *	@abstract	Replaces the current Growl delegate with a new one, or removes
 *	 the Growl delegate.
 *	@param	newDelegate
 *	@result	Returns false and does nothing else if a pointer that was passed in
 *	 is unsatisfactory (because it is non-<code>NULL</code>, but at least one
 *	 required member of it is <code>NULL</code>). Otherwise, sets or unsets the
 *	 delegate and returns true.
 *	@discussion	When <code>newDelegate</code> is non-<code>NULL</code>, sets
 *	 the delegate to <code>newDelegate</code>. When it is <code>NULL</code>,
 *	 the current delegate will be unset, and no delegate will be in place.
 *
 *	 It is legal for <code>newDelegate</code> to be the current delegate;
 *	 nothing will happen, and Growl_SetDelegate will return true. It is also
 *	 legal for it to be <code>NULL</code>, as described above; again, it will
 *	 return true.
 *
 *	 If there was a delegate in place before the call, Growl_SetDelegate will
 *	 call the old delegate's release member if it was non-<code>NULL</code>. If
 *	 <code>newDelegate</code> is non-<code>NULL</code>, Growl_SetDelegate will
 *	 call <code>newDelegate->retain</code>, and set the delegate to its return
 *	 value.
 *
 *	 If you are using Growl-WithInstaller.framework, and an older version of
 *	 Growl is installed on the user's system, the user will automatically be
 *	 prompted to update.
 *
 *	 GrowlApplicationBridge currently does not copy this structure, nor does it
 *	 retain any of the CF objects in the structure (it regards the structure as
 *	 a container that retains the objects when they are added and releases them
 *	 when they are removed or the structure is destroyed). Also,
 *	 GrowlApplicationBridge currently does not modify any member of the
 *	 structure, except possibly the referenceCount by calling the retain and
 *	 release members.
 */
GROWL_EXPORT Boolean Growl_SetDelegate(struct Growl_Delegate *newDelegate);

/*!	@function	Growl_GetDelegate
 *	@abstract	Returns the current Growl delegate, if any.
 *	@result	The current Growl delegate.
 *	@discussion	Returns the last pointer passed into Growl_SetDelegate, or
 *	 <code>NULL</code> if no such call has been made.
 *
 *	 This function follows standard Core Foundation reference-counting rules.
 *	 Because it is a Get function, not a Copy function, it will not retain the
 *	 delegate on your behalf. You are responsible for retaining and releasing
 *	 the delegate as needed.
 */
GROWL_EXPORT struct Growl_Delegate *Growl_GetDelegate(void);

#pragma mark -

//	@functiongroup	Posting Growl notifications

/*!	@function	Growl_PostNotification
 *	@abstract	Posts a Growl notification.
 *	@param	notification	The notification to post.
 *	@discussion	This is the preferred means for sending a Growl notification.
 *	 The notification name and at least one of the title and description are
 *	 required (all three are preferred). All other parameters may be
 *	 <code>NULL</code> (or 0 or false as appropriate) to accept default values.
 *
 *	 If using the Growl-WithInstaller framework, if Growl is not installed the
 *	 user will be prompted to install Growl.
 *	 If the user cancels, this function will have no effect until the next
 *	 application session, at which time when it is called the user will be
 *	 prompted again. The user is also given the option to not be prompted again.
 *	 If the user does choose to install Growl, the requested notification will
 *	 be displayed once Growl is installed and running.
 */
GROWL_EXPORT void Growl_PostNotification(const struct Growl_Notification *notification);

/*!	@function Growl_PostNotificationWithDictionary
*	@abstract	Notifies using a userInfo dictionary suitable for passing to
*	 CFDistributedNotificationCenter.
*	@param	userInfo	The dictionary to notify with.
*	@discussion	Before Growl 0.6, your application would have posted
*	 notifications using CFDistributedNotificationCenter by creating a userInfo
*	 dictionary with the notification data. This had the advantage of allowing
*	 you to add other data to the dictionary for programs besides Growl that
*	 might be listening.
*
*	 This function allows you to use such dictionaries without being restricted
*	 to using CFDistributedNotificationCenter. The keys for this dictionary
 *	 can be found in GrowlDefines.h.
*/
GROWL_EXPORT void Growl_PostNotificationWithDictionary(CFDictionaryRef userInfo);

/*!	@function	Growl_NotifyWithTitleDescriptionNameIconPriorityStickyClickContext
 *	@abstract	Posts a Growl notification using parameter values.
 *	@param	title	The title of the notification.
 *	@param	description	The description of the notification.
 *	@param	notificationName	The name of the notification as listed in the
 *	 registration dictionary.
 *	@param	iconData	Data representing a notification icon. Can be <code>NULL</code>.
 *	@param	priority	The priority of the notification (-2 to +2, with -2
 *	 being Very Low and +2 being Very High).
 *	@param	isSticky	If true, requests that this notification wait for a
 *	 response from the user.
 *	@param	clickContext	An object to pass to the clickCallback, if any. Can
 *	 be <code>NULL</code>, in which case the clickCallback is not called.
 *	@discussion	Creates a temporary Growl_Notification, fills it out with the
 *	 supplied information, and calls Growl_PostNotification on it.
 *	 See struct Growl_Notification and Growl_PostNotification for more
 *	 information.
 *
 *	 The icon data can be in any format supported by NSImage. As of Mac OS X
 *	 10.3, this includes the .icns, TIFF, JPEG, GIF, PNG, PDF, and PICT formats.
 */
GROWL_EXPORT void Growl_NotifyWithTitleDescriptionNameIconPriorityStickyClickContext(
 /*inhale*/
	CFStringRef title,
	CFStringRef description,
	CFStringRef notificationName,
	CFDataRef iconData,
	signed int priority,
	Boolean isSticky,
	CFPropertyListRef clickContext);

#pragma mark -

//	@functiongroup	Registering

/*!	@function Growl_RegisterWithDictionary
 *	@abstract	Register your application with Growl without setting a delegate.
 *	@discussion	When you call this function with a dictionary,
 *	 GrowlApplicationBridge registers your application using that dictionary.
 *	 If you pass <code>NULL</code>, GrowlApplicationBridge will ask the delegate
 *	 (if there is one) for a dictionary, and if that doesn't work, it will look
 *	 in your application's bundle for an auto-discoverable plist.
 *	 (XXX refer to more information on that)
 *
 *	 If you pass a dictionary to this function, it must include the
 *	 <code>GROWL_APP_NAME</code> key, unless a delegate is set.
 *
 *	 This function is mainly an alternative to the delegate system introduced
 *	 with Growl 0.6. Without a delegate, you cannot receive callbacks such as
 *	 <code>growlIsReady</code> (since they are sent to the delegate). You can,
 *	 however, set a delegate after registering without one.
 *
 *	 This function was introduced in Growl.framework 0.7.
 *	@result <code>false</code> if registration failed (e.g. if Growl isn't installed).
 */
GROWL_EXPORT Boolean Growl_RegisterWithDictionary(CFDictionaryRef regDict);

/*!	@function	Growl_Reregister
 *	@abstract	Updates your registration with Growl.
 *	@discussion	If your application changes the contents of the
 *	 GROWL_NOTIFICATIONS_ALL key in the registrationDictionary member of the
 *	 Growl delegate, or if it changes the value of that member, or if it
 *	 changes the contents of its auto-discoverable plist, call this function
 *	 to have Growl update its registration information for your application.
 *
 *	 Otherwise, this function does not normally need to be called. If you're
 *	 using a delegate, your application will be registered when you set the
 *	 delegate if both the delegate and its registrationDictionary member are
 *	 non-<code>NULL</code>.
 *
 *	 This function is now implemented using
 *	 <code>Growl_RegisterWithDictionary</code>.
 */
GROWL_EXPORT void Growl_Reregister(void);

#pragma mark -

/*!	@function	Growl_SetWillRegisterWhenGrowlIsReady
 *	@abstract	Tells GrowlApplicationBridge to register with Growl when Growl
 *	 launches (or not).
 *	@discussion	When Growl has started listening for notifications, it posts a
 *	 <code>GROWL_IS_READY</code> notification on the Distributed Notification
 *	 Center. GrowlApplicationBridge listens for this notification, using it to
 *	 perform various tasks (such as calling your delegate's
 *	 <code>growlIsReady</code> callback, if it has one). If this function is
 *	 called with <code>true</code>, one of those tasks will be to reregister
 *	 with Growl (in the manner of <code>Growl_Reregister</code>).
 *
 *	 This attribute is automatically set back to <code>false</code>
 *	 (the default) after every <code>GROWL_IS_READY</code> notification.
 *	@param	flag	<code>true</code> if you want GrowlApplicationBridge to register with
 *	 Growl when next it is ready; <code>false</code> if not.
 */
GROWL_EXPORT void Growl_SetWillRegisterWhenGrowlIsReady(Boolean flag);
/*!	@function	Growl_WillRegisterWhenGrowlIsReady
 *	@abstract	Reports whether GrowlApplicationBridge will register with Growl
 *	 when Growl next launches.
 *	@result	<code>true</code> if GrowlApplicationBridge will register with
 *	 Growl when next it posts GROWL_IS_READY; <code>false</code> if not.
 */
GROWL_EXPORT Boolean Growl_WillRegisterWhenGrowlIsReady(void);

#pragma mark -

//	@functiongroup	Obtaining registration dictionaries

/*!	@function	Growl_CopyRegistrationDictionaryFromDelegate
 *	@abstract	Asks the delegate for a registration dictionary.
 *	@discussion	If no delegate is set, or if the delegate's
 *	 <code>registrationDictionary</code> member is <code>NULL</code>, this
 *	 function returns <code>NULL</code>.
 *
 *	 This function does not attempt to clean up the dictionary in any way - for
 *	 example, if it is missing the <code>GROWL_APP_NAME</code> key, the result
 *	 will be missing it too. Use
 *	 <code>Growl_CreateRegistrationDictionaryByFillingInDictionary</code> or
 *	 <code>Growl_CreateRegistrationDictionaryByFillingInDictionaryRestrictedToKeys</code>
 *	 to try to fill in missing keys.
 *
 *	 This function was introduced in Growl.framework 0.7.
 *	@result A registration dictionary.
 */
GROWL_EXPORT CFDictionaryRef Growl_CopyRegistrationDictionaryFromDelegate(void);

/*!	@function	Growl_CopyRegistrationDictionaryFromBundle
 *	@abstract	Looks in a bundle for a registration dictionary.
 *	@discussion	This function looks in a bundle for an auto-discoverable
 *	 registration dictionary file using <code>CFBundleCopyResourceURL</code>.
 *	 If it finds one, it loads the file using <code>CFPropertyList</code> and
 *	 returns the result.
 *
 *	 If you pass <code>NULL</code> as the bundle, the main bundle is examined.
 *
 *	 This function does not attempt to clean up the dictionary in any way - for
 *	 example, if it is missing the <code>GROWL_APP_NAME</code> key, the result
 *	 will be missing it too. Use
 *	 <code>Growl_CreateRegistrationDictionaryByFillingInDictionary:</code> or
 *	 <code>Growl_CreateRegistrationDictionaryByFillingInDictionaryRestrictedToKeys</code>
 *	 to try to fill in missing keys.
 *
 *	 This function was introduced in Growl.framework 0.7.
 *	@result A registration dictionary.
 */
GROWL_EXPORT CFDictionaryRef Growl_CopyRegistrationDictionaryFromBundle(CFBundleRef bundle);

/*!	@function	Growl_CreateBestRegistrationDictionary
 *	@abstract	Obtains a registration dictionary, filled out to the best of
 *	 GrowlApplicationBridge's knowledge.
 *	@discussion	This function creates a registration dictionary as best
 *	 GrowlApplicationBridge knows how.
 *
 *	 First, GrowlApplicationBridge examines the Growl delegate (if there is
 *	 one) and gets the registration dictionary from that. If no such dictionary
 *	 was obtained, GrowlApplicationBridge looks in your application's main
 *	 bundle for an auto-discoverable registration dictionary file. If that
 *	 doesn't exist either, this function returns <code>NULL</code>.
 *
 *	 Second, GrowlApplicationBridge calls
 *	 <code>Growl_CreateRegistrationDictionaryByFillingInDictionary</code> with
 *	 whatever dictionary was obtained. The result of that function is the
 *	 result of this function.
 *
 *	 GrowlApplicationBridge uses this function when you call
 *	 <code>Growl_SetDelegate</code>, or when you call
 *	 <code>Growl_RegisterWithDictionary</code> with <code>NULL</code>.
 *
 *	 This function was introduced in Growl.framework 0.7.
 *	@result	A registration dictionary.
 */
GROWL_EXPORT CFDictionaryRef Growl_CreateBestRegistrationDictionary(void);

#pragma mark -

//	@functiongroup	Filling in registration dictionaries

/*!	@function	Growl_CreateRegistrationDictionaryByFillingInDictionary
 *	@abstract	Tries to fill in missing keys in a registration dictionary.
 *	@param	regDict	The dictionary to fill in.
 *	@result	The dictionary with the keys filled in.
 *	@discussion	This function examines the passed-in dictionary for missing keys,
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
 *	 This function was introduced in Growl.framework 0.7.
 */
GROWL_EXPORT CFDictionaryRef Growl_CreateRegistrationDictionaryByFillingInDictionary(CFDictionaryRef regDict);
/*!	@function	Growl_CreateRegistrationDictionaryByFillingInDictionaryRestrictedToKeys
 *	@abstract	Tries to fill in missing keys in a registration dictionary.
 *	@param	regDict	The dictionary to fill in.
 *	@param	keys	The keys to fill in. If <code>NULL</code>, any missing keys are filled in.
 *	@result	The dictionary with the keys filled in.
 *	@discussion	This function examines the passed-in dictionary for missing keys,
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
 *	 This function was introduced in Growl.framework 0.7.
 */
GROWL_EXPORT CFDictionaryRef Growl_CreateRegistrationDictionaryByFillingInDictionaryRestrictedToKeys(CFDictionaryRef regDict, CFSetRef keys);

/*!	@brief	Tries to fill in missing keys in a notification dictionary.
 *	@param	notifDict	The dictionary to fill in.
 *	@return	The dictionary with the keys filled in. This will be a separate instance from \a notifDict.
 *	@discussion	This function examines the \a notifDict for missing keys, and 
 *	 tries to get them from the last known registration dictionary. As of 1.1, 
 *	 the keys that it will look for are:
 *
 *	 \li <code>GROWL_APP_NAME</code>
 *	 \li <code>GROWL_APP_ICON</code>
 *
 *	@since Growl.framework 1.1
 */
GROWL_EXPORT CFDictionaryRef Growl_CreateNotificationDictionaryByFillingInDictionary(CFDictionaryRef notifDict);

#pragma mark -

//	@functiongroup	Querying Growl's status

/*!	@function	Growl_IsInstalled
 *	@abstract	Determines whether the Growl prefpane and its helper app are
 *	 installed.
 *	@result	Returns true if Growl is installed, false otherwise.
 */
GROWL_EXPORT Boolean Growl_IsInstalled(void);

/*!	@function	Growl_IsRunning
 *	@abstract	Cycles through the process list to find whether GrowlHelperApp
 *	 is running.
 *	@result	Returns true if Growl is running, false otherwise.
 */
GROWL_EXPORT Boolean Growl_IsRunning(void);

#pragma mark -

//	@functiongroup	Launching Growl

/*!	@typedef	GrowlLaunchCallback
 *	@abstract	Callback to notify you that Growl is running.
 *	@param	context	The context pointer passed to Growl_LaunchIfInstalled.
 *	@discussion	Growl_LaunchIfInstalled calls this callback function if Growl
 *	 was already running or if it launched Growl successfully.
 */
typedef void (*GrowlLaunchCallback)(void *context);

/*!	@function	Growl_LaunchIfInstalled
 *	@abstract	Launches GrowlHelperApp if it is not already running.
 *	@param	callback	A callback function which will be called if Growl was successfully
 *	 launched or was already running. Can be <code>NULL</code>.
 *	@param	context	The context pointer to pass to the callback. Can be <code>NULL</code>.
 *	@result	Returns true if Growl was successfully launched or was already
 *	 running; returns false and does not call the callback otherwise.
 *	@discussion	Returns true and calls the callback (if the callback is not
 *	 <code>NULL</code>) if the Growl helper app began launching or was already
 *	 running. Returns false and performs no other action if Growl could not be
 *	 launched (e.g. because the Growl preference pane is not properly installed).
 *
 *	 If <code>Growl_CreateBestRegistrationDictionary</code> returns
 *	 non-<code>NULL</code>, this function will register with Growl atomically.
 *
 *	 The callback should take a single argument; this is to allow applications
 *	 to have context-relevant information passed back. It is perfectly
 *	 acceptable for context to be <code>NULL</code>. The callback itself can be
 *	 <code>NULL</code> if you don't want one.
 */
GROWL_EXPORT Boolean Growl_LaunchIfInstalled(GrowlLaunchCallback callback, void *context);

#pragma mark -
#pragma mark Constants

/*!	@defined	GROWL_PREFPANE_BUNDLE_IDENTIFIER
 *	@abstract	The CFBundleIdentifier of the Growl preference pane bundle.
 *	@discussion	GrowlApplicationBridge uses this to determine whether Growl is
 *	 currently installed, by searching for the Growl preference pane. Your
 *	 application probably does not need to use this macro itself.
 */
#ifndef GROWL_PREFPANE_BUNDLE_IDENTIFIER
#define GROWL_PREFPANE_BUNDLE_IDENTIFIER	CFSTR("com.growl.prefpanel")
#endif

__END_DECLS

#endif /* _GROWLAPPLICATIONBRIDGE_CARBON_H_ */
