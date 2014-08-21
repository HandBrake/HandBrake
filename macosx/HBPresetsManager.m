/*  HBPresets.m $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsManager.h"
#import "HBPreset.h"

#import "HBUtilities.h"

NSString *HBPresetsChangedNotification = @"HBPresetsChangedNotification";

@interface HBPresetsManager () <HBTreeNodeDelegate>

@property (nonatomic, readonly, copy) NSURL *fileURL;

/* Dictionaries for individual presets ("Devices" folder) */
- (NSDictionary *)createUniversalPreset;
- (NSDictionary *)createiPodPreset;
- (NSDictionary *)createiPhoneiPodtouchPreset;
- (NSDictionary *)createiPadPreset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createAppleTV2Preset;
- (NSDictionary *)createAppleTV3Preset;
- (NSDictionary *)createAndroidPreset;
- (NSDictionary *)createAndroidTabletPreset;
- (NSDictionary *)createW8PhonePreset;

/* Dictionaries for individual presets ("Regular" folder) */
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createHighProfilePreset;

@end

@implementation HBPresetsManager

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Init the root of the tree, it won't never be shown in the UI
        _root = [[HBPreset alloc] initWithFolderName:@"Root" builtIn:YES];
        _root.delegate = self;
    }
    return self;
}

- (instancetype)initWithURL:(NSURL *)url
{
    self = [self init];
    if (self)
    {
        _fileURL = [url copy];
        [self loadPresetsFromURL:url];
    }
    return self;
}

- (void)dealloc
{
    [_fileURL release];
    [_defaultPreset release];

    [_root release];

    [super dealloc];
}

- (NSIndexPath *)indexPathOfPreset:(HBPreset *)preset
{
    __block NSIndexPath *retValue = nil;

    // Visit the whole tree to find the index path.
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
    {
        if ([obj isEqualTo:preset])
        {
            retValue = [idx retain];
            *stop = YES;
        }
    }];

    return [retValue autorelease];
}

#pragma mark - HBTreeNode delegate

- (void)nodeDidChange
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBPresetsChangedNotification object:nil];
}

#pragma mark - Load/Save

- (BOOL)loadPresetsFromURL:(NSURL *)url
{
    NSArray *presetsArray = [[NSArray alloc] initWithContentsOfURL:url];

    for (NSDictionary *child in presetsArray)
    {
        [self.root.children addObject:[self loadFromDict:child]];
    }

    [presetsArray release];

    // If the preset list is empty,
    // readd the built in presets.
    if (self.root.children.count == 0)
    {
        [self generateBuiltInPresets];
    }

    if (self.defaultPreset == nil)
    {
        [self selectNewDefault];
    }

    return YES;
}

- (BOOL)savePresetsToURL:(NSURL *)url
{
    NSMutableArray *presetsArray = [[[NSMutableArray alloc] init] autorelease];

    for (HBPreset *node in self.root.children)
    {
        [presetsArray addObject:[self convertToDict:node]];
    }

    return [presetsArray writeToURL:url atomically:YES];
    
    return YES;
}

- (BOOL)savePresets
{
    return [self savePresetsToURL:self.fileURL];
}

#pragma mark - NSDictionary conversions

/**
 *  Converts the NSDictionary to a HBPreset object,
 */
- (HBPreset *)loadFromDict:(NSDictionary *)dict
{
    HBPreset *node = nil;
    if ([dict[@"Folder"] boolValue])
    {
        node = [[[HBPreset alloc] initWithFolderName:dict[@"PresetName"]
                                              builtIn:![dict[@"Type"] boolValue]] autorelease];

        for (NSDictionary *child in dict[@"ChildrenArray"])
        {
            [node.children addObject:[self loadFromDict:child]];
        }
    }
    else
    {
        node = [[[HBPreset alloc] initWithName:dict[@"PresetName"]
                                        content:dict
                                        builtIn:![dict[@"Type"] boolValue]] autorelease];
        node.isDefault = [dict[@"Default"] boolValue];

        if ([dict[@"Default"] boolValue])
        {
            self.defaultPreset = node;
        }
    }

    if (!node.isBuiltIn)
    {
        node.delegate = self;
    }

    return node;
}

