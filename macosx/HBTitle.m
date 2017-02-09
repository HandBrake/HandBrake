/*  HBTitle.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitle.h"
#import "HBTitle+Private.h"
#import "HBChapter.h"
#import "HBPreset.h"
#import "NSDictionary+HBAdditions.h"

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
extern NSString *keySubTrackLanguageIsoCode;
extern NSString *keySubTrackType;

@interface HBTitle ()

@property (nonatomic, readonly) hb_title_t *hb_title;
@property (nonatomic, readonly) hb_handle_t *hb_handle;
@property (nonatomic, readwrite, copy) NSString *name;

@property (nonatomic, readwrite) NSArray *audioTracks;
@property (nonatomic, readwrite) NSArray *subtitlesTracks;
@property (nonatomic, readwrite) NSArray *chapters;

@end

@implementation HBTitle

- (instancetype)initWithTitle:(hb_title_t *)title handle:(hb_handle_t *)handle featured:(BOOL)featured
{
    self = [super init];
    if (self)
    {
        if (!title)
        {
            return nil;
        }

        _hb_title = title;
        _hb_handle = handle;
        _featured = featured;
    }

    return self;
}

- (NSString *)name
{
    if (!_name)
    {
        _name = @(self.hb_title->name);

        // Use the bundle name for eyetv
        NSURL *parentURL = self.url.URLByDeletingLastPathComponent;
        if ([parentURL.pathExtension caseInsensitiveCompare:@"eyetv"] == NSOrderedSame)
        {
            _name = parentURL.URLByDeletingPathExtension.lastPathComponent;
        }

        // If the name is empty use file/directory name
        if (_name.length == 0)
        {
            _name = [@(self.hb_title->path) lastPathComponent];
        }
    }

    return _name;
}

- (BOOL)isStream
{
    return (self.hb_title->type == HB_STREAM_TYPE || self.hb_title->type == HB_FF_STREAM_TYPE);
}

- (NSString *)description
{
    if (self.hb_title->type == HB_BD_TYPE)
    {
        return [NSString stringWithFormat:@"%@ %d (%05d.MPLS) - %@",
                 @(self.hb_title->name), self.hb_title->index, self.hb_title->playlist,
                 self.timeCode];
    }
    else
    {
        return [NSString stringWithFormat:@"%@ %d - %@",
                 @(self.hb_title->name), self.hb_title->index,
                 self.timeCode];
    }
}

- (NSURL *)url
{
    return [NSURL fileURLWithPath:@(_hb_title->path)];
}

- (int)index
{
    return self.hb_title->index;
}

- (int)angles
{
    return self.hb_title->angle_count;
}

- (int)duration
{
    return (self.hb_title->hours * 3600) + (self.hb_title->minutes * 60) + (self.hb_title->seconds);
}

- (int)frames
{
    double frames = (double)self.hb_title->duration / 90000.f * self.hb_title->vrate.num / self.hb_title->vrate.den;
    return (int)ceil(frames);
}

- (NSString *)timeCode
{
    return [NSString stringWithFormat:@"%02dh%02dm%02ds",
            self.hb_title->hours, self.hb_title->minutes, self.hb_title->seconds];
}

- (int)width
{
    return _hb_title->geometry.width;
}

- (int)height
{
    return _hb_title->geometry.height;
}

- (int)parWidth
{
    return _hb_title->geometry.par.num;
}

- (int)parHeight
{
    return _hb_title->geometry.par.den;
}

- (int)autoCropTop
{
    return _hb_title->crop[0];
}

- (int)autoCropBottom
{
    return _hb_title->crop[1];
}

- (int)autoCropLeft
{
    return _hb_title->crop[2];
}

- (int)autoCropRight
{
    return _hb_title->crop[3];
}


- (NSArray *)audioTracks
{
    if (!_audioTracks)
    {
        NSMutableArray *tracks = [NSMutableArray array];
        hb_list_t *list = self.hb_title->list_audio;
        int count = hb_list_count(list);

        // Initialize the audio list of available audio tracks from this title
        for (int i = 0; i < count; i++)
        {
            hb_audio_config_t *audio = hb_list_audio_config_item(list, i);
            [tracks addObject: @{keyAudioTrackIndex: @(i + 1),
                                           keyAudioTrackName: [NSString stringWithFormat: @"%d: %@", i, @(audio->lang.description)],
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
        hb_list_t *list = self.hb_title->list_subtitle;
        int count = hb_list_count(list);

        for (int i = 0; i < count; i++)
        {
            hb_subtitle_t *subtitle = hb_list_item(self.hb_title->list_subtitle, i);

            // Human-readable representation of subtitle->source
            NSString *bitmapOrText  = subtitle->format == PICTURESUB ? @"Bitmap" : @"Text";
            NSString *subSourceName = @(hb_subsource_name(subtitle->source));

            // Use the native language name if available
            iso639_lang_t *language = lang_for_code2(subtitle->iso639_2);
            NSString *nativeLanguage = strlen(language->native_name) ? @(language->native_name) : @(language->eng_name);

            /* create a dictionary of source subtitle information to store in our array */
            [tracks addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", i, nativeLanguage, bitmapOrText, subSourceName],
                                              keySubTrackType: @(subtitle->source),
                                              keySubTrackLanguageIsoCode: @(subtitle->iso639_2)}];
        }

        _subtitlesTracks = [tracks copy];
    }
    
    return _subtitlesTracks;
}

- (NSArray<HBChapter *> *)chapters
{
    if (!_chapters)
    {
        NSMutableArray<HBChapter *> *chapters = [NSMutableArray array];

        for (int i = 0; i < hb_list_count(self.hb_title->list_chapter); i++)
        {
            hb_chapter_t *chapter = hb_list_item(self.hb_title->list_chapter, i);

            if (chapter != NULL)
            {
                NSString *title;
                if (chapter->title != NULL)
                {
                    title = @(chapter->title);
                }
                else
                {
                    title = [NSString stringWithFormat:@"Chapter %d", i + 1];
                }

                [chapters addObject:[[HBChapter alloc] initWithTitle:title
                                                               index:i + 1
                                                            duration:chapter->duration]];
            }
        }

        _chapters = [chapters copy];
    }

    return _chapters;
}

- (NSDictionary *)jobSettingsWithPreset:(HBPreset *)preset
{
    NSDictionary *result = nil;

    hb_dict_t *hb_preset = [preset content].hb_value;
    hb_dict_t *job = hb_preset_job_init(self.hb_handle, self.hb_title->index, hb_preset);

    if (job)
    {
        result = [[NSDictionary alloc] initWithHBDict:job];
    }

    hb_dict_free(&hb_preset);
    hb_dict_free(&job);

    return result;
}

@end
