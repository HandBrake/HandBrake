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

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMp4OptionsEnabled
{
    return [NSSet setWithObjects:@"container", nil];
}

- (BOOL)mp4iPodCompatibleEnabled
{
    return ((self.container & HB_MUX_MASK_MP4) != 0) && (self.video.encoder & HB_VCODEC_H264_MASK);
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMp4iPodCompatibleEnabled
{
    return [NSSet setWithObjects:@"container", @"video.encoder", nil];
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

#pragma mark - Attributed description

- (void)initStyles
{
    if (!ps)
    {
        // Attributes
        ps = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
        ps.headIndent = 88.0;
        ps.paragraphSpacing = 1.0;
        ps.tabStops = @[[[NSTextTab alloc] initWithType:
                          NSRightTabStopType location: 88],
                         [[NSTextTab alloc] initWithType:
                          NSLeftTabStopType location: 90]];

        detailAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont smallSystemFontSize]],
                        NSParagraphStyleAttributeName: ps};

        detailBoldAttr = @{NSFontAttributeName: [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]],
                            NSParagraphStyleAttributeName: ps};

        titleAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:[NSFont systemFontSize]],
                       NSParagraphStyleAttributeName: ps};

        shortHeightAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:2.0]};
    }
}

- (NSAttributedString *)titleAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    // Job name
    [attrString appendString:self.description withAttributes:titleAttr];

    // Range type
    NSString *startStopString = @"";
    if (self.range.type == HBRangeTypeChapters)
    {
        startStopString = (self.range.chapterStart == self.range.chapterStop) ?
        [NSString stringWithFormat:@"Chapter %d", self.range.chapterStart + 1] :
        [NSString stringWithFormat:@"Chapters %d through %d", self.range.chapterStart + 1, self.range.chapterStop + 1];
    }
    else if (self.range.type == HBRangeTypeSeconds)
    {
        startStopString = [NSString stringWithFormat:@"Seconds %d through %d", self.range.secondsStart, self.range.secondsStop];
    }
    else if (self.range.type == HBRangeTypeFrames)
    {
        startStopString = [NSString stringWithFormat:@"Frames %d through %d", self.range.frameStart, self.range.frameStop];
    }

    NSMutableString *passesString = [NSMutableString string];
    // check to see if our first subtitle track is Foreign Language Search, in which case there is an in depth scan
    if (self.subtitles.tracks.firstObject.sourceTrackIdx == 1)
    {
        [passesString appendString:@"1 Foreign Language Search Pass - "];
    }
    if (self.video.qualityType != 1 && self.video.twoPass == YES)
    {
        if (self.video.turboTwoPass == YES)
        {
            [passesString appendString:@"2 Video Passes First Turbo"];
        }
        else
        {
            [passesString appendString:@"2 Video Passes"];
        }
    }

    if (passesString.length)
    {
        [attrString appendString:[NSString stringWithFormat:@" (Title %d, %@, %@) ▸ %@\n",
                                  self.titleIdx, startStopString, passesString, self.outputFileName]
                  withAttributes:detailAttr];
    }
    else
    {
        [attrString appendString:[NSString stringWithFormat:@" (Title %d, %@) ▸ %@\n",
                                  self.titleIdx, startStopString, self.outputFileName]
                  withAttributes:detailAttr];
    }

    return attrString;
}

