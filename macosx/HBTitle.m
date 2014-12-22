/*  HBTitle.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitle.h"

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

@interface HBTitle ()

@property (nonatomic, readwrite) NSString *name;

@property (nonatomic, readwrite) NSArray *audioTracks;
@property (nonatomic, readwrite) NSArray *subtitlesTracks;
@property (nonatomic, readwrite) NSArray *chapters;

@end

@implementation HBTitle

- (instancetype)initWithTitle:(hb_title_t *)title featured:(BOOL)featured
{
    self = [super init];
    if (self)
    {
        if (!title)
        {
            [self release];
            return nil;
        }

        _hb_title = title;
        _featured = featured;
    }

    return self;
}

- (void)dealloc
{
    [_name release];
    [_audioTracks release];
    [_subtitlesTracks release];
    [_chapters release];

    [super dealloc];
}
- (NSString *)name
{
    if (!_name)
    {
        _name = [@(self.hb_title->name) retain];
    }

    return _name;
}

- (NSString *)description
{
    if (self.hb_title->type == HB_BD_TYPE)
    {
        return [NSString stringWithFormat:@"%@ %d (%05d.MPLS) - %02dh%02dm%02ds",
                 @(self.hb_title->name), self.hb_title->index, self.hb_title->playlist,
                 self.hb_title->hours, self.hb_title->minutes, self.hb_title->seconds];
    }
    else
    {
        return [NSString stringWithFormat:@"%@ %d - %02dh%02dm%02ds",
                 @(self.hb_title->name), self.hb_title->index,
                 self.hb_title->hours, self.hb_title->minutes, self.hb_title->seconds];
    }
}

- (NSInteger)angles
{
    return self.hb_title->angle_count;
}

- (NSArray *)audioTracks
{
    if (!_audioTracks)
    {
        NSMutableArray *tracks = [NSMutableArray array];
        hb_audio_config_t *audio;
        hb_list_t *list = self.hb_title->list_audio;
        int count = hb_list_count(list);

        // Initialize the audio list of available audio tracks from this title
        for (int i = 0; i < count; i++)
        {
            audio = (hb_audio_config_t *) hb_list_audio_config_item(list, i);
            [tracks addObject: @{keyAudioTrackIndex: @(i + 1),
                                           keyAudioTrackName: [NSString stringWithFormat: @"%d: %s", i, audio->lang.description],
                                           keyAudioInputBitrate: @(audio->in.bitrate / 1000),
                                           keyAudioInputSampleRate: @(audio->in.samplerate),
                                           keyAudioInputCodec: @(audio->in.codec),
                                           keyAudioInputCodecParam: @(audio->in.codec_param),
                                           keyAudioInputChannelLayout: @(audio->in.channel_layout),
                                           keyAudioTrackLanguageIsoCode: @(audio->lang.iso639_2)}];
        }

        _audioTracks = [tracks copy];
    }

    return _audioTracks;
}

- (NSArray *)subtitlesTracks
{
    if (!_subtitlesTracks)
    {
        NSMutableArray *tracks = [NSMutableArray array];
        hb_subtitle_t *subtitle;
        hb_list_t *list = self.hb_title->list_audio;
        int count = hb_list_count(list);

        NSMutableArray *forcedSourceNamesArray = [[NSMutableArray alloc] init];
        //NSString *foreignAudioSearchTrackName = nil;

        for (int i = 0; i < count; i++)
        {
            subtitle = (hb_subtitle_t *)hb_list_item(self.hb_title->list_subtitle, i);

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
            [tracks addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", i, nativeLanguage, bitmapOrText, subSourceName],
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

        _subtitlesTracks = [tracks copy];
    }
    
    return _subtitlesTracks;
}

- (NSArray *)chapters
{
    if (!_chapters)
    {
        NSMutableArray *chapters = [NSMutableArray array];

        for (int i = 0; i < hb_list_count(self.hb_title->list_chapter); i++)
        {
            hb_chapter_t *chapter = hb_list_item(self.hb_title->list_chapter, i);

            if (chapter != NULL)
            {
                if (chapter->title != NULL)
                {
                    [chapters addObject:[NSString
                                         stringWithFormat:@"%s",
                                         chapter->title]];
                }
                else
                {
                    [chapters addObject:[NSString
                                         stringWithFormat:@"Chapter %d",
                                         i + 1]];
                }
            }
        }

        _chapters = [chapters copy];
    }

    return _chapters;
}


@end
