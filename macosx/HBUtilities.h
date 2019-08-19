/*  HBUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBUtilities : NSObject

/**
 *  Returns a formatted string that contains the application version.
 */
+ (NSString *)handBrakeVersion;

/**
 *  Returns the url of the current <user>/Library/Application Support/HandBrake folder.
 */
+ (nullable NSURL *)appSupportURL;

/**
 * Returns the url of the current version documentation.
 */
@property (nonatomic, readonly, class) NSURL *documentationURL;

/**
 *  Writes a message to standard error.
 *  The message will show up in the output panel and in the activity log.
 *
 *  @param format a standard c format string with varargs.
 */
+ (void)writeToActivityLog:(const char *)format, ...;
+ (void)writeErrorToActivityLog:(NSError *)error;
+ (void)writeToActivityLogWithNoHeader:(NSString *)text;

/**
 Whether to resolve the security-scoped bookmarks or not.

 Security-scoped bookmarks can't be resolved in a XPC service.
 Use this options to avoid not useful errors
 */
@property (nonatomic, class, readwrite) BOOL resolveBookmarks;

+ (nullable NSURL *)URLFromBookmark:(NSData *)bookmark;
+ (nullable NSData *)bookmarkFromURL:(NSURL *)url;
+ (nullable NSData *)bookmarkFromURL:(NSURL *)url options:(NSURLBookmarkCreationOptions)options;

+ (NSURL *)mediaURLFromURL:(NSURL *)URL;

+ (NSString *)isoCodeForNativeLang:(NSString *)language;
+ (NSString *)iso6392CodeFor:(NSString *)language;
+ (NSString *)languageCodeForIso6392Code:(NSString *)language;

typedef NS_ENUM(NSUInteger, HBPrivacyConsentState) {
    HBPrivacyConsentStateUnknown,
    HBPrivacyConsentStateDenied,
    HBPrivacyConsentStateGranted,
};

+ (HBPrivacyConsentState)determinePermissionToAutomateTarget:(NSString *)bundleIdentifier promptIfNeeded:(BOOL)promptIfNeeded;

@end

NS_ASSUME_NONNULL_END
