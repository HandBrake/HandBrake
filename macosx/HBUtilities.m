/*  HBUtilities.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBUtilities.h"
#import "HBDirectUtilities.h"

#import <Cocoa/Cocoa.h>

#include "handbrake/lang.h"

static BOOL hb_resolveBookmarks = YES;

HB_OBJC_DIRECT_MEMBERS
@interface HBURLPair : NSObject
@property (nonatomic) NSURL *URL;
@property (nonatomic) NSUInteger length;
@property (nonatomic) NSURL *volumeURL;
@end

HB_OBJC_DIRECT_MEMBERS
@implementation HBURLPair
@end

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

+ (NSURL *)defaultDestinationFolderURL
{
    return [[NSFileManager.defaultManager URLsForDirectory:NSMoviesDirectory inDomains:NSUserDomainMask] firstObject];
}

+ (NSURL *)documentationURL
{
    return [NSURL URLWithString:@"https://handbrake.fr/docs/"];
}

+ (NSURL *)documentationBaseURL
{
    return [NSURL URLWithString:@"https://handbrake.fr/docs/en/latest/"];
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

    NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark
                                           options:NSURLBookmarkResolutionWithSecurityScope | NSURLBookmarkResolutionWithoutMounting
                                     relativeToURL:nil
                               bookmarkDataIsStale:&isStale error:&error];

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

+ (NSArray<NSURL *> *)baseURLs:(NSArray<NSURL *> *)fileURLs
{
    NSMutableArray<HBURLPair *> *pairs = [[NSMutableArray alloc] init];
    NSMutableSet<NSURL *> *volumeURLs = [[NSMutableSet alloc] init];

    for (NSURL *fileURL in fileURLs)
    {
        NSURL *volumeURL = nil;
        [fileURL getResourceValue:&volumeURL forKey:NSURLVolumeURLKey error:nil];

        if (volumeURL)
        {
            HBURLPair *pair = [[HBURLPair alloc] init];
            pair.URL = fileURL.URLByDeletingLastPathComponent;
            pair.length = fileURL.path.stringByDeletingLastPathComponent.length;
            pair.volumeURL = volumeURL;

            [pairs addObject:pair];
            [volumeURLs addObject:volumeURL];
        }
    }

    NSMutableArray<NSURL *> *baseURLs = [[NSMutableArray alloc] init];

    for (NSURL *volumeURL in volumeURLs)
    {
        HBURLPair *currentPair = nil;
        for (HBURLPair *pair in pairs)
        {
            if ([pair.volumeURL isEqualTo:volumeURL])
            {
                if (currentPair == nil)
                {
                    currentPair = pair;
                }
                else if (pair.length < currentPair.length)
                {
                    currentPair = pair;
                }
            }
        }
        if (currentPair)
        {
            [baseURLs addObject:currentPair.URL];
        }
    }

    return baseURLs;
}

+ (NSURL *)eyetvMediaURL:(NSURL *)url
{
    // We're looking at an EyeTV package - try to open its enclosed .mpg or .ts media file
    NSString *mediaName;
    NSUInteger count = [[url.path stringByAppendingString:@"/"] completePathIntoString:&mediaName
                                                                         caseSensitive:YES
                                                                      matchesIntoArray:nil
                                                                           filterTypes:@[@"mpg", @"ts"]];
    if (count > 0)
    {
        // Found an mpeg inside the eyetv package, make it our scan path
        return [NSURL fileURLWithPath:mediaName];
    }
    else
    {
        return nil;
    }
}

+ (NSArray<NSURL *> *)expandURLs:(NSArray<NSURL *> *)fileURLs recursive:(BOOL)recursive
{
    NSFileManager *manager = NSFileManager.defaultManager;
    NSMutableArray<NSURL *> *mutableFileURLs = [NSMutableArray array];

    NSDirectoryEnumerationOptions options = NSDirectoryEnumerationSkipsHiddenFiles |
                                            NSDirectoryEnumerationSkipsPackageDescendants |
                                            NSDirectoryEnumerationSkipsSubdirectoryDescendants;

    // Check first if it's a DVD-Video or Bluray
    if (fileURLs.count == 1)
    {
        NSURL *directoryURL = fileURLs.firstObject;

        NSNumber *isDirectory = nil;
        [directoryURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

        if (isDirectory.boolValue == YES)
        {
            if ([directoryURL.pathExtension isEqualToString:@"eyetv"])
            {
                NSURL *eyetvMediaURL = [HBUtilities eyetvMediaURL:directoryURL];
                if (eyetvMediaURL)
                {
                    return @[eyetvMediaURL];
                }
            }

            if ([directoryURL.pathExtension isEqualToString:@"dvdmedia"] ||
                [directoryURL.lastPathComponent isEqualToString:@"VIDEO_TS"])
            {
                return fileURLs;
            }

            NSArray<NSURL *> *content = [manager contentsOfDirectoryAtURL:directoryURL
                                                includingPropertiesForKeys:nil
                                                                   options:options
                                                                     error:NULL];

            for (NSURL *url in content)
            {
                if ([url.lastPathComponent isEqualToString:@"VIDEO_TS"] ||
                    [url.lastPathComponent isEqualToString:@"BDMV"])
                {
                    return fileURLs;
                }
            }
        }
    }

    if (recursive)
    {
        options &= ~NSDirectoryEnumerationSkipsSubdirectoryDescendants;
    }

    // If not, recursively enumerate all the files and directories
    for (NSURL *url in fileURLs)
    {
        NSNumber *isDirectory = nil;
        [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

        if (isDirectory.boolValue == NO)
        {
            [mutableFileURLs addObject:url];
        }
        else if ([url.pathExtension isEqualToString:@"eyetv"])
        {
            NSURL *eyetvMediaURL = [HBUtilities eyetvMediaURL:url];
            if (eyetvMediaURL)
            {
                [mutableFileURLs addObject:eyetvMediaURL];
            }
        }
        else if ([url.pathExtension isEqualToString:@"dvdmedia"] ||
                 [url.lastPathComponent isEqualToString:@"VIDEO_TS"])
        {
            // Skip
        }
        else
        {
            NSDirectoryEnumerator<NSURL *> *enumerator = [manager enumeratorAtURL:url
                                                       includingPropertiesForKeys:@[NSURLIsDirectoryKey]
                                                                          options:options
                                                                     errorHandler:NULL];

            for (NSURL *enumeratorURL in enumerator)
            {
                [enumeratorURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

                if (isDirectory.boolValue == YES)
                {
                    if ([enumeratorURL.pathExtension isEqualToString:@"eyetv"])
                    {
                        NSURL *eyetvMediaURL = [HBUtilities eyetvMediaURL:enumeratorURL];
                        if (eyetvMediaURL)
                        {
                            [mutableFileURLs addObject:enumeratorURL];
                        }
                        [enumerator skipDescendants];
                    }
                    else if ([enumeratorURL.pathExtension isEqualToString:@"dvdmedia"] ||
                             [enumeratorURL.lastPathComponent isEqualToString:@"VIDEO_TS"])
                    {
                        [enumerator skipDescendants];
                    }
                }
                else
                {
                    [mutableFileURLs addObject:enumeratorURL];
                }
            }
        }
    }

    [mutableFileURLs sortUsingComparator:^NSComparisonResult(id  _Nonnull obj1, id  _Nonnull obj2) {
        return [[obj1 path] localizedStandardCompare:[obj2 path]];
    }];

    return mutableFileURLs;
}

+ (NSArray<NSString *> *)supportedExtensions
{
    return @[@"srt", @"ssa", @"ass"];
}

+ (NSArray<NSURL *> *)extractURLs:(NSArray<NSURL *> *)fileURLs withExtension:(NSArray<NSString *> *)extensions
{
    NSMutableArray<NSURL *> *extractedFileURLs = [NSMutableArray array];

    for (NSURL *fileURL in fileURLs)
    {
        BOOL isMatch = NO;
        for (NSString *extension in extensions)
        {
            if ([fileURL.pathExtension caseInsensitiveCompare:extension] == NSOrderedSame)
            {
                isMatch = YES;
                break;
            }
        }
        if (isMatch)
        {
            [extractedFileURLs addObject:fileURL];
        }
    }

    return extractedFileURLs;
}

+ (NSArray<NSURL *> *)trimURLs:(NSArray<NSURL *> *)fileURLs withExtension:(NSArray<NSString *> *)excludedExtensions
{
    NSMutableArray<NSURL *> *trimmedURLs = [NSMutableArray array];

    for (NSURL *fileURL in fileURLs)
    {
        BOOL excluded = NO;
        NSString *extension = fileURL.pathExtension;

        if (extension)
        {
            for (NSString *excludedExtension in excludedExtensions)
            {
                if ([extension caseInsensitiveCompare:excludedExtension] == NSOrderedSame)
                {
                    excluded = YES;
                    break;
                }
            }
        }

        if (excluded == NO)
        {
            [trimmedURLs addObject:fileURL];
        }
    }
    return trimmedURLs;
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
                case -1744: //errAEEventWouldRequireUserConsent: 10.14 or later
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
