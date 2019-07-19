/*  HBUtilities.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBUtilities.h"
#import <Cocoa/Cocoa.h>

#import "HBTitle.h"
#import "HBJob.h"

#include "common.h"
#include "lang.h"

static NSDateFormatter *_timeFormatter = nil;
static NSDateFormatter *_dateFormatter = nil;
static NSDateFormatter *_releaseDateFormatter = nil;

@implementation HBUtilities

+ (void)initialize
{
    if (self == [HBUtilities class]) {
        _dateFormatter = [[NSDateFormatter alloc] init];
        [_dateFormatter setDateStyle:NSDateFormatterShortStyle];
        [_dateFormatter setTimeStyle:NSDateFormatterNoStyle];

        _timeFormatter = [[NSDateFormatter alloc] init];
        [_timeFormatter setDateStyle:NSDateFormatterNoStyle];
        [_timeFormatter setTimeStyle:NSDateFormatterShortStyle];

        _releaseDateFormatter = [[NSDateFormatter alloc] init];
        [_releaseDateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ss'Z'"];
    }
}

+ (NSString *)handBrakeVersion
{
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
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
    return [NSURL URLWithString:@"https://handbrake.fr/docs/en/1.2.0/"];
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

+ (nullable NSURL *)URLFromBookmark:(NSData *)bookmark
{
    NSParameterAssert(bookmark);

    NSError *error;
    BOOL isStale;

    NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];

    if (error)
    {
        NSString *error_message = [NSString stringWithFormat:@"Failed to resolved bookmark: %@", error];
        [HBUtilities writeToActivityLog:"%s", error_message.UTF8String];
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
    if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:URL.path])
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

+ (nullable NSDate *)releaseDate:(HBTitle *)title
{
    if ([title.metadata.releaseDate length] == 0)
    {
        return nil;
    }
    else
    {
        return [_releaseDateFormatter dateFromString:title.metadata.releaseDate];
    }
}

+ (NSString *)automaticNameForJob:(HBJob *)job
{
    HBTitle *title = job.title;
    
    NSDate *releaseDate = [self releaseDate:title];
    if (releaseDate == nil)
    {
        NSDictionary* fileAttribs = [[NSFileManager defaultManager] attributesOfItemAtPath:job.fileURL.path error:nil];
        releaseDate = [fileAttribs objectForKey:NSFileCreationDate];
    }

    // Generate a new file name
    NSString *fileName = [HBUtilities automaticNameForSource:title.name
                                                       title:title.index
                                                    chapters:NSMakeRange(job.range.chapterStart + 1, job.range.chapterStop + 1)
                                                     quality:job.video.qualityType ? job.video.quality : 0
                                                     bitrate:!job.video.qualityType ? job.video.avgBitrate : 0
                                                  videoCodec:job.video.encoder
                                                creationDate:releaseDate];
    return fileName;
}

+ (NSString *)automaticExtForJob:(HBJob *)job
{
    NSString *extension = @(hb_container_get_default_extension(job.container));

    if (job.container & HB_MUX_MASK_MP4)
    {
        BOOL anyCodecAC3 = [job.audio anyCodecMatches:HB_ACODEC_AC3] || [job.audio anyCodecMatches:HB_ACODEC_AC3_PASS];
        // Chapter markers are enabled if the checkbox is ticked and we are doing p2p or we have > 1 chapter
        BOOL chapterMarkers = (job.chaptersEnabled) &&
        (job.range.type != HBRangeTypeChapters || job.range.chapterStart < job.range.chapterStop);

        NSString *defaultExtension = [[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultMpegExtension"];

        if ([defaultExtension isEqualToString:@".m4v"] ||
            ((YES == anyCodecAC3 || YES == chapterMarkers) && [defaultExtension isEqualToString:@"Auto"]))
        {
            extension = @"m4v";
        }
    }

    return extension;
}

+ (NSString *)defaultNameForJob:(HBJob *)job
{
    // Generate a new file name
    NSString *fileName = job.title.name;

    // If Auto Naming is on. We create an output filename by using the
    // format set int he preferences.
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"])
    {
        fileName = [self automaticNameForJob:job];
    }

    // use the correct extension based on the container
    NSString *ext = [self automaticExtForJob:job];
    fileName = [fileName stringByAppendingPathExtension:ext];
    return fileName;
}

+ (NSString *)automaticNameForSource:(NSString *)sourceName
                               title:(NSUInteger)title
                            chapters:(NSRange)chaptersRange
                             quality:(double)quality
                             bitrate:(int)bitrate
                          videoCodec:(uint32_t)codec
                         creationDate:(NSDate *)creationDate
{
    NSMutableString *name = [[NSMutableString alloc] init];
    // The format array contains the tokens as NSString
    NSArray<NSString *> *format = [[NSUserDefaults standardUserDefaults] objectForKey:@"HBAutoNamingFormat"];

    for (NSString *formatKey in format)
    {
        if ([formatKey isEqualToString:@"{Source}"])
        {
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBAutoNamingRemoveUnderscore"])
            {
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"_" withString:@" "];
            }
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBAutoNamingRemovePunctuation"])
            {
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"-" withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"." withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"," withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@";" withString:@""];
            }
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBAutoNamingTitleCase"])
            {
                sourceName = [sourceName capitalizedString];
            }
            [name appendString:sourceName];
        }
        else if ([formatKey isEqualToString:@"{Title}"])
        {
            [name appendFormat:@"%lu", (unsigned long)title];
        }
        else if ([formatKey isEqualToString:@"{Date}"])
        {
            NSDate *date = [NSDate date];
            NSString *dateString = [[_dateFormatter stringFromDate:date] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
            [name appendString:dateString];
        }
        else if ([formatKey isEqualToString:@"{Time}"])
        {
            NSDate *date = [NSDate date];
            [name appendString:[_timeFormatter stringFromDate:date]];
        }
        else if ([formatKey isEqualToString:@"{Creation-Date}"])
        {
            NSString *dateString = [[_dateFormatter stringFromDate:creationDate] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
            [name appendString:dateString];

        }
        else if ([formatKey isEqualToString:@"{Creation-Time}"])
        {
            [name appendString:[_timeFormatter stringFromDate:creationDate]];
        }
        else if ([formatKey isEqualToString:@"{Chapters}"])
        {
            if (chaptersRange.location == chaptersRange.length)
            {
                [name appendFormat:@"%lu", (unsigned long)chaptersRange.location];
            }
            else
            {
                [name appendFormat:@"%lu-%lu", (unsigned long)chaptersRange.location, (unsigned long)chaptersRange.length];
            }
        }
        else if ([formatKey isEqualToString:@"{Quality/Bitrate}"])
        {
            if (bitrate)
            {
                [name appendString:@"abr"];
                [name appendString:[NSString stringWithFormat:@"%d", bitrate]];
            }
            else
            {
                // Append the right quality suffix for the selected codec (rf/qp)
                [name appendString:[@(hb_video_quality_get_name(codec)) lowercaseString]];
                [name appendString:[NSString stringWithFormat:@"%0.2f", quality]];
            }
        }
        else
        {
            [name appendString:formatKey];
        }
    }

    return [name copy];
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