- (NSAttributedString *)presetAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\tPreset: " withAttributes:detailBoldAttr];
    [attrString appendString:@"\t" withAttributes:detailAttr];
    [attrString appendString:self.presetName withAttributes:detailAttr];
    [attrString appendString:@"\n" withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)formatAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    NSMutableString *options = [NSMutableString string];

    [options appendString:@(hb_container_get_name(self.container))];

    if (self.chaptersEnabled)
    {
        [options appendString:@", Chapter Markers"];
    }

    if ((self.container & HB_MUX_MASK_MP4) && self.mp4HttpOptimize)
    {
        [options appendString:@", Web optimized"];
    }

    if ((self.container & HB_MUX_MASK_MP4)  && self.mp4iPodCompatible)
    {
        [options appendString:@", iPod 5G support"];
    }

    if ([options hasPrefix:@", "])
    {
        [options deleteCharactersInRange:NSMakeRange(0, 2)];
    }

    [attrString appendString:@"\tFormat: \t" withAttributes:detailBoldAttr];
    [attrString appendString:options withAttributes:detailAttr];
    [attrString appendString:@"\n" withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)destinationAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\tDestination: "             withAttributes:detailBoldAttr];
    [attrString appendString:@"\t"                          withAttributes:detailAttr];
    [attrString appendString:self.completeOutputURL.path    withAttributes:detailAttr];
    [attrString appendString:@"\n"                          withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)pictureAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    NSString *pictureInfo = self.picture.summary;
    if (self.picture.keepDisplayAspect)
    {
        pictureInfo = [pictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
    }
    [attrString appendString:@"\tPicture: " withAttributes:detailBoldAttr];
    [attrString appendString:@"\t"          withAttributes:detailAttr];
    [attrString appendString:pictureInfo    withAttributes:detailAttr];
    [attrString appendString:@"\n"          withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)filtersAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    NSMutableString *summary = [NSMutableString string];
    HBFilters *filters = self.filters;

    // Detelecine
    if (![filters.detelecine isEqualToString:@"off"])
    {
        if ([filters.detelecine isEqualToString:@"custom"])
        {
            [summary appendFormat:@", Detelecine (%@)", filters.detelecineCustomString];
        }
        else
        {
            [summary appendFormat:@", Detelecine (%@)", [[[HBFilters detelecinePresetsDict] allKeysForObject:filters.detelecine] firstObject]];
        }
    }
    else if (![filters.deinterlace isEqualToString:@"off"])
    {
        // Deinterlace or Decomb
        NSString *type =  [[[HBFilters deinterlaceTypesDict] allKeysForObject:filters.deinterlace] firstObject];

        if ([filters.deinterlacePreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@ (%@)", type, filters.deinterlaceCustomString];
        }
        else
        {
            if ([filters.deinterlace isEqualToString:@"decomb"])
            {
                [summary appendFormat:@", %@ (%@)", type, [[[HBFilters decombPresetsDict] allKeysForObject:filters.deinterlacePreset] firstObject]];
            }
            else if ([filters.deinterlace isEqualToString:@"deinterlace"])
            {
                [summary appendFormat:@", %@ (%@)", type, [[[HBFilters deinterlacePresetsDict] allKeysForObject:filters.deinterlacePreset] firstObject]];
            }
        }
    }

    // Deblock
    if (filters.deblock > 0)
    {
        [summary appendFormat:@", Deblock (%d)", filters.deblock];
    }

    // Denoise
    if (![filters.denoise isEqualToString:@"off"])
    {
        [summary appendFormat:@", Denoise (%@", [[[HBFilters denoiseTypesDict] allKeysForObject:filters.denoise] firstObject]];
        if (![filters.denoisePreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", [[[HBFilters denoisePresetDict] allKeysForObject:filters.denoisePreset] firstObject]];

            if ([filters.denoise isEqualToString:@"nlmeans"])
            {
                [summary appendFormat:@", %@", [[[HBFilters nlmeansTunesDict] allKeysForObject:filters.denoiseTune] firstObject]];
            }
        }
        else
        {
            [summary appendFormat:@", %@", filters.denoiseCustomString];
        }

        [summary appendString:@")"];

    }

    // Sharpen
    if (![filters.sharpen isEqualToString:@"off"])
    {
        [summary appendFormat:@", Sharpen (%@", [[[HBFilters sharpenTypesDict] allKeysForObject:filters.sharpen] firstObject]];
        if (![filters.sharpenPreset isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", [[[HBFilters sharpenPresetDict] allKeysForObject:filters.sharpenPreset] firstObject]];

            if ([filters.sharpen isEqualToString:@"unsharp"])
            {
                [summary appendFormat:@", %@", [[[HBFilters sharpenTunesDict] allKeysForObject:filters.sharpenTune] firstObject]];
            }
            else if ([filters.sharpen isEqualToString:@"lapsharp"])
            {
                [summary appendFormat:@", %@", [[[HBFilters sharpenTunesDict] allKeysForObject:filters.sharpenTune] firstObject]];
            }
        }
        else
        {
            [summary appendFormat:@", %@", filters.sharpenCustomString];
        }

        [summary appendString:@")"];

    }

    // Grayscale
    if (filters.grayscale)
    {
        [summary appendString:@", Grayscale"];
    }

    if ([summary hasPrefix:@", "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 2)];
    }

    // Optional String for Picture Filters
    if (summary.length)
    {
        [attrString appendString:@"\tFilters: " withAttributes:detailBoldAttr];
        [attrString appendString:@"\t"          withAttributes:detailAttr];
        [attrString appendString:summary        withAttributes:detailAttr];
        [attrString appendString:@"\n"          withAttributes:detailAttr];
    }
    
    return attrString;
}

- (NSAttributedString *)videoAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    NSMutableString *videoInfo = [NSMutableString string];

    [videoInfo appendFormat:@"Encoder: %@, ", @(hb_video_encoder_get_name(self.video.encoder))];

    [videoInfo appendString:@"Framerate: "];

    if (self.video.frameRate == 0)
    {
        if (self.video.frameRateMode == 0)
        {
            // we are using same as source with vfr
            [videoInfo appendFormat:@"Same as source (variable)"];
        }
        else
        {
            [videoInfo appendFormat:@"Same as source (constant)"];
        }
    }
    else
    {
        // we have a specified, constant framerate
        if (self.video.frameRateMode == 0)
        {
            [videoInfo appendFormat:@"Peak %@ (may be lower)", @(hb_video_framerate_get_name(self.video.frameRate))];
        }
        else
        {
            [videoInfo appendFormat:@"Peak %@ (constant frame rate)", @(hb_video_framerate_get_name(self.video.frameRate))];
        }
    }

    if (self.video.qualityType == 0) // ABR
    {
        [videoInfo appendFormat:@", Bitrate: %d kbps", self.video.avgBitrate];
    }
    else // CRF
    {
        [videoInfo appendFormat:@", Constant Quality: %.2f %s" ,self.video.quality, hb_video_quality_get_name(self.video.encoder)];
    }

    [attrString appendString:@"\tVideo: " withAttributes:detailBoldAttr];
    [attrString appendString:@"\t" withAttributes:detailAttr];
    [attrString appendString:videoInfo withAttributes:detailAttr];
    [attrString appendString:@"\n" withAttributes:detailAttr];

    if (hb_video_encoder_get_presets(self.video.encoder) != NULL)
    {
        NSMutableString *encoderPresetInfo = [NSMutableString string];

        if (self.video.advancedOptions)
        {
            // we are using the old advanced panel
            if (self.video.videoOptionExtra.length)
            {
                [encoderPresetInfo appendString:self.video.videoOptionExtra];
            }
            else
            {
                [encoderPresetInfo appendString:@"default settings"];
            }
        }
        else
        {
            // we are using the x264 system
            [encoderPresetInfo appendFormat:@"Preset: %@", self.video.preset];

            if (self.video.tune.length || self.video.fastDecode)
            {
                [encoderPresetInfo appendString:@", Tune: "];

                if (self.video.tune.length)
                {
                    [encoderPresetInfo appendString:self.video.tune];
                }
                if (self.video.fastDecode)
                {
                    [encoderPresetInfo appendString:@" - fastdecode"];
                }
            }
            if (self.video.videoOptionExtra.length)
            {
                [encoderPresetInfo appendFormat:@", Options: %@", self.video.videoOptionExtra];
            }
            if (self.video.profile.length)
            {
                [encoderPresetInfo appendFormat:@", Profile: %@", self.video.profile];
            }
            if (self.video.level.length)
            {
                [encoderPresetInfo appendFormat:@", Level: %@", self.video.level];
            }
        }
        [attrString appendString:@"\tVideo Options: "   withAttributes:detailBoldAttr];
        [attrString appendString:@"\t"                  withAttributes:detailAttr];
        [attrString appendString:encoderPresetInfo      withAttributes:detailAttr];
        [attrString appendString:@"\n"                  withAttributes:detailAttr];
    }
    else
    {
        // we are using libavcodec
        NSString *lavcInfo = @"";
        if (self.video.videoOptionExtra.length)
        {
            lavcInfo = self.video.videoOptionExtra;
        }
        else
        {
            lavcInfo = @"default settings";
        }

        [attrString appendString:@"\tVideo Options: " withAttributes:detailBoldAttr];
        [attrString appendString:@"\t"                  withAttributes:detailAttr];
        [attrString appendString:lavcInfo               withAttributes:detailAttr];
        [attrString appendString:@"\n"                  withAttributes:detailAttr];
    }
    
    return attrString;
}

- (NSAttributedString *)audioAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    BOOL secondLine = NO;

    [attrString appendString:@"\tAudio: " withAttributes: detailBoldAttr];

    for (HBAudioTrack *audioTrack in self.audio.tracks)
    {
        if (audioTrack.isEnabled)
        {
            NSMutableString *detailString = [NSMutableString stringWithFormat:@"%@ ▸ Encoder: %@",
                                      self.audio.sourceTracks[audioTrack.sourceTrackIdx][keyAudioTrackName],
                                      @(hb_audio_encoder_get_name(audioTrack.encoder))];

            if ((audioTrack.encoder  & HB_ACODEC_PASS_FLAG) == 0)
            {
                [detailString appendFormat:@", Mixdown: %@, Samplerate: %@, Bitrate: %d kbps",
                                            @(hb_mixdown_get_name(audioTrack.mixdown)),
                                            audioTrack.sampleRate ? [NSString stringWithFormat:@"%@ khz", @(hb_audio_samplerate_get_name(audioTrack.sampleRate))] : @"Auto",
                                            audioTrack.bitRate];

                if (0.0 < audioTrack.drc)
                {
                    [detailString appendFormat:@", DRC: %.2f", audioTrack.drc];
                }

                if (0.0 != audioTrack.gain)
                {
                    [detailString appendFormat:@", Gain: %.2f", audioTrack.gain];
                }
            }

            [attrString appendString:@"\t" withAttributes: detailAttr];
            if (secondLine)
            {
                [attrString appendString:@"\t" withAttributes: detailAttr];
            }
            else
            {
                secondLine = YES;
            }
            [attrString appendString:detailString withAttributes: detailAttr];
            [attrString appendString:@"\n" withAttributes: detailAttr];
        }
    }

    return attrString;
}

