/*  HBJob.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob.h"
#import "HBPreset.h"

#import "HBAudioDefaults.h"
#import "HBSubtitlesDefaults.h"

#import "HBCodingUtilities.h"

#include "hb.h"

NSString *HBContainerChangedNotification = @"HBContainerChangedNotification";
NSString *HBChaptersChangedNotification  = @"HBChaptersChangedNotification";

@interface HBJob ()
@property (nonatomic, readonly) NSString *name;
@end

@implementation HBJob

@synthesize uuid = _uuid;

- (instancetype)initWithTitle:(HBTitle *)title andPreset:(HBPreset *)preset
{
    self = [super init];
    if (self) {
        NSParameterAssert(title);
        NSParameterAssert(preset);

        _title = title;
        _titleIdx = title.index;

        _name = [title.name copy];
        _fileURL = title.url;

        _container = HB_MUX_MP4;
        _angle = 1;

        _range = [[HBRange alloc] initWithTitle:title];
        _video = [[HBVideo alloc] initWithJob:self];
        _picture = [[HBPicture alloc] initWithTitle:title];
        _filters = [[HBFilters alloc] init];

        _audio = [[HBAudio alloc] initWithTitle:title];
        _subtitles = [[HBSubtitles alloc] initWithTitle:title];

        _chapterTitles = [title.chapters mutableCopy];

        _uuid = [[NSUUID UUID] UUIDString];

        [self applyPreset:preset];
    }

    return self;
}

- (void)applyPreset:(HBPreset *)preset
{
    if (preset.isDefault)
    {
        self.presetName = [NSString stringWithFormat:@"%@ (Default)", preset.name];
    }
    else
    {
        self.presetName = preset.name;
    }

    NSDictionary *content = preset.content;

    self.container = hb_container_get_from_name(hb_container_sanitize_name([content[@"FileFormat"] UTF8String]));

    // MP4 specifics options.
    self.mp4HttpOptimize = [content[@"Mp4HttpOptimize"] boolValue];
    self.mp4iPodCompatible = [content[@"Mp4iPodCompatible"] boolValue];

    // Chapter Markers
    self.chaptersEnabled = [content[@"ChapterMarkers"] boolValue];

    [@[self.audio, self.subtitles, self.filters, self.picture, self.video] makeObjectsPerformSelector:@selector(applyPreset:)
                                                                                                           withObject:content];
}

- (void)applyCurrentSettingsToPreset:(NSMutableDictionary *)dict
{
    dict[@"FileFormat"] = @(hb_container_get_name(self.container));
    dict[@"ChapterMarkers"] = @(self.chaptersEnabled);
    // MP4 specifics options.
    dict[@"Mp4HttpOptimize"] = @(self.mp4HttpOptimize);
    dict[@"Mp4iPodCompatible"] = @(self.mp4iPodCompatible);

    [@[self.video, self.filters, self.picture, self.audio, self.subtitles] makeObjectsPerformSelector:@selector(writeToPreset:)
                                                                                                           withObject:dict];
}

- (void)setContainer:(int)container
{
    _container = container;

    [self.audio containerChanged:container];
    [self.subtitles containerChanged:container];
    [self.video containerChanged];

    // post a notification for any interested observers to indicate that our video container has changed
    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (void)setTitle:(HBTitle *)title
{
    _title = title;
    self.range.title = title;
}

- (void)setChaptersEnabled:(BOOL)chaptersEnabled
{
    _chaptersEnabled = chaptersEnabled;
    [[NSNotificationCenter defaultCenter] postNotificationName:HBChaptersChangedNotification object:self];
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"mp4OptionsEnabled"])
    {
        retval = [NSSet setWithObjects:@"container", nil];
    }

    if ([key isEqualToString:@"mp4iPodCompatibleEnabled"])
    {
        retval = [NSSet setWithObjects:@"container", @"video.encoder", nil];
    }

    return retval;
}

- (NSString *)description
{
    return self.name;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBJob *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_state = HBJobStateReady;
        copy->_name = [_name copy];
        copy->_presetName = [_presetName copy];
        copy->_titleIdx = _titleIdx;
        copy->_uuid = [[NSUUID UUID] UUIDString];

        copy->_fileURL = [_fileURL copy];
        copy->_destURL = [_destURL copy];

        copy->_container = _container;
        copy->_angle = _angle;
        copy->_mp4HttpOptimize = _mp4HttpOptimize;
        copy->_mp4iPodCompatible = _mp4iPodCompatible;

        copy->_range = [_range copy];
        copy->_video = [_video copy];
        copy->_picture = [_picture copy];
        copy->_filters = [_filters copy];

        copy->_video.job = copy;

        copy->_audio = [_audio copy];
        copy->_subtitles = [_subtitles copy];

        copy->_chaptersEnabled = _chaptersEnabled;
        copy->_chapterTitles = [[NSMutableArray alloc] initWithArray:_chapterTitles copyItems:YES];
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBVideoVersion"];

    encodeInt(_state);
    encodeObject(_name);
    encodeObject(_presetName);
    encodeInt(_titleIdx);
    encodeObject(_uuid);

    encodeObject(_fileURL);
    encodeObject(_destURL);

    encodeInt(_container);
    encodeInt(_angle);
    encodeBool(_mp4HttpOptimize);
    encodeBool(_mp4iPodCompatible);

    encodeObject(_range);
    encodeObject(_video);
    encodeObject(_picture);
    encodeObject(_filters);

    encodeObject(_audio);
    encodeObject(_subtitles);

    encodeBool(_chaptersEnabled);
    encodeObject(_chapterTitles);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:@"HBVideoVersion"];

    if (version == 1 && (self = [super init]))
    {
        decodeInt(_state);
        decodeObject(_name, NSString);
        decodeObject(_presetName, NSString);
        decodeInt(_titleIdx);
        decodeObject(_uuid, NSString);

        decodeObject(_fileURL, NSURL);
        decodeObject(_destURL, NSURL);

        decodeInt(_container);
        decodeInt(_angle);
        decodeBool(_mp4HttpOptimize);
        decodeBool(_mp4iPodCompatible);

        decodeObject(_range, HBRange);
        decodeObject(_video, HBVideo);
        decodeObject(_picture, HBPicture);
        decodeObject(_filters, HBFilters);

        _video.job = self;

        decodeObject(_audio, HBAudio);
        decodeObject(_subtitles, HBSubtitles);

        decodeBool(_chaptersEnabled);
        decodeObject(_chapterTitles, NSMutableArray);

        return self;
    }

    return nil;
}

@end
