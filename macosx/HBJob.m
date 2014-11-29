/*  HBJob.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob.h"
#import "HBAudioDefaults.h"
#import "HBSubtitlesDefaults.h"
#import "HBPreset.h"

#include "lang.h"

extern NSString *keyAudioTrackIndex;
extern NSString *keyAudioTrackName;
extern NSString *keyAudioInputBitrate;
extern NSString *keyAudioInputSampleRate;
extern NSString *keyAudioInputCodec;
extern NSString *keyAudioInputCodecParam;
extern NSString *keyAudioInputChannelLayout;
extern NSString *keyAudioTrackLanguageIsoCode;

extern NSString *keySubTrackName;
extern NSString *keySubTrackIndex;
extern NSString *keySubTrackLanguage;
extern NSString *keySubTrackLanguageIsoCode;
extern NSString *keySubTrackType;

extern NSString *keySubTrackForced;
extern NSString *keySubTrackBurned;
extern NSString *keySubTrackDefault;

extern NSString *keySubTrackSrtOffset;
extern NSString *keySubTrackSrtFilePath;
extern NSString *keySubTrackSrtCharCode;

@implementation HBJob

- (instancetype)initWithTitle:(hb_title_t *)title url:(NSURL *)fileURL andPreset:(HBPreset *)preset
{
    self = [super init];
    if (self) {
        _title = title;
        _fileURL = [fileURL copy];

        _audioTracks = [[NSMutableArray alloc] init];
        _subtitlesTracks = [[NSMutableArray alloc] init];
        _chapters = [[NSMutableArray alloc] init];

        _audioDefaults = [[HBAudioDefaults alloc] init];
        _subtitlesDefaults = [[HBSubtitlesDefaults alloc] init];

        [self loadAudioTracks];
        [self loadSubtitlesTracks];
        [self loadChapters];
    }
    return self;
}

- (void)applyPreset:(HBPreset *)preset
{
    [self.audioDefaults applySettingsFromPreset:preset.content];
    [self.subtitlesDefaults applySettingsFromPreset:preset.content];
}

#pragma mark - initialization

- (void)loadAudioTracks
{
    hb_audio_config_t *audio;
    hb_list_t *list = self.title->list_audio;
    int count = hb_list_count(list);

    // Initialize the audio list of available audio tracks from this title
    for (int i = 0; i < count; i++)
    {
        audio = (hb_audio_config_t *) hb_list_audio_config_item(list, i);
        [self.audioTracks addObject: @{keyAudioTrackIndex: @(i + 1),
                                       keyAudioTrackName: [NSString stringWithFormat: @"%d: %s", i, audio->lang.description],
                                       keyAudioInputBitrate: @(audio->in.bitrate / 1000),
                                       keyAudioInputSampleRate: @(audio->in.samplerate),
                                       keyAudioInputCodec: [NSNumber numberWithUnsignedInteger: audio->in.codec],
                                       keyAudioInputCodecParam: [NSNumber numberWithUnsignedInteger: audio->in.codec_param],
                                       keyAudioInputChannelLayout: @(audio->in.channel_layout),
                                       keyAudioTrackLanguageIsoCode: @(audio->lang.iso639_2)}];
    }
}

- (void)loadSubtitlesTracks
{
    hb_subtitle_t *subtitle;
    hb_list_t *list = self.title->list_audio;
    int count = hb_list_count(list);

    NSMutableArray *forcedSourceNamesArray = [[NSMutableArray alloc] init];
    //NSString *foreignAudioSearchTrackName = nil;

    for (int i = 0; i < count; i++)
    {
        subtitle = (hb_subtitle_t *)hb_list_item(self.title->list_subtitle, i);

        /* Human-readable representation of subtitle->source */
        NSString *bitmapOrText  = subtitle->format == PICTURESUB ? @"Bitmap" : @"Text";
        NSString *subSourceName = @(hb_subsource_name(subtitle->source));

        /* if the subtitle track can be forced, add its source name to the array */
        if (hb_subtitle_can_force(subtitle->source) && [forcedSourceNamesArray containsObject:subSourceName] == NO)
        {
            [forcedSourceNamesArray addObject:subSourceName];
        }

        // Use the native language name if available
        iso639_lang_t *language = lang_for_code2(subtitle->iso639_2);
        NSString *nativeLanguage = strlen(language->native_name) ? @(language->native_name) : @(language->eng_name);

        /* create a dictionary of source subtitle information to store in our array */
        [self.subtitlesTracks addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", i, nativeLanguage, bitmapOrText, subSourceName],
                                          keySubTrackIndex: @(i),
                                          keySubTrackType: @(subtitle->source),
                                          keySubTrackLanguage: nativeLanguage,
                                          keySubTrackLanguageIsoCode: @(subtitle->iso639_2)}];
    }

    /* now set the name of the Foreign Audio Search track */
    if ([forcedSourceNamesArray count])
    {
        [forcedSourceNamesArray sortUsingComparator:^(id obj1, id obj2)
         {
             return [((NSString *)obj1) compare:((NSString *)obj2)];
         }];

        NSString *tempList = @"";
        for (NSString *tempString in forcedSourceNamesArray)
        {
            if ([tempList length])
            {
                tempList = [tempList stringByAppendingString:@", "];
            }
            tempList = [tempList stringByAppendingString:tempString];
        }
        //foreignAudioSearchTrackName = [NSString stringWithFormat:@"Foreign Audio Search (Bitmap) (%@)", tempList];
    }
    else
    {
        //foreignAudioSearchTrackName = @"Foreign Audio Search (Bitmap)";
    }
    [forcedSourceNamesArray release];
}

- (void)loadChapters
{
    for (int i = 0; i < hb_list_count(self.title->job->list_chapter); i++)
    {
        hb_chapter_t *chapter = hb_list_item(self.title->job->list_chapter, i);
        if (chapter != NULL)
        {
            if (chapter->title != NULL)
            {
                [self.chapters addObject:[NSString
                                                stringWithFormat:@"%s",
                                                chapter->title]];
            }
            else
            {
                [self.chapters addObject:[NSString
                                                stringWithFormat:@"Chapter %d",
                                                i + 1]];
            }
        }
    }
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
}

- (id)initWithCoder:(NSCoder *)decoder
{
    return nil;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    return nil;
}

@end
