/*  HBMockTitle

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBMockTitle.h"
#import "HBChapter.h"

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
                         keyAudioTrackName: [NSString stringWithFormat: @"%d: %s", 0, "English"],
                         keyAudioInputBitrate: @104,
                         keyAudioInputSampleRate: @48000,
                         keyAudioInputCodec: @65536,
                         keyAudioInputCodecParam: @86018,
                         keyAudioInputChannelLayout: @3,
                         keyAudioTrackLanguageIsoCode: @"eng"}];

    [tracks addObject: @{keyAudioTrackIndex: @2,
                         keyAudioTrackName: [NSString stringWithFormat: @"%d: %s", 1, "Italian"],
                         keyAudioInputBitrate: @104,
                         keyAudioInputSampleRate: @48000,
                         keyAudioInputCodec: @65536,
                         keyAudioInputCodecParam: @86018,
                         keyAudioInputChannelLayout: @3,
                         keyAudioTrackLanguageIsoCode: @"ita"}];
    return [tracks copy];
}

- (NSArray *)subtitlesTracks
{
    NSMutableArray *tracks = [NSMutableArray array];
    NSString *nativeLanguage = @"English";

    // create a dictionary of source subtitle information to store in our array
    [tracks addObject:@{keySubTrackName: [NSString stringWithFormat:@"%d: %@ (%@) (%@)", 0, nativeLanguage, @"Bitmap", @"VobSub"],
                        keySubTrackType: @0,
                        keySubTrackLanguageIsoCode: @"eng"}];

    return [tracks copy];
}

- (NSArray<HBChapter *> *)chapters
{
    NSMutableArray *chapters = [NSMutableArray array];

    for (int i = 0; i < 10; i++)
    {
        NSString *title = [NSString stringWithFormat:@"Chapter %d", i + 1];
        [chapters addObject:[[HBChapter alloc] initWithTitle:title
                                                        index:i + 1
                                                    duration:100]];
    }
    return [chapters copy];
}


@end
