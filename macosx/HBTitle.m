/*  HBTitle.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitle.h"
#import "HBTitle+Private.h"
#import "HBChapter.h"
#import "HBPreset.h"
#import "NSDictionary+HBAdditions.h"
#import "HBLocalizationUtilities.h"
#import "HBCodingUtilities.h"
#import "HBSecurityAccessToken.h"
#import "HBUtilities.h"

#include "handbrake/lang.h"

@interface HBMetadata ()

@property (nonatomic, readonly, nullable) hb_metadata_t *metadata;
@property (nonatomic, copy, nullable) NSString *releaseDate;

@end

@implementation HBMetadata
- (instancetype)initWithMetadata:(hb_metadata_t *)metadata
{
    self = [super init];
    if (self)
    {
        _metadata = metadata;
    }
    return self;
}

- (NSString *)releaseDate
{
    if (_metadata && _metadata->dict)
    {
        const char *releaseDate = hb_dict_get_string(_metadata->dict, "ReleaseDate");
        if (releaseDate)
        {
            return @(releaseDate);
        }
    }
    return nil;
}

@end

@implementation HBTitleAudioTrack

- (instancetype)initWithDisplayName:(NSString *)displayName
{
    self = [super init];
    if (self)
    {
        _index = -1;
        _displayName = [displayName copy];
        _title = @"";
        _isoLanguageCode = @"";
    }
    return self;
}

- (instancetype)initWithAudioTrack:(hb_audio_config_t *)audio index:(int)index
{
    self = [super init];
    if (self)
    {
        _index = audio->index;
        _displayName = [NSString stringWithFormat: @"%d: %@", index, @(audio->lang.description)];
        _title = audio->in.name ? @(audio->in.name) : nil;
        _bitRate = audio->in.bitrate / 1000;
        _sampleRate = audio->in.samplerate;
        _codec = audio->in.codec;
        _codecParam = audio->in.codec_param;
        _channelLayout = audio->in.channel_layout;

        _isoLanguageCode = @(audio->lang.iso639_2);
    }
    return self;
}

+ (BOOL)supportsSecureCoding { return YES; }

- (void)encodeWithCoder:(nonnull NSCoder *)coder
{
    encodeInt(_index);
    encodeObject(_displayName);
    encodeObject(_title);
    encodeInt(_bitRate);
    encodeInt(_sampleRate);
    encodeInt(_codec);
    encodeInt(_codecParam);
    encodeInteger(_channelLayout);

    encodeObject(_isoLanguageCode);
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)decoder
{
    self = [super init];
    if (self)
    {
        decodeInt(_index);
        decodeObjectOrFail(_displayName, NSString);
        decodeObject(_title, NSString);
        decodeInt(_bitRate);
        decodeInt(_sampleRate);
        decodeInt(_codec);
        decodeInt(_codecParam);
        decodeInteger(_channelLayout);
        decodeObjectOrFail(_isoLanguageCode, NSString);
    }
    return self;
fail:
    return nil;
}

@end


@interface HBTitleSubtitlesTrack ()

@property (nonatomic, readonly, nullable) NSData *bookmark;

@property (nonatomic, readwrite) NSInteger accessCount;
@property (nonatomic, readwrite) HBSecurityAccessToken *fileURLToken;

@end

@implementation HBTitleSubtitlesTrack

- (instancetype)initWithDisplayName:(NSString *)displayName type:(int)type fileURL:(nullable NSURL *)fileURL
{
    self = [super init];
    if (self)
    {
        _index = -1;
        _displayName = [displayName copy];
        _type = type;
        _isoLanguageCode = @"und";
        _fileURL = fileURL;
    }
    return self;
}

- (instancetype)initWithSubtitlesTrack:(hb_subtitle_t *)subtitle index:(int)index
{
    self = [super init];
    if (self)
    {
        _index = index;
        _displayName = [NSString stringWithFormat:@"%d: %@", index, @(subtitle->lang)];
        _title = subtitle->name ? @(subtitle->name) : nil;
        _type = subtitle->source;
        _isoLanguageCode = @(subtitle->iso639_2);
    }
    return self;
}

- (void)refreshSecurityScopedResources
{
    if (_bookmark)
    {
        NSURL *resolvedURL = [HBUtilities URLFromBookmark:_bookmark];
        if (resolvedURL)
        {
            _fileURL = resolvedURL;
        }
    }
}

- (BOOL)startAccessingSecurityScopedResource
{
#ifdef __SANDBOX_ENABLED__
    if (self.accessCount == 0)
    {
        if (_fileURL)
        {
            self.fileURLToken = [HBSecurityAccessToken tokenWithObject:_fileURL];
        }
    }
    self.accessCount += 1;
    return YES;
#else
    return NO;
#endif
}

- (void)stopAccessingSecurityScopedResource
{
#ifdef __SANDBOX_ENABLED__
    self.accessCount -= 1;
    NSAssert(self.accessCount >= 0, @"[HBSubtitles stopAccessingSecurityScopedResource:] unbalanced call");
    if (self.accessCount == 0)
    {
        self.fileURLToken = nil;
    }
#endif
}

+ (BOOL)supportsSecureCoding { return YES ;}

- (void)encodeWithCoder:(nonnull NSCoder *)coder
{
    encodeInt(_index);
    encodeObject(_displayName);
    encodeObject(_title);
    encodeInt(_type);
    encodeObject(_isoLanguageCode);
#ifdef __SANDBOX_ENABLED__
    if (_fileURL)
    {
        if (!_bookmark)
        {
            _bookmark = [HBUtilities bookmarkFromURL:_fileURL
                                                options:NSURLBookmarkCreationWithSecurityScope | NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess];
        }
        encodeObject(_bookmark);
    }
#endif
    encodeObject(_fileURL);
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)decoder
{
    self = [super init];
    if (self)
    {
        decodeInt(_index);
        decodeObjectOrFail(_displayName, NSString);
        decodeObject(_title, NSString);
        decodeInt(_type);
        decodeObjectOrFail(_isoLanguageCode, NSString);

#ifdef __SANDBOX_ENABLED__
        decodeObject(_bookmark, NSData);

        if (_bookmark)
        {
            decodeObjectOrFail(_fileURL, NSURL);
        }
#else
        decodeObject(_fileURL, NSURL);
#endif
    }
    return self;
fail:
    return nil;
}

@end

@interface HBTitle ()

@property (nonatomic, readonly) hb_title_t *hb_title;
@property (nonatomic, readonly) hb_handle_t *hb_handle;
@property (nonatomic, readwrite, copy) NSString *name;
@property (nonatomic, readwrite) HBMetadata *metadata;
@property (nonatomic, readwrite) NSArray *audioTracks;
@property (nonatomic, readwrite) NSArray<HBTitleSubtitlesTrack *> *subtitlesTracks;
@property (nonatomic, readwrite) NSArray<HBChapter *> *chapters;

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

        _metadata = [[HBMetadata alloc] initWithMetadata:title->metadata];
    }

    return self;
}

-  (NSString *)name
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
            _name =  @(self.hb_title->path).lastPathComponent;
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
        return [NSString stringWithFormat:@"%d - %@ - %05d.MPLS",
                 self.hb_title->index, self.timeCode, self.hb_title->playlist];
    }
    else if (self.hb_title->type == HB_DVD_TYPE)
    {
        return [NSString stringWithFormat:@"%d - %@",
                self.hb_title->index, self.timeCode];
    }
    else
    {
        return [NSString stringWithFormat:@"%d - %@ - %@",
                self.hb_title->index, self.timeCode,  @(self.hb_title->name)];
    }
}

- (NSString *)shortFormatDescription
{
    NSMutableString *format = [[NSMutableString alloc] init];

    [format appendFormat:@"%dx%d", _hb_title->geometry.width, _hb_title->geometry.height];

    if (_hb_title->geometry.par.num != 1 || _hb_title->geometry.par.den != 1)
    {
        [format appendFormat:@" (%dx%d)", _hb_title->geometry.width * _hb_title->geometry.par.num / _hb_title->geometry.par.den,
         _hb_title->geometry.height];
    }

    [format appendString:@", "];

    NSString *fps = [NSString localizedStringWithFormat:HBKitLocalizedString(@"%.6g FPS", @"Title short description -> video format"), _hb_title->vrate.num / (double)_hb_title->vrate.den];
    [format appendString:fps];

    NSString *dynamicRange = @"SDR";

    if (_hb_title->hdr_10_plus)
    {
        dynamicRange = @"HDR10+";
    }
    else if (_hb_title->mastering.has_primaries && _hb_title->mastering.has_luminance)
    {
        dynamicRange = @"HDR10";
    }
    else if (_hb_title->color_transfer == 16 || _hb_title->color_transfer == 18)
    {
        dynamicRange = @"HDR";
    }

    if (_hb_title->dovi.dv_profile && _hb_title->hdr_10_plus)
    {
        dynamicRange = [NSString stringWithFormat:@"Dolby Vision %d.%d HDR10+", _hb_title->dovi.dv_profile, _hb_title->dovi.dv_bl_signal_compatibility_id];
    }
    else if (_hb_title->dovi.dv_profile)
    {
        dynamicRange = [NSString stringWithFormat:@"Dolby Vision %d.%d", _hb_title->dovi.dv_profile, _hb_title->dovi.dv_bl_signal_compatibility_id];
    }

    [format appendFormat:@", %@ (", dynamicRange];

    int bit_depth = hb_get_bit_depth(_hb_title->pix_fmt);
    if (bit_depth)
    {
        [format appendFormat:@"%d-bit ", hb_get_bit_depth(_hb_title->pix_fmt)];
    }

    int h_shift, v_shift, chroma_available;
    chroma_available = hb_get_chroma_sub_sample(_hb_title->pix_fmt, &h_shift, &v_shift);
    if (chroma_available == 0)
    {
        int h_value = 4 >> h_shift;
        int v_value = v_shift ? 0 : h_value;
        [format appendFormat:@"4:%d:%d", h_value, v_value];
    }

    if (bit_depth || chroma_available == 0)
    {
        [format appendString:@", "];
    }

    [format appendFormat:@"%d-%d-%d)", _hb_title->color_prim, _hb_title->color_transfer, _hb_title->color_matrix];

    hb_list_t *audioList = _hb_title->list_audio;
    int audioCount = hb_list_count(audioList);

    if (audioCount > 1)
    {
        [format appendFormat:HBKitLocalizedString(@", %d audio tracks", @"Title short description -> audio format"), audioCount];
    }
    else if (audioCount == 1)
    {
        [format appendString:HBKitLocalizedString(@", 1 audio track", @"Title short description -> audio format")];
    }

    hb_list_t *subList = _hb_title->list_subtitle;
    int subCount = hb_list_count(subList);

    if (subCount > 1)
    {
        [format appendFormat:HBKitLocalizedString(@", %d subtitles tracks", @"Title short description -> subtitles format"), subCount];
    }
    else if (subCount == 1)
    {
        [format appendString:HBKitLocalizedString(@", 1 subtitles track", @"Title short description -> subtitles format")];
    }

    return format;
}

- (NSURL *)url
{
    return [NSURL fileURLWithPath:@(_hb_title->path)];
}

- (int)index
{
    return self.hb_title->index;
}

- (BOOL)keepDuplicateTitles
{
    return self.hb_title->keep_duplicate_titles;
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
    return [NSString stringWithFormat:@"%02d:%02d:%02d",
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

- (int)looseAutoCropTop
{
    return _hb_title->loose_crop[0];
}

- (int)looseAutoCropBottom
{
    return _hb_title->loose_crop[1];
}

- (int)looseAutoCropLeft
{
    return _hb_title->loose_crop[2];
}

- (int)looseAutoCropRight
{
    return _hb_title->loose_crop[3];
}

- (NSArray<HBTitleAudioTrack *> *)audioTracks
{
    if (!_audioTracks)
    {
        NSMutableArray<HBTitleAudioTrack *> *tracks = [NSMutableArray array];
        hb_list_t *list = self.hb_title->list_audio;
        int count = hb_list_count(list);

        // Initialize the audio list of available audio tracks from this title
        for (int i = 0; i < count; i++)
        {
            hb_audio_config_t *audio = hb_list_audio_config_item(list, i);
            [tracks addObject:[[HBTitleAudioTrack alloc ] initWithAudioTrack:audio index:i]];
        }

        _audioTracks = [tracks copy];
    }

    return _audioTracks;
}

- (NSArray<HBTitleSubtitlesTrack *> *)subtitlesTracks
{
    if (!_subtitlesTracks)
    {
        NSMutableArray<HBTitleSubtitlesTrack *> *tracks = [NSMutableArray array];
        hb_list_t *list = self.hb_title->list_subtitle;
        int count = hb_list_count(list);

        for (int i = 0; i < count; i++)
        {
            hb_subtitle_t *subtitle = hb_list_item(self.hb_title->list_subtitle, i);
            [tracks addObject:[[HBTitleSubtitlesTrack alloc] initWithSubtitlesTrack:subtitle index:i]];
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
        uint64_t currentTime = 0;
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

                if (title == NULL)
                {
                    title = [NSString stringWithFormat:HBKitLocalizedString(@"Chapter %d", "Title -> chapter name"), i + 1];
                }

                [chapters addObject:[[HBChapter alloc] initWithTitle:title
                                                               index:i + 1
                                                           timestamp:currentTime
                                                            duration:chapter->duration]];
                currentTime += chapter->duration;
            }
        }

        _chapters = [chapters copy];
    }

    return _chapters;
}

- (nullable NSDictionary *)jobSettingsWithPreset:(HBPreset *)preset
{
    NSDictionary *result = nil;

    hb_dict_t *hb_preset = preset.content.hb_value;
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
