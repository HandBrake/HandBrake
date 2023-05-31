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

- (NSString *)automaticName
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
        NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:self.fileURL.path error:nil];
        creationDate = attrs[NSFileCreationDate];
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
            NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:self.fileURL.path error:nil];
            NSDate *modificationDate = attrs[NSFileModificationDate];
            if (modificationDate)
            {
                NSString *dateString = [[formatter stringFromDate:modificationDate] stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
                [name appendString:dateString];
            }
        }
        else if ([formatKey isEqualToString:@"{Modification-Time}"])
        {
            NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:self.fileURL.path error:nil];
            NSDate *modificationDate = attrs[NSFileModificationDate];
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
    NSString *fileName = self.title.name;

    // Create an output filename by using the format set in the preferences
    if ([NSUserDefaults.standardUserDefaults boolForKey:HBDefaultAutoNaming])
    {
        fileName = self.automaticName;
    }

    // Automatic name can be empty if the format is empty
    if (fileName.length == 0)
    {
        fileName = self.title.name;
    }

    return [fileName stringByAppendingPathExtension:self.automaticExt];
}

@end
