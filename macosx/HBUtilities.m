/*  HBUtilities.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBUtilities.h"
#import <Cocoa/Cocoa.h>

#include "handbrake/lang.h"

static BOOL hb_resolveBookmarks = YES;

@implementation HBUtilities

+ (NSString *)handBrakeVersion
{
    NSDictionary *infoDictionary = NSBundle.mainBundle.infoDictionary;
    return [NSString stringWithFormat:@"Handbrake Version: %@ (%@)",
            infoDictionary[@"CFBundleShortVersionString"],
            infoDictionary[@"CFBundleVersion"]];
}

+ (NSURL *)appSupportURL
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSURL *appSupportURL = [[[fileManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask]
                             firstObject] URLByAppendingPathComponent:@"HandBrake"];

    if (appSupportURL && ![fileManager fileExistsAtPath:appSupportURL.path])
    {
        [fileManager createDirectoryAtPath:appSupportURL.path withIntermediateDirectories:YES attributes:nil error:NULL];
    }

    return appSupportURL;
}

+ (NSURL *)documentationURL
{
    return [NSURL URLWithString:@"https://handbrake.fr/docs/en/1.3.0/"];
}

+ (void)writeToActivityLog:(const char *)format, ...
{
    va_list args;
    va_start(args, format);
    if (format != nil)
    {
        char str[1024];
        vsnprintf(str, 1024, format, args);

        time_t _now = time(NULL);
        struct tm *now  = localtime(&_now);
        fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, str);
    }
    va_end(args);
}

+ (void)writeErrorToActivityLog:(NSError *)error
{
    [self writeToActivityLog:"Error domain: %s", error.domain.UTF8String];
    [self writeToActivityLog:"Error code: %d", error.code];
    if (error.localizedDescription)
    {
        [self writeToActivityLog:"Error description: %s", error.localizedDescription.UTF8String];
    }
    if (error.debugDescription)
    {
        [self writeToActivityLog:"Error debug description: %s", error.debugDescription.UTF8String];
    }
}

+ (void)writeToActivityLogWithNoHeader:(NSString *)text
{
    fprintf(stderr, "%s", text.UTF8String);
}

+ (void)setResolveBookmarks:(BOOL)resolveBookmarks
{
    hb_resolveBookmarks = resolveBookmarks;
}

+ (BOOL)resolveBookmarks
{
    return hb_resolveBookmarks;
}

+ (nullable NSURL *)URLFromBookmark:(NSData *)bookmark
{
    if (hb_resolveBookmarks == NO)
    {
        return nil;
    }

    NSParameterAssert(bookmark);

    NSError *error;
    BOOL isStale;

    NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];

    if (error)
    {
        [HBUtilities writeErrorToActivityLog:error];
    }

    return isStale ? nil : url;
}

+ (nullable NSData *)bookmarkFromURL:(NSURL *)url options:(NSURLBookmarkCreationOptions)options
{
    NSParameterAssert(url);

    NSError *error;

    NSData *bookmark = [url bookmarkDataWithOptions:options includingResourceValuesForKeys:nil relativeToURL:nil error:&error];

    if (error)
    {
        NSString *error_message = [NSString stringWithFormat:@"Failed to create bookmark: %@", error];
        [HBUtilities writeToActivityLog:"%s", error_message.UTF8String];
    }

    return bookmark;
}

+ (nullable NSData *)bookmarkFromURL:(NSURL *)url
{
    return [HBUtilities bookmarkFromURL:url options:NSURLBookmarkCreationWithSecurityScope];
}

+ (NSURL *)mediaURLFromURL:(NSURL *)URL
{
    NSURL *mediaURL = URL;

    // We check to see if the chosen file at path is a package
    if ([NSWorkspace.sharedWorkspace isFilePackageAtPath:URL.path])
    {
        [HBUtilities writeToActivityLog:"trying to open a package at: %s", URL.path.UTF8String];
        // We check to see if this is an .eyetv package
        if ([URL.pathExtension isEqualToString:@"eyetv"])
        {
            [HBUtilities writeToActivityLog:"trying to open eyetv package"];
            // We're looking at an EyeTV package - try to open its enclosed .mpg media file
            NSString *mpgname;
            NSUInteger n = [[URL.path stringByAppendingString: @"/"]
                            completePathIntoString: &mpgname caseSensitive: YES
                            matchesIntoArray: nil
                            filterTypes: @[@"mpg"]];
            if (n > 0)
            {
                // Found an mpeg inside the eyetv package, make it our scan path
                [HBUtilities writeToActivityLog:"found mpeg in eyetv package"];
                mediaURL = [NSURL fileURLWithPath:mpgname];
            }
            else
            {
                // We did not find an mpeg file in our package, so we do not call performScan
                [HBUtilities writeToActivityLog:"no valid mpeg in eyetv package"];
            }
        }
        // We check to see if this is a .dvdmedia package
        else if ([URL.pathExtension isEqualToString:@"dvdmedia"])
        {
            // path IS a package - but dvdmedia packages can be treaded like normal directories
            [HBUtilities writeToActivityLog:"trying to open dvdmedia package"];
        }
        else
        {
            // The package is not an eyetv package, try to open it anyway
            [HBUtilities writeToActivityLog:"not a known to package"];
        }
    }
#ifndef __SANDBOX_ENABLED__
    else
    {
        // path is not a package, so we call perform scan directly on our file
        if ([URL.lastPathComponent isEqualToString:@"VIDEO_TS"])
        {
            [HBUtilities writeToActivityLog:"trying to open video_ts folder (video_ts folder chosen)"];
            // If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
            mediaURL = URL.URLByDeletingLastPathComponent;
        }
        else
        {
            [HBUtilities writeToActivityLog:"trying to open a folder or file"];
        }
    }
#endif

    return mediaURL;
}

+ (NSString *)isoCodeForNativeLang:(NSString *)language
{
    const iso639_lang_t *lang = lang_get_next(NULL);
    for (lang = lang_get_next(lang); lang != NULL; lang = lang_get_next(lang))
    {
        NSString *nativeLanguage = strlen(lang->native_name) ? @(lang->native_name) : @(lang->eng_name);

        if ([language isEqualToString:nativeLanguage])
        {
            return @(lang->iso639_2);
        }
    }

    return @"und";
}

+ (NSString *)iso6392CodeFor:(NSString *)aLanguage
{
    iso639_lang_t *lang = lang_for_english(aLanguage.UTF8String);
    if (lang)
    {
        return @(lang->iso639_2);
    }
    return @"und";
}

+ (NSString *)languageCodeForIso6392Code:(NSString *)aLanguage
{
    iso639_lang_t *lang = lang_for_code2(aLanguage.UTF8String);
    if (lang)
    {
        return @(lang->eng_name);
    }
    return @"Unknown";
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_14
enum {
    errAEEventWouldRequireUserConsent = -1744,
};
#endif

+ (HBPrivacyConsentState)determinePermissionToAutomateTarget:(NSString *)bundleIdentifier promptIfNeeded:(BOOL)promptIfNeeded
{
    if (@available(macOS 10.14, *))
    {
        const char *identifierCString = bundleIdentifier.UTF8String;
        AEAddressDesc addressDesc;

        if ([bundleIdentifier isEqualToString:@"com.apple.systemevents"])
        {
            // Mare sure system events is running, if not the consent alert will not be shown.
            BOOL result = [NSWorkspace.sharedWorkspace launchAppWithBundleIdentifier:bundleIdentifier options:0 additionalEventParamDescriptor:nil launchIdentifier:NULL];
            if (result == NO)
            {
                [HBUtilities writeToActivityLog:"Automation: couldn't launch %s.", bundleIdentifier.UTF8String];
            }
        }

        OSErr descResult = AECreateDesc(typeApplicationBundleID, identifierCString, strlen(identifierCString), &addressDesc);

        if (descResult == noErr)
        {
            OSStatus permission = AEDeterminePermissionToAutomateTarget(&addressDesc, typeWildCard, typeWildCard, promptIfNeeded);
            AEDisposeDesc(&addressDesc);

            HBPrivacyConsentState result;

            switch (permission)
            {
                case errAEEventWouldRequireUserConsent:
                    [HBUtilities writeToActivityLog:"Automation: request user consent for %s.", bundleIdentifier.UTF8String];
                    result = HBPrivacyConsentStateUnknown;
                    break;
                case noErr:
                    [HBUtilities writeToActivityLog:"Automation: permission granted for %s.", bundleIdentifier.UTF8String];
                    result = HBPrivacyConsentStateGranted;
                    break;
                case errAEEventNotPermitted:
                    [HBUtilities writeToActivityLog:"Automation: permission not granted for %s.", bundleIdentifier.UTF8String];
                    result = HBPrivacyConsentStateDenied;
                    break;
                case procNotFound:
                default:
                    [HBUtilities writeToActivityLog:"Automation: permission unknown."];
                    result = HBPrivacyConsentStateUnknown;
                    break;
            }
            return result;
        }
    }

    return HBPrivacyConsentStateGranted;
}

@end