- (NSAttributedString *)subtitlesAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    BOOL secondLine = NO;

    [attrString appendString:@"\tSubtitles: " withAttributes:detailBoldAttr];

    for (HBSubtitlesTrack *track in self.subtitles.tracks)
    {
        // Ignore the none track.
        if (track.isEnabled)
        {
            NSMutableString *detailString = [NSMutableString string];

            // remember that index 0 of Subtitles can contain "Foreign Audio Search
            [detailString appendString:self.subtitles.sourceTracks[track.sourceTrackIdx][@"keySubTrackName"]];

            if (track.forcedOnly)
            {
                [detailString appendString:@", Forced Only"];
            }
            if (track.burnedIn)
            {
                [detailString appendString:@", Burned In"];
            }
            if (track.def)
            {
                [detailString appendString:@", Default"];
            }

            [attrString appendString:@"\t" withAttributes: detailAttr];
            if (secondLine)
            {
                [attrString appendString:@"\t" withAttributes: detailAttr];
            }
            else
            {
                secondLine = YES;
            }
            [attrString appendString:detailString withAttributes: detailAttr];
            [attrString appendString:@"\n" withAttributes: detailAttr];
        }
    }

    return attrString;
}

- (NSAttributedString *)attributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    [self initStyles];

    @autoreleasepool
    {
        [attrString appendAttributedString:[self titleAttributedDescription]];
        [attrString appendAttributedString:[self presetAttributedDescription]];
        [attrString appendAttributedString:[self formatAttributedDescription]];
        [attrString appendAttributedString:[self destinationAttributedDescription]];
        [attrString appendAttributedString:[self pictureAttributedDescription]];
        [attrString appendAttributedString:[self filtersAttributedDescription]];
        [attrString appendAttributedString:[self videoAttributedDescription]];
        if (self.audio.countOfTracks > 1)
        {
            [attrString appendAttributedString:[self audioAttributedDescription]];
        }
        if (self.subtitles.countOfTracks > 1)
        {
            [attrString appendAttributedString:[self subtitlesAttributedDescription]];
        }
    }

    [attrString deleteCharactersInRange:NSMakeRange(attrString.length - 1, 1)];

    return attrString;
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

