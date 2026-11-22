/*  HBJob+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+HBAdditions.h"
#import "HBPreferencesKeys.h"
#import "handbrake/handbrake.h"

static NSDateFormatter *_timeFormatter = nil;
static NSDateFormatter *_dateFormatter = nil;
static NSDateFormatter *_dateISOFormatter = nil;
static NSDateFormatter *_releaseDateFormatter = nil;

static NSString * const HBAutoIncrementToken = @"{Auto-Increment}";

// Marks where {Auto-Increment} goes while a name is rendered. A control
// character cannot come from the format text field and is not expected in
// source names, so it is safe to search for afterwards.
static NSString * const HBAutoIncrementPlaceholder = @"\x1F";

static NSUInteger autoIncrementPadding(void)
{
    NSInteger padding = [NSUserDefaults.standardUserDefaults integerForKey:HBAutoNamingAutoIncrementPadding];
    return MAX(HB_AUTO_INCREMENT_PAD_MIN, MIN(HB_AUTO_INCREMENT_PAD_MAX, padding));
}

/// Builds a regular expression that matches a file name stem rendered from
/// template, with any run of digits in place of the {Auto-Increment}
/// placeholder. Capture group 1 holds the number.
static NSRegularExpression *autoIncrementRegex(NSString *template)
{
    NSArray<NSString *> *parts = [template.precomposedStringWithCanonicalMapping componentsSeparatedByString:HBAutoIncrementPlaceholder];
    NSMutableString *pattern = [NSMutableString stringWithString:@"^"];

    [parts enumerateObjectsUsingBlock:^(NSString *part, NSUInteger idx, BOOL *stop) {
        if (idx == 1)
        {
            [pattern appendString:@"([0-9]{1,9})"];
        }
        else if (idx > 1)
        {
            // Repeated tokens always render the same number
            [pattern appendString:@"\\1"];
        }
        [pattern appendString:[NSRegularExpression escapedPatternForString:part]];
    }];
    [pattern appendString:@"$"];

    // macOS file systems are case-insensitive by default
    return [NSRegularExpression regularExpressionWithPattern:pattern
                                                     options:NSRegularExpressionCaseInsensitive
                                                       error:NULL];
}

/// The {Auto-Increment} number in fileName, or 0 if it does not match.
/// When extensions is not nil, the file's extension must be one of them.
static NSUInteger autoIncrementNumberInFileName(NSRegularExpression *regex, NSString *fileName, NSSet<NSString *> *extensions)
{
    if (extensions && ![extensions containsObject:fileName.pathExtension.lowercaseString])
    {
        return 0;
    }

    NSString *stem = fileName.stringByDeletingPathExtension.precomposedStringWithCanonicalMapping;
    NSTextCheckingResult *match = [regex firstMatchInString:stem options:0 range:NSMakeRange(0, stem.length)];
    return match ? (NSUInteger)[stem substringWithRange:[match rangeAtIndex:1]].integerValue : 0;
}

@implementation HBJob (HBAdditions)

+ (void)initialize
{
    if (self == [HBJob class])
    {
        _dateFormatter = [[NSDateFormatter alloc] init];
        [_dateFormatter setDateStyle:NSDateFormatterShortStyle];
        [_dateFormatter setTimeStyle:NSDateFormatterNoStyle];

        _dateISOFormatter = [[NSDateFormatter alloc] init];
        [_dateISOFormatter setDateFormat:@"yyyy-MM-dd"];

        _timeFormatter = [[NSDateFormatter alloc] init];
        [_timeFormatter setDateStyle:NSDateFormatterNoStyle];
        [_timeFormatter setTimeStyle:NSDateFormatterShortStyle];

        _releaseDateFormatter = [[NSDateFormatter alloc] init];
        [_releaseDateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ss'Z'"];
    }
}

+ (BOOL)autoIncrementEnabled
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    return [ud boolForKey:HBDefaultAutoNaming] &&
           [[ud arrayForKey:HBAutoNamingFormat] containsObject:HBAutoIncrementToken];
}

/// Picks the {Auto-Increment} value for a name rendered from template:
/// one past the highest number used by a matching file in the destination
/// folder or by a reserved destination in the same folder. A free number
/// already present in currentName is kept if it is not past that value.
- (NSUInteger)autoIncrementValueForTemplate:(NSString *)template
                               reservingURLs:(nullable NSArray<NSURL *> *)reservedURLs
                                 currentName:(nullable NSString *)currentName
{
    NSRegularExpression *regex = autoIncrementRegex(template);
    NSURL *folderURL = self.destinationFolderURL;
    if (regex == nil || folderURL == nil)
    {
        return 1;
    }

    NSMutableArray<NSString *> *fileNames = [NSMutableArray array];

    NSArray<NSString *> *contents = [NSFileManager.defaultManager contentsOfDirectoryAtPath:folderURL.path error:NULL];
    if (contents)
    {
        [fileNames addObjectsFromArray:contents];
    }

    NSString *folderPath = folderURL.URLByStandardizingPath.path;
    for (NSURL *url in reservedURLs)
    {
        NSString *parentPath = url.URLByDeletingLastPathComponent.URLByStandardizingPath.path;
        if ([parentPath caseInsensitiveCompare:folderPath] == NSOrderedSame)
        {
            [fileNames addObject:url.lastPathComponent];
        }
    }

    NSSet<NSString *> *extensions = [self autoIncrementExtensions];
    NSMutableIndexSet *used = [NSMutableIndexSet indexSet];
    for (NSString *fileName in fileNames)
    {
        NSUInteger number = autoIncrementNumberInFileName(regex, fileName, extensions);
        if (number > 0)
        {
            [used addIndex:number];
        }
    }

    NSUInteger next = used.count ? used.lastIndex + 1 : 1;

    // The current name's extension may not be updated yet, so it is not checked
    NSUInteger current = currentName ? autoIncrementNumberInFileName(regex, currentName, nil) : 0;
    if (current > 0 && current <= next && ![used containsIndex:current])
    {
        return current;
    }

    return next;
}

/// Extensions of the files that continue this job's {Auto-Increment} series.
/// With the MP4 extension preference on Auto, each job picks .mp4 or .m4v
/// depending on its audio and chapters, so one series can mix both.
- (NSSet<NSString *> *)autoIncrementExtensions
{
    if ((self.container & HB_MUX_MASK_MP4) &&
        [[NSUserDefaults.standardUserDefaults stringForKey:HBDefaultMpegExtension] isEqualToString:@"Auto"])
    {
        return [NSSet setWithObjects:@"mp4", @"m4v", nil];
    }
    return [NSSet setWithObject:self.automaticExt.lowercaseString];
}

- (NSString *)automaticName
{
    return [self automaticNameReservingURLs:nil currentName:nil];
}

- (NSString *)automaticNameReservingURLs:(nullable NSArray<NSURL *> *)reservedURLs
                             currentName:(nullable NSString *)currentName
{
    NSString *template = [self automaticNameTemplate];

    if ([template containsString:HBAutoIncrementPlaceholder])
    {
        NSUInteger value = [self autoIncrementValueForTemplate:template
                                                 reservingURLs:reservedURLs
                                                   currentName:currentName];
        NSString *number = [NSString stringWithFormat:@"%0*lu", (int)autoIncrementPadding(), (unsigned long)value];
        template = [template stringByReplacingOccurrencesOfString:HBAutoIncrementPlaceholder withString:number];
    }

    return template;
}

/// Renders the naming format, leaving HBAutoIncrementPlaceholder
/// wherever the {Auto-Increment} token appears
- (NSString *)automaticNameTemplate
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    NSMutableString *name = [[NSMutableString alloc] init];

    NSDateFormatter *formatter = [ud boolForKey:HBAutoNamingISODateFormat] ? _dateISOFormatter : _dateFormatter;
    NSString *sourceName = self.title.name;
    NSDate *creationDate = self.title.metadata.releaseDate.length ?
                            [_releaseDateFormatter dateFromString:self.title.metadata.releaseDate] :
                            nil;

    if (creationDate == nil)
    {
        [self.fileURL getResourceValue:&creationDate forKey:NSURLCreationDateKey error:NULL];
    }

    for (NSString *formatKey in [ud objectForKey:HBAutoNamingFormat])
    {
        if ([formatKey isEqualToString:@"{Source}"])
        {
            if ([ud boolForKey:HBAutoNamingRemoveUnderscore])
            {
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"_" withString:@" "];
            }
            if ([ud boolForKey:HBAutoNamingRemovePunctuation])
            {
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"-" withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"." withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@"," withString:@""];
                sourceName = [sourceName stringByReplacingOccurrencesOfString:@";" withString:@""];
            }
            if ([ud boolForKey:HBAutoNamingTitleCase])
            {
                sourceName = [sourceName capitalizedString];
            }
            [name appendString:sourceName];
        }
        else if ([formatKey isEqualToString:@"{Title}"])
        {
            [name appendFormat:@"%lu", (unsigned long)self.title.index];
        }
        else if ([formatKey isEqualToString:@"{Angle}"])
        {
            [name appendFormat:@"%lu", (unsigned long)self.angle];
        }
        else if ([formatKey isEqualToString:@"{Chapters}"])
        {
            NSRange chaptersRange = NSMakeRange(self.range.chapterStart + 1, self.range.chapterStop + 1);
            if (chaptersRange.location == chaptersRange.length)
            {
                [name appendFormat:@"%lu", (unsigned long)chaptersRange.location];
            }
            else
            {
                [name appendFormat:@"%lu-%lu", (unsigned long)chaptersRange.location, (unsigned long)chaptersRange.length];
            }
        }
        else if ([formatKey isEqualToString:@"{Preset}"])
        {
            [name appendString:self.presetName];
        }
        else if ([formatKey isEqualToString:@"{Width}"])
        {
            [name appendFormat:@"%d", self.picture.storageWidth];
        }
        else if ([formatKey isEqualToString:@"{Height}"])
        {
            [name appendFormat:@"%d", self.picture.storageHeight];
        }
        else if ([formatKey isEqualToString:@"{Codec}"])
        {
            int vcodec = self.video.encoder;
            NSString *codecName = @"Unknown";

            if (vcodec & HB_VCODEC_AV1_MASK)
            {
                codecName = @"AV1";
            }
            else if (vcodec & HB_VCODEC_H264_MASK)
            {
                codecName = @"H.264";
            }
            else if (vcodec & HB_VCODEC_H265_MASK)
            {
                codecName = @"H.265";
            }
            else if (vcodec == HB_VCODEC_THEORA)
            {
                codecName = @"Theora";
            }
            else if (vcodec == HB_VCODEC_FFMPEG_MPEG2)
            {
                codecName = @"MPEG-2";
            }
            else if (vcodec == HB_VCODEC_FFMPEG_MPEG4)
            {
                codecName = @"MPEG-4";
            }
            else if (vcodec == HB_VCODEC_FFMPEG_VP8)
            {
                codecName = @"VP8";
            }
            else if (vcodec == HB_VCODEC_FFMPEG_VP9 || vcodec == HB_VCODEC_FFMPEG_VP9_10BIT)
            {
                codecName = @"VP9";
            }
            [name appendString:codecName];
        }
        else if ([formatKey isEqualToString:@"{Encoder}"])
        {
            [name appendString:@(hb_video_encoder_get_short_name(self.video.encoder))];
        }
        else if ([formatKey isEqualToString:@"{Bit-Depth}"])
        {
            [name appendFormat:@"%dbit", hb_video_encoder_get_depth(self.video.encoder)];
        }
        else if ([formatKey isEqualToString:@"{Quality/Bitrate}"])
        {
            if (self.video.qualityType == HBVideoQualityTypeAvgBitrate)
            {
                [name appendString:[NSString stringWithFormat:@"%d", self.video.avgBitrate]];
            }
            else
            {
                [name appendString:[NSString stringWithFormat:@"%0.2f", self.video.quality]];
            }
        }
        else if ([formatKey isEqualToString:@"{Quality-Type}"])
        {
            if (self.video.qualityType == HBVideoQualityTypeAvgBitrate)
            {
                [name appendString:@"kbps"];
            }
            else
            {
                // Append the right quality suffix for the selected codec (rf/qp)
                [name appendString:[@(hb_video_quality_get_name(self.video.encoder)) lowercaseString]];
            }
        }
        else if ([formatKey isEqualToString:@"{Date}"])
        {
            NSDate *date = [NSDate date];
            NSString *dateString = [[formatter stringFromDate:date] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
            [name appendString:dateString];
        }
        else if ([formatKey isEqualToString:@"{Time}"])
        {
            NSDate *date = [NSDate date];
            [name appendString:[_timeFormatter stringFromDate:date]];
        }
        else if ([formatKey isEqualToString:@"{Creation-Date}"])
        {
            if (creationDate)
            {
                NSString *dateString = [[formatter stringFromDate:creationDate] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
                [name appendString:dateString];
            }
        }
        else if ([formatKey isEqualToString:@"{Creation-Time}"])
        {
            if (creationDate)
            {
                [name appendString:[_timeFormatter stringFromDate:creationDate]];
            }
        }
        else if ([formatKey isEqualToString:@"{Modification-Date}"])
        {
            NSDate *modificationDate = nil;
            [self.fileURL getResourceValue:&modificationDate forKey:NSURLContentModificationDateKey error:NULL];
            if (modificationDate)
            {
                NSString *dateString = [[formatter stringFromDate:modificationDate] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
                [name appendString:dateString];
            }
        }
        else if ([formatKey isEqualToString:HBAutoIncrementToken])
        {
            [name appendString:HBAutoIncrementPlaceholder];
        }
        else if ([formatKey isEqualToString:@"{Modification-Time}"])
        {
            NSDate *modificationDate = nil;
            [self.fileURL getResourceValue:&modificationDate forKey:NSURLContentModificationDateKey error:NULL];
            if (modificationDate)
            {
                [name appendString:[_timeFormatter stringFromDate:modificationDate]];
            }
        }
        else
        {
            [name appendString:formatKey];
        }
    }

    return [name copy];
}

- (NSString *)automaticExt
{
    NSString *extension = @(hb_container_get_default_extension(self.container));

    if (self.container & HB_MUX_MASK_MP4)
    {
        BOOL anyCodecAC3 = [self.audio anyCodecMatches:HB_ACODEC_AC3] || [self.audio anyCodecMatches:HB_ACODEC_AC3_PASS];
        // Chapter markers are enabled if the checkbox is ticked and we are doing p2p or we have > 1 chapter
        BOOL chapterMarkers = (self.chaptersEnabled) &&
        (self.range.type != HBRangeTypeChapters || self.range.chapterStart < self.range.chapterStop);

        NSString *defaultExtension = [NSUserDefaults.standardUserDefaults stringForKey:HBDefaultMpegExtension];

        if ([defaultExtension isEqualToString:@".m4v"] ||
            ((YES == anyCodecAC3 || YES == chapterMarkers) && [defaultExtension isEqualToString:@"Auto"]))
        {
            extension = @"m4v";
        }
    }

    return extension;
}

- (NSString *)defaultName
{
    return [self defaultNameReservingURLs:nil];
}

- (NSString *)defaultNameReservingURLs:(nullable NSArray<NSURL *> *)reservedURLs
{
    NSString *fileName = self.title.name;

    // Create an output filename by using the format set in the preferences
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBDefaultAutoNaming])
    {
        fileName = [self automaticNameReservingURLs:reservedURLs currentName:nil];
    }

    // Automatic name can be empty if the format is empty
    if (fileName.length == 0)
    {
        fileName = self.title.name;
    }

    return [fileName stringByAppendingPathExtension:self.automaticExt];
}

- (void)setDestinationFolderURL:(NSURL *)destinationFolderURL sameAsSource:(BOOL)useSourceFolderDestination
{
    // If destination mode is set to same as source, try to set the source folder url
    if (self.title.isStream && useSourceFolderDestination)
    {
        NSURL *titleParentURL = self.title.url.URLByDeletingLastPathComponent;
        if ([titleParentURL.path hasPrefix:destinationFolderURL.path])
        {
            destinationFolderURL = titleParentURL;
        }
    }

    self.destinationFolderURL = destinationFolderURL;
}

@end
