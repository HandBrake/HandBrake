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

#import "HBAudioTransformers.h"
#import "HBLocalizationUtilities.h"

#import "HBRange.h"
#import "HBVideo.h"
#import "HBPicture.h"
#import "HBFilters.h"
#import "HBAudio.h"
#import "HBSubtitles.h"

#include "handbrake/handbrake.h"

// Text Styles
static NSDictionary            *detailAttr;
static NSDictionary            *detailBoldAttr;
static NSDictionary            *shortHeightAttr;
static HBMixdownTransformer    *mixdownTransformer;

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

- (NSArray<NSString *> *)angles
{
    NSMutableArray<NSString *> *angles = [NSMutableArray array];
    for (int i = 1; i <= self.title.angles; i++)
    {
        [angles addObject:[NSString stringWithFormat: @"%d", i]];
    }
    return angles;
}

- (NSArray<NSString *> *)containers
{
    NSMutableArray<NSString *> *containers = [NSMutableArray array];

    for (const hb_container_t *container = hb_container_get_next(NULL);
         container != NULL;
         container  = hb_container_get_next(container))
    {
        NSString *title = nil;
        if (container->format & HB_MUX_MASK_MP4)
        {
            title = @"MP4";
        }
        else if (container->format & HB_MUX_MASK_MKV)
        {
            title = @"MKV";
        }
        else if (container->format & HB_MUX_MASK_WEBM)
        {
            title = @"WebM";
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
    if (!detailAttr)
    {
        
        CGFloat indent = 100;
        NSBundle *bundle = [NSBundle bundleForClass:[HBJob class]];
        NSString *currentLocalization = bundle.preferredLocalizations.firstObject;
        if ([currentLocalization hasPrefix:@"de"])
        {
            indent = 120;
        }
        
        // Attributes
        NSMutableParagraphStyle *ps = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
        ps.headIndent = indent;
        ps.paragraphSpacing = 1.0;
        ps.tabStops = @[[[NSTextTab alloc] initWithType:NSRightTabStopType location:indent - 2],
                        [[NSTextTab alloc] initWithType:NSLeftTabStopType location:indent]];

        detailAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                       NSParagraphStyleAttributeName: ps,
                       NSForegroundColorAttributeName: NSColor.labelColor};

        detailBoldAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                           NSParagraphStyleAttributeName: ps,
                           NSForegroundColorAttributeName: NSColor.labelColor};

        shortHeightAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:6.0]};
    }
}

- (void)initTransformers
{
    if (!mixdownTransformer)
    {
        mixdownTransformer = [[HBMixdownTransformer alloc] init];
    }
}

- (NSString *)rangeDescription
{
    // Range type
    NSString *startStopString = @"";
    if (self.range.type == HBRangeTypeChapters)
    {
        startStopString = (self.range.chapterStart == self.range.chapterStop) ?
        [NSString stringWithFormat:HBKitLocalizedString(@"Chapter %d", @"Title description"), self.range.chapterStart + 1] :
        [NSString stringWithFormat:HBKitLocalizedString(@"Chapters %d through %d", @"Title description"), self.range.chapterStart + 1, self.range.chapterStop + 1];
    }
    else if (self.range.type == HBRangeTypeSeconds)
    {
        startStopString = [NSString stringWithFormat:HBKitLocalizedString(@"Seconds %d through %d", @"Title description"), self.range.secondsStart, self.range.secondsStop];
    }
    else if (self.range.type == HBRangeTypeFrames)
    {
        startStopString = [NSString stringWithFormat:HBKitLocalizedString(@"Frames %d through %d", @"Title description"), self.range.frameStart, self.range.frameStop];
    }

    NSMutableString *passesString = [NSMutableString string];
    // check to see if our first subtitle track is Foreign Language Search, in which case there is an in depth scan
    if (self.subtitles.tracks.firstObject.sourceTrackIdx == 1)
    {
        [passesString appendString:HBKitLocalizedString(@"1 Foreign Language Search Pass - ", @"Title description")];
    }
    if (self.video.qualityType != 1 && self.video.twoPass == YES)
    {
        if (self.video.turboTwoPass == YES)
        {
            [passesString appendString:HBKitLocalizedString(@"2 Video Passes First Turbo", @"Title description")];
        }
        else
        {
            [passesString appendString:HBKitLocalizedString(@"2 Video Passes", @"Title description")];
        }
    }

    if (passesString.length)
    {
        return [NSString stringWithFormat:HBKitLocalizedString(@"Title %d, %@, %@", @"Title description"),
                self.titleIdx, startStopString, passesString];
    }
    else
    {
        return [NSString stringWithFormat:HBKitLocalizedString(@"Title %d, %@", @"Title description"),
                self.titleIdx, startStopString];
    }
}

