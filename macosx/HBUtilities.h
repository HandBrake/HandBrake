/*  HBUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBJob;

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
 *  Writes a message to standard error.
 *  The message will show up in the output panel and in the activity log.
 *
 *  @param format a standard c format string with varargs.
 */
+ (void)writeToActivityLog:(const char *)format, ...;

+ (nullable NSURL *)URLFromBookmark:(NSData *)bookmark;
+ (nullable NSData *)bookmarkFromURL:(NSURL *)url;
+ (nullable NSData *)bookmarkFromURL:(NSURL *)url options:(NSURLBookmarkCreationOptions)options;

+ (NSURL *)mediaURLFromURL:(NSURL *)URL;

+ (NSString *)automaticNameForJob:(HBJob *)job;
+ (NSString *)automaticExtForJob:(HBJob *)job;
+ (NSString *)defaultNameForJob:(HBJob *)job;

/**
 *  Generates a file name automatically based on the inputs,
 *  it can be configured with NSUserDefaults.
 *
 *  @param sourceName    the name of the source file
 *  @param title         the title number
 *  @param chaptersRange the selected chapters range
 *  @param quality       the video encoder quality
 *  @param bitrate       the video encoder bitrate
 *  @param videoCodec    the video encoder type
 *
 *  @return a NSString containing the required info
 */
+ (NSString *)automaticNameForSource:(NSString *)sourceName
                               title:(NSUInteger)title
                            chapters:(NSRange)chaptersRange
                             quality:(double)quality
                             bitrate:(int)bitrate
                          videoCodec:(uint32_t)codec
                        creationDate:(NSDate *)creationDate;

+ (NSString *)isoCodeForNativeLang:(NSString *)language;
+ (NSString *)iso6392CodeFor:(NSString *)language;
+ (NSString *)languageCodeForIso6392Code:(NSString *)language;

@end

NS_ASSUME_NONNULL_END
