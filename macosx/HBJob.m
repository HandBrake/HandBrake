/*  HBJob.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob.h"
#import "HBJob+Private.h"
#import "HBTitle+Private.h"

#import "HBAudioDefaults.h"
#import "HBSubtitlesDefaults.h"
#import "HBMutablePreset.h"

#import "HBCodingUtilities.h"
#import "HBLocalizationUtilities.h"
#import "HBUtilities.h"
#import "HBSecurityAccessToken.h"

#include "handbrake/handbrake.h"

NSString *HBContainerChangedNotification = @"HBContainerChangedNotification";
NSString *HBChaptersChangedNotification  = @"HBChaptersChangedNotification";

@interface HBJob ()

@property (nonatomic, readonly) NSString *name;

/**
 Store the security scoped bookmarks, so we don't
 regenerate it each time
 */
@property (nonatomic, readonly) NSData *fileURLBookmark;
@property (nonatomic, readwrite) NSData *destinationFolderURLBookmark;

/**
 Keep track of security scoped resources status.
 */
@property (nonatomic, readwrite) HBSecurityAccessToken *fileURLToken;
@property (nonatomic, readwrite) HBSecurityAccessToken *destinationFolderURLToken;
@property (nonatomic, readwrite) HBSecurityAccessToken *subtitlesToken;
@property (nonatomic, readwrite) NSInteger accessCount;

@end

@implementation HBJob

- (nullable instancetype)initWithTitle:(HBTitle *)title preset:(HBPreset *)preset subtitles:(NSArray<NSURL *> *)subtitlesURLs
{
    self = [super init];
    if (self) {
        NSParameterAssert(title);
        NSParameterAssert(preset);

        _title = title;
        _titleIdx = title.index;
        _keepDuplicateTitles = title.keepDuplicateTitles;
        _stream = title.isStream;

        _name = [title.name copy];
        _fileURL = title.url;

        _container = HB_MUX_MP4;
        _angle = 1;

        _range = [[HBRange alloc] initWithTitle:title];
        _video = [[HBVideo alloc] initWithJob:self];
        _picture = [[HBPicture alloc] initWithTitle:title];
        _filters = [[HBFilters alloc] init];

        _audio = [[HBAudio alloc] initWithJob:self];
        _subtitles = [[HBSubtitles alloc] initWithJob:self];

        _chapterTitles = [title.chapters copy];
        _metadataPassthru = YES;
        _presetName = @"";

        for (NSURL *url in subtitlesURLs)
        {
            [self.subtitles addExternalSourceTrackFromURL:url addImmediately:NO];
        }

        if ([self applyPreset:preset error:NULL] == NO)
        {
            return nil;
        }
    }

    return self;
}

- (nullable instancetype)initWithTitle:(HBTitle *)title preset:(HBPreset *)preset
{
    self = [self initWithTitle:title preset:preset subtitles:@[]];
    return self;
}

#pragma mark - HBPresetCoding

- (BOOL)applyPreset:(HBPreset *)preset error:(NSError * __autoreleasing *)outError
{
    NSAssert(self.title, @"HBJob: calling applyPreset: without a valid title loaded");

    NSDictionary *jobSettings = [self.title jobSettingsWithPreset:preset];

    if (jobSettings)
    {
        self.presetName = preset.name;

        self.container = hb_container_get_from_name([preset[@"FileFormat"] UTF8String]);

        // MP4 specifics options.
        self.optimize = [preset[@"Optimize"] boolValue];
        self.mp4iPodCompatible = [preset[@"Mp4iPodCompatible"] boolValue];

        self.alignAVStart = [preset[@"AlignAVStart"] boolValue];

        self.chaptersEnabled = [preset[@"ChapterMarkers"] boolValue];
        self.metadataPassthru = [preset[@"MetadataPassthru"] boolValue];

        [self.audio applyPreset:preset jobSettings:jobSettings];
        [self.subtitles applyPreset:preset jobSettings:jobSettings];
        [self.video applyPreset:preset jobSettings:jobSettings];
        [self.picture applyPreset:preset jobSettings:jobSettings];
        [self.filters applyPreset:preset jobSettings:jobSettings];

        return YES;
    }
    else
    {
        if (outError != NULL)
        {
            *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Invalid preset", @"HBJob -> invalid preset"),
                                                                          NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(@"The preset is not a valid, try to select a different one.", @"Job preset -> invalid preset recovery suggestion")}];
        }

        return NO;
    }
}

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset.name = self.presetName;

    preset[@"FileFormat"] = @(hb_container_get_short_name(self.container));

    // MP4 specifics options.
    preset[@"Optimize"] = @(self.optimize);
    preset[@"AlignAVStart"] = @(self.alignAVStart);
    preset[@"Mp4iPodCompatible"] = @(self.mp4iPodCompatible);

    preset[@"ChapterMarkers"] = @(self.chaptersEnabled);
    preset[@"MetadataPassthru"] = @(self.metadataPassthru);

    [@[self.video, self.filters, self.picture, self.audio, self.subtitles] makeObjectsPerformSelector:@selector(writeToPreset:)
                                                                                                           withObject:preset];
}

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    [@[self.video, self.range, self.filters, self.picture, self.audio, self.subtitles] makeObjectsPerformSelector:@selector(setUndo:)
                                                                                                       withObject:_undo];
    [self.chapterTitles makeObjectsPerformSelector:@selector(setUndo:) withObject:_undo];
}