- (NSAttributedString *)rangeAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\t" withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Range:", @"Range description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t" withAttributes:detailAttr];
    [attrString appendString:self.rangeDescription withAttributes:detailAttr];
    [attrString appendString:@"\n" withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)presetAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\t" withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Preset:", @"Preset description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t" withAttributes:detailAttr];
    [attrString appendString:self.presetName withAttributes:detailAttr];
    [attrString appendString:@"\n" withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)formatAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    NSMutableString *options = [NSMutableString string];

    NSString *containerName;

    if (self.container & HB_MUX_MASK_MP4)
    {
        containerName = @"MP4";
    }
    else if (self.container & HB_MUX_MASK_MKV)
    {
        containerName = @"MKV";
    }
    else if (self.container & HB_MUX_MASK_WEBM)
    {
        containerName = @"WebM";
    }
    else
    {
        containerName = @(hb_container_get_name(self.container));
    }

    [options appendString:containerName];


    if (self.chaptersEnabled)
    {
        [options appendString:HBKitLocalizedString(@", Chapter Markers", @"Format description")];
    }

    if ((self.container & HB_MUX_MASK_MP4) && self.mp4HttpOptimize)
    {
        [options appendString:HBKitLocalizedString(@", Web Optimized", @"Format description")];
    }

    if ((self.container & HB_MUX_MASK_MP4) && self.alignAVStart)
    {
        [options appendString:HBKitLocalizedString(@", Align A/V Start", @"Format description")];
    }

    if ((self.container & HB_MUX_MASK_MP4)  && self.mp4iPodCompatible)
    {
        [options appendString:HBKitLocalizedString(@", iPod 5G Support", @"Format description")];
    }

    if ([options hasPrefix:@", "])
    {
        [options deleteCharactersInRange:NSMakeRange(0, 2)];
    }

    [attrString appendString:@"\t"      withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Format:", @"Format description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t"     withAttributes:detailAttr];
    [attrString appendString:options    withAttributes:detailAttr];
    [attrString appendString:@"\n"      withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)sourceAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\t"                          withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Source:", @"Source description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t"                         withAttributes:detailAttr];
    [attrString appendString:self.fileURL.path              withAttributes:detailAttr];
    [attrString appendString:@"\n"                          withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)destinationAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    [attrString appendString:@"\t"                          withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Destination:", @"Destination description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t"                         withAttributes:detailAttr];
    [attrString appendString:self.destinationURL.path    withAttributes:detailAttr];
    [attrString appendString:@"\n"                          withAttributes:detailAttr];

    return attrString;
}