/**
 *  Converts the HBPreset and its childrens to a NSDictionary.
 */
- (NSDictionary *)convertToDict:(HBPreset *)node
{
    NSMutableDictionary *output = [[NSMutableDictionary alloc] init];
    [output addEntriesFromDictionary:node.content];

    output[@"PresetName"] = node.name;
    output[@"Folder"] = @(!node.isLeaf);
    output[@"Type"] = @(!node.isBuiltIn);
    output[@"Default"] = @(node.isDefault);

    if (!node.isLeaf)
    {
        NSMutableArray *childArray = [[NSMutableArray alloc] init];
        for (HBPreset *child in node.children)
        {
            [childArray addObject:[self convertToDict:child]];
        }

        output[@"ChildrenArray"] = childArray;
        [childArray release];
    }

    return [output autorelease];
}

#pragma mark - Presets Management

- (BOOL)checkBuiltInsForUpdates
{
    __block BOOL retValue = NO;

    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        NSDictionary *dict = [obj content];

        if ([obj isBuiltIn] && [obj isLeaf])
        {
            if (!dict[@"PresetBuildNumber"] ||
                [dict[@"PresetBuildNumber"] intValue] < [[[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"] intValue])
            {
                retValue = YES;
                *stop = YES;
            }
        }
    }];

    return retValue;
}

- (void)addPreset:(NSDictionary *)preset
{
    HBPreset *presetNode = [[HBPreset alloc] initWithName:preset[@"PresetName"]
                                                   content:preset
                                                   builtIn:NO];

    [self.root insertObject:presetNode inChildrenAtIndex:[self.root countOfChildren]];
    [presetNode release];

    [self savePresets];
}

- (void)deletePresetAtIndexPath:(NSIndexPath *)idx
{
    HBPreset *parentNode = self.root;

    // Find the preset parent array
    // and delete it.
    NSUInteger currIdx = 0;
    NSUInteger i = 0;
    for (i = 0; i < idx.length - 1; i++)
    {
        currIdx = [idx indexAtPosition:i];

        if (parentNode.children.count > currIdx)
        {
            parentNode = [parentNode.children objectAtIndex:currIdx];
        }
    }

    currIdx = [idx indexAtPosition:i];

    if (parentNode.children.count > currIdx)
    {
        if ([[parentNode.children objectAtIndex:currIdx] isDefault])
        {
            [parentNode removeObjectFromChildrenAtIndex:currIdx];
            // Try to select a new default preset
            [self selectNewDefault];
        }
        else
        {
            [parentNode removeObjectFromChildrenAtIndex:currIdx];
        }
    }
}

/**
 *  Private method to select a new default
 *  when the default preset is deleted.
 */
- (void)selectNewDefault
{
    __block HBPreset *normalPreset = nil;
    __block HBPreset *firstUserPreset = nil;

    // Search for a possibile new default preset
    // Try to use "Normal" or the first user preset.
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        if ([obj isBuiltIn] && [obj isLeaf])
        {
            if ([[obj name] isEqualToString:@"Normal"])
            {
                normalPreset = obj;
            }
        }
        else if ([obj isLeaf])
        {
            firstUserPreset = obj;
            *stop = YES;
        }
    }];

    if (normalPreset)
    {
        self.defaultPreset = normalPreset;
        normalPreset.isDefault = YES;
    }
    else if (firstUserPreset)
    {
        self.defaultPreset = firstUserPreset;
        firstUserPreset.isDefault = YES;
    }
}

- (void)setDefaultPreset:(HBPreset *)defaultPreset
{
    if (defaultPreset && defaultPreset.isLeaf)
    {
        if (_defaultPreset)
        {
            _defaultPreset.isDefault = NO;
            [_defaultPreset autorelease];
        }
        defaultPreset.isDefault = YES;
        _defaultPreset = [defaultPreset retain];

        [self nodeDidChange];
    }
}

