/*  HBUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBSecurityAccessToken.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSURL (HBSecurityScope) <HBSecurityScope>
@end

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
 *  Returns the default destination URL
 */
+ (nullable NSURL *)defaultDestinationFolderURL;


/**
 * Returns the url of the documentation.
 */
@property (nonatomic, readonly, class) NSURL *documentationURL;


/**
 * Returns the url of the current English version documentation.
 */
@property (nonatomic, readonly, class) NSURL *documentationBaseURL;

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

/// Find the common base paths of an array of NSURL
/// - Parameter fileURLs: an array of NSURL
+ (NSArray<NSURL *> *)baseURLs:(NSArray<NSURL *> *)fileURLs;

/// Expand an array of NSURL into an array that contains all the top level
/// files and content of the top level folders
/// If the recursive option is set, recursively expand every subfolder.
/// If the input is a single folder, check if it's a DVD-Video and Bluray
/// and ignore the sub folders.
/// - Parameters:
///   - fileURLs: an array of NSURL
///   - recursive: whether to recursively expand sub folders
+ (NSArray<NSURL *> *)expandURLs:(NSArray<NSURL *> *)fileURLs recursive:(BOOL)recursive;

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