- (NSAttributedString *)dimensionsAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

    NSString *pictureInfo = self.picture.summary;
    [attrString appendString:@"\t"      withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Dimensions:", @"Dimensions description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t"             withAttributes:detailAttr];
    [attrString appendString:pictureInfo       withAttributes:detailAttr];
    [attrString appendString:@"\n"             withAttributes:detailAttr];

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
            [summary appendFormat:@", %@ (%@)", HBKitLocalizedString(@"Detelecine", @"Dimensions description"), filters.detelecineCustomString];
        }
        else
        {
            [summary appendFormat:@", %@ (%@)", HBKitLocalizedString(@"Detelecine", @"Dimensions description"), [[[HBFilters detelecinePresetsDict] allKeysForObject:filters.detelecine] firstObject]];
        }
    }
    else if (![filters.deinterlace isEqualToString:@"off"])
    {
        // Deinterlace filters
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
                [summary appendFormat:@", %@ (%@)", type, [[[HBFilters yadifPresetsDict] allKeysForObject:filters.deinterlacePreset] firstObject]];
            }
            else if ([filters.deinterlace isEqualToString:@"bwdif"])
            {
                [summary appendFormat:@", %@ (%@)", type, [[[HBFilters bwdifPresetsDict] allKeysForObject:filters.deinterlacePreset] firstObject]];
            }
        }
    }

    // Deblock
    if (![filters.deblock isEqualToString:@"off"])
    {
        [summary appendFormat:@", %@ (%@", HBKitLocalizedString(@"Deblock", @"Filters description"), [[[HBFilters deblockPresetDict] allKeysForObject:filters.deblock] firstObject]];
        if (![filters.deblock isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", [[[HBFilters deblockTunesDict] allKeysForObject:filters.deblockTune] firstObject]];
        }
        else
        {
            [summary appendFormat:@", %@", filters.deblockCustomString];
        }

        [summary appendString:@")"];
    }

    // Denoise
    if (![filters.denoise isEqualToString:@"off"])
    {
        [summary appendFormat:@", %@ (%@", HBKitLocalizedString(@"Denoise", @"Filters description"), [[[HBFilters denoiseTypesDict] allKeysForObject:filters.denoise] firstObject]];
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

    // Chroma Smooth
    if (![filters.chromaSmooth isEqualToString:@"off"])
    {
        [summary appendFormat:@", %@ (%@", HBKitLocalizedString(@"Chroma Smooth", @"Filters description"), [[[HBFilters chromaSmoothPresetDict] allKeysForObject:filters.chromaSmooth] firstObject]];
        if (![filters.chromaSmooth isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", [[[HBFilters chromaSmoothTunesDict] allKeysForObject:filters.chromaSmoothTune] firstObject]];
        }
        else
        {
            [summary appendFormat:@", %@", filters.chromaSmoothCustomString];
        }

        [summary appendString:@")"];
    }

    // Sharpen
    if (![filters.sharpen isEqualToString:@"off"])
    {
        [summary appendFormat:@", %@ (%@", HBKitLocalizedString(@"Sharpen", @"Filters description"), [[[HBFilters sharpenTypesDict] allKeysForObject:filters.sharpen] firstObject]];
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
        [summary appendFormat:@", %@", HBKitLocalizedString(@"Grayscale", @"Filters description")];
    }

    // Colorspace
    if (![filters.colorspace isEqualToString:@"off"])
    {
        [summary appendFormat:@", %@ (%@", HBKitLocalizedString(@"Colorspace", @"Filters description"), [[[HBFilters colorspacePresetDict] allKeysForObject:filters.colorspace] firstObject]];
        if ([filters.colorspace isEqualToString:@"custom"])
        {
            [summary appendFormat:@", %@", filters.colorspaceCustomString];
        }

        [summary appendString:@")"];
    }

    if ([summary hasPrefix:@", "])
    {
        [summary deleteCharactersInRange:NSMakeRange(0, 2)];
    }

    // Optional String for Picture Filters
    if (summary.length)
    {
        [attrString appendString:@"\t"          withAttributes:detailAttr];
        [attrString appendString:HBKitLocalizedString(@"Filters:", @"Filters description") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t"         withAttributes:detailAttr];
        [attrString appendString:summary        withAttributes:detailAttr];
        [attrString appendString:@"\n"          withAttributes:detailAttr];
    }

    return attrString;
}

- (NSAttributedString *)videoAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    NSMutableString *videoInfo = [NSMutableString string];

    const char *encoderName = hb_video_encoder_get_name(self.video.encoder);
    [videoInfo appendFormat:HBKitLocalizedString(@"Encoder: %@, ", @"Video description"),
                            encoderName ? @(encoderName) : HBKitLocalizedString(@"Unknown", @"Video description")];

    [videoInfo appendString:HBKitLocalizedString(@"Framerate: ", @"Video description")];

    if (self.video.frameRate == 0)
    {
        if (self.video.frameRateMode == 0)
        {
            // we are using same as source with vfr
            [videoInfo appendString:HBKitLocalizedString(@"Same as source (variable)", @"Video description")];
        }
        else
        {
            [videoInfo appendString:HBKitLocalizedString(@"Same as source (constant)", @"Video description")];
        }
    }
    else
    {
        // we have a specified, constant framerate
        if (self.video.frameRateMode == 0)
        {
            [videoInfo appendFormat:HBKitLocalizedString(@"Peak %@ (may be lower)", @"Video description"), @(hb_video_framerate_get_name(self.video.frameRate))];
        }
        else
        {
            [videoInfo appendFormat:HBKitLocalizedString(@"Peak %@ (constant frame rate)", @"Video description"), @(hb_video_framerate_get_name(self.video.frameRate))];
        }
    }

    if (self.video.qualityType == 0) // ABR
    {
        [videoInfo appendFormat:@", "];
        [videoInfo appendFormat:HBKitLocalizedString(@"Bitrate: %d kbps", @"Video description"), self.video.avgBitrate];
    }
    else // CRF
    {
        [videoInfo appendFormat:@", "];
        [videoInfo appendString:[NSString localizedStringWithFormat:HBKitLocalizedString(@"Constant Quality: %.2f %s", @"Video description"), self.video.quality, hb_video_quality_get_name(self.video.encoder)]];
    }

    [attrString appendString:@"\t"       withAttributes:detailAttr];
    [attrString appendString:HBKitLocalizedString(@"Video:", @"Video description") withAttributes:detailBoldAttr];
    [attrString appendString:@" \t"      withAttributes:detailAttr];
    [attrString appendString:videoInfo   withAttributes:detailAttr];
    [attrString appendString:@"\n"       withAttributes:detailAttr];

    if (hb_video_encoder_get_presets(self.video.encoder) != NULL)
    {
        NSMutableString *encoderPresetInfo = [NSMutableString string];

        // we are using the x264 system
        [encoderPresetInfo appendFormat:HBKitLocalizedString(@"Preset: %@", @"Video description"), self.video.preset];

        if (self.video.tune.length || self.video.fastDecode)
        {
            [encoderPresetInfo appendString:@", "];
            [encoderPresetInfo appendString:HBKitLocalizedString(@"Tune: ", @"Video description")];

            if (self.video.tune.length)
            {
                [encoderPresetInfo appendString:self.video.tune];
            }
            if (self.video.fastDecode)
            {
                [encoderPresetInfo appendString:HBKitLocalizedString(@" - fastdecode", @"Video description")];
            }
        }
        if (self.video.videoOptionExtra.length)
        {
            [encoderPresetInfo appendString:@", "];
            [encoderPresetInfo appendFormat:HBKitLocalizedString(@"Options: %@", @"Video description"), self.video.videoOptionExtra];
        }
        if (self.video.profile.length)
        {
            [encoderPresetInfo appendString:@", "];
            [encoderPresetInfo appendFormat:HBKitLocalizedString(@"Profile: %@", @"Video description"), self.video.profile];
        }
        if (self.video.level.length)
        {
            [encoderPresetInfo appendString:@", "];
            [encoderPresetInfo appendFormat:HBKitLocalizedString(@"Level: %@", @"Video description"), self.video.level];
        }

        [attrString appendString:@"\t"                  withAttributes:detailAttr];
        [attrString appendString:HBKitLocalizedString(@"Video Options:", @"Video description") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t"                 withAttributes:detailAttr];
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
            lavcInfo = HBKitLocalizedString(@"default settings", @"Video description");
        }

        [attrString appendString:@"\t"      withAttributes:detailBoldAttr];
        [attrString appendString:HBKitLocalizedString(@"Video Options:", @"Video description") withAttributes:detailAttr];
        [attrString appendString:@" \t"     withAttributes:detailAttr];
        [attrString appendString:lavcInfo   withAttributes:detailAttr];
        [attrString appendString:@"\n"      withAttributes:detailAttr];
    }

    return attrString;
}

- (NSAttributedString *)audioAttributedDescription
{
    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
    BOOL secondLine = NO;

    [attrString appendString:@"\t" withAttributes: detailBoldAttr];
    [attrString appendString:HBKitLocalizedString(@"Audio:", @"Audio description") withAttributes: detailBoldAttr];
    [attrString appendString:@" " withAttributes: detailBoldAttr];

    for (HBAudioTrack *audioTrack in self.audio.tracks)
    {
        if (audioTrack.isEnabled)
        {
            NSMutableString *detailString = [NSMutableString stringWithFormat:HBKitLocalizedString(@"%@ ▸ Encoder: %@", @"Audio description"),
                                      self.audio.sourceTracks[audioTrack.sourceTrackIdx].displayName,
                                      @(hb_audio_encoder_get_name(audioTrack.encoder))];

            if ((audioTrack.encoder & HB_ACODEC_PASS_FLAG) == 0)
            {
                NSString *mixdown = [mixdownTransformer transformedValue:@(audioTrack.mixdown)];

                [detailString appendString:@", "];
                [detailString appendFormat:HBKitLocalizedString(@"Mixdown: %@, Samplerate: %@, Bitrate: %d kbps", @"Audio description"),
                                            mixdown,
                                            audioTrack.sampleRate ? [NSString stringWithFormat:@"%@ khz", @(hb_audio_samplerate_get_name(audioTrack.sampleRate))] : @"Auto",
                                            audioTrack.bitRate];

                if (0.0 < audioTrack.drc)
                {
                    [detailString appendString:@", "];
                    [detailString appendString:[NSString localizedStringWithFormat:HBKitLocalizedString(@"DRC: %.2f", @"Audio description"), audioTrack.drc]];
                }

                if (0.0 != audioTrack.gain)
                {
                    [detailString appendString:@", "];
                    [detailString appendString:[NSString localizedStringWithFormat:HBKitLocalizedString(@"Gain: %.2f", @"Audio description"), audioTrack.gain]];
                }
            }

            if (audioTrack.title.length)
            {
                [detailString appendString:@", "];
                [detailString appendFormat:HBKitLocalizedString(@"Title: %@", @"Audio track title description"), audioTrack.title];
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

    [attrString appendString:@"\t" withAttributes: detailBoldAttr];
    [attrString appendString:HBKitLocalizedString(@"Subtitles:", @"Subtitles description") withAttributes: detailBoldAttr];
    [attrString appendString:@" " withAttributes: detailBoldAttr];

    for (HBSubtitlesTrack *track in self.subtitles.tracks)
    {
        // Ignore the none track.
        if (track.isEnabled)
        {
            NSMutableString *detailString = [NSMutableString string];

            // remember that index 0 of Subtitles can contain "Foreign Audio Search
            [detailString appendString:self.subtitles.sourceTracks[track.sourceTrackIdx].displayName];

            if (track.title.length)
            {
                [detailString appendString:@", "];
                [detailString appendFormat:HBKitLocalizedString(@"Title: %@", @"Subtitles track title description"), track.title];
            }

            if (track.forcedOnly)
            {
                [detailString appendString:@", "];
                [detailString appendString:HBKitLocalizedString(@"Forced Only", @"Subtitles description")];
            }
            if (track.burnedIn)
            {
                [detailString appendString:@", "];
                [detailString appendString:HBKitLocalizedString(@"Burned In", @"Subtitles description")];
            }
            if (track.def)
            {
                [detailString appendString:@", "];
                [detailString appendString:HBKitLocalizedString(@"Default", @"Subtitles description")];
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
    [self initTransformers];

    @autoreleasepool
    {
        [attrString appendAttributedString:[self presetAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        [attrString appendAttributedString:[self sourceAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        [attrString appendAttributedString:[self destinationAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        [attrString appendAttributedString:[self formatAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        [attrString appendAttributedString:[self rangeAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        [attrString appendAttributedString:[self dimensionsAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        NSAttributedString *filters = [self filtersAttributedDescription];
        if (filters.length)
        {
            [attrString appendAttributedString:[self filtersAttributedDescription]];
            [attrString appendString:@"\n" withAttributes: shortHeightAttr];
        }

        [attrString appendAttributedString:[self videoAttributedDescription]];
        [attrString appendString:@"\n" withAttributes: shortHeightAttr];

        if (self.audio.countOfTracks > 1)
        {
            [attrString appendAttributedString:[self audioAttributedDescription]];
            [attrString appendString:@"\n" withAttributes: shortHeightAttr];
        }
        if (self.subtitles.countOfTracks > 1)
        {
            [attrString appendAttributedString:[self subtitlesAttributedDescription]];
        }
    }

    [attrString deleteCharactersInRange:NSMakeRange(attrString.length - 1, 1)];

    return attrString;
}

#pragma mark - Short descriptions

- (NSString *)videoShortDescription
{
    NSMutableString *info = [NSMutableString string];

    const char *encoderName = hb_video_encoder_get_name(self.video.encoder);
    [info appendString:encoderName ? @(encoderName) : HBKitLocalizedString(@"Unknown", @"HBJob -> video short description encoder name")];

    [info appendString:@", "];

    if (self.video.frameRate == 0)
    {
        if (self.video.frameRateMode == 0)
        {
            // we are using same as source with vfr
            [info appendString:HBKitLocalizedString(@"VFR", @"HBJob -> video short description framerate")];
        }
        else
        {
            [info appendString:HBKitLocalizedString(@"CRF", @"HBJob -> video short description framerate")];
        }
    }
    else
    {
        // we have a specified, constant framerate
        const char *frameRate = hb_video_framerate_get_name(self.video.frameRate);
        if (frameRate)
        {
            [info appendString:@(frameRate)];
        }
        if (self.video.frameRateMode == 0)
        {
            [info appendString:HBKitLocalizedString(@" FPS PFR", @"HBJob -> video short description framerate")];
        }
        else
        {
            [info appendString:HBKitLocalizedString(@" FPS CFR", @"HBJob -> video short description framerate")];
        }
    }

    return info;
}

- (NSString *)audioShortDescription
{
    NSMutableString *info = [NSMutableString string];

    NSUInteger index = 0;
    for (HBAudioTrack *audioTrack in self.audio.tracks)
    {
        if (audioTrack.isEnabled)
        {
            const char *encoder = hb_audio_encoder_get_name(audioTrack.encoder);
            if (encoder)
            {
                [info appendString:@(encoder)];
            }

            if ((audioTrack.encoder & HB_ACODEC_PASS_FLAG) == 0)
            {
                NSString *mixdown = [mixdownTransformer transformedValue:@(audioTrack.mixdown)];
                if (mixdown)
                {
                    [info appendString:@", "];
                    [info appendString:mixdown];
                }
            }

            [info appendString:@"\n"];
        }

        if (index == 1) {
            break;
        }
        index += 1;
    }

    if (self.audio.tracks.count > 3)
    {
        NSUInteger count = self.audio.tracks.count - 3;
        if (count == 1)
        {
            [info appendString:HBKitLocalizedString(@"+ 1 additional audio track", @"HBJob -> audio short description")];
        }
        else
        {
            [info appendFormat:HBKitLocalizedString(@"+ %lu additional audio tracks", @"HBJob -> audio short description"), (unsigned long)count];
        }
    }

    if ([info hasSuffix:@"\n"])
    {
        [info deleteCharactersInRange:NSMakeRange(info.length - 1, 1)];
    }

    return info;
}

- (NSString *)subtitlesShortDescription
{
    NSMutableString *info = [NSMutableString string];

    NSUInteger index = 0;
    for (HBSubtitlesTrack *track in self.subtitles.tracks)
    {
        // Ignore the none track.
        if (track.isEnabled)
        {
            // remember that index 0 of Subtitles can contain "Foreign Audio Search
            [info appendString:self.subtitles.sourceTracks[track.sourceTrackIdx].displayName];

            if (track.burnedIn)
            {
                [info appendString:HBKitLocalizedString(@", Burned", @"HBJob -> subtitles short description")];
            }

            [info appendString:@"\n"];
        }

        if (index == 1) {
            break;
        }
        index += 1;
    }

    if (self.subtitles.tracks.count > 3)
    {
        NSUInteger count = self.subtitles.tracks.count - 3;
        if (count == 1)
        {
            [info appendString:HBKitLocalizedString(@"+ 1 additional subtitles track", @"HBJob -> subtitles short description")];
        }
        else
        {
            [info appendFormat:HBKitLocalizedString(@"+ %lu additional subtitles tracks", @"HBJob -> subtitles short description"), (unsigned long)count];
        }
    }

    if ([info hasSuffix:@"\n"])
    {
        [info deleteCharactersInRange:NSMakeRange(info.length - 1, 1)];
    }

    return info;
}

- (NSString *)shortDescription
{
    NSMutableString *info = [NSMutableString string];

    [self initTransformers];

    [info appendString:[self videoShortDescription]];

    NSString *audioInfo = [self audioShortDescription];
    if (audioInfo.length)
    {
        [info appendString:@"\n"];
        [info appendString:audioInfo];
    }

    NSString *subtitlesInfo = [self subtitlesShortDescription];
    if (subtitlesInfo.length)
    {
        [info appendString:@"\n"];
        [info appendString:subtitlesInfo];
    }

    if (self.chaptersEnabled && self.chapterTitles.count > 1)
    {
        [info appendString:@"\n"];
        [info appendString:HBKitLocalizedString(@"Chapter Markers", @"HBJob -> chapters short description")];
    }

    return info;
}

- (NSString *)filtersShortDescription
{
    NSMutableString *summary = [NSMutableString string];
    HBFilters *filters = self.filters;

    // Detelecine
    if (![filters.detelecine isEqualToString:@"off"])
    {
        [summary appendString:HBKitLocalizedString(@"Detelecine", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    // Comb detect
    if (![filters.combDetection isEqualToString:@"off"])
    {
        [summary appendString:HBKitLocalizedString(@"Comb Detect", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    // Deinterlace
    if (![filters.deinterlace isEqualToString:@"off"])
    {
        // Deinterlace filters
        NSString *type = [[[HBFilters deinterlaceTypesDict] allKeysForObject:filters.deinterlace] firstObject];
        if (type)
        {
            [summary appendString:type];
            [summary appendString:@", "];
        }
    }

    // Deblock
    if (![filters.deblock isEqualToString:@"off"])
    {
        [summary appendString:HBKitLocalizedString(@"Deblock", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    // Denoise
    if (![filters.denoise isEqualToString:@"off"])
    {
        NSString *type = [[[HBFilters denoiseTypesDict] allKeysForObject:filters.denoise] firstObject];
        if (type)
        {
            [summary appendString:type];
            [summary appendString:@", "];
        }
    }

    // Chroma Smooth
    if (![filters.chromaSmooth isEqualToString:@"off"])
    {
        [summary appendString:HBKitLocalizedString(@"Chroma Smooth", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    // Sharpen
    if (![filters.sharpen isEqualToString:@"off"])
    {
        NSString *type = [[[HBFilters sharpenTypesDict] allKeysForObject:filters.sharpen] firstObject];
        if (type)
        {
            [summary appendString:type];
            [summary appendString:@", "];
        }
    }

    // Grayscale
    if (filters.grayscale)
    {
        [summary appendString:HBKitLocalizedString(@"Grayscale", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    // Colorspace
    if (![filters.colorspace isEqualToString:@"off"])
    {
        [summary appendString:HBKitLocalizedString(@"Colorspace", @"HBJob -> filters short description")];
        [summary appendString:@", "];
    }

    if ([summary hasSuffix:@", "])
    {
        [summary deleteCharactersInRange:NSMakeRange(summary.length - 2, 2)];
    }

    if (summary.length == 0)
    {
        [summary appendString:HBKitLocalizedString(@"None", @"HBJob -> filters short description")];
    }

    return summary;
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
        return @"MP4";
    }
    else if (container & HB_MUX_MASK_MKV)
    {
        return @"MKV";
    }
    else if (container & HB_MUX_MASK_WEBM)
    {
        return @"WebM";
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
    if ([value isEqualToString:@"MP4"])
    {
        return @(HB_MUX_AV_MP4);
    }
    else if ([value isEqualToString:@"MKV"])
    {
        return @(HB_MUX_AV_MKV);
    }
    else if ([value isEqualToString:@"WebM"])
    {
        return @(HB_MUX_AV_WEBM);
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