#pragma mark - Built In Generation

- (void)loadPresetsForType:(NSString *)type fromSel:(SEL[])selArray length:(int)len
{
    HBPreset *folderNode = [[HBPreset alloc] initWithFolderName:type builtIn:YES];

    for (int i = 0; i < len; i++)
    {
        NSMutableDictionary *presetDict = [self performSelector:selArray[i]];
        presetDict[@"PresetBuildNumber"] = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
        HBPreset *presetNode = [[HBPreset alloc] initWithName:presetDict[@"PresetName"]
                                                       content:presetDict
                                                       builtIn:YES];
        [folderNode.children addObject:presetNode];
        [presetNode release];
    }

    [self.root insertObject:folderNode inChildrenAtIndex:0];
    [folderNode release];
}

/**
 * Built-in preset folders at the root of the hierarchy
 *
 * Re-create new built-in presets programmatically and add them to the array
 *
 * Note: the built-in presets will *not* sort themselves alphabetically,
 * so they will appear in the order you create them.
 */
- (void)generateBuiltInPresets
{
    SEL devicesPresets[] = { @selector(createUniversalPreset),
        @selector(createiPodPreset),
        @selector(createiPhoneiPodtouchPreset),
        @selector(createiPadPreset),
        @selector(createAppleTVPreset),
        @selector(createAppleTV2Preset),
        @selector(createAppleTV3Preset),
        @selector(createAndroidPreset),
        @selector(createAndroidTabletPreset),
        @selector(createW8PhonePreset)
    };
    
    SEL regularPresets[] = { @selector(createNormalPreset),
        @selector(createHighProfilePreset)};
    
    [self deleteBuiltInPresets];

    [self loadPresetsForType:@"Regular" fromSel:regularPresets length:2];
    [self loadPresetsForType:@"Devices" fromSel:devicesPresets length:10];

    if (self.defaultPreset == nil)
    {
        [self selectNewDefault];
    }


    [HBUtilities writeToActivityLog: "built in presets updated to build number: %d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]];
}

- (void)deleteBuiltInPresets
{
    [self willChangeValueForKey:@"root"];
    NSMutableArray *nodeToRemove = [[NSMutableArray alloc] init];
    for (HBPreset *node in self.root.children)
    {
        if (node.isBuiltIn)
        {
            [nodeToRemove addObject:node];
        }
    }
    [self.root.children removeObjectsInArray:nodeToRemove];
    [nodeToRemove release];
    [self didChangeValueForKey:@"root"];
}

#pragma mark -
#pragma mark Individual Preset Definitions
/* These NSDictionary definitions contain settings for one built-in preset */

- (NSMutableDictionary *)createUniversalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"Universal";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for compatibility with all Apple devices (including the iPod 6G and later). Includes Dolby Digital audio for surround sound.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"fast";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"baseline";
    preset[@"VideoLevel"] = @"3.0";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @720;
    preset[@"PictureHeight"] = @576;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    audioTrack2Array[@"AudioTrack"] = @1;
    audioTrack2Array[@"AudioEncoder"] = @"AC3 Passthru";
    audioTrack2Array[@"AudioMixdown"] = @"None";
    audioTrack2Array[@"AudioSamplerate"] = @"Auto";
    audioTrack2Array[@"AudioBitrate"] = @"160";
    audioTrack2Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack2Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createiPodPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"iPod";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for playback on the iPod with Video (all generations).";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @1;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"baseline";
    preset[@"VideoLevel"] = @"1.3";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @22.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @320;
    preset[@"PictureHeight"] = @240;
    preset[@"PicturePAR"] = @0; //none
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @1;
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createiPhoneiPodtouchPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"iPhone & iPod touch";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for handheld iOS devices (iPhone 4, iPod touch 3G and later).";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @22.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @960;
    preset[@"PictureHeight"] = @640;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createiPadPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"iPad";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for playback on the iPad (all generations).";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @1280;
    preset[@"PictureHeight"] = @720;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createAppleTVPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"AppleTV";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for the original AppleTV. Includes Dolby Digital audio for surround sound. Also compatible with iOS devices released since 2009.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"qpmin=4:cabac=0:ref=2:b-pyramid=none:weightb=0:weightp=0:vbv-maxrate=9500:vbv-bufsize=9500";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @960;
    preset[@"PictureHeight"] = @720;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    audioTrack2Array[@"AudioTrack"] = @1;
    audioTrack2Array[@"AudioEncoder"] = @"AC3 Passthru";
    audioTrack2Array[@"AudioMixdown"] = @"None";
    audioTrack2Array[@"AudioSamplerate"] = @"Auto";
    audioTrack2Array[@"AudioBitrate"] = @"160";
    audioTrack2Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack2Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createAppleTV2Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"AppleTV 2";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for the second-generation AppleTV. Includes Dolby Digital audio for surround sound. NOT compatible with the original AppleTV.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @1280;
    preset[@"PictureHeight"] = @720;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    audioTrack2Array[@"AudioTrack"] = @1;
    audioTrack2Array[@"AudioEncoder"] = @"AC3 Passthru";
    audioTrack2Array[@"AudioMixdown"] = @"None";
    audioTrack2Array[@"AudioSamplerate"] = @"Auto";
    audioTrack2Array[@"AudioBitrate"] = @"160";
    audioTrack2Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack2Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createAppleTV3Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"AppleTV 3";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for the third-generation AppleTV. Includes Dolby Digital audio for surround sound. NOT compatible with the original AppleTV. May stutter on the second-generation AppleTV.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"4.0";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @1920;
    preset[@"PictureHeight"] = @1080;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @3; //fast
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    audioTrack2Array[@"AudioTrack"] = @1;
    audioTrack2Array[@"AudioEncoder"] = @"AC3 Passthru";
    audioTrack2Array[@"AudioMixdown"] = @"None";
    audioTrack2Array[@"AudioSamplerate"] = @"Auto";
    audioTrack2Array[@"AudioBitrate"] = @"160";
    audioTrack2Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack2Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createAndroidPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"Android";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's settings for midrange devices running Android 2.3 or later.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @0;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"main";
    preset[@"VideoLevel"] = @"3.0";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @22.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @720;
    preset[@"PictureHeight"] = @576;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"128";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createAndroidTabletPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"Android Tablet";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's preset for tablets running Android 2.3 or later.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @0;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"main";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @22.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @1280;
    preset[@"PictureHeight"] = @720;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"128";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createW8PhonePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"Windows Phone 8";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's preset for Windows Phone 8 devices";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @0;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"main";
    preset[@"VideoLevel"] = @"3.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @22.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"30";
    preset[@"VideoFramerateMode"] = @"pfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @1280;
    preset[@"PictureHeight"] = @720;
    preset[@"PicturePAR"] = @0; //None
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @1;
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"128";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createNormalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"Normal";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @1; //default
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's normal, default settings.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @0;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"veryfast";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"main";
    preset[@"VideoLevel"] = @"4.0";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"Same as source";
    preset[@"VideoFramerateMode"] = @"vfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @0;
    preset[@"PictureHeight"] = @0;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @0; //off
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSMutableDictionary *)createHighProfilePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    preset[@"PresetName"] = @"High Profile";
    preset[@"Type"] = @0; //factory
    preset[@"Default"] = @0;
    preset[@"Folder"] = @NO;
    preset[@"PresetDescription"] = @"HandBrake's general-purpose preset for High Profile H.264 video.";
    
    /* Container format and related settings */
    preset[@"FileFormat"] = @"MP4 file";
    preset[@"Mp4LargeFile"] = @1;
    preset[@"Mp4HttpOptimize"] = @0;
    preset[@"Mp4iPodCompatible"] = @0;
    
    /* Chapter markers */
    preset[@"ChapterMarkers"] = @1;
    
    /* Video encoder and advanced options */
    preset[@"VideoEncoder"] = @"H.264 (x264)";
    preset[@"lavcOption"] = @"";
    preset[@"x264Option"] = @"";
    preset[@"x264UseAdvancedOptions"] = @0;
    preset[@"VideoPreset"] = @"medium";
    preset[@"VideoTune"] = @"";
    preset[@"VideoOptionExtra"] = @"";
    preset[@"VideoProfile"] = @"high";
    preset[@"VideoLevel"] = @"4.1";
    
    /* Video rate control */
    preset[@"VideoAvgBitrate"] = @"2500";
    preset[@"VideoTwoPass"] = @0;
    preset[@"VideoTurboTwoPass"] = @0;
    preset[@"VideoQualityType"] = @2; //cq
    preset[@"VideoQualitySlider"] = @20.0f;
    
    /* Video frame rate */
    preset[@"VideoFramerate"] = @"Same as source";
    preset[@"VideoFramerateMode"] = @"vfr";
    
    /* Picture size */
    preset[@"UsesPictureSettings"] = @1;
    preset[@"PictureWidth"] = @0;
    preset[@"PictureHeight"] = @0;
    preset[@"PicturePAR"] = @2; //loose
    preset[@"PictureModulus"] = @2;
    preset[@"PictureKeepRatio"] = @0; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    preset[@"UsesPictureFilters"] = @1;
    preset[@"PictureDecomb"] = @2; //default
    preset[@"PictureDecombCustom"] = @"";
    preset[@"PictureDecombDeinterlace"] = @1; //decomb
    preset[@"PictureDeinterlace"] = @0;
    preset[@"PictureDeinterlaceCustom"] = @"";
    preset[@"PictureDetelecine"] = @0;
    preset[@"PictureDetelecineCustom"] = @"";
    preset[@"PictureDenoise"] = @0;
    preset[@"PictureDenoiseCustom"] = @"";
    preset[@"PictureDeblock"] = @0;
    preset[@"VideoGrayScale"] = @0;
    
    /* Picture crop */
    preset[@"PictureAutoCrop"] = @1;
    preset[@"PictureTopCrop"] = @0;
    preset[@"PictureBottomCrop"] = @0;
    preset[@"PictureLeftCrop"] = @0;
    preset[@"PictureRightCrop"] = @0;
    
    /* Auto Passthru */
    preset[@"AudioEncoderFallback"] = @"AC3 (ffmpeg)";
    preset[@"AudioAllowAACPass"] = @1;
    preset[@"AudioAllowAC3Pass"] = @1;
    preset[@"AudioAllowDTSHDPass"] = @1;
    preset[@"AudioAllowDTSPass"] = @1;
    preset[@"AudioAllowMP3Pass"] = @1;
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    audioTrack1Array[@"AudioTrack"] = @1;
    audioTrack1Array[@"AudioEncoder"] = @"AAC (avcodec)";
    audioTrack1Array[@"AudioMixdown"] = @"Dolby Pro Logic II";
    audioTrack1Array[@"AudioSamplerate"] = @"Auto";
    audioTrack1Array[@"AudioBitrate"] = @"160";
    audioTrack1Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack1Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    audioTrack2Array[@"AudioTrack"] = @1;
    audioTrack2Array[@"AudioEncoder"] = @"AC3 Passthru";
    audioTrack2Array[@"AudioMixdown"] = @"None";
    audioTrack2Array[@"AudioSamplerate"] = @"Auto";
    audioTrack2Array[@"AudioBitrate"] = @"160";
    audioTrack2Array[@"AudioTrackGainSlider"] = @0.0f;
    audioTrack2Array[@"AudioTrackDRCSlider"] = @0.0f;
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    preset[@"AudioList"] = [NSMutableArray arrayWithArray:audioListArray];
    [audioListArray release];
    
    /* Subtitles (note: currently ignored) */
    preset[@"Subtitles"] = @"None";
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

@end
