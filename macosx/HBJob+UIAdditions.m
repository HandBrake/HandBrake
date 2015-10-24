/*  HBJob+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+UIAdditions.h"

#import "HBAttributedStringAdditions.h"
#import "HBTitle.h"
#import "HBJob.h"

#import "HBAudioTrack.h"
#import "HBAudioDefaults.h"

#import "HBSubtitlesTrack.h"

#import "HBPicture+UIAdditions.h"
#import "HBFilters+UIAdditions.h"

#include "hb.h"

// Text Styles
static NSMutableParagraphStyle *ps;
static NSDictionary            *detailAttr;
static NSDictionary            *detailBoldAttr;
static NSDictionary            *titleAttr;
static NSDictionary            *shortHeightAttr;

@implementation HBJob (UIAdditions)

- (BOOL)mp4OptionsEnabled
{
    return ((self.container & HB_MUX_MASK_MP4) != 0);
}

- (BOOL)mp4iPodCompatibleEnabled
{
    return ((self.container & HB_MUX_MASK_MP4) != 0) && (self.video.encoder & HB_VCODEC_H264_MASK);
}

- (NSArray *)angles
{
    NSMutableArray *angles = [NSMutableArray array];
    for (int i = 1; i <= self.title.angles; i++)
    {
        [angles addObject:[NSString stringWithFormat: @"%d", i]];
    }
    return angles;
}

- (NSArray *)containers
{
    NSMutableArray *containers = [NSMutableArray array];

    for (const hb_container_t *container = hb_container_get_next(NULL);
         container != NULL;
         container  = hb_container_get_next(container))
    {
        NSString *title = nil;
        if (container->format & HB_MUX_MASK_MP4)
        {
            title = NSLocalizedString(@"MP4 File", @"");
        }
        else if (container->format & HB_MUX_MASK_MKV)
        {
            title = NSLocalizedString(@"MKV File", @"");
        }
        else
        {
            title = @(container->name);
        }
        [containers addObject:title];
    }

    return [containers copy];
}

- (void)initStyles
{
    if (!ps)
    {
        // Attributes
        ps = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
        [ps setHeadIndent: 40.0];
        [ps setParagraphSpacing: 1.0];
        [ps setTabStops:@[]];    // clear all tabs
        [ps addTabStop: [[NSTextTab alloc] initWithType: NSLeftTabStopType location: 20.0]];

        detailAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont smallSystemFontSize]],
                        NSParagraphStyleAttributeName: ps};

        detailBoldAttr = @{NSFontAttributeName: [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]],
                            NSParagraphStyleAttributeName: ps};

        titleAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont systemFontSize]],
                       NSParagraphStyleAttributeName: ps};

        shortHeightAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:2.0]};
    }
}

- (NSAttributedString *)attributedDescription
{
    // Below should be put into a separate method but I am way too f'ing lazy right now
    NSMutableAttributedString *finalString = [[NSMutableAttributedString alloc] initWithString: @""];

    [self initStyles];

    @autoreleasepool
    {
        // First line, we should strip the destination path and just show the file name and add the title num and chapters (if any)
        NSString *summaryInfo;

        NSString *titleString = [NSString stringWithFormat:@"Title %d", self.titleIdx];

        NSString *startStopString = @"";
        if (self.range.type == HBRangeTypeChapters)
        {
            // Start Stop is chapters
            startStopString = (self.range.chapterStart == self.range.chapterStop) ?
            [NSString stringWithFormat:@"Chapter %d", self.range.chapterStart + 1] :
            [NSString stringWithFormat:@"Chapters %d through %d", self.range.chapterStart + 1, self.range.chapterStop + 1];
        }
        else if (self.range.type == HBRangeTypeSeconds)
        {
            // Start Stop is seconds
            startStopString = [NSString stringWithFormat:@"Seconds %d through %d", self.range.secondsStart, self.range.secondsStop];
        }
        else if (self.range.type == HBRangeTypeFrames)
        {
            // Start Stop is Frames
            startStopString = [NSString stringWithFormat:@"Frames %d through %d", self.range.frameStart, self.range.frameStop];
        }
        NSString *passesString = @"";
        // check to see if our first subtitle track is Foreign Language Search, in which case there is an in depth scan
        if (self.subtitles.tracks.firstObject.sourceTrackIdx == 1)
        {
            passesString = [passesString stringByAppendingString:@"1 Foreign Language Search Pass - "];
        }
        if (self.video.qualityType == 1 || self.video.twoPass == NO)
        {
            passesString = [passesString stringByAppendingString:@"1 Video Pass"];
        }
        else
        {
            if (self.video.turboTwoPass == YES)
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes First Turbo"];
            }
            else
            {
                passesString = [passesString stringByAppendingString:@"2 Video Passes"];
            }
        }

        [finalString appendString:[NSString stringWithFormat:@"%@", self.description] withAttributes:titleAttr];

        // lets add the output file name to the title string here
        NSString *outputFilenameString = self.destURL.lastPathComponent;

        summaryInfo = [NSString stringWithFormat: @" (%@, %@, %@) -> %@", titleString, startStopString, passesString, outputFilenameString];

        [finalString appendString:[NSString stringWithFormat:@"%@\n", summaryInfo] withAttributes:detailAttr];

        // Insert a short-in-height line to put some white space after the title
        [finalString appendString:@"\n" withAttributes:shortHeightAttr];
        // End of Title Stuff

        // Second Line  (Preset Name)
        [finalString appendString: @"Preset: " withAttributes:detailBoldAttr];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", self.presetName] withAttributes:detailAttr];

        // Third Line  (Format Summary)
        NSString *audioCodecSummary = @"";	//	This seems to be set by the last track we have available...
        // Lets also get our audio track detail since we are going through the logic for use later

        NSMutableArray *audioDetails = [NSMutableArray array];
        BOOL autoPassthruPresent = NO;

        for (HBAudioTrack *audioTrack in self.audio.tracks)
        {
            if (audioTrack.enabled)
            {
                audioCodecSummary = [NSString stringWithFormat: @"%@", audioTrack.codec[keyAudioCodecName]];
                NSNumber *drc = @(audioTrack.drc);
                NSNumber *gain = @(audioTrack.gain);
                NSString *detailString = [NSString stringWithFormat: @"%@ Encoder: %@ Mixdown: %@ SampleRate: %@(khz) Bitrate: %@(kbps), DRC: %@, Gain: %@",
                                          audioTrack.track[keyAudioTrackName],
                                          audioTrack.codec[keyAudioCodecName],
                                          audioTrack.mixdown[keyAudioMixdownName],
                                          audioTrack.sampleRate[keyAudioSampleRateName],
                                          audioTrack.bitRate[keyAudioBitrateName],
                                          (0.0 < [drc floatValue]) ? (NSObject *)drc : (NSObject *)@"Off",
                                          (0.0 != [gain floatValue]) ? (NSObject *)gain : (NSObject *)@"Off"
                                          ];
                [audioDetails addObject: detailString];
                // check if we have an Auto Passthru output track
                if ([audioTrack.codec[keyAudioCodecName] isEqualToString: @"Auto Passthru"])
                {
                    autoPassthruPresent = YES;
                }
            }
        }

        NSString *jobFormatInfo;
        if (self.chaptersEnabled)
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio, Chapter Markers\n",
                             @(hb_container_get_name(self.container)), @(hb_video_encoder_get_name(self.video.encoder)), audioCodecSummary];
        else
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video  %@ Audio\n",
                             @(hb_container_get_name(self.container)), @(hb_video_encoder_get_name(self.video.encoder)), audioCodecSummary];

        [finalString appendString: @"Format: " withAttributes:detailBoldAttr];
        [finalString appendString: jobFormatInfo withAttributes:detailAttr];

        // Optional String for muxer options
        NSMutableString *containerOptions = [NSMutableString stringWithString:@""];
        if ((self.container & HB_MUX_MASK_MP4) && self.mp4HttpOptimize)
        {
            [containerOptions appendString:@" - Web optimized"];
        }
        if ((self.container & HB_MUX_MASK_MP4)  && self.mp4iPodCompatible)
        {
            [containerOptions appendString:@" - iPod 5G support"];
        }
        if ([containerOptions hasPrefix:@" - "])
        {
            [containerOptions deleteCharactersInRange:NSMakeRange(0, 3)];
        }
        if (containerOptions.length)
        {
            [finalString appendString:@"Container Options: " withAttributes:detailBoldAttr];
            [finalString appendString:containerOptions       withAttributes:detailAttr];
            [finalString appendString:@"\n"                  withAttributes:detailAttr];
        }

        // Fourth Line (Destination Path)
        [finalString appendString: @"Destination: " withAttributes:detailBoldAttr];
        [finalString appendString: self.destURL.path withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];


        // Fifth Line Picture Details
        NSString *pictureInfo = [NSString stringWithFormat:@"%@", self.picture.summary];
        if (self.picture.keepDisplayAspect)
        {
            pictureInfo = [pictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
        }
        [finalString appendString:@"Picture: " withAttributes:detailBoldAttr];
        [finalString appendString:pictureInfo  withAttributes:detailAttr];
        [finalString appendString:@"\n"        withAttributes:detailAttr];

        /* Optional String for Picture Filters */
        if (self.filters.summary.length)
        {
            NSString *pictureFilters = [NSString stringWithFormat:@"%@", self.filters.summary];
            [finalString appendString:@"Filters: "   withAttributes:detailBoldAttr];
            [finalString appendString:pictureFilters withAttributes:detailAttr];
            [finalString appendString:@"\n"          withAttributes:detailAttr];
        }

        // Sixth Line Video Details
        NSString * videoInfo = [NSString stringWithFormat:@"Encoder: %@", @(hb_video_encoder_get_name(self.video.encoder))];

        // for framerate look to see if we are using vfr detelecine
        if (self.video.frameRate == 0)
        {
            if (self.video.frameRateMode == 0)
            {
                // we are using same as source with vfr detelecine
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Variable Frame Rate)", videoInfo];
            }
            else
            {
                // we are using a variable framerate without dropping frames
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: Same as source (Constant Frame Rate)", videoInfo];
            }
        }
        else
        {
            // we have a specified, constant framerate
            if (self.video.frameRateMode == 0)
            {
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Peak Frame Rate)", videoInfo, @(hb_video_framerate_get_name(self.video.frameRate))];
            }
            else
            {
                videoInfo = [NSString stringWithFormat:@"%@ Framerate: %@ (Constant Frame Rate)", videoInfo, @(hb_video_framerate_get_name(self.video.frameRate))];
            }
        }


        if (self.video.qualityType == 0) // ABR
        {
            videoInfo = [NSString stringWithFormat:@"%@ Bitrate: %d(kbps)", videoInfo, self.video.avgBitrate];
        }
        else // CRF
        {
            videoInfo = [NSString stringWithFormat:@"%@ Constant Quality: %.2f", videoInfo ,self.video.quality];
        }

        [finalString appendString: @"Video: " withAttributes:detailBoldAttr];
        [finalString appendString: videoInfo withAttributes:detailAttr];
        [finalString appendString:@"\n" withAttributes:detailAttr];


        if (hb_video_encoder_get_presets(self.video.encoder) != NULL)
        {
            // we are using x264/x265
            NSString *encoderPresetInfo = @"";
            if (self.video.advancedOptions)
            {
                // we are using the old advanced panel
                if (self.video.videoOptionExtra.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString:self.video.videoOptionExtra];
                }
                else
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString:@"default settings"];
                }
            }
            else
            {
                // we are using the x264 system
                encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@"Preset: %@", self.video.preset]];

                if (self.video.tune.length || self.video.fastDecode)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString:@" - Tune: "];

                    if (self.video.tune.length)
                    {
                        encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@"%@", self.video.tune]];

                        if (self.video.fastDecode)
                        {
                            encoderPresetInfo = [encoderPresetInfo stringByAppendingString:@","];
                        }
                    }
                    if (self.video.fastDecode)
                    {
                        encoderPresetInfo = [encoderPresetInfo stringByAppendingString:@"fastdecode"];
                    }
                }
                if (self.video.videoOptionExtra.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Options: %@", self.video.videoOptionExtra]];
                }
                if (self.video.profile.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Profile: %@", self.video.profile]];
                }
                if (self.video.level.length)
                {
                    encoderPresetInfo = [encoderPresetInfo stringByAppendingString: [NSString stringWithFormat:@" - Level: %@", self.video.level]];
                }
            }
            [finalString appendString: @"Video Options: " withAttributes:detailBoldAttr];
            [finalString appendString: encoderPresetInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }
        else
        {
            // we are using libavcodec
            NSString *lavcInfo = @"";
            if (self.video.videoOptionExtra.length)
            {
                lavcInfo = [lavcInfo stringByAppendingString:self.video.videoOptionExtra];
            }
            else
            {
                lavcInfo = [lavcInfo stringByAppendingString: @"default settings"];
            }
            [finalString appendString: @"Encoder Options: " withAttributes:detailBoldAttr];
            [finalString appendString: lavcInfo withAttributes:detailAttr];
            [finalString appendString:@"\n" withAttributes:detailAttr];
        }


        // Seventh Line Audio Details
        for (NSString *anAudioDetail in audioDetails)
        {
            if (anAudioDetail.length)
            {
                [finalString appendString: [NSString stringWithFormat: @"Audio: "] withAttributes: detailBoldAttr];
                [finalString appendString: anAudioDetail withAttributes: detailAttr];
                [finalString appendString: @"\n" withAttributes: detailAttr];
            }
        }

        // Eigth Line Auto Passthru Details
        // only print Auto Passthru settings if we have an Auro Passthru output track
        if (autoPassthruPresent == YES)
        {
            NSString *autoPassthruFallback = @"", *autoPassthruCodecs = @"";
            HBAudioDefaults *audioDefaults = self.audio.defaults;
            autoPassthruFallback = [autoPassthruFallback stringByAppendingString:@(hb_audio_encoder_get_name(audioDefaults.encoderFallback))];
            if (audioDefaults.allowAACPassthru)
            {
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"AAC"];
            }
            if (audioDefaults.allowAC3Passthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"AC3"];
            }
            if (audioDefaults.allowDTSHDPassthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"DTS-HD"];
            }
            if (audioDefaults.allowDTSPassthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"DTS"];
            }
            if (audioDefaults.allowMP3Passthru)
            {
                if (autoPassthruCodecs.length)
                {
                    autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@", "];
                }
                autoPassthruCodecs = [autoPassthruCodecs stringByAppendingString:@"MP3"];
            }
            [finalString appendString: @"Auto Passthru Codecs: " withAttributes: detailBoldAttr];
            if (autoPassthruCodecs.length)
            {
                [finalString appendString: autoPassthruCodecs withAttributes: detailAttr];
            }
            else
            {
                [finalString appendString: @"None" withAttributes: detailAttr];
            }
            [finalString appendString: @"\n" withAttributes: detailAttr];
            [finalString appendString: @"Auto Passthru Fallback: " withAttributes: detailBoldAttr];
            [finalString appendString: autoPassthruFallback withAttributes: detailAttr];
            [finalString appendString: @"\n" withAttributes: detailAttr];
        }
        
        // Ninth Line Subtitle Details
        int i = 0;
        for (HBSubtitlesTrack *track in self.subtitles.tracks)
        {
            // Ignore the none track.
            if (i == self.subtitles.tracks.count - 1)
            {
                continue;
            }
            
            /* remember that index 0 of Subtitles can contain "Foreign Audio Search*/
            [finalString appendString: @"Subtitle: " withAttributes:detailBoldAttr];
            [finalString appendString: self.subtitles.sourceTracks[track.sourceTrackIdx][@"keySubTrackName"] withAttributes:detailAttr];
            if (track.forcedOnly)
            {
                [finalString appendString: @" - Forced Only" withAttributes:detailAttr];
            }
            if (track.burnedIn)
            {
                [finalString appendString: @" - Burned In" withAttributes:detailAttr];
            }
            if (track.def)
            {
                [finalString appendString: @" - Default" withAttributes:detailAttr];
            }
            [finalString appendString:@"\n" withAttributes:detailAttr];
            i++;
        }
    }

    [finalString deleteCharactersInRange:NSMakeRange(finalString.length - 1, 1)];

    return finalString;
}

@end

@implementation HBContainerTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    int container = [value intValue];
    if (container & HB_MUX_MASK_MP4)
    {
        return NSLocalizedString(@"MP4 File", @"");
    }
    else if (container & HB_MUX_MASK_MKV)
    {
        return NSLocalizedString(@"MKV File", @"");
    }
    else
    {
        const char *name = hb_container_get_name(container);
        if (name)
        {
            return @(name);
        }
        else
        {
            return nil;
        }
    }
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value isEqualToString:NSLocalizedString(@"MP4 File", @"")])
    {
        return @(HB_MUX_AV_MP4);
    }
    else if ([value isEqualToString:NSLocalizedString(@"MKV File", @"")])
    {
        return @(HB_MUX_AV_MKV);
    }

    return @(hb_container_get_from_name([value UTF8String]));
}

@end


@implementation HBURLTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    if (value)
        return [value path];
    else
        return nil;
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if (value)
    {
        return [NSURL fileURLWithPath:value];
    }
    return nil;
}

@end