- (void)setPresetName:(NSString *)presetName
{
    if (![presetName isEqualToString:_presetName])
    {
        [[self.undo prepareWithInvocationTarget:self] setPresetName:_presetName];
    }
    _presetName = [presetName copy];
}

- (void)setDestinationFolderURL:(NSURL *)destinationFolderURL
{
    if (![destinationFolderURL isEqualTo:_destinationFolderURL])
    {
        [[self.undo prepareWithInvocationTarget:self] setDestinationFolderURL:_destinationFolderURL];
    }
    _destinationFolderURL = [destinationFolderURL copy];

#ifdef __SANDBOX_ENABLED__
    // Clear the bookmark to regenerate it later
    self.destinationFolderURLBookmark = nil;
#endif
}

- (void)setDestinationFileName:(NSString *)destinationFileName
{
    if (![destinationFileName isEqualTo:_destinationFileName])
    {
        [[self.undo prepareWithInvocationTarget:self] setDestinationFileName:_destinationFileName];
    }
    _destinationFileName = [destinationFileName copy];
}

- (BOOL)validateDestinationFileName:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        NSString *value = *ioValue;

        if ([value rangeOfString:@"/"].location != NSNotFound)
        {
            if (outError)
            {
                *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid name", @"HBJob -> invalid name error description"),
                                                                                  NSLocalizedRecoverySuggestionErrorKey: HBKitLocalizedString(@"The file name can't contain the / character.", @"HBJob -> invalid name error recovery suggestion")}];
            }
            return NO;
        }
        if (value.length == 0)
        {
            if (outError)
            {
                *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid name", @"HBJob -> invalid name error description"),
                                                                                  NSLocalizedRecoverySuggestionErrorKey: HBKitLocalizedString(@"The file name can't be empty.", @"HBJob -> invalid name error recovery suggestion")}];
            }
            return NO;
        }
    }

    if (*ioValue == nil)
    {
        if (outError)
        {
            *outError = [NSError errorWithDomain:@"HBError" code:0 userInfo:@{NSLocalizedDescriptionKey: HBKitLocalizedString(@"Invalid name", @"HBJob -> invalid name error description"),
                                                                              NSLocalizedRecoverySuggestionErrorKey: HBKitLocalizedString(@"The file name can't be empty.", @"HBJob -> invalid name error recovery suggestion")}];
        }
        return NO;
    }

    return retval;
}

- (NSURL *)destinationURL
{
    return [self.destinationFolderURL URLByAppendingPathComponent:self.destinationFileName isDirectory:NO];
}

