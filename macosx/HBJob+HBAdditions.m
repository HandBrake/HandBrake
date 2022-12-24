/*  HBJob+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+HBAdditions.h"
#import "HBPreferencesKeys.h"
#import "handbrake/handbrake.h"

static NSDateFormatter *_timeFormatter = nil;
static NSDateFormatter *_dateFormatter = nil;
static NSDateFormatter *_releaseDateFormatter = nil;

@implementation HBJob (HBAdditions)

+ (void)initialize
{
    if (self == [HBJob class]) {
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

- (NSString *)automaticName
{
    HBTitle *title = self.title;

    NSDate *releaseDate = title.metadata.releaseDate.length ? [_releaseDateFormatter dateFromString:title.metadata.releaseDate] : nil;
    if (releaseDate == nil)
    {
        NSDictionary *fileAttribs = [[NSFileManager defaultManager] attributesOfItemAtPath:self.fileURL.path error:nil];
        releaseDate = [fileAttribs objectForKey:NSFileCreationDate];
    }

    // Generate a new file name
    NSString *fileName = [self automaticNameForSource:title.name
                                                       title:title.index
                                                    chapters:NSMakeRange(self.range.chapterStart + 1, self.range.chapterStop + 1)
                                                     quality:self.video.qualityType ? self.video.quality : 0
                                                     bitrate:!self.video.qualityType ? self.video.avgBitrate : 0
                                                  videoCodec:self.video.encoder
                                                creationDate:releaseDate];
    return fileName;
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

- (NSString *)automaticNameForSource:(NSString *)sourceName
                               title:(NSUInteger)title
                            chapters:(NSRange)chaptersRange
                             quality:(double)quality
                             bitrate:(int)bitrate
                          videoCodec:(uint32_t)codec
                        creationDate:(NSDate *)creationDate
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    NSMutableString *name = [[NSMutableString alloc] init];

    // The format array contains the tokens as NSString
    NSArray<NSString *> *format = [ud objectForKey:HBAutoNamingFormat];

    for (NSString *formatKey in format)
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

@end
