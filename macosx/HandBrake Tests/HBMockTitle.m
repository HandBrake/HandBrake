//
//  HBMockTitle.m
//  HandBrake
//
//  Created by Damiano Galassi on 30/05/15.
//
//

#import "HBMockTitle.h"

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

@implementation HBMockTitle

- (instancetype)init
{
    self = [super init];
    return self;
}

- (NSString *)name
{
    return @"Test.mkv";
}

- (BOOL)isStream
{
    return YES;
}

- (NSString *)description
{
    return @"Test Title";
}

- (NSURL *)url
{
    return [NSURL fileURLWithPath:@"/Test.mkv"];
}

- (int)index
{
    return 1;
}

- (int)angles
{
    return 1;
}

- (int)duration
{
    return 60;
}

- (int)frames
{
    return 60 * 25;
}

- (NSString *)timeCode
{
    return @"00:01:00";
}

- (int)width
{
    return 1280;
}

- (int)height
{
    return 720;
}

- (int)parWidth
{
    return 1;
}

- (int)parHeight
{
    return 1;
}

- (int)autoCropTop
{
    return 20;
}

- (int)autoCropBottom
{
    return 22;
}

- (int)autoCropLeft
{
    return 12;
}

- (int)autoCropRight
{
    return 15;
}

- (NSArray *)audioTracks
{
    NSMutableArray *tracks = [NSMutableArray array];
    [tracks addObject: @{keyAudioTrackIndex: @1,
                         keyAudioTrackName: [NSString stringWithFormat: @"%d: %s", 1, "English"],
                         keyAudioInputBitrate: @104,
                         keyAudioInputSampleRate: @48000,
                         keyAudioInputCodec: @65536,
                         keyAudioInputCodecParam: @86018,
                         keyAudioInputChannelLayout: @3,
                         keyAudioTrackLanguageIsoCode: @"eng"}];
    return [tracks copy];
}

- (NSArray *)subtitlesTracks
{
    /*if (!_subtitlesTracks)
    {
        NSMutableArray *tracks = [NSMutableArray array];
        hb_subtitle_t *subtitle;
        hb_list_t *list = self.hb_title->list_subtitle;
        int count = hb_list_count(list);

        for (int i = 0; i < count; i++)
        {
            subtitle = (hb_subtitle_t *) hb_list_item(self.hb_title->list_subtitle, i);

            // Human-readable representation of subtitle->source
            NSString *bitmapOrText  = subtitle->format == PICTURESUB ? @"Bitmap" : @"Text";
            NSString *subSourceName = @(hb_subsource_name(subtitle->source));

            // Use the native language name if available
            iso639_lang_t *language = lang_for_code2(subtitle->iso639_2);
            NSString *nativeLanguage = strlen(language->native_name) ? @(language->native_name) : @(language->eng_name);

            // create a dictionary of source subtitle information to store in our array
            [tracks addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", i, nativeLanguage, bitmapOrText, subSourceName],
                                keySubTrackIndex: @(i),
                                keySubTrackType: @(subtitle->source),
                                keySubTrackLanguage: nativeLanguage,
                                keySubTrackLanguageIsoCode: @(subtitle->iso639_2)}];
        }

        _subtitlesTracks = [tracks copy];
    }*/

    return nil;
}

- (NSArray *)chapters
{
    return @[@"Chapter 1", @"Chapter 2"];
}


@end