- (void)setContainer:(int)container
{
    if (container != _container)
    {
        [[self.undo prepareWithInvocationTarget:self] setContainer:_container];
    }

    _container = container;

    [self.audio setContainer:container];
    [self.subtitles setContainer:container];
    [self.video containerChanged];

    // post a notification for any interested observers to indicate that our video container has changed
    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (void)setAngle:(int)angle
{
    if (angle != _angle)
    {
        [[self.undo prepareWithInvocationTarget:self] setAngle:_angle];
    }
    _angle = angle;
}

- (void)setTitle:(HBTitle *)title
{
    _title = title;
    self.range.title = title;
    _keepDuplicateTitles = title.keepDuplicateTitles;
}

- (void)setOptimize:(BOOL)optimize
{
    if (optimize != _optimize)
    {
        [[self.undo prepareWithInvocationTarget:self] setOptimize:_optimize];
    }
    _optimize = optimize;

    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (void)setAlignAVStart:(BOOL)alignAVStart
{
    if (alignAVStart != _alignAVStart)
    {
        [[self.undo prepareWithInvocationTarget:self] setAlignAVStart:_alignAVStart];
    }
    _alignAVStart = alignAVStart;

    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (void)setMp4iPodCompatible:(BOOL)mp4iPodCompatible
{
    if (mp4iPodCompatible != _mp4iPodCompatible)
    {
        [[self.undo prepareWithInvocationTarget:self] setMp4iPodCompatible:_mp4iPodCompatible];
    }
    _mp4iPodCompatible = mp4iPodCompatible;

    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (void)setChaptersEnabled:(BOOL)chaptersEnabled
{
    if (chaptersEnabled != _chaptersEnabled)
    {
        [[self.undo prepareWithInvocationTarget:self] setChaptersEnabled:_chaptersEnabled];
    }
    _chaptersEnabled = chaptersEnabled;
    [[NSNotificationCenter defaultCenter] postNotificationName:HBChaptersChangedNotification object:self];
}

- (void)setMetadataPassthru:(BOOL)metadataPassthru
{
    if (metadataPassthru != _metadataPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setMetadataPassthru:_metadataPassthru];
    }
    _metadataPassthru = metadataPassthru;
    [[NSNotificationCenter defaultCenter] postNotificationName:HBContainerChangedNotification object:self];
}

- (NSString *)description
{
    return self.name;
}

- (void)refreshSecurityScopedResources
{
    if (_fileURLBookmark)
    {
        NSURL *resolvedURL = [HBUtilities URLFromBookmark:_fileURLBookmark];
        if (resolvedURL)
        {
            _fileURL = resolvedURL;
        }
    }
    if (_destinationFolderURLBookmark)
    {
        NSURL *resolvedURL = [HBUtilities URLFromBookmark:_destinationFolderURLBookmark];
        if (resolvedURL)
        {
            _destinationFolderURL = resolvedURL;
        }
    }
    [self.subtitles refreshSecurityScopedResources];
}

- (BOOL)startAccessingSecurityScopedResource
{
#ifdef __SANDBOX_ENABLED__
    if (self.accessCount == 0)
    {
        self.fileURLToken = [HBSecurityAccessToken tokenWithObject:self.fileURL];
        self.destinationFolderURLToken = [HBSecurityAccessToken tokenWithObject:self.destinationFolderURL];
        self.subtitlesToken = [HBSecurityAccessToken tokenWithObject:self.subtitles];
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
    NSAssert(self.accessCount >= 0, @"[HBJob stopAccessingSecurityScopedResource:] unbalanced call");
    if (self.accessCount == 0)
    {
        self.fileURLToken = nil;
        self.destinationFolderURLToken = nil;
        self.subtitlesToken = nil;
    }
#endif
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBJob *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_name = [_name copy];
        copy->_presetName = [_presetName copy];
        copy->_titleIdx = _titleIdx;
        copy->_stream = _stream;

        copy->_fileURLBookmark = [_fileURLBookmark copy];
        copy->_destinationFolderURLBookmark = [_destinationFolderURLBookmark copy];

        copy->_fileURL = [_fileURL copy];
        copy->_destinationFolderURL = [_destinationFolderURL copy];
        copy->_destinationFileName = [_destinationFileName copy];

        copy->_container = _container;
        copy->_angle = _angle;
        copy->_optimize = _optimize;
        copy->_mp4iPodCompatible = _mp4iPodCompatible;
        copy->_alignAVStart = _alignAVStart;

        copy->_range = [_range copy];
        copy->_video = [_video copy];
        copy->_picture = [_picture copy];
        copy->_filters = [_filters copy];

        copy->_video.job = copy;

        copy->_audio = [_audio copy];
        copy->_audio.job = copy;
        copy->_subtitles = [_subtitles copy];
        copy->_subtitles.job = copy;

        copy->_chaptersEnabled = _chaptersEnabled;
        copy->_chapterTitles = [[NSArray alloc] initWithArray:_chapterTitles copyItems:YES];

        copy->_metadataPassthru = _metadataPassthru;
        copy->_hwDecodeUsage = _hwDecodeUsage;
        copy->_keepDuplicateTitles = _keepDuplicateTitles;
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
    [coder encodeInt:6 forKey:@"HBJobVersion"];

    encodeObject(_name);
    encodeObject(_presetName);
    encodeInt(_titleIdx);
    encodeBool(_stream);

#ifdef __SANDBOX_ENABLED__
    if (!_fileURLBookmark)
    {
        _fileURLBookmark = [HBUtilities bookmarkFromURL:_fileURL
                                                options:NSURLBookmarkCreationWithSecurityScope |
                                                        NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess];
    }

    encodeObject(_fileURLBookmark);

    if (!_destinationFolderURLBookmark)
    {
        __attribute__((unused)) HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithObject:_destinationFolderURL];
        _destinationFolderURLBookmark = [HBUtilities bookmarkFromURL:_destinationFolderURL];
        token = nil;
    }

    encodeObject(_destinationFolderURLBookmark);

#endif

    encodeObject(_fileURL);
    encodeObject(_destinationFolderURL);
    encodeObject(_destinationFileName);

    encodeInt(_container);
    encodeInt(_angle);
    encodeBool(_optimize);
    encodeBool(_mp4iPodCompatible);
    encodeBool(_alignAVStart);

    encodeObject(_range);
    encodeObject(_video);
    encodeObject(_picture);
    encodeObject(_filters);

    encodeObject(_audio);
    encodeObject(_subtitles);

    encodeBool(_chaptersEnabled);
    encodeObject(_chapterTitles);

    encodeBool(_metadataPassthru);
    encodeInteger(_hwDecodeUsage);
    encodeBool(_keepDuplicateTitles);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:@"HBJobVersion"];

    if (version == 6 && (self = [super init]))
    {
        decodeObjectOrFail(_name, NSString);
        decodeObjectOrFail(_presetName, NSString);
        decodeInt(_titleIdx); if (_titleIdx < 0) { goto fail; }
        decodeBool(_stream);

#ifdef __SANDBOX_ENABLED__
        decodeObject(_fileURLBookmark, NSData)
        decodeObject(_destinationFolderURLBookmark, NSData)
#endif
        decodeObjectOrFail(_fileURL, NSURL);
        decodeObject(_destinationFolderURL, NSURL);
        decodeObject(_destinationFileName, NSString);

        decodeInt(_container); if (_container != HB_MUX_MP4 && _container != HB_MUX_MKV && _container != HB_MUX_WEBM) { goto fail; }
        decodeInt(_angle); if (_angle < 0) { goto fail; }
        decodeBool(_optimize);
        decodeBool(_mp4iPodCompatible);
        decodeBool(_alignAVStart);

        decodeObjectOrFail(_range, HBRange);
        decodeObjectOrFail(_video, HBVideo);
        decodeObjectOrFail(_picture, HBPicture);
        decodeObjectOrFail(_filters, HBFilters);

        _video.job = self;

        decodeObjectOrFail(_audio, HBAudio);
        decodeObjectOrFail(_subtitles, HBSubtitles);

        _audio.job = self;
        _video.job = self;

        decodeBool(_chaptersEnabled);
        decodeCollectionOfObjectsOrFail(_chapterTitles, NSArray, HBChapter);

        decodeBool(_metadataPassthru);
        decodeInteger(_hwDecodeUsage); if (_hwDecodeUsage != HBJobHardwareDecoderUsageNone && _hwDecodeUsage != HBJobHardwareDecoderUsageAlways && _hwDecodeUsage != HBJobHardwareDecoderUsageFullPathOnly) { goto fail; }
        decodeBool(_keepDuplicateTitles);

        return self;
    }

fail:
    return nil;
}

@end
