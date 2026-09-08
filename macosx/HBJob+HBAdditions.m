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

static NSUInteger autoIncrementPadding(void)
{
    NSInteger padding = [NSUserDefaults.standardUserDefaults integerForKey:HBAutoNamingAutoIncrementPadding];
    return MAX(HB_AUTO_INCREMENT_PAD_MIN, MIN(HB_AUTO_INCREMENT_PAD_MAX, padding));
}

// Highest value that fits in the configured padding, e.g. 99 for 2 digits
static NSUInteger autoIncrementMax(NSUInteger padding)
{
    NSUInteger max = 1;
    while (padding-- > 0)
    {
        max *= 10;
    }
    return max - 1;
}

static NSUInteger autoIncrementValue(NSUInteger offset)
{
    NSUInteger max = autoIncrementMax(autoIncrementPadding());
    NSInteger next = [NSUserDefaults.standardUserDefaults integerForKey:HBAutoNamingAutoIncrementNext];
    if (next < 1 || next > max)
    {
        next = 1;
    }
    return (next - 1 + offset) % max + 1;
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
           [[ud arrayForKey:HBAutoNamingFormat] containsObject:@"{Auto-Increment}"];
}

+ (void)advanceAutoIncrementBy:(NSUInteger)count
{
    if (count == 0 || ![self autoIncrementEnabled])
    {
        return;
    }

    NSUInteger max  = autoIncrementMax(autoIncrementPadding());
    NSUInteger used = autoIncrementValue(count - 1);
    [NSUserDefaults.standardUserDefaults setInteger:used % max + 1
                                             forKey:HBAutoNamingAutoIncrementNext];
}

- (NSString *)automaticName
{
    return [self automaticNameWithAutoIncrementOffset:0];
}

- (NSString *)automaticNameWithAutoIncrementOffset:(NSUInteger)offset
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
        else if ([formatKey isEqualToString:@"{Auto-Increment}"])
        {
            [name appendFormat:@"%0*lu", (int)autoIncrementPadding(), (unsigned long)autoIncrementValue(offset)];
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
    return [self defaultNameWithAutoIncrementOffset:0];
}

- (NSString *)defaultNameWithAutoIncrementOffset:(NSUInteger)offset
{
    NSString *fileName = self.title.name;

    // Create an output filename by using the format set in the preferences
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBDefaultAutoNaming])
    {
        fileName = [self automaticNameWithAutoIncrementOffset:offset];
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
